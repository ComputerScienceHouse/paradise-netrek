/***  Pop-up motd window code.  [BDyess] 11/21/93  ***/

#include "config.h"
#include <stdlib.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

#define S_MOTD 0
#define S_SYSDEF 1
#define S_CREDITS 2
#define S_MAX 3

/* handles keystrokes in the motd window */
void
motdWinEvent(W_Event *evt)
{
    static int state = S_MOTD;
    int key = evt->key;

    if(evt->type == W_EV_BUTTON) {	/* buttonpress [BDyess] */
      switch(key) {
	case W_LBUTTON:			/* scroll forward [BDyess] */
	  key = 'f';
	  break;
        case W_RBUTTON:			/* scroll backward [BDyess] */
	  key = 'b';
	  break;
	case W_MBUTTON:			/* jump to beginning [BDyess] */
          currpage = motddata;
	  if(currpage == NULL) break;
	  showMotd(motdWin);
	  state = S_MOTD;
	  break;
      }
    }
    switch (key) {
    case 'f':			/* scroll forward */
	if (currpage == NULL) {
	    currpage = motddata;
	    if (currpage == NULL)
		break;
	}
	if (currpage->next == NULL)
	    break;
	if (currpage->next)
	    currpage->next->prev = currpage;
	currpage = currpage->next;
	showMotd(motdWin);
	state = S_MOTD;
	break;
    case 'b':			/* Scroll motd backward */
	if (currpage == NULL || currpage->prev == NULL)
	    break;
	currpage = currpage->prev;
	showMotd(motdWin);
	state = S_MOTD;
	break;
    case ' ':			/* unmap window */
    case 27:			/* space or escape [BDyess] */
	showMotdWin();
	break;
    case '\t':	        /* tab: cycle between motd, sysdef, and credits
				   [BDyess] */
	W_ClearWindow(motdWin);
	state = (state + 1) % S_MAX;
	/* FALLTHRU */
    case 'r':			/* refresh */
	switch (state) {
	  case S_MOTD:
	    showMotd(motdWin);
	    break;
	  case S_SYSDEF:
	    showValues(motdWin);
	    break;
	  case S_CREDITS:
	    showCredits(motdWin);
	    break;
	}
	break;
    }
}

/* handles map/unmap requests */
void
showMotdWin(void)
{
    if (!motdWin) {
	motdWin = W_MakeWindow(
			       "Motd",
			       0, 0, winside, winside, NULL,
			       (char *) 0, BORDER, foreColor);
	W_MapWindow(motdWin);
	currpage = motddata;
	showMotd(motdWin);
    } else if (W_IsMapped(motdWin)) {
	W_UnmapWindow(motdWin);
    } else {
	W_MapWindow(motdWin);
	currpage = motddata;
	showMotd(motdWin);
    }
}
