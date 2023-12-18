#******************************************************************************#
#*                             M2000 - the Philips                            *#
#*                ||||||||||||||||||||||||||||||||||||||||||||                *#
#*                ████████|████████|████████|████████|████████                *#
#*                ███||███|███||███|███||███|███||███|███||███                *#
#*                ███||███||||||███|███||███|███||███|███||███                *#
#*                ████████|||||███||███||███|███||███|███||███                *#
#*                ███|||||||||███|||███||███|███||███|███||███                *#
#*                ███|||||||███|||||███||███|███||███|███||███                *#
#*                ███||||||████████|████████|████████|████████                *#
#*                ||||||||||||||||||||||||||||||||||||||||||||                *#
#*                                  emulator                                  *#
#*                                                                            *#
#*   Copyright (C) 1996-2023 by Marcel de Kogel and the M2000 team.           *#
#*                                                                            *#
#*   See the file "LICENSE" for information on usage and redistribution of    *#
#*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       *#
#******************************************************************************#

CC	= gcc	# C compiler used
CFLAGS  = -Wall -O2
LDFLAGS  = -s

OBJECTS = M2000.o P2000.o Z80.o Main.o Z80Debug.o

all:    m2000

m2000:	$(OBJECTS)
	$(CC) $(LDFLAGS) -o ../M2000 $(OBJECTS) -lallegro -lallegro_main -lallegro_primitives -lallegro_image -lallegro_audio -lallegro_dialog -lallegro_memfile

M2000.o:    M2000.c P2000.h Z80.h Z80IO.h
P2000.o:    P2000.c P2000.h Z80.h Z80IO.h
Z80.o:      Z80.c Z80.h Z80DAA.h Z80IO.h
Z80Debug.o: Z80Debug.c Z80.h Z80IO.h Z80Dasm.h
Main.o: 	Allegro/*.c Allegro/*.h Common.h P2000.h Z80.h Z80IO.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o