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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro5/allegro_native_dialog.h>
#include "Main.h"
#include "UIstrings.h"

ALLEGRO_MENU *menu = NULL;

void UpdateViewMenu() {
  int i;
  for (i=1; i< sizeof(Displays)/sizeof(*Displays); i++)
    al_set_menu_item_flags(menu, i + DISPLAY_WINDOW_MENU, videomode == i ?  ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
}

void UpdateVolumeMenu () {
  al_set_menu_item_flags(menu, OPTIONS_VOLUME_HIGH_ID, mastervolume == 10 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  al_set_menu_item_flags(menu, OPTIONS_VOLUME_MEDIUM_ID, mastervolume = 4 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  al_set_menu_item_flags(menu, OPTIONS_VOLUME_LOW_ID, mastervolume == 1 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
}

void UpdateCpuSpeedMenu () {
  al_set_menu_item_flags(menu, SPEED_500_ID, CpuSpeed == 500 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  al_set_menu_item_flags(menu, SPEED_200_ID,  CpuSpeed == 200 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  al_set_menu_item_flags(menu, SPEED_120_ID,  CpuSpeed == 120 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  al_set_menu_item_flags(menu, SPEED_100_ID,  CpuSpeed == 100 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  al_set_menu_item_flags(menu, SPEED_50_ID,  CpuSpeed == 50 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  al_set_menu_item_flags(menu, SPEED_20_ID,  CpuSpeed == 20 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
  al_set_menu_item_flags(menu, SPEED_10_ID,  CpuSpeed == 10 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX);
}

void CreateEmulatorMenu() 
{
  ALLEGRO_MENU_INFO menu_info[] = {
    { _(FILE_MENU_ID), 0, 0, NULL },
      { _(FILE_INSERT_CASSETTE_ID), FILE_INSERT_CASSETTE_ID, 0, NULL },
      { _(FILE_INSERTRUN_CASSETTE_ID), FILE_INSERTRUN_CASSETTE_ID, 0, NULL },
      { _(FILE_REMOVE_CASSETTE_ID), FILE_REMOVE_CASSETTE_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { _(FILE_INSERT_CARTRIDGE_ID), FILE_INSERT_CARTRIDGE_ID, 0, NULL },
      { _(FILE_REMOVE_CARTRIDGE_ID), FILE_REMOVE_CARTRIDGE_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "Reset (F5)", FILE_RESET_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { _(FILE_SAVE_STATE_ID), FILE_SAVE_STATE_ID, 0, NULL },
      { _(FILE_LOAD_STATE_ID), FILE_LOAD_STATE_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { _(FILE_SAVE_SCREENSHOT_ID), FILE_SAVE_SCREENSHOT_ID, 0, NULL },
      { _(FILE_LOAD_VIDEORAM_ID), FILE_LOAD_VIDEORAM_ID, 0, NULL },
      { _(FILE_SAVE_VIDEORAM_ID), FILE_SAVE_VIDEORAM_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { _(FILE_EXIT_ID), FILE_EXIT_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,

    { _(DISPLAY_WINDOW_MENU), DISPLAY_WINDOW_MENU, 0, NULL },
      { _(DISPLAY_SCANLINES), DISPLAY_SCANLINES, scanlines ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { _(DISPLAY_SMOOTHING), DISPLAY_SMOOTHING, smoothing ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { "640 x 480", DISPLAY_WINDOW_640x480, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "960 x 720", DISPLAY_WINDOW_960x720, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "1280 x 960", DISPLAY_WINDOW_1280x960, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "1600 x 1200", DISPLAY_WINDOW_1600x1200, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "1920 x 1440", DISPLAY_WINDOW_1920x1440, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
#ifndef __linux__
      ALLEGRO_MENU_SEPARATOR,
#endif
#ifdef _WIN32
      { _(DISPLAY_FULLSCREEN), DISPLAY_FULLSCREEN, 0, NULL },
#endif
#ifdef __APPLE__
      { _(DISPLAY_FULLSCREEN_APPLE), DISPLAY_FULLSCREEN, 0, NULL },
#endif
      ALLEGRO_END_OF_MENU,

    { _(SPEED_MENU_ID), SPEED_MENU_ID, 0, NULL },
      { _(SPEED_CPU_MENU_ID), SPEED_CPU_MENU_ID, 0, NULL },
        { "500%", SPEED_500_ID, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        { "200%", SPEED_200_ID, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        { "120%", SPEED_120_ID, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        { "100%", SPEED_100_ID, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        { "50%",  SPEED_50_ID , ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        { "20%",  SPEED_20_ID , ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        { "10%",  SPEED_10_ID , ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        ALLEGRO_END_OF_MENU,
      ALLEGRO_MENU_SEPARATOR,
      { "50 Hz", FPS_50_ID, IFreq == 50 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      { "60 Hz", FPS_60_ID, IFreq == 60 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { _(SPEED_SYNC), SPEED_SYNC, Sync ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_MENU_SEPARATOR,
      { _(SPEED_PAUSE), SPEED_PAUSE, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_END_OF_MENU,

    { _(KEYBOARD_MENU_ID), KEYBOARD_MENU_ID, 0, NULL },
      {_(KEYBOARD_ZOEK_ID), KEYBOARD_ZOEK_ID, 0, NULL },
      {_(KEYBOARD_START_ID), KEYBOARD_START_ID, 0, NULL },
      {_(KEYBOARD_STOP_ID), KEYBOARD_STOP_ID, 0, NULL },
      {_(KEYBOARD_CLEARCAS_ID), KEYBOARD_CLEARCAS_ID, 0, NULL },
      ALLEGRO_MENU_SEPARATOR,
      {_(KEYBOARD_SYMBOLIC_ID), KEYBOARD_SYMBOLIC_ID, keyboardmap==1 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      {_(KEYBOARD_POSITIONAL_ID), KEYBOARD_POSITIONAL_ID, keyboardmap==0 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
      ALLEGRO_END_OF_MENU,

    { _(OPTIONS_MENU_ID), OPTIONS_MENU_ID, 0, NULL },
      {soundDetected ? _(OPTIONS_SOUND_ID) : _(OPTIONS_SOUND_NOT_DETECTED_ID), OPTIONS_SOUND_ID, soundDetected ? (soundmode ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX) : ALLEGRO_MENU_ITEM_DISABLED, NULL },
      { _(OPTIONS_VOLUME_MENU_ID), OPTIONS_VOLUME_MENU_ID, 0, NULL },
        { _(OPTIONS_VOLUME_HIGH_ID), OPTIONS_VOLUME_HIGH_ID, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        { _(OPTIONS_VOLUME_MEDIUM_ID), OPTIONS_VOLUME_MEDIUM_ID, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        { _(OPTIONS_VOLUME_LOW_ID), OPTIONS_VOLUME_LOW_ID, ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        ALLEGRO_END_OF_MENU,
      ALLEGRO_MENU_SEPARATOR,
      {joyDetected ? _(OPTIONS_JOYSTICK_ID) : _(OPTIONS_JOYSTICK_NOT_DETECTED_ID), OPTIONS_JOYSTICK_ID, joyDetected ? (joymode ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX) : ALLEGRO_MENU_ITEM_DISABLED, NULL },
      { _(OPTIONS_JOYSTICK_MAP), OPTIONS_JOYSTICK_MAP, 0, NULL },
        { _(OPTIONS_JOYSTICK_MAP_0_ID), OPTIONS_JOYSTICK_MAP_0_ID, joymode ? (joymap==0 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX) : ALLEGRO_MENU_ITEM_DISABLED, NULL },
        { _(OPTIONS_JOYSTICK_MAP_1_ID), OPTIONS_JOYSTICK_MAP_1_ID, joymode ? (joymap==1 ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX) : ALLEGRO_MENU_ITEM_DISABLED, NULL },
        ALLEGRO_END_OF_MENU,
      ALLEGRO_MENU_SEPARATOR,
      { "Language / Taal->", OPTIONS_LANGUAGE_MENU_ID, 0, NULL },
        { "English", OPTIONS_ENGLISH_ID, uilanguage ? ALLEGRO_MENU_ITEM_CHECKBOX : ALLEGRO_MENU_ITEM_CHECKED, NULL },
        { "Nederlands", OPTIONS_NEDERLANDS_ID, uilanguage ? ALLEGRO_MENU_ITEM_CHECKED : ALLEGRO_MENU_ITEM_CHECKBOX, NULL },
        ALLEGRO_END_OF_MENU,
      ALLEGRO_MENU_SEPARATOR,
      { _(OPTIONS_SAVE_PREFERENCES), OPTIONS_SAVE_PREFERENCES, 0, NULL },
      ALLEGRO_END_OF_MENU,

    { _(HELP_MENU_ID), HELP_MENU_ID, 0, NULL },
      { _(HELP_ABOUT_ID), HELP_ABOUT_ID, 0, NULL },
      ALLEGRO_END_OF_MENU,
    ALLEGRO_END_OF_MENU
  };

  if (menu) { 
    al_remove_display_menu(display);
    al_destroy_menu(menu);
  }
  menu = al_build_menu(menu_info);
  if (!joyDetected) al_remove_menu_item(menu, OPTIONS_JOYSTICK_MAP);
  UpdateVolumeMenu();
  UpdateCpuSpeedMenu();
  UpdateViewMenu();
  al_set_display_menu(display, menu);
}