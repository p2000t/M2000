#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "p2000t_roms.h"
#include "../Z80.h"
#include "libretro.h"

#define VIDEO_BUFFER_WIDTH 480
#define VIDEO_BUFFER_HEIGHT 480

#define NUMBER_OF_CHARS 224
#define CHAR_WIDTH 12
#define CHAR_HEIGHT 20
#define CHAR_WIDTH_ORIG 6
#define CHAR_HEIGHT_ORIG 10

static uint32_t *frame_buf;
static byte *font_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static unsigned phase;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void load_SAA5050_font()
{
   byte *p = font_buf;
   int linePixelsPrev, linePixels, linePixelsNext;
   int pixelN, pixelE, pixelS, pixelW, pixelSW, pixelSE, pixelNW, pixelNE;
   // Stretch 6x10 characters to 12x20, so we can do character rounding 
   // 96 alphanumeric chars + 64 continuous graphic chars + 64 seperated graphic chars = 224 chars in total
   for (int i = 0; i < (96 + 64 + 64) * CHAR_HEIGHT_ORIG; i+=CHAR_HEIGHT_ORIG) 
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
               *p = *(p+1) = *(p+CHAR_WIDTH) = *(p+CHAR_WIDTH+1) = 0xff;
            else if (i < 96 * CHAR_HEIGHT_ORIG) // character rounding (only for alphanumeric chars)
            { 
               // for character rounding, look at 8 pixels around current pixel
               pixelN = linePixelsPrev & 0x20;
               pixelE = linePixels & 0x10;
               pixelS = linePixelsNext & 0x20;
               pixelW = linePixels & 0x40;
               pixelNE = linePixelsPrev & 0x10;
               pixelSE = linePixelsNext & 0x10;
               pixelSW = linePixelsNext & 0x40;
               pixelNW = linePixelsPrev & 0x40;
               // rounding in NW direction
               if (pixelN && pixelW && !pixelNW)
                  *p = 0xff;
               // rounding in NE direction
               if (pixelN && pixelE && !pixelNE)
                  *(p+1) = 0xff;
               // rounding in SE direction
               if (pixelS && pixelE && !pixelSE)
                  *(p+CHAR_WIDTH+1) = 0xff;
               // rounding in SW direction
               if (pixelS && pixelW && !pixelSW)
                  *(p+CHAR_WIDTH) = 0xff;
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
}

void retro_init(void)
{
   //log_cb(RETRO_LOG_INFO, "M2000 Init");
   frame_buf = calloc(VIDEO_BUFFER_WIDTH * VIDEO_BUFFER_HEIGHT, sizeof(uint32_t));
   font_buf = calloc(NUMBER_OF_CHARS * CHAR_WIDTH * CHAR_HEIGHT, sizeof(byte));
   load_SAA5050_font();
}

void retro_deinit(void)
{
   free(frame_buf);
   free(font_buf);
   frame_buf = NULL;
   font_buf = NULL;
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
   info->library_name     = "M2000 - Philips P2000T Emulator";
   info->library_version  = "v0.9";
   info->need_fullpath    = false;
   info->valid_extensions = "cas|p2000t";
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   float aspect = 4.0f / 3.0f;
   float sampling_rate = 30000.0f; //44100 ?

   info->timing = (struct retro_system_timing) {
      .fps = 50.0,
      .sample_rate = sampling_rate,
   };

   info->geometry = (struct retro_game_geometry) {
      .base_width   = VIDEO_BUFFER_WIDTH,
      .base_height  = VIDEO_BUFFER_HEIGHT,
      .aspect_ratio = aspect,
   };
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   bool no_content = true;

   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   //x_coord = 0;
   //y_coord = 0;
}

static void update_input(void)
{
   input_poll_cb();

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
   {
      /* Stub */
   }
}

static void render(void)
{
   /* Try rendering straight into VRAM if we can. */
   uint32_t *buf = NULL;
   unsigned stride = 0;
   struct retro_framebuffer fb = {0};
   fb.width = VIDEO_BUFFER_WIDTH;
   fb.height = VIDEO_BUFFER_HEIGHT;
   fb.access_flags = RETRO_MEMORY_ACCESS_WRITE;
   if (environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb) && fb.format == RETRO_PIXEL_FORMAT_XRGB8888)
   {
      buf = fb.data;
      stride = fb.pitch >> 2;
   }
   else
   {
      buf = frame_buf;
      stride = VIDEO_BUFFER_WIDTH;
   }

   uint32_t color_r = 0xff << 16;
   uint32_t color_g = 0xff <<  8;
   uint32_t color_b = 0xff;

   uint32_t *line = buf;
   for (unsigned y = 0; y < VIDEO_BUFFER_HEIGHT; y++, line += stride)
   {
      unsigned index_y = (y >> 4) & 1;
      for (unsigned x = 0; x < VIDEO_BUFFER_WIDTH; x++)
      {
         unsigned index_x = (x >> 4) & 1;
         line[x] = (index_y ^ index_x) ? color_r : color_g;
      }
   }

   byte *p = font_buf + 33 * CHAR_WIDTH * CHAR_HEIGHT;
   for (unsigned y=0;y<CHAR_HEIGHT;y++)
   {
      for (unsigned x = 0; x < CHAR_WIDTH; x++)
      {
         buf[x + y * VIDEO_BUFFER_WIDTH] = *p++ ? color_b : 0;
      }
   }

   video_cb(buf, VIDEO_BUFFER_WIDTH, VIDEO_BUFFER_HEIGHT, stride << 2);
}

static void check_variables(void)
{
}

static void audio(void)
{
   for (unsigned i = 0; i < 30000 / 50; i++, phase++)
   {
      int16_t val = 0x800 * sinf(2.0f * 3.14159265 * phase * 300.0f / 30000.0f);
      audio_cb(val, val);
   }

   phase %= 100;
}

void retro_run(void)
{
   update_input();
   render();
   audio();

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}


bool retro_load_game(const struct retro_game_info *info)
{
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   check_variables();

   (void)info;
   return true;
}

void retro_unload_game(void)
{
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
   return 2;
}

bool retro_serialize(void *data_, size_t size)
{
   if (size < 2)
      return false;

   uint8_t *data = data_;
   data[0] = 0; //x_coord;
   data[1] = 0; //y_coord;
   return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
   if (size < 2)
      return false;

   //const uint8_t *data = data_;
   // x_coord = data[0] & 31;
   // y_coord = data[1] & 31;
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
