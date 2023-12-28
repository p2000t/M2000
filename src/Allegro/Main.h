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
/*   Copyright (C) 2013-2023 by the M2000 team.                               */
/*                                                                            */
/*   See the file "LICENSE" for information on usage and redistribution of    */
/*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       */
/******************************************************************************/

#pragma once

#define FULLSCREEN_VIDEO_MODE 99

#include "../P2000.h"

static int DisplayWidth, DisplayHeight, DisplayHBorder, DisplayVBorder, DisplayTileWidth, DisplayTileHeight;
int videomode, optimalVideomode;
int scanlines;
int smoothing;

ALLEGRO_CONFIG *config = NULL;

ALLEGRO_AUDIO_STREAM *stream = NULL;
ALLEGRO_MIXER *mixer = NULL;

ALLEGRO_PATH *docPath = NULL;
ALLEGRO_PATH *userScreenshotsPath = NULL;
ALLEGRO_PATH *userVideoRamDumpsPath = NULL;
ALLEGRO_PATH *userCassettesPath = NULL;
ALLEGRO_PATH *userCartridgesPath = NULL;
ALLEGRO_PATH *userSavestatesPath = NULL;
ALLEGRO_PATH *currentTapePath = NULL;

int buf_size;
int sample_rate;
signed char *soundbuf = NULL;      /* Pointer to sound buffer               */
int mastervolume;                  /* Master volume setting                 */

ALLEGRO_EVENT event;
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_MOUSE_CURSOR *hiddenMouse = NULL;
ALLEGRO_MONITOR_INFO monitorInfo;
ALLEGRO_EVENT_QUEUE *eventQueue = NULL; // generic queue for keyboard and windows events
ALLEGRO_KEYBOARD_STATE kbdstate;
char *Title="M2000 - Philips P2000 emulator"; /* Title for Window  */
char DocumentPath[FILENAME_MAX];
char ProgramPath[FILENAME_MAX];

int keyboardmap;

ALLEGRO_BITMAP *FontBuf = NULL;
ALLEGRO_BITMAP *FontBuf_bk = NULL;
ALLEGRO_BITMAP *smFontBuf = NULL;
ALLEGRO_BITMAP *smFontBuf_bk = NULL;

static unsigned char joyKeyMapping[2][5] = {
  { 23, 21,  0,  2, 17 }, /* right, down, left, up, fire-button */
  {  2, -1,  0, -1, 17 }  /* Fraxxon mode, using keys left/up for moving */ 
};
int joymode;
int joyDetected;               
int joymap;
int uilanguage;                   
ALLEGRO_JOYSTICK *joystick = NULL;
ALLEGRO_JOYSTICK_STATE joyState;
bool lastJoyState[5];

ALLEGRO_EVENT_QUEUE *timerQueue = NULL;
ALLEGRO_TIMER *timer;

int soundmode;                     /* Sound mode, 1=on                      */
int soundDetected;
static int *OldCharacter;          /* Holds characters on the screen        */

static int Displays[][2] = { 
  // width height 
  {   -1,    -1 }, //autodetect
  {  640,   480 },
  {  960,   720 },
  { 1280,   960 },
  { 1600,  1200 },
  { 1920,  1440 },
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

static const char *installCassettes[] = {
  "Basic Demo Cassette (zijde A).cas",
  "Basic Demo Cassette (zijde B).cas",
  NULL
};
static const char *installCartridges[] = {
  "Basic 1.1 NL.bin",
  "Familiegeheugen 2.0 NL.bin",
  NULL
};

void refreshPath(ALLEGRO_PATH **path, const char * newCPath)
{
  al_destroy_path(*path);
  (*path) = al_create_path(newCPath);
  al_set_path_filename((*path), NULL);
}

void InitVideoMode() 
{
  int i;
  if (videomode <= 0 || videomode >= sizeof(Displays)/sizeof(*Displays)) {
    // autodetect best videomode, which should fit within 75% of the screensize
    for (i = sizeof(Displays)/sizeof(*Displays)-1; i>1; i--) {
      if ((Displays[i][0] / 40 * 42) <= 0.8*(monitorInfo.x2 - monitorInfo.x1) && 
          (Displays[i][1] / 24 * 25) <= 0.8*(monitorInfo.y2 - monitorInfo.y1))
        break;
    }
    optimalVideomode = videomode = i; // 640 x 480 will be the minimum
    if (Verbose) printf("optmimal window size: %i x %i ... ", Displays[videomode][0], Displays[videomode][1]);
  }
}

void UpdateDisplaySettings() 
{
  DisplayWidth = Displays[videomode][0];
  DisplayHeight = Displays[videomode][1];
  DisplayTileWidth = DisplayWidth / 40;
  DisplayTileHeight = DisplayHeight / 24;
  DisplayHBorder = DisplayTileWidth;
  DisplayVBorder = DisplayTileHeight / 2;
  if (Verbose) printf("DisplayTileWidth: %i, DisplayTileHeight: %i\n", DisplayTileWidth, DisplayTileHeight);
}