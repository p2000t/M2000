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
  "[filename] = name of the file to load as a cartridge [BASIC.bin]",
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
#if defined(MSDOS) || defined(LINUX_SVGA)
  "  -video <mode>              - Select video mode (T-model emulation only) [0]",
#ifdef MSDOS
  "                               0 - 256x240   1 - 640x480",
#else
  "                               0 - 320x240   1 - 640x480",
#endif
#else
  "  -video <mode>              - Select window size [0]",
  "                               0 - 500x300   1 - 520x490",
#endif
  "  -ram <size>                - Select amount of RAM installed [32KB]",
  "  -printer <filename>        - Select file for printer output "
#ifdef MSDOS
                                 "[PRN]",
#else
                                 "[stdout]",
#endif
  "  -printertype <value>       - Select printer type [0]",
  "                               0 - Daisy wheel   1 - Matrix",
  "",
  "  -tape <filename>           - Select tape image to use [P2000.cas]",
  "  -boot <value>              - Allow/Don't allow BASIC to boot from tape [0]",
  "                               0 - Don't allow booting",
  "                               1 - Allow booting",
  "  -font <filename>           - Select font to use [Default.fnt]",
#ifdef SOUND
  "  -sound <mode>              - Select sound mode [255]",
  "                               0 - No sound",
#ifdef MSDOS
  "                               1 - PC Speaker",
  "                               2 - SoundBlaster",
#else
  "                               1 - /dev/dsp",
#endif
  "                               255 - Detect",
  "  -volume <value>            - Select initial volume [10]",
  "                               0 - Silent    15 - Maximum",
#endif
#ifdef JOYSTICK
  "  -joystick <mode>           - Select joystick mode [1]",
  "                               0 - No joystick support  1 - Joystick support",
#endif
#ifdef MITSHM
  "  -shm <mode>                - Use/Don't use MITSHM extensions for X [1]",
  "                               0 - Don't use SHM   1 - Use SHM",
#endif
#ifdef UNIX_X
  "  -savecpu <mode>            - Save/Don't save CPU when inactive [1]",
  "                               0 - Don't save CPU   1 - Save CPU",
#endif
  NULL
};
