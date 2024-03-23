#include <stdio.h>
#include <stdlib.h>
#include "libretro.h"
#include "../../libretro-common/include/retro_timers.h"
#include "p2000t_roms.h"
#include "../Z80.h"
#include "../P2000.h"
#include "../Common.h"

#define VIDEO_BUFFER_WIDTH 480
#define VIDEO_BUFFER_HEIGHT 480
#define SAMPLE_RATE 30000
#define NUMBER_OF_CHARS 224
#define CHAR_WIDTH 12
#define CHAR_HEIGHT 20
#define CHAR_WIDTH_ORIG 6
#define CHAR_HEIGHT_ORIG 10

static uint32_t *frame_buf;
static byte *font_buf;
static signed char *soundbuf = NULL;
static int16_t *audioOutBuffer;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static int *OldCharacter;          /* Holds characters on the screen        */
static int buf_size;

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

/**
  * P2000 required function implementations
 **/
void SyncEmulation(void) // not needed, as frontend will take care of syncing
{
}

void Pause(int ms) 
{
   retro_sleep(ms);
}

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
/*** This function is called when the sound register is written to        ***/
/****************************************************************************/
void Sound(int toggle)
{
   static int last=-1;
   int pos,val;

   if (toggle!=last) {
      last=toggle;
      pos=(buf_size-1)-((buf_size-1)*Z80_ICount/Z80_IPeriod);
      val=(toggle)? -1 : 1;
      soundbuf[pos]=val;
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
      if (soundbuf[i]) 
      {
         soundstate = soundbuf[i] << 14;
         soundbuf[i] = 0;
      }
      //low pass filtering
      smooth = (smooth << 1) + soundstate - smooth; 
      smooth >>= 1;
      audioOutBuffer[2*i] = audioOutBuffer[2*i+1] = smooth;
   }
   soundstate >>= 1;
   audio_batch_cb(audioOutBuffer, buf_size);
}

void Keyboard(void) 
{
   // not yet implemented
}

void PutImage (void)
{
   video_cb(frame_buf, VIDEO_BUFFER_WIDTH, VIDEO_BUFFER_HEIGHT, VIDEO_BUFFER_WIDTH << 2);
}

/****************************************************************************/
/*** Put a character in the display buffer                                ***/
/****************************************************************************/
void PutChar(int x, int y, int c, int fg, int bg, int si)
{
   int K = c + (fg << 8) + (bg << 16) + (si << 24);
   if (K == OldCharacter[y * 40 + x])
      return;

   OldCharacter[y * 40 + x] = K;
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

void retro_init(void)
{
   log_cb(RETRO_LOG_INFO, "retro_init\n");
   frame_buf = calloc(VIDEO_BUFFER_WIDTH * VIDEO_BUFFER_HEIGHT, sizeof(uint32_t));
   font_buf = calloc(NUMBER_OF_CHARS * CHAR_WIDTH * CHAR_HEIGHT, sizeof(byte));
   OldCharacter = calloc(80 * 24, sizeof(int));
   buf_size = SAMPLE_RATE / IFreq;
   soundbuf = calloc(buf_size, sizeof(char));
   audioOutBuffer = calloc(buf_size * 2, sizeof(int16_t)); // 2 for stereo
   InitP2000(monitor_rom, BasicNL_rom);
}

void retro_deinit(void)
{
   TrashP2000();
   free(frame_buf);
   free(font_buf);
   free(soundbuf);
   free(audioOutBuffer);
   free(OldCharacter);
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
   info->need_fullpath    = true;
   info->valid_extensions = "cas";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing = (struct retro_system_timing) {
      .fps = 50.0,
      .sample_rate = (float)SAMPLE_RATE,
   };

   info->geometry = (struct retro_game_geometry) {
      .base_width   = VIDEO_BUFFER_WIDTH,
      .base_height  = VIDEO_BUFFER_HEIGHT,
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
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
}

void retro_reset(void)
{
   log_cb(RETRO_LOG_INFO, "retro_reset\n");
   //x_coord = 0;
   //y_coord = 0;
}

// static void update_input(void)
// {
//    input_poll_cb();

//    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
//    {
//       /* Stub */
//    }
// }

static void check_variables(void)
{
}

void retro_run(void)
{
   Z80_Execute();

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}

bool retro_load_game(const struct retro_game_info *info)
{
   log_cb(RETRO_LOG_INFO, "retro_load_game\n");
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   check_variables();
   InsertCassette(info->path, fopen(info->path, "rb"), true);
   return true;
}

void retro_unload_game(void)
{
   log_cb(RETRO_LOG_INFO, "retro_unload_game\n");
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
