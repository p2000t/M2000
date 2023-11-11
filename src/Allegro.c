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

#define CHAR_PIXEL_WIDTH 6 //must be even
#define CHAR_PIXEL_HEIGHT 2
#define CHAR_TILE_WIDTH (6*CHAR_PIXEL_WIDTH)
#define CHAR_TILE_HEIGHT (10*CHAR_PIXEL_HEIGHT)
#define FONT_BITMAP_WIDTH (96+64+64)*(CHAR_TILE_WIDTH)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_native_dialog.h> 
#include <allegro5/allegro_memfile.h>
#include "Allegro.h"
#include "P2000.h"
#include "Common.h"
#include "Icon.h"
#include "AllegroKeyboard.h"
#include "Menu.h"

/****************************************************************************/
/*** Deallocate resources taken by InitMachine()                          ***/
/****************************************************************************/
void TrashMachine(void)
{
  if (Verbose) printf("\n\nShutting down...\n");
  if (soundbuf) free (soundbuf);
  if (OldCharacter) free (OldCharacter);
}

int InitAllegro() 
{
  if (Verbose) printf("Initialising Allegro... ");
  if (!al_init()) return ShowErrorMessage("Allegro could not initialize its core.");
  if (!al_init_primitives_addon()) return ShowErrorMessage("Allegro could not initialize primitives addon.");
  if (!al_init_image_addon()) return ShowErrorMessage("Allegro could not initialize image addon.");
  if (!al_init_native_dialog_addon()) return ShowErrorMessage("Allegro could not initialize native dialog addon.");
  if (!al_install_keyboard())return ShowErrorMessage("Allegro could not install keyboard.");
  if (Verbose) puts("OK");
  return 1;
}

int ShowErrorMessage(const char *format, ...)
{
  char string[1024];
  va_list args;
  va_start(args, format);
  vsprintf(string, format, args);
  va_end(args);
  al_show_native_message_box(NULL, Title, "", string, "", ALLEGRO_MESSAGEBOX_ERROR);
  return 0; //always return error code
}

void ClearScreen() 
{
  al_set_target_bitmap(al_get_backbuffer(display));
  al_clear_to_color(al_map_rgb(0, 0, 0));
  memset(OldCharacter, -1, 80 * 24 * sizeof(int)); //clear old screen characters
}

void ResetAudioStream() 
{
    if (Verbose) printf("  Creating the audio stream: ");
    for (buf_size = 4096; buf_size >= 128; buf_size /= 2) if (buf_size * IFreq <= 44100) break;
    sample_rate = buf_size * IFreq;
    if (Verbose) printf("%d Hz, buffer size %d...", sample_rate, buf_size);
    if (soundbuf) free(soundbuf);
    soundbuf = malloc(buf_size);
    if (stream) {
      al_detach_audio_stream(stream);
      al_destroy_audio_stream(stream);
    }
    stream = al_create_audio_stream(4, buf_size, sample_rate, ALLEGRO_AUDIO_DEPTH_UINT8, ALLEGRO_CHANNEL_CONF_1);
    if (!stream || !soundbuf) {
      if (Verbose) puts("FAILED");
      soundmode = 0;
    }
    else if (Verbose) puts("OK");

    if (Verbose) printf("  Connecting to the default mixer...");
    if (!mixer) mixer = al_get_default_mixer();
    if (!al_attach_audio_stream_to_mixer(stream, mixer))
    {
      if (Verbose) puts("FAILED");
      soundmode = 0;
    }
    else if (Verbose) puts("OK");
}

void UpdateWindowTitle() 
{
  static char windowTitle[ALLEGRO_NEW_WINDOW_TITLE_MAX_SIZE];
  static char tapeFileName[50];
  if (TapeName) {
    const char * lastSeparator = strrchr(TapeName, PATH_SEPARATOR);
    if (lastSeparator)
      strcpy(tapeFileName, lastSeparator + 1);
    else
      strcpy(tapeFileName, TapeName);
  } else {
    strcpy(tapeFileName, "empty");
  }
  sprintf(windowTitle, "%s [%s]", Title, tapeFileName);
  al_set_window_title(display, windowTitle);
}

void ToggleFullscreen() 
{
  int fullScreen = al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW;
  fullScreen = !fullScreen; //toggle fullscreen
  al_set_display_flag(display , ALLEGRO_FULLSCREEN_WINDOW , fullScreen);
  ClearScreen();

  if (fullScreen) {
    //fullscreen: hide menu and mouse
    al_remove_display_menu(display);
    al_hide_mouse_cursor(display);

    _DisplayWidth = DisplayWidth;
    _DisplayHeight = DisplayHeight;
    _DisplayTileWidth = DisplayTileWidth;
    _DisplayTileHeight = DisplayTileHeight;
    _DisplayHBorder = DisplayHBorder;
    _DisplayVBorder = DisplayVBorder;

    ALLEGRO_MONITOR_INFO info;
    for (int i=0; i<al_get_num_video_adapters(); i++) {
      al_get_monitor_info(i, &info);
      if (info.x1 == 0 && info.y1 == 0) {
        //primary display found
        if (Verbose) printf("Primary fullscreen display: %i x %i\n", info.x2 - info.x1, info.y2 - info.y1);
        DisplayHeight = info.y2 - info.y1;
        DisplayWidth = DisplayHeight * 4 / 3;
        DisplayVBorder = 0;
        DisplayHBorder = (info.x2 - info.x1 - DisplayWidth) / 2;
        DisplayTileWidth = DisplayWidth / 40;
        DisplayTileHeight = DisplayHeight / 24;

        al_resize_display(display, DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder);
        break;
      }
    }
  } else {
    //back to window mode
    DisplayWidth = _DisplayWidth;
    DisplayHeight = _DisplayHeight;
    DisplayTileWidth = _DisplayTileWidth;
    DisplayTileHeight = _DisplayTileHeight;
    DisplayHBorder = _DisplayHBorder;
    DisplayVBorder = _DisplayVBorder;
    al_resize_display(display, DisplayWidth + 2*DisplayHBorder, DisplayHeight -menubarHeight + 2*DisplayVBorder);
    al_show_mouse_cursor(display);
    al_set_display_menu(display,  menu);
  }
}

/****************************************************************************/
/*** Initialise all resources needed by the Linux/SVGALib implementation  ***/
/****************************************************************************/
int InitMachine(void)
{
  int startFullScreen = videomode == FULLSCREEN_VIDEO_MODE;
  //only support CPU speeds 10, 20, 50, 100, 120, 200 and 500
  CpuSpeed = Z80_IPeriod*IFreq*100/2500000;
  if (CpuSpeed > 350) CpuSpeed = 500;
  else if (CpuSpeed > 160) CpuSpeed = 200;
  else if (CpuSpeed > 110) CpuSpeed = 120;
  else if (CpuSpeed > 75) CpuSpeed = 100;
  else if (CpuSpeed > 35) CpuSpeed = 50;
  else if (CpuSpeed > 15) CpuSpeed = 20;
  else CpuSpeed = 10;

  //only support 50Hz and 60Hz
  IFreq = IFreq >= 55 ? 60 : 50; 

  if (!InitAllegro()) return 0;
  
  cassetteChooser = al_create_native_file_dialog(NULL, 
    "Select an existing or new cassette file", "*.*", 0); //file doesn't have to exist
  cartridgeChooser = al_create_native_file_dialog(NULL, 
    "Select a .bin cartridge file", "*.bin", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
  screenshotChooser = al_create_native_file_dialog(NULL,
    "Save as .png or .bmp file",  "*.png;*.bmp", ALLEGRO_FILECHOOSER_SAVE);
  vRamLoadChooser = al_create_native_file_dialog(NULL,
    "Select a .vram file",  "*.vram", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
  vRamSaveChooser = al_create_native_file_dialog(NULL,
    "Save as .vram file",  "*.vram", ALLEGRO_FILECHOOSER_SAVE);

  if (joymode) {
    if (Verbose) printf("Initialising and detecting joystick... ");
    joymode=0; //assume not found
    if (al_install_joystick()) {
      if ((joystick = al_get_joystick(0)) != NULL) {
        joymode = 1;
        if (Verbose) puts("OK");
      }
    }
    if (!joymode && Verbose) puts("FAILED");
  }

  if (Verbose) printf("Creating the display window... ");
  al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE
#ifdef __linux__
    | ALLEGRO_GTK_TOPLEVEL // required for menu in Linux
#endif
  );
  al_set_new_display_option(ALLEGRO_SINGLE_BUFFER, 1, ALLEGRO_REQUIRE); //require single buffer
  UpdateDisplaySettings();
  if ((display = al_create_display(DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder)) == NULL)
    return ShowErrorMessage("Could not initialize display.");
  if (Verbose) puts("OK");

  if (Verbose) printf("Creating timer and queues... ");
  eventQueue = al_create_event_queue();
  timerQueue =  al_create_event_queue();
  timer = al_create_timer(1.0 / IFreq);
  if (!eventQueue || !timerQueue || !timer)
    return ShowErrorMessage("Could not initialize timer and event queues.");
  if (Verbose) puts("OK");

  UpdateWindowTitle();

  //set app icon
  ALLEGRO_FILE *iconFile;
  if ((iconFile = al_open_memfile(p2000icon_png, p2000icon_png_len, "r")) != NULL) {
    ALLEGRO_BITMAP *bm = al_load_bitmap_f(iconFile, ".png");
    al_set_display_icon(display, bm);
    al_fclose(iconFile);
    al_destroy_bitmap(bm);
  }

  al_register_event_source(eventQueue, al_get_display_event_source(display));
  //al_register_event_source(eventQueue, al_get_keyboard_event_source());
  al_register_event_source(eventQueue, al_get_default_menu_event_source());
  al_register_event_source(timerQueue, al_get_timer_event_source(timer));

  if (P2000_Mode) { /* M-mode uses black and white palette */
    Pal[0] = Pal[1] = Pal[2] = 0;
    Pal[3] = Pal[4] = Pal[5] = 255;
  }

  if (Verbose) printf("  Allocating cache buffers... ");
  OldCharacter = malloc(80 * 24 * sizeof(int));
  if (!OldCharacter) return ShowErrorMessage("Could not allocate character buffer.");
  ClearScreen();
  if (Verbose) puts("OK");

  if (soundmode) {
    /* sound init */
    if (Verbose) printf("Initializing sound...");
    if (!al_install_audio()) soundmode = 0;
    if (!al_reserve_samples(0)) soundmode = 0;
    if (Verbose) puts(soundmode ? "OK" :"FAILED");
    ResetAudioStream();
  }

  // create menu
  if (Verbose) printf("Creating menu...");
  CreateEmulatorMenu();
  menubarHeight = al_get_display_height(display) - (DisplayHeight + 2.0*DisplayVBorder);
  // fix display height after menu was attached
  if (menubarHeight > 0)
    al_resize_display(display, DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder -menubarHeight);
  if (Verbose) puts(menu ? "OK" : "FAILED");

  if (startFullScreen)
    ToggleFullscreen();

  /* start the 50Hz/60Hz timer */
  al_start_timer(timer);

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

  if (!soundoff && soundmode) {
    int8_t *playbuf = al_get_audio_stream_fragment(stream);
    if (playbuf) {
      for (i=0;i<buf_size;++i) {
        if (soundbuf[i]) {
          soundstate=soundbuf[i];
          soundbuf[i]=0;
          sample_count=sample_rate/1000;
        }
        playbuf[i]=soundstate+128;
        if (!--sample_count) {
          sample_count=sample_rate/1000;
          if (soundstate>0) --soundstate;
          if (soundstate<0) ++soundstate;
        }
      }
      al_set_audio_stream_fragment(stream, playbuf);
    }
  }

  // sync emulation by waiting for timer event (fired 50/60 times a second)
  if (Sync) {
    if (al_get_next_event(timerQueue, &event)) {
#ifdef DEBUG
      printf("Sync lagged behind @ ts %f...\n", event.timer.timestamp);
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

  if (soundoff || !soundmode) 
    return;

  if (toggle!=last) {
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
  int pixelSW, pixelSE, pixelNW, pixelNE, pixelNN;
  int pixelSSW, pixelSSE, pixelWNW, pixelENE, pixelESE, pixelWSW, pixelNNW, pixelNNE;
  char *TempBuf;
  FILE *F;

  al_set_new_bitmap_flags(ALLEGRO_MAG_LINEAR | ALLEGRO_MIN_LINEAR);
  if (Verbose) printf("Loading font %s...\n", filename);
  if (!FontBuf) {
    if (Verbose) printf("  Creating font bitmap... ");
    if ((FontBuf = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT)) == NULL)
      return ShowErrorMessage("Could not create font bitmap.");
    if (Verbose) puts("OK");
  }

  if (!FontBuf_bk) {
    if (Verbose) printf("  Creating font background bitmap... ");
    if ((FontBuf_bk = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT)) == NULL)
      return ShowErrorMessage("Could not create font background bitmap.");
    if (Verbose) puts("OK");
  }

  if (!P2000_Mode) {
    if (!FontBuf_scaled) { //double height
      if (Verbose) printf("  Creating double-height font bitmap... ");
      if ((FontBuf_scaled = al_create_bitmap(FONT_BITMAP_WIDTH, 2*CHAR_TILE_HEIGHT)) == NULL)
        return ShowErrorMessage("Could not create double-height font bitmap.");
      if (Verbose) puts("OK");
    }

    if (!FontBuf_bk_scaled) {
      if (Verbose) printf("  Creating double-height font background bitmap... ");
      if ((FontBuf_bk_scaled = al_create_bitmap(FONT_BITMAP_WIDTH, 2*CHAR_TILE_HEIGHT)) == NULL)
        return ShowErrorMessage("Could not create double-height font background bitmap.");
      if (Verbose) puts("OK");
    }
  }

  al_set_target_bitmap(FontBuf);
  al_clear_to_color(al_map_rgb(0, 0, 0));
  al_set_target_bitmap(FontBuf_bk);
  al_clear_to_color(al_map_rgb(255, 255, 255));

  if (Verbose) printf("  Allocating memory for temp buffer for font... ");
  TempBuf = malloc(2240);
  if (!TempBuf)
    return ShowErrorMessage("Could not allocate temp buffer for font.");
  if (Verbose) puts("OK");

  if (Verbose) printf("  Opening font file %s... ", filename);
  i = 0;
  F = fopen(filename, "rb");
  if (F) {
    printf("Reading... ");
    if (fread(TempBuf, 2240, 1, F)) i = 1;
    fclose(F);
  }
  if (Verbose) puts(i ? "OK" : "FAILED");
  if (!i) return ShowErrorMessage("Could not read font file %s", filename);

  // Stretch 6x10 characters to 12x20, so we can do character rounding 
  // 96 alpha + 64 graphic (cont) + 64 graphic (sep)
  for (i = 0; i < (96 + 64 + 64) * 10; i += 10) { 
    linePixelsPrevPrev = 0;
    linePixelsPrev = 0;
    linePixels = 0;
    linePixelsNext = TempBuf[i] << 6;
    linePixelsNextNext = TempBuf[i+1] << 6;
    for (line = 0; line < 10; ++line) {
      y = line * CHAR_PIXEL_HEIGHT;
      linePixelsPrevPrev = linePixelsPrev >> 6;
      linePixelsPrev = linePixels >> 6;
      linePixels = linePixelsNext >> 6;
      linePixelsNext = linePixelsNextNext >> 6;
      linePixelsNextNext = line < 8 ? TempBuf[i + line + 2] : 0;

      for (pixelPos = 0; pixelPos < 6; ++pixelPos) {
        x = (i * 6 / 10 + pixelPos) * CHAR_PIXEL_WIDTH;
        if (i < 96 * 10) x-=CHAR_PIXEL_WIDTH/2; // center alpanum characters
        if (linePixels & 0x20) // bit 6 set = pixel set
          drawFontRegion(x, y, x + CHAR_PIXEL_WIDTH, y + CHAR_PIXEL_HEIGHT);
        else {
          /* character rounding */
          if (i < 96 * 10) { // check if within alpanum character range
            // for character rounding, look at 18 pixel around current pixel
            // using 16-wind compass notation + NN and SS
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
            pixelNN = linePixelsPrevPrev & 0x20;
            
            // the extra rounding pixels are in the shape of a (rotated) L
            // rounding in NW direction
            if (pixelN && pixelW && (!pixelNW || (!pixelNE && pixelNNE) || (!pixelSW && pixelWSW))) {
              if (pixelSW && pixelNN) //alternative rounding for outer side of V
                drawFontRegion(x,y,x+CHAR_PIXEL_WIDTH/2,y+CHAR_PIXEL_HEIGHT);
              else
                drawFontRegion(x,y,x+CHAR_PIXEL_WIDTH/2,y+CHAR_PIXEL_HEIGHT/2);
            }
            // rounding in NE direction
            if (pixelN && pixelE && (!pixelNE || (!pixelNW && pixelNNW) || (!pixelSE && pixelESE))) {
              if (pixelSE && pixelNN) //alternative rounding for outer side of V
                drawFontRegion(x+CHAR_PIXEL_WIDTH/2,y,x+CHAR_PIXEL_WIDTH,y+CHAR_PIXEL_HEIGHT);
              else
                drawFontRegion(x+CHAR_PIXEL_WIDTH/2,y,x+CHAR_PIXEL_WIDTH,y+CHAR_PIXEL_HEIGHT/2);
            }
            // rounding in SE direction
            if (pixelS && pixelE && (!pixelSE || (!pixelSW && pixelSSW) || (!pixelNE && pixelENE))) {
              drawFontRegion(x+CHAR_PIXEL_WIDTH/2, y+CHAR_PIXEL_HEIGHT/2, x+CHAR_PIXEL_WIDTH, y+CHAR_PIXEL_HEIGHT);
            }
            // rounding in SW direction
            if (pixelS && pixelW && (!pixelSW || (!pixelSE && pixelSSE) || (!pixelNW && pixelWNW))) {
              drawFontRegion(x, y+CHAR_PIXEL_HEIGHT/2, x+CHAR_PIXEL_WIDTH/2, y+CHAR_PIXEL_HEIGHT);
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

  if (!P2000_Mode) {
    al_set_target_bitmap(FontBuf_scaled);
    al_draw_scaled_bitmap(FontBuf, 
      0.0, 0.0, FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT, 
      0.0, 0.0, FONT_BITMAP_WIDTH, 2.0*CHAR_TILE_HEIGHT, 0);
      
    al_set_target_bitmap(FontBuf_bk_scaled);
    al_draw_scaled_bitmap(FontBuf_bk, 
      0.0, 0.0, FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT, 
      0.0, 0.0, FONT_BITMAP_WIDTH, 2.0*CHAR_TILE_HEIGHT, 0);
  }

  //al_save_bitmap("FontBuf.png", FontBuf);
  return 1;
}

void SaveScreenshot() 
{
  makeScreenshot = al_show_native_file_dialog(display, screenshotChooser) && al_get_native_file_dialog_count(screenshotChooser) > 0;
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
  static byte delayedShiftedKeyPress = 0;
  if (delayedShiftedKeyPress) {
    if (delayedShiftedKeyPress > 100) {
      ReleaseKey(delayedShiftedKeyPress-100);
      ReleaseKey(72); //release LSHIFT
      delayedShiftedKeyPress = 0;
    } else {
      PushKey(delayedShiftedKeyPress);
      delayedShiftedKeyPress += 100;
    }
    return; //stop handling rest of keys
  }

  int i,j,k;
  byte keyPressed;
  bool isCombiKey, isNormalKey, isShiftKey;
  bool isSpecialKeyPressed = 0;
  byte keyCode, keyCodeCombi;
  bool al_shift_down, al_ctrl_alt_cmd_down;
  bool isP2000ShiftDown;
  FILE *f;

  static int pausePressed = 0;
  static bool isNextEvent = false;

  static byte queuedKeys[NUMBER_OF_KEYMAPPINGS] = {0};
  static byte activeKeys[NUMBER_OF_KEYMAPPINGS] = {0};

  //read keyboard state
  al_get_keyboard_state(&kbdstate);
  al_shift_down = al_key_down(&kbdstate,ALLEGRO_KEY_LSHIFT) || al_key_down(&kbdstate,ALLEGRO_KEY_RSHIFT);
  al_ctrl_alt_cmd_down = al_key_down(&kbdstate,ALLEGRO_KEY_COMMAND) ||
     al_key_down(&kbdstate,ALLEGRO_KEY_LCTRL) || 
     al_key_down(&kbdstate,ALLEGRO_KEY_RCTRL) ||
     al_key_down(&kbdstate,ALLEGRO_KEY_ALT);

  if (!al_ctrl_alt_cmd_down) {
    if (keyboardmap == 0) {
      /* Positional Key Mapping */
      //fill P2000 KeyMap
      for (i = 0; i < 80; i++) {
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
    else {
      /* Symbolic Key Mapping */
      isP2000ShiftDown = (~KeyMap[9] & 0xff) ? 1 : 0; // 1 when one of the shift keys is pressed
      for (i = 0; i < NUMBER_OF_KEYMAPPINGS; i++) {
        keyPressed = keyMappings[i][0];
        isCombiKey = keyMappings[i][1] != keyMappings[i][3];
        isNormalKey = !isCombiKey && (keyMappings[i][2] == 0) && (keyMappings[i][4] == 1);
        isShiftKey = keyMappings[i][al_shift_down ? 4 : 2];
        keyCode = keyMappings[i][al_shift_down ? 3 : 1];
        keyCodeCombi = isCombiKey ? keyMappings[i][al_shift_down ? 1 : 3] : -1;

        if (queuedKeys[i] || al_key_down(&kbdstate, keyPressed)) {
          if (isCombiKey) 
            ReleaseKey(keyCodeCombi);
          if (isNormalKey || (isShiftKey == isP2000ShiftDown)) {
            queuedKeys[i] = 0;
            PushKey(keyCode);
          } else {
            // first, the shift must be pressed/un-pressed in this interrupt
            // then in the next interrupt the target key itself will be pressed
            KeyMap[9] = isShiftKey ? 0xfe : 0xff; // 0xfe = LSHIFT
            queuedKeys[i] = 1;
          }
          activeKeys[i] = 1;
          if (!isNormalKey) 
            isSpecialKeyPressed = true;
        } else if (activeKeys[i]) {
          // unpress key and second key in P2000's keyboard matrix
          if (isCombiKey) ReleaseKey(keyCodeCombi);
          ReleaseKey(keyCode);
          activeKeys[i] = 0;
        }
      }
      if (!isSpecialKeyPressed) {
        if (al_key_down(&kbdstate,ALLEGRO_KEY_LSHIFT)) KeyMap[9] &= ~0b00000001; else KeyMap[9] |= 0b00000001;
        if (al_key_down(&kbdstate,ALLEGRO_KEY_RSHIFT)) KeyMap[9] &= ~0b10000000; else KeyMap[9] |= 0b10000000;
        if (al_key_down(&kbdstate,ALLEGRO_KEY_CAPSLOCK)) KeyMap[3] &= ~0b00000001; else KeyMap[3] |= 0b00000001;
      }
    }
  }

  // handle window and menu events
  while ((isNextEvent = al_get_next_event(eventQueue, &event)) || pausePressed) {
    //printf("event.type=%i\n", event.type);

    if (pausePressed) { // pressing F9 can also unpause
      al_get_keyboard_state(&kbdstate);
      if (al_key_up(&kbdstate, ALLEGRO_KEY_F9)) {
        pausePressed = 0;
        al_set_menu_item_flags(menu, SPEED_PAUSE, ALLEGRO_MENU_ITEM_CHECKBOX);
      }
      if (al_key_up(&kbdstate, ALLEGRO_KEY_RIGHT)) {
        break; //advance one frame
      }
    }
    if (!isNextEvent) 
      event.type = 0; //clear event type from last event

    if (event.type == ALLEGRO_EVENT_DISPLAY_FOUND)
      ClearScreen();

    if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)  { //window close icon was clicked
      Z80_Running = 0;
      break;
    }

    if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
      al_acknowledge_resize(display);
      if (!(al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW)) {
        if (firstResize) {
          firstResize = 0;
#ifdef __linux__
          /* fix the display height after menu was attached */
          al_resize_display(display, DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder -menubarHeight);
          ClearScreen();
          return;
  #endif
        }
        DisplayWidth = event.display.width * 40 / 42;
        DisplayHeight = event.display.height * 24 / 25;
        if (3 * DisplayWidth > 4 * DisplayHeight)
          DisplayWidth = DisplayHeight * 4 / 3;
        else
          DisplayHeight = DisplayWidth * 3 / 4;
        DisplayTileWidth = DisplayWidth / 40;
        DisplayTileHeight = DisplayHeight / 24;
        DisplayWidth = DisplayTileWidth * 40;
        DisplayHeight = DisplayTileHeight * 24;
        DisplayHBorder = (event.display.width - DisplayWidth) / 2;
        DisplayVBorder = (event.display.height - DisplayHeight) / 2;
        UpdateViewMenu(-1); //deselect the view-dimensions in menu
        ClearScreen();
      }
    }

    if (event.type == ALLEGRO_EVENT_MENU_CLICK) {
      switch (event.user.data1) {
        case FILE_INSERT_CASSETTE_ID:
        case FILE_INSERTRUN_CASSETTE_ID:
          if (al_show_native_file_dialog(display, cassetteChooser) && al_get_native_file_dialog_count(cassetteChooser) > 0) {
            InsertCassette(AppendExtensionIfMissing(al_get_native_file_dialog_path(cassetteChooser, 0), ".cas"));
            UpdateWindowTitle();
            if (event.user.data1 == FILE_INSERTRUN_CASSETTE_ID) {
              Z80_Reset();
            }
          }
          break;
        case FILE_REMOVE_CASSETTE_ID:
          RemoveCassette();
          UpdateWindowTitle();
          break;
        case FILE_INSERT_CARTRIDGE_ID:
          if (al_show_native_file_dialog(display, cartridgeChooser) && al_get_native_file_dialog_count(cartridgeChooser) > 0)
            InsertCartridge(al_get_native_file_dialog_path(cartridgeChooser, 0));
          break;
        case FILE_REMOVE_CARTRIDGE_ID:
          RemoveCartridge();
          break;
        case FILE_RESET_ID:
          Z80_Reset();
          break;
        case FILE_SAVE_SCREENSHOT_ID:
          SaveScreenshot();
          break;
        case FILE_LOAD_VIDEORAM_ID:
          if (al_show_native_file_dialog(display, vRamLoadChooser) && al_get_native_file_dialog_count(vRamLoadChooser) > 0) {
            if ((f = fopen(al_get_native_file_dialog_path(vRamLoadChooser, 0), "rb")) != NULL) {
              fread(VRAM, 1, 0x1000, f); //read full 4K
              fclose(f);
              RefreshScreen();
            } 
          }
          break;
        case FILE_SAVE_VIDEORAM_ID:
          if (al_show_native_file_dialog(display, vRamSaveChooser) && al_get_native_file_dialog_count(vRamSaveChooser) > 0) {
            if ((f = fopen(AppendExtensionIfMissing(al_get_native_file_dialog_path(vRamSaveChooser, 0), ".vram"), "wb")) != NULL) {
              fwrite(VRAM, 1, 0x1000, f); //write full 4K
              fclose(f);
            }
          }
          break;
        case FILE_EXIT_ID:
          Z80_Running = 0;
          break;
        case SPEED_SYNC:
          Sync = !Sync;
          break;
        case SPEED_PAUSE:
          pausePressed = !pausePressed;
          break;
        case SPEED_10_ID: case SPEED_20_ID: case SPEED_50_ID: case SPEED_100_ID: case SPEED_200_ID: case SPEED_500_ID:
          CpuSpeed = event.user.data1 - SPEED_OFFSET;
          Z80_IPeriod=(2500000*CpuSpeed)/(100*IFreq);
          UpdateCpuSpeedMenu(menu, CpuSpeed);
          break;
        case FPS_50_ID: case FPS_60_ID:
          IFreq = (event.user.data1 == FPS_50_ID ? 50 : 60);
          if ((IFreq == 50 && CpuSpeed == 120) || (IFreq == 60 && CpuSpeed == 100)) {
            CpuSpeed = 2*IFreq;
            UpdateCpuSpeedMenu(menu, CpuSpeed);
          } else {
            Z80_IPeriod=(2500000*CpuSpeed)/(100*IFreq);
          }
          al_set_timer_speed(timer, 1.0 / IFreq);
          ResetAudioStream();
          al_set_menu_item_flags(menu, FPS_50_ID, IFreq==50 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
          al_set_menu_item_flags(menu, FPS_60_ID, IFreq==60 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
          break;
        case KEYBOARD_POSITIONAL_ID:
        case KEYBOARD_SYMBOLIC_ID:
          keyboardmap = !keyboardmap;
          al_set_menu_item_flags(menu, KEYBOARD_SYMBOLIC_ID, keyboardmap==1 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
          al_set_menu_item_flags(menu, KEYBOARD_POSITIONAL_ID, keyboardmap==0 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
          break;
        case KEYBOARD_ZOEK_ID:
          PushKey(72); //LSHIFT
          delayedShiftedKeyPress = 59;
          break;
        case KEYBOARD_START_ID:
          PushKey(72); //LSHIFT
          delayedShiftedKeyPress = 56;
          break;
        case KEYBOARD_STOP_ID:
          PushKey(72); //LSHIFT
          delayedShiftedKeyPress = 16;
          break;
        case KEYBOARD_CLEARCAS_ID:
          PushKey(72); //LSHIFT
          delayedShiftedKeyPress = 51;
          break;
        case OPTIONS_SOUND_ID:
          soundoff = (!soundoff);
          break;
        case OPTIONS_VOLUME_HIGH_ID: case OPTIONS_VOLUME_MEDIUM_ID: case OPTIONS_VOLUME_LOW_ID:
          mastervolume = event.user.data1 - OPTIONS_VOLUME_OFFSET;
          UpdateVolumeMenu(menu,mastervolume);
          break;
        case OPTIONS_JOYSTICK_ID:
          joymode = !joymode;
          break;
        case OPTIONS_JOYSTICK_MAP_0_ID:
        case OPTIONS_JOYSTICK_MAP_1_ID:
          joymap = !joymap;
          al_set_menu_item_flags(menu, OPTIONS_JOYSTICK_MAP_0_ID, joymap==0 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
          al_set_menu_item_flags(menu, OPTIONS_JOYSTICK_MAP_1_ID, joymap==1 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
          break;
        case HELP_ABOUT_ID:
          al_show_native_message_box(display,
            "M2000 - Philips P2000 emulator", "Version "M2000_VERSION,
            "Thanks to Marcel de Kogel for creating this awesome emulator back in 1996.",
            NULL, 0);
          break;
        case DISPLAY_SCANLINES:
          scanlines = !scanlines;
          ClearScreen();
          break;
        case DISPLAY_FULLSCREEN:
          ToggleFullscreen();
          break;
        case DISPLAY_WINDOW_640x480: case DISPLAY_WINDOW_800x600: case DISPLAY_WINDOW_960x720: case DISPLAY_WINDOW_1280x960:
          videomode = event.user.data1 - DISPLAY_WINDOW_MENU - 1;
          UpdateDisplaySettings();
          UpdateViewMenu(videomode);
          al_resize_display(display, DisplayWidth + 2* DisplayHBorder, DisplayHeight -menubarHeight + 2*DisplayVBorder);
          ClearScreen();
          break;
      }
    }
  }

  /* press F5 to Reset (or trace in DEBUG mode) */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F5))
#ifdef DEBUG
    Z80_Trace = !Z80_Trace;
#else
    Z80_Reset ();
#endif

  /* press F7 for screenshot */
  if (al_key_down(&kbdstate, ALLEGRO_KEY_F7))
    SaveScreenshot();
  
  /* F9 = pause / unpause */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F9)) {
    pausePressed = !pausePressed;
    al_set_menu_item_flags(menu, SPEED_PAUSE, pausePressed ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  }

  /* F10 = toggle sound on/off */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F10)) {
    soundoff = (!soundoff);
    al_set_menu_item_flags(menu, OPTIONS_SOUND_ID, soundoff ? ALLEGRO_MENU_ITEM_CHECKBOX : ALLEGRO_MENU_ITEM_CHECKED);
  }

  /* F11 toggle fullscreen */
  if (al_key_up(&kbdstate, ALLEGRO_KEY_F11))
    ToggleFullscreen();

  /* ALT-F4 or CTRL-Q to quit M2000 */
  if ((al_key_down(&kbdstate, ALLEGRO_KEY_ALT) && al_key_down(&kbdstate, ALLEGRO_KEY_F4)) ||
      (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_down(&kbdstate, ALLEGRO_KEY_Q)))
    Z80_Running = 0;

  // handle joystick
  if (joymode) {
    al_get_joystick_state(joystick, &joyState);
    for (i = 0; i < 5; i++) { // 4 directions and 1 button
      if ((i < 4 && joyState.stick[0].axis[i%2] == -2*(i/2)+1) ||
          (i == 4 && joyState.button[0])) {
        PushKey(joyKeyMapping[joymap][i]);
        lastJoyState[i] = 1;
      }
      else {
        if (lastJoyState[i]) ReleaseKey(joyKeyMapping[joymap][i]);
        lastJoyState[i] = 0;
      }
    }
  } 
}

/****************************************************************************/
/*** Pause specified ammount of time                                      ***/
/****************************************************************************/
void Pause(int ms) {
  al_rest((double)ms / 1000.0);
}

void DrawScanlines() {
  int i; 
  ALLEGRO_COLOR evenLineColor = al_map_rgba(0, 0, 0, 50);
  ALLEGRO_COLOR scanlineColor = al_map_rgba(0, 0, 0, 150);
  for (i=DisplayVBorder; i<DisplayHeight + DisplayVBorder; i+=3)
  {
    al_draw_line(DisplayHBorder, i, DisplayWidth + DisplayHBorder, i, scanlineColor, 1);
    al_draw_line(DisplayHBorder, i+1, DisplayWidth + DisplayHBorder, i+1, evenLineColor, 1);
  }
}

void DrawScanlinesTile(int x, int y) {
  int i; 
  ALLEGRO_COLOR evenLineColor = al_map_rgba(0, 0, 0, 50);
  ALLEGRO_COLOR scanlineColor = al_map_rgba(0, 0, 0, 150);
  for (i=DisplayVBorder+y*DisplayTileHeight; i<DisplayVBorder+(y+1)*DisplayTileHeight; i+=3)
  {
    al_draw_line(DisplayHBorder + x*DisplayTileWidth, i, DisplayHBorder + (x+1)*DisplayTileWidth, i, evenLineColor, 1);
    al_draw_line(DisplayHBorder + x*DisplayTileWidth, i+2, DisplayHBorder + (x+1)*DisplayTileWidth, i+2, scanlineColor, 1);
  }
}

/****************************************************************************/
/*** This function is called by the screen refresh drivers to copy the    ***/
/*** off-screen buffer to the actual display                              ***/
/****************************************************************************/
static void PutImage (void) {
#ifdef __linux__
  if (scanlines) DrawScanlines();
#endif
  al_flip_display();
  if (makeScreenshot) {
    makeScreenshot = 0;
    al_save_bitmap(AppendExtensionIfMissing(al_get_native_file_dialog_path(screenshotChooser, 0), ".png"), al_get_target_bitmap());
  }
#ifdef __linux__
  al_clear_to_color(al_map_rgb(0, 0, 0));
#endif
}

/****************************************************************************/
/*** Put a character in the display buffer for P2000M emulation mode      ***/
/****************************************************************************/
static inline void PutChar_M(int x, int y, int c, int eor, int ul) 
{
#ifndef __linux__
  int K = c + (eor << 8) + (ul << 16);
  if (K == OldCharacter[y * 80 + x])
    return;
  OldCharacter[y * 80 + x] = K;
#endif

  al_set_target_bitmap(al_get_backbuffer(display));
  al_draw_scaled_bitmap(
      (eor ? FontBuf_bk : FontBuf), c * CHAR_TILE_WIDTH, 0.0, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 
      DisplayHBorder + 0.5 * x * DisplayTileWidth, DisplayVBorder + y * DisplayTileHeight, 0.5 * DisplayTileWidth, DisplayTileHeight, 0);
  if (ul)
    al_draw_filled_rectangle(
        DisplayHBorder + 0.5 * x * DisplayTileWidth, DisplayVBorder + (y + 1) * DisplayTileHeight - 2.0, 
        DisplayHBorder + 0.5 * (x + 1) * DisplayTileWidth, DisplayVBorder + (y + 1) * DisplayTileHeight - 1.0, 
        al_map_rgb(255, 255, 255));
#ifndef __linux__
  if (scanlines) DrawScanlinesTile(x, y);
#endif
}

/****************************************************************************/
/*** Put a character in the display buffer for P2000T emulation mode      ***/
/****************************************************************************/
static inline void PutChar_T(int x, int y, int c, int fg, int bg, int si)
{
#ifndef __linux__
  int K = c + (fg << 8) + (bg << 16) + (si << 24);
  if (K == OldCharacter[y * 40 + x])
    return;
  OldCharacter[y * 40 + x] = K;
#endif

  al_set_target_bitmap(al_get_backbuffer(display));
  al_draw_tinted_scaled_bitmap(
      (si ? FontBuf_scaled : FontBuf),
      al_map_rgba(Pal[fg * 3], Pal[fg * 3 + 1], Pal[fg * 3 + 2], 255), 
      c * CHAR_TILE_WIDTH, (si >> 1) * CHAR_TILE_HEIGHT, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT,
      DisplayHBorder + x * DisplayTileWidth, DisplayVBorder + y * DisplayTileHeight,
      DisplayTileWidth, DisplayTileHeight, 0);
  if (bg)
    al_draw_tinted_scaled_bitmap(
        (si ? FontBuf_bk_scaled : FontBuf_bk),
        al_map_rgba(Pal[bg * 3], Pal[bg * 3 + 1], Pal[bg * 3 + 2], 0), 
        c * CHAR_TILE_WIDTH, (si >> 1) * CHAR_TILE_HEIGHT, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 
        DisplayHBorder + x * DisplayTileWidth, DisplayVBorder + y * DisplayTileHeight,
        DisplayTileWidth, DisplayTileHeight, 0);
#ifndef __linux__
  if (scanlines) DrawScanlinesTile(x, y);
#endif
}