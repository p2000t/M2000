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

// This file contains the P2000 hardware emulation code

#include "P2000.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define HEADER_SIZE 256 // .cas files uses 256 byte block-headers, while actual P2000T uses 32 byte block-headers
#define HEADER_OFFSET 48 // actual 32 bytes of header data starts at offset 48 in the 256 byte .cas block-header
#define SPACE 32 // space character

byte Verbose     = 0;
const char *ROMName    = "P2000ROM.bin";
const char *CartName   = "BASIC.bin";
const char *FontName   = "Default.fnt";
const char *TapeName   = "Default.cas";
const char *PrnName    = "Printer.out";
FILE *PrnStream  = NULL;
FILE *TapeStream = NULL;
int TapeProtect  = 0;
int UPeriod      = 1;
int IFreq        = 50;
int Sync         = 1;
int CpuSpeed     = 100;
int TapeBootEnabled = 1;
int PrnType      = 0;
int RAMSizeKb    = 32;
int Z80_IRQ      = Z80_IGNORE_INT;
int ColdBoot     = 1;
int NMI          = 0;

byte SoundReg=0,ScrollReg=0,OutputReg=0,DISAReg=0,RAMMapper=0;
int RAMBanks=0;
byte *ROM;
byte *VRAM;
byte *RAM = NULL;
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
    if (RAMBanks && Value < RAMBanks) {
      if (Verbose) printf(">> Bank %i selected\n", Value);
      RAMMapper=Value;
      for (i=0xE000;i<0x10000;i+=256)
        ReadPage[i>>8]=WritePage[i>>8]=RAM+i-0x6000+Value*8192;
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
   if (PrnName) inputstatus&=0xFD;
   if (PrnType) inputstatus&=0xFB;
   return inputstatus;
  }
  case 3:       /* Scroll Register (T-version only) */
   return ScrollReg;
  case 4:       /* Reserved for I/O cartridge */
   break;
  case 5:       /* Beeper */
   return SoundReg;
  case 6:       /* Reserved for I/O cartridge */
   break;
  case 7:       /* DISAS (M-version only) */
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
   if (RAMBanks) {
    return RAMMapper;
   }
   break;
 }
 return 0xFF;
}

int InitRAM()
{
  int i;
  int RAMSize=RAMSizeKb*1024;
  RAMBanks = RAMMapper = 0;
  if (RAMSize<=16384) RAMSize=16384;
  else if (RAMSize<=32768) RAMSize=32768;
  else {
    RAMBanks = (RAMSize-32768)/8192;
    if (RAMBanks > 16) RAMBanks = 16; //keeping it realistic :)
    RAMSize=32768+RAMBanks*8192;
  }

  if (Verbose) printf ("Allocating memory: %uK RAM...",RAMSize/1024);
  if (RAM) free(RAM);
  RAM = malloc(RAMSize);
  if (!RAM) {
    if (Verbose) printf ("FAILED\n");
    return 0;
  }
  memset (RAM,0,RAMSize);
  if (Verbose) printf ("OK\n");

  for (i=0x0000;i<0xA000;i+=256) {
    if (i<RAMSize)
      ReadPage[(i+0x6000)>>8]=WritePage[(i+0x6000)>>8]=RAM+i;
    else {
      ReadPage[(i+0x6000)>>8]=NoRAMRead;
      WritePage[(i+0x6000)>>8]=NoRAMWrite;
    }
  }
  return 1;
}

/******************************************************************************/
/*** Allocate memory, load ROM images, initialise mapper, VDP and CPU and   ***/
/*** the emulation. This function returns 0 in case of a failure            ***/
/******************************************************************************/
word Exit_PC;
int InitP2000 (byte* monitor_rom, byte *cartridge_rom)
{
  FILE *f;
  int i,j;
  
  if (Verbose) printf ("Allocating memory: 20K ROM, 4K VRAM... ");
  ROM=malloc (0x5000);
  VRAM=malloc (0x1000);
  if (!ROM || !VRAM)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  memset (ROM,0xFF,0x5000);
  memset (VRAM,0,0x1000);
  if (Verbose) printf ("OK\n");

  for (i=0;i<256;++i)
  {
   ReadPage[i]=NoRAMRead;
   WritePage[i]=NoRAMWrite;
   NoRAMRead[i]=0xFF;
  }
  for (i=0;i<0x5000;i+=256)
  {
   ReadPage[i>>8]=ROM+i;
   WritePage[i>>8]=NoRAMWrite;
  }
  for (i=0x0000;i<0x0800;i+=256)
    ReadPage[(i+0x5000)>>8]=WritePage[(i+0x5000)>>8]=VRAM+i;

  if (!InitRAM()) return 0;

  if (monitor_rom)
    memcpy (ROM,monitor_rom,0x1000);
  else
  {
    if (Verbose) printf ("Loading ROMs:\n");
    if (Verbose) printf ("  Opening %s... ",ROMName);
    j=0;
    f=fopen(ROMName,"rb");
    if(f)
    {
    if(fread(ROM,1,0x1000,f)==0x1000) j=1;
    fclose (f);
    }
    if(Verbose) puts(j? "OK":"FAILED");
    if(!j) return 0;
  }

  if (Verbose) printf ("  Patching");
  for (j=0;ROMPatches[j];++j)
  {
   if (Verbose) printf ("...%04X",ROMPatches[j]);
   ROM[ROMPatches[j]+0]=0xED;
   ROM[ROMPatches[j]+1]=0xFE;
   ROM[ROMPatches[j]+2]=0xC9;
  }

  if (cartridge_rom) 
  {
      memcpy (ROM+0x1000,cartridge_rom,0x4000);
  }
  else 
  {
    if(Verbose) printf(" OK\n  Opening %s... ",CartName);
    j=0;
    f=fopen(CartName,"rb");
    if (f)
    {
    if (fread(ROM+0x1000,1,0x4000,f)) j=1;
    fclose(f);
    }
    if(Verbose) puts (j? "OK":"FAILED");
    /*  if(!j) return 0; */
  }

  if (!LoadFont(FontName)) 
    return 0;

  memset (KeyMap,0xFF,sizeof(KeyMap));
  Z80_Reset ();
  
  return 1;
}

int StartP2000 (void)
{
  if (Verbose) puts ("Starting P2000 emulation...");
  Exit_PC=Z80 ();
  if (Verbose) printf("EXITED at PC = %Xh\n",Exit_PC);
  return 1;
}

/****************************************************************************/
/*** Free memory allocated by InitP2000()                                 ***/
/****************************************************************************/
void TrashP2000 (void)
{
 if (TapeStream) fclose (TapeStream);
 if (PrnStream) fclose (PrnStream);
 if (ROM) free (ROM);
 if (VRAM) free (VRAM);
 if (RAM) free (RAM);
}

/****************************************************************************/
/*** Removes current cassette                                             ***/
/****************************************************************************/
void RemoveCassette()
{
  if (Verbose) printf ("Removing tape... ");
  if (TapeStream) fclose (TapeStream);
  TapeStream = NULL;
  TapeName = NULL;
  TapeProtect = 0;
  if (Verbose) puts ("OK");
}

/****************************************************************************/
/*** Insert cassette tape file.                                           ***/
/****************************************************************************/
void InsertCassette(const char *filename, FILE *f, int readOnly)
{
  if (Verbose) printf("Opening cassette file %s", filename);
  if (Verbose) printf(readOnly ? " (readonly)... " : "... ");
  if (!f) {
    if (Verbose) puts("FAILED");
    return;
  }

  static char _TapeName[FILENAME_MAX];
  strcpy (_TapeName,filename);
  TapeName=_TapeName;

  if (TapeStream) fclose (TapeStream); //close previous stream
  TapeProtect = readOnly;
  TapeStream = f;
  rewind (TapeStream);
  if (Verbose) puts("OK");
}

/****************************************************************************/
/*** Removes current cartridge                                            ***/
/****************************************************************************/
void RemoveCartridge()
{
  memset (ROM + 0x1000, 0xFF, 0x4000);
  ColdBoot = 1;
  Z80_Reset ();
}

/****************************************************************************/
/*** Insert cartridge file and resets Z80                                 ***/
/****************************************************************************/
void InsertCartridge(const char *filename, FILE *f)
{
  static char _CartName[FILENAME_MAX];
  int success=0;
  strcpy (_CartName,filename);
  CartName=_CartName;

  if(Verbose) printf(" OK\n  Opening cartridge %s... ",_CartName);
  if (f)
  {
    if (fread(ROM+0x1000,1,0x4000,f)) success=1;
    fclose(f);
    ColdBoot = 1;
    Z80_Reset ();
  }
  if(Verbose) puts (success? "OK":"FAILED");
}

/****************************************************************************/
/*** Refresh screen, check keyboard events and return interrupt id        ***/
/****************************************************************************/
int Z80_Interrupt(void)
{
 static int UCount=1;
 Keyboard ();
 FlushSound ();
 if (!--UCount)
 {
  UCount=UPeriod;
  RefreshScreen ();
 }
 SyncEmulation();
 if (NMI) {
  NMI=0; //reset flag
  return Z80_NMI_INT;
 }
 return (OutputReg&0x40) ? 0x00FF : Z80_IGNORE_INT;
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
 static byte tapebuf[1024+256] = {0};
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
   //every tape interaction sets/enables the keyboard interrupt
   OutputReg|=0x40; //set keyboard interrupt in output reg
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
      if (fseek (TapeStream,j+i*(1024+HEADER_SIZE)-1,SEEK_SET))
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
      if (fseek (TapeStream,j-i*(1024+HEADER_SIZE),SEEK_SET))
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
     /* Truncate the tape image */
     if (TapeStream && !TapeProtect)
     {
      if (ftruncate(fileno(TapeStream),ftell(TapeStream)) != 0)
        if (Verbose&4) puts ("EOT ftruncate error");
      Z80_WRMEM (caserror,0);
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
       for (j=0x00;j<0x20;++j)
        tapebuf[j+HEADER_OFFSET]=Z80_RDMEM (0x6030+j);
       l=m=Z80_RDWORD (lengte);
       if (l>1024) l=1024;
       Z80_WRWORD (lengte,m-l);
       for (j=0;j<l;++j)
        tapebuf[j+HEADER_SIZE]=Z80_RDMEM ((k+j)&0xFFFF);
       for (j=l;j<1024;++j)
        tapebuf[j+HEADER_SIZE]=0;
       k=(k+1024)&0xFFFF;
       if (!fwrite(tapebuf,1024+HEADER_SIZE,1,TapeStream))
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
       if (!fread(tapebuf,1024+HEADER_SIZE,1,TapeStream))
       {
        Z80_WRMEM (caserror,0x4D);
        break;
       }
       for (j=0;j<0x20;++j)
        Z80_WRMEM (0x6030+j,tapebuf[j+HEADER_OFFSET]);
       l=m=Z80_RDWORD (lengte);
       if (l>1024) l=1024;
       Z80_WRWORD (lengte,m-l);
       if (k>=0x5000 && k<0x6000)
       {
        /* We're reading to video memory. Emulate a loading picture */
        for (j=0;j<l;j+=80)
        {
         for (m=j;m<l && m<(j+80);++m)
          Z80_WRMEM((k+m)&0xFFFF,tapebuf[m+HEADER_SIZE]);
         RefreshScreen ();
         Keyboard ();
         if (!Z80_Running) return;
         Pause (200);
        }
       }
       else
       {
        for (j=0;j<l;++j)
         Z80_WRMEM((k+j)&0xFFFF,tapebuf[j+HEADER_SIZE]);
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
     if (Verbose&4) puts ("Tape status");
     i=Z80_In (0x20);
     if (ColdBoot && !TapeBootEnabled)
      i|=0x10;
     ColdBoot=0;
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
   if (PrnName) {
    if (!PrnStream) PrnStream= fopen (PrnName,"wb");
    if (PrnStream) {
      fputc (R->BC.B.l,PrnStream);
      if (R->BC.B.l==10) fflush (PrnStream);
    } else if (Verbose) printf("Failed to open printer stream to %s\n", PrnName);
   }
   R->IFF1=R->IFF2=1;
   break;
  default:
   printf ("Unknown patch called at %u\n",R->PC.W.l-2);
 }
}

// when doblank is 1, flashing characters are not displayed this refresh
static int doblank=1;

/****************************************************************************/
/*** Refresh screen (P2000T model)                                        ***/
/****************************************************************************/
void RefreshScreen_T(void)
{
  byte *S;
  int fg, bg, si, gr, fl, cg, FG, BG, conceal;
  int hg, hg_active, hg_c, hg_fg, hg_cg, hg_conceal;
  int x, y;
  int c;
  int lastcolor;
  int eor;
  int found_si;

  S = VRAM + ScrollReg;
  found_si = 0; // init to no double height codes found

  for (y = 0; y < 24; ++y)
  {
    /* Initial values:
       foreground=7 (white)
       background=0 (black)
       normal height
       graphics off
       flashing off
       contiguous graphics
       hold graphics off
       reveal display */
    fg = 7;
    bg = 0;
    si = 0; // superimpose
    gr = 0;
    fl = 0; // flashing
    cg = 1;
    hg = 0;
    conceal = 0;
    hg_active = hg; // init the HG mode settings
    hg_c = SPACE;
    hg_cg = cg;
    hg_fg = fg;
    hg_conceal = conceal;
    lastcolor = fg;
    for (x = 0; x < 40; ++x)
    {
      /* Get character */
      c = S[x] & 0x7f;
      /* If bit 7 is set, invert the colours */
      eor = S[x] & 0x80;
      if (!(c & 0x60))
      {
        /* Control code found. Parse it */
        switch (c & 0x1f)
        {
        /* New text colour () */
        case 0x01: // red
        case 0x02: // green
        case 0x03: // yellow
        case 0x04: // blue
        case 0x05: // magenta
        case 0x06: // cyan
        case 0x07: // white
          fg = lastcolor = c & 0x0f;
          gr = conceal = hg = 0;
          break;
        /* New graphics colour */
        case 0x11: // red
        case 0x12: // green
        case 0x13: // yellow
        case 0x14: // blue
        case 0x15: // magenta
        case 0x16: // cyan
        case 0x17: // white
          fg = lastcolor = c & 0x0f;
          gr = 1;
          conceal = 0;
          break;
        /* Flash */
        case 0x08:
          fl = 1;
          break;
        /* Steady */
        case 0x09:
          fl = 0;
          break;
        /* End box (?) */
        case 0x0a:
          break;
        /* Start box (?) */
        case 0x0b:
          break;
        /* Normal height */
        case 0x0c:
          si = 0;
          break;
        /* Double height */
        case 0x0d:
          si = 1;
          if (!found_si)
            found_si = 1;
          break;
        /* reserved for compatability reasons; these are still graphic mode */
        case 0x00:
        case 0x0e:
        case 0x0f:
        case 0x10:
        case 0x1b:
          break;
        /* conceal display */
        case 0x18:
          conceal = 1;
          break;
        /* contiguous graphics */
        case 0x19:
          cg = 1;
          break;
        /* separated graphics */
        case 0x1a:
          cg = 0;
          break;
        /* black background */
        case 0x1c:
          bg = 0;
          break;
        /* new background */
        case 0x1d:
          bg = lastcolor;
          break;
        /* hold graphics */
        case 0x1e:
          if (!hg)
          {
            hg = 1;
            if (gr) 
            {
              hg_active = 1;
              hg_fg = fg;
            }
          }
          break;
        /* release graphics */
        case 0x1f:
          hg = 0;
          break;
        }
        c = SPACE; // control chars are displayed as space by default
      }
      else 
      {
        hg_c = c;
        hg_cg = cg; // hold display of seperated/contiguous mode
        hg_conceal = conceal;
      }

      if (hg_active)
        c = hg_c;

      /* Check for flashing characters and concealed display */
      if ((fl && doblank) || (hg_active ? hg_conceal : conceal))
        c = SPACE;

      /* Check if graphics are on */
      if ((gr || hg_active) && (c & 0x20)) // c from 32..63
      {
        c += (c & 0x40) ? 64 : 96;
        if (!(hg_active ? hg_cg : cg))
          c += 64;
      }
      /* If double height code on previous line and double height
         is not set, display a space character */
      if (found_si == 2 && !si)
        c = SPACE;

      /* Get the foreground and background colours */
      FG = (hg_active ? hg_fg : fg);
      BG = bg;
      if (eor)
      {
        FG = FG ^ 7;
        BG = BG ^ 7;
      }
      /* Put the character in the screen buffer */
      PutChar(x, y, c - 32, FG, BG, (si ? found_si : 0));

      // update HG mode
      hg_active = (hg && gr);
      if (gr)
      {
        hg_fg = fg;
        hg_conceal = conceal;
      }
    }

    /* Update the double height state
       If there was a double height code on this line, do not
       update the character pointer. If there was one on the
       previous line, add two lines to the character pointer */
    if (found_si)
    {
      if (++found_si == 3)
      {
        S += 160;
        found_si = 0;
      }
    }
    else
      S += 80; // move to next line in VRAM
  }
}

/****************************************************************************/
/*** Refresh screen. This function updates the blanking state and then    ***/
/*** calls RefreshScreen_T() and finally it calls PutImage() to copy the  ***/
/*** off-screen buffer to the actual display                              ***/
/****************************************************************************/
void RefreshScreen(void)
{
  static int BCount = 0;
  // Update blanking count
  // flashing is on for 48 cycles and off for 16 cycles (64-48)
  BCount++;
  if (BCount == 48 / UPeriod) doblank = 1;
  if (BCount == 64 / UPeriod) doblank = BCount = 0;
  // Update the screen buffer
  RefreshScreen_T();
  // Put the image on the screen
  PutImage();
}