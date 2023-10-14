/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                  SB.c                                ***/
/***                                                                      ***/
/*** This file contains the P2000 sound hardware emulation code for       ***/
/*** MS-DOS/SoundBlaster implementations                                  ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include "P2000.h"
#include "MSDOS.h"

#include <conio.h>
#include <stdlib.h>
#include <go32.h>
#include <dpmi.h>
#include <string.h>
#include <stdio.h>
#include <dos.h>
#include <pc.h>
#include <sys/farptr.h>

#define SAMPLE_RATE     44100                 /* Playback rate in Hz        */

struct _SB_Info
{
  word baseport;
  byte irq;
  byte dma_low;
  byte dma_high;
  byte type;
  word emu_baseport;
  word mpu_baseport;
};

static volatile int ok_to_write_buffer=0;    /* Set every SB interrupt      */
static int num_samples;                      /* Number of samples in buffer */
static int got_sb=0;                         /* 1 if SB detected            */
static unsigned sb_dsp_version;              /* SB DSP version              */
static word sb_port;                         /* Port at which SB is located */
static byte sb_dma_channel;                  /* SB DMA channel number       */
static byte sb_irq;                          /* SB IRQ number               */
static byte sb_irq_int;                      /* SB interrupt number         */
static unsigned sb_buf[2];                   /* DMA buffer addresses        */
static unsigned sb_phys;                     /* Physical buffer address     */
static unsigned sb_sel;                      /* DMA buffer selector         */
static signed char *playbuf,*soundbuf;       /* Sound reg buffers           */
static byte sb_default_pic1,sb_default_pic2; /* Original PIC values         */
static int sb_bufnum;                        /* Current buffer being played */
static _go32_dpmi_seginfo sbirq;             /* SB interrupt id             */
static int highspeed=0;                      /* 1 if using higspeed mode    */
static int dma_size=0;                       /* Size of DMA buffer          */
static int soundstate;                       /* Current sound output value  */
int mastervolume=10;                         /* Master volume setting       */

static struct _SB_Info SB_Info;              /* BLASTER settings            */

/****************************************************************************/
/*** This function parses the BLASTER environment variable setting        ***/
/****************************************************************************/
static void GetSBInfo (void)
{
  char *blaster=getenv ("BLASTER");
  memset (&SB_Info,0,sizeof(SB_Info));
  if (blaster)
  {
   strupr (blaster);
   while (*blaster)
   {
    while (*blaster==' ' || *blaster=='\t')
     ++blaster;
    switch (*blaster++)
    {
     case 'A': SB_Info.baseport=(word)strtol(blaster,NULL,16);
               break;
     case 'I': SB_Info.irq=(byte)strtol(blaster,NULL,10);
               break;
     case 'D': SB_Info.dma_low=(byte)strtol(blaster,NULL,10);
               break;
     case 'H': SB_Info.dma_high=(byte)strtol(blaster,NULL,10);
               break;
     case 'T': SB_Info.type=(byte)strtol(blaster,NULL,10);
               break;
     case 'E': SB_Info.emu_baseport=(word)strtol(blaster,NULL,16);
               break;
     case 'P': SB_Info.mpu_baseport=(word)strtol(blaster,NULL,16);
               break;
    }
    while (*blaster && *blaster!=' ' && *blaster!='\t')
     ++blaster;
   }
  }
}

/****************************************************************************/
/*** This function reads a value from the SB DSP                          ***/
/****************************************************************************/
static int sb_read_dsp()
{
 int x;
 for (x=0; x<0xfffff; x++)
  if (inportb(0x0E + sb_port) & 0x80)
   return inportb(0x0A+sb_port);
 return -1; 
}

/****************************************************************************/
/*** This function writes a value to the SB DSP                           ***/
/****************************************************************************/
static int sb_write_dsp(byte val)
{
 int x;
 for (x=0; x<0xfffff; x++)
 {
  if (!(inportb(0x0C+sb_port) & 0x80))
  {
   outportb(0x0C+sb_port, val);
   return 1;
  }
 }
 return 0;
}

/****************************************************************************/
/*** This function turnes the SB speaker connection on                    ***/
/****************************************************************************/
static void sb_speaker_on (void)
{
 sb_write_dsp(0xD1);
}

/****************************************************************************/
/*** This function turnes the SB speaker connection off                   ***/
/****************************************************************************/
static void sb_speaker_off (void)
{
 sb_write_dsp(0xD3);
}

/****************************************************************************/
/*** This function sets the playback rate and returns the actual one set  ***/
/****************************************************************************/
static unsigned sb_set_sample_rate(unsigned rate)
{
 unsigned rateval,realfreq;
 rateval=256-1000000/rate;
 realfreq=1000000/(256-rateval);
 sb_write_dsp(0x40);
 sb_write_dsp((byte)rateval);
 highspeed=(rateval>211);
 return realfreq;
}

/****************************************************************************/
/*** This function resets the SB DSP. It returns 0 in case of a failure   ***/
/****************************************************************************/
static int sb_reset_dsp()
{
 int i,j;
 for (i=0;i<16;++i)
 {
  outportb(0x06+sb_port, 1);
  for (j=0;j<10;++j) inportb (0x06+sb_port);
  outportb(0x06+sb_port, 0);
  if (sb_read_dsp() == 0xAA)
   return 1;
 }
 return 0;
}

/****************************************************************************/
/*** This function returns the SB DSP version                             ***/
/****************************************************************************/
static unsigned sb_read_dsp_version()
{
 unsigned hi, lo;
 sb_write_dsp(0xE1);
 hi = sb_read_dsp();
 lo = sb_read_dsp();
 return ((hi << 8) + lo);
}

/****************************************************************************/
/*** This function initialises the SB for playing back the specified      ***/
/*** number of samples in auto-initialised DMA mode                       ***/
/****************************************************************************/
static void sb_play_buffer(int size)
{
 sb_write_dsp(0x48);
 sb_write_dsp((size-1) & 0xff);
 sb_write_dsp((size-1) >> 8);
 if (highspeed)
  sb_write_dsp(0x90);
 else
  sb_write_dsp(0x1C);
}

/****************************************************************************/
/*** This function is called when a buffer has been played. It fills the  ***/
/*** buffer which has just been played and sets the ok_to_fill_buffer     ***/
/*** variable                                                             ***/
/****************************************************************************/
static int sb_interrupt(void)
{
 static int int_busy=0;
 static int int_count=16;
 static int sample_count=SAMPLE_RATE/1000;
 int i;
 unsigned offset;
 inportb(sb_port+0x0E);
 nofunc();
 outportb(0x20, 0x20);
 nofunc();
 if (sb_irq>=8)
  outportb(0xA0, 0x20);
 if (!int_busy)
 {
  int_busy=1;
  __enable();
  if (!--int_count)
  {
   int tmp=_dma_todo (sb_dma_channel);
   sb_bufnum=(tmp<=dma_size)? 0:1;
   int_count=16;
  }
  _farsetsel (sb_sel);
  offset=sb_buf[sb_bufnum];
  for (i=0;i<num_samples;++i)
  {
   if (playbuf[i])
   {
    soundstate=playbuf[i];
    playbuf[i]=0;
    sample_count=SAMPLE_RATE/1000;
   }
   _farnspokeb (offset+i,soundstate+128);
   if (!--sample_count)
   {
    sample_count=SAMPLE_RATE/1000;
    if (soundstate>0) --soundstate;
    if (soundstate<0) ++soundstate;
   }
  }
  sb_bufnum=(sb_bufnum+1)&1;
  int_busy=0;
 }
 else
 {
  sb_bufnum=(sb_bufnum+1)&1;
  __enable();
 }
 ok_to_write_buffer=1;
 return 0;
}

/****************************************************************************/
/*** This function installs our interrupt handler                         ***/
/****************************************************************************/
static void sb_install_interrupts()
{
 sbirq.pm_offset = (unsigned long)sb_interrupt;
 sbirq.pm_selector = _my_cs();
 SetInt (sb_irq_int, &sbirq);
}

/****************************************************************************/
/*** This function looks for an SB and returns 1 if one has been detected ***/
/****************************************************************************/
static int sb_detect()
{
 GetSBInfo ();
 sb_port=SB_Info.baseport;
 if (sb_port==0)
  return 0;
 if (Verbose) printf ("Checking for a SoundBlaster at %X...",sb_port);
 if (!sb_reset_dsp())
 {
  if (Verbose) printf ("FAILED\n");
  return 0;
 }
 sb_dsp_version=sb_read_dsp_version();
 if (sb_dsp_version<0x200)
 {
  if (Verbose) printf ("MODEL TOO OLD\n");
  return 0;
 }
 if (Verbose) printf ("OK\n");
 sb_dma_channel=SB_Info.dma_low;
 sb_irq=SB_Info.irq;
 sb_irq_int=(sb_irq<8)? sb_irq+8 : sb_irq+0x70-8;
 got_sb=1;
 return 1;
}

/****************************************************************************/
/*** Initialise resources needed by the SB implementation. This function  ***/
/*** returns 0 in case of a failure                                       ***/
/****************************************************************************/
int SB_Init (void)
{
 int dma_len,i;
 if (mastervolume<0) mastervolume=0;
 if (mastervolume>15) mastervolume=15;
 if (!got_sb)
 {
  if (!sb_detect())
   return 0;
 }
 sb_set_sample_rate(SAMPLE_RATE);
 dma_size=SAMPLE_RATE/IFreq;
 num_samples=dma_size;
 dma_len=dma_size*2;
 if (Verbose) printf ("Allocating buffers...");
 playbuf=malloc (dma_len/2);
 if (!playbuf)
 {
  if (Verbose) printf ("FAILED\n");
  return 0;
 }
 soundbuf=malloc (dma_len/2);
 if (!soundbuf)
 {
  if (Verbose) printf ("FAILED\n");
  free (playbuf);
  return 0;
 }
 memset (playbuf,0,dma_len/2);
 memset (soundbuf,0,dma_len/2);
 if (Verbose) printf ("OK\nAllocating DMA buffer...");
 if (_dma_allocate_mem(dma_len, &sb_sel, &sb_buf[0], &sb_phys) == 0)
 {
  if (Verbose) printf ("FAILED\n");
  free (playbuf);
  free (soundbuf);
  return 0;
 }
 if (Verbose) printf ("OK\n");
 sb_default_pic1 = inportb(0x21);
 if (sb_irq > 7)
 {
  nofunc();
  outportb(0x21, sb_default_pic1 & 0xFB); 
  nofunc();
  sb_default_pic2 = inportb(0xA1);
  outportb(0xA1, sb_default_pic2 & (~(1<<(sb_irq-8))));
 }
 else
 {
  nofunc();
  outportb(0x21, sb_default_pic1 & (~(1<<sb_irq)));
 }
 sb_install_interrupts();
 sb_speaker_on();
 sb_buf[1] = sb_buf[0] + dma_len/2;
 _farsetsel (sb_sel);
 for (i=0;i<num_samples*2;++i)
  _farnspokeb (sb_buf[0]+i,0x80);
 sb_bufnum = 0;
 _dma_start(sb_dma_channel, sb_phys, dma_len, 1);
 sb_play_buffer(dma_len/2);
 return 1;
}

/****************************************************************************/
/*** Free all resources taken by SB_Init()                                ***/
/****************************************************************************/
void SB_Reset (void)
{
 if (!got_sb)
  return;
 sb_speaker_off ();
 sb_write_dsp (0xD0);
 _dma_stop(sb_dma_channel);
 sb_reset_dsp();
 ResetInt (sb_irq_int,&sbirq);
 outportb(0x21, sb_default_pic1);
 nofunc();
 if (sb_irq>7)
  outportb(0xA1, sb_default_pic2);
 _dma_free_mem(sb_sel);
 free (soundbuf);
 free (playbuf);
}

/****************************************************************************/
/*** This function is called when the sound register is written to        ***/
/****************************************************************************/
void SB_Sound (int toggle)
{
 static int last=-1;
 int pos,val;
 if (toggle!=last)
 {
  last=toggle;
  pos=(num_samples-1)-(num_samples*Z80_ICount/Z80_IPeriod);
  val=(toggle)? (-mastervolume*8):(mastervolume*8);
  if (Sync)
   soundbuf[pos]=val;
  else
   playbuf[pos]=val;
 }
}

/****************************************************************************/
/*** Flush sound output and sync emulation                                ***/
/****************************************************************************/
void SB_FlushSound (void)
{
 if (Sync)
 {
  while (!ok_to_write_buffer);
  __disable ();
  memcpy (playbuf,soundbuf,num_samples*sizeof(signed char));
  ok_to_write_buffer=0;
  __enable ();
  memset (soundbuf,0,num_samples*sizeof(signed char));
 }
}

/****************************************************************************/
/*** Increase volume                                                      ***/
/****************************************************************************/
void SB_IncreaseVolume (void)
{
 if (mastervolume<15) ++mastervolume;
}

/****************************************************************************/
/*** Decrease volume                                                      ***/
/****************************************************************************/
void SB_DecreaseVolume (void)
{
 if (mastervolume) --mastervolume;
}

