#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro5/allegro_native_dialog.h>

#define FILE_EXIT_ID 1
#define FILE_INSERT_CASSETTE_ID 2
#define FILE_INSERTRUN_CASSETTE_ID 3
#define FILE_REMOVE_CASSETTE_ID 4
#define FILE_INSERT_CARTRIDGE_ID 5
#define FILE_REMOVE_CARTRIDGE_ID 6
#define FILE_RESET_ID 7
#define FILE_SAVE_SCREENSHOT_ID 8
#define FILE_LOAD_VIDEORAM_ID 9
#define FILE_SAVE_VIDEORAM_ID 10

#define VIEW_SHOW_SCANLINES_ID 20
#define VIEW_WINDOW_SIZE_960_720 21

#define KEYBOARD_SYMBOLIC_ID 30
#define KEYBOARD_POSITIONAL_ID 31
#define KEYBOARD_ZOEK_ID 32
#define KEYBOARD_START_ID 33
#define KEYBOARD_STOP_ID 34

#define OPTIONS_SOUND_ID 40
#define OPTIONS_JOYSTICK_ID 41
#define OPTIONS_JOYSTICK_MAP_0_ID 42
#define OPTIONS_JOYSTICK_MAP_1_ID 43
#define OPTIONS_JOYSTICK_MAP 44

#define SPEED_SYNC 50
#define SPEED_500_ID 1500
#define SPEED_200_ID 1200
#define SPEED_100_ID 1100
#define SPEED_50_ID  1050
#define SPEED_20_ID  1020
#define SPEED_10_ID  1010

#define HELP_ABOUT_ID 100

ALLEGRO_MENU * CreateEmulatorMenu(ALLEGRO_DISPLAY *display, 
  int videomode,
  int keyboardmap,
  int soundmode,
  int joymode,
  int joymap,
  int cpuSpeed,
  int Sync
) {
  ALLEGRO_MENU_INFO menu_info[] = {
    ALLEGRO_START_OF_MENU("File", 0),
      { "Insert Cassette...", FILE_INSERT_CASSETTE_ID, 0, NULL },
      { "Insert, Load and Run Cassette...", FILE_INSERTRUN_CASSETTE_ID, 0, NULL },
      { "Remove Cassette...", FILE_REMOVE_CASSETTE_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "Insert Cartridge...", FILE_INSERT_CARTRIDGE_ID, 0, NULL },
      { "Remove Cartridge...", FILE_REMOVE_CARTRIDGE_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "Reset", FILE_RESET_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "Save Screenshot...", FILE_SAVE_SCREENSHOT_ID, 0, NULL },
      { "Load Video RAM...", FILE_LOAD_VIDEORAM_ID, 0, NULL },
      { "Save Video RAM...", FILE_SAVE_VIDEORAM_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "Exit", FILE_EXIT_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,

    ALLEGRO_START_OF_MENU("View", 0),
      { "Show Scanlines", VIEW_SHOW_SCANLINES_ID, videomode ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_MENU_SEPARATOR,
      ALLEGRO_START_OF_MENU("Window Sizes", 0),
        { "960x720", VIEW_WINDOW_SIZE_960_720, 0, NULL },
        ALLEGRO_END_OF_MENU,
      ALLEGRO_END_OF_MENU,

    ALLEGRO_START_OF_MENU("CPU Speed", 0),
      { "500%", SPEED_500_ID, cpuSpeed == 500 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "200%", SPEED_200_ID, cpuSpeed == 200 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "100%", SPEED_100_ID, cpuSpeed == 100 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "50%",  SPEED_50_ID , cpuSpeed == 50 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "20%",  SPEED_20_ID , cpuSpeed == 20 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "10%",  SPEED_10_ID , cpuSpeed == 10 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "Sync On/Off", SPEED_SYNC, Sync ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_END_OF_MENU,

    ALLEGRO_START_OF_MENU("Keyboard", 0),
      {"[ZOEK] - show cassette index", KEYBOARD_ZOEK_ID, 0, NULL },
      {"[START] - start loaded program", KEYBOARD_START_ID, 0, NULL },
      {"[STOP] - pause/halt program", KEYBOARD_STOP_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      {"Symbolic Key Mapping", KEYBOARD_SYMBOLIC_ID, keyboardmap==1 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      {"Positional Key Mapping", KEYBOARD_POSITIONAL_ID, keyboardmap==0 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_END_OF_MENU,

    ALLEGRO_START_OF_MENU("Options", 0),
      {"Sound On/Off", OPTIONS_SOUND_ID, soundmode ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_DISABLED, NULL },
      ALLEGRO_MENU_SEPARATOR,
      {joymode ? "Joystick On/Off" : "Joystick Not Detected", OPTIONS_JOYSTICK_ID, joymode ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_DISABLED, NULL },
      ALLEGRO_START_OF_MENU("Joystick Mapping", OPTIONS_JOYSTICK_MAP),
        { "Joystick emulates cursorkeys + spacebar", OPTIONS_JOYSTICK_MAP_0_ID, joymode ? (joymap==0 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX) : ALLEGRO_MENU_ITEM_DISABLED, NULL },
        { "Fraxxon mode: left/right emulates cursorkeys left/up", OPTIONS_JOYSTICK_MAP_1_ID, joymode ? (joymap==1 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX) : ALLEGRO_MENU_ITEM_DISABLED, NULL },
        ALLEGRO_END_OF_MENU,
      ALLEGRO_END_OF_MENU,

    ALLEGRO_START_OF_MENU("Help", 0),
      {"About M2000", HELP_ABOUT_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,
    ALLEGRO_END_OF_MENU
  };

  ALLEGRO_MENU *menu = al_build_menu(menu_info);
  if (!joymode) al_remove_menu_item(menu, OPTIONS_JOYSTICK_MAP);
  al_set_display_menu(display, menu);
  return menu;
}