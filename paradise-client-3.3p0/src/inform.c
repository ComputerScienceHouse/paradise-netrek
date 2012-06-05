/*
 * inform.c
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <math.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/* Display information about the nearest objext to mouse */

/*
** When the player asks for info, this routine finds the object
** nearest the mouse, either player or planet, and pop up a window
** with the desired information in it.
**
** We intentionally provide less information than is actually
** available.  Keeps the fog of war up.
**
** There is a different sized window for each type player/planet
** and we take care to keep it from extending beyond the main
** window boundaries.
*/

/* Prototypes */
static void inform_planet_paradise P((struct planet * k));
static void Info_list_normal P((struct player * j));
static void Info_list_paradise P((struct player * j));
static void Info_list_small P((struct player * j));
static void inform_planet_normal P((struct planet * k));

int     last_key = 0;

void
inform(W_Window ww, int x, int y, char key)
{
    char    buf[BUFSIZ];
    int     line = 0;
    register struct player *j;
    register struct planet *k;
    int     mx, my;
    struct obtype *target;
    int     windowWidth, windowHeight;

    mx = x;
    my = y;
    last_key = key;
    if (key == 'i') {
	target = gettarget(ww, x, y, TARG_PLAYER | TARG_SELF | TARG_ASTRAL);
    } else if (key == 'I') {
	target = gettarget(ww, x, y, TARG_PLAYER | TARG_SELF);
    } else {			/* control 'i' */
	target = gettarget(ww, x, y, TARG_ASTRAL);
	key = 'i';
    }
    if (target == NULL)
	return;			/* NULL returned from gettarget indicates no
				   target found. [BDyess] */
    infomapped = 1;
    if (keepInfo > 0 &&
	key != 'I')		/* don't blast the long window */
	infowin_up = keepInfo;

    /*
       This is pretty lame.  We make a graphics window for the info window so
       we can accurately space the thing to barely fit into the galactic map
       or whatever.
    */

    windowWidth = W_WindowWidth(ww);
    windowHeight = W_WindowHeight(ww);
    if (ww == playerw) {
	windowWidth *= W_Textwidth;
	windowHeight *= W_Textheight;
    }
    infotype = target->o_type;
    if (target->o_type == PLAYERTYPE) {
	/* Too close to the edge? */
	if (mx + 23 * W_Textwidth + 2 > windowWidth)
	    mx = windowWidth - 23 * W_Textwidth - 2;
	if (my + 8 * W_Textheight + 2 > windowHeight)
	    my = windowHeight - 8 * W_Textheight - 2;
	if (key == 'i') {
	    infow = W_MakeWindow("info", mx, my, 23 * W_Textwidth, 
	                         8 * W_Textheight, ww, (char *) 0, BORDER, 
				 foreColor);
	    W_MapWindow(infow);
	    j = &players[target->o_num];
	    infothing = (void *) j;
	    Info_list_small(j);
	} else {		/* New information window! */
	    if (!paradise) {	/* if a normal server then */
		infow = W_MakeWindow("info", mx, my, 23 * W_Textwidth,
			    8 * W_Textheight, ww, (char *) 0, BORDER,foreColor);
		W_MapWindow(infow);
		j = &players[target->o_num];
		infothing = (void *) j;
		(void) sprintf(buf, "%s (%c%c):", j->p_name,
			     teaminfo[j->p_teami].letter, shipnos[j->p_no]);
		W_WriteText(infow, W_Textwidth, W_Textheight * line++,
			    playerColor(j), buf, strlen(buf), shipFont(j));
		Info_list_normal(j);
	    } else {		/* else if a paradise server */
		if (mx + 50 * W_Textwidth + 2 > windowWidth)
		    mx = windowWidth - 50 * W_Textwidth - 2;
		if (my + 25 * W_Textheight + 2 > windowHeight)
		    my = windowHeight - 22 * W_Textheight - 2;
		infow = W_MakeWindow("info", mx, my, 50 * W_Textwidth,
			   22 * W_Textheight, ww, (char *) 0, BORDER,foreColor);
		W_MapWindow(infow);
		j = &players[target->o_num];
		infothing = (void *) j;
		(void) sprintf(buf, "%s (%c%c):", j->p_name,
			     teaminfo[j->p_teami].letter, shipnos[j->p_no]);
		W_WriteText(infow, W_Textwidth, W_Textheight * line++,
			    playerColor(j), buf, strlen(buf), shipFont(j));
		Info_list_paradise(j);
	    }
	}
    } else {			/* Planet */
	if (paradise) {
	    /* Too close to the edge? */
	    if (mx + 25 * W_Textwidth + 2 > windowWidth)
		mx = windowWidth - 25 * W_Textwidth - 2;
	    if (my + 5 * W_Textheight + 2 > windowHeight)
		my = windowHeight - 5 * W_Textheight - 2;

	    infow = W_MakeWindow("info", mx, my, W_Textwidth * 25, 
	                         W_Textheight * 5, ww, (char *) 0, BORDER,
				 foreColor);
	} else {
	    /* Too close to the edge? */
	    if (mx + 25 * W_Textwidth + 2 > windowWidth)
		mx = windowWidth - 25 * W_Textwidth - 2;
	    if (my + 3 * W_Textheight + 2 > windowHeight)
		my = windowHeight - 5 * W_Textheight - 2;

	    infow = W_MakeWindow("info", mx, my, W_Textwidth * 25, 
	                         W_Textheight * 3, ww, (char *) 0, BORDER,
				 foreColor);
	}
	W_MapWindow(infow);
	k = &planets[target->o_num];
	infothing = (void *) k;

	if (!paradise) {	/* if not a paradise server */
	    inform_planet_normal(k);
	} else {		/* else must be a paradise server */
	    inform_planet_paradise(k);	/* go display paradise info */
	}
    }
}


void
destroyInfo(void)
{
    W_DestroyWindow(infow);
    infow = 0;
    infomapped = 0;
    infotype = 0;
    infoupdate = 0;
}


static void
Info_list_small(struct player *j)
{
    char    buf[100];
    int     line = 0;
    double  dist;

    (void) sprintf(buf, "%s (%c%c):", j->p_name, teaminfo[j->p_teami].letter,
		   shipnos[j->p_no]);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), shipFont(j));
    (void) sprintf(buf, "Login   %-s", j->p_login);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf),
		W_RegularFont);
    (void) sprintf(buf, "Display %-s", j->p_monitor);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf),
		W_RegularFont);
    (void) sprintf(buf, "Speed   %-d", j->p_speed);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf),
		W_RegularFont);
    (void) sprintf(buf, "kills   %-4.2f", j->p_kills);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf),
		W_RegularFont);
    dist = hypot((double) (me->p_x - j->p_x),
		 (double) (me->p_y - j->p_y)) / (double) GRIDSIZE;
    (void) sprintf(buf, "dist    %-1.2f sectors", dist);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf),
		W_RegularFont);
    (void) sprintf(buf, "S-Class %c%c", j->p_ship->s_desig[0],
                                        j->p_ship->s_desig[1]);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf),
		W_RegularFont);

    if (j->p_swar & idx_to_mask(me->p_teami))
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		    "WAR", 3,
		    W_RegularFont);
    else if (j->p_hostile & idx_to_mask(me->p_teami))
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		    "HOSTILE", 7,
		    W_RegularFont);
    else
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		    "PEACEFUL", 8,
		    W_RegularFont);
}

static void
inform_planet_normal(struct planet *k)
{
    char    buf[100];
    int     line = 0;

    if (k->pl_info & idx_to_mask(me->p_teami)) {
	(void) sprintf(buf, "%s (%c)", k->pl_name,
		       teaminfo[mask_to_idx(k->pl_owner)].letter);
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k),
		    buf, strlen(buf), planetFont(k));
	(void) sprintf(buf, "Armies %d", k->pl_armies);
	W_WriteText(infow, W_Textwidth, W_Textheight * line++,
		    planetColor(k), buf, strlen(buf), W_RegularFont);
	(void) sprintf(buf, "%s %s %s   %s",
		       (k->pl_flags & PLREPAIR ? "RPR" : "      "),
		       (k->pl_flags & PLFUEL ? "FUEL" : "    "),
		       (k->pl_flags & PLAGRI ? "AGRI" : "    "),
		       team_bit_string(k->pl_info));
	W_WriteText(infow, W_Textwidth, W_Textheight * line++,
		    planetColor(k), buf, strlen(buf), W_RegularFont);
    } else {			/* else player has no info on planet */
	(void) sprintf(buf, "%s", k->pl_name);
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k),
		    buf, strlen(buf), W_RegularFont);
	(void) sprintf(buf, "No other info");
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k),
		    buf, strlen(buf), W_RegularFont);
    }
}


/*  This function provides info about planets for a paradise version 2.0
server.  */

static void
inform_planet_paradise(struct planet *k)
{
    char    buf[100];
    int     line = 0;

    if (k->pl_flags & PLSTAR) {	/* test if planet is a star */
	(void) sprintf(buf, "%s", k->pl_name);
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, textColor,
		    buf, strlen(buf), W_RegularFont);
	(void) sprintf(buf, "STAR  ");
	W_WriteText(infow, W_Textwidth, W_Textheight * line++,
		    textColor, buf, strlen(buf), W_RegularFont);
    } else if (!(k->pl_info & idx_to_mask(me->p_teami))) {	/* else if no info */
	(void) sprintf(buf, "%s", k->pl_name);
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k),
		    buf, strlen(buf), W_RegularFont);
	(void) sprintf(buf, "No other info");
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k),
		    buf, strlen(buf), W_RegularFont);
    } else {			/* else we have info */
	(void) sprintf(buf, "%s (%c)", k->pl_name,
		       teaminfo[mask_to_idx(k->pl_owner)].letter);
	W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k),
		    buf, strlen(buf), W_RegularFont);
	(void) sprintf(buf, "Armies %d", k->pl_armies);
	W_WriteText(infow, W_Textwidth, W_Textheight * line++,
		    planetColor(k), buf, strlen(buf), W_RegularFont);
	(void) sprintf(buf, "%s %s %s %s",
		       (k->pl_flags & PLREPAIR ? "RPR" : "      "),
		       (k->pl_flags & PLFUEL ? "FUEL" : "    "),
		       (k->pl_flags & PLAGRI ? "AGRI" : "    "),
		       (k->pl_flags & PLSHIPYARD ? "SHPYD" : "     "));
	W_WriteText(infow, W_Textwidth, W_Textheight * line++,
		    planetColor(k), buf, strlen(buf), W_RegularFont);
	(void) sprintf(buf, "atmos: ");
	switch (k->pl_flags & PLATMASK) {
	case PLPOISON:
	    strcat(buf, "TOXC  surfc: ");
	    break;
	case PLATYPE3:
	    strcat(buf, "TNTD  surfc: ");
	    break;
	case PLATYPE2:
	    strcat(buf, "THIN  surfc: ");
	    break;
	case PLATYPE1:
	    strcat(buf, "STND  surfc: ");
	    break;
	default:
	    strcat(buf, "      surfc: ");
	    break;
	};
	if (k->pl_flags & PLDILYTH)
	    strcat(buf, "D");
	else
	    strcat(buf, " ");
	if (k->pl_flags & PLMETAL)
	    strcat(buf, "M");
	else
	    strcat(buf, " ");
	if (k->pl_flags & PLARABLE)
	    strcat(buf, "A");
	else
	    strcat(buf, " ");
	W_WriteText(infow, W_Textwidth, W_Textheight * line++,
		    planetColor(k), buf, strlen(buf), W_RegularFont);
	sprintf(buf, "Time: %-5d Visit: %s",
		((idx_to_mask(me->p_teami) == k->pl_owner) ? 0
		 : (int) (status2->clock - k->pl_timestamp)),
		team_bit_string(k->pl_info));
	W_WriteText(infow, W_Textwidth, W_Textheight * line++,
		    planetColor(k), buf, strlen(buf), W_RegularFont);
    }
}



static void
Info_list_normal(struct player *j)
{
    char    buf[80];
    int     line = 0;
    struct ratings r;

    get_ratings(j, &r);
    sprintf(buf, "%s (%c%c):", j->p_name, teaminfo[j->p_teami].letter,
	    shipnos[j->p_no]);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    strcpy(buf, "        Rating Total");
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Bombing: %5.2f  %5d", r.r_bombrat, r.r_armies);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Planets: %5.2f  %5d", r.r_planetrat, r.r_planets);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Offense: %5.2f  %5d", r.r_offrat, r.r_kills);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Defense: %5.2f  %5d", r.r_defrat, r.r_losses);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "  Maxkills: %6.2f", r.r_maxkills);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "  Hours:    %6.2f", (float) j->p_stats.st_tticks / 36000.0);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
}

static void
Info_list_paradise(struct player *j)
{
    char    buf[80];
    int     line = 0;
    struct ratings r;

    get_ratings(j, &r);

    sprintf(buf, "Name: %s", j->p_name);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Rank: %s", ranks2[j->p_stats2.st_rank].name);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Royalty: %s", royal[j->p_stats2.st_royal].name);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Genocides: %4d", r.r_genocides);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "DI:     %7.2f", r.r_di);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Maxkills:%6.2f", j->p_stats2.st_tmaxkills);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Kills:     %4d", j->p_stats2.st_tkills);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Losses:    %4d", j->p_stats2.st_tlosses);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "T-hours: %6.2f", (float) j->p_stats2.st_tticks / 36000.0);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "   ");
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    strcpy(buf, "            Rating Total");
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Bombing:   %5.2f  %6d", r.r_bombrat, r.r_armies);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Planets:   %5.2f  %6d", r.r_planetrat, r.r_planets);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Resources: %5.2f  %6d", r.r_resrat, r.r_resources);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Dooshes:   %5.2f  %6d", r.r_dooshrat, r.r_dooshes);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Ratio:     %5.2f", j->p_stats2.st_tkills /
	    ((j->p_stats2.st_tlosses) ? j->p_stats2.st_tlosses : 1.0));
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Offense:   %5.2f", r.r_offrat);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "   ");
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "RATINGS");
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Special ships: %7.2f", r.r_specrat);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Battle:        %7.2f", r.r_batrat);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Strategy:      %7.2f", r.r_stratrat);
    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);

    line = 1;
    sprintf(buf, "   ");
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "JUMPSHIP STATS");
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Planets:    %7d", j->p_stats2.st_jsplanets);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Hours:      %7.2f", (float) j->p_stats2.st_jsticks / 36000.0);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "JS rating:  %7.2f", r.r_jsrat);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);

    sprintf(buf, "   ");
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "STARBASE STATS");
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Kills:        %4d", j->p_stats2.st_sbkills);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Losses:       %4d", j->p_stats2.st_sblosses);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Hours:     %7.2f", (float) j->p_stats2.st_sbticks / 36000.0);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Maxkills:  %7.2f", j->p_stats2.st_sbmaxkills);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "SB rating: %7.2f", r.r_sbrat);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);

    sprintf(buf, "   ");
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "WARBASE STATS");
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Kills:        %4d", j->p_stats2.st_wbkills);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Losses:       %4d", j->p_stats2.st_wblosses);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Hours:     %7.2f", (float) j->p_stats2.st_wbticks / 36000.0);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "Maxkills:  %7.2f", j->p_stats2.st_wbmaxkills);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
    sprintf(buf, "WB rating: %7.2f", r.r_wbrat);
    W_WriteText(infow, W_Textwidth * 28, W_Textheight * line++, playerColor(j),
		buf, strlen(buf), W_RegularFont);
}

/*
   constantly updating info window code [BDyess]
*/
void
updateInform(void)
{
    if (!infomapped || !paradise)
	return;			/* disabled for Bronco servers */
    if (infotype == PLAYERTYPE && last_key == 'i' && (redrawPlayer[me->p_no] ||
			 redrawPlayer[((struct player *) infothing)->p_no]))
	infoupdate = 1;
    if (infoupdate) {
	infoupdate = 0;
	W_ClearWindow(infow);
	if (infotype == PLAYERTYPE) {
	    /* if(isAlive((struct player*)infothing)) { */
	    if (last_key == 'i')/* use small info */
		Info_list_small((struct player *) infothing);
	    else if (!paradise)
		Info_list_normal((struct player *) infothing);
	    else
		Info_list_paradise((struct player *) infothing);
	    /*
	       } else { destroyInfo(); }
	    */
	} else {		/* planet */
	    if (!paradise)
		inform_planet_normal((struct planet *) infothing);
	    else
		inform_planet_paradise((struct planet *) infothing);
	}
    }
}
