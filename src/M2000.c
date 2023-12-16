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

// This file contains the startup code. It is implementation agnotic.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include "P2000.h"

extern int keyboardmap;
extern int soundmode;
extern int mastervolume;
extern int joymode;
extern int joymap;
extern int videomode;
extern int scanlines;
extern int smoothing;

static char * ProgramPath;
static char * DocumentPath;
static char _CartName[FILENAME_MAX];
static char _ROMName[FILENAME_MAX];
static char _FontName[FILENAME_MAX];
static char _TapeName[FILENAME_MAX];
static char _PrnName[FILENAME_MAX];

/* Check the command line argument looking for the cartridge or tape file name */
static void ProcessArgument (int argc,char *argv[]) 
{
  if (argc != 2) return;
  char *dot = strrchr(argv[1], '.');
  if (dot && !strcasecmp(dot, ".bin"))
    CartName=argv[1];
  else 
    TapeName=argv[1]; // else asume tape filename
}

/* Expand to absolute path */
static char * MakeFullPath (char *dest, const char *src, char *root)
{
  if (!src) return NULL;
  if (!strchr(src,'/') && !strchr(src,'\\')) {
    /* If no path is given, assume file is in root path */
    strcpy (dest,root);
    strcat (dest,src);
  } else
    strcpy (dest,src);

  return dest;
}

int M2000_main(int argc,char *argv[])
{
  /* Optionally a cartridge or tape filename can be passed as first argument */
  ProcessArgument (argc,argv);
  // don't boot from the default tape
  if (!strcmp(TapeName, "Default.cas")) 
    TapeBootEnabled = 0;

  ProgramPath = GetResourcesPath();
  DocumentPath = GetDocumentsPath();
  if (Verbose) printf("ProgramPath=%s\n", ProgramPath);
  if (Verbose) printf("DocumentPath=%s\n", DocumentPath);

  TapeName = MakeFullPath(_TapeName, TapeName, DocumentPath);
  CartName = MakeFullPath(_CartName, CartName, ProgramPath);
  ROMName = MakeFullPath(_ROMName, ROMName, ProgramPath);
  FontName = MakeFullPath(_FontName, FontName, ProgramPath);
  PrnName = MakeFullPath(_PrnName, PrnName, DocumentPath);

  /* Check for valid variables */
  IFreq = IFreq >= 55 ? 60 : 50; //only support 50Hz and 60Hz
  if (UPeriod<1) UPeriod=1;
  if (UPeriod>10) UPeriod=10;
  //only support CPU speeds 10, 20, 50, 100, 120, 200 and 500
  if (CpuSpeed > 350) CpuSpeed = 500;
  else if (CpuSpeed > 160) CpuSpeed = 200;
  else if (CpuSpeed > 110) CpuSpeed = 120;
  else if (CpuSpeed > 75) CpuSpeed = 100;
  else if (CpuSpeed > 35) CpuSpeed = 50;
  else if (CpuSpeed > 15) CpuSpeed = 20;
  else CpuSpeed = 10;
  Z80_IPeriod=(2500000*CpuSpeed)/(100*IFreq);

  /* Start emulated P2000 */
  if (!InitMachine()) return 0;
  StartP2000(); // P2000 loop
  /* Trash emulated P2000 */
  TrashP2000();
  TrashMachine ();
  return EXIT_SUCCESS;
}
