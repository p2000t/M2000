/******************************************************************************/
/* Philips P2000 floppy controller and extension-board interrupt emulation.     */
/******************************************************************************/

/* This module models a NEC uPD765 floppy disk controller (FDC) and the part
 * of the Z80 Counter/Timer Circuit (CTC) used by the Philips extension board.
 * The emulator forwards I/O accesses here; the guest's own Z80 instructions
 * move each disk byte into or out of RAM. This module never accesses RAM or
 * changes CPU registers, including during the monitor's JWS boot sequence.
 *
 * Command flow:
 *   COMMAND: collect an opcode and its parameters through the data port.
 *   READ/WRITE: service one sector-buffer byte per data-port access.
 *   RESULT: return status bytes, then accept the next command.
 * SPECIFY and SEEK/RECALIBRATE return directly to COMMAND. Seek completion
 * is recorded separately until the guest issues SENSE INTERRUPT STATUS.
 *
 * Port map (all addresses are hexadecimal):
 *   88-8B: CTC channels 0-3; count events and select interrupt vectors.
 *   8C/8E: main status register; reports phase and transfer direction.
 *   8D/8F: data register; carries command, sector or result bytes by phase.
 *   90: output control latch; input bit 0 is data request (DRQ).
 * CTC channel 0 receives controller completion events, channel 1 receives
 * not-ready events, and channel 3 receives the emulator's keyboard tick.
 *
 * One raw 320 KiB JWS image backs drive A: (765 unit select 1). All state
 * belongs to the single static fdc instance and is accessed on the emulator
 * thread. A disabled board retains its image but ignores I/O. An enabled
 * board may have no image, in which case disk commands report not ready.
 * Board presence is independent of the guest-controlled reset bit at 90.
 *
 * Seeks and byte availability are immediate: there is no rotational timing,
 * general CTC timer emulation or independent DMA engine. The raw image has
 * no metadata for CRC errors, deleted address marks or arbitrary formatting.
 * Controller state is not included in the emulator's save-state format.
 *
 * Port wiring: Philips P2000 Field Support Manual, figures 3.14 and 4.12.
 * https://electrickery.hosting.philpem.me.uk/comp/p2000c/doc/P2000MT_FSupp.pdf
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "FDC.h"

/* Raw image layout: cylinder, then head, then sector. Physical cylinders
 * start at 0, but JWS stores cylinder IDs C=1..40 and sector IDs R=1..16.
 * The sector size code N is 1, meaning 128 << 1 = 256 bytes per sector. */
#define FDC_CYLINDERS 40
#define FDC_HEADS 2
#define FDC_SECTORS 16
#define FDC_SECTOR_SIZE 256
#define FDC_IMAGE_SIZE (FDC_CYLINDERS * FDC_HEADS * FDC_SECTORS * FDC_SECTOR_SIZE)
#define FDC_DRIVE 1

/* Output latch at port 90. DMA selects the data-transfer path; the guest
 * still moves bytes with IN/OUT instructions. TC (terminal count) ends a
 * transfer early. ENABLE releases controller reset; MOTOR makes a mounted
 * drive ready without modelling spin-up time. */
#define FDC_DMA 0x01
#define FDC_TC 0x02
#define FDC_ENABLE 0x04
#define FDC_MOTOR 0x08

/* uPD765 result bits: ST0 describes completion, ST1 describes disk errors. */
#define FDC_ST0_ABNORMAL 0x40
#define FDC_ST0_NOT_READY 0x48
#define FDC_ST1_NO_DATA 0x04
#define FDC_ST1_WRITE_PROTECT 0x02
#define FDC_ST1_DATA_ERROR 0x20
#define FDC_ST1_END_CYLINDER 0x80

/* Each CTC channel counts external events down from its programmed reload
 * value (a programmed zero means 256). waiting means the next port write
 * supplies that value; pending means a vector awaits CPU acknowledgement;
 * inService means its handler has been entered and has not issued RETI. */
typedef struct {
    byte control;
    unsigned reload, count;
    int waiting, pending, inService;
} FDC_CTC;

typedef enum { FDC_COMMAND, FDC_READ, FDC_WRITE, FDC_RESULT } FDC_Phase;

/* Private state shared by the command parser, transfer engine and CTC.
 * cylinder[] stores physical seek positions for the four possible unit IDs;
 * seekPending is a bit mask of units with status awaiting a SENSE command.
 * Only FDC_DRIVE has an image; the remaining units can report not ready. */
static struct {
    FILE *image;
    int readOnly, enabled;
    byte control, cylinder[4], seekStatus[4], seekPending;
    /* Buffers hold one command, one result packet and one sector at a time. */
    byte command[9], result[7], sector[FDC_SECTOR_SIZE];
    unsigned commandCount, commandSize, resultCount, resultIndex, dataIndex;
    /* Current sector ID: cylinder/head/record/size code (C/H/R/N).
     * unit also carries the selected head in bit 2; eot is the last sector
     * number requested on a track. dataIndex is the next byte in sector[]. */
    byte c, h, r, n, unit, eot;
    int multiTrack, nonDMA, readTrack;
    FDC_Phase phase;
    byte vector;
    FDC_CTC ctc[4];
} fdc;

/****************************************************************************/
/*** Count one external event on a CTC channel                            ***/
/****************************************************************************/
/* channel is an internal channel index (0..3). Ignore events until that
 * channel has a count loaded and interrupts enabled. When its count expires,
 * reload it and latch a pending interrupt. Delivery is deferred until
 * FDC_Interrupt(), so this function never interrupts the CPU directly.
 */
static void FDC_Event(unsigned channel) {
    FDC_CTC *ctc = &fdc.ctc[channel];
    if (!(ctc->control & 0x80) || ctc->waiting || !ctc->count) return;
    if (!--ctc->count) {
        ctc->count = ctc->reload;
        ctc->pending = 1;
    }
}

/****************************************************************************/
/*** Reset the floppy chip without resetting the whole board              ***/
/****************************************************************************/
/* Abort any command or transfer, discard unread results and seek status,
 * and reset physical head positions and transfer mode. Also discard pending
 * disk events on CTC channels 0/1. Keep the image, board-enable setting,
 * control latch and CTC programming; port 90 uses this narrower reset.
 */
static void FDC_ControllerReset(void) {
    fdc.phase = FDC_COMMAND;
    fdc.commandCount = fdc.resultCount = fdc.resultIndex = fdc.dataIndex = 0;
    fdc.seekPending = 0;
    fdc.nonDMA = 0;
    memset(fdc.cylinder, 0, sizeof(fdc.cylinder));
    fdc.ctc[0].pending = fdc.ctc[1].pending = 0;
}

/****************************************************************************/
/*** Reset controller, control latch and CTC state                        ***/
/****************************************************************************/
/* Used on machine reset and when board presence changes. Retain the mounted
 * image, its write-protection state and the host's enabled setting. Clearing
 * the control latch holds the floppy chip in reset until the guest sets the
 * ENABLE bit at port 90. Buffered partial writes are abandoned.
 */
void FDC_Reset(void) {
    FDC_ControllerReset();
    fdc.control = fdc.vector = 0;
    memset(fdc.ctc, 0, sizeof(fdc.ctc));
}

/****************************************************************************/
/*** Report whether the extension board is enabled                        ***/
/****************************************************************************/
/* Return 1 for an enabled board, or 0 for a disabled board. This is a board
 * presence test, not a disk-ready test: an enabled board can have no image,
 * its motor stopped, or its controller held in reset.
 */
int FDC_IsActive(void) {
    return fdc.enabled;
}

/****************************************************************************/
/*** Change board presence while retaining mounted media                  ***/
/****************************************************************************/
/* Treat any nonzero enabled value as true. A change resets controller and
 * CTC state; setting the existing value does nothing. Disabling the board
 * keeps its image open, so re-enabling it restores access to the same disk.
 * The caller handles any machine reset or RAM expansion needed by the guest.
 */
void FDC_SetEnabled(int enabled) {
    if (fdc.enabled != !!enabled) {
        fdc.enabled = !!enabled;
        FDC_Reset();
    }
}

/****************************************************************************/
/*** Close the mounted image and release all board state                  ***/
/****************************************************************************/
/* Disable the board and clear its controller/CTC state after closing the
 * image. Safe to call before initialization or more than once. Completed
 * sector writes were already flushed by FDC_Out(); partial writes are lost.
 */
void FDC_Cleanup(void) {
    if (fdc.image) fclose(fdc.image);
    memset(&fdc, 0, sizeof(fdc));
}

/****************************************************************************/
/*** Mount a raw JWS image and enable the controller                      ***/
/****************************************************************************/
/* imagePath is a host filename. Try read/write access first, then read-only
 * access, and validate the exact supported image size before replacing the
 * current disk. Return 1 on success; return 0 on a null path, open failure
 * or unsupported size, leaving the previous image and board state intact.
 *
 * On success this module owns the open file until replacement or cleanup;
 * it does not retain imagePath. Read-only access becomes guest-visible write
 * protection. The board is reset and enabled, but the caller must reset the
 * machine separately if the new disk should be booted.
 */
int FDC_Init(const char *imagePath) {
    FILE *image;
    long size;
    int readOnly = 0;
    if (!imagePath) return 0;
    image = fopen(imagePath, "r+b");
    if (!image) {
        image = fopen(imagePath, "rb");
        readOnly = 1;
    }
    if (!image) {
        fprintf(stderr, "Failed to open floppy image '%s': %s\n", imagePath, strerror(errno));
        return 0;
    }
    size = fseek(image, 0, SEEK_END) == 0 ? ftell(image) : -1;
    if (size != FDC_IMAGE_SIZE) {
        fprintf(stderr, "Invalid floppy image '%s': expected a raw 327680-byte JWS disk\n", imagePath);
        fclose(image);
        return 0;
    }
    FDC_Cleanup();
    fdc.image = image;
    fdc.readOnly = readOnly;
    fdc.enabled = 1;
    FDC_Reset();
    return 1;
}

/****************************************************************************/
/*** Expose a prepared result packet to the guest                         ***/
/****************************************************************************/
/* The caller must fill result[0..count-1] first; count is at most seven.
 * Switch to RESULT, rewind the result cursor and clear the command parser.
 * If interrupt is nonzero, signal completion through CTC channel 0. DRQ
 * falls because the controller is no longer in a sector-transfer phase.
 */
static void FDC_Result(unsigned count, int interrupt) {
    fdc.phase = FDC_RESULT;
    fdc.resultIndex = 0;
    fdc.resultCount = count;
    fdc.commandCount = 0;
    if (interrupt) FDC_Event(0);
}

/****************************************************************************/
/*** Complete a disk command with status and a sector ID                  ***/
/****************************************************************************/
/* Build the seven-byte result ST0, ST1, ST2, C, H, R, N from the supplied
 * status bits and current transfer position. Merge the selected drive/head
 * into ST0, enter RESULT and signal completion through CTC channel 0.
 * Used for both successful completion and errors; no data bytes move here.
 */
static void FDC_Finish(byte st0, byte st1, byte st2) {
    fdc.result[0] = st0 | (fdc.unit & 3) | ((fdc.h & 1) << 2);
    fdc.result[1] = st1;
    fdc.result[2] = st2;
    fdc.result[3] = fdc.c;
    fdc.result[4] = fdc.h;
    fdc.result[5] = fdc.r;
    fdc.result[6] = fdc.n;
    FDC_Result(7, 1);
}

/****************************************************************************/
/*** Check whether the selected drive can transfer disk data              ***/
/****************************************************************************/
/* Return nonzero only when an image is mounted, the command selects drive A:
 * and the motor bit is set. The caller has already checked board/controller
 * enablement. Other unit IDs and an empty drive report not ready.
 */
static int FDC_Ready(void) {
    return fdc.image && (fdc.unit & 3) == FDC_DRIVE && (fdc.control & FDC_MOTOR);
}

/****************************************************************************/
/*** Translate the current JWS sector ID to an image offset               ***/
/****************************************************************************/
/* Return a byte offset in the raw cylinder/head/sector-ordered image.
 * Subtract one from C and R because JWS IDs start at 1; H starts at 0.
 * The caller must have validated the geometry and seek position before
 * using this offset. The formula assumes fixed 256-byte sectors.
 */
static long FDC_Offset(void) {
    return (((long)fdc.c - 1) * FDC_HEADS + fdc.h) *
           FDC_SECTORS * FDC_SECTOR_SIZE + (fdc.r - 1) * FDC_SECTOR_SIZE;
}

/****************************************************************************/
/*** Read the current sector into the transfer buffer                     ***/
/****************************************************************************/
/* Require a mounted image and a validated C/H/R position. Return 1 after
 * loading all 256 bytes and resetting dataIndex to zero. On seek/read
 * failure, enter an error RESULT phase and return 0. Both read and write
 * commands load this buffer; writes replace bytes in it before committing
 * a complete sector back to the image.
 */
static int FDC_LoadSector(void) {
    if (fseek(fdc.image, FDC_Offset(), SEEK_SET) ||
        fread(fdc.sector, 1, sizeof(fdc.sector), fdc.image) != sizeof(fdc.sector)) {
        FDC_Finish(FDC_ST0_ABNORMAL, FDC_ST1_DATA_ERROR, 0);
        return 0;
    }
    fdc.dataIndex = 0;
    return 1;
}

/****************************************************************************/
/*** Advance after transferring a complete sector                         ***/
/****************************************************************************/
/* Continue at the next sector up to EOT (end of track). With the multi-track
 * flag set, finishing head 0 continues at sector 1 on head 1 of the same
 * cylinder. Load the next sector, or finish the command when no sectors
 * remain. This function never seeks to the next physical cylinder.
 * READ TRACK completes normally at EOT; READ/WRITE DATA report end of
 * cylinder if the guest has not terminated the transfer using TC.
 */
static void FDC_NextSector(void) {
    if (fdc.r < fdc.eot) {
        ++fdc.r;
    } else if (fdc.multiTrack && !fdc.h) {
        fdc.h = 1;
        fdc.r = 1;
    } else {
        /* Reaching EOT without terminal count is an end-of-cylinder result.
         * READ TRACK (used by the monitor) completes normally at EOT. */
        FDC_Finish(fdc.readTrack ? 0 : FDC_ST0_ABNORMAL,
                   fdc.readTrack ? 0 : FDC_ST1_END_CYLINDER, 0);
        return;
    }
    FDC_LoadSector();
}

/****************************************************************************/
/*** Decode and start a fully collected uPD765 command                    ***/
/****************************************************************************/
/* FDC_Out() calls this only after collecting the opcode's required bytes.
 * The low five opcode bits select the command; upper bits carry modifiers.
 * Status commands prepare immediate results, SPECIFY changes transfer mode,
 * and SEEK/RECALIBRATE record completion for SENSE INTERRUPT STATUS.
 *
 * Data commands validate readiness, C/H/R/N, EOT, physical seek position and
 * write protection before accessing the image. Valid transfers enter READ
 * or WRITE with the first sector buffered; READ ID returns metadata only.
 * Invalid requests produce status results rather than host memory accesses.
 * Unsupported opcodes produce the one-byte invalid-command result (80h).
 */
static void FDC_Execute(void) {
    byte op = fdc.command[0] & 0x1F;
    unsigned drive = fdc.command[1] & 3;
    fdc.commandCount = 0;
    switch (op) {
        case 0x03: /* SPECIFY */
            fdc.nonDMA = fdc.command[2] & 1;
            break;
        case 0x04: /* SENSE DRIVE STATUS */
            fdc.result[0] = (fdc.command[1] & 7) | 0x08;
            if (!fdc.cylinder[drive]) fdc.result[0] |= 0x10;
            if (drive == FDC_DRIVE && fdc.image) {
                if (fdc.control & FDC_MOTOR) fdc.result[0] |= 0x20;
                if (fdc.readOnly) fdc.result[0] |= 0x40;
            }
            FDC_Result(1, 0);
            break;
        case 0x07: /* RECALIBRATE */
        case 0x0F: /* SEEK: physical cylinder is zero-based. */
            fdc.cylinder[drive] = op == 7 ? 0 : fdc.command[2];
            fdc.seekStatus[drive] = 0x20 | (fdc.command[1] & 7);
            if (drive != FDC_DRIVE || !fdc.image) fdc.seekStatus[drive] |= FDC_ST0_NOT_READY;
            fdc.seekPending |= 1 << drive;
            FDC_Event(0);
            break;
        case 0x08: /* SENSE INTERRUPT STATUS */
            for (drive = 0; drive < 4; ++drive) {
                if (fdc.seekPending & (1 << drive)) break;
            }
            if (drive == 4) {
                fdc.result[0] = 0x80;
                FDC_Result(1, 0);
            } else {
                fdc.result[0] = fdc.seekStatus[drive];
                fdc.result[1] = fdc.cylinder[drive];
                fdc.seekPending &= ~(1 << drive);
                FDC_Result(2, 0);
            }
            break;
        case 0x02: /* READ TRACK */
        case 0x05: /* WRITE DATA */
        case 0x06: /* READ DATA */
        case 0x0A: /* READ ID */
            fdc.unit = fdc.command[1] & 7;
            fdc.c = fdc.command[2];
            fdc.h = fdc.command[3];
            fdc.r = fdc.command[4];
            fdc.n = fdc.command[5];
            fdc.eot = fdc.command[6];
            fdc.multiTrack = fdc.command[0] & 0x80;
            fdc.readTrack = op == 2;
            if (op == 0x0A) {
                fdc.c = fdc.cylinder[drive] + 1;
                fdc.h = (fdc.unit >> 2) & 1;
                fdc.r = 1;
                fdc.n = 1;
            }
            if (!FDC_Ready()) {
                FDC_Finish(FDC_ST0_NOT_READY, 0, 0);
                FDC_Event(1);
            } else if (!fdc.c || fdc.c > FDC_CYLINDERS ||
                       fdc.h >= FDC_HEADS || fdc.h != ((fdc.unit >> 2) & 1) ||
                       !fdc.r || fdc.r > FDC_SECTORS || fdc.n != 1 ||
                       fdc.c != fdc.cylinder[drive] + 1 ||
                       (op != 0x0A && (fdc.eot < fdc.r || fdc.eot > FDC_SECTORS))) {
                FDC_Finish(FDC_ST0_ABNORMAL, FDC_ST1_NO_DATA, 0);
            } else if (op == 0x0A) {
                FDC_Finish(0, 0, 0);
            } else if (op == 5 && fdc.readOnly) {
                FDC_Finish(FDC_ST0_ABNORMAL, FDC_ST1_WRITE_PROTECT, 0);
            } else {
                fdc.phase = op == 5 ? FDC_WRITE : FDC_READ;
                FDC_LoadSector();
            }
            break;
        default:
            /* Unsupported commands return the 765 invalid-command result. */
            fdc.result[0] = 0x80;
            FDC_Result(1, 0);
            break;
    }
}

/****************************************************************************/
/*** Decode a write to one of the four CTC channel ports                  ***/
/****************************************************************************/
/* port must be 88h..8Bh. If a time constant was requested, this byte loads
 * the countdown (zero means 256). Otherwise bit 0 selects a control word:
 * bit 1 resets the channel, bit 2 requests a count byte, and bit 7 enables
 * interrupts. An even byte on channel 0 sets the shared vector base.
 * Only externally supplied events are counted; timer mode is not simulated.
 */
static void FDC_CTCOut(byte port, byte value) {
    unsigned channel = port & 3;
    FDC_CTC *ctc = &fdc.ctc[channel];
    if (ctc->waiting) {
        ctc->reload = ctc->count = value ? value : 256;
        ctc->waiting = 0;
    } else if (value & 1) {
        ctc->control = value;
        if (value & 2) {
            ctc->pending = ctc->inService = 0;
            ctc->count = 0;
        }
        ctc->waiting = !!(value & 4);
        if (!(value & 0x80)) ctc->pending = 0;
    } else if (!channel) {
        fdc.vector = value & 0xF8;
    }
}

/****************************************************************************/
/*** Handle an emulated write to the extension-board ports                ***/
/****************************************************************************/
/* port is the low byte of the Z80 I/O address; value is the byte written.
 * Ignore writes while the host has disabled the board. Route CTC writes to
 * FDC_CTCOut(), and control-latch writes to reset/terminal-count handling.
 *
 * At the data port, COMMAND collects bytes until FDC_Execute() can run.
 * WRITE accepts sector bytes only through the selected transfer path.
 * Commit and flush each complete sector before advancing; host I/O failures
 * become disk-error results. TC finishes an active transfer immediately,
 * discarding any partial write. Writes in other phases/ports are ignored.
 */
void FDC_Out(byte port, byte value) {
    if (!fdc.enabled) return;
    if (port >= 0x88 && port <= 0x8B) {
        FDC_CTCOut(port, value);
        return;
    }
    if (port == 0x90) {
        byte previous = fdc.control;
        fdc.control = value & 0x0F;
        if (!(value & FDC_ENABLE)) {
            FDC_ControllerReset();
        } else if (!(previous & FDC_ENABLE)) {
            /* Reset completion is collected by SENSE INTERRUPT STATUS. */
            unsigned drive;
            for (drive = 0; drive < 4; ++drive) fdc.seekStatus[drive] = 0xC0 | drive;
            fdc.seekPending = 0x0F;
            FDC_Event(0);
        } else if ((value & FDC_TC) && !(previous & FDC_TC) &&
                   (fdc.phase == FDC_READ || fdc.phase == FDC_WRITE)) {
            /* A partial write is discarded; only complete sectors are saved. */
            FDC_Finish(0, 0, 0);
        }
        return;
    }
    if ((port != 0x8D && port != 0x8F) || !(fdc.control & FDC_ENABLE)) return;
    if (fdc.phase == FDC_WRITE) {
        if (!(fdc.control & FDC_DMA) && !fdc.nonDMA) return;
        fdc.sector[fdc.dataIndex++] = value;
        if (fdc.dataIndex == sizeof(fdc.sector)) {
            if (fseek(fdc.image, FDC_Offset(), SEEK_SET) ||
                fwrite(fdc.sector, 1, sizeof(fdc.sector), fdc.image) != sizeof(fdc.sector) ||
                fflush(fdc.image)) {
                FDC_Finish(FDC_ST0_ABNORMAL, FDC_ST1_DATA_ERROR, 0);
            } else FDC_NextSector();
        }
    } else if (fdc.phase == FDC_COMMAND && !(fdc.control & FDC_DMA)) {
        if (!fdc.commandCount) {
            static const byte sizes[32] = {
                1,1,9,3,2,9,9,2,1,1,2,1,1,1,1,3,
                1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
            };
            memset(fdc.command, 0, sizeof(fdc.command));
            fdc.commandSize = sizes[value & 0x1F];
        }
        fdc.command[fdc.commandCount++] = value;
        if (fdc.commandCount == fdc.commandSize) FDC_Execute();
    }
}

/****************************************************************************/
/*** Return status, sector data or result bytes for an I/O read           ***/
/****************************************************************************/
/* port is the low byte of the Z80 I/O address. Return FFh for a disabled
 * board, an unavailable register or a read in the wrong transfer direction.
 * CTC ports return countdown values; port 90 reports DRQ while transferring.
 *
 * Main-status bits are RQM (80h, register ready), DIO (40h, controller to
 * CPU), execution/non-DMA (20h), and controller busy (10h). In DMA mode the
 * guest polls DRQ for sector bytes rather than RQM. A data-port read advances
 * one byte only: the last sector byte advances/finishes the transfer, and
 * the last result byte returns the parser to COMMAND.
 */
byte FDC_In(byte port) {
    byte value;
    if (!fdc.enabled) return 0xFF;
    if (port >= 0x88 && port <= 0x8B) return fdc.ctc[port & 3].count & 0xFF;
    if (port == 0x90) {
        return (fdc.control & FDC_ENABLE) &&
               (fdc.phase == FDC_READ || fdc.phase == FDC_WRITE) ? 1 : 0;
    }
    if (!(fdc.control & FDC_ENABLE)) return 0xFF;
    if (port == 0x8C || port == 0x8E) {
        switch (fdc.phase) {
            case FDC_COMMAND: return 0x80 | (fdc.commandCount ? 0x10 : 0);
            case FDC_RESULT: return 0xD0;
            case FDC_READ: return fdc.nonDMA ? 0xF0 : 0x50;
            case FDC_WRITE: return fdc.nonDMA ? 0xB0 : 0x10;
        }
    }
    if (port != 0x8D && port != 0x8F) return 0xFF;
    if (fdc.phase == FDC_RESULT && !(fdc.control & FDC_DMA)) {
        value = fdc.result[fdc.resultIndex++];
        if (fdc.resultIndex == fdc.resultCount) fdc.phase = FDC_COMMAND;
        return value;
    }
    if (fdc.phase == FDC_READ && ((fdc.control & FDC_DMA) || fdc.nonDMA)) {
        value = fdc.sector[fdc.dataIndex++];
        if (fdc.dataIndex == sizeof(fdc.sector)) FDC_NextSector();
        return value;
    }
    return 0xFF;
}

/****************************************************************************/
/*** Poll CTC events and acknowledge the next eligible interrupt          ***/
/****************************************************************************/
/* Call once per emulator keyboard tick. A nonzero keyboardTick supplies an
 * event to CTC channel 3. interruptsEnabled must reflect whether the CPU
 * can accept a maskable interrupt now; when false, pending events remain
 * latched and the function returns Z80_IGNORE_INT.
 *
 * Channels are checked in priority order, 0 highest. An in-service channel
 * blocks itself and lower priorities, but permits higher-priority nesting.
 * Return the vector base plus twice the selected channel, marking it in
 * service, or Z80_IGNORE_INT if none is eligible. The caller must deliver a
 * returned vector to the CPU and forward its eventual RETI to FDC_Reti().
 */
int FDC_Interrupt(int interruptsEnabled, int keyboardTick) {
    unsigned channel;
    if (!fdc.enabled) return Z80_IGNORE_INT;
    if (keyboardTick) FDC_Event(3);
    if (!interruptsEnabled) return Z80_IGNORE_INT;
    for (channel = 0; channel < 4; ++channel) {
        FDC_CTC *ctc = &fdc.ctc[channel];
        if (ctc->inService) break;
        if (ctc->pending) {
            ctc->pending = 0;
            ctc->inService = 1;
            return fdc.vector + channel * 2;
        }
    }
    return Z80_IGNORE_INT;
}

/****************************************************************************/
/*** Release the highest-priority interrupt currently in service          ***/
/****************************************************************************/
/* Called by the emulator when the guest executes RETI. Clear the first
 * in-service channel to allow its own and lower-priority pending events to
 * be delivered on a later poll. With nested interrupts this releases the
 * most recently accepted, higher-priority handler. Do nothing if none is
 * in service; this does not clear seek status awaiting a SENSE command.
 */
void FDC_Reti(void) {
    unsigned channel;
    for (channel = 0; channel < 4; ++channel) {
        if (fdc.ctc[channel].inService) {
            fdc.ctc[channel].inService = 0;
            break;
        }
    }
}
