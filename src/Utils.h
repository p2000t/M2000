#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "P2000.h"

char szBitmapFile[256];            /* Next screen shot file                 */
char szVideoRamFile[256];          /* Next video memory dump file           */

/**************************************************************************/
/*** Creates next filename for the Screenshot and VideoMem output files ***/
/**************************************************************************/
static int NextOutputFile(char *filename)
{
  int ix = strlen(filename) - 5;
  if (filename[ix] == '9')
  {
    filename[ix] = '0';
    ix--;
    if (filename[ix] == '9')
    {
      filename[ix] = '0';
      ix--;
      filename[ix]++;
    }
    else
    {
      filename[ix]++;
      if (filename[ix] == '0')
        return 0;
    }
  }
  else
    filename[ix]++;
  return 1;
}

static void InitScreenshotFile()
{
  FILE *f;
  strcpy(szBitmapFile, "M2000.bmp");
  while ((f = fopen(szBitmapFile, "rb")) != NULL)
  {
    fclose(f);
    if (!NextOutputFile(szBitmapFile))
      break;
  }
  if (Verbose)
    printf("  Next screenshot will be %s\n", szBitmapFile);
}

static void InitVRAMFile()
{
  FILE *f;
  strcpy(szVideoRamFile, "VRAM2000.bin");
  while ((f = fopen(szVideoRamFile, "rb")) != NULL)
  {
    fclose(f);
    if (!NextOutputFile(szVideoRamFile))
      break;
  }
  if (Verbose)
    printf("  Next video RAM dump will be %s\n", szVideoRamFile);
}

static int WriteVRAMFile() 
{
  FILE *out;
  if ((out = fopen(szVideoRamFile, "wb")) != NULL)
  {
    printf("  Writing video memory to %s...", szVideoRamFile);
    fwrite(VRAM, 1, 24 * 80, out);
    fclose(out);
    printf("OK\n");
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}