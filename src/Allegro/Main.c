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

#define CHAR_PIXEL_WIDTH 2 //must be even
#define CHAR_PIXEL_HEIGHT 2
#define CHAR_TILE_WIDTH (6*CHAR_PIXEL_WIDTH)
#define CHAR_TILE_SPACE 3
#define CHAR_TILE_HEIGHT (10*CHAR_PIXEL_HEIGHT)
#define FONT_BITMAP_WIDTH (96+64+64)*(CHAR_TILE_WIDTH + CHAR_TILE_SPACE)

#ifdef __APPLE__
#define ALLEGRO_UNSTABLE // needed for al_clear_keyboard_state();
#define OSX_COMMAND_KEY_FIX if (al_key_down(&kbdstate,ALLEGRO_KEY_COMMAND)) al_clear_keyboard_state(display);
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_native_dialog.h> 
#include "../P2000.h"
#include "../M2000.h"
#include "../Common.h"
#include "Main.h"
#include "Keyboard.h"
#include "Menu.h"
#include "State.h"
#include "Config.h"

/****************************************************************************/
/*** Deallocate resources taken by InitMachine()                          ***/
/****************************************************************************/
void TrashMachine(void)
{
  if (Verbose) printf("\n\nShutting down...\n");
  if (soundbuf) free (soundbuf);
  if (OldCharacter) free (OldCharacter);
}

void ShowErrorMessage(const char *format, ...)
{
  char string[1024];
  va_list args;
  va_start(args, format);
  vsprintf(string, format, args);
  va_end(args);
  al_show_native_message_box(NULL, Title, "", string, "", ALLEGRO_MESSAGEBOX_ERROR);
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
      soundDetected = 0;
    }
    else if (Verbose) puts("OK");

    if (Verbose) printf("  Connecting to the default mixer...");
    if (!mixer) mixer = al_get_default_mixer();
    if (!al_attach_audio_stream_to_mixer(stream, mixer))
    {
      if (Verbose) puts("FAILED");
      soundDetected = 0;
    }
    else if (Verbose) puts("OK");
}

void UpdateWindowTitle() 
{
  static char windowTitle[ALLEGRO_NEW_WINDOW_TITLE_MAX_SIZE];
  sprintf(windowTitle, "%s [%s]", Title, currentTapePath ? al_get_path_filename(currentTapePath) : _(NO_CASSETTE));
  al_set_window_title(display, windowTitle);
}

void ToggleFullscreen() 
{
#ifdef __linux__
  return;
#endif
  ClearScreen();
  if (al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW) {
    //back to window mode
    UpdateDisplaySettings();
    if (Verbose) printf("Back to window %ix%i\n", DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder);
#ifdef __APPLE__
    al_set_display_flag(display , ALLEGRO_FULLSCREEN_WINDOW , 0);
    al_resize_display(display, DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder);
#else
    al_resize_display(display, DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder);
    al_set_display_flag(display , ALLEGRO_FULLSCREEN_WINDOW , 0);
#endif 
    al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT); //al_show_mouse_cursor(display);
    al_set_display_menu(display,  menu);
  } else {
    //go fullscreen and hide menu and mouse
    al_remove_display_menu(display);
    al_set_mouse_cursor(display, hiddenMouse); //al_hide_mouse_cursor(display);
    DisplayHeight = ((monitorInfo.y2 - monitorInfo.y1) / 240) * 240;
    DisplayWidth = DisplayHeight * 4 / 3;
    DisplayVBorder = (monitorInfo.y2 - monitorInfo.y1 - DisplayHeight) / 2;
    DisplayHBorder = (monitorInfo.x2 - monitorInfo.x1 - DisplayWidth) / 2;
    DisplayTileWidth = DisplayWidth / 40;
    DisplayTileHeight = DisplayHeight / 24;
    if (Verbose) printf("Fullscreen resizing to %ix%i\n",DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder);
    al_resize_display(display, DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder);
    al_set_display_flag(display , ALLEGRO_FULLSCREEN_WINDOW , 1);
  }
}

int CopyFile(ALLEGRO_PATH *sourceFolder, const char * filename, ALLEGRO_PATH *destinationFolder) 
{
  if (!al_make_directory(al_path_cstr(destinationFolder, PATH_SEPARATOR))) return 1;

  al_set_path_filename(sourceFolder, filename);
  al_set_path_filename(destinationFolder, filename);
  const char * sourceFile = al_path_cstr(sourceFolder, PATH_SEPARATOR);
  const char * destFile = al_path_cstr(destinationFolder, PATH_SEPARATOR);
  al_set_path_filename(sourceFolder, NULL);
  al_set_path_filename(destinationFolder, NULL);
  if (!al_filename_exists(sourceFile)) {
    if (Verbose) printf("[CopyFile] SourceFile %s does not exist\n", sourceFile);
    return 1;
  }
  if (al_filename_exists(destFile)) {
    if (Verbose) printf("[CopyFile] DestFile %s already exists\n", destFile);
    return 0;
  }

  if (Verbose) printf("[CopyFile] Copying %s to %s ...", sourceFile, destFile);
  FILE *f_s;
  FILE *f_d;
  char buf[1024];
  size_t size;
  int ret = 1;
  if ((f_s = fopen(sourceFile, "rb"))) {
    if ((f_d = fopen(destFile, "wb"))) {
      while ((size = fread(buf, 1, 1024, f_s)))
        fwrite(buf, 1, size, f_d);
      fclose(f_d);
      ret = 0;
    }
    fclose(f_s);
  }
  if (Verbose) puts(ret ? "FAILED" : "OK");
  return ret;
}

void InitDocumentFolders() 
{
  int i;
  ALLEGRO_PATH *resourcePath = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
  // debian install check
  if (!strcmp(al_path_cstr(resourcePath, PATH_SEPARATOR), "/usr/bin/"))
    resourcePath = al_create_path_for_directory("/usr/share/M2000/");

  CopyFile(resourcePath, "README.md", docPath);
  al_set_path_filename(docPath, NULL);

  if (!al_filename_exists(al_path_cstr(userCassettesPath, PATH_SEPARATOR)))
    for (i=0; installCassettes[i]; i++)
      CopyFile(resourcePath, installCassettes[i], userCassettesPath);

  if (!al_filename_exists(al_path_cstr(userCartridgesPath, PATH_SEPARATOR)))
    for (i=0; installCartridges[i]; i++)
      CopyFile(resourcePath, installCartridges[i], userCartridgesPath);

  al_make_directory(al_path_cstr(userScreenshotsPath, PATH_SEPARATOR));

  al_make_directory(al_path_cstr(userVideoRamDumpsPath, PATH_SEPARATOR));

  al_make_directory(al_path_cstr(userSavestatesPath, PATH_SEPARATOR));

  al_destroy_path(resourcePath);
}

/****************************************************************************/
/*** Initialise all resources needed by the Allegro implementation        ***/
/*** Returns 0 on init failure                                            ***/
/****************************************************************************/
int InitMachine(void)
{
  int startFullScreen = (videomode == FULLSCREEN_VIDEO_MODE);

  if (TapeName) 
    currentTapePath = al_create_path(TapeName);

  /* get primary display monitor info */
  for (int i=0; i<al_get_num_video_adapters(); i++) {
    if (al_get_monitor_info(i, &monitorInfo) && monitorInfo.x1 == 0 && monitorInfo.y1 == 0) {
      if (Verbose) printf("Primary fullscreen display: %i x %i\n", monitorInfo.x2 - monitorInfo.x1, monitorInfo.y2 - monitorInfo.y1);
      break; //primary display found
    }
  }

  InitDocumentFolders();

  if (Verbose) printf("Initialising and detecting joystick... ");
  joyDetected=0; //assume not found
  if (al_install_joystick()) {
    if ((joystick = al_get_joystick(0)) != NULL) {
      joyDetected = 1;
      if (Verbose) puts("OK");
    }
  }
  if (!joyDetected && Verbose) puts("FAILED");

  if (Verbose) printf("Creating the display window... ");
  InitVideoMode();
  UpdateDisplaySettings();
  al_set_new_display_option(ALLEGRO_SINGLE_BUFFER, 1, ALLEGRO_REQUIRE); //require single buffer
  al_set_new_display_option(ALLEGRO_VSYNC, 2, ALLEGRO_REQUIRE); //disable vsync
#ifdef __linux__
  al_set_new_display_flags (ALLEGRO_WINDOWED | ALLEGRO_GTK_TOPLEVEL); // ALLEGRO_GTK_TOPLEVEL required for menu in Linux
  // for Linux create smallest display, as it does not correctly scale back
  display = al_create_display(Displays[1][0], Displays[1][1]);
#else
  al_set_new_display_flags (ALLEGRO_WINDOWED);
  display = al_create_display(DisplayWidth + 2*DisplayHBorder, DisplayHeight + 2*DisplayVBorder);
#endif
  if (!display) {
    ShowErrorMessage("Could not initialize display.");
    return 0;
  }
  if (Verbose) puts("OK");

  if (Verbose) printf("Creating timer and queues... ");
  eventQueue = al_create_event_queue();
  timerQueue =  al_create_event_queue();
  timer = al_create_timer(1.0 / IFreq);
  if (!eventQueue || !timerQueue || !timer) {
    ShowErrorMessage("Could not initialize timer and event queues.");
    return 0;
  }
  if (Verbose) puts("OK");

  UpdateWindowTitle();
#ifdef _WIN32
  al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
  ALLEGRO_BITMAP *iconbm = al_create_bitmap(32,32);
  al_set_target_bitmap(iconbm);
  al_draw_filled_circle(16,16,8,al_map_rgb(255, 0, 0));
  al_set_display_icon(display, iconbm);
  al_destroy_bitmap(iconbm);
#endif

  ALLEGRO_BITMAP *empty = al_create_bitmap(1,1);
  hiddenMouse = al_create_mouse_cursor(empty,0,0);
  al_destroy_bitmap(empty);

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
  if (!OldCharacter) {
    ShowErrorMessage("Could not allocate character buffer.");
    return 0;
  }
  ClearScreen();
  if (Verbose) puts("OK");

  /* sound init */
  if (Verbose) printf("Initializing sound...");
  soundDetected = al_install_audio() && al_reserve_samples(0);
  if (Verbose) puts(soundDetected ? "OK" :"FAILED");
  ResetAudioStream();

  // create menu
  if (Verbose) printf("Creating menu...");
  CreateEmulatorMenu();

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

  if (soundmode && soundDetected) {
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
}

void SyncEmulation(void)
{
  // sync emulation by waiting for timer event (fired 50/60 times a second)
  if (Sync) {
    if (al_get_next_event(timerQueue, &event)) {
      if (Debug)
        printf("Sync lagged behind @ ts %f...\n", event.timer.timestamp);
      al_flush_event_queue(timerQueue);
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

  if (!soundmode || !soundDetected) 
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
int LoadFont(const char *filename)
{
  int i, line, x, y, pixelPos;
  int linePixels, linePixelsPrev, linePixelsNext;
  int pixelN, pixelE, pixelS, pixelW;
  int pixelSW, pixelSE, pixelNW, pixelNE;
  char *TempBuf;
  FILE *F;

  if (Verbose) printf("Loading font %s...\n", filename);
  al_set_new_bitmap_flags(ALLEGRO_CONVERT_BITMAP);
  FontBuf = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT);
  FontBuf_bk = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT);

  al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
  smFontBuf = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT);
  smFontBuf_bk = al_create_bitmap(FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT);
  
  if (!FontBuf || !FontBuf_bk || !smFontBuf || !smFontBuf_bk ) {
    ShowErrorMessage("Could not create font bitmaps.");
    return 0;
  }

  al_set_target_bitmap(FontBuf);
  al_clear_to_color(al_map_rgb(0, 0, 0));
  al_set_target_bitmap(FontBuf_bk);
  al_clear_to_color(al_map_rgb(255, 255, 255));

  if (Verbose) printf("  Allocating memory for temp buffer for font... ");
  TempBuf = malloc(2240);
  if (!TempBuf) {
    ShowErrorMessage("Could not allocate temp buffer for font.");
    return 0;
  }
  if (Verbose) puts("OK");

  if (Verbose) printf("  Opening font file %s... ", filename);
  i = 0;
  F = fopen(filename, "rb");
  if (F) {
    if (Verbose) printf("Reading... ");
    if (fread(TempBuf, 2240, 1, F)) i = 1;
    fclose(F);
  }
  if (Verbose) puts(i ? "OK" : "FAILED");
  if (!i) {
    ShowErrorMessage("Could not read font file %s", filename);
    return 0;
  }

  // Stretch 6x10 characters to 12x20, so we can do character rounding 
  // 96 alpha + 64 graphic (cont) + 64 graphic (sep)
  for (i = 0; i < (96 + 64 + 64) * 10; i += 10) { 
    linePixelsPrev = 0;
    linePixels = 0;
    linePixelsNext = TempBuf[i] << 6;
    for (line = 0; line < 10; ++line) {
      y = line * CHAR_PIXEL_HEIGHT;
      linePixelsPrev = linePixels >> 6;
      linePixels = linePixelsNext >> 6;
      linePixelsNext = line < 9 ? TempBuf[i + line + 1] : 0;

      for (pixelPos = 0; pixelPos < 6; ++pixelPos) {
        x = (i * 6 / 10 + pixelPos) * CHAR_PIXEL_WIDTH + i / 10 * CHAR_TILE_SPACE;
        if (linePixels & 0x20) // bit 6 set = pixel set
          drawFontRegion(
            x - (pixelPos ? 0 : 1), 
            y - (line ? 0 : 1), 
            x + CHAR_PIXEL_WIDTH + (pixelPos == 5 ? 1 : 0), 
            y + CHAR_PIXEL_HEIGHT+ (line == 9 ? 1 : 0));
        else {
          /* character rounding */
          if (i < 96 * 10) { // check if within alpanum character range
            // for character rounding, look at 8 pixels around current pixel
            pixelN = linePixelsPrev & 0x20;
            pixelE = linePixels & 0x10;
            pixelS = linePixelsNext & 0x20;
            pixelW = linePixels & 0x40;
            pixelNE = linePixelsPrev & 0x10;
            pixelSE = linePixelsNext & 0x10;
            pixelSW = linePixelsNext & 0x40;
            pixelNW = linePixelsPrev & 0x40;
            
            // rounding in NW direction
            if (pixelN && pixelW && !pixelNW) {
              drawFontRegion(x,y,x+CHAR_PIXEL_WIDTH/2,y+CHAR_PIXEL_HEIGHT/2);
            }
            // rounding in NE direction
            if (pixelN && pixelE && !pixelNE) {
              drawFontRegion(x+CHAR_PIXEL_WIDTH/2,y,x+CHAR_PIXEL_WIDTH,y+CHAR_PIXEL_HEIGHT/2);
            }
            // rounding in SE direction
            if (pixelS && pixelE && !pixelSE) {
              drawFontRegion(x+CHAR_PIXEL_WIDTH/2, y+CHAR_PIXEL_HEIGHT/2, x+CHAR_PIXEL_WIDTH, y+CHAR_PIXEL_HEIGHT);
            }
            // rounding in SW direction
            if (pixelS && pixelW && !pixelSW) {
              drawFontRegion(x, y+CHAR_PIXEL_HEIGHT/2, x+CHAR_PIXEL_WIDTH/2, y+CHAR_PIXEL_HEIGHT);
            }
          }
        }
        //process next pixel to the right
        linePixelsPrev<<=1;
        linePixels<<=1;
        linePixelsNext<<=1;
      }
    }
  }
  free(TempBuf);

  // copy the font bitmaps to smoothened bitmaps
  al_set_target_bitmap(smFontBuf);
  al_draw_scaled_bitmap(FontBuf, 0, 0, FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT, 0, 0, FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT, 0);
  al_set_target_bitmap(smFontBuf_bk);
  al_draw_scaled_bitmap(FontBuf_bk, 0, 0, FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT, 0, 0, FONT_BITMAP_WIDTH, CHAR_TILE_HEIGHT, 0); 

  //al_save_bitmap("FontBuf.png", FontBuf);
  return 1;
}

#ifdef _WIN32
const wchar_t *GetWideFilename(const char * filename)
{
  static wchar_t _filename_utf16[2*FILENAME_MAX];
  ALLEGRO_USTR *_filename = al_ustr_new(filename);
  al_ustr_encode_utf16(_filename, _filename_utf16, 2*FILENAME_MAX-1);
  al_ustr_free(_filename);
  return _filename_utf16;
}
#endif

void SaveVideoRAM(const char * filename) 
{
  int i;
  FILE *f;
#ifdef _WIN32
  f = _wfopen(GetWideFilename(filename), L"wb");
#else
  f = fopen(filename, "wb");
#endif
  if (f != NULL) {
    // for each of the 24 lines, write 40 chars and skip 40 chars
    for (i=0;i<24;i++)
      fwrite(VRAM + ScrollReg + i*80, 1, 40, f); 
    fclose(f);
  }
}

void OpenCassetteDialog(bool boot) 
{
  FILE *f;
  const char *_cassetteFilePath = NULL;
  al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
  ALLEGRO_FILECHOOSER *cassetteChooser = NULL;
  cassetteChooser = al_create_native_file_dialog(al_path_cstr(userCassettesPath, PATH_SEPARATOR), _(DIALOG_LOAD_CASSETTE), "*.*", 0); //file doesn't have to exist
  if (al_show_native_file_dialog(display, cassetteChooser) && al_get_native_file_dialog_count(cassetteChooser) > 0) {
    _cassetteFilePath = AppendExtensionIfMissing(al_get_native_file_dialog_path(cassetteChooser, 0), ".cas");
    // first try open for update, then try create then try read-only
#ifdef _WIN32
    if ((f = _wfopen(GetWideFilename(_cassetteFilePath), L"r+b")) == NULL) f = _wfopen(GetWideFilename(_cassetteFilePath), L"w+b");
    InsertCassette(_cassetteFilePath, f ? f : _wfopen(GetWideFilename(_cassetteFilePath), L"rb"), (f == NULL));
#else
    if ((f = fopen(_cassetteFilePath,"r+b")) == NULL) f = fopen(_cassetteFilePath, "w+b");
    InsertCassette(_cassetteFilePath, f ? f : fopen(_cassetteFilePath, "rb"), (f == NULL));
#endif
    al_destroy_path(currentTapePath);
    currentTapePath = al_create_path(TapeName);
    UpdateWindowTitle();
    refreshPath(&userCassettesPath, TapeName);
    if (boot)
      Z80_Reset();
  }
  al_destroy_native_file_dialog(cassetteChooser);
  if (al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW)
    al_set_mouse_cursor(display, hiddenMouse);
}

void IndicateActionDone() {
  //briefly flash white screen to indicate action was done
  al_clear_to_color(al_map_rgb(255, 255, 255));
  al_flip_display();
  Pause(20);
  ClearScreen();
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
  bool al_shift_down;
  bool isP2000ShiftDown;
  FILE *f;

  ALLEGRO_FILECHOOSER *cartridgeChooser = NULL;
  ALLEGRO_FILECHOOSER *screenshotChooser = NULL;
  ALLEGRO_FILECHOOSER *vRamLoadChooser = NULL;
  ALLEGRO_FILECHOOSER *vRamSaveChooser = NULL;
  ALLEGRO_FILECHOOSER *stateLoadChooser = NULL;
  ALLEGRO_FILECHOOSER *stateSaveChooser = NULL;

  static int pausePressed = 0;
  static bool isNextEvent = false;

  static byte queuedKeys[NUMBER_OF_KEYMAPPINGS] = {0};
  static byte activeKeys[NUMBER_OF_KEYMAPPINGS] = {0};

  //read keyboard state
  al_get_keyboard_state(&kbdstate);
  al_shift_down = al_key_down(&kbdstate,ALLEGRO_KEY_LSHIFT) || al_key_down(&kbdstate,ALLEGRO_KEY_RSHIFT);
  
  static int debugLastKeydown = -1;
  if (Debug) for (i=0;i<256;i++) {
    if (al_key_down(&kbdstate,i)) {
      if (i!=debugLastKeydown) printf("Allegro key down code: %i\n", i);
      debugLastKeydown = i;
    } else if (i==debugLastKeydown) debugLastKeydown = -1;
  }

  if (!al_key_down(&kbdstate,ALLEGRO_KEY_LCTRL) && !al_key_down(&kbdstate,ALLEGRO_KEY_ALT)) {
    if (keyboardmap == 0) {
      /* Positional Key Mapping */
      //fill P2000 KeyMap
      for (i = 0; i < 80; i++) {
        k = i / 8;
        j = 1 << (i % 8);
        if (keymask[i] == ALLEGRO_KEY_UNKNOWN)
          continue;
        if (al_key_down(&kbdstate, (keymask[i] & 0xff)) || (keymask[i] && al_key_down(&kbdstate, (keymask[i] >> 8)))) {
          KeyMap[k] &= ~j;
#ifdef __APPLE__
          OSX_COMMAND_KEY_FIX
#endif
        }
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
#ifdef __APPLE__
          OSX_COMMAND_KEY_FIX
#endif
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
    if (Debug && isNextEvent) printf("Allegro display/menu event.type: %i\n", event.type);
#ifdef __APPLE__
    if (isNextEvent) al_clear_keyboard_state(display); //event? -> reset keyboard state
#endif

    if (pausePressed) { // pressing Ctrl-P toggles pause
      al_get_keyboard_state(&kbdstate);
      if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_P)) {
        pausePressed = Z80_Trace = 0;
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

    if (event.type == ALLEGRO_EVENT_MENU_CLICK) {
      switch (event.user.data1) {
        case FILE_INSERT_CASSETTE_ID:
        case FILE_INSERTRUN_CASSETTE_ID:
          OpenCassetteDialog(event.user.data1 == FILE_INSERTRUN_CASSETTE_ID);
          break;
        case FILE_REMOVE_CASSETTE_ID:
          al_destroy_path(currentTapePath);
          currentTapePath = NULL;
          RemoveCassette();
          UpdateWindowTitle();
          break;
        case FILE_INSERT_CARTRIDGE_ID:
          cartridgeChooser = al_create_native_file_dialog(al_path_cstr(userCartridgesPath, PATH_SEPARATOR), _(DIALOG_LOAD_CARTRIDGE), "*.bin", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
          if (al_show_native_file_dialog(display, cartridgeChooser) && al_get_native_file_dialog_count(cartridgeChooser) > 0) {
            const char *_filename = al_get_native_file_dialog_path(cartridgeChooser, 0);
#ifdef _WIN32
            f = _wfopen(GetWideFilename(_filename), L"rb");
#else
            f = fopen(_filename, "rb");
#endif
            InsertCartridge(_filename, f);
            refreshPath(&userCartridgesPath, CartName);
          }
          al_destroy_native_file_dialog(cartridgeChooser);
          break;
        case FILE_REMOVE_CARTRIDGE_ID:
          RemoveCartridge();
          break;
        case FILE_RESET_ID:
          Z80_Reset();
          break;
        case FILE_SAVE_SCREENSHOT_ID:
          screenshotChooser = al_create_native_file_dialog(al_path_cstr(userScreenshotsPath, PATH_SEPARATOR), _(DIALOG_SAVE_SCREENSHOT),  "*.png;*.bmp", ALLEGRO_FILECHOOSER_SAVE);
          if (al_show_native_file_dialog(display, screenshotChooser) && al_get_native_file_dialog_count(screenshotChooser) > 0) {
            al_save_bitmap(AppendExtensionIfMissing(al_get_native_file_dialog_path(screenshotChooser, 0), ".png"), al_get_target_bitmap());
            refreshPath(&userScreenshotsPath, al_get_native_file_dialog_path(screenshotChooser, 0));
          }
          al_destroy_native_file_dialog(screenshotChooser);
          break;
        case FILE_LOAD_VIDEORAM_ID:
          vRamLoadChooser = al_create_native_file_dialog(al_path_cstr(userVideoRamDumpsPath, PATH_SEPARATOR), _(DIALOG_LOAD_VRAM),  "*.vram", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
          if (al_show_native_file_dialog(display, vRamLoadChooser) && al_get_native_file_dialog_count(vRamLoadChooser) > 0) {
            const char *_filename = al_get_native_file_dialog_path(vRamLoadChooser, 0);
#ifdef _WIN32
            f = _wfopen(GetWideFilename(_filename), L"rb");
#else
            f = fopen(_filename, "rb");
#endif
            if (f != NULL) {
              // for each of the 24 lines, read 40 chars and skip 40 chars
              for (i=0;i<24;i++)
                fread(VRAM + ScrollReg + i*80, 1, 40, f); 
              fclose(f);
              RefreshScreen();
              refreshPath(&userVideoRamDumpsPath, al_get_native_file_dialog_path(vRamLoadChooser, 0));
            } 
          }
          al_destroy_native_file_dialog(vRamLoadChooser);
          break;
        case FILE_SAVE_VIDEORAM_ID:
          vRamSaveChooser = al_create_native_file_dialog(al_path_cstr(userVideoRamDumpsPath, PATH_SEPARATOR), _(DIALOG_SAVE_VRAM),  "*.vram", ALLEGRO_FILECHOOSER_SAVE);
          if (al_show_native_file_dialog(display, vRamSaveChooser) && al_get_native_file_dialog_count(vRamSaveChooser) > 0) {
            SaveVideoRAM(AppendExtensionIfMissing(al_get_native_file_dialog_path(vRamSaveChooser, 0), ".vram"));
            refreshPath(&userVideoRamDumpsPath, al_get_native_file_dialog_path(vRamSaveChooser, 0));
          }
          al_destroy_native_file_dialog(vRamSaveChooser);
          break;
        case FILE_LOAD_STATE_ID:
          stateLoadChooser = al_create_native_file_dialog(al_path_cstr(userSavestatesPath, PATH_SEPARATOR), _(DIALOG_LOAD_STATE),  "*.sav", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
          if (al_show_native_file_dialog(display, stateLoadChooser) && al_get_native_file_dialog_count(stateLoadChooser) > 0) {
            LoadState(al_get_native_file_dialog_path(stateLoadChooser, 0), NULL);
            refreshPath(&userSavestatesPath, al_get_native_file_dialog_path(stateLoadChooser, 0));
          }
          al_destroy_native_file_dialog(stateLoadChooser);
          break;
        case FILE_SAVE_STATE_ID:
          stateSaveChooser = al_create_native_file_dialog(al_path_cstr(userSavestatesPath, PATH_SEPARATOR), _(DIALOG_SAVE_STATE),  "*.sav", ALLEGRO_FILECHOOSER_SAVE);
          if (al_show_native_file_dialog(display, stateSaveChooser) && al_get_native_file_dialog_count(stateSaveChooser) > 0) {
            SaveState(AppendExtensionIfMissing(al_get_native_file_dialog_path(stateSaveChooser, 0), ".sav"), NULL);
            refreshPath(&userSavestatesPath, al_get_native_file_dialog_path(stateSaveChooser, 0));
          }
          al_destroy_native_file_dialog(stateSaveChooser);
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
        case SPEED_10_ID: CpuSpeed=10; goto setSpeed;
        case SPEED_20_ID: CpuSpeed=20; goto setSpeed;
        case SPEED_50_ID: CpuSpeed=50; goto setSpeed;
        case SPEED_100_ID: CpuSpeed=100; goto setSpeed;
        case SPEED_120_ID: CpuSpeed=120; goto setSpeed;
        case SPEED_200_ID: CpuSpeed=200; goto setSpeed;
        case SPEED_500_ID: CpuSpeed=500;
          setSpeed:
          Z80_IPeriod=(2500000*CpuSpeed)/(100*IFreq);
          UpdateCpuSpeedMenu();
          break;
        case FPS_50_ID: case FPS_60_ID:
          IFreq = (event.user.data1 == FPS_50_ID ? 50 : 60);
          if ((IFreq == 50 && CpuSpeed == 120) || (IFreq == 60 && CpuSpeed == 100)) {
            CpuSpeed = 2*IFreq;
            UpdateCpuSpeedMenu();
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
        case HARDWARE_T38_ID: RAMSizeKb=16; goto updateMem;
        case HARDWARE_T54_ID: RAMSizeKb=32; goto updateMem;
        case HARDWARE_T102_ID: RAMSizeKb=80; goto updateMem;
          updateMem:
          InitRAM();
          UpdateMemoryMenu();
          ColdBoot = 1;
          Z80_Reset();
          break;
        case OPTIONS_SOUND_ID:
          soundmode = !soundmode;
          break;
        case OPTIONS_VOLUME_HIGH_ID: mastervolume=10; goto updateVol;
        case OPTIONS_VOLUME_MEDIUM_ID: mastervolume=4; goto updateVol;
        case OPTIONS_VOLUME_LOW_ID: mastervolume=1; 
          updateVol:
          UpdateVolumeMenu();
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
        case OPTIONS_ENGLISH_ID:
        case OPTIONS_NEDERLANDS_ID:
          uilanguage = event.user.data1 - OPTIONS_ENGLISH_ID;
          al_set_menu_item_flags(menu, OPTIONS_ENGLISH_ID, uilanguage==0 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
          al_set_menu_item_flags(menu, OPTIONS_NEDERLANDS_ID, uilanguage==1 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
          CreateEmulatorMenu();
          ClearScreen();
          break;
        case HELP_ABOUT_ID:
          al_show_native_message_box(display,
            "M2000 - Philips P2000 emulator", "",
            _(HELP_ABOUT_MSG_ID),
            NULL, 0);
          break;
        case DISPLAY_SCANLINES:
          scanlines = !scanlines;
          ClearScreen();
          break;
        case DISPLAY_SMOOTHING:
          smoothing = !smoothing;
          ClearScreen();
          break;
        case DISPLAY_FULLSCREEN:
          ToggleFullscreen();
          break;
        case DISPLAY_WINDOW_640x480: case DISPLAY_WINDOW_960x720: case DISPLAY_WINDOW_1280x960: 
        case DISPLAY_WINDOW_1600x1200: case DISPLAY_WINDOW_1920x1440:
          videomode = event.user.data1 - DISPLAY_WINDOW_MENU;
          UpdateDisplaySettings();
          UpdateViewMenu();
          al_resize_display(display, DisplayWidth + 2* DisplayHBorder, DisplayHeight + 2*DisplayVBorder);
          ClearScreen();
          break;
      }
      if (Z80_Running && !delayedShiftedKeyPress) SaveConfig(); //auto save config
    }
  }

  // Ctrl-1           -  ZOEK key (show cassette index)
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && (al_key_up(&kbdstate, ALLEGRO_KEY_1) || al_key_up(&kbdstate, ALLEGRO_KEY_PAD_1))) {
    PushKey(72); //LSHIFT
    delayedShiftedKeyPress = 59;
  }
  // Ctrl-3           -  START key (start loaded program)
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && (al_key_up(&kbdstate, ALLEGRO_KEY_3) || al_key_up(&kbdstate, ALLEGRO_KEY_PAD_3))) {
    PushKey(72); //LSHIFT
    delayedShiftedKeyPress = 56;
  }
  // Ctrl-.           -  STOP key (pause/halt program)
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && (al_key_up(&kbdstate, ALLEGRO_KEY_FULLSTOP) || al_key_up(&kbdstate, ALLEGRO_KEY_PAD_DELETE))) {
    PushKey(72); //LSHIFT
    delayedShiftedKeyPress = 16;
  }
  // Ctrl-7           -  WIS key (clear cassette)
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && (al_key_up(&kbdstate, ALLEGRO_KEY_7) || al_key_up(&kbdstate, ALLEGRO_KEY_PAD_7))) {
    PushKey(72); //LSHIFT
    delayedShiftedKeyPress = 51;
  }

  // Ctrl-I           -  Insert cassette dialog
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_I) && al_key_up(&kbdstate, ALLEGRO_KEY_LCTRL))
    OpenCassetteDialog(false);

  // Ctrl-O           -  Open/Boot Cassette dialog
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_O) && al_key_up(&kbdstate, ALLEGRO_KEY_LCTRL))
    OpenCassetteDialog(true);

  // Ctrl-E           -  Eject current cassette
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_E)) {
    al_destroy_path(currentTapePath);
    currentTapePath = NULL;
    RemoveCassette();
    UpdateWindowTitle();
  }

  // Ctrl-R           -  Reset P2000
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_R))
    Z80_Reset();

  // Ctrl-Enter       -  Toggle fullscreen on/off (not supported on Linux)
  if ((al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) || al_key_down(&kbdstate, ALLEGRO_KEY_ALT)) && al_key_up(&kbdstate, ALLEGRO_KEY_ENTER))
    ToggleFullscreen();

  // Ctrl-L           -  Toggle scanlines on/off
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_L)) {
    scanlines = !scanlines;
    ClearScreen();
  }

  // Ctrl-Q           -  Quit emulator
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_Q))
    Z80_Running = 0;

  // Ctrl-C           -  Save current state to file (without dialog)
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_C)) {
    SaveState(NULL, userSavestatesPath);
    IndicateActionDone();
  }

  // Ctrl-V           -  Load previously saved state (without dialog)
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_V))
    LoadState(NULL, userSavestatesPath);

  // Ctrl-S           -  Save screenshot to file (without dialog)
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_S)) {
    static char extension[25];
    time_t now = time(NULL);
    strftime(extension, 25, " %Y-%m-%d %H-%M-%S.png", localtime(&now));
    al_set_path_filename(userScreenshotsPath, currentTapePath ? al_get_path_filename(currentTapePath) : "Screenshot");
    al_set_path_extension(userScreenshotsPath, extension);
    al_save_bitmap(al_path_cstr(userScreenshotsPath, PATH_SEPARATOR), al_get_target_bitmap());
    al_set_path_filename(userScreenshotsPath, NULL);
    IndicateActionDone();
  }

  // Ctrl-D           -  Dump visible video RAM to file (without dialog)
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_D)) {
    static char extension[26];
    time_t now = time(NULL);
    strftime(extension, 26, " %Y-%m-%d %H-%M-%S.vram", localtime(&now));
    al_set_path_filename(userVideoRamDumpsPath, currentTapePath ? al_get_path_filename(currentTapePath) : "VideoRAM");
    al_set_path_extension(userVideoRamDumpsPath, extension);
    SaveVideoRAM(al_path_cstr(userVideoRamDumpsPath, PATH_SEPARATOR));
    al_set_path_filename(userVideoRamDumpsPath, NULL);
    IndicateActionDone();
  }
  
  // Ctrl-P           -  Toggle pause on/off
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_P)) {
    pausePressed = !pausePressed;
    al_set_menu_item_flags(menu, SPEED_PAUSE, pausePressed ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  }

  // Ctrl-M           -  Toggle mute/unmute
  if (al_key_down(&kbdstate, ALLEGRO_KEY_LCTRL) && al_key_up(&kbdstate, ALLEGRO_KEY_M)) {
    soundmode = !soundmode;
    al_set_menu_item_flags(menu, OPTIONS_SOUND_ID, soundmode ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  }

  /* press F5 to enable trace in DEBUG mode */
  if (Debug && al_key_up(&kbdstate, ALLEGRO_KEY_F5)) {
    Z80_Trace = 1;
    pausePressed = 1;
  }

  // handle joystick
  if (joymode && joyDetected) {
    al_get_joystick_state(joystick, &joyState);
    for (i = 0; i < 5; i++) { // 4 directions and 1 button
      if ((i < 4 && joyState.stick[0].axis[i%2]*(-2.0*(i/2)+1.0) > 0.5) ||
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
void Pause(int ms) 
{
  al_rest((double)ms / 1000.0);
}

void DrawTileScanlines(int tileX, int tileY) 
{
  float i;
  float vpixel = DisplayTileHeight/30.0;
  ALLEGRO_COLOR evenLineColor = al_map_rgba(0, 0, 0, 50);
  ALLEGRO_COLOR scanlineColor = al_map_rgba(0, 0, 0, 120);
  for (i=DisplayVBorder+tileY*DisplayTileHeight; i<DisplayVBorder+(tileY+1)*DisplayTileHeight; i+=3.0*vpixel)
  {
    al_draw_line(DisplayHBorder + tileX*DisplayTileWidth, i+0.45*vpixel, DisplayHBorder + (tileX+1)*DisplayTileWidth, i+0.45*vpixel, scanlineColor, vpixel);
    al_draw_line(DisplayHBorder + tileX*DisplayTileWidth, i+2.45*vpixel, DisplayHBorder + (tileX+1)*DisplayTileWidth, i+2.45*vpixel, evenLineColor, vpixel);
  }
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
  al_draw_scaled_bitmap(
    eor ? (smoothing ? smFontBuf_bk : FontBuf_bk) : (smoothing ? smFontBuf : FontBuf), 
    c * (CHAR_TILE_WIDTH + CHAR_TILE_SPACE), 0.0, CHAR_TILE_WIDTH, CHAR_TILE_HEIGHT, 
    DisplayHBorder + 0.5 * x * DisplayTileWidth, DisplayVBorder + y * DisplayTileHeight, 
    0.5 * DisplayTileWidth, DisplayTileHeight, 0);
  if (ul)
    al_draw_filled_rectangle(
      DisplayHBorder + 0.5 * x * DisplayTileWidth, DisplayVBorder + (y + 1) * DisplayTileHeight - 2.0, 
      DisplayHBorder + 0.5 * (x + 1) * DisplayTileWidth, DisplayVBorder + (y + 1) * DisplayTileHeight - 1.0, 
      al_map_rgb(255, 255, 255));
  if (scanlines) DrawTileScanlines(x, y);
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
  al_draw_tinted_scaled_bitmap(
    smoothing ? smFontBuf : FontBuf,
    al_map_rgba(Pal[fg * 3], Pal[fg * 3 + 1], Pal[fg * 3 + 2], 255), 
    c * (CHAR_TILE_WIDTH + CHAR_TILE_SPACE), (si >> 1) * CHAR_TILE_HEIGHT/2, 
    CHAR_TILE_WIDTH, si ? CHAR_TILE_HEIGHT/2 : CHAR_TILE_HEIGHT,
    DisplayHBorder + x * DisplayTileWidth, DisplayVBorder + y * DisplayTileHeight,
    DisplayTileWidth, DisplayTileHeight, 0);
  if (bg)
    al_draw_tinted_scaled_bitmap(
      smoothing ? smFontBuf_bk : FontBuf_bk,
      al_map_rgba(Pal[bg * 3], Pal[bg * 3 + 1], Pal[bg * 3 + 2], 0), 
      c * (CHAR_TILE_WIDTH + CHAR_TILE_SPACE), (si >> 1) * CHAR_TILE_HEIGHT/2, 
      CHAR_TILE_WIDTH, si ? CHAR_TILE_HEIGHT/2 : CHAR_TILE_HEIGHT,
      DisplayHBorder + x * DisplayTileWidth, DisplayVBorder + y * DisplayTileHeight,
      DisplayTileWidth, DisplayTileHeight, 0);
  if (scanlines) DrawTileScanlines(x, y);
}

char *GetResourcesPath() 
{
  strcpy (ProgramPath, al_path_cstr(al_get_standard_path(ALLEGRO_RESOURCES_PATH), PATH_SEPARATOR));

  // debian install check
  if (!strcmp(ProgramPath,"/usr/bin/"))
    strcpy(ProgramPath, "/usr/share/M2000/");

  return ProgramPath;
}

char *GetDocumentsPath() 
{
  strcpy(DocumentPath, GetResourcesPath()); //fallback to program path
  ALLEGRO_PATH *docPath = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
  if (docPath) {
    al_append_path_component(docPath, "M2000");
    if (al_make_directory(al_path_cstr(docPath, PATH_SEPARATOR)))
      strcpy(DocumentPath, al_path_cstr(docPath, PATH_SEPARATOR));
    al_destroy_path(docPath);
  }
  return DocumentPath;
}

int main(int argc,char *argv[]) 
{
  if (!al_init()) { ShowErrorMessage("Allegro could not initialize its core."); return EXIT_FAILURE; }
  if (!al_init_primitives_addon()) { ShowErrorMessage("Allegro could not initialize primitives addon."); return EXIT_FAILURE; }
  if (!al_init_image_addon()) { ShowErrorMessage("Allegro could not initialize image addon."); return EXIT_FAILURE; }
  if (!al_init_native_dialog_addon()) { ShowErrorMessage("Allegro could not initialize native dialog addon."); return EXIT_FAILURE; }
  if (!al_install_keyboard()) { ShowErrorMessage("Allegro could not install keyboard."); return EXIT_FAILURE; }

  /* create M2000 folder inside user's Documents folder */
  docPath = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
  al_append_path_component(docPath, "M2000");
  if (!al_make_directory(al_path_cstr(docPath, PATH_SEPARATOR))) { 
    ShowErrorMessage("Can't create documents folder %s.", al_path_cstr(docPath, PATH_SEPARATOR)); 
    return EXIT_FAILURE; 
  }

  InitConfig();
  if (Verbose) {
    uint32_t version = al_get_allegro_version();
    printf("Using Allegro libs version %i.%i.%i\n",version >> 24, (version >> 16) & 255, (version >> 8) & 255);
  }
  return M2000_main(argc, argv);
}
