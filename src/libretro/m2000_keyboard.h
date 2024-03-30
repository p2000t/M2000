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

#define P2000_LESS     (RETROK_OEM_102 | (RETROK_DELETE << 16))

int key_map[] =
{
   RETROK_LEFT,        RETROK_6,      RETROK_UP,        RETROK_q,       RETROK_3,            RETROK_5,         RETROK_7,     RETROK_4,
   RETROK_TAB,         RETROK_h,      RETROK_z,         RETROK_s,       RETROK_d,            RETROK_g,         RETROK_j,     RETROK_f,
   RETROK_KP_ENTER,    RETROK_SPACE,  RETROK_KP_PERIOD, RETROK_KP0,     RETROK_BACKSLASH,    RETROK_DOWN,      RETROK_COMMA, RETROK_RIGHT,
   RETROK_CAPSLOCK,    RETROK_n,      P2000_LESS,       RETROK_x,       RETROK_c,            RETROK_b,         RETROK_m,     RETROK_v,
   RETROK_BACKQUOTE,   RETROK_y,      RETROK_a,         RETROK_w,       RETROK_e,            RETROK_t,         RETROK_u,     RETROK_r,
   RETROK_KP_MULTIPLY, RETROK_9,      RETROK_KP_DIVIDE, RETROK_NUMLOCK, RETROK_BACKSPACE,    RETROK_0,         RETROK_1,     RETROK_MINUS,
   RETROK_KP9,         RETROK_o,      RETROK_KP8,       RETROK_KP7,     RETROK_RETURN,       RETROK_p,         RETROK_8,     RETROK_LEFTBRACKET,
   RETROK_KP3,         RETROK_PERIOD, RETROK_KP2,       RETROK_KP1,     RETROK_RIGHTBRACKET, RETROK_SLASH,     RETROK_k,     RETROK_2,
   RETROK_KP6,         RETROK_l,      RETROK_KP5,       RETROK_KP4,     RETROK_EQUALS,       RETROK_SEMICOLON, RETROK_i,     RETROK_QUOTE,
   RETROK_LSHIFT,      -1,            -1,               -1,             -1,                  -1,               -1,           RETROK_RSHIFT
};
unsigned key_map_len = 80;

//maps ASCII characters to P2000T keycodes
unsigned osk_ascii_map[][3] = 
{ 
   // { ASCII code, P2000T code, shift }
   { 257, 56, 1 }, // <START>
   { 256, 52, 0 }, // Enter
   {  65, 34, 1 }, // A
   {  66, 29, 1 }, // B
   {  67, 28, 1 }, // C
   {  68, 12, 1 }, // D
   {  69, 36, 1 }, // E
   {  70, 15, 1 }, // F
   {  71, 13, 1 }, // G
   {  72,  9, 1 }, // H
   {  73, 70, 1 }, // I
   {  74, 14, 1 }, // J
   {  75, 62, 1 }, // K
   {  76, 65, 1 }, // L
   {  77, 30, 1 }, // M
   {  78, 25, 1 }, // N
   {  79, 49, 1 }, // O
   {  80, 53, 1 }, // P
   {  81,  3, 1 }, // Q
   {  82, 39, 1 }, // R
   {  83, 11, 1 }, // S
   {  84, 37, 1 }, // T
   {  85, 38, 1 }, // U
   {  86, 31, 1 }, // V
   {  87, 35, 1 }, // W
   {  88, 27, 1 }, // X
   {  89, 33, 1 }, // Y
   {  90, 10, 1 }, // Z
   {  97, 34, 0 }, // a
   {  98, 29, 0 }, // b
   {  99, 28, 0 }, // c
   { 100, 12, 0 }, // d
   { 101, 36, 0 }, // e
   { 102, 15, 0 }, // f
   { 103, 13, 0 }, // g
   { 104,  9, 0 }, // h
   { 105, 70, 0 }, // i
   { 106, 14, 0 }, // j
   { 107, 62, 0 }, // k
   { 108, 65, 0 }, // l
   { 109, 30, 0 }, // m
   { 110, 25, 0 }, // n
   { 111, 49, 0 }, // o
   { 112, 53, 0 }, // p
   { 113,  3, 0 }, // q
   { 114, 39, 0 }, // r
   { 115, 11, 0 }, // s
   { 116, 37, 0 }, // t
   { 117, 38, 0 }, // u
   { 118, 31, 0 }, // v
   { 119, 35, 0 }, // w
   { 120, 27, 0 }, // x
   { 121, 33, 0 }, // y
   { 122, 10, 0 }, // z
   {  64, 55, 0 }, // At @
   {  95, 20, 0 }, // Hash #
   {  63, 61, 1 }, // Question ?
   {  33, 46, 1 }, // Exclamation !
   {  48, 45, 0 }, // 0
   {  49, 46, 0 }, // 1
   {  50, 63, 0 }, // 2
   {  51,  4, 0 }, // 3
   {  52,  7, 0 }, // 4
   {  53,  5, 0 }, // 5
   {  54,  1, 0 }, // 6
   {  55,  6, 0 }, // 7
   {  56, 54, 0 }, // 8
   {  57, 41, 0 }, // 9
   {  91, 44, 0 }, // Backspace
   {  74, 14, 1 }, // J
   {  47, 61, 0 }, // Slash /
   {  78, 25, 1 }, // N
   {  32, 17, 0 }, // Space
   { 259, 59, 1 }, // <ZOEK>
   { 258, 16, 1 }, // <STOP>
   {   0,  0, 0 }, // Terminator
};

unsigned osk_map_length = sizeof(osk_ascii_map) / sizeof(osk_ascii_map[0]) - 1; //exclude terminator
