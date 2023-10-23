/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                               allegro.c                              ***/
/***                                                                      ***/
/*** This file contains the Allegro 5 drivers.                            ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/*** Copyright (C) Stefano Bodrato 2013                                   ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#define ALLEGRO_STATICLINK
// #define SCREEN_TILE_WIDTH 24
// #define SCREEN_TILE_HEIGHT 30
#define CHAR_TILE_WIDTH 24
#define CHAR_TILE_HEIGHT 30
#define CHAR_PIXEL_WIDTH 4
#define CHAR_PIXEL_HEIGHT 3

#define FONT_BITMAP_WIDTH (96+64+64)*CHAR_TILE_WIDTH

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 

#include "P2000.h"
#include "Unix.h"
#include "Utils.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

//#undef SOUND

#ifdef SOUND
#include <allegro5/allegro_audio.h>

ALLEGRO_AUDIO_STREAM *stream = NULL;
ALLEGRO_MIXER *mixer = NULL;
ALLEGRO_EVENT_QUEUE *queue = NULL;
ALLEGRO_EVENT event;
int buf_size;
int sample_rate;

signed char *soundbuf;      /* Pointer to sound buffer               */

int8_t *playbuf;
int mastervolume=4;               /* Master volume setting                 */
static int sound_active=1;
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#ifndef NR_KEYS
#define NR_KEYS 128
#endif

ALLEGRO_DISPLAY *display = NULL;

char *Title="M2000 v0.7-SNAPSHOT"; /* Title for -help output            */

ALLEGRO_KEYBOARD_STATE kbdstate;

int videomode;                    /* T emulation only: 
                                        0=960x720
                                        1=960x720 (pixelated font)         */ 
static int *OldCharacter;         /* Holds characters on the screen        */

ALLEGRO_BITMAP *FontBuf = NULL;
ALLEGRO_BITMAP *FontBuf_bk = NULL;
ALLEGRO_BITMAP *FontBuf_scaled = NULL;
ALLEGRO_BITMAP *FontBuf_bk_scaled = NULL;
ALLEGRO_BITMAP *ScreenshotBuf = NULL;

//byte *DisplayBuf;               /* Screen buffer                            */

//unsigned ReadTimerMin;             /* Minimum number of micro seconds       */
//                                   /* between interrupts                    */
static int width = 960;
static int height = 720;          /* Width and height of the display buffer   */
//static int OldTimer=0;             /* Value of timer at previous interrupt  */
//static int NewTimer=0;             /* New value of the timer                */
int soundmode=255;                 /* Sound mode, 255=auto-detect           */
#ifdef SOUND
static int soundoff=0;             /* If 1, sound is turned off             */
#endif
#ifdef JOYSTICK
int joymode=1;                     /* If 0, do not use joystick             */
#endif
//static struct termios termold;     /* Original terminal settings            */

static byte Pal[8*3] =             /* SAA5050 palette                       */
{
  0x00,0x00,0x00, //black
  0xFF,0x00,0x00, //red
  0x00,0xFF,0x00, //green
  0xFF,0xFF,0x00, //yellow
  0x00,0x00,0xFF, //blue
  0xFF,0x00,0xFF, //magenta
  0x00,0xFF,0xFF, //cyan
  0xFF,0xFF,0xFF  //white
};

//static int PausePressed=0;               /* 1 if pause key is pressed       */
//static byte keybstatus[NR_KEYS];         /* 1 if a certain key is pressed   */
//static int makeshot=0;                   /* 1 -> take a screen shot         */
static int calloptions=0;                /* 1 -> call OptionsDialogue()     */

/*
Y \ X   0       1        2       3        4       5        6       7
0       LEFT    6        up      Q        3       5        7       4
1       TAB     H        Z       S        D       G        J       F
2       . *     SPACE    00 *    0 *      #       DOWN     ,       RIGHT
3       SHLOCK  N        <       X        C       B        M       V
4       CODE    Y        A       W        E       T        U       R
5       CLRLN   9        + *     - *      BACKSP  0        1       -
6       9 *     O        8 *     7 *      ENTER   P        8       @
7       3 *     .        2 *     1 *      ->      /        K       2
8       6 *     L        5 *     4 *      1/4     ;        I       :
9       LSHIFT                                                     RSHIFT
*/

static unsigned char keymask[]=
{
  ALLEGRO_KEY_LEFT,       ALLEGRO_KEY_6,         ALLEGRO_KEY_UP,          ALLEGRO_KEY_Q,          ALLEGRO_KEY_3,          ALLEGRO_KEY_5,         ALLEGRO_KEY_7,      ALLEGRO_KEY_4,
  ALLEGRO_KEY_TAB,        ALLEGRO_KEY_H,         ALLEGRO_KEY_Z,           ALLEGRO_KEY_S,          ALLEGRO_KEY_D,          ALLEGRO_KEY_G,         ALLEGRO_KEY_J,      ALLEGRO_KEY_F,
  ALLEGRO_KEY_PAD_ENTER,  ALLEGRO_KEY_SPACE,     ALLEGRO_KEY_PAD_DELETE,  ALLEGRO_KEY_PAD_0,      ALLEGRO_KEY_SLASH,      ALLEGRO_KEY_DOWN,      ALLEGRO_KEY_COMMA,  ALLEGRO_KEY_RIGHT,
  ALLEGRO_KEY_CAPSLOCK,   ALLEGRO_KEY_N,         ALLEGRO_KEY_BACKSLASH2,  ALLEGRO_KEY_X,          ALLEGRO_KEY_C,          ALLEGRO_KEY_B,         ALLEGRO_KEY_M,      ALLEGRO_KEY_V,
  ALLEGRO_KEY_LCTRL,      ALLEGRO_KEY_Y,         ALLEGRO_KEY_A,           ALLEGRO_KEY_W,          ALLEGRO_KEY_E,          ALLEGRO_KEY_T,         ALLEGRO_KEY_U,      ALLEGRO_KEY_R,
  ALLEGRO_KEY_BACKSLASH,  ALLEGRO_KEY_9,         ALLEGRO_KEY_PAD_PLUS,    ALLEGRO_KEY_OPENBRACE,  ALLEGRO_KEY_BACKSPACE,  ALLEGRO_KEY_0,         ALLEGRO_KEY_1,      ALLEGRO_KEY_PAD_MINUS,
  ALLEGRO_KEY_PAD_9,      ALLEGRO_KEY_O,         ALLEGRO_KEY_PAD_8,       ALLEGRO_KEY_PAD_7,      ALLEGRO_KEY_ENTER,      ALLEGRO_KEY_P,         ALLEGRO_KEY_8,      ALLEGRO_KEY_SEMICOLON,
  ALLEGRO_KEY_PAD_3,      ALLEGRO_KEY_FULLSTOP,  ALLEGRO_KEY_PAD_2,       ALLEGRO_KEY_PAD_1,      ALLEGRO_KEY_RCTRL,      ALLEGRO_KEY_MINUS,     ALLEGRO_KEY_K,      ALLEGRO_KEY_2,
  ALLEGRO_KEY_PAD_6,      ALLEGRO_KEY_L,         ALLEGRO_KEY_PAD_5,       ALLEGRO_KEY_PAD_4,      ALLEGRO_KEY_CLOSEBRACE, ALLEGRO_KEY_TILDE,     ALLEGRO_KEY_I,      ALLEGRO_KEY_QUOTE,
  ALLEGRO_KEY_LSHIFT,     0,                     0,                       0,                      0,                      0,                     0,                  ALLEGRO_KEY_RSHIFT
};


/****************************************************************************/
/*** This function is called by the screen refresh drivers to copy the    ***/
/*** off-screen buffer to the actual display                              ***/
/****************************************************************************/
static void PutImage (void)
{
  al_unlock_bitmap(al_get_backbuffer(display));
  al_flip_display();
}


/****************************************************************************/
/*** Deallocate resources taken by InitMachine()                          ***/
/****************************************************************************/
void TrashMachine(void)
{
  if (Verbose) printf("\n\nShutting down...\n");
  al_destroy_display(display);
  al_uninstall_keyboard();
  al_shutdown_primitives_addon();
  al_shutdown_image_addon();
#ifdef SOUND
  al_destroy_event_queue(queue);
  al_destroy_audio_stream(stream);
  al_uninstall_audio();
  free (soundbuf);
#endif

 /*
#ifdef SOUND
 TrashSound ();
#endif
#ifdef JOYSTICK
 TrashJoystick ();
#endif
*/

  if (FontBuf) al_destroy_bitmap(FontBuf);
  if (FontBuf_bk) al_destroy_bitmap(FontBuf_bk);
  if (FontBuf_scaled) al_destroy_bitmap(FontBuf_scaled);
  if (FontBuf_bk_scaled) al_destroy_bitmap(FontBuf_bk_scaled);
  if (ScreenshotBuf) al_destroy_bitmap(ScreenshotBuf);
  if (OldCharacter) free (OldCharacter);
  //if (CharacterCache) free (CharacterCache);
}

/****************************************************************************/
/*** Initialise all resources needed by the Linux/SVGALib implementation  ***/
/****************************************************************************/
int InitMachine(void)
{
  // int c,i,j;
  int i;

  /* This block has been moved in 'LoadFont' */
  /*
  if (Verbose)
   printf ("Initialising Allegro drivers:\n");
  if (!al_init() || !al_init_primitives_addon()) {
    fprintf(stderr, "Failed to initialize allegro!\n");
    return -1;
  }
  */

  if (Verbose)
    printf("Initialising keyboard...");

  if (!al_install_keyboard())
  {
    if (Verbose)
      printf("FAILED\n");
    return -1;
  }

  printf("OK\nCreating the output window... ");

  display = al_create_display(width, height);

  al_clear_to_color(al_map_rgb(0, 0, 0));
  // al_flip_display();

  if (Verbose)
  {
    if (!display)
    {
      printf("FAILED\n");
      return -1;
    }
    else
      printf("OK\n");
  }

  if (P2000_Mode)
  {
    Pal[0] = Pal[1] = Pal[2] = 0;
    Pal[3] = Pal[4] = Pal[5] = 255;
  }

  /*
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
  */

  if (Verbose)
    printf("  Allocating cache buffers... ");
  if (P2000_Mode)
    i = 80 * 24;
  else
    i = 40 * 24;
  OldCharacter = malloc(i * sizeof(int));
  if (!OldCharacter)
  {
    if (Verbose)
      puts("FAILED");
    return (0);
  }

  memset(OldCharacter, -1, i * sizeof(int));

  /*
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
  }*/

  if (Verbose)
    puts("OK");
  /*
   #ifdef JOYSTICK
   InitJoystick (joymode);
  #endif
  */
  
  InitScreenshotFile();
  InitVRAMFile();

#ifdef SOUND
  if (Verbose)
    printf("Initializing sound...");

  if (!al_install_audio())
  {
    if (Verbose)
      printf("FAILED\n");
    sound_active = 0;
  }

  if (!al_reserve_samples(0))
  {
    if (Verbose)
      printf("FAILED\n");
    sound_active = 0;
  }

  if (Verbose)
    printf("OK\n");

  if (Verbose)
    printf("  Creating the audio stream: ");

  for (i = 4096; i >= 128; i /= 2)
    if (i * IFreq <= 44100)
      break;
  sample_rate = i * IFreq; // should this be within for loop?
  if (Verbose)
    printf("%d Hz...", sample_rate);
  /* The actual sampling rate might be different from the optimal one.
     Here we calculate the optimal buffer size */
  buf_size = sample_rate / IFreq;
  for (i = 1; (1 << i) <= buf_size; ++i)
    ;
  if (((1 << i) - buf_size) > (buf_size - (1 << (i - 1))))
    --i;

  buf_size = 1 << i;
  soundbuf = malloc(buf_size);

  stream = al_create_audio_stream(2, buf_size, sample_rate, ALLEGRO_AUDIO_DEPTH_UINT8, ALLEGRO_CHANNEL_CONF_1);

  if (Verbose)
  {
    if (!stream || !soundbuf)
    {
      printf("FAILED\n");
      sound_active = 0;
    }
    else
      printf("OK\n");
  }

  if (Verbose)
    printf("  Connecting to the default mixer...");
  mixer = al_get_default_mixer();
  if (Verbose)
  {
    if (!al_attach_audio_stream_to_mixer(stream, mixer))
    {
      printf("FAILED\n");
      sound_active = 0;
    }
    else
      printf("OK\n");
  }
  else
  {
    if (!al_attach_audio_stream_to_mixer(stream, mixer))
      sound_active = 0;
  }

  if (Verbose)
    printf("  Registering the sound event...");
  queue = al_create_event_queue();
  al_register_event_source(queue, al_get_audio_stream_event_source(stream));
  al_register_event_source(queue, al_get_display_event_source(display));

  if (!queue)
  {
    if (Verbose)
      printf("FAILED\n");
    sound_active = 0;
  }
  while ((playbuf = al_get_audio_stream_fragment(stream)) == NULL)
  {
  };

  if (Verbose)
    printf("OK\n");

    // al_set_mixer_postprocess_callback(mixer, mixer_pp_callback, mixer);

//  InitSound (soundmode);
#endif

  /*
   if (Verbose) printf ("  Initialising timer...");
   ReadTimerMin=1000000/IFreq;
   OldTimer=ReadTimer ();
  */

  /*
  if (Verbose) printf ("  Initialising keyboard...");
  if (keyboard_init()) { if (Verbose) printf ("FAILED\n"); return 0; }
  keyboard_seteventhandler (keyb_handler);
  */
  // if (Verbose) printf ("OK\n");
  return 1;
}

void drawFontRegion(float x1, float y1, float x2, float y2)
{
  /* Draw the font on an internal bitmap */
  al_set_target_bitmap(FontBuf);
  al_draw_filled_rectangle(x1, y1, x2, y2, al_map_rgb(255, 255, 255));

  /* Draw the inverted font on an internal bitmap */
  al_set_target_bitmap(FontBuf_bk);
  al_draw_filled_rectangle(x1, y1, x2, y2, al_map_rgb(0, 0, 0));
}

/****************************************************************************/
/*** This function loads a font and converts it if necessary              ***/
/****************************************************************************/
int LoadFont(char *filename)
{
  int i, line, x, y, pixelPos;
  int linePixels, linePixelsPrev, linePixelsNext;
  int pixelN, pixelE, pixelS, pixelW, pixelSW, pixelSE, pixelNW, pixelNE;
  char *TempBuf;
  FILE *F;

  if (Verbose)
    printf("Initialising Allegro drivers...");
  if (!al_init() || !al_init_primitives_addon() || !al_init_image_addon())
  {
    if (Verbose)
      puts("FAILED");
    return -1;
  }
  if (Verbose)
    puts("OK");

  al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);

  if (Verbose)
    printf("Loading font %s...\n", filename);
  if (Verbose)
    printf("  Allocating memory... ");

  if (!FontBuf)
  {
    FontBuf = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT);
    if (!FontBuf)
    {
      if (Verbose)
        puts("FAILED");
      return -1;
    }
  }

  if (!FontBuf_bk)
  {
    FontBuf_bk = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT);
    if (!FontBuf_bk)
    {
      if (Verbose)
        puts("FAILED");
      return -1;
    }
  }

  if (!P2000_Mode)
  {
    if (!FontBuf_scaled) //double height
    {
      FontBuf_scaled = al_create_bitmap(FONT_BITMAP_WIDTH, 2*CHAR_TILE_HEIGHT);
      if (!FontBuf_scaled)
      {
        if (Verbose)
          puts("FAILED");
        return -1;
      }
    }

    if (!FontBuf_bk_scaled)
    {
      FontBuf_bk_scaled = al_create_bitmap(FONT_BITMAP_WIDTH, 2*CHAR_TILE_HEIGHT);
      if (!FontBuf_bk_scaled)
      {
        if (Verbose)
          puts("FAILED");
        return -1;
      }
    }
  }

  al_set_target_bitmap(FontBuf_bk);
  al_clear_to_color(al_map_rgb(255, 255, 255));

  al_set_target_bitmap(FontBuf);
  al_clear_to_color(al_map_rgb(0, 0, 0));

  TempBuf = malloc(2240);
  if (!TempBuf)
  {
    if (Verbose)
      puts("FAILED");
    return -1;
  }
  if (Verbose)
    puts("OK");
  if (Verbose)
    printf("  Opening... ");
  i = 0;
  F = fopen(filename, "rb");
  if (F)
  {
    printf("Reading... ");
    if (fread(TempBuf, 2240, 1, F))
      i = 1;
    fclose(F);
  }
  if (Verbose)
    puts((i) ? "OK" : "FAILED");
  if (!i)
    return 0;

  /* Stretch 6x10 characters to 12x20, so we can do character rounding */
  for (i = 0; i < (96 + 64 + 64) * 10; i += 10) //96 alpha + 64 graphic (cont) + 64 graphic (sep)
  {
    linePixels = 0;
    linePixelsNext = TempBuf[i] << 6;
    for (line = 0; line < 10; ++line)
    {
      y = line * CHAR_PIXEL_HEIGHT;
      linePixelsPrev = linePixels >> 6;
      linePixels = linePixelsNext >> 6;
      linePixelsNext = line < 9 ? TempBuf[i + line + 1] : 0;

      for (pixelPos = 0; pixelPos < 6; ++pixelPos)
      {
        x = (i * 6 / 10 + pixelPos) * CHAR_PIXEL_WIDTH;
        if (linePixels & 0x20) // bit 6 set = pixel set
        {
          drawFontRegion(x, y, x + CHAR_PIXEL_WIDTH, y + CHAR_PIXEL_HEIGHT);
        }
        else 
        {
          if (i < 96 * 10 && videomode == 0) // check if within alpanum character range
          {
            // for character rounding, look at surrounding pixels
            pixelN = line > 0 ? (linePixelsPrev & 0x20) : 0;
            pixelE = pixelPos < 5 ? (linePixels & 0x10) : 0;
            pixelS = line < 9 ? (linePixelsNext & 0x20) : 0;
            pixelW = pixelPos > 0 ? (linePixels & 0x40) : 0;
            pixelNE = (line > 0 && pixelPos < 5) ? (linePixelsPrev & 0x10) : 0;
            pixelSE = (line < 9 && pixelPos < 5) ? (linePixelsNext & 0x10) : 0;
            pixelSW = (line < 9 && pixelPos > 0) ? (linePixelsNext & 0x40) : 0;
            pixelNW = (line > 0 && pixelPos > 0) ? (linePixelsPrev & 0x40) : 0;

            // the extra rounding pixels are in the shape of a (rotated) L

            // rounding in NW direction
            if (pixelN && pixelW && !pixelNW)
            {
              drawFontRegion(x,   y,   x+3, y+1);
              drawFontRegion(x,   y+1, x+1, y+2);
            }
            // rounding in NE direction
            if (pixelN && pixelE && !pixelNE)
            {
              drawFontRegion(x+1, y,   x+4, y+1);
              drawFontRegion(x+3, y+1, x+4, y+2);
            }
            // rounding in SE direction
            if (pixelS && pixelE && !pixelSE)
            {
              drawFontRegion(x+3, y+1, x+4, y+2);
              drawFontRegion(x+1, y+2, x+4, y+3);
            }
            // rounding in SW direction
            if (pixelS && pixelW && !pixelSW)
            {
              drawFontRegion(x  , y+1, x+1, y+2);
              drawFontRegion(x  , y+2, x+3, y+3);
            }
          }
        }
        //process next pixel to the right
        linePixels<<=1;
        linePixelsPrev<<=1;
        linePixelsNext<<=1;
      }
    }
  }
  free(TempBuf);

  if (!P2000_Mode)
  {
    al_set_target_bitmap(FontBuf_bk_scaled);
    al_draw_scaled_bitmap(FontBuf_bk, 0.0, 0.0, FONT_BITMAP_WIDTH, 
      CHAR_TILE_HEIGHT, 0.0, 0.0, FONT_BITMAP_WIDTH, 
      2*(CHAR_TILE_HEIGHT), 0);

    al_set_target_bitmap(FontBuf_scaled);
    al_draw_scaled_bitmap(FontBuf, 0.0, 0.0, FONT_BITMAP_WIDTH, 
      CHAR_TILE_HEIGHT, 0.0, 0.0, FONT_BITMAP_WIDTH, 
      2*(CHAR_TILE_HEIGHT), 0);
  }

  // al_set_target_bitmap((ALLEGRO_BITMAP *)display);
  al_set_window_title(display, Title);

  //al_save_bitmap("test.bmp", FontBuf);

  return 1;
}

/****************************************************************************/
/*** This function is called at every interrupt to update the P2000       ***/
/*** keyboard matrix and check for special events                         ***/
/****************************************************************************/
void Keyboard(void)
{

  int i, j, k;

  // al_rest(0.005);
  // al_rest(0.015);
  // al_rest(1.0/IFreq-(IFreq/10000));

  // keyboard_update ();
  al_get_keyboard_state(&kbdstate);

  for (i = 0; i < 80; i++)
  {
    k = i / 8;
    j = 1 << (i % 8);
    if (!keymask[i])
      continue;
    if (al_key_down(&kbdstate, keymask[i]))
      KeyMap[k] &= ~j;
    else
      KeyMap[k] |= j;
  }

  if (al_key_down(&kbdstate, ALLEGRO_KEY_ESCAPE) || al_key_down(&kbdstate, ALLEGRO_KEY_F10))
    Z80_Running = 0;

#ifdef DEBUG
  if (al_key_down(&kbdstate, ALLEGRO_KEY_F4))
    Z80_Trace = !Z80_Trace;
#endif

  if (al_key_down(&kbdstate, ALLEGRO_KEY_ENTER))
  {
  }

  if (al_key_down(&kbdstate, ALLEGRO_KEY_F6))
    calloptions = 1;

  if (al_key_down(&kbdstate, ALLEGRO_KEY_F7))
  {
    while (al_key_down(&kbdstate, ALLEGRO_KEY_F7))
      al_get_keyboard_state(&kbdstate);
    printf("  Writing screen shot to %s...", szBitmapFile);

    if (ScreenshotBuf)
      al_destroy_bitmap(ScreenshotBuf); // clean up previous screenshot
    ScreenshotBuf = al_clone_bitmap(al_get_target_bitmap());
    al_save_bitmap(szBitmapFile, ScreenshotBuf);
    printf("OK\n");
    NextOutputFile(szBitmapFile);
  }

  if (al_key_down(&kbdstate, ALLEGRO_KEY_F3) && !P2000_Mode)
  {
    while (al_key_down(&kbdstate, ALLEGRO_KEY_F3))
      al_get_keyboard_state(&kbdstate);
    WriteVRAMFile();
    NextOutputFile(szVideoRamFile);
  }

  if (al_key_down(&kbdstate, ALLEGRO_KEY_F9))
  {
    // PausePressed=2;
    while (al_key_down(&kbdstate, ALLEGRO_KEY_F9))
      al_get_keyboard_state(&kbdstate);
    // al_clear_to_color(al_map_rgb(0,0,0));
    // al_flip_display();
    while (!al_key_down(&kbdstate, ALLEGRO_KEY_F9))
      al_get_keyboard_state(&kbdstate);
    while (al_key_down(&kbdstate, ALLEGRO_KEY_F9))
      al_get_keyboard_state(&kbdstate);
  }

  if (al_key_down(&kbdstate, ALLEGRO_KEY_F5))
  {
    soundoff = (!soundoff);
    while (al_key_down(&kbdstate, ALLEGRO_KEY_F5))
      al_get_keyboard_state(&kbdstate);
  }

  if (al_key_down(&kbdstate, ALLEGRO_KEY_F11))
  {
    if (mastervolume)
      --mastervolume;
  }
  if (al_key_down(&kbdstate, ALLEGRO_KEY_F12))
  {
    if (mastervolume < 15)
      ++mastervolume;
  }

  FlushSound();
  al_rest(0.96 / IFreq);

  if ((!soundoff) && (sound_active))
  {
    while (al_get_next_event(queue, &event))
    {
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        Z80_Running = 0;
      if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT)
      {
        // playbuf=al_get_audio_stream_fragment(stream);
        al_set_audio_stream_fragment(stream, playbuf);
      }
    }
  }
  else
  {
    while (al_get_next_event(queue, &event))
    {
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
        Z80_Running = 0;
    }
    al_rest(0.04 / IFreq);
  }

  if (calloptions)
  {
    calloptions = 0;
    //al_flip_display();
    OptionsDialogue();
    //al_flip_display();
  }

  /*
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
 */
}

/****************************************************************************/
/*** This function is called every interrupt to flush sound pipes and     ***/
/*** sync emulation                                                       ***/
/****************************************************************************/
void FlushSound(void)
{
  int i;
  static int soundstate = 0;
  static int sample_count = 1;

#ifdef SOUND
  if ((soundoff) || (!sound_active))
    return;

  for (i = 0; i < buf_size; ++i)
  {
    if (soundbuf)
    {
      soundstate = soundbuf[i];
      soundbuf[i] = 0;
      sample_count = sample_rate / 1000;
    }

    if (playbuf)
      playbuf[i] = soundstate + 128;

    if (!--sample_count)
    {
      sample_count = sample_rate / 1000;
      if (soundstate > 0)
        --soundstate;
      if (soundstate < 0)
        ++soundstate;
    }
  }
#endif
}

/****************************************************************************/
/*** This function is called when the sound register is written to        ***/
/****************************************************************************/
void Sound(int toggle)
{
  static int last = -1;
  int pos, val;
  int p;

  if ((soundoff) || (!sound_active))
    return;

  if (toggle != last)
  {
    last = toggle;
    // pos=(buf_size-1)-(buf_size*Z80_ICount/Z80_IPeriod);
    pos = (buf_size - 1) - (buf_size * Z80_ICount / Z80_IPeriod);
    val = (toggle) ? (-mastervolume * 8) : (mastervolume * 8);

    for (p = 0; p < (sample_rate / 2600); p++)
    {
      // if ((pos-p)>0)
      //	soundbuf[pos-p]=val;
      if ((pos + p) < buf_size)
        soundbuf[pos + p] = val;
    }
  }
}

/****************************************************************************/
/*** Pause specified ammount of time                                      ***/
/****************************************************************************/
void Pause(int ms)
{
  /*
  int i,j;
  j=ReadTimer();
  i=j+ms*1000;
  while ((j-i)<0) j=ReadTimer();
  OldTimer=j;
  */
  al_rest((float)ms / 1000.0);
}

/****************************************************************************/
/*** Put a character in the display buffer for P2000M emulation mode      ***/
/****************************************************************************/
static inline void PutChar_M(int x, int y, int c, int eor, int ul)
{
  int K;
  K = c + (eor << 8) + (ul << 16);
  if (K == OldCharacter[y * 80 + x])
    return;
  OldCharacter[y * 80 + x] = K;

  // printf("-M-");

  al_draw_bitmap_region(
      (eor ? FontBuf_bk : FontBuf), 0.5 * c * CHAR_TILE_WIDTH, 
      0.0, 0.5 * CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 0.5 * x * CHAR_TILE_WIDTH,
      y * CHAR_TILE_HEIGHT, 0);
  if (ul)
    al_draw_filled_rectangle(
        0.5 * x * CHAR_TILE_WIDTH, (y + 1) * CHAR_TILE_HEIGHT - 2.0, 
        0.5 * (x + 1) * CHAR_TILE_WIDTH, (y + 1) * CHAR_TILE_HEIGHT - 1.0, 
        al_map_rgb(255, 255, 255));
}

/****************************************************************************/
/*** Put a character in the display buffer for P2000T emulation mode      ***/
/****************************************************************************/
static inline void PutChar_T(int x, int y, int c, int fg, int bg, int si)
{
  int K;
  K = c + (fg << 8) + (bg << 16) + (si << 24);
  if (K == OldCharacter[y * 40 + x])
    return;
  OldCharacter[y * 40 + x] = K;

  // printf("-T-");
  //  printf("%c",c);

  al_lock_bitmap(al_get_backbuffer(display), ALLEGRO_PIXEL_FORMAT_ANY, 0);

  al_draw_tinted_scaled_bitmap(
      (si ? FontBuf_scaled : FontBuf),
      al_map_rgba(Pal[fg * 3], Pal[fg * 3 + 1], Pal[fg * 3 + 2], 255), 
      c * CHAR_TILE_WIDTH, (si >> 1) * CHAR_TILE_HEIGHT, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT,
      x * CHAR_TILE_WIDTH, y * CHAR_TILE_HEIGHT, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 0);
  if (bg)
    al_draw_tinted_scaled_bitmap(
        (si ? FontBuf_bk_scaled : FontBuf_bk),
        al_map_rgba(Pal[bg * 3], Pal[bg * 3 + 1], Pal[bg * 3 + 2], 0), 
        c * CHAR_TILE_WIDTH, (si >> 1) * CHAR_TILE_HEIGHT, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 
        x * CHAR_TILE_WIDTH, y * CHAR_TILE_HEIGHT, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 0);
}

/****************************************************************************/
/*** Common.h contains the system-independent part of the screen refresh  ***/
/*** drivers                                                              ***/
/****************************************************************************/
#include "Common.h"
