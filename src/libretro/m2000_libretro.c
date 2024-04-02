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
/*   Copyright (C) 1996-2023 by the M2000 team.                               */
/*                                                                            */
/*   See the file "LICENSE" for information on usage and redistribution of    */
/*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       */
/******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libretro.h>
#include <retro_timers.h>
#include <file/file_path.h>
#include "m2000_roms.h"
#include "m2000_keyboard.h"
#include "m2000_saa5050.h"
#include "../Z80.h"
#include "../P2000.h"

#define VIDEO_BUFFER_WIDTH 480
#define VIDEO_BUFFER_HEIGHT 480
#define SAMPLE_RATE 30000
#define P2000T_VRAM_SIZE 0x1000
#define NUMBER_OF_CHARS (96 + 64 + 64)
#define CHAR_WIDTH 12
#define CHAR_HEIGHT 20
#define CHAR_WIDTH_ORIG 6
#define CHAR_HEIGHT_ORIG 10
#define OSKS_TOTAL_CHARS 36
#define OSKS_HIGHLIGHT_XPOS 19
#define OSKS_LINE_YPOS 23
#define DEBOUNCE_NORMAL 30
#define DEBOUNCE_FAST 5
#define M2000_VARIABLE_KEYBOARD_MAPPING "m2000_keyboard_mapping"
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

static uint32_t *frame_buf;
static byte *font_buf;
static byte *osks_display;
static signed char *sound_buf = NULL;
static int16_t *audio_batch_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static int *display_char_buf;
static int buf_size;
static Z80_Regs registers;
static bool osks_visible = false;
static int osks_index = 0;
static char default_cas_path[MAX_PATH];
static enum keyboard_mapping_mode keyboard_mode = SYMBOLIC;

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }

/* ========================================================================== */
/*                 Start of M2000 function implementations                    */
/* ========================================================================== */

/****************************************************************************/
/*** Sync emulation on every interrupt                                    ***/
/****************************************************************************/
void SyncEmulation(void) 
{
   /* not needed, as frontend (e.g. RetroArch) will take care of syncing */
}

/****************************************************************************/
/*** Pauses a specified ammount of milliseconds                           ***/
/****************************************************************************/
void Pause(int ms) 
{
   retro_sleep(ms);
}

/****************************************************************************/
/*** This function creates the SAA5050 font with character rounding       ***/
/****************************************************************************/
int LoadFont(const char *filename)
{
   byte *font_ptr = font_buf;
   int line_pixels_prev, line_pixels, line_pixels_next;
   int pixel_n, pixel_e, pixel_s, pixel_w, pixel_sw, pixel_se, pixel_nw, pixel_ne;

   /* Stretch 6x10 characters to 12x20, so we can do character rounding  */
   /* 96 alphanum chars + 64 cont. graphic chars + 64 sep. graphic chars */
   for (int i = 0; i < NUMBER_OF_CHARS * CHAR_HEIGHT_ORIG; i += CHAR_HEIGHT_ORIG) 
   { 
      line_pixels_prev = 0;
      line_pixels = 0;
      line_pixels_next = saa5050_fnt[i] << CHAR_WIDTH_ORIG;
      for (int j = 0; j < CHAR_HEIGHT_ORIG; ++j) 
      {
         line_pixels_prev = line_pixels >> CHAR_WIDTH_ORIG;
         line_pixels = line_pixels_next >> CHAR_WIDTH_ORIG;
         line_pixels_next = j < (CHAR_HEIGHT_ORIG-1) ? saa5050_fnt[i + j + 1] : 0;
         for (int k = 0; k < CHAR_WIDTH_ORIG; ++k)
         {
            if (line_pixels & 0x20) /* bit 6 set = pixel set */
            {
               *font_ptr = *(font_ptr+1) = *(font_ptr+CHAR_WIDTH) = 
                  *(font_ptr+CHAR_WIDTH+1) = 0xff;
            }
            /* character rounding (only for alphanumeric chars) */
            else if (i < 96 * CHAR_HEIGHT_ORIG)
            { 
               /* for character rounding, look at 8 pixels around current pixel */
               pixel_n  = line_pixels_prev & 0x20;
               pixel_e  = line_pixels      & 0x10;
               pixel_s  = line_pixels_next & 0x20;
               pixel_w  = line_pixels      & 0x40;
               pixel_ne = line_pixels_prev & 0x10;
               pixel_se = line_pixels_next & 0x10;
               pixel_sw = line_pixels_next & 0x40;
               pixel_nw = line_pixels_prev & 0x40;

               /* rounding in NW direction */
               if (pixel_n && pixel_w && !pixel_nw) *font_ptr = 0xff;
               /* rounding in NE direction */
               if (pixel_n && pixel_e && !pixel_ne) *(font_ptr+1) = 0xff;
               /* rounding in SE direction */
               if (pixel_s && pixel_e && !pixel_se) *(font_ptr+CHAR_WIDTH+1) = 0xff;
               /* rounding in SW direction */
               if (pixel_s && pixel_w && !pixel_sw) *(font_ptr+CHAR_WIDTH) = 0xff;
            }
            /* process next pixel to the right */
            font_ptr += 2;
            line_pixels_prev<<=1;
            line_pixels<<=1;
            line_pixels_next<<=1;
         }
         font_ptr += CHAR_WIDTH;
      }
   }
   memcpy(font_ptr, saa5050_fnt_extra, saa5050_fnt_extra_size);
   return 1;
}

/****************************************************************************/
/*** This function is called when the P2000T's sound register is written  ***/
/****************************************************************************/
void Sound(int toggle)
{
   static int last = -1;
   int pos,val;

   if (toggle != last) {
      last = toggle;
      pos  = (buf_size-1) - ((buf_size-1)*Z80_ICount/Z80_IPeriod);
      val  =( toggle)? -1 : 1;
      sound_buf[pos]=val;
   }
}

/****************************************************************************/
/*** This function is called every interrupt to flush sound pipes and     ***/
/*** sync emulation                                                       ***/
/****************************************************************************/
void FlushSound(void)
{
   static int sound_state = 0;
   static int smooth = 0;

   for (int i=0; i<buf_size; ++i) 
   {
      if (sound_buf[i]) 
      {
         sound_state = sound_buf[i] << 10;
         sound_buf[i] = 0;
      }
      /* low pass filtering */
      smooth = (smooth << 1) + sound_state - smooth; 
      smooth >>= 1;
      audio_batch_buf[2*i] = audio_batch_buf[2*i+1] = smooth;
   }
   sound_state >>= 1;
   audio_batch_cb(audio_batch_buf, buf_size);
}

/****************************************************************************/
/*** Poll the keyboard on every interrupt                                 ***/
/****************************************************************************/
static int delayed_key = -1;
static int debounce_device = -1;
static int debounce_id = -1;
static int debounce_count = 0;
static int debounce_init = DEBOUNCE_NORMAL;

void set_debounce(int device, int id) 
{
   debounce_device = device;
   debounce_id = id;
   debounce_count = debounce_init; /* debounce for 0.5 seconds */
}

void push_key(int code) 
{
   KeyMap[code / 8] &= ~(1 << (code % 8));
}

void push_key_with_shift(int code, bool shift_pressed_last_frame) 
{
   push_key(P2000_KEYCODE_LSHIFT);
   if (shift_pressed_last_frame)
      push_key(code);
   else
      delayed_key = code;
}

#define JOY_0(id) input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, id)
#define KEYB_0(id) input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, id)

void Keyboard(void) 
{
   /* handle delayed key presses */
   if (delayed_key >= 0) 
      push_key(delayed_key);
   delayed_key = -1;

   /* handle debounce */
   if (debounce_device >= 0)
   {
      /* skip handling of other keys while source key/button is pressed */
      if (input_state_cb(0, debounce_device, 0, debounce_id) && --debounce_count > 0)
         return;
      debounce_init = debounce_count ? DEBOUNCE_NORMAL : DEBOUNCE_FAST;
      debounce_device = debounce_id = -1;
   }

   /* poll latest keyboard state */
   input_poll_cb();

   bool shift_pressed_last_frame = (~KeyMap[9] & 0xff) ? 1 : 0;

   /* release all P2000T keys by clearing KeyMap */
   for (int i = 0; i < 10; i++)
      KeyMap[i] = 0xff;

   /* On-Screen Key Selector (OSKS) handling */
   osks_visible = JOY_0(RETRO_DEVICE_ID_JOYPAD_L) || JOY_0(RETRO_DEVICE_ID_JOYPAD_L2);
   if (osks_visible)
   {
      /* handle OSK left/down */
      if (JOY_0(RETRO_DEVICE_ID_JOYPAD_LEFT) || JOY_0(RETRO_DEVICE_ID_JOYPAD_DOWN))
      {
         if (--osks_index < 0)
            osks_index = osks_map_length -1;
         set_debounce(RETRO_DEVICE_JOYPAD, JOY_0(RETRO_DEVICE_ID_JOYPAD_LEFT) 
            ? RETRO_DEVICE_ID_JOYPAD_LEFT : RETRO_DEVICE_ID_JOYPAD_DOWN);
      }
      /* handle OSK right/up */
      if (JOY_0(RETRO_DEVICE_ID_JOYPAD_RIGHT) || JOY_0(RETRO_DEVICE_ID_JOYPAD_UP))
      {
         if (++osks_index >= osks_map_length)
            osks_index = 0;
         set_debounce(RETRO_DEVICE_JOYPAD, JOY_0(RETRO_DEVICE_ID_JOYPAD_RIGHT) 
            ? RETRO_DEVICE_ID_JOYPAD_RIGHT : RETRO_DEVICE_ID_JOYPAD_UP);
      }
      /* handle OSK fire/trigger */
      if (JOY_0(RETRO_DEVICE_ID_JOYPAD_A) || JOY_0(RETRO_DEVICE_ID_JOYPAD_B))
      {
         unsigned p2000_key_code = osks_ascii_map[osks_index].p2000_code;
         if (osks_ascii_map[osks_index].is_shifted)
            push_key_with_shift(p2000_key_code, shift_pressed_last_frame);
         else
            push_key(p2000_key_code);
      }

      /* prepare OSK display array */
      osks_mapping_t *osks_ptr  = osks_ascii_map + osks_index;
      if (osks_index < OSKS_HIGHLIGHT_XPOS)
         osks_ptr += osks_map_length;
      osks_ptr -= OSKS_HIGHLIGHT_XPOS;
      for (int i = 0; i < 40; i++, osks_ptr++)
      {
         if (!osks_ptr->ascii_code)
            osks_ptr = osks_ascii_map;
         osks_display[i] = osks_ptr->ascii_code - 32;
      }
      return;
   }

   /*************************/
   /* handle keyboard input */
   /*************************/
   if (keyboard_mode == POSITIONAL)
   {
      /* positional key mapping */
      for (int i = 0; i < pos_key_map_len; i++) 
      {
         int retro_key = pos_key_map[i] & 0xffff;
         int retro_key2 = pos_key_map[i] >> 16;
         if (KEYB_0(retro_key) || (retro_key2 && KEYB_0(retro_key2)))
            push_key(i);
      }
   }
   else
   {
      /* symbolic key mapping */
      bool retro_shift_down = KEYB_0(RETROK_LSHIFT) || KEYB_0(RETROK_RSHIFT);
      bool sym_key_pressed = false;
      for (int i = 0; i < sym_key_map_len; i++)
      {
         sym_key_mapping_t sym_mapping = sym_key_map[i];
         if (KEYB_0(sym_mapping.retro_key))
         {
            bool simple_mapping = (sym_mapping.p2000_key + LSHIFT == sym_mapping.shifted_p2000_key);
            if (!simple_mapping && sym_key_pressed)
               continue;

            unsigned p2000_key = retro_shift_down ? sym_mapping.shifted_p2000_key : sym_mapping.p2000_key;
            bool p2000_key_needs_shift = p2000_key >= LSHIFT;
            unsigned p2000_base_key = p2000_key_needs_shift ? p2000_key-LSHIFT : p2000_key;

            if (p2000_key_needs_shift == shift_pressed_last_frame)
               push_key(p2000_base_key);
            else
               delayed_key = p2000_base_key;

            if (p2000_key_needs_shift)
               push_key(P2000_KEYCODE_LSHIFT);

            sym_key_pressed = true;
            if (!simple_mapping)
               break;
         }
      }
      if (sym_key_pressed)
         return;
      if (retro_shift_down)
         push_key(KEYB_0(RETROK_LSHIFT) ? P2000_KEYCODE_LSHIFT : P2000_KEYCODE_RSHIFT);
   }

   /*************************************************/
   /* map F1 to <START>, F2 to <STOP>, F3 to <ZOEK> */
   /*************************************************/
   if (KEYB_0(RETROK_F1))
      push_key_with_shift(P2000_KEYCODE_NUM_3, shift_pressed_last_frame);
   if (KEYB_0(RETROK_F2))
      push_key_with_shift(P2000_KEYCODE_NUM_PERIOD, shift_pressed_last_frame);
   if (KEYB_0(RETROK_F3))
      push_key_with_shift(P2000_KEYCODE_NUM_1, shift_pressed_last_frame);

   /***********************************/
   /* map joypad input to key presses */
   /***********************************/
   if (JOY_0(RETRO_DEVICE_ID_JOYPAD_UP))
      push_key(P2000_KEYCODE_UP);
   if (JOY_0(RETRO_DEVICE_ID_JOYPAD_DOWN))
      push_key(P2000_KEYCODE_DOWN);
   if (JOY_0(RETRO_DEVICE_ID_JOYPAD_LEFT))
      push_key(P2000_KEYCODE_LEFT);
   if (JOY_0(RETRO_DEVICE_ID_JOYPAD_RIGHT))
      push_key(P2000_KEYCODE_RIGHT);
   if (JOY_0(RETRO_DEVICE_ID_JOYPAD_A) || JOY_0(RETRO_DEVICE_ID_JOYPAD_B))
      push_key(P2000_KEYCODE_SPACE);
   if (JOY_0(RETRO_DEVICE_ID_JOYPAD_START))
      push_key_with_shift(P2000_KEYCODE_NUM_3, shift_pressed_last_frame);
   if (JOY_0(RETRO_DEVICE_ID_JOYPAD_SELECT))
      push_key_with_shift(P2000_KEYCODE_NUM_PERIOD, shift_pressed_last_frame);
}

/****************************************************************************/
/*** Put a character in the display buffer                                ***/
/****************************************************************************/
void PutChar(int x, int y, int c, int fg, int bg, int si)
{
   /* check if we need to display OSKS on bottom line */
   if (osks_visible && y == OSKS_LINE_YPOS) 
   {
      c = osks_display[x];
      fg = P2000T_BLACK;
      bg = x == OSKS_HIGHLIGHT_XPOS ? P2000T_YELLOW : P2000T_CYAN;
      si = 0;
   }

   int display_char = c + (fg << 8) + (bg << 16) + (si << 24);
   /* skip if character is already on screen */
   if (display_char == display_char_buf[y * 40 + x])
      return;

   display_char_buf[y * 40 + x] = display_char;
   byte *font_buf_ptr = font_buf + c * CHAR_WIDTH * CHAR_HEIGHT + (si >> 1) * CHAR_WIDTH * CHAR_HEIGHT/2;

   for (int j = 0; j < CHAR_HEIGHT; j++)
   {
      for (int i = 0; i < CHAR_WIDTH; i++)
      {
         frame_buf[(x * CHAR_WIDTH + i) + (y * CHAR_HEIGHT + j) * VIDEO_BUFFER_WIDTH] = 
            *font_buf_ptr++ ? pal_xrgb[fg] : pal_xrgb[bg];
      }
      if (si && (j & 1) == 0)
         font_buf_ptr -= CHAR_WIDTH;
   }
}

/****************************************************************************/
/*** Push the display buffer for actual rendering on every interrupt      ***/
/****************************************************************************/
void PutImage (void)
{
   video_cb(frame_buf, VIDEO_BUFFER_WIDTH, VIDEO_BUFFER_HEIGHT, VIDEO_BUFFER_WIDTH << 2);
}

/* ========================================================================== */
/*             Start of Libretro Core function implementations                */
/* ========================================================================== */

void retro_init(void)
{
   /* log_cb(RETRO_LOG_INFO, "retro_init called\n"); */
   frame_buf = calloc(VIDEO_BUFFER_WIDTH * VIDEO_BUFFER_HEIGHT, sizeof(uint32_t));
   font_buf = calloc(NUMBER_OF_CHARS * CHAR_WIDTH * CHAR_HEIGHT + saa5050_fnt_extra_size, sizeof(byte));
   display_char_buf = calloc(80 * 24, sizeof(int));
   buf_size = SAMPLE_RATE / IFreq;
   sound_buf = calloc(buf_size, sizeof(char));
   audio_batch_buf = calloc(buf_size * 2, sizeof(int16_t)); /* * 2 for stereo */
   osks_display = calloc(OSKS_TOTAL_CHARS, sizeof(char));
   InitP2000(monitor_rom, basic_nl_rom);
   PrnName = NULL; /* disable printing */
}

void retro_deinit(void)
{
   TrashP2000();
   free(frame_buf);
   free(font_buf);
   free(sound_buf);
   free(audio_batch_buf);
   free(display_char_buf);
   free(osks_display);
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "M2000";
   info->library_version  = "v0.9.1";
   info->need_fullpath    = true;
   info->valid_extensions = "cas|p2000t";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing = (struct retro_system_timing) {
      .fps = 50.0,
      .sample_rate = (float)SAMPLE_RATE,
   };

   info->geometry = (struct retro_game_geometry) {
      .base_width   = 240,
      .base_height  = 240,
      .max_width    = 240,
      .max_height   = 240,
      .aspect_ratio =  4.0f / 3.0f,
   };
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

static void update_variables(void)
{
   struct retro_variable var;

   var.key = M2000_VARIABLE_KEYBOARD_MAPPING;
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      keyboard_mode = !strcmp(var.value, "positional") ? POSITIONAL : SYMBOLIC;
   }
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   /* M2000 can start without .cas game */
   bool no_content = true;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

    /* provide core variables to frontend */
   static struct retro_variable variables[] = {
      { M2000_VARIABLE_KEYBOARD_MAPPING, "Keyboard mapping; symbolic|positional" },
      { NULL, NULL },
   };
   environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);

   /* create empty keyboard callback, to allow for "Game Focus" detection */
   struct retro_keyboard_callback cb_kb = { NULL };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb_kb);

   /* setup log */
   log_cb = environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)
      ? logging.log : fallback_log;
}

void retro_reset(void)
{
   Z80_Reset();
}

void retro_run(void)
{
   /* check if M2000 core variables were updated */
   bool updated = false;
   if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   /* execute a period of Z80 emulation */
   Z80_Execute();
}

bool retro_load_game(const struct retro_game_info *info)
{
   static const struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up"                     },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down"                   },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left"                   },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right"                  },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Space/Fire"             },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Space/Fire"             },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "<START>"                },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "<STOP>"                 },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "On-Screen Key Selector" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "On-Screen Key Selector" },
   };
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)desc);

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   /* if path to .cas game is given, load it read-only */
   if (info && info->path) 
   {
      TapeBootEnabled = 1;
      InsertCassette(info->path, fopen(info->path, "rb"), true);
   }
   else 
   {
      /* else try to open/create 'default.cas' in Saves folder */
      const char *saves_dir = NULL;
      if(environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &saves_dir) && saves_dir)
      {
         snprintf (default_cas_path, sizeof(default_cas_path), "%s%c%s", saves_dir, PATH_DEFAULT_SLASH_C(), TapeName);
         /* try to open 'default.cas' for update, else create it */
         FILE *f = fopen(default_cas_path,"r+b");
         TapeBootEnabled = 0;
         InsertCassette(default_cas_path, f ? f : fopen(default_cas_path, "w+b"), false);
      }
   }
   return true;
}

void retro_unload_game(void)
{
   RemoveCassette();
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_PAL;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   return false;
}

size_t retro_serialize_size(void)
{
   return sizeof(registers) + P2000T_VRAM_SIZE + RAMSizeKb * 1024;
}

bool retro_serialize(void *data_, size_t size)
{
   if (size != retro_serialize_size())
      return false;

   Z80_GetRegs(&registers);  
   uint8_t *data = data_;
   memcpy(data, &registers, sizeof(registers));
   data += sizeof(registers);
   memcpy(data, VRAM, P2000T_VRAM_SIZE);
   data += P2000T_VRAM_SIZE;
   memcpy(data, RAM, RAMSizeKb * 1024);
   return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
   if (size != retro_serialize_size())
      return false;

   const uint8_t *data = data_;
   memcpy(&registers, data, sizeof(registers));
   data += sizeof(registers);
   memcpy(VRAM, data, P2000T_VRAM_SIZE);
   data += P2000T_VRAM_SIZE;
   memcpy(RAM, data, RAMSizeKb * 1024);
   Z80_SetRegs(&registers);
   return true;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}
