/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                MSDOS.c                               ***/
/***                                                                      ***/
/*** This file contains the MS-DOS drivers. Sound implementation is in    ***/
/*** SB.c, some miscellanious assembly routines are in Asm.S              ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include "P2000.h"
#include "MSDOS.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/farptr.h>
#include <go32.h>
#include <dpmi.h>
#include <pc.h>
#include <dos.h>
#include <crt0.h>
#include <math.h>
#include <conio.h>

int _crt0_startup_flags = _CRT0_FLAG_NONMOVE_SBRK |
                          _CRT0_FLAG_LOCK_MEMORY  |
                          _CRT0_FLAG_DROP_EXE_SUFFIX;

#define NUM_STACKS      16         /* Number of IRQ stacks                  */
#define STACK_SIZE      8192       /* Size of each IRQ stack                */

char *Title="M2000 MS-DOS 0.6";    /* Title for -help output                */

int videomode;                     /* T emulation only: 0=256x240 1=640x480 */ 
static int *OldCharacter;          /* Holds characters on the screen        */
static int *CharacterCache;        /* Current cache contents                */
static byte *FontBuf;              /* Pointer to font used                  */
word cs_alias;                     /* Used by assembly routines in Asm.S    */
static int nOldVideoMode;          /* Original video mode                   */
volatile unsigned TimerCount=0;    /* Incremented every timer tick          */
unsigned short DosSelector;        /* Selector for first megabyte           */
unsigned ReadTimerMin=47694;       /* Minimum ammount of timer ticks        */
                                   /* between interrupts                    */
static int OldTimer=0;             /* Value of timer at previous interrupt  */
static int NewTimer=0;             /* New value of the timer                */
int soundmode=255;                 /* Sound mode, 255=auto-detect           */
int joymode=1;                     /* If 0, do not use joystick             */
static int soundoff=0;             /* If 1, sound is turned off             */
static int in_options_dialogue=0;  /* If 1, pass keyboard events to the old */
                                   /* keyboard interrupt routine            */

static byte regs_256x240[25]=      /* CRTC regs for a 256x240, 8bpp mode    */
{
  0xE3,
  0x4F,0x3F,0x40,0x92,
  0x44,0x10,0x0D,0x3E,
  0x00,0x41,0x00,0x00,
  0x00,0x00,0x00,0x00,
  0xEA,0xAC,0xDF,0x20,
  0x40,0xE7,0x06,0xA3
};

static byte VGA_Palette[8*3] =     /* SAA5050 palette                       */
{
   0x00,0x00,0x00 , 0xFF,0x00,0x00 , 0x00,0xFF,0x00 , 0xFF,0xFF,0x00 ,
   0x00,0x00,0xFF , 0xFF,0x00,0xFF , 0x00,0xFF,0xFF , 0xFF,0xFF,0xFF
};

struct font2screen_struct
{
 unsigned low;
 unsigned high;
};
static struct font2screen_struct *font2screen=NULL;

char szBitmapFile[256];            /* Next screen shot file                 */

static volatile int PausePressed=0;      /* 1 if pause key is pressed       */
static volatile byte keybstatus[128];    /* 1 if a certain key is pressed   */
static volatile byte extkeybstatus[128]; /* Holds the extended keys         */
static _go32_dpmi_seginfo keybirq;       /* Keyboard interrupt id           */
static int makeshot=0;                   /* 1 -> take a screen shot         */
static int calloptions=0;                /* 1 -> call OptionsDialogue()     */

typedef struct joyposstruct
{
 unsigned x,y;
} joypos;

static int gotjoy=0;                     /* 1 if a joystick is present      */
static joypos joycentre;                 /* Centre position                 */

static int keymask[256]=
{
 0x000,0x000,0x5BF,0x77F,0x0EF,0x07F,0x0DF,0x0FD,
 0x0BF,0x6BF,0x5FD,0x5DF,0x57F,0x8EF,0x5EF,0x1FE,
 0x0F7,0x4F7,0x4EF,0x47F,0x4DF,0x4FD,0x4BF,0x8BF,
 0x6FD,0x6DF,0x67F,0x7EF,0x6EF,0x0FE,0x4FB,0x1F7,
 0x1EF,0x17F,0x1DF,0x1FD,0x1BF,0x7BF,0x8FD,0x8DF,
 0x87F,0x4FE,0x9FE,0x2EF,0x1FB,0x3F7,0x3EF,0x37F,
 0x3DF,0x3FD,0x3BF,0x2BF,0x7FD,0x7DF,0x97F,0x5FE,
 0x0FB,0x2FD,0x3FE,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x5F7,0x000,0x6F7,
 0x6FB,0x6FE,0x5F7,0x8F7,0x8FB,0x8FE,0x5FB,0x7F7,
 0x7FB,0x7FE,0x2F7,0x2FB,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,

 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x2FE,0x27F,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x5FB,0x000,0x000,
 0x2DF,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x0FB,0x000,0x000,0x0FE,0x000,0x27F,0x000,0x000,
 0x2DF,0x000,0x000,0x3FB,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000
};

/****************************************************************************/
/*** This function is called to set a number of palette RGB values        ***/
/****************************************************************************/
static void VGA_SetPalette (unsigned char *pal, int nColours)
{
 int i;
 __disable ();
 while ((inportb(0x3DA)&0x08)!=0); /* Wait until vertical retrace is off    */
 while ((inportb(0x3DA)&0x08)==0); /* Now wait until it is on               */
 outportb (0x3C8,0);               /* Start updating palette                */
 for (i=0;i<nColours*3;++i)
  outportb(0x3C9,pal[i]/4);
 __enable ();
}

/****************************************************************************/
/*** This function resets the videomode to the one detected at startup    ***/
/****************************************************************************/
void VGA_Reset (void)
{
 __dpmi_regs r;
 r.x.ax=(short)nOldVideoMode;
 __dpmi_int (0x10, &r);
}

/****************************************************************************/
/*** This function sets the VGA CRTC regs                                 ***/
/****************************************************************************/
static void VGA_SetRegs (byte *regs)
{
 int i;
 __disable ();
 while (inportb(0x3DA)&8);
 while ((inportb(0x3DA)&8)==0);
 outportw (0x3C4,0x100);                        /* sequencer reset          */
 outportb (0x3C2,regs[0]);                      /* misc. output reg         */
 outportw (0x3C4,0x300);                        /* clear sequencer reset    */
 outportw (0x3D4,((regs[0x12]&0x7F)<<8)+0x11);  /* deprotect regs 0-7       */
 for (i=0;i<24;++i)
  outportw (0x3D4,(regs[i+1]<<8)+i);
 __enable ();
}

/****************************************************************************/
/*** This function initialises the VGA display adapter                    ***/
/****************************************************************************/
static int VGA_Init (void)
{
 __dpmi_regs r;
 int i;
 r.x.ax=0x0F00;
 __dpmi_int (0x10,&r);
 nOldVideoMode=(int)(r.h.al & 0x7F);
 if (!P2000_Mode)
 {
  if (!videomode)
  {
   /* 256x240, 8bpp */
   r.x.ax=0x0013;
   __dpmi_int (0x10,&r);
   VGA_SetRegs (regs_256x240);
   VGA_SetPalette (VGA_Palette,8);
  }
  else
  {
   /* 640x480, 4 bit planes */
   r.x.ax=0x0012;
   __dpmi_int (0x10,&r);
   for (i=0;i<8;++i)
   {
    r.x.ax=0x1000;
    r.x.bx=i+(i<<8);
    __dpmi_int (0x10,&r);
   }
   VGA_SetPalette (VGA_Palette,8);
  }
 }
 else
 {
  /* 640x480, monochrome */
  r.x.ax=0x0012;
  __dpmi_int (0x10,&r);
  VGA_Palette[0]=VGA_Palette[1]=VGA_Palette[2]=0;
  VGA_Palette[3]=VGA_Palette[4]=VGA_Palette[5]=255;
 }
 return 1;
}

/****************************************************************************/
/*** This function is called by the screen refresh drivers to copy the    ***/
/*** off-screen display buffer to the actual screen. It is a no-op in the ***/
/*** MS-DOS implementation                                                ***/
/****************************************************************************/
static void PutImage (void)
{
}

/****************************************************************************/
/*** This function is called when a key is pressed or released            ***/
/****************************************************************************/
static int keyb_interrupt (void)
{
 unsigned code;
 static int extkey;
 int tmp;
 tmp=0;
 code=inportb (0x60);              /* get scancode                          */
 if (code<0xE0)                    /* ignore codes >0xE0                    */
 {
  if (code & 0x80)                 /* key is released                       */
  {
   code&=0x7F;
   if (extkey)
   {
    extkeybstatus[code]=0;
    tmp=keymask[code+128];
   }
   else
   {
    keybstatus[code]=0;
    tmp=keymask[code];
   }
   if (tmp)
    KeyMap[tmp>>8]|=(~(tmp&0xFF));
  }
  else                             /* key is pressed                        */
  {
   if (extkey)
   {
    if (!extkeybstatus[code])
    {
     extkeybstatus[code]=1;
     if (in_options_dialogue)      /* jump to old interrupt handler         */
      return 1;
     PausePressed=0;
     tmp=keymask[code+128];
     switch (code)
     {
     }
    }
   }
   else
   {
    if (!keybstatus[code])
    {
     keybstatus[code]=1;
     if (in_options_dialogue)
      return 1;
     tmp=keymask[code];
     if (code==VK_F8 || code==VK_F9)
     {
      if (PausePressed)
       PausePressed=0;
      else
      {
       if (code==VK_F8)
        PausePressed=2;
       else
        PausePressed=1;
      }
     }
     else
      PausePressed=0;
     switch (code)
     {
      case VK_Escape:
      case VK_F10:
       Z80_Running=0;
       break;
#ifdef DEBUG
      case VK_F4:
       Z80_Trace=!Z80_Trace;
       break;
#endif
      case VK_F7:
       makeshot=1;
       break;
      case VK_F5:
       soundoff=(!soundoff);
       break;
      case VK_F11:
       SB_DecreaseVolume ();
       break;
      case VK_F12:
       SB_IncreaseVolume ();
       break;
      case VK_F6:
       calloptions=1;
       break;
     }
    }
   }
   if (tmp)
    KeyMap[tmp>>8]&=(tmp&0xFF);
  }
  extkey=0;
 }
 else
  if (code==0xE0)
   extkey=1;
 code=inportb (0x61);              /* acknowledge interrupt                  */
 outportb (0x61,code | 0x80);
 outportb (0x61,code);
 outportb (0x20,0x20);
 return 0;
}

/****************************************************************************/
/*** Initialise the keyboard handler                                      ***/
/****************************************************************************/
static int Keyb_Init (void)
{
 keybirq.pm_offset=(int)keyb_interrupt;
 keybirq.pm_selector=_go32_my_cs();
 SetInt (9,&keybirq);
 return 1;
}

/****************************************************************************/
/*** Reset the keyboard interrupt to its  old value                       ***/
/****************************************************************************/
static void Keyb_Reset (void)
{
 ResetInt (9,&keybirq);
}

/****************************************************************************/
/*** Get the current joystick position                                    ***/
/****************************************************************************/
static int _JoyGetPos (joypos *jp)
{
 unsigned tmp;
 tmp=JoyGetPos ();
 jp->x=(unsigned) (tmp&0xFFFF);
 if (jp->x>=10000)
  return 0;
 jp->y=(unsigned) (tmp>>16);
 if (jp->y>=10000)
  return 0;
 return 1;
}

/****************************************************************************/
/*** Detect joystick and initialise structures                            ***/
/****************************************************************************/
static int Joy_Init (void)
{
 joypos jp;
 if (!_JoyGetPos (&jp))
  return 0;
 joycentre.x=jp.x;
 joycentre.y=jp.y;
 gotjoy=1;
 return 1;
}

/****************************************************************************/
/*** Check for joystick events                                            ***/
/****************************************************************************/
static int Joy_Check (void)
{
 joypos jp;
 int J;
 int joystatus;
 if (!gotjoy)
  return 0;
 J=0;
 _JoyGetPos (&jp);
 joystatus=inportb (0x201);
 if (jp.x<(joycentre.x/2))
  J|=0x40;
 else
  if (jp.x>(joycentre.x*3/2))
   J|=0x80;
 if (jp.y<(joycentre.y/2))
  J|=0x10;
 else
  if (jp.y>((joycentre.y*3)/2))
   J|=0x20;
 if ((joystatus&0x10)==0)
  J|=0x01;
 if ((joystatus&0x20)==0)
  J|=0x02;
 return J;
}

/****************************************************************************/
/*** Deallocate resources taken by InitMachine()                          ***/
/****************************************************************************/
void TrashMachine(void)
{
 VGA_Reset ();
 if(Verbose) printf("\n\nShutting down...\n");
 if (soundmode==2) SB_Reset ();
 RestoreTimer ();
 Keyb_Reset ();
 ExitStacks ();
 if (font2screen) free (font2screen);
 if (FontBuf) free (FontBuf);
 if (OldCharacter) free (OldCharacter);
 if (CharacterCache) free (CharacterCache);
}

/****************************************************************************/
/*** Update szBitmapFile[]                                                ***/
/****************************************************************************/
static int NextBitmapFile ()
{
 char *p;
 p=szBitmapFile+strlen(szBitmapFile)-5;
 if (*p=='9')
 {
  *p='0';
  --p;
  if (*p=='9')
  {
   *p='0';
   --p;
   (*p)++;
  }
  else
  {
   (*p)++;
   if (*p=='0')
    return 0;
  }
 }
 else
  (*p)++;
 return 1;
}

/****************************************************************************/
/*** Initialise all resources needed by the MS-DOS implementation         ***/
/****************************************************************************/
int InitMachine(void)
{
 int c,i,j;
 FILE *bitmapfile;
 if (Verbose)
  printf ("Initialising MS-DOS drivers:\n");
 cs_alias=_my_ds();
 DosSelector=_dos_ds;
 if (Verbose)
  printf ("Allocating stack space... ");
 i=InitStacks (NUM_STACKS,STACK_SIZE);
 if (!i)
 {
  if (Verbose) puts ("FAILED");
  return 0;
 }
 if (Verbose) printf ("OK\nAllocating cache buffers... ");
 if (P2000_Mode) i=80*24;
 else i=40*24;
 OldCharacter=malloc (i*sizeof(int));
 if (!OldCharacter)
 {
  if (Verbose) puts ("FAILED");
  return(0);
 }
 memset (OldCharacter,-1,i*sizeof(int));
 if (!P2000_Mode && videomode)
 {
  i=(96+128)*3*sizeof(int);
  CharacterCache=malloc(i);
  if (!CharacterCache)
  {
   if (Verbose) puts ("FAILED");
   return(0);
  }
  memset (CharacterCache,-1,i);
 }
 if (!P2000_Mode && !videomode)
 {
  if (Verbose)
   printf ("OK\nAllocating conversion buffer... ");
  font2screen=malloc(8*8*64*sizeof(struct font2screen_struct));
  if (!font2screen)
  {
   if (Verbose) puts ("FAILED");
   return 0;
  }
  if (Verbose)
   printf ("OK\nInitialising conversion buffer... ");
  for (i=0;i<8;++i)
   for (j=0;j<8;++j)
    for (c=0;c<64;++c)
    {
     unsigned a;
     a=0;
#ifdef LSB_FIRST
     if (c&0x08) a|=i; else a|=j;
     if (c&0x04) a|=(i<<8); else a|=(j<<8);
     if (c&0x02) a|=(i<<16); else a|=(j<<16);
     if (c&0x01) a|=(i<<24); else a|=(j<<24);
#else
     if (c&0x01) a|=i; else a|=j;
     if (c&0x02) a|=(i<<8); else a|=(j<<8);
     if (c&0x04) a|=(i<<16); else a|=(j<<16);
     if (c&0x08) a|=(i<<24); else a|=(j<<24);
#endif
     font2screen[i*8*64+j*64+c].high=a;
     a=0;
#ifdef LSB_FIRST
     if (c&0x20) a|=i; else a|=j;
     if (c&0x10) a|=(i<<8); else a|=(j<<8);
#else
     if (c&0x10) a|=i; else a|=j;
     if (c&0x20) a|=(i<<8); else a|=(j<<8);
#endif
     font2screen[i*8*64+j*64+c].low=a;
    }
 }
 if (Verbose) puts ("OK");
 if (joymode)
 {
  if (Verbose) printf ("Detecting joystick... ");
  i=Joy_Init ();
  if (Verbose) puts ((i)? "Found":"Not found");
 }
 strcpy (szBitmapFile,"M2000.BMP");
 while ((bitmapfile=fopen(szBitmapFile,"rb"))!=NULL)
 {
  fclose (bitmapfile);
  if (!NextBitmapFile())
   break;
 }
 if (Verbose)
  printf ("Next screenshot will be %s\n",szBitmapFile);
 if (soundmode)
 {
  if (soundmode==2 || soundmode==255)
   if (!SB_Init())
   {
    if (soundmode==2) soundmode=0;
   }
   else
    soundmode=2;
 }
 if (Verbose) printf ("Initialising timer...\n");
 ReadTimerMin=1192380/IFreq;
 StartTimer ();
 OldTimer=ReadTimer ();
 if (Verbose) printf ("Initialising keyboard...\n");
 Keyb_Init ();
 if (Verbose)
 {
  fprintf (stderr,"Press space to run rom code...\n");
  while (!keybstatus[VK_Space]);
 }
 VGA_Init ();
 return(1);
}

/****************************************************************************/
/*** Load specified font and convert it if necessary                      ***/
/****************************************************************************/
int LoadFont (char *filename)
{
 FILE *F;
 int i,j,k,l,c,d;
 char *TempBuf;
 if (Verbose) printf("Loading font %s...\n",filename);
 if (Verbose) printf ("  Allocating memory... ");
 if (!FontBuf)
 {
  FontBuf=malloc (8960);
  if (!FontBuf)
  {
   if (Verbose) puts ("FAILED");
   return 0;
  }
 }
 TempBuf=malloc (2240);
 if (!TempBuf)
 {
  if (Verbose) puts ("FAILED");
  return 0;
 }
 if (Verbose) puts ("OK");
 if (Verbose) printf ("  Opening... ");
 i=0;
 F=fopen(filename,"rb");
 if (F)
 {
  if (Verbose) printf ("Reading... ");
  if (fread(TempBuf,2240,1,F)) i=1;
  fclose(F);
 }
 if(Verbose) puts ((i)? "OK":"FAILED");
 if(!i) return 0;
 if (Verbose) printf ("  Converting... ");
 if (P2000_Mode)
 {
  /* Stretch characters to 8x10 */
  for (i=0;i<96*10;++i)
   FontBuf[i]=TempBuf[i]<<1;
  for (i=96*10;i<(96+128)*10;++i)
  {
   c=TempBuf[i];
   FontBuf[i]=((c&7)<<1) | (c&1) | ((c&0x38)<<2) | (c&0x10);
  }
 }
 else if (videomode)
 {
  /* Stretch characters to 16x20 */
  for (i=0;i<96*10;i+=10)
  {
   for (j=0;j<10;++j)
   {
    c=TempBuf[i+j]; k=0;
    if (c&0x10) k|=0x7000;
    if (c&0x08) k|=0x0E00;
    if (c&0x04) k|=0x01C0;
    if (c&0x02) k|=0x0038;
    if (c&0x01) k|=0x0007;
    l=k;
    if (j!=9)
    {
     d=TempBuf[i+j+1];
     if ((c&0x10) && (d&0x08) && !(d&0x10)) l|=0x0800;
     if ((c&0x08) && (d&0x04) && !(d&0x08)) l|=0x0100;
     if ((c&0x04) && (d&0x02) && !(d&0x04)) l|=0x0020;
     if ((c&0x02) && (d&0x01) && !(d&0x02)) l|=0x0004;
     if ((d&0x10) && (c&0x08) && !(d&0x08)) l|=0x1000;
     if ((d&0x08) && (c&0x04) && !(d&0x04)) l|=0x0200;
     if ((d&0x04) && (c&0x02) && !(d&0x02)) l|=0x0040;
     if ((d&0x02) && (c&0x01) && !(d&0x01)) l|=0x0008;
    }
    if (j!=0)
    {
     d=TempBuf[i+j-1];
     if ((c&0x10) && (d&0x08) && !(d&0x10)) k|=0x0800;
     if ((c&0x08) && (d&0x04) && !(d&0x08)) k|=0x0100;
     if ((c&0x04) && (d&0x02) && !(d&0x04)) k|=0x0020;
     if ((c&0x02) && (d&0x01) && !(d&0x02)) k|=0x0004;
     if ((d&0x10) && (c&0x08) && !(d&0x08)) k|=0x1000;
     if ((d&0x08) && (c&0x04) && !(d&0x04)) k|=0x0200;
     if ((d&0x04) && (c&0x02) && !(d&0x02)) k|=0x0040;
     if ((d&0x02) && (c&0x01) && !(d&0x01)) k|=0x0008;
    }
    FontBuf[(i+j)*4+0]=k>>8; FontBuf[(i+j)*4+2]=l>>8;
    FontBuf[(i+j)*4+1]=k>>0; FontBuf[(i+j)*4+3]=l>>0;
   }
  }
  for (i=96*10;i<(96+128)*10;++i)
  {
   c=TempBuf[i]; j=0;
   if (c&0x20) j|=0xC000;
   if (c&0x18) j|=0x3F00;
   if (c&0x04) j|=0x00C0;
   if (c&0x03) j|=0x003F;
   FontBuf[i*4]=FontBuf[i*4+2]=j>>8; FontBuf[i*4+1]=FontBuf[i*4+3]=j;
  }
 }
 else
 {
  /* Clear the upper two bits of each character */
  for (i=0;i<(96+128)*10;++i)
   FontBuf[i]=TempBuf[i]&0x3F;
 }
 if (Verbose) puts ("OK");
 free (TempBuf);
 return 1;
}

/****************************************************************************/
/*** This function is called at every interrupt to update the P2000       ***/
/*** keyboard matrix and check for special events                         ***/
/****************************************************************************/
void Keyboard(void)
{
 int i;
 if (PausePressed)
 {
  i=0;
  if (PausePressed==2)             /* Blank screen                          */
  {
   i=1;
   inportb (0x3BA);
   inportb (0x3DA);
   outportb (0x3C0,0);
  }
  while (PausePressed);
  if (i)
  {
   inportb (0x3BA);
   inportb (0x3DA);
   outportb (0x3C0,0x20);
  }
  OldTimer=ReadTimer ();
 }
 if (makeshot)
 {
  /* Copy screen contents to buffer and write bitmap file */
  byte *p;
  sound (500);
  i=(P2000_Mode)? (640*480/8):(videomode)? (640*480*7/8):(256*240);
  p=malloc (i);
  if (p)
  {
   if (!P2000_Mode)
   {
    if (!videomode)
    {
     /* 256x240, 8bpp. Convert to 4bpp to decrease file size */
     dosmemget (0xA0000,i,p);
     for (i=0;i<256*240/2;++i)
      p[i]=(p[i*2]<<4)|(p[i*2+1]);
     WriteBitmap (szBitmapFile,4,8,256,240,240,p+4,VGA_Palette);
    }
    else
    {
     byte *q;
     int a,b,c;
     /* 640x480, 4bpp - convert bitplaned data first */
     outportw (0x3CE,0x0004);      /* Select bit plane 1                    */
     dosmemget (0xA0000,640*480/8,p);
     outportw (0x3CE,0x0104);      /* Select bit plane 2                    */
     dosmemget (0xA0000,640*480/8,p+640*480/8);
     outportw (0x3CE,0x0204);      /* Select bit plane 3                    */
     dosmemget (0xA0000,640*480/8,p+640*480*2/8);
     for (i=0,q=p+640*480*3/8;i<640*480/8;++i,q+=4)
     {
      a=p[i]; b=p[640*480/8+i]; c=p[640*480*2/8+i];
      q[0]=((a&0x80)>>3) | ((b&0x80)>>2) | ((c&0x80)>>1) |
           ((a&0x40)>>6) | ((b&0x40)>>5) | ((c&0x40)>>4);
      q[1]=((a&0x20)>>1) | ((b&0x20)>>0) | ((c&0x20)<<1) |
           ((a&0x10)>>4) | ((b&0x10)>>3) | ((c&0x10)>>2);
      q[2]=((a&0x08)<<1) | ((b&0x08)<<2) | ((c&0x08)<<3) |
           ((a&0x04)>>2) | ((b&0x04)>>1) | ((c&0x04)>>0);
      q[3]=((a&0x02)<<3) | ((b&0x02)<<4) | ((c&0x02)<<5) |
           ((a&0x01)>>0) | ((b&0x01)<<1) | ((c&0x01)<<2);
     }
     WriteBitmap (szBitmapFile,4,8,640,640,480,p+640*480*3/8,VGA_Palette);
    }
   }
   else
   {
    /* 640x480, monochrome */
    dosmemget (0xA0000,i,p);
    WriteBitmap (szBitmapFile,1,2,640,640,480,p,VGA_Palette);
   }
   free (p);
   NextBitmapFile ();
  }
  nosound ();
  makeshot=0;
 }
 if (calloptions)
 {
  calloptions=0;
  /* switch to text mode */
  VGA_Reset ();
  /* Pass keyboard events to original handler */
  in_options_dialogue=1;
  /* call the options dialogue function */
  OptionsDialogue ();
  /* Switch back to our own keyboard handler */
  in_options_dialogue=0;
  /* switch back to graphics mode */
  VGA_Init ();
  /* Flush caches and put the image back on the screen */
  memset (OldCharacter,-1,((P2000_Mode)? 80:40)*24*sizeof(int));
  if (!P2000_Mode && videomode)
   memset (CharacterCache,-1,(96+128)*3*sizeof(int));
  RefreshScreen ();
 }
 /* Keyboard is checked in the interrupt routine
    Only check for joystick events here */
 i=Joy_Check ();
 if (i&0x40)
  KeyMap[0]&=0xFE;
 else if (!keybstatus[VK_Ctrl] && !extkeybstatus[VK_Left])
  KeyMap[0]|=0x01;
 if (i&0x80)
  KeyMap[2]&=0x7F;
 else if (!extkeybstatus[VK_Ctrl] && !extkeybstatus[VK_Right])
  KeyMap[2]|=0x80;
 if (i&0x10)
  KeyMap[0]&=0xFB;
 else if (!keybstatus[VK_Alt] && !extkeybstatus[VK_Up])
  KeyMap[0]|=0x04;
 if (i&0x20)
  KeyMap[2]&=0xDF;
 else if (!extkeybstatus[VK_Alt] && !extkeybstatus[VK_Down])
  KeyMap[2]|=0x20;
 if (i&0x0F)
  KeyMap[2]&=0xFD;
 else if (!keybstatus[VK_Space])
  KeyMap[2]|=0x02;
}

/****************************************************************************/
/*** This function is called every interrupt to flush sound pipes and     ***/
/*** sync emulation                                                       ***/
/****************************************************************************/
void FlushSound (void)
{
 if (soundmode==2)
  SB_FlushSound ();                /* Syncs to the SoundBlaster interrupt   */
 else
 {
  if (Sync)
  {
   static int too_slow=0;
   do
    NewTimer=ReadTimer ();
   while ((NewTimer-OldTimer)<0);
   OldTimer+=ReadTimerMin;
   if ((OldTimer-NewTimer)<0)
   {
    if (++too_slow>10)
    {
     OldTimer=ReadTimer();
     too_slow=0;
    }
   }
   else
    too_slow=0;
  }
 }
}

/****************************************************************************/
/*** This function is called when the sound register is written to        ***/
/****************************************************************************/
void Sound (int toggle)
{
 int tmp;
 if (!soundmode || soundoff)
  return;
 if (soundmode==2)
 {
  SB_Sound (toggle);
  return;
 }
 tmp=inportb(0x61)&0xFC;
 if (toggle) tmp|=2;
 outportb(0x61,tmp);
}

/****************************************************************************/
/*** Pause specified ammount of time                                      ***/
/****************************************************************************/
void Pause (int ms)
{
 int i,j;
 j=ReadTimer();
 i=j+ms*1192;
 while ((j-i)<0) j=ReadTimer();
 OldTimer=j;
}

/****************************************************************************/
/*** Put a character on the screen for P2000M emulation mode. This        ***/
/*** function checks if the character isn't already put on the same       ***/
/*** position by an earlier screen refresh                                ***/
/****************************************************************************/
static inline void PutChar_M (int x,int y,int c,int eor,int ul)
{
 int K;
 unsigned p;
 K=c+(eor<<8)+(ul<<16);
 if (K==OldCharacter[y*80+x]) return;
 OldCharacter[y*80+x]=K;
 p=0xA0000+y*1600+x;
 _farsetsel (DosSelector);
 c*=10;
 if (eor)
 {
  K=FontBuf[c];
  _farnspokeb (p,~K);
  _farnspokeb (p+80,~K);
  K=FontBuf[c+1];
  _farnspokeb (p+160,~K);
  _farnspokeb (p+80+160,~K);
  K=FontBuf[c+2];
  _farnspokeb (p+320,~K);
  _farnspokeb (p+80+320,~K);
  K=FontBuf[c+3];
  _farnspokeb (p+480,~K);
  _farnspokeb (p+80+480,~K);
  K=FontBuf[c+4];
  _farnspokeb (p+640,~K);
  _farnspokeb (p+80+640,~K);
  K=FontBuf[c+5];
  _farnspokeb (p+800,~K);
  _farnspokeb (p+80+800,~K);
  K=FontBuf[c+6];
  _farnspokeb (p+960,~K);
  _farnspokeb (p+80+960,~K);
  K=FontBuf[c+7];
  _farnspokeb (p+1120,~K);
  _farnspokeb (p+80+1120,~K);
  K=FontBuf[c+8];
  _farnspokeb (p+1280,~K);
  _farnspokeb (p+80+1280,~K);
  if (ul) K=255; else K=FontBuf[c+9];
  _farnspokeb (p+1440,~K);
  _farnspokeb (p+80+1440,~K);
 }
 else
 {
  K=FontBuf[c];
  _farnspokeb (p,K);
  _farnspokeb (p+80,K);
  K=FontBuf[c+1];
  _farnspokeb (p+160,K);
  _farnspokeb (p+80+160,K);
  K=FontBuf[c+2];
  _farnspokeb (p+320,K);
  _farnspokeb (p+80+320,K);
  K=FontBuf[c+3];
  _farnspokeb (p+480,K);
  _farnspokeb (p+80+480,K);
  K=FontBuf[c+4];
  _farnspokeb (p+640,K);
  _farnspokeb (p+80+640,K);
  K=FontBuf[c+5];
  _farnspokeb (p+800,K);
  _farnspokeb (p+80+800,K);
  K=FontBuf[c+6];
  _farnspokeb (p+960,K);
  _farnspokeb (p+80+960,K);
  K=FontBuf[c+7];
  _farnspokeb (p+1120,K);
  _farnspokeb (p+80+1120,K);
  K=FontBuf[c+8];
  _farnspokeb (p+1280,K);
  _farnspokeb (p+80+1280,K);
  if (ul) K=255; else K=FontBuf[c+9];
  _farnspokeb (p+1440,K);
  _farnspokeb (p+80+1440,K);
 }
}

/****************************************************************************/
/*** Copy 16 pixels at [p] to [q]. Assume write mode 1                    ***/
/****************************************************************************/
static inline void vga_copymem (unsigned p,unsigned q)
{
 _farnspokeb(q,_farnspeekb(p));
 _farnspokeb(q+1,_farnspeekb(p+1));
}

/****************************************************************************/
/*** Latch the address first, then do a write                             ***/
/****************************************************************************/
static inline void vga_writemem (unsigned p,byte v)
{
 asm __volatile__ (
 " .byte 0x64           \n"
 " movb (%%edx),%%ah    \n"
 " .byte 0x64           \n"
 " movb %%al,(%%edx)    \n"
 :
 :"a" (v),
  "d" (p)
 :"eax"
 );
}

/****************************************************************************/
/*** Put character row in cache memory                                    ***/
/****************************************************************************/
static inline void putchar_t_hires (unsigned p,int c,int fg,int bg)
{
 int K;
 K=FontBuf[c];
 outportb (0x3CF,K);
 vga_writemem (p,fg);
 outportb (0x3CF,~K);
 vga_writemem (p,bg);
 K=FontBuf[c+1];
 outportb (0x3CF,K);
 vga_writemem (p+1,fg);
 outportb (0x3CF,~K);
 vga_writemem (p+1,bg);
 K=FontBuf[c+2];
 outportb (0x3CF,K);
 vga_writemem (p+2,fg);
 outportb (0x3CF,~K);
 vga_writemem (p+2,bg);
 K=FontBuf[c+3];
 outportb (0x3CF,K);
 vga_writemem (p+3,fg);
 outportb (0x3CF,~K);
 vga_writemem (p+3,bg);
}

/****************************************************************************/
/*** Put superimposed character row in cache memory                       ***/
/****************************************************************************/
static inline void putchar_t_hires_si (unsigned p,int c,int fg,int bg)
{
 int K;
 K=FontBuf[c];
 outportb (0x3CF,K);
 vga_writemem (p,fg);
 vga_writemem (p+2,fg);
 outportb (0x3CF,~K);
 vga_writemem (p,bg);
 vga_writemem (p+2,bg);
 K=FontBuf[c+1];
 outportb (0x3CF,K);
 vga_writemem (p+1,fg);
 vga_writemem (p+3,fg);
 outportb (0x3CF,~K);
 vga_writemem (p+1,bg);
 vga_writemem (p+3,bg);
 K=FontBuf[c+2];
 outportb (0x3CF,K);
 vga_writemem (p+4,fg);
 vga_writemem (p+6,fg);
 outportb (0x3CF,~K);
 vga_writemem (p+4,bg);
 vga_writemem (p+6,bg);
 K=FontBuf[c+3];
 outportb (0x3CF,K);
 vga_writemem (p+5,fg);
 vga_writemem (p+7,fg);
 outportb (0x3CF,~K);
 vga_writemem (p+5,bg);
 vga_writemem (p+7,bg);
}

/****************************************************************************/
/*** Put a character on the screen for P2000T emulation mode. This        ***/
/*** function checks if the character isn't already put on the same       ***/
/*** position by an earlier screen refresh, if it isn't it checks if the  ***/
/*** character has been cached if using 640x480 mode and uses VGA write   ***/
/*** mode 1 to quickly copy the cached character to the screen            ***/
/****************************************************************************/
static inline void PutChar_T (int x,int y,int c,int fg,int bg,int si)
{
 int K,L;
 unsigned p,q;
 struct font2screen_struct *s;
 K=c+(fg<<8)+(bg<<16)+(si<<24);
 if (K==OldCharacter[y*40+x]) return;
 OldCharacter[y*40+x]=K;
 _farsetsel (DosSelector);
 if (!videomode)
 {
  /* If we're using 256x240 mode, simply put the character on the screen */
  s=font2screen+fg*8*64+bg*64;
  p=0xA0000+y*256*10+x*6+8;
  c*=10;
  if (!si)
  {
   K=FontBuf[c];
   _farnspokew(p,s[K].low);
   _farnspokel(p+2,s[K].high);
   K=FontBuf[c+1];
   _farnspokew(p+256,s[K].low);
   _farnspokel(p+2+256,s[K].high);
   K=FontBuf[c+2];
   _farnspokew(p+512,s[K].low);
   _farnspokel(p+2+512,s[K].high);
   K=FontBuf[c+3];
   _farnspokew(p+768,s[K].low);
   _farnspokel(p+2+768,s[K].high);
   K=FontBuf[c+4];
   _farnspokew(p+1024,s[K].low);
   _farnspokel(p+2+1024,s[K].high);
   K=FontBuf[c+5];
   _farnspokew(p+1280,s[K].low);
   _farnspokel(p+2+1280,s[K].high);
   K=FontBuf[c+6];
   _farnspokew(p+1536,s[K].low);
   _farnspokel(p+2+1536,s[K].high);
   K=FontBuf[c+7];
    _farnspokew(p+1792,s[K].low);
   _farnspokel(p+2+1792,s[K].high);
   K=FontBuf[c+8];
   _farnspokew(p+2048,s[K].low);
    _farnspokel(p+2+2048,s[K].high);
   K=FontBuf[c+9];
   _farnspokew(p+2304,s[K].low);
   _farnspokel(p+2+2304,s[K].high);
  }
  else
  {
   if (si==2)
    c+=5;
   K=FontBuf[c];
   _farnspokew(p,s[K].low);
   _farnspokew(p+256,s[K].low);
   _farnspokel(p+2,s[K].high);
   _farnspokel(p+2+256,s[K].high);
   K=FontBuf[c+1];
   _farnspokew(p+512,s[K].low);
   _farnspokew(p+768,s[K].low);
   _farnspokel(p+2+512,s[K].high);
   _farnspokel(p+2+768,s[K].high);
   K=FontBuf[c+2];
   _farnspokew(p+1024,s[K].low);
   _farnspokew(p+1280,s[K].low);
   _farnspokel(p+2+1024,s[K].high);
   _farnspokel(p+2+1280,s[K].high);
   K=FontBuf[c+3];
   _farnspokew(p+1536,s[K].low);
   _farnspokew(p+1792,s[K].low);
   _farnspokel(p+2+1536,s[K].high);
   _farnspokel(p+2+1792,s[K].high);
   K=FontBuf[c+4];
   _farnspokew(p+2048,s[K].low);
   _farnspokew(p+2304,s[K].low);
   _farnspokel(p+2+2048,s[K].high);
   _farnspokel(p+2+2304,s[K].high);
  }
 }
 else
 {
  /* We're using 640x480x16. Check if the character is cached */
  L=c*(fg%3);     /* We've got just enough memory to cache 3 character sets */
  q=0xA0000+y*1600+x*2;
  p=0xA0000+640*480/8+L*40;
  outportw (0x3CE,0x0000);
  outportw (0x3CE,0x0001);
  if (CharacterCache[L]!=K)
  {
   /* The character isn't cached, so update the cache */
   CharacterCache[L]=K;
   c*=40;
   /* Select write mode 2 */
   outportw (0x3CE,0x0205);
   /* Select GC index 8 - bit mask */
   outportb (0x3CE,8);
   if (!si)
   {
    putchar_t_hires (p,c,fg,bg);
    putchar_t_hires (p+4,c+4,fg,bg);
    putchar_t_hires (p+8,c+8,fg,bg);
    putchar_t_hires (p+12,c+12,fg,bg);
    putchar_t_hires (p+16,c+16,fg,bg);
    putchar_t_hires (p+20,c+20,fg,bg);
    putchar_t_hires (p+24,c+24,fg,bg);
    putchar_t_hires (p+28,c+28,fg,bg);
    putchar_t_hires (p+32,c+32,fg,bg);
    putchar_t_hires (p+36,c+36,fg,bg);
   }
   else
   {
    if (si==2)
     c+=20;
    putchar_t_hires_si (p,c,fg,bg);
    putchar_t_hires_si (p+8,c+4,fg,bg);
    putchar_t_hires_si (p+16,c+8,fg,bg);
    putchar_t_hires_si (p+24,c+12,fg,bg);
    putchar_t_hires_si (p+32,c+16,fg,bg);
   }
  }
  /* Now, copy the cached character to the screen */
  outportw (0x3CE,0x0105);
  vga_copymem (p,q);
  vga_copymem (p+2,q+80);
  vga_copymem (p+4,q+160);
  vga_copymem (p+6,q+240);
  vga_copymem (p+8,q+320);
  vga_copymem (p+10,q+400);
  vga_copymem (p+12,q+480);
  vga_copymem (p+14,q+560);
  vga_copymem (p+16,q+640);
  vga_copymem (p+18,q+720);
  vga_copymem (p+20,q+800);
  vga_copymem (p+22,q+880);
  vga_copymem (p+24,q+960);
  vga_copymem (p+26,q+1040);
  vga_copymem (p+28,q+1120);
  vga_copymem (p+30,q+1200);
  vga_copymem (p+32,q+1280);
  vga_copymem (p+34,q+1360);
  vga_copymem (p+36,q+1440);
  vga_copymem (p+38,q+1520);
 }
}

/****************************************************************************/
/*** Common.h contains the system-independent part of the screen refresh  ***/
/*** drivers                                                              ***/
/****************************************************************************/
#include "Common.h"
