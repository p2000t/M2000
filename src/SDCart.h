#ifndef _SDCART_H
#define _SDCART_H

#include "Z80.h"

#define BASEPORT        0x40
#define PORT_SERIAL     (BASEPORT | 0x00)
#define PORT_CLKSTART   (BASEPORT | 0x01)
#define PORT_DESELECT   (BASEPORT | 0x02)
#define PORT_SELECT     (BASEPORT | 0x03)
#define PORT_LED_IO     (BASEPORT | 0x04)
#define PORT_ADDR_LOW   (BASEPORT | 0x08)
#define PORT_ADDR_HIGH  (BASEPORT | 0x09)
#define PORT_ROM_BANK   (BASEPORT | 0x0A)
#define PORT_RAM_BANK   (BASEPORT | 0x0B)
#define PORT_ROM_IO     (BASEPORT | 0x0C)
#define PORT_RAM_IO     (BASEPORT | 0x0D)

#define SD_BLOCK_READ   (17|0x40)
#define SD_BLOCK_WRITE  (24|0x40)

extern const char *SD_RomName;
extern const char *SD_ImgName;

void SDCart_Init(const char *romPath, const char *sdImagePath);
void SDCart_Cleanup();
void SDCart_Out(byte port, byte value);
byte SDCart_In(byte port);

#endif // _SDCART_H

