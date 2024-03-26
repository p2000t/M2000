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
#*   Copyright (C) 2023 by the M2000 team.                                    *#
#*                                                                            *#
#*   See the file "LICENSE" for information on usage and redistribution of    *#
#*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       *#
#******************************************************************************#

all: build

build:
	$(MAKE) -C src/allegro all

clean:
	$(MAKE) -C src/allegro clean

.PHONY: clean