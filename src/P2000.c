/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                P2000.c                               ***/
/***                                                                      ***/
/*** This file contains the P2000 hardware emulation code                 ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include "P2000.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

byte Verbose     = 1;
char *ROMName    = "P2000ROM.bin";
char *CartName   = "BASIC.bin";
char *FontName   = "Default.fnt";
char *TapeName   = "P2000.cas";
char *PrnName    = NULL;
FILE *PrnStream  = NULL;
FILE *TapeStream = NULL;
int TapeProtect  = 0;
int P2000_Mode   = 0;
int UPeriod      = 2;
int IFreq        = 50;
int Sync         = 1;
int TapeBootEnabled = 0;
int PrnType      = 0;
int RAMSize      = 32;
int Z80_IRQ      = Z80_IGNORE_INT;

byte SoundReg=0,ScrollReg=0,OutputReg=0,DISAReg=0,RAMMapper=0;
byte RAMMask=0;
byte *ROM;
byte *VRAM;
byte *RAM;
byte NoRAMWrite[0x100];
byte NoRAMRead[0x100];
byte *ReadPage[256];
byte *WritePage[256];
byte KeyMap[10];
word ROMPatches[] = { 0x04F1, 0xE5D, 0x0000 };

/****************************************************************************/
/*** These macros are used by the Z80_Patch() function to read and write  ***/
/*** word-sized variables from/to memory                                  ***/
/****************************************************************************/
static unsigned Z80_RDWORD (dword a)
{
 return Z80_RDMEM(a)+Z80_RDMEM((a+1)&65535)*256;
}

static void Z80_WRWORD (dword a,unsigned v)
{
 Z80_WRMEM (a,v);
 Z80_WRMEM ((a+1)&65535,v>>8);
}

/****************************************************************************/
/*** Write a value to given I/O port                                      ***/
/****************************************************************************/
void Z80_Out (byte Port, byte Value)
{
 int i;
 switch (Port>>4)
 {
  case 0:       /* Read the key-matrix */
   return;
  case 1:       /* Output to cassette/printer */
   OutputReg=Value;
   return;
  case 2:       /* Input from cassette/printer */
   return;
  case 3:       /* Scroll Register (T-version only) */
   ScrollReg=Value;
   return;
  case 4:       /* Reserved for I/O cartridge */
   break;
  case 5:       /* Beeper */
   SoundReg=Value;
   Sound (Value&1);
   return;
  case 6:       /* Reserved for I/O cartridge */
   break;
  case 7:       /* DISAS (M-version only) */
   /* If bit 1 is set, Video refresh is
      disabled when CPU accesses video RAM */
   if (P2000_Mode)
    DISAReg=Value;
   return;
 }
 switch (Port)
 {
  /* CTC */
  case 0x88:
  case 0x89:
  case 0x8A:
  case 0x8B:
   break;
  /* Floppy controller */
  case 0x8D:
  case 0x8E:
  case 0x8F:
  case 0x90:
   break;
  /* RAM Bank select */
  case 0x94:
   if (RAMMask)
   {
    Value&=RAMMask;
    RAMMapper=Value;
    for (i=0xE000;i<0x10000;i+=256)
     ReadPage[i>>8]=WritePage[i>>8]=&RAM[i-24576+Value*8192];
   }
   return;
 }
}

/****************************************************************************/
/*** Read value from given I/O port                                       ***/
/****************************************************************************/
byte Z80_In (byte Port)
{
 switch (Port>>4)
 {
  case 0:       /* Read the key-matrix */
   if (OutputReg&0x40)
    return KeyMap[0] & KeyMap[1] & KeyMap[2] & KeyMap[3] & KeyMap[4] &
           KeyMap[5] & KeyMap[6] & KeyMap[7] & KeyMap[8] & KeyMap[9];
   else
    if ((Port&0x0F)>9)
     return 0xFF;
    else
     return KeyMap[Port&0xFF];
  case 1:       /* Output to cassette/printer */
   return OutputReg;
  case 2:       /* Input from cassette/printer */
  {
   static int inputstatus=0;
   inputstatus|=0xBF;
   inputstatus^=0x40;           /* toggle input clock */
   if (TapeStream) inputstatus&=0xEF;
   if (!TapeProtect) inputstatus&=0xF7;
   if (PrnStream) inputstatus&=0xFD;
   if (PrnType) inputstatus&=0xFB;
   return inputstatus;
  }
  case 3:       /* Scroll Register (T-version only) */
   if (!P2000_Mode)
    return ScrollReg;
  case 4:       /* Reserved for I/O cartridge */
   break;
  case 5:       /* Beeper */
   return SoundReg;
  case 6:       /* Reserved for I/O cartridge */
   break;
  case 7:       /* DISAS (M-version only) */
   if (P2000_Mode)
    return DISAReg;
   break;
 }
 switch (Port)
 {
  /* CTC */
  case 0x88:
  case 0x89:
  case 0x8A:
  case 0x8B:
   break;
  /* Floppy controller */
  case 0x8D:
  case 0x8E:
  case 0x8F:
  case 0x90:
   break;
  /* RAM Bank select */
  case 0x94:
   if (RAMMask)
    return RAMMapper;
   break;
 }
 return 0xFF;
}

/****************************************************************************/
/*** Allocate memory, load ROM images, initialise mapper, VDP and CPU and ***/
/*** the emulation. This function returns 0 in case of a failure          ***/
/****************************************************************************/
#ifdef MSDOS
int InitMachineDone=0;
#endif
word Exit_PC;
int StartP2000 (void)
{
  FILE *F;
  int *T,I,J;

  /*** STARTUP CODE starts here: ***/

  T=(int *)"\01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#ifdef LSB_FIRST
  if(*T!=1)
  {
    printf("********** This machine is high-endian. **********\n");
    printf("Take #define LSB_FIRST out and compile fMSX again.\n");
    return(0);
  }
#else
  if(*T==1)
  {
    printf("********* This machine is low-endian. **********\n");
    printf("Insert #define LSB_FIRST and compile fMSX again.\n");
    return(0);
  }
#endif

  RAMSize*=1024;
  if (RAMSize>40960)
  {
   I=RAMSize-32768;
   if (I&8191) I=(I&(~8191))+8192;
   I/=8192;
   for(J=1;J<I;J<<=1);RAMMask=J-1;
   RAMSize=32768+J*8192;
  }
  else
  {
   if (RAMSize<=16384) RAMSize=16384;
   else if (RAMSize<=32768) RAMSize=32768;
   else RAMSize=40960;
  }
  if (Verbose)
   printf ("Allocating memory: 20KB ROM, 4KB VRAM, %uKB RAM...",RAMSize/1024);
  ROM=malloc (0x5000);
  VRAM=malloc (0x1000);
  RAM=malloc (RAMSize);
  if (!ROM || !VRAM || !RAM)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  memset (ROM,0xFF,0x5000);
  memset (VRAM,0,0x1000);
  memset (RAM,0,RAMSize);
  if (Verbose) printf ("OK\n");
  for (I=0;I<256;++I)
  {
   ReadPage[I]=NoRAMRead;
   WritePage[I]=NoRAMWrite;
   NoRAMRead[I]=0xFF;
  }
  for (I=0;I<0x5000;I+=256)
  {
   ReadPage[I>>8]=ROM+I;
   WritePage[I>>8]=NoRAMWrite;
  }
  if (P2000_Mode)
   for (I=0x0000;I<0x1000;I+=256)
    ReadPage[(I+0x5000)>>8]=WritePage[(I+0x5000)>>8]=VRAM+I;
  else
   for (I=0x0000;I<0x0800;I+=256)
    ReadPage[(I+0x5000)>>8]=WritePage[(I+0x5000)>>8]=VRAM+I;
  for (I=0x0000;I<((RAMSize<0xA000)? RAMSize:0xA000);I+=256)
  {
   ReadPage[(I+0x6000)>>8]=WritePage[(I+0x6000)>>8]=RAM+I;
  }

  if (Verbose) printf ("Loading ROMs:\n");
  if (Verbose) printf ("  Opening %s... ",ROMName);
  J=0;
  F=fopen(ROMName,"rb");
  if(F)
  {
   if(fread(ROM,1,0x1000,F)==0x1000) J=1;
   fclose (F);
  }
  if(Verbose) puts(J? "OK":"FAILED");
  if(!J) return(0);
  if (Verbose) printf ("  Patching");
  for (J=0;ROMPatches[J];++J)
  {
   if (Verbose) printf ("...%04X",ROMPatches[J]);
   ROM[ROMPatches[J]+0]=0xED;
   ROM[ROMPatches[J]+1]=0xFE;
   ROM[ROMPatches[J]+2]=0xC9;
  }
  if(Verbose) printf(" OK\n  Opening %s... ",CartName);
  J=0;
  F=fopen(CartName,"rb");
  if (F)
  {
   if (fread(ROM+0x1000,1,0x4000,F)) J=1;
   fclose(F);
  }
  if(Verbose) puts (J? "OK":"FAILED");
/*  if(!J) return 0; */

  if (TapeName)
  {
   if (Verbose) printf ("Opening tape image %s... ",TapeName);
   TapeStream=fopen (TapeName,"a+b");
   if (Verbose) puts ((TapeStream)? "OK":"FAILED");
   if (TapeStream) rewind (TapeStream);
  }

  if (Verbose) printf ("Opening printer stream %s... ",
                       (PrnName)?PrnName:"stdout");
  PrnStream=(PrnName)? fopen (PrnName,"wb") : stdout;
  if (PrnStream && !PrnName) PrnName="stdout";
  if (Verbose) puts ((PrnStream)? "OK":"FAILED");

  if (!LoadFont(FontName)) return 0;

  memset (KeyMap,0xFF,sizeof(KeyMap));

#ifdef MSDOS
  if (!InitMachine())
   return 0;
  InitMachineDone=1;
#else
  if (Verbose) puts ("Starting P2000 emulation...");
#endif
  Z80_Reset ();
  Exit_PC=Z80 ();
#ifndef MSDOS
  if (Verbose) printf("EXITED at PC = %Xh\n",Exit_PC);
#endif
  return(1);
}

/****************************************************************************/
/*** Free memory allocated by StartP2000()                                ***/
/****************************************************************************/
void TrashP2000 (void)
{
#ifdef MSDOS
 if (InitMachineDone)
 {
  TrashMachine ();
  if (Verbose) printf("EXITED at PC = %Xh\n",Exit_PC);
 }
#endif
 if (ROM) free (ROM);
 if (VRAM) free (VRAM);
 if (RAM) free (RAM);
}

/****************************************************************************/
/*** Change tape image, font used, etc.                                   ***/
/****************************************************************************/
void OptionsDialogue (void)
{
 static char _TapeName[256],_FontName[256],_PrnName[256];
 char buf[256];
 char *p;
 int tmp;
 do
 {
  printf ("Options currently in use are:\n"
          "Tape image       - %s\n"
          "Printer log file - %s\n"
          "Font file name   - %s\n"
          "Z80 CPU speed    - %u%%\n",
          (TapeStream)? TapeName:"none",
          (PrnStream)? PrnName:"none",
          FontName,
          Z80_IPeriod*IFreq*100/2500000);
  printf ("Available commands are:\n"
          "t <filename>     - Change tape image\n"
          "p <filename>     - Change printer log file\n"
          "f <filename>     - Change font file name\n"
          "c <percentage>   - Change Z80 CPU speed\n"
          "b                - Back\n"
          "q                - Quit emulator\n> ");
  fflush (stdout);
  buf[2]='\0';
  fgets (buf,sizeof(buf),stdin);
  p=strchr (buf,'\n');
  if (p) *p='\0';
  fflush (stdin);
  switch (buf[0])
  {
   case 't': case 'T':
    strcpy (_TapeName,buf+2);
    TapeName=_TapeName;
    fclose (TapeStream);
    if (Verbose) printf ("Opening tape image %s... ",TapeName);
    TapeStream=fopen (_TapeName,"a+b");
    if (TapeStream) rewind (TapeStream);
    if (Verbose) puts ((TapeStream)? "OK":"FAILED");
    break;
   case 'p': case 'P':
    strcpy (_PrnName,buf+2);
    PrnName=_PrnName;
    fclose (PrnStream);
    if (Verbose) printf ("Opening printer log file %s... ",PrnName);
    PrnStream=fopen (PrnName,"wb");
    if (Verbose) puts ((PrnStream)? "OK":"FAILED");
    break;
   case 'f': case 'F':
    strcpy (_FontName,buf+2);
    if (LoadFont (_FontName)) FontName=_FontName;
    break;
   case 'c': case 'C':
    tmp=atoi(buf+2);
    if (tmp<10) tmp=10;
    if (tmp>1000) tmp=1000;
    Z80_IPeriod=(2500000*tmp)/(100*IFreq);
    break;
   case 'q': case 'Q':
    Z80_Running=0;
    break;
  }
 }
 while (buf[0]!='q' && buf[0]!='Q' && buf[0]!='b' && buf[0]!='B');
}

/****************************************************************************/
/*** Refresh screen, check keyboard events and return interrupt id        ***/
/****************************************************************************/
int Z80_Interrupt(void)
{
 static int UCount=1;
 Keyboard ();
 FlushSound ();
 if (UCount)
  --UCount;
 else
 {
  UCount=UPeriod;
  RefreshScreen ();
 }
 return (OutputReg&0x40)? 0x00FF:Z80_IGNORE_INT;
}

void Z80_Reti (void) { }
void Z80_Retn (void) { }

/****************************************************************************/
/*** This is called when ED FE occurs and is used to emulate tape and     ***/
/*** access                                                               ***/
/****************************************************************************/
void Z80_Patch (Z80_Regs *R)
{
 #define caserror       0x6017
 #define lengte         0x601A
 #define recleng        0x6034
 #define transfer       0x6030
 #define telblok        0x606E
 #define stacas         0x6060
 #define motorstat      0x6050
 #define desleng        0x606A
 #define des1           0x6068
 #define descrip        0x6030
 #define recnum         0x604F
 #define fileleng       0x6032
 static byte tapebuf[1024+256];
 int i,j,k,l,m;
 switch (R->PC.W.l-2)
 {
  /**************************************************************************/
  /** 0x04F1: Tape functions                                               **/
  /**************************************************************************/
  case 0x04F1:
   if (Verbose&4) printf ("Tape function called: ");
   Z80_WRWORD (lengte,Z80_RDWORD(recleng));
   Z80_WRWORD (des1,Z80_RDWORD(descrip));
   Z80_WRWORD (desleng,0x20);
   Z80_WRMEM (telblok,Z80_RDMEM(recnum));
   switch (R->AF.B.h)
   {
    /*************************************************************************
       Initialise tape system
       Input : None
       Output: [caserror]=error status
    *************************************************************************/
    case 0:
     if (Verbose&4) puts ("Initialise tape system");
     Z80_WRMEM (stacas,0);
     Z80_WRMEM (motorstat,0);
     break;
    /*************************************************************************
       Rewind tape
       Input : None
       Output: [caserror]=error status
    *************************************************************************/
    case 1:
     if (Verbose&4) puts ("Rewind tape");
     if (TapeStream)
     {
      rewind (TapeStream);
      Z80_WRMEM (caserror,0);
     }
     else
      Z80_WRMEM (caserror,0x41);
     break;
    /*************************************************************************
       Skip blocks forwards
       Input : telblok=nr. of blocks to skip
       Output: [caserror]=error status
    *************************************************************************/
    case 2:
     i=Z80_RDMEM (telblok);
     if (Verbose&4)
      printf ("Skip block (forward): %u block%s\n",i,(i==1)? "":"s");
     if (TapeStream)
     {
      j=ftell (TapeStream);
      if (fseek (TapeStream,j+i*(1024+256)-1,SEEK_SET))
      {
       rewind (TapeStream);
       Z80_WRMEM (caserror,0x45);
      }
      else
       if ((j=fgetc(TapeStream))==EOF)
       {
        rewind (TapeStream);
        Z80_WRMEM (caserror,0x45);
       }
       else
        Z80_WRMEM (caserror,0);
     }
     else
      Z80_WRMEM (caserror,0x41);
     break;
    /*************************************************************************
       Skip blocks backwards
       Input : telblok=nr. of blocks to skip
       Output: [caserror]=error status
    *************************************************************************/
    case 3:
     i=Z80_RDMEM (telblok);
     if (Verbose&4)
      printf ("Skip block (backward): %u block%s\n",i,(i==1)? "":"s");
     if (TapeStream)
     {
      j=ftell (TapeStream);
      if (fseek (TapeStream,j-i*(1024+256),SEEK_SET))
      {
       rewind (TapeStream);
       Z80_WRMEM (caserror,0x45);
      }
      else
       if ((j=fgetc(TapeStream))==EOF)
       {
        rewind (TapeStream);
        Z80_WRMEM (caserror,0x45);
       }
       else
       {
        Z80_WRMEM (caserror,0);
        ungetc (j,TapeStream);
        Z80_WRMEM (telblok,0);
       }
     }
     else
      Z80_WRMEM (caserror,0x41);
     break;
    /*************************************************************************
       Write end of tape mark
       Input : none
       Output: [caserror]=error status
    *************************************************************************/
    case 4:
     if (Verbose&4) puts ("EOT");
     if (TapeStream && !TapeProtect)
     {
      /* Truncate the tape image */
#ifdef HAVE_FTRUNCATE
      ftruncate (fileno(TapeStream),ftell(TapeStream));
      fclose (TapeStream);
      TapeStream=fopen (TapeName,"a+b");
      if (TapeStream)
       Z80_WRMEM (caserror,0);
      else
       Z80_WRMEM (caserror,0x41);         /* No tape */
#else
       Z80_WRMEM (caserror,0);
#endif
     }
     else
      Z80_WRMEM (caserror,(TapeStream)? 0x47:0x41);
     break;
    /*************************************************************************
       Write blocks
       Input : [lengte]=nr. of bytes to transfer from memory
               [fileleng]=nr. of bytes to write to tape
               [transfer]=memory address
       Output: [caserror]=error status
    *************************************************************************/
    case 5:
     i=Z80_RDWORD(fileleng);
     i=(i-1)&0xFFFF;
     i=(i/0x400)+1;
     k=Z80_RDWORD(transfer);
     if (Verbose&4)
      printf ("Write block: %u bytes, %u block%s at %04X\n",
              Z80_RDWORD(lengte),i,(i==1)?"":"s",k);
     if (TapeStream && !TapeProtect)
     {
      Z80_WRMEM (caserror,0);
      for (;i;--i)
      {
       Z80_WRMEM (recnum,i);
       for (j=0x00;j<0x100;++j)
        tapebuf[j]=Z80_RDMEM (0x6000+j);
       l=m=Z80_RDWORD (lengte);
       if (l>1024) l=1024;
       Z80_WRWORD (lengte,m-l);
       for (j=0;j<l;++j)
        tapebuf[j+256]=Z80_RDMEM ((k+j)&0xFFFF);
       for (j=l;j<1024;++j)
        tapebuf[j+256]=0;
       k=(k+1024)&0xFFFF;
       if (!fwrite(tapebuf,1024+256,1,TapeStream))
       {
        rewind (TapeStream);
        Z80_WRMEM (caserror,0x45);
        break;
       }
      }
     }
     else
      Z80_WRMEM (caserror,(TapeStream)? 0x47:0x41);
     break;
    /*************************************************************************
       Read blocks
       Input : [lengte]=nr. of bytes to transfer to memory
               [fileleng]=nr. of bytes to read from tape
               [transfer]=memory address
       Output: [caserror]=error status
    *************************************************************************/
    case 6:
    {
     static int delay_next_load=0;
     i=Z80_RDWORD(fileleng);
     i=(i-1)&0xFFFF;
     i=(i/0x400)+1;
     k=Z80_RDWORD(transfer);
     if (Verbose&4)
      printf ("Read block: %u bytes, %u block%s at %04X\n",
              Z80_RDWORD(lengte),i,(i==1)?"":"s",k);
     if (TapeStream)
     {
      for (;i;--i)
      {
       if (!fread(tapebuf,1024+256,1,TapeStream))
       {
        Z80_WRMEM (caserror,0x4D);
        break;
       }
       for (j=0x30;j<0x50;++j)
        Z80_WRMEM (0x6000+j,tapebuf[j]);
       l=m=Z80_RDWORD (lengte);
       if (l>1024) l=1024;
       Z80_WRWORD (lengte,m-l);
       if (k>=0x5000 && k<0x6000)
       {
        /* We're reading to video memory. Emulate a loading picture */
        for (j=0;j<l;j+=80)
        {
         for (m=j;m<l && m<(j+80);++m)
          Z80_WRMEM((k+m)&0xFFFF,tapebuf[m+256]);
         RefreshScreen ();
         Keyboard ();
         if (!Z80_Running) return;
         Pause (200);
        }
        delay_next_load=1;
       }
       else
       {
        if (delay_next_load)
        {
         delay_next_load=0;
         for (j=0;j<60;++j)
         {
          /* Maybe someone wants a screen shot */
          Keyboard ();
          if (!Z80_Running) break;
          Pause (50);
         }
        }
        for (j=0;j<l;++j)
         Z80_WRMEM((k+j)&0xFFFF,tapebuf[j+256]);
       }
       k=(k+1024)&0xFFFF;
      }
     }
     else
      Z80_WRMEM (caserror,0x41);
     break;
    }
    /*************************************************************************
       Tape status
       Input : None
       Output: A=Tape status
               CF=1 -> Write protect
               ZF=1 -> No tape
    *************************************************************************/
    case 7:
    {
     static int first=1;
     if (Verbose&4) puts ("Tape status");
     i=Z80_In (0x20);
     if (first && !TapeBootEnabled)
      i|=0x10;
     first=0;
     if (i&8)
      R->AF.B.l|=1;
     else
      R->AF.B.l&=0xFE;
     i=(i>>4)|(i<<4);
     R->AF.B.h=~i;
     if (R->AF.B.h&1)
      R->AF.B.l&=0xBF;
     else
      R->AF.B.l|=0x40;
     return;
    }
    default:
     if (Verbose&4) printf ("Invalid function number %u\n",R->AF.B.h);
     Z80_WRMEM (caserror,0x4B);
     break;
   }
   R->AF.B.h=Z80_RDMEM (caserror);
   if (R->AF.B.h)
    R->AF.B.l&=0xBF;
   else
    R->AF.B.l|=0x40;
   if (TapeStream)
    fseek (TapeStream,ftell(TapeStream),SEEK_SET);
   break;
  /**************************************************************************/
  /** 0x0E5D: Output a byte to the serial port                             **/
  /**************************************************************************/
  case 0xE5D:
   if (PrnStream)
   {
    fputc (R->BC.B.l,PrnStream);
    if (R->BC.B.l==10) fflush (PrnStream);
   }
   R->IFF1=R->IFF2=1;
   break;
  default:
   printf ("Unknown patch called at %u\n",R->PC.W.l-2);
 }
}

