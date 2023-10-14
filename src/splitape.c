/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                              splitape.c                              ***/
/***                                                                      ***/
/*** This program splits a tape image to several tape images all          ***/
/*** containing only one file                                             ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include <stdio.h>
#include <ctype.h>

int main (int argc, char *argv[])
{
 static unsigned char buffer[1024+256];
 FILE *infile;
 FILE *outfile;
 char filename[13];
 int filecount=0;
 int i,j;
 printf ("splitape: Tape image splitter\n"
         "Copyright (C) Marcel de Kogel 1996\n");
 if (argc!=2)
 {
  printf ("Usage: splitape <tape image>\n");
  return 1;
 }
 infile=fopen (argv[1],"rb");
 if (!infile)
 {
  printf ("Can't open %s\n",argv[1]);
  return 1;
 }
 while (fread(buffer,1024+256,1,infile))
 {
  for (i=0;i<8;++i)
  {
   j=buffer[0x36+i];
   if (!isprint(j) || j==' ' || j=='?' || j=='*')
    j='_';
   filename[i]=j;
  }
  filename[8]='.';
  filename[9]=(filecount/100)%10+'0';
  filename[10]=(filecount/10)%10+'0';
  filename[11]=(filecount)%10+'0';
  filename[12]='\0';
  i=buffer[0x4F];
  if (!i) i=256;
  outfile=fopen (filename,"wb");
  if (outfile)
   printf ("Writing %s - %d blocks\n",filename,i);
  else
  {
   printf ("Can't open %s\n",filename);
   return 1;
  }
  fwrite (buffer,1024+256,1,outfile);
  for (--i;i;--i)
  {
   fread (buffer,1024+256,1,infile);
   fwrite (buffer,1024+256,1,outfile);
  }
  fclose (outfile);
  filecount++;
 }
 fclose (infile);
 return 0;
}

