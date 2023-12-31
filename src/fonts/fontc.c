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
/*   Copyright (C) 1996-2023 by Marcel de Kogel and the M2000 team.           */
/*                                                                            */
/*   See the file "LICENSE" for information on usage and redistribution of    */
/*   this file, and for a DISCLAIMER OF ALL WARRANTIES.                       */
/******************************************************************************/

// This file contains the font compiler

#include <stdio.h>
#include <string.h>

int main (int argc,char *argv[])
{
 FILE *infile,*outfile;
 int a,count,shift;
 char buf[256],*p;
 char putbuf[10];
 printf ("fontc: Teletext font compiler\n"
         "Copyright (C) Marcel de Kogel 1996\n");
 if (argc!=3)
 {
  printf ("Usage: fontc <infile> <outfile>\n");
  return 1;
 }
 infile=fopen (argv[1],"rt");
 if (!infile)
 {
  printf ("Cannot open %s\n",argv[1]);
  return 2;
 }
 outfile=fopen (argv[2],"wb");
 if (!outfile)
 {
  printf ("Cannot open %s\n",argv[2]);
  return 3;
 }
 count=0;
 while (fgets(buf,256,infile))
 {
  if (buf[0]=='-' || buf[0]=='+')
  {
   p=strchr (buf,' ');
   if (p) *p='\0';
   p=strchr (buf,'\n');
   if (p) *p='\0';
   p=buf+strlen(buf)-1;
   a=0; shift=0;
   while (p>=buf)
   {
    if (*p=='+') a|=(1<<shift);
    shift++;
    p--;
   }
   putbuf[count++]=a;
   if (count==10)
   {
    fwrite (putbuf,1,10,outfile);
    count=0;
   }
  }
  else
  {
   if (count)
   {
    for (a=count;a;a=(a+1)%10)
     fputc (0,outfile);
    fwrite (putbuf,1,count,outfile);
    count=0;
   }
  }
 }
 fclose (outfile);
 fclose (infile);
 return 1;
}

