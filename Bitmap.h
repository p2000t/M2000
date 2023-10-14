/****************************************************************************/
/**                                                                        **/
/**                                Bitmap.h                                **/
/**                                                                        **/
/** Routines to create screen shots in Windows bitmap format               **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996                                     **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

/****************************************************************************/
/* Return values:                                                           */
/* >=0 - Succes                                                             */
/* -1  - File creation error                                                */
/* -2  - Write error                                                        */
/****************************************************************************/
int WriteBitmap (char *szFileName, int nBitsPerPixel, int nColoursUsed,
                 int nWidthImageBuffer,
                 int nWidthImage, int nHeightImage,
                 char *pBitmapBits, char *pPalette);
