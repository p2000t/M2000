/****************************************************************************/
/**                                                                        **/
/**                                  INT.c                                 **/
/**                                                                        **/
/** Stack and interrupt manipulation routines for DJGPP                    **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996                                     **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <stdlib.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <stdio.h>
#include <string.h>

#include "INT.h"

#define MIN_STACK_SIZE          1024    /* Minumum stack size */
#define MAX_STACK_SIZE          524288  /* Maximum stack size */
#define MIN_STACK_NR            4       /* Minimum number of stacks */
#define MAX_STACK_NR            256     /* Maximum number of stacks */

static unsigned **StackPtr;     /* Pointer to an array of stack pointers */
static int StackSize;           /* Size of a stack */
static int NumStacks;           /* Number of stacks */
static int NumStacksUsed;       /* Number of stacks currently used */
static int NumFailed;           /* Number of allocations currently failed */

/* Number of calls to AllocStack() that failed */
static int TotalFailed=0;
/* Number of calls to AllocStack() that succeeded */
static int TotalSuccess=0;
/* Maximum number of stacks used simultaniously */
static int MaxStacksUsed=0;
/* Maximum stack size used */
static int MaxStackSize=0;

/* Lock variables */
static int LockData (void *addr,unsigned size)
{
 unsigned long base;
 __dpmi_meminfo info;
 if (__dpmi_get_segment_base_address(_go32_my_ds(),&base)==-1)
  return 0;
 info.handle=0;
 info.address=base+(unsigned)addr;
 info.size=size;
 if (__dpmi_lock_linear_region(&info)==-1)
  return 0;
 return 1;
}

/* Unlock variables */
static int UnlockData (void *addr,unsigned size)
{
 unsigned long base;
 __dpmi_meminfo info;
 if (__dpmi_get_segment_base_address(_go32_my_ds(),&base)==-1)
  return 0;
 info.handle=0;
 info.address=base+(unsigned)addr;
 info.size=size;
 if (__dpmi_unlock_linear_region(&info)==-1)
  return 0;
 return 1;
}

/****************************************************************************/
/*** GetStackInfo()                                                       ***/
/*** Get some info useful for debugging                                   ***/
/****************************************************************************/
void GetStackInfo (int *nFailed,int *nSuccess,int *nUsed,int *nMaxSize)
{
 *nFailed=TotalFailed;
 *nSuccess=TotalSuccess;
 *nUsed=MaxStacksUsed;
 *nMaxSize=MaxStackSize;
}

/****************************************************************************/
/*** ExitStacks()                                                         ***/
/*** Free all resources allocated by InitStacks()                         ***/
/****************************************************************************/
void ExitStacks (void)
{
 int i,j;
 int maxsize=0;
 unsigned char *p;
 if (!StackPtr)
  return;
 for (i=0;i<NumStacks;++i)
  if (StackPtr[i])
  {
   p=(unsigned char*)StackPtr[i];
   for (j=0;j<StackSize;++j)
    if (p[j]!=0xF6) break;
   j=(StackSize-j)&(~3);
   if (maxsize<j) maxsize=j;
   UnlockData (StackPtr[i],StackSize);
   free (StackPtr[i]);
  }
  else
   break;
 UnlockData (StackPtr,NumStacks*sizeof(unsigned*));
 free (StackPtr);
 StackPtr=0;
 MaxStackSize=maxsize;
}

/****************************************************************************/
/*** InitStacks()                                                         ***/
/*** Allocate stacks and initialise structures                            ***/
/****************************************************************************/
int InitStacks (int nr,int size)
{
 int i;
 if (StackPtr)
  return 1;                     /* already called */
 if (size<MIN_STACK_SIZE)
  size=MIN_STACK_SIZE;
 if (size>MAX_STACK_SIZE)
  size=MAX_STACK_SIZE;
 if (nr<MIN_STACK_NR)
  nr=MIN_STACK_NR;
 if (nr>MAX_STACK_NR)
  nr=MAX_STACK_NR;
 if (size&3)
  size=(size&(~3))+4;           /* stack must be dword-aligned */
 StackPtr=malloc (nr*sizeof(unsigned*));
 if (!StackPtr)
  return 0;
 if (!LockData (StackPtr,nr*sizeof(unsigned*)))
  return 0;
 StackSize=size;
 NumStacks=nr;
 for (i=0;i<nr;++i)
 {
  StackPtr[i]=malloc(size);
  if (!StackPtr[i] || !LockData (StackPtr[i],size))
  {
   ExitStacks ();
   return 0;
  }
  memset (StackPtr[i],0xF6,size);
 }
 NumStacksUsed=0;
 NumFailed=0;
 return 1;
}

/****************************************************************************/
/*** AllocStack()                                                         ***/
/*** Allocate a stack, return 0 when no stack could be allocated          ***/
/*** FreeStack() must be called wether a stack could be allocated or not  ***/
/*** Clobbers all registers                                               ***/
/****************************************************************************/
asm ("
.globl _AllocStack
_AllocStack:
 popl   %esi
 movl   _NumStacksUsed,%eax
 cmpl   _NumStacks,%eax
 jae    AllocStack_Failed
 incl   %eax
 movl   %eax,_NumStacksUsed
 cmpl   _MaxStacksUsed,%eax
 jbe    Alloc_1
 movl   %eax,_MaxStacksUsed
Alloc_1:
 incl   _TotalSuccess
 movl   _StackPtr,%ebx
 decl   %eax
 leal   (%ebx,%eax,4),%ebx
 movl   (%ebx),%ebx
 addl   _StackSize,%ebx
 subl   $64,%ebx
 movw   %ss,%cx
 movl   %esp,%edx
 movw   %ds,%ax
 movw   %ax,%ss
 movl   %ebx,%esp
 pushl  %edx
 pushw  %cx
 movl   $1,%eax
 jmpl   %esi
AllocStack_Failed:
 incl   _NumFailed
 incl   _TotalFailed
 subl   %eax,%eax
 jmpl   %esi

/****************************************************************************/
/*** FreeStack()                                                          ***/
/*** Frees a stack previously allocated by AllocStack()                   ***/
/*** Clobbers all registers                                               ***/
/****************************************************************************/
.globl _FreeStack
_FreeStack:
 popl   %esi
 cmpl   $0,_NumFailed
 jne    FreeStack_Failed
 popw   %ax
 popl   %edx
 movw   %ax,%ss
 movl   %edx,%esp
 decl   _NumStacksUsed
 jmpl   %esi
FreeStack_Failed:
 decl   _NumFailed
 jmpl   %esi
");

/****************************************************************************/
/*** The interrupt handler definition                                     ***/
/****************************************************************************/
#define FILL 0x00               /* must be unique            */
static unsigned char handler[] =
{
 0x60,                                  /* pushad                       */
 0x0F,0xA8,                             /* push   gs                    */
 0x0F,0xA0,                             /* push   fs                    */
 0x06,                                  /* push   es                    */
 0x1E,                                  /* push   ds                    */
 0x2E,0xA1,FILL,FILL,FILL,FILL,         /* mov    eax,[cs:_cs_alias]    */
 0x8E,0xD8,                             /* mov    ds,ax                 */
 0x8E,0xC0,                             /* mov    es,ax                 */
 0x8E,0xE0,                             /* mov    fs,ax                 */
 0x8E,0xE8,                             /* mov    gs,ax                 */
 0xFC,                                  /* cld                          */
 0xE8,FILL,FILL,FILL,FILL,              /* call   _AllocStack           */
 0x85,0xC0,                             /* test   eax,eax               */
 0x74,0x11,                             /* jz     _0                    */
 0xE8,FILL,FILL,FILL,FILL,              /* call   _interrupt_handler    */
 0x85,0xC0,                             /* test   eax,eax               */
 0x74,0x08,                             /* jz     _0                    */
 0x9C,                                  /* pushfd                       */
 0x9A,FILL,FILL,FILL,FILL,FILL,FILL,    /* call   _old_handler          */
                                        /* _0:                          */
 0xFA,                                  /* cli                          */
 0xE8,FILL,FILL,FILL,FILL,              /* call   _FreeStack            */
 0x1F,                                  /* pop    ds                    */
 0x07,                                  /* pop    es                    */
 0x0F,0xA1,                             /* pop    fs                    */
 0x0F,0xA9,                             /* pop    gs                    */
 0x61,                                  /* popad                        */
 0xCF                                   /* iretd                        */
};

static int cs_alias;            /* selector of our data segment */

/****************************************************************************/
/*** SetInt()                                                             ***/
/*** Allocate an interrupt handler and set the interrupt vector           ***/
/*** The user-written interrupt handler must return 0 if the old          ***/
/*** interrupt vector isn't to be called, and 1 otherwise                 ***/
/****************************************************************************/
int SetInt (int nr,_go32_dpmi_seginfo *info)
{
 int i;
 unsigned char *p;
 _go32_dpmi_seginfo oldinfo;
 /* Initialise cs_alias */
 cs_alias=_my_ds ();
 /* Make sure InitStacks() is called */
 if (!InitStacks (8,8192))
  return 0;
 /* Allocate and lock memory */
 p=malloc (sizeof(handler));
 if (!p)
  return 0;
 if (!LockData (p,sizeof(handler)))
  return 0;
 /* Get old interrupt vector */
 _go32_dpmi_get_protected_mode_interrupt_vector (nr,&oldinfo);
 /* Copy the handler structure to the allocated handler */
 memcpy (p,handler,sizeof(handler));
 /* Initialise the handler */
 i=-1;
 while (p[++i]!=FILL);
 *(unsigned*)(p+i)=(unsigned)&cs_alias;
 i+=3;
 while (p[++i]!=FILL);
 *(unsigned*)(p+i)=((unsigned)AllocStack)-((unsigned)(p+i+4));
 i+=3;
 while (p[++i]!=FILL);
 *(unsigned*)(p+i)=((unsigned)info->pm_offset)-((unsigned)(p+i+4));
 i+=3;
 while (p[++i]!=FILL);
 *(unsigned*)(p+i)=(unsigned)oldinfo.pm_offset;
 *(unsigned short*)(p+i+4)=(unsigned short)oldinfo.pm_selector;
 i+=5;
 while (p[++i]!=FILL);
 *(unsigned*)(p+i)=((unsigned)FreeStack)-((unsigned)(p+i+4));
 /* Fill the info structure */
 info->pm_offset=(unsigned)p;
 info->pm_selector=_go32_my_cs();
 /* Set the interrupt vector */
 _go32_dpmi_set_protected_mode_interrupt_vector (nr,info);
 /* return success */
 return 1;
}

/****************************************************************************/
/*** ResetInt()                                                           ***/
/*** Free resources allocated by SetInt()                                 ***/
/****************************************************************************/
int ResetInt (int nr,_go32_dpmi_seginfo *info)
{
 unsigned char *p;
 int i;
 /* get pointer */
 p=(unsigned char*)info->pm_offset;
 /* make sure it's valid */
 if (!p) return 0;
 /* check if it points to an interrupt handler */
 if (*(unsigned*)p!=*(unsigned*)handler) return 0;
 /* get old interrupt vector */
 i=-1;
 while (handler[++i]!=FILL);
 i+=3;
 while (handler[++i]!=FILL);
 i+=3;
 while (handler[++i]!=FILL);
 i+=3;
 while (handler[++i]!=FILL);
 info->pm_offset=*(unsigned*)(p+i);
 info->pm_selector=*(unsigned short*)(p+i+4);
 /* set old interrupt vector */
 _go32_dpmi_set_protected_mode_interrupt_vector (nr,info);
 /* unlock and free memory */
 UnlockData (p,sizeof(handler));
 free (p);
 return 1;
}
