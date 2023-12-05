#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro5/allegro.h>

#define CONFIG_FILENAME "M2000.cfg"

void ParseConfig() 
{
  if (!config) return;
  P2000_Mode      = atoi(al_get_config_value(config, "Hardware",  "model"));
  RAMSize         = atoi(al_get_config_value(config, "Hardware",  "ram"));
  TapeBootEnabled = atoi(al_get_config_value(config, "Hardware",  "boot"));
  PrnType         = atoi(al_get_config_value(config, "Hardware",  "printertype"));
  ROMName         =      al_get_config_value(config, "Hardware",  "romfile");
  FontName        =      al_get_config_value(config, "Hardware",  "font");

  TapeName        =      al_get_config_value(config, "File",      "tape");
  CartName        =      al_get_config_value(config, "File",      "cart");
  PrnName         =      al_get_config_value(config, "File",      "printer");

  IFreq           = atoi(al_get_config_value(config, "Speed",     "ifreq"));
  CpuSpeed        = atoi(al_get_config_value(config, "Speed",     "cpuspeed"));
  Sync            = atoi(al_get_config_value(config, "Speed",     "sync"));
  UPeriod         = atoi(al_get_config_value(config, "Speed",     "uperiod"));

  videomode       = atoi(al_get_config_value(config, "Display",   "video"));
  scanlines       = atoi(al_get_config_value(config, "Display",   "scanlines"));
  smoothing       = atoi(al_get_config_value(config, "Display",   "smoothing"));

  keyboardmap     = atoi(al_get_config_value(config, "Keyboard",   "keymap"));

  soundmode       = atoi(al_get_config_value(config, "Options",   "sound"));
  mastervolume    = atoi(al_get_config_value(config, "Options",   "volume"));
  joymode         = atoi(al_get_config_value(config, "Options",   "joystick"));
  joymap          = atoi(al_get_config_value(config, "Options",   "joymap"));

  Verbose         = atoi(al_get_config_value(config, "Debug",     "verbose"));
#ifdef DEBUG
  sscanf(al_get_config_value(config, "Debug", "trap"), "%X", &Z80_Trap);
#endif
}

void InitConfig(ALLEGRO_PATH * docPath) 
{
  al_set_path_filename(docPath, CONFIG_FILENAME);
  //al_remove_filename(al_path_cstr(docPath, PATH_SEPARATOR)); //REMOVE!

  if (!(config = al_load_config_file(al_path_cstr(docPath, PATH_SEPARATOR)))) {
    config = al_create_config();

    /* Hardware */
    al_add_config_comment(config, "Hardware",   "model=<type>          Select P2000 model [0]");
    al_add_config_comment(config, "Hardware",   "                      0 - P2000T");
    al_add_config_comment(config, "Hardware",   "                      1 - P2000M (experimental)");
    al_add_config_comment(config, "Hardware",   "ram=<value>           Select amount of RAM installed [32KB]");
    al_add_config_comment(config, "Hardware",   "boot=<mode>           Allow/Don't allow BASIC to boot from tape [1]");
    al_add_config_comment(config, "Hardware",   "                      0 - Don't allow booting");
    al_add_config_comment(config, "Hardware",   "                      1 - Allow booting");
    al_add_config_comment(config, "Hardware",   "printertype=<type>    Select printer type [0]");
    al_add_config_comment(config, "Hardware",   "                      0 - Daisy wheel");
    al_add_config_comment(config, "Hardware",   "                      1 - Matrix");
    al_add_config_comment(config, "Hardware",   "romfile=<file>        Select P2000 ROM file [P2000ROM.bin]");
    al_add_config_comment(config, "Hardware",   "font=<filename>       Select SAA5050 font to use [Default.fnt]");
    al_set_config_value  (config, "Hardware",   "model", "0");
    al_set_config_value  (config, "Hardware",   "ram", "32");
    al_set_config_value  (config, "Hardware",   "boot", "1");
    al_set_config_value  (config, "Hardware",   "printertype", "0");
    al_set_config_value  (config, "Hardware",   "romfile", "P2000ROM.bin");
    al_set_config_value  (config, "Hardware",   "font", "Default.fnt");
    al_add_config_comment(config, "Hardware",   NULL);

    /* File */
    al_add_config_comment(config, "File",       "tape=<filename>       Select tape image to use [Default.cas]");
    al_add_config_comment(config, "File",       "cart=<filename>       Select cartridge image to use [BASIC.bin]");
    al_add_config_comment(config, "File",       "printer=<filename>    Select file for printer output [Printer.out]");
    al_set_config_value  (config, "File",       "tape", "Default.cas");
    al_set_config_value  (config, "File",       "cart", "BASIC.bin");
    al_set_config_value  (config, "File",       "printer", "Printer.out");
    al_add_config_comment(config, "File",       NULL);

    /* Speed */
    al_add_config_comment(config, "Speed",      "ifreq=<frequency>     Select interrupt frequency [50] Hz");
    al_add_config_comment(config, "Speed",      "cpuspeed=<speed>      Set Z80 CPU speed [100]%");
    al_add_config_comment(config, "Speed",      "sync=<mode>           Sync/Do not sync emulation [1]");
    al_add_config_comment(config, "Speed",      "                      0 - Do not sync");
    al_add_config_comment(config, "Speed",      "                      1 - Sync");
    al_add_config_comment(config, "Speed",      "                      Emulation will be too fast if sync is turned off");
    al_add_config_comment(config, "Speed",      "uperiod=<period>      Number of interrupts per screen update [1]");
    al_add_config_comment(config, "Speed",      "                      Try uperiod 2 or uperiod 3 if emulation is a bit slow");
    al_set_config_value  (config, "Speed",      "ifreq", "50");
    al_set_config_value  (config, "Speed",      "cpuspeed", "100");
    al_set_config_value  (config, "Speed",      "sync", "1");
    al_set_config_value  (config, "Speed",      "uperiod", "1");
    al_add_config_comment(config, "Speed",      NULL);
    
    /* Display */
    al_add_config_comment(config, "Display",    "video=<mode>          Select video mode/window size [0]");
    al_add_config_comment(config, "Display",    "                      0  - Autodetect best window size");
    al_add_config_comment(config, "Display",    "                      1  - 640x480");
    al_add_config_comment(config, "Display",    "                      2  - 800x600");
    al_add_config_comment(config, "Display",    "                      3  - 960x720");
    al_add_config_comment(config, "Display",    "                      4  - 1280x960");
    al_add_config_comment(config, "Display",    "                      5  - 1440x1080");
    al_add_config_comment(config, "Display",    "                      6  - 1600x1200");
    al_add_config_comment(config, "Display",    "                      7  - 1920x1440");
    al_add_config_comment(config, "Display",    "                      99 - Full Screen (not supported on Linux)");
    al_add_config_comment(config, "Display",    "scanlines=<mode>      Show/Do not show scanlines [0]");
    al_add_config_comment(config, "Display",    "                      0 - Do not show scanlines");
    al_add_config_comment(config, "Display",    "                      1 - Show scanlines");
    al_add_config_comment(config, "Display",    "smoothing=<mode>      Use display smoothing [1]");
    al_add_config_comment(config, "Display",    "                      0 - Do not use smoothing");
    al_add_config_comment(config, "Display",    "                      1 - Use smoothing");
    al_set_config_value  (config, "Display",    "video", "0");
    al_set_config_value  (config, "Display",    "scanlines", "0");
    al_set_config_value  (config, "Display",    "smoothing", "1");
    al_add_config_comment(config, "Display",    NULL);

    /* Keyboard */
    al_add_config_comment(config, "Keyboard",   "keymap <mode>         Select keyboard mapping [1]");
    al_add_config_comment(config, "Keyboard",   "                      0 - Positional mapping");
    al_add_config_comment(config, "Keyboard",   "                      1 - Symbolic mapping");
    al_set_config_value  (config, "Keyboard",   "keymap", "1");
    al_add_config_comment(config, "Keyboard",   NULL);

    /* Options */
    al_add_config_comment(config, "Options",    "sound=<mode>          Select sound mode [1]");
    al_add_config_comment(config, "Options",    "                      0 - No sound  ");
    al_add_config_comment(config, "Options",    "                      1 - Sound on");
    al_add_config_comment(config, "Options",    "volume=<value>        Select initial volume");
    al_add_config_comment(config, "Options",    "                      0 - Silent");
    al_add_config_comment(config, "Options",    "                      ...");
    al_add_config_comment(config, "Options",    "                      15 - Maximum");
    al_add_config_comment(config, "Options",    "joystick=<mode>       Select joystick mode [1]");
    al_add_config_comment(config, "Options",    "                      0 - No joystick support");
    al_add_config_comment(config, "Options",    "                      1 - Joystick support");
    al_add_config_comment(config, "Options",    "joymap=<mode>         Select joystick mapping [0]");
    al_add_config_comment(config, "Options",    "                      0 - Moving the joystick emulates cursorkey presses");
    al_add_config_comment(config, "Options",    "                          The main button emulates pressing the spacebar");
    al_add_config_comment(config, "Options",    "                      1 - Fraxxon mode: Left/right emulates cursorkeys left/up");
    al_add_config_comment(config, "Options",    "                          The main button emulates pressing the spacebar");
    al_set_config_value  (config, "Options",    "sound", "1");
    al_set_config_value  (config, "Options",    "volume", "4");
    al_set_config_value  (config, "Options",    "joystick", "1");
    al_set_config_value  (config, "Options",    "joymap", "0");
    al_add_config_comment(config, "Options",    NULL);

    /* Debug */
    al_add_config_comment(config, "Debug",      "verbose=<level>       Select debugging messages [0]");
    al_add_config_comment(config, "Debug",      "                      0 - Silent");    
    al_add_config_comment(config, "Debug",      "                      1 - Debug messages");
    al_add_config_comment(config, "Debug",      "                      4 - Debug and Tape messages");
    al_add_config_comment(config, "Debug",      "trap=<address>        Trap execution when PC reaches specified address [-1]");
    al_set_config_value  (config, "Debug",      "verbose", "1");

    al_save_config_file(al_path_cstr(docPath, PATH_SEPARATOR), config);
  }
  ParseConfig();
}

void SaveConfig(ALLEGRO_PATH * docPath) 
{
  static char intstr[4];

  if (sprintf(intstr, "%i", IFreq))         al_set_config_value(config, "Speed", "ifreq", intstr);
  if (sprintf(intstr, "%i", CpuSpeed))      al_set_config_value(config, "Speed", "cpuspeed", intstr);
  if (sprintf(intstr, "%i", Sync))          al_set_config_value(config, "Speed", "sync", intstr);
  if (sprintf(intstr, "%i", UPeriod))       al_set_config_value(config, "Speed", "uperiod", intstr);

  if (sprintf(intstr, "%i", optimalVideomode == videomode ? 0 : videomode)) al_set_config_value(config, "Display", "video", intstr);
  if (sprintf(intstr, "%i", scanlines))     al_set_config_value(config, "Display", "scanlines", intstr);
  if (sprintf(intstr, "%i", smoothing))     al_set_config_value(config, "Display", "smoothing", intstr);
  
  if (sprintf(intstr, "%i", keyboardmap))   al_set_config_value(config, "Keyboard", "keymap", intstr);

  if (sprintf(intstr, "%i", soundmode))     al_set_config_value(config, "Options", "sound", intstr);
  if (sprintf(intstr, "%i", mastervolume))  al_set_config_value(config, "Options", "volume", intstr);
  if (sprintf(intstr, "%i", joymode))       al_set_config_value(config, "Options", "joystick", intstr);
  if (sprintf(intstr, "%i", joymap))        al_set_config_value(config, "Options", "joymap", intstr);

  al_set_path_filename(docPath, CONFIG_FILENAME);
  al_save_config_file(al_path_cstr(docPath, PATH_SEPARATOR), config);
}