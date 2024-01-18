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
#include <allegro5/allegro.h>
#include "../Z80.h"

#define CONFIG_FILENAME "M2000.cfg"
#define SUBDIR_CASSETTES "Cassettes"
#define SUBDIR_CARTRIDGES "Cartridges"
#define SUBDIR_SCREENSHOTS "Screenshots"
#define SUBDIR_VIDEORAMDUMPS "VideoRAM dumps"
#define SUBDIR_SAVESTATES "Savestates"

void ParseConfig() 
{
  if (!config) return;
  P2000_Mode      = strcmp(al_get_config_value(config, "Hardware",  "model"), "T") == 0 ? 0 : 1;
  RAMSizeKb       = atoi(al_get_config_value(config, "Hardware",  "ram"));
  TapeBootEnabled = strcmp(al_get_config_value(config, "Hardware",  "boot"), "on") == 0;
  PrnType         = atoi(al_get_config_value(config, "Hardware",  "printertype"));
  ROMName         =      al_get_config_value(config, "Hardware",  "romfile");
  FontName        =      al_get_config_value(config, "Hardware",  "font");

  TapeName        =      al_get_config_value(config, "File",      "tape");
  CartName        =      al_get_config_value(config, "File",      "cart");
  PrnName         =      al_get_config_value(config, "File",      "printer");
  userCassettesPath = al_create_path_for_directory(al_get_config_value(config, "File", "cassettes"));
  userCartridgesPath = al_create_path_for_directory(al_get_config_value(config, "File", "cartridges"));
  userScreenshotsPath = al_create_path_for_directory(al_get_config_value(config, "File", "screenshots"));
  userVideoRamDumpsPath = al_create_path_for_directory(al_get_config_value(config, "File", "videoramdumps"));
  userSavestatesPath = al_create_path_for_directory(al_get_config_value(config, "File", "savestates"));

  IFreq           = atoi(al_get_config_value(config, "Speed",     "ifreq"));
  CpuSpeed        = atoi(al_get_config_value(config, "Speed",     "cpuspeed"));
  Sync            = strcmp(al_get_config_value(config, "Speed",   "sync"), "on") == 0;
  UPeriod         = atoi(al_get_config_value(config, "Speed",     "uperiod"));

  videomode       = atoi(al_get_config_value(config, "Display",   "video"));
  scanlines       = strcmp(al_get_config_value(config, "Display", "scanlines"), "on") == 0;
  smoothing       = strcmp(al_get_config_value(config, "Display", "smoothing"), "on") == 0;

  keyboardmap     = atoi(al_get_config_value(config, "Keyboard",   "keymap"));

  soundmode       = strcmp(al_get_config_value(config, "Options", "sound"), "on") == 0;
  mastervolume    = atoi(al_get_config_value(config, "Options",   "volume"));
  audiofilter     = atoi(al_get_config_value(config, "Options",   "audiofilter"));
  joymode         = strcmp(al_get_config_value(config, "Options", "joystick"), "on") == 0;
  joymap          = atoi(al_get_config_value(config, "Options",   "joymap"));
  uilanguage      = strcmp(al_get_config_value(config, "Options", "language"), "EN") == 0 ? 0 : 1;

  Verbose         = atoi(al_get_config_value(config, "Debug",     "verbose"));
  Debug           = al_get_config_value(config, "Debug", "debug") ?
                      (strcmp(al_get_config_value(config, "Debug", "debug"), "on") == 0) : 0;

  if (Debug && al_get_config_value(config, "Debug", "trap"))
    sscanf(al_get_config_value(config, "Debug", "trap"), "%X", &Z80_Trap);
}

void InitConfig() 
{
  //requires docPath to be set already

  config = al_create_config();

  /* Hardware */
  al_add_config_comment(config, "Hardware",   "model=T|M             Set P2000 model [T]");
  al_add_config_comment(config, "Hardware",   "                      T - P2000T");
  al_add_config_comment(config, "Hardware",   "                      M - P2000M (experimental)");
  al_add_config_comment(config, "Hardware",   "ram=<value>           Set amount of RAM installed in kilobytes [32]");
  al_add_config_comment(config, "Hardware",   "boot=on|off           Allow/Don't allow BASIC to boot from tape [on]");
  al_add_config_comment(config, "Hardware",   "printertype=<type>    Set printer type [0]");
  al_add_config_comment(config, "Hardware",   "                      0 - Daisy wheel");
  al_add_config_comment(config, "Hardware",   "                      1 - Matrix");
  al_add_config_comment(config, "Hardware",   "romfile=<file>        Set P2000 ROM file [P2000ROM.bin]");
  al_add_config_comment(config, "Hardware",   "font=<filename>       Set SAA5050 font to use [Default.fnt]");
  al_set_config_value  (config, "Hardware",   "model", "T");
  al_set_config_value  (config, "Hardware",   "ram", "32");
  al_set_config_value  (config, "Hardware",   "boot", "on");
  al_set_config_value  (config, "Hardware",   "printertype", "0");
  al_set_config_value  (config, "Hardware",   "romfile", "P2000ROM.bin");
  al_set_config_value  (config, "Hardware",   "font", "Default.fnt");
  al_add_config_comment(config, "Hardware",   "");

  /* File */
  al_add_config_comment(config, "File",       "tape=<filename>       Set tape image to use [Default.cas]");
  al_add_config_comment(config, "File",       "cart=<filename>       Set cartridge image to use [BASIC.bin]");
  al_add_config_comment(config, "File",       "printer=<filename>    Set file for printer output [Printer.out]");
  al_add_config_comment(config, "File",       "cassettes=<path>      Set folder containing cassette files (.cas)");
  al_add_config_comment(config, "File",       "cartridges=<path>     Set folder containing cartridge files (.bin)");
  al_add_config_comment(config, "File",       "screenshots=<path>    Set folder to store the screenshot files (.bmp|.png)");
  al_add_config_comment(config, "File",       "videoramdumps=<path>  Set folder to store the video-RAM dump files (.vram)");
  al_add_config_comment(config, "File",       "savestates=<path>     Set folder to store the savestate files (.sav)");
  al_set_config_value  (config, "File",       "tape", "Default.cas");
  al_set_config_value  (config, "File",       "cart", "BASIC.bin");
  al_set_config_value  (config, "File",       "printer", "Printer.out");
  ALLEGRO_PATH * _docPath = al_clone_path(docPath);
  al_set_path_filename(_docPath, NULL);
  al_append_path_component(_docPath, SUBDIR_CASSETTES);
  al_set_config_value  (config, "File",       "cassettes", al_path_cstr(_docPath, PATH_SEPARATOR));
  al_replace_path_component(_docPath, -1, SUBDIR_CARTRIDGES);
  al_set_config_value  (config, "File",       "cartridges", al_path_cstr(_docPath, PATH_SEPARATOR));
  al_replace_path_component(_docPath, -1, SUBDIR_SCREENSHOTS);
  al_set_config_value  (config, "File",       "screenshots", al_path_cstr(_docPath, PATH_SEPARATOR));
  al_replace_path_component(_docPath, -1, SUBDIR_VIDEORAMDUMPS);
  al_set_config_value  (config, "File",       "videoramdumps", al_path_cstr(_docPath, PATH_SEPARATOR));
  al_replace_path_component(_docPath, -1, SUBDIR_SAVESTATES);
  al_set_config_value  (config, "File",       "savestates", al_path_cstr(_docPath, PATH_SEPARATOR));
  al_add_config_comment(config, "File",       "");
  al_destroy_path(_docPath);

  /* Speed */
  al_add_config_comment(config, "Speed",      "ifreq=<frequency>     Set interrupt frequency [50] Hz");
  al_add_config_comment(config, "Speed",      "cpuspeed=<speed>      Set Z80 CPU speed [100]%");
  al_add_config_comment(config, "Speed",      "sync=on|off           Sync/Do not sync emulation [on]");
  al_add_config_comment(config, "Speed",      "                      Emulation will be too fast if sync is turned off");
  al_add_config_comment(config, "Speed",      "uperiod=<value>       Number of interrupts per screen update [1]");
  al_add_config_comment(config, "Speed",      "                      Try uperiod 2 or uperiod 3 if emulation is a bit slow");
  al_set_config_value  (config, "Speed",      "ifreq", "50");
  al_set_config_value  (config, "Speed",      "cpuspeed", "100");
  al_set_config_value  (config, "Speed",      "sync", "on");
  al_set_config_value  (config, "Speed",      "uperiod", "1");
  al_add_config_comment(config, "Speed",      "");
  
  /* Display */
  al_add_config_comment(config, "Display",    "video=<mode>          Set video mode/window size [0]");
  al_add_config_comment(config, "Display",    "                      0  - Autodetect best window size");
  al_add_config_comment(config, "Display",    "                      1  - 640x480");
  al_add_config_comment(config, "Display",    "                      2  - 960x720");
  al_add_config_comment(config, "Display",    "                      3  - 1280x960");
  al_add_config_comment(config, "Display",    "                      4  - 1600x1200");
  al_add_config_comment(config, "Display",    "                      5  - 1920x1440");
  al_add_config_comment(config, "Display",    "                      99 - Full Screen (not supported on Linux)");
  al_add_config_comment(config, "Display",    "scanlines=on|off      Show/Do not show scanlines [off]");
  al_add_config_comment(config, "Display",    "smoothing=on|off      Use display smoothing [on]");
  al_set_config_value  (config, "Display",    "video", "0");
  al_set_config_value  (config, "Display",    "scanlines", "off");
  al_set_config_value  (config, "Display",    "smoothing", "on");
  al_add_config_comment(config, "Display",    "");

  /* Keyboard */
  al_add_config_comment(config, "Keyboard",   "keymap <mode>         Set keyboard mapping [1]");
  al_add_config_comment(config, "Keyboard",   "                      0 - Positional mapping");
  al_add_config_comment(config, "Keyboard",   "                      1 - Symbolic mapping");
  al_set_config_value  (config, "Keyboard",   "keymap", "1");
  al_add_config_comment(config, "Keyboard",   "");

  /* Options */
  al_add_config_comment(config, "Options",    "sound=on|off          Set sound mode [on]");
  al_add_config_comment(config, "Options",    "volume=<value>        Set initial sound volume");
  al_add_config_comment(config, "Options",    "                      0 - Silent");
  al_add_config_comment(config, "Options",    "                      ...");
  al_add_config_comment(config, "Options",    "                      15 - Maximum");
  al_add_config_comment(config, "Options",    "audiofilter=<mode>    Set audio filter mode [2]");
  al_add_config_comment(config, "Options",    "                      0 - No filter");
  al_add_config_comment(config, "Options",    "                      1 - Normal filter");
  al_add_config_comment(config, "Options",    "                      2 - Heavy filter");
  al_add_config_comment(config, "Options",    "joystick=on|off       Set joystick mode [on]");
  al_add_config_comment(config, "Options",    "joymap=<mode>         Set joystick mapping [0]");
  al_add_config_comment(config, "Options",    "                      0 - Moving the joystick emulates cursorkey presses");
  al_add_config_comment(config, "Options",    "                          The main button emulates pressing the spacebar");
  al_add_config_comment(config, "Options",    "                      1 - Fraxxon mode: Left/right emulates cursorkeys left/up");
  al_add_config_comment(config, "Options",    "                          The main button emulates pressing the spacebar");
  al_add_config_comment(config, "Options",    "language=EN|NL        Set UI language [EN]");
  al_set_config_value  (config, "Options",    "sound", "on");
  al_set_config_value  (config, "Options",    "volume", "4");
  al_set_config_value  (config, "Options",    "audiofilter", "1");

  al_set_config_value  (config, "Options",    "joystick", "on");
  al_set_config_value  (config, "Options",    "joymap", "0");
  al_set_config_value  (config, "Options",    "language", "EN");
  al_add_config_comment(config, "Options",    "");

  /* Debug */
  al_add_config_comment(config, "Debug",      "verbose=<level>       Set debugging messages [0]");
  al_add_config_comment(config, "Debug",      "                      0 - Silent");    
  al_add_config_comment(config, "Debug",      "                      1 - Debug messages");
  al_add_config_comment(config, "Debug",      "                      4 - Debug and Tape messages");
  al_add_config_comment(config, "Debug",      "debug=on|off          Set debugging mode [off]");
  al_add_config_comment(config, "Debug",      "trap=<address>        Trap execution when PC reaches specified address [-1]");
  al_set_config_value  (config, "Debug",      "verbose", "0");

  al_set_path_filename(docPath, CONFIG_FILENAME);
  ALLEGRO_CONFIG *add = al_load_config_file(al_path_cstr(docPath, PATH_SEPARATOR));
  if (add)
    al_merge_config_into(config, add);
  al_save_config_file(al_path_cstr(docPath, PATH_SEPARATOR), config);

  al_set_path_filename(docPath, NULL);
  ParseConfig();
}

void SaveConfig() 
{
  static char intstr[4];

  al_set_config_value(config, "File", "cassettes", al_path_cstr(userCassettesPath, PATH_SEPARATOR));
  al_set_config_value(config, "File", "cartridges", al_path_cstr(userCartridgesPath, PATH_SEPARATOR));
  al_set_config_value(config, "File", "screenshots", al_path_cstr(userScreenshotsPath, PATH_SEPARATOR));
  al_set_config_value(config, "File", "videoramdumps", al_path_cstr(userVideoRamDumpsPath, PATH_SEPARATOR));
  al_set_config_value(config, "File", "savestates", al_path_cstr(userSavestatesPath, PATH_SEPARATOR));

  if (sprintf(intstr, "%i", IFreq))         al_set_config_value(config, "Speed", "ifreq", intstr);
  if (sprintf(intstr, "%i", CpuSpeed))      al_set_config_value(config, "Speed", "cpuspeed", intstr);
  al_set_config_value(config, "Speed", "sync", Sync ? "on" : "off");
  if (sprintf(intstr, "%i", UPeriod))       al_set_config_value(config, "Speed", "uperiod", intstr);

  if (sprintf(intstr, "%i", optimalVideomode == videomode ? 0 : videomode)) al_set_config_value(config, "Display", "video", intstr);
  al_set_config_value(config, "Display", "scanlines", scanlines? "on" : "off");
  al_set_config_value(config, "Display", "smoothing", smoothing? "on" : "off");
  
  if (sprintf(intstr, "%i", keyboardmap))   al_set_config_value(config, "Keyboard", "keymap", intstr);

  if (sprintf(intstr, "%i", RAMSizeKb))     al_set_config_value(config, "Hardware", "ram", intstr);

  al_set_config_value(config, "Options", "sound", soundmode ? "on" : "off");
  if (sprintf(intstr, "%i", mastervolume))  al_set_config_value(config, "Options", "volume", intstr);
  if (sprintf(intstr, "%i", audiofilter))   al_set_config_value(config, "Options", "audiofilter", intstr);
  al_set_config_value(config, "Options", "joystick", joymode ? "on" : "off");
  if (sprintf(intstr, "%i", joymap))        al_set_config_value(config, "Options", "joymap", intstr);
  al_set_config_value(config, "Options", "language", uilanguage ? "NL" : "EN");

  al_set_path_filename(docPath, CONFIG_FILENAME);
  al_save_config_file(al_path_cstr(docPath, PATH_SEPARATOR), config);
}