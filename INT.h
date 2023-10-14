/****************************************************************************/
/**                                                                        **/
/**                                  INT.h                                 **/
/**                                                                        **/
/** Stack and interrupt manipulation routines for DJGPP                    **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996                                     **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <dpmi.h>
#include <go32.h>

/****************************************************************************/
/*** GetStackInfo()                                                       ***/
/*** Get some info useful for debugging                                   ***/
/****************************************************************************/
void GetStackInfo (int *nFailed,int *nSuccess,int *nUsed,int *nMaxSize);

/****************************************************************************/
/*** ExitStacks()                                                         ***/
/*** Free all resources allocated by InitStacks()                         ***/
/****************************************************************************/
void ExitStacks (void);

/****************************************************************************/
/*** InitStacks()                                                         ***/
/*** Allocate stacks and initialise structures                            ***/
/****************************************************************************/
int InitStacks (int nr,int size);

/****************************************************************************/
/*** AllocStack()                                                         ***/
/*** Allocate a stack, return 0 when no stack could be allocated          ***/
/*** FreeStack() must be called wether a stack could be allocated or not  ***/
/*** Clobbers all registers                                               ***/
/****************************************************************************/
void AllocStack (void);

/****************************************************************************/
/*** FreeStack()                                                          ***/
/*** Frees a stack previously allocated by AllocStack()                   ***/
/*** Clobbers all registers                                               ***/
/****************************************************************************/
void FreeStack (void);

/****************************************************************************/
/*** SetInt()                                                             ***/
/*** Allocate an interrupt handler and set the interrupt vector           ***/
/*** The user-written interrupt handler must return 0 if the old          ***/
/*** interrupt vector isn't to be called, and 1 otherwise                 ***/
/****************************************************************************/
int SetInt (int nr,_go32_dpmi_seginfo *info);

/****************************************************************************/
/*** ResetInt()                                                           ***/
/*** Free resources allocated by SetInt()                                 ***/
/****************************************************************************/
int ResetInt (int nr,_go32_dpmi_seginfo *info);
