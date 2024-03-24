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
#include <stdlib.h>
#include "libretro.h"
#include "retro_timers.h"
#include "m2000_roms.h"
#include "m2000_keyboard.h"
#include "../Z80.h"
#include "../P2000.h"

#define VIDEO_BUFFER_WIDTH 480
#define VIDEO_BUFFER_HEIGHT 480
#define SAMPLE_RATE 30000
#define P2000T_VRAM_SIZE 0x1000
#define NUMBER_OF_CHARS 224
#define CHAR_WIDTH 12
#define CHAR_HEIGHT 20
#define CHAR_WIDTH_ORIG 6
#define CHAR_HEIGHT_ORIG 10

#define P2000_KEYCODE_UP 2
#define P2000_KEYCODE_DOWN 21
#define P2000_KEYCODE_LEFT 0
#define P2000_KEYCODE_RIGHT 23
#define P2000_KEYCODE_SPACE 17
#define P2000_KEYCODE_LSHIFT 72
#define P2000_KEYCODE_NUM_3 56
#define P2000_KEYCODE_NUM_PERIOD 16

static uint32_t *frame_buf;
static byte *font_buf;
static signed char *sound_buf = NULL;
static int16_t *audio_batch_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static int *display_char_buf;
static int buf_size;
static Z80_Regs registers;

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

/****************************************************************************/
/*** Sync emulation on every interrupt                                    ***/
/****************************************************************************/
void SyncEmulation(void) 
{
   // not needed, as frontend (e.g. RetroArch) will take care of syncing
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
   byte *p = font_buf;
   int linePixelsPrev, linePixels, linePixelsNext;
   int pixelN, pixelE, pixelS, pixelW, pixelSW, pixelSE, pixelNW, pixelNE;
   // Stretch 6x10 characters to 12x20, so we can do character rounding 
   // 96 alphanum chars + 64 cont. graphic chars + 64 sep. graphic chars
   for (int i = 0; i < (96 + 64 + 64) * CHAR_HEIGHT_ORIG; i += CHAR_HEIGHT_ORIG) 
   { 
      linePixelsPrev = 0;
      linePixels = 0;
      linePixelsNext = SAA5050_fnt[i] << CHAR_WIDTH_ORIG;
      for (int j = 0; j < CHAR_HEIGHT_ORIG; ++j) 
      {
         linePixelsPrev = linePixels >> CHAR_WIDTH_ORIG;
         linePixels = linePixelsNext >> CHAR_WIDTH_ORIG;
         linePixelsNext = j < (CHAR_HEIGHT_ORIG-1) ? SAA5050_fnt[i + j + 1] : 0;
         for (int k = 0; k < CHAR_WIDTH_ORIG; ++k)
         {
            if (linePixels & 0x20) // bit 6 set = pixel set
            {
               *p = *(p+1) = *(p+CHAR_WIDTH) = *(p+CHAR_WIDTH+1) = 0xff;
            }
            else if (i < 96 * CHAR_HEIGHT_ORIG) // character rounding (only for alphanumeric chars)
            { 
               // for character rounding, look at 8 pixels around current pixel
               pixelN  = linePixelsPrev & 0x20;
               pixelE  = linePixels     & 0x10;
               pixelS  = linePixelsNext & 0x20;
               pixelW  = linePixels     & 0x40;
               pixelNE = linePixelsPrev & 0x10;
               pixelSE = linePixelsNext & 0x10;
               pixelSW = linePixelsNext & 0x40;
               pixelNW = linePixelsPrev & 0x40;
               // rounding in NW direction
               if (pixelN && pixelW && !pixelNW) *p = 0xff;
               // rounding in NE direction
               if (pixelN && pixelE && !pixelNE) *(p+1) = 0xff;
               // rounding in SE direction
               if (pixelS && pixelE && !pixelSE) *(p+CHAR_WIDTH+1) = 0xff;
               // rounding in SW direction
               if (pixelS && pixelW && !pixelSW) *(p+CHAR_WIDTH) = 0xff;
            }
            //process next pixel to the right
            p += 2;
            linePixelsPrev<<=1;
            linePixels<<=1;
            linePixelsNext<<=1;
         }
         p += CHAR_WIDTH;
      }
   }
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
   static int soundstate = 0;
   static int smooth = 0;

   for (int i=0; i<buf_size; ++i) 
   {
      if (sound_buf[i]) 
      {
         soundstate = sound_buf[i] << 13;
         sound_buf[i] = 0;
      }
      //low pass filtering
      smooth = (smooth << 1) + soundstate - smooth; 
      smooth >>= 1;
      audio_batch_buf[2*i] = audio_batch_buf[2*i+1] = smooth;
   }
   soundstate >>= 1;
   audio_batch_cb(audio_batch_buf, buf_size);
}

static void PushKey(byte p2000KeyCode)
{
  byte mRow = p2000KeyCode / 8;
  byte mCol = 1 << (p2000KeyCode % 8);
  if (mRow < 10) KeyMap[mRow] &= ~mCol;
}

static void ReleaseKey(byte p2000KeyCode)
{
  byte mRow = p2000KeyCode / 8;
  byte mCol = 1 << (p2000KeyCode % 8);
  if (mRow < 10) KeyMap[mRow] |= mCol;
}

/****************************************************************************/
/*** Poll the keyboard on every interrupt                                 ***/
/****************************************************************************/
void Keyboard(void) 
{
   input_poll_cb();

   /*************************/
   /* handle keyboard input */
   /*************************/
   bool shiftPressed = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LSHIFT) || input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RSHIFT);
   for (int i = 0; i < key_map_len; i++) 
   {
      if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, key_map[i]))
      {
         if (key_map[i] != RETROK_QUOTE || shiftPressed)
         PushKey(i);
      }
      else
         ReleaseKey(i);
   }

   /***********************************/
   /* map joypad input to key presses */
   /***********************************/
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP)
      || input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R)
      || input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
      PushKey(P2000_KEYCODE_UP); // Fraxxon uses Up-key for moving spaceship to the right 
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
      PushKey(P2000_KEYCODE_DOWN);
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)
      || input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L)
      || input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
      PushKey(P2000_KEYCODE_LEFT);
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)
      || input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R)
      || input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
      PushKey(P2000_KEYCODE_RIGHT);
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) 
      || input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))
      PushKey(P2000_KEYCODE_SPACE);
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
   {
      //emulate <START> key press
      PushKey(P2000_KEYCODE_LSHIFT);
      PushKey(P2000_KEYCODE_NUM_3);
   }
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT))
   {
      //emulate <STOP> key press
      PushKey(P2000_KEYCODE_LSHIFT);
      PushKey(P2000_KEYCODE_NUM_PERIOD);
   }

}

/****************************************************************************/
/*** Put a character in the display buffer                                ***/
/****************************************************************************/
void PutChar(int x, int y, int c, int fg, int bg, int si)
{
   int K = c + (fg << 8) + (bg << 16) + (si << 24);
   if (K == display_char_buf[y * 40 + x]) //skip if character is already on screen
      return;

   display_char_buf[y * 40 + x] = K;
   uint32_t *buf = frame_buf;
   byte *p = font_buf + c * CHAR_WIDTH * CHAR_HEIGHT + (si >> 1) * CHAR_WIDTH * CHAR_HEIGHT/2;

   for (unsigned j = 0; j < CHAR_HEIGHT; j++)
   {
      for (unsigned i = 0; i < CHAR_WIDTH; i++)
         buf[(x * CHAR_WIDTH + i) + (y * CHAR_HEIGHT + j) * VIDEO_BUFFER_WIDTH] = *p++ ? PalXRGB[fg] : PalXRGB[bg];
      if (si && (j & 1) == 0)
         p -= CHAR_WIDTH;
   }
}

/****************************************************************************/
/*** Push the display buffer for actual rendering on every interrupt      ***/
/****************************************************************************/
void PutImage (void)
{
   video_cb(frame_buf, VIDEO_BUFFER_WIDTH, VIDEO_BUFFER_HEIGHT, VIDEO_BUFFER_WIDTH << 2);
}

void retro_init(void)
{
   //log_cb(RETRO_LOG_INFO, "retro_init called\n");
   frame_buf = calloc(VIDEO_BUFFER_WIDTH * VIDEO_BUFFER_HEIGHT, sizeof(uint32_t));
   font_buf = calloc(NUMBER_OF_CHARS * CHAR_WIDTH * CHAR_HEIGHT, sizeof(byte));
   display_char_buf = calloc(80 * 24, sizeof(int));
   buf_size = SAMPLE_RATE / IFreq;
   sound_buf = calloc(buf_size, sizeof(char));
   audio_batch_buf = calloc(buf_size * 2, sizeof(int16_t)); // * 2 for stereo
   InitP2000(monitor_rom, BasicNL_rom);
}

void retro_deinit(void)
{
   TrashP2000();
   free(frame_buf);
   free(font_buf);
   free(sound_buf);
   free(audio_batch_buf);
   free(display_char_buf);
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
   info->library_version  = "v0.9";
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
      .base_width   = 320,
      .base_height  = 240,
      .max_width    = 320,
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

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   bool no_content = true;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   // create empty keyboard callback, to allow for "Game Focus" detection
   struct retro_keyboard_callback cb_kb = { NULL };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb_kb);

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
}

void retro_reset(void)
{
   Z80_Reset();
}

void retro_run(void)
{
   Z80_Execute();
}

bool retro_load_game(const struct retro_game_info *info)
{
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;

   static const struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up key"      },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down key"    },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left key"    },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right key"   },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Space key"   },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Space key"   },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Left key"    },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Right/Up key"},
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Left key"    },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Right/Up key"},
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "<Stop> key"  },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "<Start> key" },
   };
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)desc);

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   // Load the cassette file (read-only)
   if (info && info->path) 
   {
      InsertCassette(info->path, fopen(info->path, "rb"), true);
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
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}
