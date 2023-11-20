CC	= gcc	# C compiler used

CFLAGS  = -Wall -O2 -DALLEGRO # -DDEBUG
LFLAGS  = -s

OBJECTS = M2000.o P2000.o Z80.o Z80Debug.o Allegro.o

all:    m2000

m2000:	$(OBJECTS)
	$(CC) $(LFLAGS) -o ../M2000 $(OBJECTS) -lallegro -lallegro_main -lallegro_primitives -lallegro_image -lallegro_audio -lallegro_dialog -lallegro_memfile

M2000.o:    M2000.c P2000.h Z80.h Z80IO.h
P2000.o:    P2000.c P2000.h Z80.h Z80IO.h
Z80.o:      Z80.c Z80.h Z80CDx86.h Z80DAA.h Z80IO.h
Z80Debug.o: Z80Debug.c Z80.h Z80IO.h Z80Dasm.h
Allegro.o:  Allegro.c Allegro.h Common.h P2000.h Z80.h Z80IO.h Icon.h Menu.h AllegroKeyboard.h

clean:
ifeq ($(OS),Windows_NT)
	del /Q /F *.o 2>NUL
else
	rm -f *.o
endif
