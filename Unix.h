/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                 Unix.h                               ***/
/***                                                                      ***/
/*** This file contains various Unix function prototypes used by both the ***/
/*** X-Windows and the Linux/SVGALib implementations                      ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#ifdef SOUND
void InitSound (int mode);
void TrashSound (void);
void IncreaseSoundVolume (void);
void DecreaseSoundVolume (void);
void WriteSound (int toggle);
int Sound_FlushSound (void);
extern int mastervolume;
#endif

#ifdef JOYSTICK
void InitJoystick (int mode);
void TrashJoystick (void);
int ReadJoystick (void);
#endif

int ReadTimer (void);
