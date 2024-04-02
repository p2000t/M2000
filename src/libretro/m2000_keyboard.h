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
#define LSHIFT P2000_KEYCODE_LSHIFT
#define P2000_KEYCODE_RSHIFT 79
#define P2000_KEYCODE_NUM_1 59
#define P2000_KEYCODE_NUM_3 56
#define P2000_KEYCODE_NUM_PERIOD 16 
#define KEYMAP_LESS_GREATER    (RETROK_OEM_102 | (RETROK_DELETE << 16))

int key_map[] =
{
   RETROK_LEFT,        RETROK_6,      RETROK_UP,           RETROK_q,       RETROK_3,            RETROK_5,         RETROK_7,     RETROK_4,
   RETROK_TAB,         RETROK_h,      RETROK_z,            RETROK_s,       RETROK_d,            RETROK_g,         RETROK_j,     RETROK_f,
   RETROK_KP_ENTER,    RETROK_SPACE,  RETROK_KP_PERIOD,    RETROK_KP0,     RETROK_BACKSLASH,    RETROK_DOWN,      RETROK_COMMA, RETROK_RIGHT,
   RETROK_CAPSLOCK,    RETROK_n,      KEYMAP_LESS_GREATER, RETROK_x,       RETROK_c,            RETROK_b,         RETROK_m,     RETROK_v,
   RETROK_BACKQUOTE,   RETROK_y,      RETROK_a,            RETROK_w,       RETROK_e,            RETROK_t,         RETROK_u,     RETROK_r,
   RETROK_KP_MULTIPLY, RETROK_9,      RETROK_KP_DIVIDE,    RETROK_NUMLOCK, RETROK_BACKSPACE,    RETROK_0,         RETROK_1,     RETROK_MINUS,
   RETROK_KP9,         RETROK_o,      RETROK_KP8,          RETROK_KP7,     RETROK_RETURN,       RETROK_p,         RETROK_8,     RETROK_LEFTBRACKET,
   RETROK_KP3,         RETROK_PERIOD, RETROK_KP2,          RETROK_KP1,     RETROK_RIGHTBRACKET, RETROK_SLASH,     RETROK_k,     RETROK_2,
   RETROK_KP6,         RETROK_l,      RETROK_KP5,          RETROK_KP4,     RETROK_EQUALS,       RETROK_SEMICOLON, RETROK_i,     RETROK_QUOTE,
   RETROK_LSHIFT,      -1,            -1,                  -1,             -1,                  -1,               -1,           RETROK_RSHIFT
};
unsigned key_map_len = 80;

enum keyboard_mapping 
{
   SYMBOLIC   = 0, 
   POSITIONAL = 1
};

typedef struct sym_key_mapping
{
   unsigned retro_key;
   unsigned p2000_key;
   unsigned shifted_p2000_key;
} sym_key_mapping_t;

sym_key_mapping_t sym_key_map[] =
{
  { RETROK_a,                     34,   LSHIFT + 34 }, // A       a
  { RETROK_b,                     29,   LSHIFT + 29 }, // B       b
  { RETROK_c,                     28,   LSHIFT + 28 }, // C       c
  { RETROK_d,                     12,   LSHIFT + 12 }, // D       d
  { RETROK_e,                     36,   LSHIFT + 36 }, // E       e
  { RETROK_f,                     15,   LSHIFT + 15 }, // F       f
  { RETROK_g,                     13,   LSHIFT + 13 }, // G       g
  { RETROK_h,                      9,   LSHIFT +  9 }, // H       h
  { RETROK_i,                     70,   LSHIFT + 70 }, // I       i
  { RETROK_j,                     14,   LSHIFT + 14 }, // J       j
  { RETROK_k,                     62,   LSHIFT + 62 }, // K       k
  { RETROK_l,                     65,   LSHIFT + 65 }, // L       l
  { RETROK_m,                     30,   LSHIFT + 30 }, // M       m
  { RETROK_n,                     25,   LSHIFT + 25 }, // N       n
  { RETROK_o,                     49,   LSHIFT + 49 }, // O       o
  { RETROK_p,                     53,   LSHIFT + 53 }, // P       p
  { RETROK_q,                      3,   LSHIFT +  3 }, // Q       q
  { RETROK_r,                     39,   LSHIFT + 39 }, // R       r
  { RETROK_s,                     11,   LSHIFT + 11 }, // S       s
  { RETROK_t,                     37,   LSHIFT + 37 }, // T       t
  { RETROK_u,                     38,   LSHIFT + 38 }, // U       u
  { RETROK_v,                     31,   LSHIFT + 31 }, // V       v
  { RETROK_w,                     35,   LSHIFT + 35 }, // W       w
  { RETROK_x,                     27,   LSHIFT + 27 }, // X       x
  { RETROK_y,                     33,   LSHIFT + 33 }, // Y       y
  { RETROK_z,                     10,   LSHIFT + 10 }, // Z       z
  { RETROK_1,                     46,   LSHIFT + 46 }, // 1       !
  { RETROK_2,                     63,            55 }, // 2       @
  { RETROK_3,                      4,            20 }, // 3       #
  { RETROK_4,                      7,   LSHIFT +  7 }, // 4       $
  { RETROK_5,                      5,   LSHIFT +  5 }, // 5       %
  { RETROK_6,                      1,   LSHIFT + 55 }, // 6       ↑
  { RETROK_7,                      6,   LSHIFT +  1 }, // 7       &
  { RETROK_8,                     54,   LSHIFT + 71 }, // 8       *  
  { RETROK_9,                     41,   LSHIFT + 54 }, // 9       (
  { RETROK_0,                     45,   LSHIFT + 41 }, // 0       )
  { RETROK_EQUALS,       LSHIFT + 45,            42 }, // =       +
  { RETROK_MINUS,                 47,   LSHIFT + 47 }, // -       _
  { RETROK_LEFTBRACKET,  LSHIFT + 60,            68 }, // ←       ¼
  { RETROK_RIGHTBRACKET,          60,   LSHIFT + 68 }, // →       ¾
  { RETROK_SEMICOLON,             69,            71 }, // ;       :
  { RETROK_QUOTE,        LSHIFT +  6,   LSHIFT + 63 }, // '       "
  { RETROK_LEFT,                   0,   LSHIFT +  0 }, // LEFT    LEFT LINE
  { RETROK_RIGHT,                 23,   LSHIFT + 23 }, // RIGHT   [free]
  { RETROK_UP,                     2,   LSHIFT +  2 }, // UP      LEFTUP
  { RETROK_DOWN,                  21,   LSHIFT + 21 }, // DOWN    RIGHTDOWN
  { RETROK_TAB,                    8,   LSHIFT +  8 }, // TAB     [free]
  { RETROK_COMMA,                 22,            26 }, // ,       <
  { RETROK_PERIOD,                57,   LSHIFT + 26 }, // .       >
  { RETROK_SPACE,                 17,   LSHIFT + 17 }, // SPACE   [free]
  { RETROK_BACKSPACE,             44,            40 }, // BACKSP  CLRLN
  { RETROK_DELETE,                44,   LSHIFT + 40 }, // BACKSP  CLRSCR
  { RETROK_SLASH,                 61,   LSHIFT + 61 }, // /       ?
  { RETROK_RETURN,                52,   LSHIFT + 52 }, // ENTER   [free]
  { RETROK_BACKSLASH,    LSHIFT + 20,   LSHIFT + 20 }, // █       █
  { RETROK_BACKQUOTE,             32,   LSHIFT + 32 }, // CODE    [free]
  { RETROK_KP9,                   48,   LSHIFT + 48 }, // 9       ?
  { RETROK_KP8,                   50,   LSHIFT + 50 }, // 8       ?
  { RETROK_KP7,                   51,   LSHIFT + 51 }, // 7       CASS WIS
  { RETROK_KP6,                   64,   LSHIFT + 64 }, // 6       ?
  { RETROK_KP5,                   66,   LSHIFT + 66 }, // 5       CLR+LIST
  { RETROK_KP4,                   67,   LSHIFT + 67 }, // 4       ?
  { RETROK_KP3,                   56,   LSHIFT + 56 }, // 3       START
  { RETROK_KP2,                   58,   LSHIFT + 58 }, // 2       ?
  { RETROK_KP1,                   59,   LSHIFT + 59 }, // 1       ZOEK
  { RETROK_KP0,                   19,   LSHIFT + 19 }, // 0       ?
  { RETROK_KP_PERIOD,             18,   LSHIFT + 18 }, // 00      ?
  { RETROK_KP_ENTER,              16,   LSHIFT + 16 }, // .       STOP
  { RETROK_NUMLOCK,               43,   LSHIFT + 43 }, // NUMPAD -
  { RETROK_KP_DIVIDE,             42,   LSHIFT + 42 }, // NUMPAD +
  { RETROK_KP_MULTIPLY,           40,   LSHIFT + 40 }, // NUMPAD CLRLN
};
unsigned sym_key_map_len = sizeof(sym_key_map) / sizeof(sym_key_map[0]);

typedef struct osks_mapping
{
   unsigned ascii_code; /* note: values > 255 are used for extra keys */
   unsigned p2000_code;
   unsigned is_shifted;
} osks_mapping_t;

//maps ASCII characters to P2000T keycodes
osks_mapping_t osks_ascii_map[] = 
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
   {  40, 54, 1 }, // Left parenthesis (
   {  41, 41, 1 }, // Right parenthesis )
   {  64, 55, 0 }, // At @
   {  38,  1, 1 }, // Ampersand &
   {  36,  7, 1 }, // Dollar $
   {  37,  5, 1 }, // Percent %
   {  95, 20, 0 }, // Hash #
   {  43, 69, 1 }, // Plus +
   {  45, 47, 0 }, // Minus -
   {  42, 71, 1 }, // Asterix *
   {  47, 61, 0 }, // Slash  /
   {  61, 45, 1 }, // Equals =
   {  46, 57, 0 }, // Period .
   {  44, 22, 0 }, // Comma ,
   {  58, 71, 0 }, // Colon :
   {  59, 69, 0 }, // Semi Colon ;
   {  34, 63, 1 }, // Double Quote "
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
   { 261, 32, 0 }, // <CODE>
   { 260, 44, 0 }, // Backspace
   {  32, 17, 0 }, // Space
   { 259, 59, 1 }, // <ZOEK>
   { 258, 16, 1 }, // <STOP>
   {   0,  0, 0 }, // Terminator
};
unsigned osks_map_length = sizeof(osks_ascii_map) / sizeof(osks_ascii_map[0]) - 1; //exclude terminator
