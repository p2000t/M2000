#pragma once
#define DEFAULT_VIDEO_MODE 1
#define FULLSCREEN_VIDEO_MODE 9

static int DisplayWidth, DisplayHeight, DisplayHBorder, DisplayVBorder, DisplayTileWidth, DisplayTileHeight;
static int _DisplayWidth, _DisplayHeight, _DisplayHBorder, _DisplayVBorder, _DisplayTileWidth, _DisplayTileHeight;
int videomode = DEFAULT_VIDEO_MODE;
int scanlines = 0;
int menubarHeight = 0;
int makeScreenshot = 0;
int ignoreResizeEvent = 0;

ALLEGRO_AUDIO_STREAM *stream = NULL;
ALLEGRO_MIXER *mixer = NULL;
ALLEGRO_FILECHOOSER *cassetteChooser = NULL;
ALLEGRO_FILECHOOSER *cartridgeChooser = NULL;
ALLEGRO_FILECHOOSER *screenshotChooser = NULL;
ALLEGRO_FILECHOOSER *vRamLoadChooser = NULL;
ALLEGRO_FILECHOOSER *vRamSaveChooser = NULL;

int buf_size;
int sample_rate;
signed char *soundbuf = NULL;      /* Pointer to sound buffer               */
int mastervolume=4;               /* Master volume setting                 */

ALLEGRO_EVENT event;
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_EVENT_QUEUE *eventQueue = NULL; // generic queue for keyboard and windows events
ALLEGRO_KEYBOARD_STATE kbdstate;
char *Title="M2000 - Philips P2000 emulator"; /* Title for Window  */

int keyboardmap = 1;               /* 1 = symbolic keyboard mapping         */

ALLEGRO_BITMAP *FontBuf = NULL;
ALLEGRO_BITMAP *FontBuf_bk = NULL;
ALLEGRO_BITMAP *FontBuf_scaled = NULL;
ALLEGRO_BITMAP *FontBuf_bk_scaled = NULL;

static unsigned char joyKeyMapping[2][5] = {
  { 23, 21,  0,  2, 17 }, /* right, down, left, up, fire-button */
  {  2, -1,  0, -1, 17 }  /* Fraxxon mode, using keys left/up for moving */ 
};
int joymode=1;                     /* If 0, do not use joystick             */
int joymap=0;                      /* 0 = default joystick-key mapping      */
ALLEGRO_JOYSTICK *joystick = NULL;
ALLEGRO_JOYSTICK_STATE joyState;
bool lastJoyState[5];

ALLEGRO_EVENT_QUEUE *timerQueue = NULL;
ALLEGRO_TIMER *timer;
static int CpuSpeed;

int soundmode=255;                 /* Sound mode, 255=auto-detect           */
static int soundoff=0;             /* If 1, sound is turned off             */
static int *OldCharacter;          /* Holds characters on the screen        */

static int Displays[][2] = { 
  // width height 
  {  640,   480 },
  {  800,   600 },
  {  960,   720 },
  { 1280,   960 },
};

static unsigned char Pal[8*3] =    /* SAA5050 palette                       */
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

void UpdateDisplaySettings() {
  if (videomode < 0 || videomode >= sizeof(Displays)/sizeof(*Displays)) videomode = DEFAULT_VIDEO_MODE;
  DisplayWidth = Displays[videomode][0];
  DisplayHeight = Displays[videomode][1];
  DisplayTileWidth = DisplayWidth / 40;
  DisplayTileHeight = DisplayHeight / 24;
  DisplayHBorder = DisplayTileWidth;
  DisplayVBorder = DisplayTileHeight / 2;
}