/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                Help.h                                ***/
/***                                                                      ***/
/*** This file contains the messages printed when the -help command line  ***/
/*** option is used                                                       ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

char *HelpText[] =
{
  "Usage: m2000 [-option1 [-option2...]] [filename]",
  "[filename] = optional cassette (.cas) or cartridge (.bin) to preload",
  "[-option]  =",
#ifdef DEBUG
  "  -trap [-tr] <address>      - Trap execution when PC reaches address [-1]",
#endif
  "  -help                      - Print this help page",
  "  -verbose <flags>           - Select debugging messages [1]",
  "                               0 - Silent     1 - Startup messages",
  "                               4 - Tape",
  "  -romfile <file>            - Select P2000 ROM dump file [P2000ROM.bin]",
  "  -cpuspeed <speed>          - Set Z80 CPU speed [100%]",
  "  -ifreq <frequency>         - Set interrupt frequency [50Hz]",
  "  -sync <value>              - Sync/Do not sync emulation [1]",
  "                               0 - Dot no sync   1 - Sync",
  "  -uperiod <value>           - Set number of interrupts per screen update [1]",
  "  -t / -m                    - Select P2000 model [-t]",
#ifdef ALLEGRO
  "  -keymap <mode>             - Select keyboard mapping [1]",
  "                                0 - Positional mapping",
  "                                1 - Symbolic mapping",
#endif
  "  -video <mode>              - Select window size [0]",
#ifdef ALLEGRO
  "                               0  - Detect best window size",
  "                               1  - 640 x 480    2 - 800 x 600",
  "                               3  - 960 x 720    4 - 1280 x 960",
  "                               5  - 1440 x 1080  6 - 1600 x 1200",
  "                               7  - 1920 x 1440",
  "                               99 - Full screen (not supported on Linux)",
#else //MS-DOS
  "                               0 - 256x240   1 - 640x480",
#endif
  "  -ram <size>                - Select amount of RAM installed [32KB]",
  "  -printer <filename>        - Select file for printer output [Printer.out]",
  "  -printertype <value>       - Select printer type [0]",
  "                               0 - Daisy wheel   1 - Matrix",
  "",
  "  -tape <filename>           - Select tape image to use [Default.cas]",
  "  -boot <value>              - Allow/Don't allow BASIC to boot from tape [0]",
  "                               0 - Don't allow booting",
  "                               1 - Allow booting",
  "  -font <filename>           - Select font to use [Default.fnt]",
  "  -sound <mode>              - Select sound mode [255]",
  "                               0 - No sound",
#ifdef MSDOS
  "                               1 - PC Speaker",
  "                               2 - SoundBlaster",
#endif
  "                               255 - Detect",
  "  -volume <value>            - Select initial volume [10]",
  "                               0 - Silent    15 - Maximum",
  "  -joystick <mode>           - Select joystick mode [1]",
  "                               0 - No joystick support  1 - Joystick support",
  "  -joymap <mode>             - Select joystick mapping [0]",
  "                               0 - Moving the joystick emulates the cursorkeys",
  "                                   The main button emulates the spacebar",
  "                               1 - Fraxxon mode: Moving left/right does left/up",
  "                                   The main button emulates the spacebar",
  NULL
};
