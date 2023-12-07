#include <stdio.h>
#include <string.h>

typedef struct {
  const int key;
  const char* value;
} LanguageEntry;

#define FILE_EXIT_ID                      1
#define FILE_INSERT_CASSETTE_ID           2
#define FILE_INSERTRUN_CASSETTE_ID        3
#define FILE_REMOVE_CASSETTE_ID           4
#define FILE_INSERT_CARTRIDGE_ID          5
#define FILE_REMOVE_CARTRIDGE_ID          6
#define FILE_RESET_ID                     7
#define FILE_SAVE_SCREENSHOT_ID           8
#define FILE_LOAD_VIDEORAM_ID             9
#define FILE_SAVE_VIDEORAM_ID             10
#define FILE_SAVE_STATE_ID                11
#define FILE_LOAD_STATE_ID                12
#define DISPLAY_WINDOW_MENU               20
#define DISPLAY_WINDOW_640x480            21
#define DISPLAY_WINDOW_800x600            22
#define DISPLAY_WINDOW_960x720            23
#define DISPLAY_WINDOW_1280x960           24
#define DISPLAY_WINDOW_1440x1080          25
#define DISPLAY_WINDOW_1600x1200          26
#define DISPLAY_WINDOW_1920x1440          27
#define DISPLAY_SCANLINES                 28
#define DISPLAY_FULLSCREEN                29
#define DISPLAY_SMOOTHING                 19
#define KEYBOARD_SYMBOLIC_ID              30
#define KEYBOARD_POSITIONAL_ID            31
#define KEYBOARD_ZOEK_ID                  32
#define KEYBOARD_START_ID                 33
#define KEYBOARD_STOP_ID                  34
#define KEYBOARD_CLEARCAS_ID              35
#define OPTIONS_SOUND_ID                  40
#define OPTIONS_VOLUME_HIGH_ID            41
#define OPTIONS_VOLUME_MEDIUM_ID          42
#define OPTIONS_VOLUME_LOW_ID             43
#define OPTIONS_JOYSTICK_ID               44
#define OPTIONS_JOYSTICK_MAP_0_ID         45
#define OPTIONS_JOYSTICK_MAP_1_ID         46
#define OPTIONS_JOYSTICK_MAP              47
#define OPTIONS_SAVE_PREFERENCES          48
#define OPTIONS_ENGLISH_ID                49
#define OPTIONS_NEDERLANDS_ID             39
#define SPEED_SYNC                        50
#define SPEED_PAUSE                       51
#define FPS_50_ID                         52
#define FPS_60_ID                         53
#define SPEED_10_ID                       54
#define SPEED_20_ID                       55
#define SPEED_50_ID                       56
#define SPEED_100_ID                      57
#define SPEED_120_ID                      58
#define SPEED_200_ID                      59
#define SPEED_500_ID                      60
#define HELP_ABOUT_ID                     61

#define FILE_MENU_ID                      62
#define HELP_MENU_ID                      63
#define HELP_ABOUT_MSG_ID                 64

static LanguageEntry ENstrings[] = {
  { FILE_MENU_ID, "File->" },
  { FILE_INSERT_CASSETTE_ID, "Insert Cassette..." },


  { OPTIONS_SAVE_PREFERENCES, "Save Preferences" },
  { HELP_MENU_ID, "Help->" },
  { HELP_ABOUT_ID, "About M2000" },
  { HELP_ABOUT_MSG_ID, "Thanks to Marcel de Kogel for creating the core of this emulator back in 1996."},
  { 0, NULL },
};

static LanguageEntry NLstrings[] = {
  { FILE_MENU_ID, "Bestand->" },
  { FILE_INSERT_CASSETTE_ID, "Invoeren Cassette..."},


  { OPTIONS_SAVE_PREFERENCES, "Bewaar voorkeuren" },
  { HELP_MENU_ID, "Hulp->" },
  { HELP_ABOUT_ID, "Over M2000" },
  { HELP_ABOUT_MSG_ID, "Met dank aan Marcel de Kogel voor het creëren van de basis voor deze emulator in 1996."},
  { 0, NULL },
};

const char* _(int key) {
  LanguageEntry *UIstrings = uilanguage == 1 ? NLstrings : ENstrings;
  int i=0;
  for (i=0; UIstrings[i].key; i++) {
    if (key == UIstrings[i].key) 
      return UIstrings[i].value;
  }
  return "[KEY NOT FOUND]";
}

const char* __(const char *  key) {
  return key;
}