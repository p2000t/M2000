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
#include <signal.h>
#include <sys/stat.h>

#include "P2000.h"
#include "Help.h"

extern char *Title;

/* Maximum configuration file size in bytes */
#define MAX_CONFIG_FILE_SIZE    1024
/* Maximum configuration filename length
   MAXPATH can't be used with MSDOS/DJGPP apps
   running in a Win95 DOS box */
#define MAX_FILE_NAME           256

/* Default extension for cartridge images */
#define _DefExt         ".bin"

static char *Options[]=
{ 
  "verbose","help","cpuspeed","ifreq","t","m",
  "sound","joystick","romfile","uperiod","trap","printertype",
  "printer","font","tape","boot","volume","ram","sync","shm",
  "savecpu","video",
  NULL
};
#define AbvOptions      Options         /* No abrevations yet */

#ifdef SOUND
extern int soundmode;
extern int mastervolume;
#endif
#ifdef JOYSTICK
extern int joymode;
#endif
#ifdef MITSHM
extern int UseSHM;
#endif
#ifdef UNIX_X
extern int SaveCPU;
#endif
extern int videomode;

static int  CpuSpeed;
static int  _argc;
static char *_argv[256];
static char MainConfigFile[MAX_CONFIG_FILE_SIZE];
static char SubConfigFile[MAX_CONFIG_FILE_SIZE];
static char szTempFileName[MAX_FILE_NAME];
static char _CartName[MAX_FILE_NAME];
static char CartNameNoExt[MAX_FILE_NAME];
static char _ROMName[MAX_FILE_NAME];
static char ProgramPath[MAX_FILE_NAME];

#ifndef MSDOS
/* Get full path name, convert all backslashes to UNIX style slashes */
static void _fixpath (char *old,char *new)
{
 strcpy (new,old);
}
#endif

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
    default: printf("Excessive filename '%s'\n",argv[N]);
             return 0;
   }
  else
  {    
   for(J=0;Options[J];J++)
    if(!strcmp(argv[N]+1,Options[J])) break;
   if (!Options[J])
    for(J=0;AbvOptions[J];J++)
     if(!strcmp(argv[N]+1,AbvOptions[J])) break;
   switch(J)
   {
    case 0:  N++;
             if(N<argc)
              Verbose=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 1:  printf ("%s\nCopyright (C) Marcel de Kogel 1996,1997\n",Title);
             for(J=0;HelpText[J];J++)
             {
              puts(HelpText[J]);
              if (!strcmp(HelpText[J],""))
              {
               printf ("-- More --");
               fflush (stdout);
               fgetc (stdin);
               fflush (stdin);
               printf ("\n\n");
              }
             }
             return 0;
    case 2:  N++;
             if(N<argc)
              CpuSpeed=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 3:  N++;
             if(N<argc)
              IFreq=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 4:  P2000_Mode=0;
             break;
    case 5:  P2000_Mode=1;
             break;
    case 6:  N++;
             if(N<argc)
#ifdef SOUND
              soundmode=atoi(argv[N])
#endif
             ;else
              misparm=1;
             break;
    case 16: N++;
             if(N<argc)
#ifdef SOUND
              mastervolume=atoi(argv[N])
#endif
             ;else
              misparm=1;
             break;
    case 7:  N++;
             if(N<argc)
#ifdef JOYSTICK
              joymode=atoi(argv[N])
#endif
             ;else
              misparm=1;
             break;
    case 8:  N++;
             if(N<argc)
              ROMName=argv[N];
             else
              misparm=1;
             break;
    case 9:  N++;
             if(N<argc)
              UPeriod=atoi(argv[N]);
             else
              misparm=1;
             break;
#ifdef DEBUG
    case 10: N++;
             sscanf(argv[N],"%X",&Z80_Trap);
             break;
#endif
    case 11: N++;
             if(N<argc)
              PrnType=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 12: N++;
             if(N<argc)
              PrnName=argv[N];
             else
              misparm=1;
             break;
    case 13: N++;
             if(N<argc)
              FontName=argv[N];
             else
              misparm=1;
             break;
    case 14: N++;
             if(N<argc)
              TapeName=argv[N];
             else
              misparm=1;
             break;
    case 15: N++;
             if(N<argc)
              TapeBootEnabled=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 17: N++;
             if(N<argc)
              RAMSize=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 18: N++;
             if(N<argc)
              Sync=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 19: N++;
             if (N<argc)
#ifdef MITSHM
              UseSHM=atoi(argv[N])
#endif
             ;else
              misparm=1;
             break;
    case 20: N++;
             if (N<argc)
#ifdef UNIX_X
              SaveCPU=atoi(argv[N])
#endif
             ;else
              misparm=1;
             break;
    case 21: N++;
             if (N<argc)
              videomode=atoi(argv[N]);
             else
              misparm=1;
             break;
    default: printf("Wrong option '%s'\n",argv[N]);
             return 0;
   }
   if (misparm)
   {
    printf("%s: Missing parameter\n",argv[N-1]);
    return 0;
   }
  }
 }
 return 1;
}

/* Parse the command line options looking for the cartridge image name */
static int GetCartName (int argc,char *argv[])
{
 int N,I,J;
 for(N=1,I=0;N<argc;N++)
 {
  if(*argv[N]!='-')
   switch(I++)
   {
    case 0:  CartName=argv[N];
             break;
    default: return 0;
   }
  else
  {    
   for(J=0;Options[J];J++)
    if(!strcmp(argv[N]+1,Options[J])) break;
   if (!Options[J])
    for(J=0;AbvOptions[J];J++)
     if(!strcmp(argv[N]+1,AbvOptions[J])) break;
   switch(J)
   {
    case 1:  return 0;
    case 0: case 2: case 3: case 6: case 7: case 8: case 9:
    case 11: case 12: case 13: case 14: case 15: case 17: case 18:
    case 16: case 19: case 20: case 21:
#ifdef DEBUG
    case 10:
#endif
             N++;
             if (N>=argc)
              return 0;
             break;
    case 4: case 5:
             break;
    default: return 0;
   }
  }
 }
 return 1;
}

/* Load the specified configuration file at the specified address
   Update _argc and _argv */
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
   _argv[_argc++]=ptr;
   while (*ptr && *ptr>' ')
    ++ptr;
   if (*ptr)
    *ptr++='\0';
  }
 }
}

/* Fix the cartridge image file name and initialise
   CartNameNoExt, used for getting the cartridge
   configuration file */
static void FixFileNames (void)
{
 char *p=NULL,*q=NULL;
 if (!strchr(CartName,'/') && !strchr(CartName,'\\'))
 {      /* If no path is given, assume emulator path */
  strcpy (_CartName,ProgramPath);
  strcat (_CartName,CartName);
 }
 else
  _fixpath (CartName,_CartName);
#ifdef MSDOS
 strlwr (_CartName);
#endif
 CartName=_CartName;
 strcpy (CartNameNoExt,CartName);
 p=CartNameNoExt;
 q=strchr(CartNameNoExt,'/');
 while (q)                      /* get last '/' */
 {
  p=++q;
  q=strchr(q,'/');
 };
 q=NULL;
 while ((p=strchr(p,'.'))!=NULL) /* get last '.' */
 {
  q=p;
  ++p;
 }
 if (q)                         /* remove extension */
  *q='\0';
 else
  strcat (CartName,_DefExt);
}

/* Fix the main ROM file name */
static void FixRomPath (void)
{
 char *p=NULL,*q=NULL;
 if (!strchr(ROMName,'/') && !strchr(ROMName,'\\'))
 {      /* If no path is given, assume emulator path */
  strcpy (_ROMName,ProgramPath);
  strcat (_ROMName,ROMName);
 }
 else
 {
  _fixpath (ROMName,_ROMName);
 }
 p=_ROMName;
 q=strchr(_ROMName,'/');
 while (q)                      /* get last '/' */
 {
  p=++q;
  q=strchr(q,'/');
 };
 q=NULL;
 while ((p=strchr(p,'.'))!=NULL) /* get last '.' */
 {
  q=p;
  ++p;
 }
 if (!q)                       /* Default extension='.bin' */
  strcat (_ROMName,_DefExt);
#ifdef MSDOS
 strlwr (_ROMName);
#endif
 ROMName=_ROMName;
}

/* Get the path of the specified filename */
static void GetPath (char *szFile,char *szPath)
{
 char *p,*q;
 strcpy (szPath,szFile);
 p=szPath;
 q=strchr(p,'/');
 while (q)                      /* get last '/' */
 {
  p=++q;
  q=strchr(q,'/');
 };
 *p='\0';                       /* remove filename */
}

int main(int argc,char *argv[])
{
 /* Initialise some variables */
 Verbose=1;
 UPeriod=1;
 CpuSpeed=100;
 IFreq=50;
#ifdef MSDOS
 PrnName="PRN";
#endif
 /* Load m2000.cfg */
 memset (MainConfigFile,0,sizeof(MainConfigFile));
 memset (SubConfigFile,0,sizeof(SubConfigFile));
#ifdef MSDOS
 strlwr (argv[0]);
#endif
 GetPath (argv[0],ProgramPath);
 _argc=1;
 _argv[0]=argv[0];
 strcpy (szTempFileName,ProgramPath);
 strcat (szTempFileName,"m2000.cfg");
 LoadConfigFile (szTempFileName,MainConfigFile);
 if (!ParseOptions(_argc,_argv))
  return 1;
 /* Get the cartridge name */
 GetCartName (argc,argv);
 FixFileNames ();
 /* Load cart.cfg */
 strcpy (szTempFileName,CartNameNoExt);
 strcat (szTempFileName,".cfg");
 _argc=1;
 LoadConfigFile (szTempFileName,SubConfigFile);
 if (!ParseOptions(_argc,_argv))
  return 1;
 /* Parse the command line options */
 if (!ParseOptions(argc,argv))
  return 1;
 FixRomPath ();
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
