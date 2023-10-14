/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                               SVGALib.c                              ***/
/***                                                                      ***/
/*** This file contains the Linux/SVGALib drivers. Sound implementation   ***/
/*** is in Unix.c                                                         ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include "P2000.h"
#include "Unix.h"
#include "Bitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <termio.h>
#include <vga.h>
#include <linux/keyboard.h>
#include <vgakeyboard.h>
#include <asm/io.h>

#ifndef SCANCODE_F11             /* Old SVGALib versions don't define these */
#define SCANCODE_F11	87
#endif
#ifndef SCANCODE_F12
#define SCANCODE_F12	88
#endif

char *Title="M2000 Linux/SVGALib 0.6"; /* Title for -help output            */

int videomode;                     /* T emulation only: 0=320x240 1=640x480 */ 
static int *OldCharacter;          /* Holds characters on the screen        */
static int *CharacterCache;        /* Current cache contents                */
static byte *FontBuf;              /* Pointer to font used                  */
byte *DisplayBuf;                  /* Used only in 320x240 mode             */
unsigned ReadTimerMin;             /* Minimum number of micro seconds       */
                                   /* between interrupts                    */
static int OldTimer=0;             /* Value of timer at previous interrupt  */
static int NewTimer=0;             /* New value of the timer                */
#ifdef SOUND
int soundmode=255;                 /* Sound mode, 255=auto-detect           */
static int soundoff=0;             /* If 1, sound is turned off             */
#endif
#ifdef JOYSTICK
int joymode=1;                     /* If 0, do not use joystick             */
#endif
int chipset=0;                     /* 0=VGA, 1=auto-detect                  */
static struct termios termold;     /* Original terminal settings            */

struct font2screen_struct
{
 unsigned low;
 unsigned high;
};
static struct font2screen_struct *font2screen=NULL;

char szBitmapFile[256];            /* Next screen shot file                 */

static byte Pal[8*3] =             /* SAA5050 palette                       */
{
   0x00,0x00,0x00 , 0xFF,0x00,0x00 , 0x00,0xFF,0x00 , 0xFF,0xFF,0x00 ,
   0x00,0x00,0xFF , 0xFF,0x00,0xFF , 0x00,0xFF,0xFF , 0xFF,0xFF,0xFF
};

static int PausePressed=0;               /* 1 if pause key is pressed       */
static byte keybstatus[NR_KEYS];         /* 1 if a certain key is pressed   */
static int makeshot=0;                   /* 1 -> take a screen shot         */
static int calloptions=0;                /* 1 -> call OptionsDialogue()     */

static int keymask[NR_KEYS]=
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
 0x2FE,0x27F,0x5FB,0x000,0x2DF,0x000,0x000,0x0FB,
 0x000,0x0FE,0x27F,0x000,0x2DF,0x000,0x000,0x3FB,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000
};

/****************************************************************************/
/*** Output given data to given I/O port                                  ***/
/****************************************************************************/
static inline volatile void outportw (word _port,word _data)
{
 outw (_data,_port);
}
static inline volatile void outportb (word _port,byte _data)
{
 outb (_data,_port);
}

/****************************************************************************/
/*** Copy off-screen buffer to actual display. This is only used in       ***/
/*** 320x240 mode                                                         ***/
/****************************************************************************/
static void PutImage (void)
{
 if (!P2000_Mode && !videomode)
 {
  char *p,*z;
  int i,j;
  p=vga_getgraphmem()+32/4;
  z=DisplayBuf;
  for (i=0;i<240;++i,p+=320/4,z+=240)
  {
   outportw (0x3C4,0x102);
   for (j=0;j<60;++j)
    p[j]=z[j*4+0];
   outportw (0x3C4,0x202);
   for (j=0;j<60;++j)
    p[j]=z[j*4+1];
   outportw (0x3C4,0x402);
   for (j=0;j<60;++j)
    p[j]=z[j*4+2];
   outportw (0x3C4,0x802);
   for (j=0;j<60;++j)
    p[j]=z[j*4+3];
  }
 }
}

/****************************************************************************/
/*** This function is called when a key is pressed or released            ***/
/****************************************************************************/
static void keyb_handler (int code,int newstatus)
{
 int tmp;
 if (code<0 || code>=NR_KEYS)
  return;
 if (newstatus) newstatus=1;
 if (keybstatus[code]!=newstatus)
 {
  if (!newstatus)
  {
   keybstatus[code]=0;
   tmp=keymask[code];
   if (tmp)
    KeyMap[tmp>>8]|=(~(tmp&0xFF));
  }
  else
  {
   keybstatus[code]=1;
   tmp=keymask[code];
   if (code==SCANCODE_F8 || code==SCANCODE_F9)
   {
    if (PausePressed)
     PausePressed=0;
    else
    {
     if (code==SCANCODE_F8)
      PausePressed=2;
     else
      PausePressed=1;
    }
   }
   else
    PausePressed=0;
   switch (code)
   {
    case SCANCODE_ESCAPE:
    case SCANCODE_F10:
     Z80_Running=0;
     break;
#ifdef DEBUG
    case SCANCODE_F4:
     Z80_Trace=!Z80_Trace;
     break;
#endif
    case SCANCODE_F7:
     makeshot=1;
     break;
#ifdef SOUND
    case SCANCODE_F5:
     soundoff=(!soundoff);
     break;
    case SCANCODE_F11:
     DecreaseSoundVolume ();
     break;
    case SCANCODE_F12:
     IncreaseSoundVolume ();
     break;
#endif
    case SCANCODE_F6:
     calloptions=1;
     break;
   }
   if (tmp)
    KeyMap[tmp>>8]&=(tmp&0xFF);
  }
 }
}

/****************************************************************************/
/*** Deallocate resources taken by InitMachine()                          ***/
/****************************************************************************/
void TrashMachine(void)
{
 if (Verbose) printf("\n\nShutting down...\n");
#ifdef SOUND
 TrashSound ();
#endif
#ifdef JOYSTICK
 TrashJoystick ();
#endif
 keyboard_close ();
 vga_setmode (TEXT);
 tcsetattr (0,0,&termold);
 if (font2screen) free (font2screen);
 if (DisplayBuf) free (DisplayBuf);
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
/*** Initialise all resources needed by the Linux/SVGALib implementation  ***/
/****************************************************************************/
int InitMachine(void)
{
 int c,i,j;
 FILE *bitmapfile;
 if (Verbose)
  printf ("Initialising Linux/SVGALib drivers:\n");
 tcgetattr (0,&termold);
 if (!chipset) vga_setchipset (VGA);
 vga_init ();
 if (Verbose) printf ("  Setting VGA mode... ");
 if (!P2000_Mode && !videomode)
  i=G320x240x256;
 else
  i=G640x480x16;
 i=vga_setmode (i);
 if (i) { if (Verbose) printf ("FAILED\n"); return 0; }
 if (!P2000_Mode)
 {
  for (i=0;i<8;++i)
   vga_setpalette (i,Pal[i*3+0]>>2,Pal[i*3+1]>>2,Pal[i*3+2]>>2);
 }
 else
 {
  Pal[0]=Pal[1]=Pal[2]=0;
  Pal[3]=Pal[4]=Pal[5]=255;
 }
 if (!P2000_Mode && !videomode)
 {
  if(Verbose)
   printf("OK\n  Allocating screen buffer... ");
  DisplayBuf=(byte *)malloc(240*240);
  if (!DisplayBuf)
  {
   if (Verbose) puts ("FAILED");
   return(0);
  }
  memset (DisplayBuf,0,240*240);
 }
 if (Verbose) printf ("OK\n  Allocating cache buffers... ");
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
   printf ("OK\n  Allocating conversion buffer... ");
  font2screen=malloc(8*8*64*sizeof(struct font2screen_struct));
  if (!font2screen)
  {
   if (Verbose) puts ("FAILED");
   return 0;
  }
  if (Verbose)
   printf ("OK\n  Initialising conversion buffer... ");
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
#ifdef JOYSTICK
 InitJoystick (joymode);
#endif
 strcpy (szBitmapFile,"M2000.bmp");
 while ((bitmapfile=fopen(szBitmapFile,"rb"))!=NULL)
 {
  fclose (bitmapfile);
  if (!NextBitmapFile())
   break;
 }
 if (Verbose)
  printf ("  Next screenshot will be %s\n",szBitmapFile);
#ifdef SOUND
  InitSound (soundmode);
#endif
 if (Verbose) printf ("  Initialising timer...\n");
 ReadTimerMin=1000000/IFreq;
 OldTimer=ReadTimer ();
 if (Verbose) printf ("  Initialising keyboard...");
 if (keyboard_init()) { if (Verbose) printf ("FAILED\n"); return 0; }
 keyboard_seteventhandler (keyb_handler);
 if (Verbose) printf ("OK\n");
 return 1;
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
 char *p;
 keyboard_update ();
 if (PausePressed)
 {
  if (PausePressed==2)                       /* Blank screen                */
  {
   outportw (0x3C4,0xF02);
   p=vga_getgraphmem ();
   for (i=0;i<320*200;++i) *p++=0;
  }
  while (PausePressed) keyboard_update ();
  OldTimer=ReadTimer ();
 }
 if (makeshot)
 {
  /* Copy screen contents to buffer and write bitmap file */
  byte *p;
  i=(P2000_Mode)? (640*480/8):(videomode)? (640*480*7/8):(240*240/2);
  p=malloc (i);
  if (p)
  {
   if (!P2000_Mode)
   {
    if (!videomode)
    {
     /* 320x240, 8bpp. Convert to 4bpp to decrease file size
        Note: DisplayBuf holds a 240x240 image to decrease
        memory needed */
     for (i=0;i<240*240/2;++i)
      p[i]=(DisplayBuf[i*2]<<4)|(DisplayBuf[i*2+1]);
     WriteBitmap (szBitmapFile,4,8,240,240,240,p,Pal);
    }
    else
    {
     /* 640x480, 4bpp - convert bitplaned data first */
     byte *q;
     int a,b,c;
     outportw (0x3CE,0x0004);      /* Select bit plane 1                    */
     memcpy (p,vga_getgraphmem(),640*480/8);
     outportw (0x3CE,0x0104);      /* Select bit plane 2                    */
     memcpy (p+640*480/8,vga_getgraphmem(),640*480/8);
     outportw (0x3CE,0x0204);      /* Select bit plane 3                    */
     memcpy (p+640*480*2/8,vga_getgraphmem(),640*480/8);
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
     WriteBitmap (szBitmapFile,4,8,640,640,480,p+640*480*3/8,Pal);
    }
   }
   else
   {
    /* 640x480, monochrome */
    memcpy (p,vga_getgraphmem(),640*480/8);
    WriteBitmap (szBitmapFile,1,2,640,640,480,p,Pal);
   }
   free (p);
   NextBitmapFile ();
  }
  makeshot=0;
 }

 if (calloptions)
 {
  struct termios term;
  calloptions=0;
  vga_flip ();
  keyboard_close ();
  tcgetattr (0,&term);
  tcsetattr (0,0,&termold);
  OptionsDialogue ();
  tcsetattr (0,0,&term);
  keyboard_init ();
  keyboard_seteventhandler (keyb_handler);
  vga_flip ();
 }
#ifdef JOYSTICK
 i=ReadJoystick ();
 if (i&0x40)
  KeyMap[0]&=0xFE;
 else if (!keybstatus[SCANCODE_LEFTCONTROL] &&
          !keybstatus[SCANCODE_CURSORBLOCKLEFT])
  KeyMap[0]|=0x01;
 if (i&0x80)
  KeyMap[2]&=0x7F;
 else if (!keybstatus[SCANCODE_RIGHTCONTROL] &&
          !keybstatus[SCANCODE_CURSORBLOCKRIGHT])
  KeyMap[2]|=0x80;
 if (i&0x10)
  KeyMap[0]&=0xFB;
 else if (!keybstatus[SCANCODE_LEFTALT] &&
          !keybstatus[SCANCODE_CURSORBLOCKUP])
  KeyMap[0]|=0x04;
 if (i&0x20)
  KeyMap[2]&=0xDF;
 else if (!keybstatus[SCANCODE_RIGHTALT] &&
          !keybstatus[SCANCODE_CURSORBLOCKDOWN])
  KeyMap[2]|=0x20;
 if (i&0x0F)
  KeyMap[2]&=0xFD;
 else if (!keybstatus[SCANCODE_SPACE])
  KeyMap[2]|=0x02;
#endif
}

/****************************************************************************/
/*** This function is called every interrupt to flush sound pipes and     ***/
/*** sync emulation                                                       ***/
/****************************************************************************/
void FlushSound (void)
{
#ifdef SOUND
 if (!Sound_FlushSound())
#endif
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
#ifdef SOUND
 if (!soundoff) WriteSound (toggle);
#endif
}

/****************************************************************************/
/*** Pause specified ammount of time                                      ***/
/****************************************************************************/
void Pause (int ms)
{
 int i,j;
 j=ReadTimer();
 i=j+ms*1000;
 while ((j-i)<0) j=ReadTimer();
 OldTimer=j;
}

/****************************************************************************/
/*** Put a character on the screen for P2000M emulation mode. This        ***/
/*** function checks if the character isn't already put on the same       ***/
/*** position by an earlier screen refresh                                ***/
/****************************************************************************/
static void PutChar_M (int x,int y,int c,int eor,int ul)
{
 int K;
 byte *p;
 K=c+(eor<<8)+(ul<<16);
 if (K==OldCharacter[y*80+x]) return;
 OldCharacter[y*80+x]=K;
 outportw (0x3CE,0x0000);
 outportw (0x3CE,0x0001);
 p=vga_getgraphmem()+y*1600+x;
 c*=10;
 if (eor) eor=0xFF;
 K=FontBuf[c];
 p[0]=p[80]=K^eor;
 K=FontBuf[c+1];
 p[160]=p[80+160]=K^eor;
 K=FontBuf[c+2];
 p[320]=p[80+320]=K^eor;
 K=FontBuf[c+3];
 p[480]=p[80+480]=K^eor;
 K=FontBuf[c+4];
 p[640]=p[80+640]=K^eor;
 K=FontBuf[c+5];
 p[800]=p[80+800]=K^eor;
 K=FontBuf[c+6];
 p[960]=p[80+960]=K^eor;
 K=FontBuf[c+7];
 p[1120]=p[80+1120]=K^eor;
 K=FontBuf[c+8];
 p[1280]=p[80+1280]=K^eor;
 if (ul) K=255; else K=FontBuf[c+9];
 p[1440]=p[80+1440]=K^eor;
}

/****************************************************************************/
/*** Copy 16 pixels at [p] to [q]. Assume write mode 1                    ***/
/****************************************************************************/
static inline void vga_copymem (byte *p,byte *q)
{
 /* Don't use "q[0]=p[0]; q[1]=p[1];" as GCC might optimise this to
    "*(word*)q=*(word*)p;", which won't work properly as the VGA adapter only
    has 8-bit latch registers */
 asm __volatile__ (
 " movb (%%edx),%%al    \n"
 " movb %%al,(%%ecx)    \n"
 " movb 1(%%edx),%%al   \n"
 " movb %%al,1(%%ecx)   \n"
 :
 :"c" (q),
  "d" (p)
 :"eax"
 );
}

/****************************************************************************/
/*** Latch the address first, then do a write                             ***/
/****************************************************************************/
static inline void vga_writemem (byte *p,byte v)
{
 /* When using "dummy=*p; *p=v;", chances are GCC will optimise out the first
    expression which will cause the data not to be latched */
 asm __volatile__ (
 " movb	(%%edx),%%ah	\n"
 " movb %%al,(%%edx)	\n"
 :
 :"a" (v),
  "d" (p)
 :"eax"
 );
}

/****************************************************************************/
/*** Put character row in cache memory                                    ***/
/****************************************************************************/
static inline void putchar_t_hires (byte *p,int c,int fg,int bg)
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
static inline void putchar_t_hires_si (byte *p,int c,int fg,int bg)
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
 byte *p,*q;
 struct font2screen_struct *s;
 K=c+(fg<<8)+(bg<<16)+(si<<24);
 if (K==OldCharacter[y*40+x]) return;
 OldCharacter[y*40+x]=K;
 if (!videomode)
 {
  /* 320x240 mode. Put character in off-screen buffer */
  s=font2screen+fg*8*64+bg*64;
  p=DisplayBuf+y*240*10+x*6;
  c*=10;
  if (!si)
  {
   K=FontBuf[c];
   *(unsigned short*)p=(unsigned short)s[K].low;
   *(unsigned*)(p+2)=s[K].high;
   K=FontBuf[c+1];
   *(unsigned short*)(p+240)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+240)=s[K].high;
   K=FontBuf[c+2];
   *(unsigned short*)(p+480)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+480)=s[K].high;
   K=FontBuf[c+3];
   *(unsigned short*)(p+720)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+720)=s[K].high;
   K=FontBuf[c+4];
   *(unsigned short*)(p+960)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+960)=s[K].high;
   K=FontBuf[c+5];
   *(unsigned short*)(p+1200)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+1200)=s[K].high;
   K=FontBuf[c+6];
   *(unsigned short*)(p+1440)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+1440)=s[K].high;
   K=FontBuf[c+7];
   *(unsigned short*)(p+1680)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+1680)=s[K].high;
   K=FontBuf[c+8];
   *(unsigned short*)(p+1920)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+1920)=s[K].high;
   K=FontBuf[c+9];
   *(unsigned short*)(p+2160)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+2160)=s[K].high;
  }
  else
  {
   if (si==2)
    c+=5;
   K=FontBuf[c];
   *(unsigned short*)p=*(unsigned short*)(p+240)=(unsigned short)s[K].low;
   *(unsigned*)(p+2)=*(unsigned*)(p+2+240)=s[K].high;
   K=FontBuf[c+1];
   *(unsigned short*)(p+480)=*(unsigned short*)(p+240+480)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+480)=*(unsigned*)(p+2+240+480)=s[K].high;
   K=FontBuf[c+2];
   *(unsigned short*)(p+960)=*(unsigned short*)(p+240+960)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+960)=*(unsigned*)(p+2+240+960)=s[K].high;
   K=FontBuf[c+3];
   *(unsigned short*)(p+1440)=*(unsigned short*)(p+240+1440)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+1440)=*(unsigned*)(p+2+240+1440)=s[K].high;
   K=FontBuf[c+4];
   *(unsigned short*)(p+1920)=*(unsigned short*)(p+240+1920)=(unsigned short)s[K].low;
   *(unsigned*)(p+2+1920)=*(unsigned*)(p+2+240+1920)=s[K].high;
  }
 }
 else
 {
  /* We're using 640x480x16. Check if the character is cached */
  L=c*(fg%3);     /* We've got just enough memory to cache 3 character sets */
  q=vga_getgraphmem()+y*1600+x*2;
  p=vga_getgraphmem()+640*480/8+L*40;
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
