#include "libretro.h"

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

#define P2000_KEYCODE_UP 2
#define P2000_KEYCODE_DOWN 21
#define P2000_KEYCODE_LEFT 0
#define P2000_KEYCODE_RIGHT 23
#define P2000_KEYCODE_SPACE 17
#define P2000_KEYCODE_LSHIFT 72
#define P2000_KEYCODE_NUM_1 59
#define P2000_KEYCODE_NUM_3 56
#define P2000_KEYCODE_NUM_PERIOD 16

#define BOTH_K_0      (RETROK_0 | (RETROK_KP0 << 16))
#define BOTH_K_1      (RETROK_1 | (RETROK_KP1 << 16))
#define BOTH_K_2      (RETROK_2 | (RETROK_KP2 << 16))
#define BOTH_K_3      (RETROK_3 | (RETROK_KP3 << 16))
#define BOTH_K_4      (RETROK_4 | (RETROK_KP4 << 16))
#define BOTH_K_5      (RETROK_5 | (RETROK_KP5 << 16))
#define BOTH_K_6      (RETROK_6 | (RETROK_KP6 << 16))
#define BOTH_K_7      (RETROK_7 | (RETROK_KP7 << 16))
#define BOTH_K_8      (RETROK_8 | (RETROK_KP8 << 16))
#define BOTH_K_9      (RETROK_9 | (RETROK_KP9 << 16))
#define BOTH_K_PERIOD (RETROK_PERIOD | (RETROK_KP_PERIOD << 16))
#define BOTH_K_ENTER  (RETROK_RETURN | (RETROK_KP_ENTER << 16))

int key_map[] =
{
   RETROK_LEFT,      -1,           RETROK_UP, RETROK_q, -1,               -1,           -1,           -1,
   -1,               RETROK_h,     RETROK_z,  RETROK_s, RETROK_d,         RETROK_g,     RETROK_j,     RETROK_f,
   BOTH_K_PERIOD,    RETROK_SPACE, -1,        BOTH_K_0, -1,               RETROK_DOWN,  RETROK_COMMA, RETROK_RIGHT,
   RETROK_CAPSLOCK,  RETROK_n,     -1,        RETROK_x, RETROK_c,         RETROK_b,     RETROK_m,     RETROK_v,
   RETROK_BACKQUOTE, RETROK_y,     RETROK_a,  RETROK_w, RETROK_e,         RETROK_t,     RETROK_u,     RETROK_r,
   -1,               -1,           -1,        -1,       RETROK_BACKSPACE, -1,           -1,           RETROK_MINUS,
   BOTH_K_9,         RETROK_o,     BOTH_K_8,  BOTH_K_7, BOTH_K_ENTER,     RETROK_p,     -1,           -1,
   BOTH_K_3,         -1,           BOTH_K_2,  BOTH_K_1, -1,               RETROK_SLASH, RETROK_k,     RETROK_QUOTE,
   BOTH_K_6,         RETROK_l,     BOTH_K_5,  BOTH_K_4, -1,               -1,           RETROK_i,     -1,
   RETROK_LSHIFT,    -1,           -1,        -1,       -1,               -1,           -1,           RETROK_RSHIFT
};

unsigned int key_map_len = 80;
