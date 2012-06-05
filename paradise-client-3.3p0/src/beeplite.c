/*
 * beeplite.c
 */
#include "copyright.h"

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

static int makelite P((struct distress *dist, char *pm));

void
rcdlite(struct distress *dist)
{
    char    message[100];
    int     len = 0;

    if (!F_allow_beeplite)
	return;

    if ((dist == NULL) ||
	(dist->sender == me->p_no) ||
	(distlite[dist->distype] == NULL))
	return;

    len = makedistress(dist, message, distlite[dist->distype]);

    if (len <= 0)
	return;

    makelite(dist, message);
}


void
litedefaults(void)
{
    if (distlite[take] == NULL)
	distlite[take] = "/c/l";
    if (distlite[base_ogg] == NULL)
	distlite[base_ogg] = "/g/m";
    if (distlite[pickup] == NULL)
	distlite[pickup] = "/p";
    if (distlite[generic] == NULL)
	distlite[generic] = "%?%S=SB%{/c%}";
}

void
liteplanet(struct planet *l, W_Color col)
{
    if (!(F_beeplite_flags & LITE_PLANETS))
	return;

    emph_planet_seq_n[l->pl_no] = beep_lite_cycle_time_planet;
    emph_planet_color[l->pl_no] = col;
    l->pl_flags |= PLREDRAW;	/* Leave redraw on until done highlighting */
}

void
liteplayer(struct player *j, W_Color col)
{
    if (!(F_beeplite_flags & (LITE_PLAYERS_MAP | LITE_PLAYERS_LOCAL)) &&
	!((j == me) && (F_beeplite_flags & LITE_SELF)))
	return;

    if (!j || (j->p_flags & PFCLOAK))
	return;

    redrawPlayer[j->p_no] = 1;

    emph_player_seq_n[j->p_no] = beep_lite_cycle_time_player;
    emph_player_color[j->p_no] = col;
}


/* small permutation on makedistress.  Searches for the highliting
** arguments, ignores everything else.
    struct distress *dist;	the info
    char   *pm;			macro to parse, used for distress and
			        macro
*/

static int
makelite(struct distress *dist, char *pm)
{
    struct player *sender;
    struct player *j;
    struct planet *l;
    char    c;
    W_Color lcol;

    sender = &players[dist->sender];

    if (!(*pm)) {
	return (0);
    }
    /* first step is to substitute variables */
    while (*pm) {
	if (*pm == '/') {
	    pm++;

	    if (!pm)
		continue;

	    if (F_beeplite_flags & LITE_COLOR) {
		/* color lite -JR */
		switch (toupper(*(pm + 1))) {
		case 'G':
		    lcol = W_Green;
		    break;
		case 'Y':
		    lcol = W_Yellow;
		    break;
		case 'R':
		    lcol = W_Red;
		    break;
		case 'C':
		    lcol = W_Cyan;
		    break;
		case 'E':
		    lcol = W_Grey;
		    break;
		case 'W':
		default:
		    lcol = W_White;
		    break;
		}
	    } else
		lcol = W_White;

	    switch (c = *(pm++)) {

	    case 'P':		/* push player id into buf */
	    case 'G':		/* push friendly player id into buf */
	    case 'H':		/* push enemy target player id into buf */

	    case 'p':		/* push player id into buf */
	    case 'g':		/* push friendly player id into buf */
	    case 'h':		/* push enemy target player id into buf */

		switch (c) {
		case 'p':
		    j = &players[dist->tclose_j];
		    break;
		case 'g':
		    j = &players[dist->tclose_fr];
		    break;
		case 'h':
		    j = &players[dist->tclose_en];
		    break;
		case 'P':
		    j = &players[dist->close_j];
		    break;
		case 'G':
		    j = &players[dist->close_fr];
		    break;
		default:
		    j = &players[dist->close_en];
		    break;
		}
		liteplayer(j, lcol);
		break;

	    case 'B':		/* highlites planet nearest sender */
	    case 'b':
		l = &planets[dist->close_pl];
		liteplanet(l, lcol);
		break;
	    case 'L':		/* highlites planet nearest pointer */
	    case 'l':
		l = &planets[dist->tclose_pl];
		liteplanet(l, lcol);
		break;
	    case 'U':		/* highlites enemy nearest pointer */
	    case 'u':
		j = &players[dist->tclose_en];
		liteplayer(j, lcol);
		break;
	    case 'c':		/* highlites sender */
	    case 'I':
	    case 'i':
		liteplayer(sender, lcol);
		break;
	    case 'M':		/* highlites me */
	    case 'm':
		liteplayer(me, lcol);
		break;
	    case '0':
		if (F_beeplite_flags & LITE_SOUNDS)
		    W_Beep();
		break;
	    default:
/* try to continue
** bad macro character is skipped entirely,
** the message will be parsed without whatever argument has occurred. - jn
*/
		warning("Bad Macro character in distress!");
		fprintf(stderr, "Unrecognizable special character in "
		                "distress pass 1: %c\n", *(pm - 1));
		break;
	    }
	} else {
	    pm++;
	}

    }


    return (1);
}
