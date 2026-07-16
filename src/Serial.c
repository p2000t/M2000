// GPIO-serial (bit-bang RS-232) emulation module.
//
// TX STATE MACHINE  (Serial_TxPortWrite)
// ----------------------------------------
// The Z80 code writes port 0x10 once per bit period (timing depends on the
// configured baud rate, e.g. ~1040 T-states at 1200 baud / 2.5 MHz). Bit 7
// of the written byte carries the serial data with inverted logic: bit7 SET
// = logical 0 (space); bit7 CLEAR = logical 1 (mark). This module detects
// the start-bit edge, accumulates 8 data bits (LSB first), validates the
// stop bit, and writes the complete byte to the host COM port.
//
// On Windows, TX bytes are queued to a ring buffer and drained by a
// background writer thread using overlapped (asynchronous) I/O, so a
// completed byte never blocks the Z80 emulation thread. On POSIX, a plain
// write() is used directly since read()/write() on the same fd do not
// block one another the way synchronous ReadFile()/WriteFile() calls on
// the same Windows HANDLE can.
//
// RX STATE MACHINE  (Serial_RxPortRead)
// ----------------------------------------
// A background thread reads bytes from the host COM port and queues them.
// Serial_RxPortRead() is called by the Z80 code at the same per-bit
// cadence (one call per bit period). It dequeues bytes from the ring
// buffer and shifts them out bit by bit. Non-inverted logic is used for
// port 0x20 bit 0: 1 = mark (idle/stop), 0 = space (start).
// The state machine advances exactly one bit per call, so the Z80 code
// drives the timing naturally — no independent timer is required.

#include "Serial.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* -----------------------------------------------------------------------
 * Platform abstraction
 * --------------------------------------------------------------------- */
#ifdef _WIN32
#  include <windows.h>
typedef HANDLE serial_fd_t;
#  define SERIAL_INVALID INVALID_HANDLE_VALUE
#else
#  include <pthread.h>
#  include <termios.h>
#  include <fcntl.h>
#  include <unistd.h>
#  include <sys/select.h>
#  include <errno.h>
typedef int serial_fd_t;
#  define SERIAL_INVALID (-1)
#endif

/* -----------------------------------------------------------------------
 * RX ring buffer  (single-producer / single-consumer, no mutex needed)
 * --------------------------------------------------------------------- */
#define RX_BUF_SIZE 256
static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile unsigned rx_head = 0;   /* written by RX background thread */
static volatile unsigned rx_tail = 0;   /* read by main (Z80) thread       */

static inline int rx_buf_empty(void) { return rx_head == rx_tail; }

static void rx_buf_push(uint8_t b)
{
    unsigned next = (rx_head + 1) % RX_BUF_SIZE;
    if (next != rx_tail) {          /* drop silently when full */
        rx_buf[rx_head] = b;
        rx_head = next;
    }
}

static uint8_t rx_buf_pop(void)
{
    uint8_t b = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return b;
}

#ifdef _WIN32
/* -----------------------------------------------------------------------
 * TX ring buffer (Windows only — POSIX writes synchronously, see below)
 * single-producer (Z80 thread) / single-consumer (TX writer thread)
 * --------------------------------------------------------------------- */
#define TX_BUF_SIZE 256
static volatile uint8_t  tx_buf[TX_BUF_SIZE];
static volatile unsigned tx_head = 0;   /* written by main (Z80) thread   */
static volatile unsigned tx_tail = 0;   /* read by TX background thread   */

static inline int tx_buf_empty(void) { return tx_head == tx_tail; }

static void tx_buf_push(uint8_t b)
{
    unsigned next = (tx_head + 1) % TX_BUF_SIZE;
    if (next != tx_tail) {          /* drop silently when full */
        tx_buf[tx_head] = b;
        tx_head = next;
    }
}

static int tx_buf_pop(uint8_t *out)
{
    if (tx_buf_empty()) return 0;
    *out = tx_buf[tx_tail];
    tx_tail = (tx_tail + 1) % TX_BUF_SIZE;
    return 1;
}
#endif

/* -----------------------------------------------------------------------
 * Shared state
 * --------------------------------------------------------------------- */
static serial_fd_t  fd             = SERIAL_INVALID;
static volatile int serial_running = 0;

#ifdef _WIN32
static HANDLE rx_thread_handle;
static HANDLE tx_thread_handle;
static HANDLE ov_read_event;     /* signalled when an overlapped read completes  */
static HANDLE ov_write_event;    /* signalled when an overlapped write completes */
static HANDLE tx_data_event;     /* signalled when new TX data is queued         */
static HANDLE shutdown_event;    /* signalled to wake blocked threads for exit    */
#else
static pthread_t rx_thread_handle;
#endif

/* -----------------------------------------------------------------------
 * TX state machine
 * --------------------------------------------------------------------- */
typedef struct {
    int     active;     /* 1 while a frame is being received */
    int     bit_count;  /* 0-7: data bit position            */
    uint8_t shift_reg;  /* accumulates data bits, LSB first  */
} TxState;

static TxState tx_state;

/* -----------------------------------------------------------------------
 * RX state machine
 * --------------------------------------------------------------------- */
typedef struct {
    uint8_t current_byte;   /* byte being shifted out to the Z80 */
    int     bit_count;      /* 0 = start bit, 1-8 = data, 9 = stop */
    int     idle;           /* 1 when no frame is in progress       */
} RxState;

static RxState rx_state;

/* -----------------------------------------------------------------------
 * Low-level write to host COM port
 *  - Windows: enqueue for the background TX writer thread (overlapped I/O)
 *  - POSIX:   write directly; read()/write() on the same fd don't block
 *             each other, so no queue/thread is needed here.
 * --------------------------------------------------------------------- */
static void host_write_byte(uint8_t b)
{
#ifdef _WIN32
    tx_buf_push(b);
    SetEvent(tx_data_event);
#else
    ssize_t n;
    do {
        n = write(fd, &b, 1);
    } while (n < 0 && errno == EINTR);
#endif
}

/* -----------------------------------------------------------------------
 * RX background thread — reads bytes from host COM port, pushes to ring
 * --------------------------------------------------------------------- */
#ifdef _WIN32
static DWORD WINAPI rx_thread_func(LPVOID arg)
{
    (void)arg;
    uint8_t b;

    while (serial_running) {
        DWORD      bytes_read = 0;
        OVERLAPPED ov;
        memset(&ov, 0, sizeof(ov));
        ov.hEvent = ov_read_event;
        ResetEvent(ov_read_event);

        if (ReadFile(fd, &b, 1, &bytes_read, &ov)) {
            if (bytes_read == 1) rx_buf_push(b);
            continue;
        }

        if (GetLastError() != ERROR_IO_PENDING) {
            Sleep(10);   /* real error: back off briefly, avoid a busy loop */
            continue;
        }

        HANDLE waitHandles[2] = { ov_read_event, shutdown_event };
        DWORD  w = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        if (w == WAIT_OBJECT_0) {
            if (GetOverlappedResult(fd, &ov, &bytes_read, FALSE) && bytes_read == 1)
                rx_buf_push(b);
        } else {
            /* Shutdown requested while a read was pending: cancel it and exit. */
            CancelIoEx(fd, &ov);
            break;
        }
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * TX background thread (Windows) — drains the TX ring buffer in batches
 * and writes them asynchronously, so a byte write can never block the
 * RX read (or vice versa) on the same HANDLE.
 * --------------------------------------------------------------------- */
static DWORD WINAPI tx_thread_func(LPVOID arg)
{
    (void)arg;
    static uint8_t batch[TX_BUF_SIZE];

    while (serial_running) {
        HANDLE waitHandles[2] = { tx_data_event, shutdown_event };
        DWORD  w = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        if (w != WAIT_OBJECT_0)
            break;   /* shutdown requested */

        unsigned n = 0;
        uint8_t  b;
        while (n < TX_BUF_SIZE && tx_buf_pop(&b))
            batch[n++] = b;
        if (n == 0)
            continue;

        unsigned offset = 0;
        while (offset < n && serial_running) {
            DWORD      written = 0;
            OVERLAPPED ov;
            memset(&ov, 0, sizeof(ov));
            ov.hEvent = ov_write_event;
            ResetEvent(ov_write_event);

            BOOL ok = WriteFile(fd, batch + offset, n - offset, &written, &ov);
            if (!ok) {
                if (GetLastError() == ERROR_IO_PENDING) {
                    HANDLE waitHandles2[2] = { ov_write_event, shutdown_event };
                    DWORD  w2 = WaitForMultipleObjects(2, waitHandles2, FALSE, INFINITE);
                    if (w2 == WAIT_OBJECT_0) {
                        if (!GetOverlappedResult(fd, &ov, &written, FALSE))
                            written = 0;
                    } else {
                        CancelIoEx(fd, &ov);
                        break;
                    }
                } else {
                    break;   /* real write error: drop the rest of this batch */
                }
            }
            if (written == 0) break;   /* avoid spinning on a stalled write */
            offset += written;
        }
    }
    return 0;
}
#else
static void *rx_thread_func(void *arg)
{
    (void)arg;
    uint8_t b;
    while (serial_running) {
        /* Use select with a 100 ms timeout so we can poll serial_running. */
        fd_set rfds;
        struct timeval tv = {0, 100000};
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        int ret = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0) {
            ssize_t n = read(fd, &b, 1);
            if (n == 1)
                rx_buf_push(b);
        }
    }
    return NULL;
}

/* Map an integer baud rate to a POSIX termios speed_t constant. */
static speed_t baud_to_speed(int baud)
{
    switch (baud) {
        case 1200:   return B1200;
        case 2400:   return B2400;
        case 4800:   return B4800;
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        default:
            fprintf(stderr, "Serial: unsupported baud rate %d, defaulting to 1200\n", baud);
            return B1200;
    }
}
#endif

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

int Serial_Init(const char *device, int baud)
{
#ifdef _WIN32
    /* ---- Open COM port for overlapped (asynchronous) I/O ---- */
    fd = CreateFileA(device,
                     GENERIC_READ | GENERIC_WRITE,
                     0,          /* exclusive access */
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     NULL);
    if (fd == SERIAL_INVALID) {
        fprintf(stderr, "Serial: cannot open %s (Windows error %lu)\n",
                device, (unsigned long)GetLastError());
        return 0;
    }

    /* ---- Configure baud/8N1, no flow control ---- */
    DCB dcb;
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(fd, &dcb)) {
        CloseHandle(fd); fd = SERIAL_INVALID;
        fprintf(stderr, "Serial: GetCommState failed\n");
        return 0;
    }
    dcb.BaudRate        = (DWORD)baud;
    dcb.ByteSize        = 8;
    dcb.Parity          = NOPARITY;
    dcb.StopBits        = ONESTOPBIT;
    dcb.fBinary         = TRUE;
    dcb.fParity         = FALSE;
    dcb.fOutxCtsFlow    = FALSE;
    dcb.fOutxDsrFlow    = FALSE;
    dcb.fDtrControl     = DTR_CONTROL_DISABLE;
    dcb.fRtsControl     = RTS_CONTROL_DISABLE;
    dcb.fOutX           = FALSE;
    dcb.fInX            = FALSE;
    if (!SetCommState(fd, &dcb)) {
        CloseHandle(fd); fd = SERIAL_INVALID;
        fprintf(stderr, "Serial: SetCommState failed\n");
        return 0;
    }

    /* ---- No artificial timeouts: overlapped I/O + events handle waiting
     *      and shutdown signalling, so let ReadFile/WriteFile block until
     *      completion or cancellation. ---- */
    COMMTIMEOUTS timeouts;
    memset(&timeouts, 0, sizeof(timeouts));
    SetCommTimeouts(fd, &timeouts);

    /* ---- Create synchronisation objects ---- */
    ov_read_event  = CreateEvent(NULL, TRUE,  FALSE, NULL);  /* manual-reset */
    ov_write_event = CreateEvent(NULL, TRUE,  FALSE, NULL);  /* manual-reset */
    tx_data_event  = CreateEvent(NULL, FALSE, FALSE, NULL);  /* auto-reset   */
    shutdown_event = CreateEvent(NULL, TRUE,  FALSE, NULL);  /* manual-reset */
    if (!ov_read_event || !ov_write_event || !tx_data_event || !shutdown_event) {
        fprintf(stderr, "Serial: CreateEvent failed\n");
        CloseHandle(fd); fd = SERIAL_INVALID;
        return 0;
    }

    /* ---- Start RX and TX threads ---- */
    serial_running   = 1;
    rx_thread_handle = CreateThread(NULL, 0, rx_thread_func, NULL, 0, NULL);
    tx_thread_handle = CreateThread(NULL, 0, tx_thread_func, NULL, 0, NULL);
    if (!rx_thread_handle || !tx_thread_handle) {
        serial_running = 0;
        SetEvent(shutdown_event);
        if (rx_thread_handle) { WaitForSingleObject(rx_thread_handle, 2000); CloseHandle(rx_thread_handle); }
        if (tx_thread_handle) { WaitForSingleObject(tx_thread_handle, 2000); CloseHandle(tx_thread_handle); }
        CloseHandle(ov_read_event);
        CloseHandle(ov_write_event);
        CloseHandle(tx_data_event);
        CloseHandle(shutdown_event);
        CloseHandle(fd); fd = SERIAL_INVALID;
        fprintf(stderr, "Serial: CreateThread failed\n");
        return 0;
    }

    tx_head = tx_tail = 0;

#else /* POSIX */
    /* ---- Open serial device ---- */
    fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == SERIAL_INVALID) {
        fprintf(stderr, "Serial: cannot open %s: %s\n", device, strerror(errno));
        return 0;
    }

    /* ---- Configure baud/8N1, no flow control ---- */
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd); fd = SERIAL_INVALID;
        fprintf(stderr, "Serial: tcgetattr failed: %s\n", strerror(errno));
        return 0;
    }
    cfmakeraw(&tty);
    speed_t speed = baud_to_speed(baud);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    tty.c_cflag &= (tcflag_t)~CSTOPB;    /* 1 stop bit   */
    tty.c_cflag &= (tcflag_t)~CRTSCTS;   /* no hw flow   */
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cc[VMIN]  = 0;   /* non-blocking read (select handles blocking) */
    tty.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd); fd = SERIAL_INVALID;
        fprintf(stderr, "Serial: tcsetattr failed: %s\n", strerror(errno));
        return 0;
    }

    /* ---- Start RX thread ---- */
    serial_running = 1;
    if (pthread_create(&rx_thread_handle, NULL, rx_thread_func, NULL) != 0) {
        serial_running = 0;
        close(fd); fd = SERIAL_INVALID;
        fprintf(stderr, "Serial: pthread_create failed\n");
        return 0;
    }
#endif

    /* Reset state machines */
    memset(&tx_state, 0, sizeof(tx_state));
    rx_state.idle         = 1;
    rx_state.bit_count    = 0;
    rx_state.current_byte = 0;

    rx_head = rx_tail = 0;

    fprintf(stdout, "Serial: opened %s at %d 8N1\n", device, baud);
    return 1;
}

void Serial_Shutdown(void)
{
    if (fd == SERIAL_INVALID) return;

    /* Signal all background threads to exit and wait for them */
    serial_running = 0;

#ifdef _WIN32
    SetEvent(shutdown_event);
    CancelIoEx(fd, NULL);   /* cancel any pending overlapped I/O on this handle */

    WaitForSingleObject(rx_thread_handle, 2000);
    WaitForSingleObject(tx_thread_handle, 2000);
    CloseHandle(rx_thread_handle);
    CloseHandle(tx_thread_handle);
    CloseHandle(ov_read_event);
    CloseHandle(ov_write_event);
    CloseHandle(tx_data_event);
    CloseHandle(shutdown_event);
    CloseHandle(fd);
#else
    /* select() timeout (100 ms) ensures the thread wakes and checks the flag */
    pthread_join(rx_thread_handle, NULL);
    close(fd);
#endif

    fd = SERIAL_INVALID;
    fprintf(stdout, "Serial: closed\n");
}

int Serial_IsActive(void)
{
    return fd != SERIAL_INVALID;
}

void Serial_TxPortWrite(byte value)
{
    /* Inverted logic: bit7 SET = logical 0 (space); bit7 CLEAR = logical 1 (mark) */
    int logical_bit = (value & 0x80) ? 0 : 1;

    if (!tx_state.active) {
        /* Waiting for start bit: a logical 0 (space) begins a frame */
        if (logical_bit == 0) {
            tx_state.active    = 1;
            tx_state.bit_count = 0;
            tx_state.shift_reg = 0;
        }
        return;
    }

    if (tx_state.bit_count < 8) {
        /* Accumulate data bit (LSB first) */
        if (logical_bit)
            tx_state.shift_reg |= (uint8_t)(1u << tx_state.bit_count);
        tx_state.bit_count++;
    } else {
        /* Bit 9 is the stop bit — should be logical 1 (mark) */
        if (logical_bit == 1) {
            /* Valid frame: send the assembled byte to the host */
            if (fd != SERIAL_INVALID)
                host_write_byte(tx_state.shift_reg);
        }
        /* Framing error (stop bit = space): byte is discarded silently */
        tx_state.active = 0;
    }
}

byte Serial_RxPortRead(void)
{
    int rx_bit;

    if (rx_state.idle) {
        /* Check for a byte waiting in the ring buffer */
        if (!rx_buf_empty()) {
            rx_state.current_byte = rx_buf_pop();
            rx_state.bit_count    = 0;
            rx_state.idle         = 0;
        } else {
            /* Line idle = mark = logical 1 = bit0 set */
            return 0x01;
        }
    }

    if (rx_state.bit_count == 0) {
        rx_bit = 0;   /* Start bit: space = logical 0 = bit0 clear */
    } else if (rx_state.bit_count <= 8) {
        /* Data bit (LSB first): non-inverted — logical level = bit0 value */
        rx_bit = (rx_state.current_byte >> (rx_state.bit_count - 1)) & 1;
    } else {
        /* Stop bit: mark = logical 1 = bit0 set */
        rx_bit       = 1;
        rx_state.idle = 1;
    }

    rx_state.bit_count++;
    return (byte)(rx_bit & 0x01);
}
