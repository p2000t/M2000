/******************************************************************************/
/* Public interface for the Philips P2000 floppy extension-board plug-in.      */
/******************************************************************************/

/* The frontend mounts an image with FDC_Init(), or enables an empty board
 * with FDC_SetEnabled(). P2000.c forwards port accesses, machine resets,
 * interrupt polling and RETI notifications through the functions below.
 * All calls belong on the emulator thread; the module owns its file and
 * controller state, while the caller owns CPU registers and RAM mapping.
 * See FDC.c for the command-phase overview and internal helper functions.
 */

#ifndef _FDC_H
#define _FDC_H

#include "Z80.h"

/* Philips extension board: CTC at 88-8B, uPD765 at 8C-8F,
 * control/DRQ latch at 90. No memory or CPU state is owned by this module.
 *
 * Images are raw JWS .dsk files: 40 cylinders, two heads, 16 x 256-byte
 * sectors, cylinder/head/sector order. Sector IDs are C=1..40, R=1..16;
 * SEEK uses physical cylinders 0..39. Drive 1 (765 unit select 1) is A:.
 * Images are opened read/write, with a read-only fallback. Unsupported sizes
 * are rejected; this is not the CPC DSK container format.
 *
 * Transfers and seeks are instantaneous, with DRQ held until serviced.
 * CTC channels 0/1 route completion/not-ready events; channel 3 receives the
 * host's keyboard tick. Rotational timing and general CTC timers are omitted.
 */
/****************************************************************************/
/*** Mount an image and enable the board                                  ***/
/****************************************************************************/
/* imagePath names a raw JWS disk on the host. Return 1 on success, or 0 on
 * failure without disturbing the existing mount or enabled state. Success
 * replaces the previous disk and resets the board; the path is not retained.
 * Completed sector writes update the file in place. The caller is responsible
 * for a machine reset and sufficient banked RAM when booting from the image.
 */
int FDC_Init(const char *imagePath);

/****************************************************************************/
/*** Close the image and disable the board                                ***/
/****************************************************************************/
/* Release the mounted file and clear all controller/CTC state. Safe before
 * initialization and on repeated calls. No buffered partial write is saved.
 */
void FDC_Cleanup(void);

/****************************************************************************/
/*** Reset the board while keeping its media and enabled setting          ***/
/****************************************************************************/
/* Abort the current operation and clear controller, control-latch and CTC
 * state. Call with a machine reset. The guest must release chip reset through
 * port 90 before issuing commands; the image remains mounted.
 */
void FDC_Reset(void);

/****************************************************************************/
/*** Query the host-controlled board-enable setting                       ***/
/****************************************************************************/
/* Return 1 if the board is enabled, otherwise 0. This does not imply that
 * an image is mounted or the emulated drive is ready.
 */
int FDC_IsActive(void);

/****************************************************************************/
/*** Enable or disable the board without ejecting its image               ***/
/****************************************************************************/
/* Any nonzero enabled value enables the board. Changing the setting resets
 * controller/CTC state; repeating the current setting does nothing. The
 * frontend handles machine resets and RAM configuration separately.
 */
void FDC_SetEnabled(int enabled);

/****************************************************************************/
/*** Forward a Z80 port write to the board                                ***/
/****************************************************************************/
/* Pass the low eight bits of the I/O address and the byte written. Handles
 * CTC, control-latch and data-register writes; ignores unhandled ports and
 * all writes when the board is disabled.
 */
void FDC_Out(byte port, byte value);

/****************************************************************************/
/*** Forward a Z80 port read to the board                                 ***/
/****************************************************************************/
/* Pass the low eight bits of the I/O address. Return a status, countdown,
 * sector or result byte as appropriate to the port and controller phase.
 * Return FFh for disabled/unavailable reads. Data reads advance the transfer.
 */
byte FDC_In(byte port);

/****************************************************************************/
/*** Supply a keyboard tick and poll for a CTC interrupt                  ***/
/****************************************************************************/
/* Call once per emulator keyboard tick, with keyboardTick nonzero when the
 * keyboard interrupt source is enabled. Set interruptsEnabled only when the
 * CPU can accept a maskable interrupt. Return Z80_IGNORE_INT if none can be
 * delivered, or an IM2 vector that the caller must deliver immediately.
 * Returning a vector acknowledges it and marks its CTC channel in service;
 * pending events are retained while CPU interrupts are disabled.
 */
int FDC_Interrupt(int interruptsEnabled, int keyboardTick);

/****************************************************************************/
/*** Notify the board that the guest executed RETI                        ***/
/****************************************************************************/
/* Release the highest-priority CTC channel in service, permitting further
 * interrupt delivery. Harmless when no channel is in service.
 */
void FDC_Reti(void);

#endif /* _FDC_H */
