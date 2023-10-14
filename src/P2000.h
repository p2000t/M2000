/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                P2000.h                               ***/
/***                                                                      ***/
/*** This file contains the P2000 hardware emulation function prototypes  ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#ifdef UNIX_X
#define UNIX
#endif
#ifdef LINUX_SVGA
#define UNIX
#endif

#ifndef UNIX_X
#undef MITSHM
#endif

#ifdef MSDOS
#define SOUND
#define JOYSTICK
#define HAVE_CLOCK
#endif

#include "Z80.h"            /* Z80 emulation declarations    */

/******** Variables used to control emulator behavior ***********************/
extern byte Verbose;            /* Debug messages ON/OFF                    */
extern byte *VRAM,*RAM,*ROM;    /* Main and Video RAMs                      */
extern int RAMSize;             /* Amount of RAM installed                  */
extern char *FontName;          /* Font file                                */
extern char *CartName;          /* Cartridge ROM file                       */
extern char *ROMName;           /* Main ROM file                            */
extern char *TapeName;          /* Tape image                               */
extern char *PrnName;           /* Printer log file                         */
extern int PrnType;             /* Printer type                             */
extern byte DISAReg;            /* Reg #0x70                                */
extern byte SoundReg;           /* Reg #0x50                                */
extern byte ScrollReg;          /* Reg #0x30                                */
extern byte OutputReg;          /* Reg #0x20                                */
extern byte KeyMap[10];         /* Keyboard map                             */
extern int P2000_Mode;          /* 0=P2000T, 1=P2000M                       */
extern int TapeBootEnabled;     /* 1 if booting enabled                     */
extern int TapeProtect;         /* 1 if tape is write-protected             */
extern int UPeriod;             /* Number of interrupts/screen update       */
extern int IFreq;               /* Number of interrupts/second              */
extern int Sync;                /* 1 if emulation should be synced          */
/****************************************************************************/

/****************************************************************************/
/*** Allocate memory, load ROM images, initialise mapper, VDP and CPU and ***/
/*** the emulation. This function returns 0 in case of a failure          ***/
/****************************************************************************/
int StartP2000(void);

/****************************************************************************/
/*** Free memory allocated by StartP2000()                                ***/
/****************************************************************************/
void TrashP2000(void);

/****************************************************************************/
/*** Change tape image, font used, etc.                                   ***/
/****************************************************************************/
void OptionsDialogue(void);

/****************************************************************************/
/*** Allocate resources needed by the machine-dependent code              ***/
/************************************************** TO BE WRITTEN BY USER ***/
int InitMachine(void);

/****************************************************************************/
/*** Deallocate all resources taken by InitMachine()                      ***/
/************************************************** TO BE WRITTEN BY USER ***/
void TrashMachine(void);

/****************************************************************************/
/*** This function is called to poll keyboard                             ***/
/************************************************** TO BE WRITTEN BY USER ***/
void Keyboard (void);

/****************************************************************************/
/*** This function is called on writes to the sound register              ***/
/************************************************** TO BE WRITTEN BY USER ***/
void Sound(int toggle);

/****************************************************************************/
/*** Flush pipes and sync emulation                                       ***/
/*** This function is called on every interrupt                           ***/
/************************************************** TO BE WRITTEN BY USER ***/
void FlushSound(void);

/****************************************************************************/
/*** Refresh the screen                                                   ***/
/************************************************** TO BE WRITTEN BY USER ***/
void RefreshScreen (void);

/****************************************************************************/
/*** Pause a while                                                        ***/
/************************************************** TO BE WRITTEN BY USER ***/
void Pause (int ms);

/****************************************************************************/
/*** Load the specified font                                              ***/
/************************************************** TO BE WRITTEN BY USER ***/
int LoadFont (char *filename);

