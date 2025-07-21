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
VPATH = ../

OBJECTS = M2000.o P2000.o Z80.o Main.o
TARGET = ../../M2000

# MacPorts setup
ifneq ("$(wildcard /opt/local/lib/liballegro*.dylib)","")
    CFLAGS  += -I/opt/local/include
    LDFLAGS += -L/opt/local/lib -Wl,-rpath,/opt/local/lib
endif

# Homebrew setup (Apple Silicon)
ifneq ("$(wildcard /opt/homebrew/lib/liballegro*.dylib)","")
    CFLAGS  += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib -Wl,-rpath,/opt/homebrew/lib
endif

all: clean m2000

m2000:	$(OBJECTS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS) -lallegro -lallegro_main -lallegro_primitives -lallegro_image -lallegro_audio -lallegro_dialog

clean:
	rm -f $(OBJECTS) $(TARGET)
