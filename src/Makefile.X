#### Makefile for M2000 Unix/X
# Following #defines can be used:
# UNIX_X		Compile Unix/X version
# DEBUG			Compile with builtin debugger
# LSB_FIRST		Compile for a low-endian machine
# __64BIT__		Compile for 64 bit machines
# __128BIT__            Compile for 128 bit machines
# X86_ASM		Use inline 80x86 assembly
# HAVE_FTRUNCATE	We have ftruncate()
# HAVE_CLOCK		clock() is available on this machine
# JOYSTICK		Use Linux joystick driver
# SOUND			Use USS(/Lite) sound driver
# MITSHM		Use MIT shared memory extensions for X
#
# If you're having trouble compiling M2000, check if the X11
# directories are correct, and modify CFLAGS and LFLAGS if not.
# Undefining MITSHM may also fix problems for several users. Be
# aware though, screen refresh is much slower when shared
# memory is not used

CC	= gcc	# C compiler used
LD	= gcc	# Linker used

CFLAGS  = -Wall -fomit-frame-pointer -O2 -I/usr/X11/include \
	  -DLSB_FIRST -DHAVE_FTRUNCATE -DHAVE_CLOCK -DUNIX_X \
	  -DSOUND -DMITSHM

LFLAGS	= -s -L/usr/X11/lib

OBJECTS = M2000.o P2000.o Z80.o Z80Debug.o \
          Unix.o X.o

all:    m2000

m2000:	$(OBJECTS)
	$(LD) $(LFLAGS) -o m2000 $(OBJECTS) -lXext -lX11

M2000.o:    M2000.c P2000.h Z80.h Help.h Z80IO.h
P2000.o:    P2000.c P2000.h Z80.h Z80IO.h
Z80.o:      Z80.c Z80.h Z80DAA.h Z80CDx86.h Z80IO.h
Z80Debug.o: Z80Debug.c Z80.h Z80IO.h Z80Dasm.h
X.o:        X.c Common.h P2000.h Z80.h Z80IO.h Unix.h
Unix.o:     Unix.c P2000.h Z80.h Z80IO.h Unix.h
