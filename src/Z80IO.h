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
/*   Copyright (C) 1996-2023 by Marcel de Kogel and the M2000 team.           */
/*                                                                            */
/*   See the file "LICENSE" for information on usage and redistribution of    */
/*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       */
/******************************************************************************/

// This file contains the prototypes for the functions accessing memory
// and I/O in M2000, the portable P2000 emulator

/****************************************************************************/
/* Input a byte from given I/O port                                         */
/****************************************************************************/
byte Z80_In (byte Port);

/****************************************************************************/
/* Output a byte to given I/O port                                          */
/****************************************************************************/
void Z80_Out (byte Port,byte Value);

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
extern byte *ReadPage[256];
#define Z80_RDMEM(a) ReadPage[(a)>>8][(a)&0xFF]

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
extern byte *WritePage[256];
#define Z80_WRMEM(a,v) WritePage[(a)>>8][(a)&0xFF]=v

/****************************************************************************/
/* Since the P2000 doesn't use memory mapped I/O nor opcode encryption, we  */
/* can simply do with the macro definitions below                           */
/****************************************************************************/
#define Z80_RDOP(A)      Z80_RDMEM(A)
#define Z80_RDOP_ARG(A)  Z80_RDMEM(A)
#define Z80_RDSTACK(A)   Z80_RDMEM(A)
#define Z80_WRSTACK(A,V) Z80_WRMEM(A,V)
