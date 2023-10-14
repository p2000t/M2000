/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                 Asm.S                                ***/
/***                                                                      ***/
/*** This file contains MS-DOS specific assembler routines                ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

.file   "Asm.S"

.data
.align 4
DisplayStart:   .long   0xA0000
TimerCount:     .long 0
OldInt8:        .long 0
OldInt8Selector:.word 0
                .globl  _DosSelector
                .globl  _cs_alias

.text

.globl _nofunc
_nofunc:
        ret

.globl ___enable
___enable:
        sti
        ret

.globl ___disable
___disable:
        cli
        ret

.globl _JoyGetPos
_JoyGetPos:
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi
        movw    $0x201,%dx
        movl    $10000,%ecx
        subl    %esi,%esi
        subl    %edi,%edi
        cli
        outb    %al,%dx
        jmp     _JoyGetPos_Loop
_JoyGetPos_Loop:
        inb     %dx,%al
        movl    %eax,%ebx
        shrl    $1,%ebx
        adcl    $0,%esi
        shrl    $1,%ebx
        adcl    $0,%edi
        testb   $3,%al
        loopnz  _JoyGetPos_Loop
        sti
        movl    %edi,%eax
        shll    $16,%eax
        movw    %si,%ax
        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        ret

.globl _ReadTimer
_ReadTimer:
        pushl   %edi
        pushl   %esi
        pushl   %edx
        pushl   %ecx
        pushl   %ebx
        movl    $50,%ecx
ReadTimer_Loop:
        call    _nofunc
        loop    ReadTimer_Loop
ReadTimer_Again:
        movl    (TimerCount),%eax
        shll    $16,%eax
        outb    %al,$0x43
        jmp     ReadTim1
ReadTim1:
        jmp     ReadTim2
ReadTim2:
        inb     $0x40,%al
        jmp     ReadTim3
ReadTim3:
        jmp     ReadTim4
ReadTim4:
        movb    %al,%ah
        inb     $0x40,%al
        xchgb   %al,%ah
        notw    %ax
        movl    (TimerCount),%ebx
        shll    $16,%ebx
        movw    %ax,%bx
        cmpl    %eax,%ebx
        jne     ReadTimer_Again
        popl    %ebx
        popl    %ecx
        popl    %edx
        popl    %esi
        popl    %edi
        ret

timer_interrupt:
        pushl   %eax
        pushw   %ds
.byte 0x2e
        movw    _cs_alias,%ax
        movw    %ax,%ds
        incl    (TimerCount)
        pushfl
        lcall   (OldInt8)
        popw    %ds
        popl    %eax
        iret

install_timer_int:
        pushal
        movl    $0x204,%eax
        movb    $8,%bl
        int     $0x31
        movl    %edx,OldInt8
        movw    %cx,OldInt8Selector
        movw    %cs,%cx
        leal    timer_interrupt,%edx
        movb    $8,%bl
        movl    $0x205,%eax
        int     $0x31
        popal
        ret

restore_timer_int:
        pushal
        movl    OldInt8,%edx
        movw    OldInt8Selector,%cx
        movb    $8,%bl
        movl    $0x205,%eax
        int     $0x31
        popal
        ret

.globl _StartTimer
_StartTimer:
        pushl   %eax
        cli
        movb    $0x34,%al
        outb    %al,$0x43
        jmp     StartTim1
StartTim1:
        jmp     StartTim2
StartTim2:
        subb    %al,%al
        outb    %al,$0x40
        jmp     StartTim3
StartTim3:
        jmp     StartTim4
StartTim4:
        outb    %al,$0x40
        sti
        call    install_timer_int
        popl    %eax
        ret

.globl _RestoreTimer
_RestoreTimer:
        pushl   %eax
        cli
        movb    $0x36,%al
        outb    %al,$0x43
        jmp     ResTim1
ResTim1:
        jmp     ResTim2
ResTim2:
        subb    %al,%al
        outb    %al,$0x40
        jmp     ResTim3
ResTim3:
        jmp     ResTim4
ResTim4:
        outb    %al,$0x40
        sti
        call    restore_timer_int
        popl    %eax
        ret
        
