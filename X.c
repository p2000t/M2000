/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                  X.c                                 ***/
/***                                                                      ***/
/*** This file contains the X-Windows drivers. Sound implementation is in ***/
/*** Unix.c                                                               ***/
/*** Parts based on code by Marat Fayzullin (fms@freeflight.com)          ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include "P2000.h"
#include "Unix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

char *Title="M2000 Unix/X 0.6"; /* Title for -help output                   */

static int bpp;                 /* Bits per pixel of the display device     */
static Display *Dsp;            /* Our display                              */
static Window Wnd;              /* Our window                               */
static Colormap DefaultCMap;    /* The default colour map                   */
static XImage *Img;             /* Pointer to our display image             */
static GC DefaultGC;            /* Default graphics context                 */
static unsigned long White,Black; /* Black and white colour values          */
static int width,height;        /* Width and height of the display buffer   */
#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
XShmSegmentInfo SHMInfo;        /* Shared memory is used when possible      */
int UseSHM=1;                   /* If 0, do not use SHM extensions          */
#endif

int videomode=0;                /* 0=500x300, 1=520x490                     */
byte *DisplayBuf;               /* Screen buffer                            */
static int *OldCharacter;       /* Holds the characters put in DisplayBuf   */
static byte *FontBuf;           /* Teletext font buffer                     */
unsigned ReadTimerMin;          /* Minimum time between interrupts (in us)  */
static int OldTimer;            /* Timer value at previous interrupt        */
static int NewTimer;            /* New value of the timer                   */
int SaveCPU=1;                  /* If 1, pause emulation when focus is out  */
#ifdef SOUND
int soundmode=255;              /* Soundmode, 255=auto detect               */
static int soundoff=0;          /* If 1, sound is turned off                */
#endif
#ifdef JOYSTICK
int joymode=1;                  /* If 0, do not use joystick                */
#endif

static byte Pal[8*3] =          /* SAA5050 palette                          */
{
   0x00,0x00,0x00 , 0xFF,0x00,0x00 , 0x00,0xFF,0x00 , 0xFF,0xFF,0x00 ,
   0x00,0x00,0xFF , 0xFF,0x00,0xFF , 0x00,0xFF,0xFF , 0xFF,0xFF,0xFF
};
static unsigned XPal[8];        /* Values of the 8 P2000 colours            */

static int PausePressed=0;      /* 1 if emulation should be paused          */
static int calloptions=0;       /* 1 if options dialogue should be called   */
static byte keybstatus[256];    /* 1=key is pressed                         */

static int keymask[256]=
{
 /* 00 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x5EF,0x1FE,0x000,0x000,0x000,0x6EF,0x000,0x000,
 /* 10 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 /* 20 */
 0x2FD,0x000,0x000,0x000,0x000,0x000,0x000,0x87F,
 0x000,0x000,0x000,0x000,0x2BF,0x57F,0x7FD,0x7DF,
 /* 30 */
 0x5DF,0x5BF,0x77F,0x0EF,0x07F,0x0DF,0x0FD,0x0BF,
 0x6BF,0x5FD,0x000,0x8DF,0x000,0x8EF,0x000,0x000,
 /* 40 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 /* 50 */
 0x000,0x0FE,0x0FB,0x27F,0x2DF,0x000,0x000,0x000,
 0x000,0x000,0x000,0x67F,0x2EF,0x7EF,0x000,0x000,
 /* 60 */
 0x4FE,0x4FB,0x3DF,0x3EF,0x1EF,0x4EF,0x17F,0x1DF,
 0x1FD,0x8BF,0x1BF,0x7BF,0x8FD,0x3BF,0x3FD,0x6FD,
 /* 70 */
 0x6DF,0x0F7,0x47F,0x1F7,0x4DF,0x4BF,0x37F,0x4F7,
 0x3F7,0x4FD,0x1FB,0x000,0x000,0x000,0x000,0x000,
 /* 80 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x2FE,0x000,0x000,
 /* 90 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 /* A0 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x5FE,0x5FB,0x000,0x5F7,0x2FB,0x5FB,
 /* B0 */
 0x2F7,0x7F7,0x7FB,0x7FE,0x8F7,0x8FB,0x8FE,0x6F7,
 0x6FB,0x6FE,0x000,0x000,0x000,0x000,0x000,0x000,
 /* C0 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 /* D0 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 /* E0 */
 0x000,0x9FE,0x97F,0x0FE,0x27F,0x3FE,0x3FE,0x000,
 0x000,0x0FB,0x2DF,0x000,0x000,0x000,0x000,0x000,
 /* F0 */
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
 0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x3FB
};

/****************************************************************************/
/*** This function is called by the screen refresh drivers to copy the    ***/
/*** off-screen buffer to the actual display                              ***/
/****************************************************************************/
static void PutImage (void)
{
#ifdef MITSHM
 if (UseSHM) XShmPutImage (Dsp,Wnd,DefaultGC,Img,0,0,0,0,width,height,False);
 else
#endif
 XPutImage (Dsp,Wnd,DefaultGC,Img,0,0,0,0,width,height);
 XFlush (Dsp);
}

/****************************************************************************/
/*** This function is called when a key is pressed or released. It        ***/
/*** updates the P2000 keyboard matrix and checks for special keypresses  ***/
/*** like F5 (toggle sound on/off)                                        ***/
/****************************************************************************/
static void keyb_handler (int code,int newstatus)
{
 int tmp;
 if (newstatus || code==XK_Caps_Lock) newstatus=1;
 if (keybstatus[code&255]!=newstatus)
 {
  if (!newstatus)
  {
   keybstatus[code&255]=0;
   tmp=keymask[code&255];
   if (tmp)
    KeyMap[tmp>>8]|=(~(tmp&0xFF));
  }
  else
  {
   keybstatus[code&255]=1;
   tmp=keymask[code&255];
   if (code==XK_F8 || code==XK_F9)
   {
    if (PausePressed)
     PausePressed=0;
    else
    {
     if (code==XK_F8)
      PausePressed=2;
     else
      PausePressed=1;
    }
   }
   else
    PausePressed=0;
   switch (code)
   {
    case XK_Escape:
    case XK_F10:
     Z80_Running=0;
     break;
#ifdef DEBUG
    case XK_F4:
     Z80_Trace=!Z80_Trace;
     break;
#endif
#ifdef SOUND
    case XK_F5:
     soundoff=(!soundoff);
     break;
    case XK_F11:
     DecreaseSoundVolume ();
     break;
    case XK_F12:
     IncreaseSoundVolume ();
     break;
#endif
    case XK_F6:
     calloptions=1;
     break;
   }
   if (tmp)
    KeyMap[tmp>>8]&=(tmp&0xFF);
  }
 }
}

/****************************************************************************/
/*** This function looks for keyboard events and passes them to the       ***/
/*** keyboard handler                                                     ***/
/****************************************************************************/
static void keyboard_update (void)
{
 XEvent E;
 int i;
 keybstatus[XK_Caps_Lock&255]=0;
 i=keymask[XK_Caps_Lock&255];
 if (i)
  KeyMap[i>>8]|=(~(i&0xFF));
 while (XCheckWindowEvent(Dsp,Wnd,KeyPressMask|KeyReleaseMask,&E))
 {
  i=XLookupKeysym ((XKeyEvent*)&E,0);
  keyb_handler (i,E.type==KeyPress);
 }
}

/****************************************************************************/
/*** This function deallocates all resources taken by InitMachine()       ***/
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
 if (Dsp && Wnd)
 {
#ifdef MITSHM
  if (UseSHM)
  {
   XShmDetach (Dsp,&SHMInfo);
   if (SHMInfo.shmaddr) shmdt (SHMInfo.shmaddr);
   if (SHMInfo.shmid>=0) shmctl (SHMInfo.shmid,IPC_RMID,0);
  }
  else
#endif
  if (Img) XDestroyImage (Img);
 }
 if (Dsp) XCloseDisplay (Dsp);
 if (FontBuf) free (FontBuf);
 if (OldCharacter) free (OldCharacter);
}

/****************************************************************************/
/*** Swap bytes in an integer. Called to support displays with a          ***/
/*** non-standard byte order                                              ***/
/****************************************************************************/
static unsigned SwapBytes (unsigned val,unsigned depth)
{
 if (depth==8)
  return val;
 if (depth==16)
  return ((val>>8)&0xFF)+((val<<8)&0xFF00);
 return ((val>>24)&0xFF)+((val>>8)&0xFF00)+
        ((val<<8)&0xFF0000)+((val<<24)&0xFF000000);
}

/****************************************************************************/
/*** This function allocates all resources used by the X-Windows          ***/
/*** implementation                                                       ***/
/****************************************************************************/
int InitMachine(void)
{
 int i;
 Screen *Scr;
 XSizeHints Hints;
 XWMHints WMHints;
 XColor Colour;
 if (Verbose)
  printf ("Initialising Unix/X drivers:\n  Opening display... ");
 Dsp=XOpenDisplay (NULL);
 if (!Dsp)
 {
  if (Verbose) printf ("FAILED\n");
  return 0;
 }
 Scr=DefaultScreenOfDisplay (Dsp);
 White=WhitePixelOfScreen (Scr);
 Black=BlackPixelOfScreen (Scr);
 DefaultGC=DefaultGCOfScreen (Scr);
 DefaultCMap=DefaultColormapOfScreen (Scr);
 bpp=DefaultDepthOfScreen (Scr);
 if (bpp!=8 && bpp!=16 && bpp!=32)
 {
  printf ("FAILED - Only 8,16 and 32 bpp displays are supported\n");
  return 0;
 }
 if (bpp==32 && sizeof(unsigned)!=4)
 {
  printf ("FAILED - 32 bpp displays are only supported on 32 bit machines\n");
  return 0;
 }
 switch (videomode)
 {
  case 1: width=520; height=490; break;
  default: videomode=0; width=500; height=300; break;
 }
 if (Verbose) printf ("OK\n  Opening window... ");
 Wnd=XCreateSimpleWindow (Dsp,RootWindowOfScreen(Scr),
 			  0,0,width,height,0,White,Black);
 if (!Wnd)
 {
  if (Verbose) printf ("FAILED\n");
  return 0;
 }
 Hints.flags=PSize|PMinSize|PMaxSize;
 Hints.min_width=Hints.max_width=Hints.base_width=width;
 Hints.min_height=Hints.max_height=Hints.base_height=height;
 WMHints.input=True;
 WMHints.flags=InputHint;
 XSetWMHints (Dsp,Wnd,&WMHints);
 XSetWMNormalHints (Dsp,Wnd,&Hints);
 XStoreName (Dsp,Wnd,Title);
 XSelectInput (Dsp,Wnd,FocusChangeMask|ExposureMask|KeyPressMask|KeyReleaseMask);
 XMapRaised (Dsp,Wnd);
 XClearWindow (Dsp,Wnd);
 if (Verbose) printf ("OK\n");
#ifdef MITSHM
 if (UseSHM)
 {
  if (Verbose) printf ("  Using shared memory:\n    Creating image... ");
  Img=XShmCreateImage (Dsp,DefaultVisualOfScreen(Scr),bpp,
                       ZPixmap,NULL,&SHMInfo,width,height);
  if (!Img)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  if (Verbose) printf ("OK\n    Getting SHM info... ");
  SHMInfo.shmid=shmget (IPC_PRIVATE,Img->bytes_per_line*Img->height,
  			IPC_CREAT|0777);
  if (SHMInfo.shmid<0)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  if (Verbose) printf ("OK\n    Allocating SHM... ");
  Img->data=SHMInfo.shmaddr=shmat(SHMInfo.shmid,0,0);
  DisplayBuf=Img->data;
  if (!DisplayBuf)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  SHMInfo.readOnly=False;
  if (Verbose) printf ("OK\n    Attaching SHM... ");
  if (!XShmAttach(Dsp,&SHMInfo))
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
 }
 else
#endif
 {
  if (Verbose) printf ("  Allocating screen buffer... ");
  DisplayBuf=malloc(bpp*width*height/8);
  if (!DisplayBuf)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
  if (Verbose) printf ("OK\n  Creating image... ");
  Img=XCreateImage (Dsp,DefaultVisualOfScreen(Scr),bpp,ZPixmap,
  		    0,DisplayBuf,width,height,8,0);
  if (!Img)
  {
   if (Verbose) printf ("FAILED\n");
   return 0;
  }
 }
 if (Verbose) printf ("OK\n  Allocating cache buffer... ");
 if (P2000_Mode) i=80*24;
 else i=40*24;
 OldCharacter=malloc (i*sizeof(int));
 if (!OldCharacter)
 {
  if (Verbose) puts ("FAILED");
  return(0);
 }
 memset (OldCharacter,-1,i*sizeof(int));
 if (Verbose) printf ("OK\n  Allocating colours... ");
 if (!P2000_Mode)
 {
  for (i=0;i<8;++i)
  {
   if (Pal[i*3+0]==0 && Pal[i*3+1]==0 && Pal[i*3+2]==0)
    XPal[i]=Black;
   else if (Pal[i*3+0]==255 && Pal[i*3+1]==255 && Pal[i*3+2]==255)
    XPal[i]=White;
   else
   {
    Colour.flags=DoRed|DoGreen|DoBlue;
    Colour.red=Pal[i*3+0]<<8;
    Colour.green=Pal[i*3+1]<<8;
    Colour.blue=Pal[i*3+2]<<8;
    if (XAllocColor(Dsp,DefaultCMap,&Colour))
#ifdef LSB_FIRST
     if (BitmapBitOrder(Dsp)==LSBFirst)
#else
     if (BitmapBitOrder(Dsp)==MSBFirst)
#endif
      XPal[i]=Colour.pixel;
     else
      XPal[i]=SwapBytes (Colour.pixel,bpp);
   }
  }
 }
 else
 {
  XPal[0]=Black;
  XPal[1]=White;
 }
 if (Verbose) printf ("OK\n");
#ifdef JOYSTICK
 InitJoystick (joymode);
#endif
#ifdef SOUND
  InitSound (soundmode);
#endif
 if (Verbose) printf ("  Initialising timer...\n");
 ReadTimerMin=1000000/IFreq;
 OldTimer=ReadTimer ();
 return 1;
}

/****************************************************************************/
/*** This function loads a font and converts it if necessary              ***/
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
  printf ("Reading... ");
  if (fread(TempBuf,2240,1,F)) i=1;
  fclose(F);
 }
 if (Verbose) puts ((i)? "OK":"FAILED");
 if (!i) return 0;
 if (P2000_Mode)
 {
  memcpy (FontBuf,TempBuf,2240);
  free (TempBuf);
  return 1;
 }
 /* Stretch characters to 12x20 */
 for (i=0;i<96*10;i+=10)
 {
  for (j=0;j<10;++j)
  {
   c=TempBuf[i+j]; k=0;
   if (c&0x10) k|=0x0300;
   if (c&0x08) k|=0x00C0;
   if (c&0x04) k|=0x0030;
   if (c&0x02) k|=0x000C;
   if (c&0x01) k|=0x0003;
   l=k;
   if (videomode && j!=9)
   {
    d=TempBuf[i+j+1];
    if ((c&0x10) && (d&0x08) && !(d&0x10)) l|=0x0080;
    if ((c&0x08) && (d&0x04) && !(d&0x08)) l|=0x0020;
    if ((c&0x04) && (d&0x02) && !(d&0x04)) l|=0x0008;
    if ((c&0x02) && (d&0x01) && !(d&0x02)) l|=0x0002;
    if ((d&0x10) && (c&0x08) && !(d&0x08)) l|=0x0100;
    if ((d&0x08) && (c&0x04) && !(d&0x04)) l|=0x0040;
    if ((d&0x04) && (c&0x02) && !(d&0x02)) l|=0x0010;
    if ((d&0x02) && (c&0x01) && !(d&0x01)) l|=0x0004;
   }
   if (videomode && j!=0)
   {
    d=TempBuf[i+j-1];
    if ((c&0x10) && (d&0x08) && !(d&0x10)) k|=0x0080;
    if ((c&0x08) && (d&0x04) && !(d&0x08)) k|=0x0020;
    if ((c&0x04) && (d&0x02) && !(d&0x04)) k|=0x0008;
    if ((c&0x02) && (d&0x01) && !(d&0x02)) k|=0x0002;
    if ((d&0x10) && (c&0x08) && !(d&0x08)) k|=0x0100;
    if ((d&0x08) && (c&0x04) && !(d&0x04)) k|=0x0040;
    if ((d&0x04) && (c&0x02) && !(d&0x02)) k|=0x0010;
    if ((d&0x02) && (c&0x01) && !(d&0x01)) k|=0x0004;
   }
   FontBuf[(i+j)*4+0]=k>>8; FontBuf[(i+j)*4+2]=l>>8;
   FontBuf[(i+j)*4+1]=k>>0; FontBuf[(i+j)*4+3]=l>>0;
  }
 }
 for (i=96*10;i<(96+128)*10;++i)
 {
  c=TempBuf[i]; k=0;
  if (c&0x20) k|=0x0C00;
  if (c&0x10) k|=0x0300;
  if (c&0x08) k|=0x00C0;
  if (c&0x04) k|=0x0030;
  if (c&0x02) k|=0x000C;
  if (c&0x01) k|=0x0003;
  FontBuf[i*4]=FontBuf[i*4+2]=k>>8; FontBuf[i*4+1]=FontBuf[i*4+3]=k;
 }
 free (TempBuf);
 return 1;
}

/****************************************************************************/
/*** This function is called every interrupt to check for window events   ***/
/*** and update the keyboard matrix                                       ***/
/****************************************************************************/
void Keyboard(void)
{
 int i;
 XEvent E;
 keyboard_update ();
 if (PausePressed)
 {
  while (PausePressed)
  {
   keyboard_update ();
   if(XCheckWindowEvent(Dsp,Wnd,ExposureMask,&E)) PutImage();
   XPeekEvent(Dsp,&E);
  }
  OldTimer=ReadTimer ();
 }
 /* If saving CPU and focus is out, sleep */
 i=0;
 while (XCheckWindowEvent(Dsp,Wnd,FocusChangeMask,&E))
  i=(E.type==FocusOut);
 if(SaveCPU&&i)
 {
  while(!XCheckWindowEvent(Dsp,Wnd,FocusChangeMask,&E)&&Z80_Running)
  {
   if(XCheckWindowEvent(Dsp,Wnd,ExposureMask,&E)) PutImage();
   XPeekEvent(Dsp,&E);
  }
  OldTimer=ReadTimer ();
 }
 if (calloptions)
 {
  calloptions=0;
  OptionsDialogue ();
 }
#ifdef JOYSTICK
 i=ReadJoystick ();
 if (i&0x40)
  KeyMap[0]&=0xFE;
 else if (!keybstatus[XK_Control_L&255] &&
          !keybstatus[XK_Left&255])
  KeyMap[0]|=0x01;
 if (i&0x80)
  KeyMap[2]&=0x7F;
 else if (!keybstatus[XK_Control_R&255] &&
          !keybstatus[XK_Right&255])
  KeyMap[2]|=0x80;
 if (i&0x10)
  KeyMap[0]&=0xFB;
 else if (!keybstatus[XK_Alt_L&255] &&
          !keybstatus[XK_Up&255])
  KeyMap[0]|=0x04;
 if (i&0x20)
  KeyMap[2]&=0xDF;
 else if (!keybstatus[XK_Alt_R&255] &&
          !keybstatus[XK_Down&255])
  KeyMap[2]|=0x20;
 if (i&0x0F)
  KeyMap[2]&=0xFD;
 else if (!keybstatus[XK_space&255])
  KeyMap[2]|=0x02;
#endif
}

/****************************************************************************/
/*** This function is called every interrupt to flush the sound pipes and ***/
/*** sync the emulation                                                   ***/
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
/*** Pause the specified time                                             ***/
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
/*** Put a character in the display buffer for P2000M emulation mode      ***/
/****************************************************************************/
#define PUTCHAR_M \
 PIXEL *p; \
 p=((PIXEL*)DisplayBuf)+y*width*10+x*6+(width-480)/2+(height-240)*width/2; \
 c*=10; \
 if (eor) \
 { \
  fg=XPal[0]; \
  bg=XPal[1]; \
 } \
 else \
 { \
  fg=XPal[1]; \
  bg=XPal[0]; \
 } \
 for (i=0;i<10;++i,p+=width,c++) \
 { \
  K=FontBuf[c]; \
  if (K&0x20) p[0]=fg; else p[0]=bg; \
  if (K&0x10) p[1]=fg; else p[1]=bg; \
  if (K&0x08) p[2]=fg; else p[2]=bg; \
  if (K&0x04) p[3]=fg; else p[3]=bg; \
  if (K&0x02) p[4]=fg; else p[4]=bg; \
  if (K&0x01) p[5]=fg; else p[5]=bg; \
 } \
 if (ul) p[-width]=p[-width+1]=p[-width+2]= \
         p[-width+3]=p[-width+4]=p[-width+5]=fg;
         
#define PUTCHAR_HIRES_M \
 PIXEL *p; \
 p=((PIXEL*)DisplayBuf)+y*width*20+x*6+(width-480)/2+(height-480)*width/2; \
 c*=10; \
 if (eor) \
 { \
  fg=XPal[0]; \
  bg=XPal[1]; \
 } \
 else \
 { \
  fg=XPal[1]; \
  bg=XPal[0]; \
 } \
 for (i=0;i<10;++i,p+=width*2,c++) \
 { \
  K=FontBuf[c]; \
  if (K&0x20) p[0]=p[0+width]=fg; else p[0]=p[0+width]=bg; \
  if (K&0x10) p[1]=p[1+width]=fg; else p[1]=p[1+width]=bg; \
  if (K&0x08) p[2]=p[2+width]=fg; else p[2]=p[2+width]=bg; \
  if (K&0x04) p[3]=p[3+width]=fg; else p[3]=p[3+width]=bg; \
  if (K&0x02) p[4]=p[4+width]=fg; else p[4]=p[4+width]=bg; \
  if (K&0x01) p[5]=p[5+width]=fg; else p[5]=p[5+width]=bg; \
 } \
 if (ul) p[-width]=p[-width+1]=p[-width+2]= \
         p[-width+3]=p[-width+4]=p[-width+5]= \
         p[-width*2]=p[-width*2+1]=p[-width*2+2]= \
         p[-width*2+3]=p[-width*2+4]=p[-width*2+5]=fg;
         
static inline void PutChar_M (int x,int y,int c,int eor,int ul)
{
 int K;
 int i,fg,bg;
 K=c+(eor<<8)+(ul<<16);
 if (K==OldCharacter[y*80+x]) return;
 OldCharacter[y*80+x]=K;
 if (!videomode)
 {
  if (bpp==8)
  {
   #undef PIXEL
   #define PIXEL byte
   PUTCHAR_M
  }
  else if (bpp==16)
  {
   #undef PIXEL
   #define PIXEL word
   PUTCHAR_M
  }
  else
  {
   #undef PIXEL
   #define PIXEL unsigned
   PUTCHAR_M
  }
 }
 else
 {
  if (bpp==8)
  {
   #undef PIXEL
   #define PIXEL byte
   PUTCHAR_HIRES_M
  }
  else if (bpp==16)
  {
   #undef PIXEL
   #define PIXEL word
   PUTCHAR_HIRES_M
  }
  else
  {
   #undef PIXEL
   #define PIXEL unsigned
   PUTCHAR_HIRES_M
  }
 }
}

/****************************************************************************/
/*** Put a character in the display buffer for P2000T emulation mode      ***/
/****************************************************************************/
#define PUTCHAR_T \
 PIXEL *p; \
 p=((PIXEL*)DisplayBuf)+y*width*10+x*12+(width-480)/2+(height-240)*width/2; \
 c*=40; \
 fg=XPal[fg]; \
 bg=XPal[bg]; \
 if (!si) \
 { \
  for (i=0;i<10;++i,p+=width,c+=4) \
  { \
   K=FontBuf[c]; \
   if (K&0x08) p[0]=fg; else p[0]=bg; \
   if (K&0x04) p[1]=fg; else p[1]=bg; \
   if (K&0x02) p[2]=fg; else p[2]=bg; \
   if (K&0x01) p[3]=fg; else p[3]=bg; \
   K=FontBuf[c+1]; \
   if (K&0x80) p[4]=fg; else p[4]=bg; \
   if (K&0x40) p[5]=fg; else p[5]=bg; \
   if (K&0x20) p[6]=fg; else p[6]=bg; \
   if (K&0x10) p[7]=fg; else p[7]=bg; \
   if (K&0x08) p[8]=fg; else p[8]=bg; \
   if (K&0x04) p[9]=fg; else p[9]=bg; \
   if (K&0x02) p[10]=fg; else p[10]=bg; \
   if (K&0x01) p[11]=fg; else p[11]=bg; \
  } \
 } \
 else \
 { \
  if (si==2) \
   c+=20; \
  for (i=0;i<10;++i,p+=width,c+=2) \
  { \
   K=FontBuf[c]; \
   if (K&0x08) p[0]=fg; else p[0]=bg; \
   if (K&0x04) p[1]=fg; else p[1]=bg; \
   if (K&0x02) p[2]=fg; else p[2]=bg; \
   if (K&0x01) p[3]=fg; else p[3]=bg; \
   K=FontBuf[c+1]; \
   if (K&0x80) p[4]=fg; else p[4]=bg; \
   if (K&0x40) p[5]=fg; else p[5]=bg; \
   if (K&0x20) p[6]=fg; else p[6]=bg; \
   if (K&0x10) p[7]=fg; else p[7]=bg; \
   if (K&0x08) p[8]=fg; else p[8]=bg; \
   if (K&0x04) p[9]=fg; else p[9]=bg; \
   if (K&0x02) p[10]=fg; else p[10]=bg; \
   if (K&0x01) p[11]=fg; else p[11]=bg; \
  } \
 }

#define PUTCHAR_HIRES_T \
 PIXEL *p; \
 p=((PIXEL*)DisplayBuf)+y*width*20+x*12+(width-480)/2+(height-480)*width/2; \
 c*=40; \
 fg=XPal[fg]; \
 bg=XPal[bg]; \
 if (!si) \
 { \
  for (i=0;i<20;++i,p+=width,c+=2) \
  { \
   K=FontBuf[c]; \
   if (K&0x08) p[0]=fg; else p[0]=bg; \
   if (K&0x04) p[1]=fg; else p[1]=bg; \
   if (K&0x02) p[2]=fg; else p[2]=bg; \
   if (K&0x01) p[3]=fg; else p[3]=bg; \
   K=FontBuf[c+1]; \
   if (K&0x80) p[4]=fg; else p[4]=bg; \
   if (K&0x40) p[5]=fg; else p[5]=bg; \
   if (K&0x20) p[6]=fg; else p[6]=bg; \
   if (K&0x10) p[7]=fg; else p[7]=bg; \
   if (K&0x08) p[8]=fg; else p[8]=bg; \
   if (K&0x04) p[9]=fg; else p[9]=bg; \
   if (K&0x02) p[10]=fg; else p[10]=bg; \
   if (K&0x01) p[11]=fg; else p[11]=bg; \
  } \
 } \
 else \
 { \
  if (si==2) \
   c+=20; \
  for (i=0;i<10;++i,p+=width*2,c+=2) \
  { \
   K=FontBuf[c]; \
   if (K&0x08) p[0]=p[0+width]=fg; else p[0]=p[0+width]=bg; \
   if (K&0x04) p[1]=p[1+width]=fg; else p[1]=p[1+width]=bg; \
   if (K&0x02) p[2]=p[2+width]=fg; else p[2]=p[2+width]=bg; \
   if (K&0x01) p[3]=p[3+width]=fg; else p[3]=p[3+width]=bg; \
   K=FontBuf[c+1]; \
   if (K&0x80) p[4]=p[4+width]=fg; else p[4]=p[4+width]=bg; \
   if (K&0x40) p[5]=p[5+width]=fg; else p[5]=p[5+width]=bg; \
   if (K&0x20) p[6]=p[6+width]=fg; else p[6]=p[6+width]=bg; \
   if (K&0x10) p[7]=p[7+width]=fg; else p[7]=p[7+width]=bg; \
   if (K&0x08) p[8]=p[8+width]=fg; else p[8]=p[8+width]=bg; \
   if (K&0x04) p[9]=p[9+width]=fg; else p[9]=p[9+width]=bg; \
   if (K&0x02) p[10]=p[10+width]=fg; else p[10]=p[10+width]=bg; \
   if (K&0x01) p[11]=p[11+width]=fg; else p[11]=p[11+width]=bg; \
  } \
 }

static inline void PutChar_T (int x,int y,int c,int fg,int bg,int si)
{
 int K;
 int i;
 K=c+(fg<<8)+(bg<<16)+(si<<24);
 if (K==OldCharacter[y*40+x]) return;
 OldCharacter[y*40+x]=K;
 if (!videomode)
 {
  if (bpp==8)
  {
   #undef PIXEL
   #define PIXEL byte
   PUTCHAR_T
  }
  else if (bpp==16)
  {
   #undef PIXEL
   #define PIXEL word
   PUTCHAR_T
  }
  else
  {
   #undef PIXEL
   #define PIXEL unsigned
   PUTCHAR_T
  }
 }
 else
 {
  if (bpp==8)
  {
   #undef PIXEL
   #define PIXEL byte
   PUTCHAR_HIRES_T
  }
  else if (bpp==16)
  {
   #undef PIXEL
   #define PIXEL word
   PUTCHAR_HIRES_T
  }
  else
  {
   #undef PIXEL
   #define PIXEL unsigned
   PUTCHAR_HIRES_T
  }
 }
}

/****************************************************************************/
/*** Common.h contains the system-independent part of the screen refresh  ***/
/*** drivers                                                              ***/
/****************************************************************************/
#include "Common.h"
