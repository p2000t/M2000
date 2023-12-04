CC	= gcc	# C compiler used
CFLAGS  = -Wall -O2
LDFLAGS  = -s
ifdef DEBUG 
CFLAGS += -DDEBUG
endif

OBJECTS = M2000.o P2000.o Z80.o AllegroMain.o
ifdef DEBUG 
OBJECTS += Z80Debug.o
endif

all:    m2000

m2000:	$(OBJECTS)
	$(CC) $(LDFLAGS) -o ../M2000 $(OBJECTS) -lallegro -lallegro_main -lallegro_primitives -lallegro_image -lallegro_audio -lallegro_dialog -lallegro_memfile

M2000.o:    M2000.c P2000.h Z80.h Z80IO.h
P2000.o:    P2000.c P2000.h Z80.h Z80IO.h
Z80.o:      Z80.c Z80.h Z80DAA.h Z80IO.h
AllegroMain.o:  Allegro/Main.c Allegro/Main.h Common.h P2000.h Z80.h Z80IO.h Allegro/Icon.h Allegro/Menu.h Allegro/Keyboard.h Allegro/State.h
ifdef DEBUG 
Z80Debug.o: debug/Z80Debug.c Z80.h Z80IO.h debug/Z80Dasm.h
endif

clean:
	rm -f *.o Allegro/*.o