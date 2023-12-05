/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                M2000.c                               ***/
/***                                                                      ***/
/*** This file contains the startup code. It is implementation agnotic.   ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

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

int endsWith(const char* path, const char * suffix) 
{
    int path_len = strlen(path);
    int suffix_len = strlen(suffix);
    int i, result;
    char * path_lower = NULL;

    if (path_len < suffix_len) return 0; // The path can't end with the suffix
    path_lower = strdup(path);
    for (i = 0; path_lower[i]; i++)
      path_lower[i] = tolower(path_lower[i]);
    result = strcmp(path_lower + path_len - suffix_len, suffix);
    free(path_lower);
    return result == 0;
}

/* Check the command line argument looking for the cartridge or tape file name */
static void ProcessArgument (int argc,char *argv[]) 
{
  if (argc != 2) return;
  if (endsWith(argv[1], ".bin"))
    CartName=argv[1];
  else 
    TapeName=argv[1]; //asume tape filename
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
  /* Optionally the cartridge or tape name passed as first argument */
  ProcessArgument (argc,argv);
  // don't boot from default tape
  if (!strcmp(TapeName, "Default.cas")) 
    TapeBootEnabled = 0;

  ProgramPath = GetResourcesPath();
  DocumentPath = GetDocumentsPath();
  if (Verbose) printf("FILENAME_MAX=%i\n", FILENAME_MAX);
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
