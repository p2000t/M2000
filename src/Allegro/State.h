
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
  static char extension[7];
  FILE *f;
  int i;

  if (chosenFilePath) {
    strcpy(stateFilePath, chosenFilePath);
  } else {
    //bump old state dump files down
    for (i=1; i<=10; i++) {
      if (i==10)
        strcpy(extension, ".dmp");
      else
        sprintf(extension, "-%i.dmp", i);

      al_set_path_filename(stateFolder, "quicksave");
      al_set_path_extension(stateFolder, extension);
      strcpy(stateFilePath, al_path_cstr(stateFolder, PATH_SEPARATOR));

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
    fwrite(RAM, 1, RAMSize, f); //write RAM
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
    al_set_path_extension(stateFolder, ".dmp");
    strcpy(stateFilePath, al_path_cstr(stateFolder, PATH_SEPARATOR));
  }

  if ((f = fopen(stateFilePath , "rb"))) {
    fread(&regs, sizeof(regs), 1, f); //read Z80 registers
    fread(ROM, 1, 0x5000, f); //read ROM
    fread(VRAM, 1, 0x1000, f); //read VRAM
    fread(RAM, 1, RAMSize, f); //read RAM
    fclose(f);
    Z80_SetRegs(&regs);
  }     
}