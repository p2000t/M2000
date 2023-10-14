/****************************************************************************/
/**                                                                        **/
/**                                Bitmap.c                                **/
/**                                                                        **/
/** Routines to create screen shots in Windows bitmap format               **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996                                     **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <stdio.h>
#include "Bitmap.h"

static void putint (char *p,int n)
{
 p[0]=n&255;
 p[1]=(n>>8)&255;
 p[2]=(n>>16)&255;
 p[3]=(n>>24)&255;
}

/****************************************************************************/
/* Return values:                                                           */
/* >=0 - Succes                                                             */
/* -1  - File creation error                                                */
/* -2  - Write error                                                        */
/****************************************************************************/
int WriteBitmap (char *szFileName, int nBitsPerPixel, int nColoursUsed,
                 int nWidthImageBuffer,
                 int nWidthImage, int nHeightImage,
                 char *pBitmapBits, char *pPalette)
{
 int i,j,nPadBytes,nImageBytes,nBufBytes;
 FILE *f;
 static char bitmapheader[]=
 {
  /* BITMAPFILEHEADER */
  0x42,0x4D,               /* 0 type='BM'                                   */
  0x00,0x00,0x00,0x00,     /* 2 size of file in bytes                       */
  0x00,0x00,               /* 6 reserved                                    */
  0x00,0x00,               /* 8 reserved                                    */
  0x00,0x00,0x00,0x00,     /* 10 offset of bitmap data                      */
  /* BITMAPINFOHEADER */
  0x28,0x00,0x00,0x00,     /* 14 size of this structure in bytes            */
  0x00,0x00,0x00,0x00,     /* 18 width of the image in pixels               */
  0x00,0x00,0x00,0x00,     /* 22 height of the image in pixels              */
  0x01,0x00,               /* 26 number of planes                           */
  0x00,0x00,               /* 28 bits per pixel                             */
  0x00,0x00,0x00,0x00,     /* 30 compression method                         */
  0x00,0x00,0x00,0x00,     /* 34 size of the image in bytes                 */
                           /* set to 0 if no compression is used            */
  0xE8,0x05,0x00,0x00,     /* 38 horizontal resolution in pixels per meter  */
  0xE8,0x05,0x00,0x00,     /* 42 vertical resolution in pixels per meter    */
  0x00,0x00,0x00,0x00,     /* 46 number of colours used in the colour table */
  0x00,0x00,0x00,0x00      /* 50 number of colours used in the image        */
 };
 f=fopen (szFileName,"wb");
 if (!f) return -1;
 bitmapheader[28]=nBitsPerPixel;
 putint (bitmapheader+46,nColoursUsed);
 putint (bitmapheader+50,nColoursUsed);
 putint (bitmapheader+18,nWidthImage);
 putint (bitmapheader+22,nHeightImage);
 putint (bitmapheader+10,sizeof(bitmapheader)+nColoursUsed*4);
 nBufBytes=(nWidthImageBuffer*nBitsPerPixel+7)/8;
 i=nImageBytes=(nWidthImage*nBitsPerPixel+7)/8;
 nPadBytes=(4-(i&3))&3;
 i+=nPadBytes;
 putint (bitmapheader+2,(i*nHeightImage)+sizeof(bitmapheader)+nColoursUsed*4);
 if (fwrite(bitmapheader,sizeof(bitmapheader),1,f)!=1)
 {
  fclose (f);
  return -2;
 }
 for (i=0;i<nColoursUsed;++i)
 {
  if (fputc(pPalette[i*3+2],f)==EOF)
  {
   fclose (f);
   return -2;
  }
  if (fputc(pPalette[i*3+1],f)==EOF)
  {
   fclose (f);
   return -2;
  }
  if (fputc(pPalette[i*3+0],f)==EOF)
  {
   fclose (f);
   return -2;
  }
  if (fputc(0,f)==EOF)
  {
   fclose (f);
   return -2;
  }
 }
 for (i=nHeightImage-1;i>=0;--i)
 {
  if (fwrite(pBitmapBits+nBufBytes*i,nImageBytes,1,f)!=1)
  {
   fclose (f);
   return -2;
  }
  for (j=0;j<nPadBytes;++j)
  {
   if (fputc(0,f)==EOF)
   {
    fclose (f);
    return -2;
   }
  }
 }
 fclose (f);
 return 0;
}
