
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <allegro5/allegro.h>
#include "P2000.h"

static Z80_Regs regs;

const char * SaveState(const char *chosenFilePath, ALLEGRO_PATH *stateFolder, ALLEGRO_PATH *tapePath)
{
  static char stateFilePath[FILENAME_MAX];
  static char extension[27];
  FILE *f;

  if (chosenFilePath) {
    strcpy(stateFilePath, chosenFilePath);
  } else {
    time_t now = time(NULL);
    strftime(extension, 27, " %Y-%m-%d %H-%M-%S.state", localtime(&now));
    al_set_path_filename(stateFolder, tapePath ? al_get_path_filename(tapePath) : "State"); 
    al_set_path_extension(stateFolder, extension);
    strcpy(stateFilePath, al_path_cstr(stateFolder, PATH_SEPARATOR));
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

void LoadState(const char * stateFilePath)
{
  if (!stateFilePath) return;

  FILE *f;
  if ((f = fopen(stateFilePath , "rb"))) {
    fread(&regs, sizeof(regs), 1, f); //read Z80 registers
    fread(ROM, 1, 0x5000, f); //read ROM
    fread(VRAM, 1, 0x1000, f); //read VRAM
    fread(RAM, 1, RAMSize, f); //read RAM
    fclose(f);
    Z80_SetRegs(&regs);
  }     
}