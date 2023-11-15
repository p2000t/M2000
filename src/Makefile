
CC	= gcc	# C compiler used
LD	= gcc	# Linker used

ifdef DJGPP # build for MS-DOS

AS	= gcc	# Assembler used
CFLAGS	= -Wall -m486 -fomit-frame-pointer -O2 -DMSDOS -DX86_ASM
ASFLAGS	= -c -Wall
LFLAGS  = -s

OBJECTS = M2000.o P2000.o Z80.o Z80Debug.o \
          MSDOS.o Asm.o DMA.o INT.o SB.o Bitmap.o

all:    m2000.exe

m2000.exe: $(OBJECTS)
	$(LD) $(LFLAGS) -Wl,-Map,m2000.map -o ../m2000.exe $(OBJECTS)

M2000.o:    M2000.c P2000.h Z80.h Help.h Z80IO.h
P2000.o:    P2000.c P2000.h Z80.h Z80IO.h
Z80.o:      Z80.c Z80.h Z80CDx86.h Z80DAA.h Z80IO.h
Z80Debug.o: Z80Debug.c Z80.h Z80IO.h Z80Dasm.h
SDOS.o:     MSDOS.c P2000.h MSDOS.h DMA.h INT.h Asm.h Bitmap.h Common.h
INT.o:      INT.c P2000.h MSDOS.h DMA.h INT.h Asm.h Bitmap.h
DMA.o:      DMA.c P2000.h MSDOS.h DMA.h INT.h Asm.h Bitmap.h
SB.o:       SB.c P2000.h MSDOS.h DMA.h INT.h Asm.h Bitmap.h
Asm.o:      Asm.S
Bitmap.o:   Bitmap.c Bitmap.h

else # build for Linux, Windows, macOS

CFLAGS  = -Wall -O2 -DALLEGRO # -DDEBUG
LFLAGS  = -s
ifdef _WIN32
  LFLAGS += -mwindows
endif

OBJECTS = M2000.o P2000.o Z80.o Z80Debug.o Allegro.o

all:    m2000

m2000:	$(OBJECTS)
	$(LD) $(LFLAGS) -o ../M2000 $(OBJECTS) -lallegro -lallegro_main -lallegro_primitives -lallegro_image -lallegro_audio -lallegro_dialog -lallegro_memfile

M2000.o:    M2000.c P2000.h Z80.h Help.h Z80IO.h
P2000.o:    P2000.c P2000.h Z80.h Z80IO.h
Z80.o:      Z80.c Z80.h Z80CDx86.h Z80DAA.h Z80IO.h
Z80Debug.o: Z80Debug.c Z80.h Z80IO.h Z80Dasm.h
Allegro.o:  Allegro.c Allegro.h Common.h P2000.h Z80.h Z80IO.h Icon.h Menu.h AllegroKeyboard.h

endif

clean:
ifdef _WIN32
	del /Q /F *.o *.MAP 2>NUL
else
ifdef DJGPP
	del *.o >NUL
	del *.map >NUL
else
	rm -f *.o *~ *.MAP 
endif
endif
