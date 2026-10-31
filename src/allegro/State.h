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
/*   Copyright (C) 2013-2023 by the M2000 team.                               */
/*                                                                            */
/*   See the file "LICENSE" for information on usage and redistribution of    */
/*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <allegro5/allegro.h>
#include "../P2000.h"

static Z80_Regs regs;

const char * SaveState(const char *chosenFilePath, ALLEGRO_PATH *stateFolder)
{
  static char stateFilePath[FILENAME_MAX];
  static char stateFilePath2[FILENAME_MAX];
  static char extension[32];
  FILE *f;
  int i;

  if (chosenFilePath) {
    strcpy(stateFilePath, chosenFilePath);
  } else {
    //bump old state dump files down
    for (i=1; i<=10; i++) {
      if (i==10)
        strcpy(extension, ".sav");
      else
        snprintf(extension, sizeof(extension), "-%i.sav", i);

      al_set_path_filename(stateFolder, "quicksave");
      al_set_path_extension(stateFolder, extension);
      strcpy(stateFilePath, al_path_cstr(stateFolder, PATH_SEPARATOR));
      al_set_path_filename(stateFolder, NULL);

      if (i==1) 
        al_remove_filename(stateFilePath);
      else
        rename(stateFilePath, stateFilePath2);

      strcpy(stateFilePath2, stateFilePath);
    }
  }

  if ((f = fopen(stateFilePath, "wb"))) {
    Z80_GetRegs(&regs);  
    fwrite(&regs, sizeof(regs), 1, f); //write Z80 registers
    fwrite(ROM, 1, 0x5000, f); //write ROM
    fwrite(VRAM, 1, 0x1000, f); //write VRAM
    fwrite(RAM, 1, RAMSizeKb * 1024, f); //write RAM
    fclose(f);
    return stateFilePath;
  }
  return NULL;
}

void LoadState(const char * chosenFilePath, ALLEGRO_PATH *stateFolder)
{
  static char stateFilePath[FILENAME_MAX];
  FILE *f;

  if (chosenFilePath) {
    strcpy(stateFilePath, chosenFilePath);
  } else {
    al_set_path_filename(stateFolder, "quicksave"); 
    al_set_path_extension(stateFolder, ".sav");
    strcpy(stateFilePath, al_path_cstr(stateFolder, PATH_SEPARATOR));
    al_set_path_filename(stateFolder, NULL);
  }

  if ((f = fopen(stateFilePath , "rb"))) {
    // Stage the whole snapshot before changing the running machine. A short
    // read must not leave registers and memory from two different states.
    Z80_Regs loadedRegs;
    size_t ramSize = (size_t)RAMSizeKb * 1024;
    size_t memorySize = 0x6000 + ramSize;
    byte *memory = malloc(memorySize);
    if (!memory) {
      fclose(f);
      ShowErrorMessage("Unable to allocate memory for loading a save state.");
      return;
    }
    if (fread(&loadedRegs, sizeof(loadedRegs), 1, f) != 1 ||
        fread(memory, 1, memorySize, f) != memorySize) {
      fclose(f);
      free(memory);
      ShowErrorMessage("Unable to read complete save state '%s'.", stateFilePath);
      return;
    }
    fclose(f);
    memcpy(ROM, memory, 0x5000);
    memcpy(VRAM, memory + 0x5000, 0x1000);
    memcpy(RAM, memory + 0x6000, ramSize);
    free(memory);
    Z80_SetRegs(&loadedRegs);
  }     
}