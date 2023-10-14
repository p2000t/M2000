/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                 Asm.h                                ***/
/***                                                                      ***/
/*** This file contains prototypes for MS-DOS specific assembler routines ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

unsigned JoyGetPos (void);
void StartTimer (void);
unsigned ReadTimer (void);
void RestoreTimer (void);
void nofunc (void);
void __enable (void);
void __disable (void);
