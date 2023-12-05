#include <stdio.h>
#include <string.h>

typedef struct {
  const char* key;
  const char* value;
} LanguageEntry;

static LanguageEntry NLstrings[] = {
  {"File->", "Bestand->"},
  {"Insert Cassette...", "Invoeren Cassette..."},
  {"Help->", "Hulp->"},
  {"About M2000", "Over M2000"},
  {"Thanks to Marcel de Kogel for creating the core of this emulator back in 1996.", "Met dank aan Marcel de Kogel voor het creëren van de basis van deze emulator in 1996."},
  { NULL, NULL },
};

const char* _(const char* key) {
  if (uilanguage == 0) return key; // english doesn't need translating
  int i=0;
  for (i=0; NLstrings[i].key; i++) {
    if (strcmp(key, NLstrings[i].key) == 0) 
      return NLstrings[i].value;
  }
  return key;
}