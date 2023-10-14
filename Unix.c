/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                 Unix.c                               ***/
/***                                                                      ***/
/*** This file contains various Unix routines used by both the X-Windows  ***/
/*** and the Linux/SVGALib implementations                                ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include "P2000.h"
#include "Unix.h"
#include <stdio.h>
#include <time.h>

#ifdef SOUND
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <string.h>

int mastervolume=10;               /* Master volume setting                 */
static int audio_fd;               /* /dev/dsp file handle                  */
static int buf_size;               /* Sound buffer size                     */
static signed char *soundbuf;      /* Pointer to sound buffer               */
static byte *playbuf;              /* Buffer used to store sound reg values */
static int sample_rate;            /* Playback rate                         */
static int got_dsp=0;              /* 1 if /dev/dsp is present              */

/****************************************************************************/
/*** Initialise resources needed by USS implementation                    ***/
/****************************************************************************/
void InitSound (int mode)
{
 int i;
 if (!mode) return;
 if (mastervolume<0) mastervolume=0;
 if (mastervolume>15) mastervolume=15;
 if (Verbose)
  printf ("  Initialising sound card...\n    Opening /dev/dsp... ");
 audio_fd=open ("/dev/dsp",O_WRONLY,0);
 if (audio_fd<0)
 {
  if (Verbose) printf ("FAILED\n");
  return;
 }
 if (Verbose) printf ("OK\n    Setting format (8 bits)... ");
 i=AFMT_U8;
 if (ioctl(audio_fd,SNDCTL_DSP_SETFMT,&i)==-1 || i!=AFMT_U8)
 {
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return;
 }
 if (Verbose) printf ("OK\n    Setting mode (mono)... ");
 i=0;
 if (ioctl(audio_fd,SNDCTL_DSP_STEREO,&i)==-1 || i!=0)
 {
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return;
 }
 /* Calculate optimal sampling frequency. This rather messy method
    is necessary because USS requires the internal sound driver's
    buffer size to be a power of 2 bytes */
 for (i=4096;i>=128;i/=2)
  if (i*IFreq<=44100) break;
 sample_rate=i*IFreq;
 if (Verbose) printf ("OK\n    Setting sampling frequency... ");
 if (ioctl(audio_fd,SNDCTL_DSP_SPEED,&sample_rate)==-1)
 {
  printf ("FAILED\n");
  close (audio_fd);
  return;
 }
 if (Verbose) printf ("%d Hertz\n",sample_rate);
 /* The actual sampling rate might be different from the optimal one.
    Here we calculate the optimal buffer size */
 buf_size=sample_rate/IFreq;
 for (i=1;(1<<i)<=buf_size;++i);
 if (((1<<i)-buf_size)>(buf_size-(1<<(i-1)))) --i;
 buf_size=1<<i;
 if (Verbose) printf ("    Allocating buffers... ");
 soundbuf=malloc(buf_size);
 playbuf=malloc(buf_size);
 if (!soundbuf || !playbuf)
 {
  if (soundbuf) free (soundbuf);
  if (playbuf) free (playbuf);
  if (Verbose) printf ("FAILED\n");
  close (audio_fd);
  return;
 }
 memset (soundbuf,0,buf_size);
 memset (playbuf,0,buf_size);
 if (Verbose) printf ("OK\n    Setting 8 buffers of %d bytes each... ",buf_size);
 i=(8<<16)|(i);
 if (ioctl(audio_fd,SNDCTL_DSP_SETFRAGMENT,&i)==-1)
 {
  printf ("FAILED\n");
  close (audio_fd);
  return;
 }
 if (Verbose)
  printf ("OK\n    Actual interrupt frequency is %3.1fHz\n",
          (float)sample_rate/(float)buf_size);
 got_dsp=1;
 return;
}

/****************************************************************************/
/*** Free resources taken by InitSound()                                  ***/
/****************************************************************************/
void TrashSound (void)
{
 if (!got_dsp) return;
 close (audio_fd);
 if (soundbuf) free (soundbuf);
 if (playbuf) free (playbuf);
 got_dsp=0;
}

/****************************************************************************/
/*** Increase master volume                                               ***/
/****************************************************************************/
void IncreaseSoundVolume (void)
{
 if (mastervolume<15) ++mastervolume;
}

/****************************************************************************/
/*** Decrease master volume                                               ***/
/****************************************************************************/
void DecreaseSoundVolume (void)
{
 if (mastervolume) --mastervolume;
}

/****************************************************************************/
/*** This function is called when the sound register is written to        ***/
/****************************************************************************/
void WriteSound (int toggle)
{
 static int last=-1;
 int pos,val;
 if (!got_dsp) return; 
 if (toggle!=last)
 {
  last=toggle;
  pos=(buf_size-1)-(buf_size*Z80_ICount/Z80_IPeriod);
  val=(toggle)? (-mastervolume*8):(mastervolume*8);
  soundbuf[pos]=val;
 }
}

/****************************************************************************/
/*** Flush sound pipes and sync emulation                                 ***/
/****************************************************************************/
int Sound_FlushSound (void)
{
 int i;
 static int soundstate=0;
 static int sample_count=1;
 if (!got_dsp) return 0;
 for (i=0;i<buf_size;++i)
 {
  if (soundbuf[i])
  {
   soundstate=soundbuf[i];
   soundbuf[i]=0;
   sample_count=sample_rate/1000;
  }
  playbuf[i]=soundstate+128;
  if (!--sample_count)
  {
   sample_count=sample_rate/1000;
   if (soundstate>0) --soundstate;
   if (soundstate<0) ++soundstate;
  }
 }
 write (audio_fd,playbuf,buf_size);
 return 1;
}
#endif

#ifdef JOYSTICK
#include <linux/joystick.h>
#include <unistd.h>
#include <fcntl.h>

static struct JS_DATA_TYPE jscentre; /* Joystick centre position            */
static int joy_fd;                   /* Joystick file handle                */

/****************************************************************************/
/*** Initialise resources needed by joystick implementation               ***/
/****************************************************************************/
void InitJoystick (int mode)
{
 if (!mode) return;
 if (Verbose)
  printf ("  Initialising joystick...\n    Opening /dev/js0... ");
 joy_fd=open ("/dev/js0",O_RDONLY);
 if (Verbose) printf ((joy_fd>=0)? "OK\n    Calibrating... ":"FAILED\n");
 if (joy_fd>=0)
 {
  read (joy_fd,&jscentre,JS_RETURN);
  if (Verbose) printf ("OK\n");
 }
}

/****************************************************************************/
/*** Free resources taken by InitJoystick()                               ***/
/****************************************************************************/
void TrashJoystick (void)
{
 if (joy_fd>=0) close (joy_fd);
}

/****************************************************************************/
/*** Read current joystick position                                       ***/
/****************************************************************************/
int ReadJoystick (void)
{
 int i;
 struct JS_DATA_TYPE js;
 i=0;
 if (joy_fd>=0)
 {
  read (joy_fd,&js,JS_RETURN);
  if (js.buttons&1) i|=1;
  if (js.buttons&2) i|=2;
  if (js.x<(jscentre.x/2)) i|=0x40;
  if (js.x>(jscentre.x*3/2)) i|=0x80;
  if (js.y<(jscentre.y/2)) i|=0x10;
  if (js.y>(jscentre.y*3/2)) i|=0x20;
 }
 return i;
}
#endif

/****************************************************************************/
/*** Return the time elapsed in microseconds                              ***/
/****************************************************************************/
int ReadTimer (void)
{
#ifdef HAVE_CLOCK
 return (int)((float)clock()*1000000.0/(float)CLOCKS_PER_SEC);
#else
 /* If clock() is unavailable, just return a large number */
 static int a=0;
 a+=1000000;
 return a;
#endif
}
