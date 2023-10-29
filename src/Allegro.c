/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                               allegro.c                              ***/
/***                                                                      ***/
/*** This file contains the Allegro 5 drivers.                            ***/
/***                                                                      ***/
/*** Copyright (C) Stefano Bodrato 2013                                   ***/
/*** Copyright (C) Dion Olsthoorn  2023                                   ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#define DISPLAY_WIDTH 960
#define DISPLAY_HEIGHT 720
#define DISPLAY_BORDER 20
#define CHAR_TILE_WIDTH 24
#define CHAR_TILE_HEIGHT 30
#define CHAR_PIXEL_WIDTH 4
#define CHAR_PIXEL_HEIGHT 3
#define FONT_BITMAP_WIDTH (96+64+64)*CHAR_TILE_WIDTH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#ifdef WIN32
#include <windows.h>
#endif
#include "P2000.h"
#include "Unix.h"
#include "Utils.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_native_dialog.h> 

ALLEGRO_AUDIO_STREAM *stream = NULL;
ALLEGRO_MIXER *mixer = NULL;
ALLEGRO_FILECHOOSER *cassetteChooser = NULL;
ALLEGRO_FILECHOOSER *cartridgeChooser = NULL;

int buf_size;
int sample_rate;
signed char *soundbuf;      /* Pointer to sound buffer               */
int mastervolume=4;               /* Master volume setting                 */
static int sound_active=1;

ALLEGRO_EVENT event;
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_EVENT_QUEUE *eventQueue = NULL; // generic queue for keyboard and windows events
ALLEGRO_KEYBOARD_STATE kbdstate;
char *Title="M2000 v0.7-SNAPSHOT"; /* Title for Window  */

int videomode;                    /* T emulation only: 
                                        0=960x720
                                        1=960x720 (pixelated font)         */ 
int keyboardmap = 1;              /* 1 = symbolic keyboard mapping         */
static int *OldCharacter;         /* Holds characters on the screen        */

ALLEGRO_BITMAP *FontBuf = NULL;
ALLEGRO_BITMAP *FontBuf_bk = NULL;
ALLEGRO_BITMAP *FontBuf_scaled = NULL;
ALLEGRO_BITMAP *FontBuf_bk_scaled = NULL;
ALLEGRO_BITMAP *ScreenshotBuf = NULL;

#ifdef JOYSTICK
static byte joyKeyMapping[2][5] = 
{
  { 23, 21,  0,  2, 17 }, /* right, down, left, up, fire-button */
  {  2, -1,  0, -1, 17 }  /* Fraxxon mode, using keys left/up for moving */ 
};
int joymode=1;                     /* If 0, do not use joystick             */
int joymap=0;                      /* 0 = default joystick-key mapping      */
ALLEGRO_JOYSTICK *joystick = NULL;
ALLEGRO_JOYSTICK_STATE joyState;
bool lastJoyState[5];
#endif

ALLEGRO_EVENT_QUEUE *timerQueue = NULL;
ALLEGRO_TIMER *timer;

int soundmode=255;                 /* Sound mode, 255=auto-detect           */
static int soundoff=0;             /* If 1, sound is turned off             */

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

/*
    P2000 Keyboard layout

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

    Keys marked with an asterix (*) are on the numeric keypad
*/
static unsigned char keymask[]=
{
  ALLEGRO_KEY_LEFT,       ALLEGRO_KEY_6,         ALLEGRO_KEY_UP,          ALLEGRO_KEY_Q,          ALLEGRO_KEY_3,          ALLEGRO_KEY_5,         ALLEGRO_KEY_7,      ALLEGRO_KEY_4,
  ALLEGRO_KEY_TAB,        ALLEGRO_KEY_H,         ALLEGRO_KEY_Z,           ALLEGRO_KEY_S,          ALLEGRO_KEY_D,          ALLEGRO_KEY_G,         ALLEGRO_KEY_J,      ALLEGRO_KEY_F,
  ALLEGRO_KEY_PAD_ENTER,  ALLEGRO_KEY_SPACE,     ALLEGRO_KEY_PAD_DELETE,  ALLEGRO_KEY_PAD_0,      ALLEGRO_KEY_SLASH,      ALLEGRO_KEY_DOWN,      ALLEGRO_KEY_COMMA,  ALLEGRO_KEY_RIGHT,
  ALLEGRO_KEY_CAPSLOCK,   ALLEGRO_KEY_N,         ALLEGRO_KEY_DELETE,      ALLEGRO_KEY_X,          ALLEGRO_KEY_C,          ALLEGRO_KEY_B,         ALLEGRO_KEY_M,      ALLEGRO_KEY_V,
  ALLEGRO_KEY_BACKQUOTE,  ALLEGRO_KEY_Y,         ALLEGRO_KEY_A,           ALLEGRO_KEY_W,          ALLEGRO_KEY_E,          ALLEGRO_KEY_T,         ALLEGRO_KEY_U,      ALLEGRO_KEY_R,
  ALLEGRO_KEY_BACKSLASH,  ALLEGRO_KEY_9,         ALLEGRO_KEY_PAD_PLUS,    ALLEGRO_KEY_OPENBRACE,  ALLEGRO_KEY_BACKSPACE,  ALLEGRO_KEY_0,         ALLEGRO_KEY_1,      ALLEGRO_KEY_PAD_MINUS,
  ALLEGRO_KEY_PAD_9,      ALLEGRO_KEY_O,         ALLEGRO_KEY_PAD_8,       ALLEGRO_KEY_PAD_7,      ALLEGRO_KEY_ENTER,      ALLEGRO_KEY_P,         ALLEGRO_KEY_8,      ALLEGRO_KEY_SEMICOLON,
  ALLEGRO_KEY_PAD_3,      ALLEGRO_KEY_FULLSTOP,  ALLEGRO_KEY_PAD_2,       ALLEGRO_KEY_PAD_1,      ALLEGRO_KEY_RCTRL,      ALLEGRO_KEY_MINUS,     ALLEGRO_KEY_K,      ALLEGRO_KEY_2,
  ALLEGRO_KEY_PAD_6,      ALLEGRO_KEY_L,         ALLEGRO_KEY_PAD_5,       ALLEGRO_KEY_PAD_4,      ALLEGRO_KEY_CLOSEBRACE, ALLEGRO_KEY_TILDE,     ALLEGRO_KEY_I,      ALLEGRO_KEY_QUOTE,
  ALLEGRO_KEY_LSHIFT,     0,                     0,                       0,                      0,                      0,                     0,                  ALLEGRO_KEY_RSHIFT
};

#define NUMBER_OF_KEYMAPPINGS 70
static byte keyMappings[NUMBER_OF_KEYMAPPINGS][5] =
{
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_F1,         59,      1,       59,      1 }, // ZOEK    [free]
  { ALLEGRO_KEY_F2,         56,      1,       16,      1 }, // START   STOP
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_A,          34,      0,       34,      1 }, // A       a
  { ALLEGRO_KEY_B,          29,      0,       29,      1 }, // B       b
  { ALLEGRO_KEY_C,          28,      0,       28,      1 }, // C       c
  { ALLEGRO_KEY_D,          12,      0,       12,      1 }, // D       d
  { ALLEGRO_KEY_E,          36,      0,       36,      1 }, // E       e
  { ALLEGRO_KEY_F,          15,      0,       15,      1 }, // F       f
  { ALLEGRO_KEY_G,          13,      0,       13,      1 }, // G       g
  { ALLEGRO_KEY_H,           9,      0,        9,      1 }, // H       h
  { ALLEGRO_KEY_I,          70,      0,       70,      1 }, // I       i
  { ALLEGRO_KEY_J,          14,      0,       14,      1 }, // J       j
  { ALLEGRO_KEY_K,          62,      0,       62,      1 }, // K       k
  { ALLEGRO_KEY_L,          65,      0,       65,      1 }, // L       l
  { ALLEGRO_KEY_M,          30,      0,       30,      1 }, // M       m
  { ALLEGRO_KEY_N,          25,      0,       25,      1 }, // N       n
  { ALLEGRO_KEY_O,          49,      0,       49,      1 }, // O       o
  { ALLEGRO_KEY_P,          53,      0,       53,      1 }, // P       p
  { ALLEGRO_KEY_Q,           3,      0,        3,      1 }, // Q       q
  { ALLEGRO_KEY_R,          39,      0,       39,      1 }, // R       r
  { ALLEGRO_KEY_S,          11,      0,       11,      1 }, // S       s
  { ALLEGRO_KEY_T,          37,      0,       37,      1 }, // T       t
  { ALLEGRO_KEY_U,          38,      0,       38,      1 }, // U       u
  { ALLEGRO_KEY_V,          31,      0,       31,      1 }, // V       v
  { ALLEGRO_KEY_W,          35,      0,       35,      1 }, // W       w
  { ALLEGRO_KEY_X,          27,      0,       27,      1 }, // X       x
  { ALLEGRO_KEY_Y,          33,      0,       33,      1 }, // Y       y
  { ALLEGRO_KEY_Z,          10,      0,       10,      1 }, // Z       z
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_1,          46,      0,       46,      1 }, // 1       !
  { ALLEGRO_KEY_2,          63,      0,       55,      0 }, // 2       @
  { ALLEGRO_KEY_3,           4,      0,       20,      0 }, // 3       #
  { ALLEGRO_KEY_4,           7,      0,        7,      1 }, // 4       $
  { ALLEGRO_KEY_5,           5,      0,        5,      1 }, // 5       %
  { ALLEGRO_KEY_6,           1,      0,       55,      1 }, // 6       ↑
  { ALLEGRO_KEY_7,           6,      0,        1,      1 }, // 7       &
  { ALLEGRO_KEY_8,          54,      0,       71,      1 }, // 8       *  
  { ALLEGRO_KEY_9,          41,      0,       54,      1 }, // 9       (
  { ALLEGRO_KEY_0,          45,      0,       41,      1 }, // 0       )
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_EQUALS,     45,      1,       42,      0 }, // =       +
  { ALLEGRO_KEY_MINUS,      47,      0,       47,      1 }, // -       _
  { ALLEGRO_KEY_OPENBRACE,  60,      1,       68,      0 }, // ←       ¼
  { ALLEGRO_KEY_CLOSEBRACE, 60,      0,       68,      1 }, // →       ¾
  { ALLEGRO_KEY_SEMICOLON,  69,      0,       71,      0 }, // ;       :
  { ALLEGRO_KEY_QUOTE,       6,      1,       63,      1 }, // '       "
  { ALLEGRO_KEY_LEFT,        0,      0,        0,      1 }, // LEFT    LEFT LINE
  { ALLEGRO_KEY_RIGHT,      23,      0,       23,      1 }, // RIGHT   [free]
  { ALLEGRO_KEY_UP,          2,      0,        2,      1 }, // UP      LEFTUP
  { ALLEGRO_KEY_DOWN,       21,      0,       21,      1 }, // DOWN    RIGHTDOWN
  { ALLEGRO_KEY_TAB,         8,      0,        8,      1 }, // TAB     [free]
  { ALLEGRO_KEY_COMMA,      22,      0,       26,      0 }, // ,       <
  { ALLEGRO_KEY_FULLSTOP,   57,      0,       26,      1 }, // .       >
  { ALLEGRO_KEY_SPACE,      17,      0,       17,      1 }, // SPACE   [free]
  { ALLEGRO_KEY_BACKSPACE,  44,      0,       40,      0 }, // BACKSP  CLRLN
  { ALLEGRO_KEY_DELETE,     44,      0,       40,      1 }, // BACKSP  CLRSCR
  { ALLEGRO_KEY_SLASH,      61,      0,       61,      1 }, // /       ?
  { ALLEGRO_KEY_ENTER,      52,      0,       52,      1 }, // ENTER   [free]
  { ALLEGRO_KEY_BACKSLASH,  20,      1,       20,      1 }, // █       [free]
  { ALLEGRO_KEY_BACKQUOTE,  32,      0,       32,      1 }, // CODE    [free]
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_PAD_9,      48,      0,       48,      1 }, // 9       ?
  { ALLEGRO_KEY_PAD_8,      50,      0,       50,      1 }, // 8       ?
  { ALLEGRO_KEY_PAD_7,      51,      0,       51,      1 }, // 7       CASS WIS
  { ALLEGRO_KEY_PAD_6,      64,      0,       64,      1 }, // 6       ?
  { ALLEGRO_KEY_PAD_5,      66,      0,       66,      1 }, // 5       CLR+LIST
  { ALLEGRO_KEY_PAD_4,      67,      0,       67,      1 }, // 4       ?
  { ALLEGRO_KEY_PAD_3,      56,      0,       56,      1 }, // 3       START
  { ALLEGRO_KEY_PAD_2,      58,      0,       58,      1 }, // 2       ?
  { ALLEGRO_KEY_PAD_1,      59,      0,       59,      1 }, // 1       ZOEK
  { ALLEGRO_KEY_PAD_0,      19,      0,       19,      1 }, // 0       ?
  { ALLEGRO_KEY_PAD_DELETE, 18,      0,       18,      1 }, // 00      ?
  { ALLEGRO_KEY_PAD_ENTER,  16,      0,       16,      1 }, // .       STOP
};

/****************************************************************************/
/*** Deallocate resources taken by InitMachine()                          ***/
/****************************************************************************/
void TrashMachine(void)
{
  if (Verbose) printf("\n\nShutting down...\n");
  al_destroy_display(display);
  al_destroy_event_queue(eventQueue);
  al_uninstall_joystick();
  al_uninstall_keyboard();
  al_shutdown_primitives_addon();
  al_shutdown_image_addon();
  al_destroy_native_file_dialog(cassetteChooser);
  al_destroy_native_file_dialog(cartridgeChooser);
  al_shutdown_native_dialog_addon();

  al_destroy_timer(timer);
  al_destroy_event_queue(timerQueue);

  //al_drain_audio_stream(stream);
  al_destroy_audio_stream(stream);
  al_destroy_mixer(mixer);
  al_uninstall_audio();
  free (soundbuf);

  if (FontBuf) al_destroy_bitmap(FontBuf);
  if (FontBuf_bk) al_destroy_bitmap(FontBuf_bk);
  if (FontBuf_scaled) al_destroy_bitmap(FontBuf_scaled);
  if (FontBuf_bk_scaled) al_destroy_bitmap(FontBuf_bk_scaled);
  if (ScreenshotBuf) al_destroy_bitmap(ScreenshotBuf);
  if (OldCharacter) free (OldCharacter);
  
  al_uninstall_system();
}

int InitAllegro()
{
  if (!al_is_system_installed())
  {
    // init allegro
    if (Verbose) printf("Initialising Allegro drivers and addons... ");
    if (!al_init() || !al_init_primitives_addon() || !al_init_image_addon())
    {
      if (Verbose) puts("FAILED");
      return 0;
    }
    if (Verbose) puts("OK");
  }
  return 1;
}

/****************************************************************************/
/*** Initialise all resources needed by the Linux/SVGALib implementation  ***/
/****************************************************************************/
int InitMachine(void)
{
  int i;

  if (!InitAllegro()) return 0;
  al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);

  al_init_native_dialog_addon();
  cassetteChooser = al_create_native_file_dialog(NULL, 
    "Select a .cas file", "*.cas", 0); //file doesn't have to exist
  cartridgeChooser = al_create_native_file_dialog(NULL, 
    "Select a .bin file", "*.bin", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);

  if (joymode) 
  {
    if (Verbose) printf("Initialising and detecting joystick... ");
    if (al_install_joystick() && (joystick = al_get_joystick(0)) != NULL && Verbose)
      puts("OK");
    else if (Verbose) puts("FAILED");
  }

  if (Verbose)
    printf("Initialising keyboard...");

  if (!al_install_keyboard())
  {
    if (Verbose)
      printf("FAILED\n");
    return 0;
  }

  printf("OK\nCreating the output window... ");
  display = al_create_display(DISPLAY_WIDTH + 2*DISPLAY_BORDER, DISPLAY_HEIGHT + 2*DISPLAY_BORDER);
  eventQueue = al_create_event_queue();
  timerQueue =  al_create_event_queue();
  timer = al_create_timer(1.0 / IFreq);
  if (Verbose)
  {
    if (!display || !eventQueue || !timerQueue)
    {
      printf("FAILED\n");
      return 0;
    }
    else
      printf("OK\n");
  }
  al_set_window_title(display, Title);
  al_clear_to_color(al_map_rgb(0, 0, 0));
  al_register_event_source(eventQueue, al_get_display_event_source(display));
  //al_register_event_source(eventQueue, al_get_keyboard_event_source());
  al_register_event_source(timerQueue, al_get_timer_event_source(timer));

  if (P2000_Mode) /* black and white palette */
  {
    Pal[0] = Pal[1] = Pal[2] = 0;
    Pal[3] = Pal[4] = Pal[5] = 255;
  }

  if (Verbose) printf("  Allocating cache buffers... ");
  i = P2000_Mode ? 80 * 24 : 40 * 24;
  OldCharacter = malloc(i * sizeof(int));
  if (!OldCharacter)
  {
    if (Verbose) puts("FAILED");
    return (0);
  }
  memset(OldCharacter, -1, i * sizeof(int));
  if (Verbose) puts("OK");
  
  InitScreenshotFile();
  InitVRAMFile();

  /* sound init */
  if (Verbose) printf("Initializing sound...");

  if (!al_install_audio())
  {
    if (Verbose) printf("FAILED\n");
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

  for (i = 4096; i >= 128; i /= 2) if (i * IFreq <= 44100) break;
  sample_rate = i * IFreq;
  if (Verbose)
    printf("%d Hz...", sample_rate);
  /* The actual sampling rate might be different from the optimal one.
     Here we calculate the optimal buffer size */
  buf_size = sample_rate / IFreq;
  for (i = 1; (1 << i) <= buf_size; ++i);
  if (((1 << i) - buf_size) > (buf_size - (1 << (i - 1)))) --i;
  buf_size = 1 << i;
  soundbuf = malloc(buf_size);

  stream = al_create_audio_stream(16, buf_size, sample_rate, ALLEGRO_AUDIO_DEPTH_UINT8, ALLEGRO_CHANNEL_CONF_1);

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

  if (Verbose) printf("  Connecting to the default mixer...");
  mixer = al_get_default_mixer();
  if (!al_attach_audio_stream_to_mixer(stream, mixer))
  {
    if (Verbose) printf("FAILED\n");
    sound_active = 0;
  }
  else if (Verbose) printf("OK\n");

#ifdef WIN32
  // hide console window on startup
  while (IsWindowVisible(GetConsoleWindow()) != FALSE)
    ShowWindow(GetConsoleWindow(), SW_HIDE); //SW_RESTORE to bring back
#endif

  return 1;
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

  if (!soundoff && sound_active)
  {
    int8_t *playbuf = al_get_audio_stream_fragment(stream);
    if (playbuf)
    {
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
      al_set_audio_stream_fragment(stream, playbuf);
    }
  }

  // sync emulation by waiting for timer event (fired 50 times a second)
  if (Sync) 
  {
    if (al_get_next_event(timerQueue, &event))
    {
#ifdef DEBUG
      if (Verbose) 
        printf("  Sync too slow [%f]...\n", event.timer.timestamp);
#endif
      while (al_get_next_event(timerQueue, &event)); //drain the timer queue
    }
    else
      al_wait_for_event(timerQueue, &event);   
  }
}

/****************************************************************************/
/*** This function is called when the sound register is written to        ***/
/****************************************************************************/
void Sound(int toggle)
{
  static int last=-1;
  int pos,val;

  if (soundoff || !sound_active) 
    return;

  if (toggle!=last)
  {
    last=toggle;
    pos=(buf_size-1)-(buf_size*Z80_ICount/Z80_IPeriod);
    val=(toggle)? (-mastervolume*8):(mastervolume*8);
    soundbuf[pos]=val;
  }
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
  int linePixelsPrevPrev, linePixelsNextNext;
  int pixelN, pixelE, pixelS, pixelW;
  int pixelSW, pixelSE, pixelNW, pixelNE;
  int pixelSSW, pixelSSE, pixelWNW, pixelENE, pixelESE, pixelWSW, pixelNNW, pixelNNE;
  char *TempBuf;
  FILE *F;

  if (!InitAllegro())
    return 0;

  if (Verbose)
    printf("Loading font %s...\n", filename);
  if (Verbose)
    printf("  Allocating memory... ");

  if (!FontBuf)
  {
    FontBuf = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT);
    if (!FontBuf)
    {
      if (Verbose) puts("FAILED");
      return 0;
    }
  }

  if (!FontBuf_bk)
  {
    FontBuf_bk = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT);
    if (!FontBuf_bk)
    {
      if (Verbose) puts("FAILED");
      return 0;
    }
  }

  if (!P2000_Mode)
  {
    if (!FontBuf_scaled) //double height
    {
      FontBuf_scaled = al_create_bitmap(FONT_BITMAP_WIDTH, 2*CHAR_TILE_HEIGHT);
      if (!FontBuf_scaled)
      {
        if (Verbose) puts("FAILED");
        return 0;
      }
    }

    if (!FontBuf_bk_scaled)
    {
      FontBuf_bk_scaled = al_create_bitmap(FONT_BITMAP_WIDTH, 2*CHAR_TILE_HEIGHT);
      if (!FontBuf_bk_scaled)
      {
        if (Verbose) puts("FAILED");
        return 0;
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
    if (Verbose) puts("FAILED");
    return 0;
  }
  if (Verbose) puts("OK");
  if (Verbose) printf("  Opening... ");
  i = 0;
  F = fopen(filename, "rb");
  if (F)
  {
    printf("Reading... ");
    if (fread(TempBuf, 2240, 1, F)) i = 1;
    fclose(F);
  }
  if (Verbose) puts((i) ? "OK" : "FAILED");
  if (!i) return 0;

  /* Stretch 6x10 characters to 12x20, so we can do character rounding */
  for (i = 0; i < (96 + 64 + 64) * 10; i += 10) //96 alpha + 64 graphic (cont) + 64 graphic (sep)
  {
    linePixelsPrevPrev = 0;
    linePixelsPrev = 0;
    linePixels = 0;
    linePixelsNext = TempBuf[i] << 6;
    linePixelsNextNext = TempBuf[i+1] << 6;
    for (line = 0; line < 10; ++line)
    {
      y = line * CHAR_PIXEL_HEIGHT;
      linePixelsPrevPrev = linePixelsPrev >> 6;
      linePixelsPrev = linePixels >> 6;
      linePixels = linePixelsNext >> 6;
      linePixelsNext = linePixelsNextNext >> 6;
      linePixelsNextNext = line < 8 ? TempBuf[i + line + 2] : 0;

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
            // for character rounding, look at 16 pixel around current pixel
            // using 16-wind compass notation
            pixelN = linePixelsPrev & 0x20;
            pixelE = linePixels & 0x10;
            pixelS = linePixelsNext & 0x20;
            pixelW = linePixels & 0x40;
            pixelNE = linePixelsPrev & 0x10;
            pixelSE = linePixelsNext & 0x10;
            pixelSW = linePixelsNext & 0x40;
            pixelNW = linePixelsPrev & 0x40;
            pixelSSW = linePixelsNextNext & 0x40;
            pixelSSE = linePixelsNextNext & 0x10;
            pixelWNW = linePixelsPrev & 0x80;
            pixelENE = linePixelsPrev & 0x08;
            pixelESE = linePixelsNext & 0x08;
            pixelWSW = linePixelsNext & 0x80;
            pixelNNW = linePixelsPrevPrev & 0x40;
            pixelNNE = linePixelsPrevPrev & 0x10;

            // the extra rounding pixels are in the shape of a (rotated) L
            // rounding in NW direction
            if (pixelN && pixelW && (!pixelNW || (!pixelNE && pixelNNE) || (!pixelSW && pixelWSW)))
            {
              // alternative rounding for a steep vertical lines (e.g. "V")
              if (!pixelE && !pixelNE && !pixelNNE && !pixelS && pixelSW)
              {
                // ⬜⬜⬜⬛
                // ⬜⬜⬛⬛
                // ⬜⬜⬛⬛
                // ⬜⬛⬛⬛ (next line)
                drawFontRegion(x,   y,   x+1, y+4);
                drawFontRegion(x+1, y,   x+2, y+3);
                drawFontRegion(x+2, y,   x+3, y+1);
              }
              else
              {
                // ⬜⬜⬜⬛
                // ⬜⬛⬛⬛
                // ⬛⬛⬛⬛
                drawFontRegion(x,   y,   x+3, y+1);
                drawFontRegion(x,   y+1, x+1, y+2);
              }
            }
            // rounding in NE direction
            if (pixelN && pixelE && (!pixelNE || (!pixelNW && pixelNNW) || (!pixelSE && pixelESE)))
            {
              // alternative rounding for a steep vertical lines (e.g. "V")
              if (!pixelW &&!pixelNW && !pixelNNW && !pixelS && pixelSE)
              {
                // ⬛⬜⬜⬜
                // ⬛⬛⬜⬜
                // ⬛⬛⬜⬜
                // ⬛⬛⬛⬜ (next line)
                drawFontRegion(x+3, y,   x+4, y+4);
                drawFontRegion(x+2, y  , x+3, y+3);
                drawFontRegion(x+1, y  , x+2, y+1);
              }
              else
              {
                // ⬛⬜⬜⬜
                // ⬛⬛⬛⬜
                // ⬛⬛⬛⬛
                drawFontRegion(x+1, y,   x+4, y+1);
                drawFontRegion(x+3, y+1, x+4, y+2);
              }
            }
            // rounding in SE direction
            if (pixelS && pixelE && (!pixelSE || (!pixelSW && pixelSSW) || (!pixelNE && pixelENE)))
            {
              // ⬛⬛⬛⬛
              // ⬛⬛⬛⬜
              // ⬛⬜⬜⬜
              drawFontRegion(x+3, y+1, x+4, y+2);
              drawFontRegion(x+1, y+2, x+4, y+3);
            }
            // rounding in SW direction
            if (pixelS && pixelW && (!pixelSW || (!pixelSE && pixelSSE) || (!pixelNW && pixelWNW)))
            {
              // ⬛⬛⬛⬛
              // ⬜⬛⬛⬛
              // ⬜⬜⬜⬛
              drawFontRegion(x  , y+1, x+1, y+2);
              drawFontRegion(x  , y+2, x+3, y+3);
            }
          }
        }
        //process next pixel to the right
        linePixelsPrevPrev<<=1;
        linePixelsPrev<<=1;
        linePixels<<=1;
        linePixelsNext<<=1;
        linePixelsNextNext<<=1;
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
  return 1;
}

bool al_key_up(ALLEGRO_KEYBOARD_STATE * kb_state, int kb_event) 
{
  if (!al_key_down(kb_state, kb_event)) return false;
  while (al_key_down(kb_state, kb_event)) al_get_keyboard_state(kb_state);
  return true;
}

void PushKey(byte keyCode)
{
  byte mRow = keyCode / 8;
  byte mCol = 1 << (keyCode % 8);
  if (mRow < 10) KeyMap[mRow] &= ~mCol;
}

void ReleaseKey(byte keyCode)
{
  byte mRow = keyCode / 8;
  byte mCol = 1 << (keyCode % 8);
  if (mRow < 10) KeyMap[mRow] |= mCol;
}

/****************************************************************************/
/*** This function is called at every interrupt to update the P2000       ***/
/*** keyboard matrix and check for special events                         ***/
/****************************************************************************/
void Keyboard(void)
{
  /* first, make sure the 50Hz timer is started */
  if (!al_get_timer_started(timer))
    al_start_timer(timer);

  int i,j,k;
  byte keyPressed;
  bool isCombiKey, isNormalKey, isShiftKey;
  bool isSpecialKeyPressed = 0;
  byte keyCode, keyCodeCombi;
  bool al_shift_down;
  bool isP2000ShiftDown;

  static byte queuedKeys[NUMBER_OF_KEYMAPPINGS] = {0};
  static byte activeKeys[NUMBER_OF_KEYMAPPINGS] = {0};

  //read keyboard state
  al_get_keyboard_state(&kbdstate);
  al_shift_down = al_key_down(&kbdstate,ALLEGRO_KEY_LSHIFT) || al_key_down(&kbdstate,ALLEGRO_KEY_RSHIFT);

  if (keyboardmap == 0)
  {
    /* Positional Key Mapping */
    //fill P2000 KeyMap
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
  }
  else
  {
    /* Symbolic Key Mapping */
    isP2000ShiftDown = (~KeyMap[9] & 0xff) ? 1 : 0; // 1 when one of the shift keys is pressed

    for (i = 0; i < NUMBER_OF_KEYMAPPINGS; i++)
    {
      keyPressed = keyMappings[i][0];
      isCombiKey = keyMappings[i][1] != keyMappings[i][3];
      isNormalKey = !isCombiKey && (keyMappings[i][2] == 0) && (keyMappings[i][4] == 1);
      isShiftKey = keyMappings[i][al_shift_down ? 4 : 2];
      keyCode = keyMappings[i][al_shift_down ? 3 : 1];
      keyCodeCombi = isCombiKey ? keyMappings[i][al_shift_down ? 1 : 3] : -1;

      if (queuedKeys[i] || al_key_down(&kbdstate, keyPressed))
      {
        if (isCombiKey) 
          ReleaseKey(keyCodeCombi);
        if (isNormalKey || (isShiftKey == isP2000ShiftDown)) 
        {
          queuedKeys[i] = 0;
          PushKey(keyCode);
        }
        else 
        {
          // first, the shift must be pressed/un-pressed in this interrupt
          // then in the next interrupt the target key itself will be pressed
          KeyMap[9] = isShiftKey ? 0xfe : 0xff; // 0xfe = LSHIFT
          queuedKeys[i] = 1;
        }
        activeKeys[i] = 1;
        if (!isNormalKey) 
          isSpecialKeyPressed = true;
      }
      else
      {
        if (activeKeys[i]) 
        {
          // unpress key and second key in P2000's keyboard matrix
          if (isCombiKey) ReleaseKey(keyCodeCombi);
          ReleaseKey(keyCode);
          activeKeys[i] = 0;
        }
      }
    }
    if (!isSpecialKeyPressed) {
      if (al_key_down(&kbdstate,ALLEGRO_KEY_LSHIFT)) KeyMap[9] &= ~0b00000001; else KeyMap[9] |= 0b00000001;
      if (al_key_down(&kbdstate,ALLEGRO_KEY_RSHIFT)) KeyMap[9] &= ~0b10000000; else KeyMap[9] |= 0b10000000;
      if (al_key_down(&kbdstate,ALLEGRO_KEY_CAPSLOCK)) KeyMap[3] &= ~0b00000001; else KeyMap[3] |= 0b00000001;
    }
  }

  /* press Escape to quit M2000 */
  if (al_key_down(&kbdstate, ALLEGRO_KEY_ESCAPE))
    Z80_Running = 0;

  /* F3 to load a .cas cassette file */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F3))
  {
    if (al_shift_down) 
      RemoveCassette();
    else if (al_show_native_file_dialog(display, cassetteChooser)) {
      InsertCassette(al_get_native_file_dialog_path(cassetteChooser, 0));
    }
  }

  /* F4 to load a .bin cartridge file */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F4))
  {
    if (al_shift_down) 
      RemoveCartridge();
    else if (al_show_native_file_dialog(display, cartridgeChooser))
      InsertCartridge(al_get_native_file_dialog_path(cartridgeChooser, 0));
  }

  if (al_key_up(&kbdstate, ALLEGRO_KEY_F5))
#ifdef DEBUG
    Z80_Trace = !Z80_Trace;
#else
    Z80_Reset ();
#endif

  if (al_key_up(&kbdstate, ALLEGRO_KEY_F6))
  {
    #ifdef WIN32
    ShowWindow(GetConsoleWindow(), SW_RESTORE);
    #endif
    OptionsDialogue();
    #ifdef WIN32
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    #endif
  }

  /* F7 = screenshot */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F7))
  {
    if (Verbose) 
      printf("  Writing screen shot to %s...", szBitmapFile);
    if (ScreenshotBuf)
      al_destroy_bitmap(ScreenshotBuf); // clean up previous screenshot
    ScreenshotBuf = al_clone_bitmap(al_get_target_bitmap());
    al_save_bitmap(szBitmapFile, ScreenshotBuf);
    if (Verbose)
      printf("OK\n");
    NextOutputFile(szBitmapFile);
  }

  /* F8 = save video RAM to file */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F8))
  {
    WriteVRAMFile();
    NextOutputFile(szVideoRamFile);
  }

  /* F9 = pause / unpause */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F9))
  {
    if (Verbose)
      printf("  Paused...\n");
    //wait for unpause
    while (!al_key_up(&kbdstate, ALLEGRO_KEY_F9))
      al_get_keyboard_state(&kbdstate);
    if (Verbose)
      printf("  ...Unpaused\n");
  }

  /* F10, F11 and F12 for sound optioons */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F10))
  {
    soundoff = (!soundoff);
  }
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F11))
  {
    if (mastervolume)
      --mastervolume;
  }
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F12))
  {
    if (mastervolume < 15)
      ++mastervolume;
  }

#ifdef JOYSTICK
  // handle joystick
  if (joystick != NULL) 
  {
    al_get_joystick_state(joystick, &joyState);
    for (i = 0; i < 5; i++) // 4 directions and 1 button
    {
      if ((i < 4 && joyState.stick[0].axis[i%2] == -2*(i/2)+1) ||
        (i == 4 && joyState.button[0]))
      {
        PushKey(joyKeyMapping[joymap][i]);
        lastJoyState[i] = 1;
      }
      else
      {
        if (lastJoyState[i]) ReleaseKey(joyKeyMapping[joymap][i]);
        lastJoyState[i] = 0;
      }
    }
  }
#endif

  //check if Window was closed
  while (al_get_next_event(eventQueue, &event))
  { 
    if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) Z80_Running = 0;
  }
}

/****************************************************************************/
/*** Pause specified ammount of time                                      ***/
/****************************************************************************/
void Pause(int ms)
{
  al_rest((double)ms / 1000.0);
}

/****************************************************************************/
/*** This function is called by the screen refresh drivers to copy the    ***/
/*** off-screen buffer to the actual display                              ***/
/****************************************************************************/
static void PutImage (void)
{
  al_flip_display();
}

/****************************************************************************/
/*** Put a character in the display buffer for P2000M emulation mode      ***/
/****************************************************************************/
static inline void PutChar_M(int x, int y, int c, int eor, int ul)
{
  int K = c + (eor << 8) + (ul << 16);
  if (K == OldCharacter[y * 80 + x])
    return;
  OldCharacter[y * 80 + x] = K;

  al_set_target_bitmap(al_get_backbuffer(display));
  al_draw_bitmap_region(
      (eor ? FontBuf_bk : FontBuf), 0.5 * c * CHAR_TILE_WIDTH, 
      0.0, 0.5 * CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 
      DISPLAY_BORDER + 0.5 * x * CHAR_TILE_WIDTH, DISPLAY_BORDER + y * CHAR_TILE_HEIGHT, 0);
  if (ul)
    al_draw_filled_rectangle(
        DISPLAY_BORDER + 0.5 * x * CHAR_TILE_WIDTH, DISPLAY_BORDER + (y + 1) * CHAR_TILE_HEIGHT - 2.0, 
        DISPLAY_BORDER + 0.5 * (x + 1) * CHAR_TILE_WIDTH, DISPLAY_BORDER + (y + 1) * CHAR_TILE_HEIGHT - 1.0, 
        al_map_rgb(255, 255, 255));
}

/****************************************************************************/
/*** Put a character in the display buffer for P2000T emulation mode      ***/
/****************************************************************************/
static inline void PutChar_T(int x, int y, int c, int fg, int bg, int si)
{
  int K = c + (fg << 8) + (bg << 16) + (si << 24);
  if (K == OldCharacter[y * 40 + x])
    return;
  OldCharacter[y * 40 + x] = K;

  al_set_target_bitmap(al_get_backbuffer(display));
  al_draw_tinted_bitmap_region(
      (si ? FontBuf_scaled : FontBuf),
      al_map_rgba(Pal[fg * 3], Pal[fg * 3 + 1], Pal[fg * 3 + 2], 255), 
      c * CHAR_TILE_WIDTH, (si >> 1) * CHAR_TILE_HEIGHT, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT,
      DISPLAY_BORDER + x * CHAR_TILE_WIDTH, DISPLAY_BORDER + y * CHAR_TILE_HEIGHT, 0);
  if (bg)
    al_draw_tinted_bitmap_region(
        (si ? FontBuf_bk_scaled : FontBuf_bk),
        al_map_rgba(Pal[bg * 3], Pal[bg * 3 + 1], Pal[bg * 3 + 2], 0), 
        c * CHAR_TILE_WIDTH, (si >> 1) * CHAR_TILE_HEIGHT, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 
        DISPLAY_BORDER + x * CHAR_TILE_WIDTH, DISPLAY_BORDER + y * CHAR_TILE_HEIGHT, 0);
}

/****************************************************************************/
/*** Common.h contains the system-independent part of the screen refresh  ***/
/*** drivers                                                              ***/
/****************************************************************************/
#include "Common.h"
