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
/*   Copyright (C) 1996-2023 by Marcel de Kogel and the M2000 team.           */
/*                                                                            */
/*   See the file "LICENSE" for information on usage and redistribution of    */
/*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       */
/******************************************************************************/

#include <allegro5/allegro.h>

/*
    P2000 Keyboard layout

    Y \ X   0       1        2       3        4       5        6       7
    0       LEFT    6        UP      Q        3       5        7       4
    1       TAB     H        Z       S        D       G        J       F
    2       . *     SPACE    00 *    0 *      #       DOWN     ,       RIGHT
    3       SHLOCK  N        <       X        C       B        M       V
    4       CODE    Y        A       W        E       T        U       R
    5       CLRLN * 9        + *     - *      BACKSP  0        1       -
    6       9 *     O        8 *     7 *      ENTER   P        8       @
    7       3 *     .        2 *     1 *      ->      /        K       2
    8       6 *     L        5 *     4 *      1/4     ;        I       :
    9       LSHIFT                                                     RSHIFT

    Keys marked with an asterix (*) are on the numeric keypad
*/
#ifdef __APPLE__
#define KEY_BETWEEN_LSHIFT_AND_Z ALLEGRO_KEY_BACKQUOTE
#else
#define KEY_BETWEEN_LSHIFT_AND_Z ALLEGRO_KEY_BACKSLASH2
#endif
static int keymask[] =
{
  ALLEGRO_KEY_LEFT,      ALLEGRO_KEY_6,        ALLEGRO_KEY_UP,           ALLEGRO_KEY_Q,       ALLEGRO_KEY_3,          ALLEGRO_KEY_5,         ALLEGRO_KEY_7,     ALLEGRO_KEY_4,
  ALLEGRO_KEY_TAB,       ALLEGRO_KEY_H,        ALLEGRO_KEY_Z,            ALLEGRO_KEY_S,       ALLEGRO_KEY_D,          ALLEGRO_KEY_G,         ALLEGRO_KEY_J,     ALLEGRO_KEY_F,
  ALLEGRO_KEY_PAD_ENTER, ALLEGRO_KEY_SPACE,    ALLEGRO_KEY_PAD_DELETE,   ALLEGRO_KEY_PAD_0,   ALLEGRO_KEY_BACKSLASH,  ALLEGRO_KEY_DOWN,      ALLEGRO_KEY_COMMA, ALLEGRO_KEY_RIGHT,
  ALLEGRO_KEY_CAPSLOCK,  ALLEGRO_KEY_N,        KEY_BETWEEN_LSHIFT_AND_Z, ALLEGRO_KEY_X,       ALLEGRO_KEY_C,          ALLEGRO_KEY_B,         ALLEGRO_KEY_M,     ALLEGRO_KEY_V,
  ALLEGRO_KEY_TILDE,     ALLEGRO_KEY_Y,        ALLEGRO_KEY_A,            ALLEGRO_KEY_W,       ALLEGRO_KEY_E,          ALLEGRO_KEY_T,         ALLEGRO_KEY_U,     ALLEGRO_KEY_R,
  ALLEGRO_KEY_PAD_MINUS, ALLEGRO_KEY_9,        ALLEGRO_KEY_PAD_ASTERISK, ALLEGRO_KEY_SLASH,   ALLEGRO_KEY_BACKSPACE,  ALLEGRO_KEY_0,         ALLEGRO_KEY_1,     ALLEGRO_KEY_MINUS,
  ALLEGRO_KEY_PAD_9,     ALLEGRO_KEY_O,        ALLEGRO_KEY_PAD_8,        ALLEGRO_KEY_PAD_7,   ALLEGRO_KEY_ENTER,      ALLEGRO_KEY_P,         ALLEGRO_KEY_8,     ALLEGRO_KEY_OPENBRACE,
  ALLEGRO_KEY_PAD_3,     ALLEGRO_KEY_FULLSTOP, ALLEGRO_KEY_PAD_2,        ALLEGRO_KEY_PAD_1,   ALLEGRO_KEY_CLOSEBRACE, ALLEGRO_KEY_SLASH,     ALLEGRO_KEY_K,     ALLEGRO_KEY_2,
  ALLEGRO_KEY_PAD_6,     ALLEGRO_KEY_L,        ALLEGRO_KEY_PAD_5,        ALLEGRO_KEY_PAD_4,   ALLEGRO_KEY_EQUALS,     ALLEGRO_KEY_SEMICOLON, ALLEGRO_KEY_I,     ALLEGRO_KEY_QUOTE,
  ALLEGRO_KEY_LSHIFT,    0,                    0,                        0,                   0,                      0,                     0,                 ALLEGRO_KEY_RSHIFT
};

#define NUMBER_OF_KEYMAPPINGS 68
static byte keyMappings[NUMBER_OF_KEYMAPPINGS][5] =
{
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_A,          34,      0,       34,      1 }, // A       a
  { ALLEGRO_KEY_B,          29,      0,       29,      1 }, // B       b
  { ALLEGRO_KEY_C,          28,      0,       28,      1 }, // C       c
  { ALLEGRO_KEY_D,          12,      0,       12,      1 }, // D       d
  { ALLEGRO_KEY_E,          36,      0,       36,      1 }, // E       e
  { ALLEGRO_KEY_F,          15,      0,       15,      1 }, // F       f
  { ALLEGRO_KEY_G,          13,      0,       13,      1 }, // G       g
  { ALLEGRO_KEY_H,           9,      0,        9,      1 }, // H       h
  { ALLEGRO_KEY_I,          70,      0,       70,      1 }, // I       i
  { ALLEGRO_KEY_J,          14,      0,       14,      1 }, // J       j
  { ALLEGRO_KEY_K,          62,      0,       62,      1 }, // K       k
  { ALLEGRO_KEY_L,          65,      0,       65,      1 }, // L       l
  { ALLEGRO_KEY_M,          30,      0,       30,      1 }, // M       m
  { ALLEGRO_KEY_N,          25,      0,       25,      1 }, // N       n
  { ALLEGRO_KEY_O,          49,      0,       49,      1 }, // O       o
  { ALLEGRO_KEY_P,          53,      0,       53,      1 }, // P       p
  { ALLEGRO_KEY_Q,           3,      0,        3,      1 }, // Q       q
  { ALLEGRO_KEY_R,          39,      0,       39,      1 }, // R       r
  { ALLEGRO_KEY_S,          11,      0,       11,      1 }, // S       s
  { ALLEGRO_KEY_T,          37,      0,       37,      1 }, // T       t
  { ALLEGRO_KEY_U,          38,      0,       38,      1 }, // U       u
  { ALLEGRO_KEY_V,          31,      0,       31,      1 }, // V       v
  { ALLEGRO_KEY_W,          35,      0,       35,      1 }, // W       w
  { ALLEGRO_KEY_X,          27,      0,       27,      1 }, // X       x
  { ALLEGRO_KEY_Y,          33,      0,       33,      1 }, // Y       y
  { ALLEGRO_KEY_Z,          10,      0,       10,      1 }, // Z       z
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_1,          46,      0,       46,      1 }, // 1       !
  { ALLEGRO_KEY_2,          63,      0,       55,      0 }, // 2       @
  { ALLEGRO_KEY_3,           4,      0,       20,      0 }, // 3       #
  { ALLEGRO_KEY_4,           7,      0,        7,      1 }, // 4       $
  { ALLEGRO_KEY_5,           5,      0,        5,      1 }, // 5       %
  { ALLEGRO_KEY_6,           1,      0,       55,      1 }, // 6       ↑
  { ALLEGRO_KEY_7,           6,      0,        1,      1 }, // 7       &
  { ALLEGRO_KEY_8,          54,      0,       71,      1 }, // 8       *  
  { ALLEGRO_KEY_9,          41,      0,       54,      1 }, // 9       (
  { ALLEGRO_KEY_0,          45,      0,       41,      1 }, // 0       )
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_EQUALS,     45,      1,       42,      0 }, // =       +
  { ALLEGRO_KEY_MINUS,      47,      0,       47,      1 }, // -       _
  { ALLEGRO_KEY_OPENBRACE,  60,      1,       68,      0 }, // ←       ¼
  { ALLEGRO_KEY_CLOSEBRACE, 60,      0,       68,      1 }, // →       ¾
  { ALLEGRO_KEY_SEMICOLON,  69,      0,       71,      0 }, // ;       :
  { ALLEGRO_KEY_QUOTE,       6,      1,       63,      1 }, // '       "
  { ALLEGRO_KEY_LEFT,        0,      0,        0,      1 }, // LEFT    LEFT LINE
  { ALLEGRO_KEY_RIGHT,      23,      0,       23,      1 }, // RIGHT   [free]
  { ALLEGRO_KEY_UP,          2,      0,        2,      1 }, // UP      LEFTUP
  { ALLEGRO_KEY_DOWN,       21,      0,       21,      1 }, // DOWN    RIGHTDOWN
  { ALLEGRO_KEY_TAB,         8,      0,        8,      1 }, // TAB     [free]
  { ALLEGRO_KEY_COMMA,      22,      0,       26,      0 }, // ,       <
  { ALLEGRO_KEY_FULLSTOP,   57,      0,       26,      1 }, // .       >
  { ALLEGRO_KEY_SPACE,      17,      0,       17,      1 }, // SPACE   [free]
  { ALLEGRO_KEY_BACKSPACE,  44,      0,       40,      0 }, // BACKSP  CLRLN
  { ALLEGRO_KEY_DELETE,     44,      0,       40,      1 }, // BACKSP  CLRSCR
  { ALLEGRO_KEY_SLASH,      61,      0,       61,      1 }, // /       ?
  { ALLEGRO_KEY_ENTER,      52,      0,       52,      1 }, // ENTER   [free]
  { ALLEGRO_KEY_BACKSLASH,  20,      1,       20,      1 }, // █       [free]
  { ALLEGRO_KEY_BACKQUOTE,  32,      0,       32,      1 }, // CODE    [free]
  //   AllegroKey     P2000Key  +shift? ShiftKey  +shift?   Char Shifted
  { ALLEGRO_KEY_PAD_9,      48,      0,       48,      1 }, // 9       ?
  { ALLEGRO_KEY_PAD_8,      50,      0,       50,      1 }, // 8       ?
  { ALLEGRO_KEY_PAD_7,      51,      0,       51,      1 }, // 7       CASS WIS
  { ALLEGRO_KEY_PAD_6,      64,      0,       64,      1 }, // 6       ?
  { ALLEGRO_KEY_PAD_5,      66,      0,       66,      1 }, // 5       CLR+LIST
  { ALLEGRO_KEY_PAD_4,      67,      0,       67,      1 }, // 4       ?
  { ALLEGRO_KEY_PAD_3,      56,      0,       56,      1 }, // 3       START
  { ALLEGRO_KEY_PAD_2,      58,      0,       58,      1 }, // 2       ?
  { ALLEGRO_KEY_PAD_1,      59,      0,       59,      1 }, // 1       ZOEK
  { ALLEGRO_KEY_PAD_0,      19,      0,       19,      1 }, // 0       ?
  { ALLEGRO_KEY_PAD_DELETE, 18,      0,       18,      1 }, // 00      ?
  { ALLEGRO_KEY_PAD_ENTER,  16,      0,       16,      1 }, // .       STOP
};