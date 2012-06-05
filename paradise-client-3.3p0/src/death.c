/*
 * death.c
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <setjmp.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

extern jmp_buf env;

static char *teamstring[4] = { /* "", */ "and the Federation",
    "and the Romulan Empire",	/* "", */
    "and the Klingon Empire",	/* "", "", "", */
"and the Orions"};

void
redraw_death_messages(void)
{
    char    buf[256];
    int     len;

    switch (me->p_whydead) {
    case KNOREASON:
	/*strcpy(buf, "You were not killed");*/
	buf[0] = 0;
	break;
    case KQUIT:
	strcpy(buf, "You have self-destructed.");
	break;
    case KTORP:
	sprintf(buf, "You were killed by a photon torpedo from %s (%c%c).",
		players[me->p_whodead].p_name,
		teaminfo[players[me->p_whodead].p_teami].letter,
		shipnos[me->p_whodead]);
	break;
    case KPLASMA:
	sprintf(buf, "You were killed by a plasma torpedo from %s (%c%c)",
		players[me->p_whodead].p_name,
		teaminfo[players[me->p_whodead].p_teami].letter,
		shipnos[me->p_whodead]);
	break;
    case KPHASER:
	sprintf(buf, "You were killed by a phaser shot from %s (%c%c)",
		players[me->p_whodead].p_name,
		teaminfo[players[me->p_whodead].p_teami].letter,
		shipnos[me->p_whodead]);
	break;
    case KPLANET:
	/* different message if killed by a star [BDyess] */
        if(planets[me->p_whodead].pl_flags && PLSTAR) {
	  sprintf(buf, "You were burned to a crisp by %s [star]",
		  planets[me->p_whodead].pl_name);
	} else {
	  sprintf(buf, "You were killed by planetary fire from %s (%c)",
		  planets[me->p_whodead].pl_name,
	       teaminfo[mask_to_idx(planets[me->p_whodead].pl_owner)].letter);
	}
	break;
    case KSHIP:
	sprintf(buf, "You were killed by an exploding ship formerly owned by %s (%c%c)",
		players[me->p_whodead].p_name,
		teaminfo[players[me->p_whodead].p_teami].letter,
		shipnos[me->p_whodead]);
	break;
    case KDAEMON:
	strcpy(buf, "You were killed by a dying daemon.");
	break;
    case KWINNER:
	if (me->p_whodead >= 0)
	    sprintf(buf, "Galaxy has been conquered by %s (%c%c) %s",
		    players[me->p_whodead].p_name,
		    teaminfo[players[me->p_whodead].p_teami].letter,
		    shipnos[players[me->p_whodead].p_no],
		    teamstring[players[me->p_whodead].p_teami]);
	else
	    sprintf(buf, "Galaxy has been conquered by %s",
		    teamstring[-1 - me->p_whodead]);
	break;
    case KGHOST:
	sprintf(buf, "You were ghostbusted.");
	break;
    case KGENOCIDE:
	sprintf(buf, "Your team was genocided by %s (%c%c) %s.",
		players[me->p_whodead].p_name,
		teaminfo[players[me->p_whodead].p_teami].letter,
		shipnos[me->p_whodead],
		teamstring[players[me->p_whodead].p_teami]);
	break;
    case KPROVIDENCE:
	sprintf(buf, "You were removed from existence by divine mercy.");
	break;
    case KTOURNEND:
	strcpy(buf, "The tournament has ended.");
	break;
    case KOVER:
	strcpy(buf, "The game has gone into overtime!");
	break;
    case KTOURNSTART:
	strcpy(buf, "The tournament has begun.");
	break;
    case KBADBIN:
	sprintf(buf, "Your netrek executable didn't verify correctly.");
	W_WriteText(mapw, 50, 70, textColor, buf, strlen(buf), 
		    W_RegularFont);
	sprintf(buf, "(could be an old copy or illegal cyborg)");
	W_WriteText(mapw, 50, 110, W_Yellow, buf, strlen(buf), 
		    W_RegularFont);
	*buf = 0;
	break;
    case KMISSILE:
	sprintf(buf, "You were killed by a missile from %s (%c%c).",
		players[me->p_whodead].p_name,
		teaminfo[players[me->p_whodead].p_teami].letter,
		shipnos[me->p_whodead]);
	break;
    case KASTEROID:
	/* asteroid death [BDyess] */
	sprintf(buf, "You were smashed to bits by an asteroid.");
	break;
    default:
	sprintf(buf, "You were killed by something unknown to this game (%d).",
		me->p_whydead);
	break;
    }
    len = strlen(buf);
    W_WriteText(mapw, 250 - len * W_Textwidth / 2, 11 * W_Textheight, W_Yellow, buf,
		len, W_RegularFont);
    /* First we check for promotions: */
    if (promoted) {
	if (!paradise) {
	    sprintf(buf, "Congratulations!  You have been promoted to %s",
		    ranks[mystats->st_rank].name);
	    W_WriteText(mapw, 150, 23 * W_Textheight, W_Yellow, buf, 
	    	        strlen(buf), W_BoldFont);
	} else {
	    sprintf(buf, "Congratulations!  You have been promoted to %s",
		    ranks2[mystats->st_rank].name);
	    W_WriteText(mapw, 150, 23 * W_Textheight, W_Yellow, buf, 
	                strlen(buf), W_BoldFont);
	}
	promoted = 0;
    }
}

void
death(void)
{
    W_Event event;

    W_ClearWindow(mapw);
    W_ClearWindow(iconWin);
    if (oldalert != PFGREEN) {
	if (extraBorder)
	    W_ChangeBorder(w, gColor);
	W_ChangeBorder(baseWin, gColor);
	oldalert = PFGREEN;
	if(autoUnZoom>0)
	    blk_zoom = 0;
    }
    if (W_IsMapped(statwin)) {
	W_UnmapWindow(statwin);
	showStats = 1;
    } else {
	showStats = 0;
    }
    if (W_IsMapped(newstatwin)) {
	W_UnmapWindow(newstatwin);
	showNewStats = 1;
    } else {
	showNewStats = 0;
    }
    if (infomapped)
	destroyInfo();
    W_UnmapWindow(planetw);
    W_UnmapWindow(planetw2);
    W_UnmapWindow(rankw);
    W_UnmapWindow(war);
    if (optionWin)
	optiondone();

    redraw_death_messages();

    while (W_EventsPending()) {
	W_NextEvent(&event);
    }
    longjmp(env, 0);
}
