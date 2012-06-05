/*
 * credits.c
 *
 * Bill Dyess
 *
 */

#include "copyright.h"

#include "config.h"
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

static char *credits = "\
NetrekII (Paradise) is Copyright 1994-2000 by the Paradise Working Group\n\
\n\
NetrekII (Paradise) was created by:\n\
   Larry Denys         (a fungusamoungus)  <sl1yn@paradise.declab.usu.edu>\n\
   Kurt Olsen          (Bubbles)           <kurto@cc.usu.edu>\n\
   Brandon Gillespie   (Lynx)              <brandon@paradise.declab.usu.edu>\n\
\n\
Developers (alphabetical order):\n\
     Dave Ahn                             Larry Denys (a fungusamoungus)\n\
     Eric Dorman                          Bill Dyess (Thought)\n\
     Rob Forsman (Hammor)                 Brandon Gillespie (Lynx)\n\
     Bob Glamm (Brazilian)                Mike McGrath (Kaos)\n\
     Heath Kehoe (Key)                    Kurt Olsen (Bubbles)\n\
     Sujal Patel (LordZeus)               Joe Rumsey (Ogre)\n\
\n\
Contributors (alphabetical order):\n\
     Scott Drellishak                     Mike Lutz\n\
     Ted Hadley                           Heiji Horde\n\
\n\
NetrekII (Paradise) copyright 1993 by:\n\
            Larry Denys, Kurt Olsen, Brandon Gillespie and Rob Forsman\n\
Netrek (XtrekII) copyright 1989 by:\n\
            Scott Silvey and Kevin Smith\n\
Xtrek copyright 1986 by:\n\
            Chris Guthrie and Ed James\n\
Short packets by:                         Heiko Wengler and Ted Hadley\n\
UDP by:                                   Andy Mcfadden\n\
Bronco Release by:                        Terrence Chang\n\
Full-Color Client by:                     Bill Dyess\n\
Artists (alphabetical order):\n\
     Brandon Gillespie (Lynx)             Mike McGrath (Kaos)\n\
     Joe Rumsey (Ogre)\n\
     Ola Andersson (Janice Rand)\n\
";

void 
showCredits(W_Window win)
{
  char *start = credits;
  char *end = credits;
  int  y = 30;
  
  if(!W_IsMapped(win)) {
    /*fprintf(stderr,"Why am I trying to write the credits into a window that doesn't exist?\n");*/
    return;
  }
  while(*start) {
    while(*end != '\n' && *end) end++;
    W_WriteText(win, 10, y, textColor, start, end - start, W_BoldFont);
    start = ++end;
    y += W_Textheight;
  }

  /* flush buffer if one exists [BDyess] */
  if(W_IsBuffered(win)) 
    W_DisplayBuffer(win);
}
