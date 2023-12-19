/******************************************************************************/
/*                             M2000 - the Philips                            */
/*                ||||||||||||||||||||||||||||||||||||||||||||                */
/*                ████████|████████|████████|████████|████████                */
/*                ███||███|███||███|███||███|███||███|███||███                */
/*                ███||███||||||███|███||███|███||███|███||███                */
/*                ████████|||||███||███||███|███||███|███||███                */
/*                ███|||||||||███|||███||███|███||███|███||███                */
/*                ███|||||||███|||||███||███|███||███|███||███                */
/*                ███||||||████████|████████|████████|████████                */
/*                ||||||||||||||||||||||||||||||||||||||||||||                */
/*                                  emulator                                  */
/*                                                                            */
/*   Copyright (C) 1996-2023 by Marcel de Kogel and the M2000 team.           */
/*                                                                            */
/*   See the file "LICENSE" for information on usage and redistribution of    */
/*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       */
/******************************************************************************/

// This file contains the P2000 hardware emulation function prototypes

#include <stdio.h>
#include "Z80.h"            /* Z80 emulation declarations    */

#if defined(_WIN32) // Windows
#define PATH_SEPARATOR '\\'
#else // Linux and others
#define PATH_SEPARATOR '/'
#endif

/******** Variables used to control emulator behavior ***********************/
extern byte Verbose;            /* Verbose messages ON/OFF                  */
extern byte *VRAM,*RAM,*ROM;    /* Main and Video RAMs                      */
extern int RAMSizeKb;           /* Amount of RAM installed in kilobytes     */
extern const char *FontName;    /* Font file                                */
extern const char *CartName;    /* Cartridge ROM file                       */
extern const char *ROMName;     /* Main ROM file                            */
extern const char *TapeName;    /* Tape image                               */
extern const char *PrnName;     /* Printer log file                         */
extern int PrnType;             /* Printer type                             */
extern byte DISAReg;            /* Reg #0x70                                */
extern byte SoundReg;           /* Reg #0x50                                */
extern byte ScrollReg;          /* Reg #0x30                                */
extern byte OutputReg;          /* Reg #0x20                                */
extern byte KeyMap[10];         /* Keyboard map                             */
extern int P2000_Mode;          /* 0=P2000T, 1=P2000M                       */
extern int TapeBootEnabled;     /* 1 if booting enabled                     */
extern int ColdBoot;            /* 1 if cold boot                           */
extern int TapeProtect;         /* 1 if tape is write-protected             */
extern int UPeriod;             /* Number of interrupts/screen update       */
extern int IFreq;               /* Number of interrupts/second              */
extern int Sync;                /* 1 if emulation should be synced          */
extern int CpuSpeed;            /* default 100                              */
/****************************************************************************/

/****************************************************************************/
/*** Allocate memory, load ROM images, initialise mapper, VDP and CPU and ***/
/*** the emulation. This function returns 0 in case of a failure          ***/
/****************************************************************************/
int StartP2000(void);

/*** (re)Allocate RAM memory ***/
int InitRAM(void);

/****************************************************************************/
/*** Free memory allocated by StartP2000()                                ***/
/****************************************************************************/
void TrashP2000(void);

/****************************************************************************/
/*** Insert cassette                                                      ***/
/****************************************************************************/
void InsertCassette(const char *filename, FILE *f);

/****************************************************************************/
/*** Removes current cassette                                             ***/
/****************************************************************************/
void RemoveCassette(void);

/****************************************************************************/
/*** Insert cartridge                                                     ***/
/****************************************************************************/
void InsertCartridge(const char *filename, FILE *f);

/****************************************************************************/
/*** Removes current cartridge                                            ***/
/****************************************************************************/
void RemoveCartridge(void);

/****************************************************************************/
/*** Allocate resources needed by the machine-dependent code              ***/
/************************************************** TO BE WRITTEN BY USER ***/
int InitMachine(void);

/****************************************************************************/
/*** Deallocate all resources taken by InitMachine()                      ***/
/************************************************** TO BE WRITTEN BY USER ***/
void TrashMachine(void);

/****************************************************************************/
/*** Poll the keyboard                                                    ***/
/*** This function is called on every interrupt                           ***/
/************************************************** TO BE WRITTEN BY USER ***/
void Keyboard (void);

/****************************************************************************/
/*** This function is called on writes to the sound register              ***/
/************************************************** TO BE WRITTEN BY USER ***/
void Sound(int toggle);

/****************************************************************************/
/*** Flush sound pipes                                                    ***/
/*** This function is called on every interrupt                           ***/
/************************************************** TO BE WRITTEN BY USER ***/
void FlushSound(void);

/****************************************************************************/
/*** Sync emulation                                                       ***/
/*** This function is called on every interrupt                           ***/
/************************************************** TO BE WRITTEN BY USER ***/
void SyncEmulation(void);

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
int LoadFont (const char *filename);

/****************************************************************************/
/*** Returns the path relative to the M2000 executable                    ***/
/************************************************** TO BE WRITTEN BY USER ***/
char *GetResourcesPath (void);

/****************************************************************************/
/*** Returns the user's home directory                                    ***/
/************************************************** TO BE WRITTEN BY USER ***/
char *GetDocumentsPath (void);

/****************************************************************************/
/*** Shows breaking error message and returns error code 1                ***/
/************************************************** TO BE WRITTEN BY USER ***/
int ReturnErrorMessage(const char *format, ...);