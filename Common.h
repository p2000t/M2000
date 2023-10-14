/*** M2000: Portable P2000 emulator *****************************************/
/***                                                                      ***/
/***                                Common.h                              ***/
/***                                                                      ***/
/*** This file contains the system-independent part of the screen refresh ***/
/*** drivers                                                              ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

/* If 1, blanking characters are not displayed this refresh */
static int doblank=1;

/****************************************************************************/
/*** Refresh screen for T-model emulation                                 ***/
/****************************************************************************/
void RefreshScreen_T (void)
{
  byte *S;
  int fg,bg,si,gr,bl,cg,hc,conceal;
  int x,y;
  int c;
  int lastcolour,lastchar;
  int eor;
  int found_si;
  int FG,BG;

  S=VRAM+ScrollReg;

  /* No double height code found yet */
  found_si=0;
  for (y=0;y<24;++y)
  {
   /* Initial values:
      foreground=7 (white)
      background=0 (black)
      normal height
      graphics off
      blanking off
      contiguous graphics
      hold character=space
      steady display */
   fg=7; bg=0; si=0; gr=0; bl=0; cg=1; hc=32; conceal=0;
   lastcolour=7; lastchar=32;
   for (x=0;x<40;++x)
   {
    /* Get character */
    c=S[x]&127;
    /* If bit 7 is set, invert the colours */
    eor=S[x]&128;
    if (!(c&0x60))
    {
     /* Control code found. Parse it */
     switch (c&31)
     {
      /* New text colour */
      case 1: case 2: case 3: case 4: case 5: case 6: case 7:
       fg=lastcolour=c&15;
       gr=conceal=0;
       break;
      /* New graphics colour */
      case 17: case 18: case 19: case 20: case 21: case 22: case 23:
       fg=lastcolour=c&15;
       gr=1;
       conceal=0;
       break;
      /* Flash */
      case 8:
       bl=1;
       conceal=0;
       break;
      /* Steady */
      case 9:
       bl=conceal=0;
       break;
      /* End box (?) */
      case 10:
       break;
      /* Start box (?) */
      case 11:
       break;
      /* Normal height */
      case 12:
       si=0;
       break;
      /* Double height */
      case 13:
       si=1;
       if (!found_si) found_si=1;
       break;
      /* reserved for compatability reasons */
       case 0: case 14: case 15: case 16: case 27:
       break;
      /* conceal display */
      case 24:
       conceal=1;
       hc=32;
       break;
      /* contiguous graphics */
      case 25:
       cg=1;
       break;
      /* separated graphics */
      case 26:
       cg=0;
       break;
      /* black background */
      case 28:
       bg=0;
       break;
      /* new background */
      case 29:
       bg=lastcolour;
       break;
      /* hold graphics */
      case 30:
       hc=lastchar;
       break;
      /* release graphics */
      case 31:
       hc=32;
       break;
     }
     if (gr) c=hc;
     else c=32;
    }
    else
     lastchar=c;
    /* Check for blanking characters and concealed display */
    if ((bl && doblank) || conceal) c=32;
    /* Check if graphics are on */
    if (gr && (c&0x20))
    {
     c+=(c&0x40)? 64:96;
     if (!cg) c+=64;
    }
    /* If double height code on previous line and double height
       is not set, display a space character */
    if (found_si==2 && !si)
     c=32;
    /* Get the foreground and background colours */
    if (!eor)
    {
     FG=fg; BG=bg;
    }
    else
    {
     FG=fg^7; BG=bg^7;
    }
    /* Put the character in the screen buffer */
    PutChar_T (x,y,c-32,FG,BG,(si)? found_si:0);
   }
   /* Update the double height state
      If there was a double height code on this line, do not
      update the character pointer. If there was one on the
      previous line, add two lines to the character pointer */
   if (found_si)
   {
    if (++found_si==3)
    {
     S+=160;
     found_si=0;
    }
   }
   else
    S+=80;
  }
}

/****************************************************************************/
/*** Refresh screen for M-model emulation                                 ***/
/****************************************************************************/
void RefreshScreen_M (void)
{
 byte *S;
 int a,c;
 int x,y;
 int eor,ul;
 S=VRAM;
 for (y=0;y<24;++y,S+=80)
  for (x=0;x<80;++x)
  {
   /* Get character */
   c=S[x]&127;
   /* If bit seven is set, underline is on */
   ul=S[x]&128;
   /* Get attributes */
   a=S[x+2048];
   /* bit 3 = inverse video */
   eor=a&8;
   /* Bit 0 = graphics */
   if ((a&1) && (c&0x20))
    c+=(c&0x40)? 64:96;
   /* If invalid character or blanking is on,
      display a space character */
   if (c<32 || ((a&16) && doblank))
    c=32;
   /* Put the character in the screen buffer */
   PutChar_M (x,y,c-32,eor,ul);
  }
}

/****************************************************************************/
/*** Refresh screen. This function updates the blanking state and then    ***/
/*** calls either RefreshScreen_T() or RefreshScreen_M() and finally it   ***/
/*** calls PutImage() to copy the off-screen buffer to the actual display ***/
/****************************************************************************/
void RefreshScreen (void)
{
 static int BCount=0;
 /* Update blanking count */
 switch (++BCount)
 {
  case 35:
   doblank=1;
   break;
  case 50:
   doblank=0;
   BCount=0;
   break;
 }
 /* Update the screen buffer */
 if (!P2000_Mode)
  RefreshScreen_T ();
 else
  RefreshScreen_M ();
 /* Put the image on the screen */
 PutImage ();
}
