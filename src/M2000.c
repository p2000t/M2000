/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                M2000.c                               ***/
/***                                                                      ***/
/*** This file contains the startup code. It's compatible with both UNIX  ***/
/*** and MSDOS implementations                                            ***/
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
#ifdef ALLEGRO
#include <allegro5/allegro.h>
#endif
#include "P2000.h"

#if defined(_WIN32) || defined(MSDOS) // Windows or DOS
#define PATH_SEPARATOR '\\'
#else // Linux and others
#define PATH_SEPARATOR '/'
#endif

extern char *Title;

/* Maximum configuration file size in bytes */
#define MAX_CONFIG_FILE_SIZE    1024
/* Maximum configuration filename length
   MAXPATH can't be used with MSDOS/DJGPP apps
   running in a Win95 DOS box */
#define MAX_FILE_NAME           256

static char *Options[]=
{ 
  /*  0 */ "verbose",
  /*  1 */ "cpuspeed",
  /*  2 */ "ifreq",
  /*  3 */ "t",
  /*  4 */ "m",
  /*  5 */ "sound",
  /*  6 */ "joystick",
  /*  7 */ "romfile",
  /*  8 */ "uperiod",
  /*  9 */ "trap",
  /* 10 */ "printertype",
  /* 11 */ "printer",
  /* 12 */ "font",
  /* 13 */ "tape",
  /* 14 */ "boot",
  /* 15 */ "volume",
  /* 16 */ "ram",
  /* 17 */ "sync",
  /* 18 */ "video",
  /* 19 */ "cart",
  /* 20 */ "keymap",
  /* 21 */ "joymap",
  /* 22 */ "scanlines",
  NULL
};

extern int keyboardmap;
extern int soundmode;
extern int mastervolume;
extern int joymode;
extern int joymap;
extern int videomode;
extern int scanlines;

static int  CpuSpeed;
static int  shadow_argc;
static char *shadow_argv[256];
static unsigned char ConfigFile[MAX_CONFIG_FILE_SIZE];
static char _ConfigFileName[MAX_FILE_NAME];
static char ProgramPath[MAX_FILE_NAME];
static char DocumentPath[MAX_FILE_NAME];
static char _CartName[MAX_FILE_NAME];
static char _ROMName[MAX_FILE_NAME];
static char _FontName[MAX_FILE_NAME];
static char _TapeName[MAX_FILE_NAME];
static char _PrnName[MAX_FILE_NAME];

int endsWith(const char* path, const char * suffix) {
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

/* Parse the command line options */
static int ParseOptions (int argc,char *argv[])
{
 int N,I,J;
 int misparm;
 for(N=1,I=0;N<argc;N++)
 {
  misparm=0;
  if(*argv[N]!='-')
   switch(I++)
   {
    case 0:  /* CartName=argv[N]; */    /* Already filled in GetCartName() */
             break;
    default: ShowErrorMessage("Excessive filename '%s'\n",argv[N]);
             return 0;
   }
  else
  {
   for(J=0;Options[J];J++)
    if(!strcmp(argv[N]+1,Options[J])) break;
   switch(J)
   {
    case 0:  N++;
             if(N<argc)
              Verbose=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 1:  N++;
             if(N<argc)
              CpuSpeed=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 2:  N++;
             if(N<argc)
              IFreq=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 3:  P2000_Mode=0;
             break;
    case 4:  P2000_Mode=1;
             break;
    case 5:  N++;
             if(N<argc)
              soundmode=atoi(argv[N])
             ;else
              misparm=1;
             break;
    case 6:  N++;
             if(N<argc)
              joymode=atoi(argv[N])
             ;else
              misparm=1;
             break;
    case 7:  N++;
             if(N<argc)
              ROMName=argv[N];
             else
              misparm=1;
             break;
    case 8:  N++;
             if(N<argc)
              UPeriod=atoi(argv[N]);
             else
              misparm=1;
             break;
#ifdef DEBUG
    case  9: N++;
             sscanf(argv[N],"%X",&Z80_Trap);
             break;
#endif
    case 10: N++;
             if(N<argc)
              PrnType=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 11: N++;
             if(N<argc)
              PrnName=argv[N];
             else
              misparm=1;
             break;
    case 12: N++;
             if(N<argc)
              FontName=argv[N];
             else
              misparm=1;
             break;
    case 13: N++;
             if(N<argc)
              TapeName=argv[N];
             else
              misparm=1;
             break;
    case 14: N++;
             if(N<argc)
              TapeBootEnabled=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 15: N++;
             if(N<argc)
              mastervolume=atoi(argv[N])
             ;else
              misparm=1;
             break;
    case 16: N++;
             if(N<argc)
              RAMSize=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 17: N++;
             if(N<argc)
              Sync=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 18: N++;
             if (N<argc)
              videomode=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 19: N++; /* cart */
             if (N<argc)
              CartName=argv[N];
             else
              misparm=1;
             break;
    case 20: N++;  /* keymap */
             if (N<argc)
              keyboardmap=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 21: N++;  /* joymap */
             if (N<argc)
              joymap=atoi(argv[N])
             ;else
              misparm=1;
             break;
    case 22: N++;  /* scanlines */
             if (N<argc)
              scanlines=atoi(argv[N]);
             else
              misparm=1;
             break;
    default: ShowErrorMessage("Wrong option '%s'\n",argv[N]);
             return 0;
   }
   if (misparm)
   {
    ShowErrorMessage("%s: Missing parameter\n",argv[N-1]);
    return 0;
   }
  }
 }
 return 1;
}

/* Parse the command line options looking for the cartridge image name */
static int GetCartOrTapeName (int argc,char *argv[]) {
  int i,j;
  for(i=1;i<argc;i++) {
    if(*argv[i]!='-') {
      if (endsWith(argv[i], ".bin"))
        CartName=argv[i];
      else {
        TapeName=argv[i];
        TapeBootEnabled = 1;
      }
      return 1;
    }
    for(j=0;Options[j];j++) {
      if(!strcmp(argv[i]+1,Options[j])) break;
    }
    if (j != 4 && j != 5) i++; // "-t" and "-m"
  }
  return 0;
}

/* Load the specified configuration file at the specified address
   Update shadow_argc and shadow_argv */
static void LoadConfigFile (char *szFileName,unsigned char *ptr)
{
 FILE *infile;
 infile=fopen (szFileName,"rb");
 if (infile==NULL)
  return;
 fread (ptr,1,MAX_CONFIG_FILE_SIZE,infile);
 fclose (infile);
 while (*ptr)
 {
  while (*ptr && *ptr<=' ')
   ++ptr;
  if (*ptr)
  {
   shadow_argv[shadow_argc++]=(char *)ptr;
   while (*ptr && *ptr>' ')
    ++ptr;
   if (*ptr)
    *ptr++='\0';
  }
 }
}

/* Expand to absolute path */
static char * MakeFullPath (char *dest, char *src, char *root)
{
  if (!src) return NULL;
  if (!strchr(src,'/') && !strchr(src,'\\')) {
    /* If no path is given, assume file is in root path */
    strcpy (dest,root);
    strcat (dest,src);
  } else {
    strcpy (dest,src);
  }
  return dest;
}

#ifdef MSDOS
/* Get the path of the specified filename */
static void GetBasePath (char *szFile,char *szPath) {
  char *p,*q;
  strcpy (szPath,szFile);
  p=szPath;
  q=strchr(p,PATH_SEPARATOR);
  while (q) {                     /* get last '/' */
    p=++q;
    q=strchr(q,PATH_SEPARATOR);
  };
  *p='\0';                       /* remove filename */
}
#endif

int main(int argc,char *argv[])
{
  /* Initialise some variables */
  Verbose=1;
  UPeriod=1;
  CpuSpeed=100;
  IFreq=50;

  /* Get the cartridge name */
  GetCartOrTapeName (argc,argv);

#ifdef ALLEGRO
  if (!al_init()) {
    puts("Allegro could not initialize its core.");
    return 1;
  }
  strcpy (ProgramPath, al_path_cstr(al_get_standard_path(ALLEGRO_RESOURCES_PATH), PATH_SEPARATOR));

  // debian install check
  if (!strcmp(ProgramPath,"/usr/bin/"))
    strcpy(ProgramPath, "/usr/share/M2000/");

  strcpy(DocumentPath, ProgramPath); //fallback to program path
  ALLEGRO_PATH *docPath = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
  if (docPath) {
    al_append_path_component(docPath, "M2000");
    if (al_make_directory(al_path_cstr(docPath, PATH_SEPARATOR)))
      strcpy(DocumentPath, al_path_cstr(docPath, PATH_SEPARATOR));
    al_destroy_path(docPath);
  }
#else
  GetBasePath (argv[0],ProgramPath);
  strcpy(DocumentPath, ProgramPath);
#endif
  if (Verbose) printf("ProgramPath=%s\n", ProgramPath);
  if (Verbose) printf("DocumentPath=%s\n", DocumentPath);

  /* Load M2000.cfg */
  memset (ConfigFile,0,sizeof(ConfigFile));
  //printf("argv[0] = %s\n",argv[0]);
  shadow_argc=1;
  shadow_argv[0]=argv[0];
  strcpy (_ConfigFileName,DocumentPath);
  strcat (_ConfigFileName,"M2000.cfg");
  LoadConfigFile (_ConfigFileName,ConfigFile);
  /* Parse the config file options */
  if (!ParseOptions(shadow_argc,shadow_argv))
    return 1;
  /* Parse the command line options */
  if (!ParseOptions(argc,argv))
    return 1;

  TapeName = MakeFullPath(_TapeName, TapeName, DocumentPath);
  CartName = MakeFullPath(_CartName, CartName, ProgramPath);
  ROMName = MakeFullPath(_ROMName, ROMName, ProgramPath);
  FontName = MakeFullPath(_FontName, FontName, ProgramPath);
  PrnName = MakeFullPath(_PrnName, PrnName, DocumentPath);

  /* Check for valid variables */
  if (IFreq<10) IFreq=10;
  if (IFreq>200) IFreq=200;
  if (UPeriod<1) UPeriod=1;
  if (UPeriod>10) UPeriod=10;
  if (CpuSpeed<10) CpuSpeed=10;
  if (CpuSpeed>1000) CpuSpeed=1000;
  Z80_IPeriod=(2500000*CpuSpeed)/(100*IFreq);

  /* Start emulated P2000 */
#ifndef MSDOS
  if (!InitMachine()) return 0;
#endif
  StartP2000();
  /* Trash emulated P2000 */
  TrashP2000();
#ifndef MSDOS
  TrashMachine ();
#endif
  return 0;
}
