// GPIO-serial (bit-bang RS-232) emulation for the P2000T.
//
// The P2000T has no hardware UART. Serial I/O is bit-banged over two GPIO
// lines that share the cassette/printer I/O ports:
//
//   TX (out): port 0x10, bit 7  — inverted: bit7 set = logical 0 (space)
//   RX (in):  port 0x20, bit 0  — non-inverted: bit0 set = logical 1 (mark)
//
// Framing: 9600 baud, 8N1 (1 start bit, 8 data bits LSB-first, 1 stop bit).

#ifndef SERIAL_H
#define SERIAL_H

#include "Z80.h"   /* for byte typedef */

/* Initialise serial emulation.
 * device: COM port or pipe path, e.g. "\\\\.\\COM4" on Windows or
 *         "/dev/ttyS0" on Linux.
 * Returns 1 on success, 0 on failure. */
int  Serial_Init(const char *device);

/* Shut down serial emulation and release all resources. */
void Serial_Shutdown(void);

/* Called by Z80_Out() whenever port 0x10 is written.
 * value is the full byte written; only bit 7 carries serial TX data.
 * Inverted logic: bit7 SET = logical 0 (space); bit7 CLEAR = logical 1 (mark). */
void Serial_TxPortWrite(byte value);

/* Called by Z80_In() whenever port 0x20 is read.
 * Returns the value to present on bit 0 of port 0x20.
 * Non-inverted logic: returns 1 for mark (idle/stop), 0 for space (start).
 * Advances the RX state machine by one bit on each call. */
byte Serial_RxPortRead(void);

#endif /* SERIAL_H */
