#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Z80.h"
#include "SDCart.h"

byte *romBuffer = NULL;
word targetAddress = 0;
byte romBank = 0;
byte ramBank = 0;
byte *ramBuffer = NULL;
byte *sdSectorBuffer = NULL;
FILE *sdImageFile = NULL;

const char *SD_RomName = "LAUNCHER.BIN";
const char *SD_ImgName = "p2000t-sd-card.img";

void SDCart_Init(const char *SDRomPath, const char *SDImgPath) {
    // create SLOT2 RAM buffer (128K)
    ramBuffer = (byte *)malloc(2 * 64 * 1024);
    if (!ramBuffer) {
        return perror("Failed to allocate memory for RAM buffer");
    }
    memset(ramBuffer, 0, 2 * 64 * 1024);

    // read ROM file into SLOT2 ROM (here 16K, while actually 128K)
    FILE *romFile = fopen(SDRomPath, "rb");
    if (romFile == NULL) {
        return perror("Failed to open ROM file");
    }
    romBuffer = (byte *)malloc(16 * 1024);
    if (!romBuffer) {
        return perror("Failed to allocate memory for ROM buffer");
    }
    fread(romBuffer, 1, 16 * 1024, romFile);
    fclose(romFile);

    sdImageFile = fopen(SDImgPath, "rb+");
    if (sdImageFile == NULL) {
        return perror("Failed to open SD image file");
    }
    sdSectorBuffer = (byte *)malloc(512);
    if (!sdSectorBuffer) {
        return perror("Failed to allocate memory for SD sector buffer");
    }
    memset(sdSectorBuffer, 0, 512);
}

void SDCart_Cleanup() {
    if (ramBuffer) free(ramBuffer);
    if (romBuffer) free(romBuffer);
    if (sdSectorBuffer) free(sdSectorBuffer);
    if (sdImageFile) fclose(sdImageFile);
}

byte SD_selected = 0;
byte commandBuffer[6] = {0, 0, 0, 0, 0, 0};
byte commandBufferIndex = 0;
byte commandProcessed = 0;
byte readBlockStarted = 0;
short readBlockIndex = -2;
short writeBlockIndex = -1;
byte dataWritten = 0;

void SDCart_Out(byte port, byte value) {
    if (romBuffer && ramBuffer) {
        switch (port) {
            case PORT_SERIAL:
                if (!SD_selected) break; // skip if SD card not selected
                if (!commandProcessed) {
                    if (commandBufferIndex < 6) {
                        commandBuffer[commandBufferIndex++] = value;
                    }
                    else {
                        commandProcessed = 1;
                        // init vars for read/write block commands
                        if (commandBuffer[0] == SD_BLOCK_READ) {
                            readBlockIndex = -2;
                            readBlockStarted = 0;
                        }
                        if (commandBuffer[0] == SD_BLOCK_WRITE) {
                            writeBlockIndex = -1;
                            dataWritten = 0;
                        }
                    }
                    break;
                }
                if (commandBuffer[0] == SD_BLOCK_WRITE) {
                    //wait for starting $FE token
                    if (writeBlockIndex == -1) {
                        if (value == 0xFE) writeBlockIndex++;
                        break;
                    }
                    // write data to buffer
                    if (writeBlockIndex < 512) {
                        sdSectorBuffer[writeBlockIndex++] = value;
                        if (writeBlockIndex == 512) {
                            // write sector bytes from buffer to SD image file
                            if (sdImageFile) {
                                fseek(sdImageFile, 512 * 
                                    (commandBuffer[1] << 24 
                                    | commandBuffer[2] << 16 
                                    | commandBuffer[3] << 8 
                                    | commandBuffer[4]), SEEK_SET);
                                fwrite(sdSectorBuffer, 1, 512, sdImageFile);
                            }
                            dataWritten = 1;
                        }
                    }
                }
                break;
            case PORT_CLKSTART: // triggers the shift registers for SD i/o
                if (readBlockStarted)
                    readBlockIndex++;
                break;
            case PORT_DESELECT:
                SD_selected = 0;
                break;
            case PORT_SELECT:
                SD_selected = 1;
                commandBufferIndex = 0;
                commandProcessed = 0;
                break;
            case PORT_LED_IO:
                // ignored
                break;
            case PORT_ADDR_LOW:
                targetAddress = (targetAddress & 0xFF00) | value;
                break;
            case PORT_ADDR_HIGH:
                targetAddress = (targetAddress & 0x00FF) | (value << 8);
                break;
            case PORT_ROM_BANK:
                //ignore value, always set to 1
                break;
            case PORT_RAM_BANK:
                ramBank = value ? 1 : value;
                break;
            case PORT_ROM_IO:
                //ignore, cannot write to ROM
                break;
            case PORT_RAM_IO:
                ramBuffer[(ramBank * 64 * 1024) + targetAddress] = value;
                break;
        }
    }
}

byte SDCart_In(byte port) {
    byte ret = 0xFF;
    if (romBuffer && ramBuffer) {
        switch (port) {
            case PORT_SERIAL:
                if (!SD_selected || !commandProcessed) break;
                ret = 0x00; // default emulated R1/R3/R7 reply to commands
                if (commandBuffer[0] == SD_BLOCK_READ) {
                    readBlockStarted = 1;
                    switch (readBlockIndex) {
                        case -2:
                            ret = 0x00;
                            break;
                        case -1:
                            // read 512 bytes from SD image file into buffer
                            // http://www.rjhcoding.com/avrc-sd-interface-4.php
                            if (sdImageFile) {
                                fseek(sdImageFile, 512 * 
                                    (commandBuffer[1] << 24 
                                    | commandBuffer[2] << 16 
                                    | commandBuffer[3] << 8 
                                    | commandBuffer[4]), SEEK_SET);
                                fread(sdSectorBuffer, 1, 512, sdImageFile);
                            }
                            ret = 0xFE; // indicate ready to send data
                            break;
                        default:
                            if (readBlockIndex < 512) {
                                ret = sdSectorBuffer[readBlockIndex];
                            }
                            break;
                    }
                }
                if (commandBuffer[0] == SD_BLOCK_WRITE) {
                    if (!dataWritten)
                        ret = 0x00;
                    else
                        ret = 0x05;
                }
                break;
            case PORT_CLKSTART:
                // ignored
                break;
            case PORT_DESELECT:
                // ignored
                break;
            case PORT_SELECT:
                // ignored
                break;
            case PORT_LED_IO:
                // ignored
                break;
            case PORT_ADDR_LOW:
                ret = targetAddress & 0xFF;
                break;
            case PORT_ADDR_HIGH:
                ret = (targetAddress >> 8) & 0xFF;
                break;
            case PORT_ROM_BANK:
                ret = 0;
                break;
            case PORT_RAM_BANK:
                ret = ramBank;
                break;
            case PORT_ROM_IO:
                ret = romBuffer[targetAddress];
                break;
            case PORT_RAM_IO:
                ret = ramBuffer[(ramBank * 64 * 1024) + targetAddress];
                break;
        }
    }
    return ret;
}