/*
 * util.c
 * added functionality to gettarget() - Bill Dyess 10/6/93
 */
#include "copyright.h"

#include "config.h"
#include <math.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/*
 * * Find the object nearest mouse.  Returns a pointer to an * obtype
 * structure.  This is used for info and locking on. *
 *
 * Because we are never interested in it, this function will * never return
 * your own ship as the target. *
 *
 * Finally, this only works on the two main windows
*/

static struct obtype _target;

struct obtype *
gettarget(W_Window ww, int x, int y, int targtype)
{
/* now can get the closest friendly/enemy player or planet.  Use
   TARG_FRIENDLY or TARG_ENEMY or'd with TARG_PLAYER or TARG_PLANET */
    register int i;
    register struct player *j;
    register struct planet *k;
    int     g_x, g_y, friendly;
    double  dist, closedist;
    int     slotnum, width;

    if (ww == mapw) {
	g_x = unScaleMapX(x);
	g_y = unScaleMapY(y);

    } else if (ww == playerw) {
	if (targtype & TARG_PLAYER) {
	    W_TranslatePoints(playerw, &x, &y);
	    y -= 2;
	    if (y < 0)
		y = 0;
	    if (*playerList == 0 || *playerList == ',') {
		if (y > 15)
		    y = 15;
		slotnum = y;
		width = W_WindowWidth(ww);
		if (x > width / 2)
		    slotnum += 16;
	    } else {
		slotnum = y;
	    }
	    if (slot[slotnum] != -1 &&
		(paradise || !(players[slot[slotnum]].p_flags & PFCLOAK))) {
		/* don't show info for cloakers on Bronco servers */
		_target.o_type = PLAYERTYPE;
		_target.o_num = slot[slotnum];
		return &_target;
	    }
	    return NULL;	/* no target found */
	} else {
	    g_x = me->p_x + unScaleLocal(x);
	    g_y = me->p_y + unScaleLocal(y);
	}
    } else if (ww) {		/* tactical window */
	g_x = me->p_x + unScaleLocal(x);
	g_y = me->p_y + unScaleLocal(y);
    } else {
	g_x = x;
	g_y = y;
    }
    closedist = blk_gwidth;

    if (targtype & TARG_ASTRAL) {
	for (i = 0, k = &planets[0]; i < nplanets; i++, k++) {
	    int     ptype = 0;
	    friendly = friendlyPlanet(k);
	    if (friendly && (targtype & TARG_ENEMY))
		continue;
	    if (!friendly && (targtype & TARG_FRIENDLY))
		continue;
	    if (k->pl_owner != idx_to_mask(me->p_teami) && (targtype & TARG_TEAM))
		continue;
	    switch (PL_TYPE(*k)) {
	    case PLPLANET:
		ptype = TARG_PLANET;
		break;
	    case PLSTAR:
		ptype = TARG_STAR;
		break;
	    case PLAST:
		ptype = TARG_PLANET;
		break;
	    case PLNEB:
		ptype = TARG_NEBULA;
		break;
	    case PLBHOLE:
		ptype = TARG_BLACKHOLE;
		break;
	    case PLPULSAR:
		ptype = TARG_STAR;
		break;
	    }
	    if (!(ptype & targtype))
		continue;
	    dist = hypot((double) (g_x - k->pl_x), (double) (g_y - k->pl_y));
	    if (dist < closedist) {
		_target.o_type = PLANETTYPE;
		_target.o_num = i;
		closedist = dist;
	    }
	}
    }
    if (targtype & TARG_PLAYER) {
	for (i = 0, j = &players[i]; i < nplayers; i++, j++) {
	    if (j->p_status != PALIVE)
		continue;
	    if ((j->p_flags & PFCLOAK) && (!(targtype & TARG_CLOAK)) && j != me)
		continue;
	    if (j == me && !(targtype & TARG_SELF))
		continue;
	    friendly = friendlyPlayer(j);
	    if (friendly && (targtype & TARG_ENEMY))
		continue;
	    if (!friendly && (targtype & TARG_FRIENDLY))
		continue;
	    if (j->p_teami != me->p_teami && targtype & TARG_TEAM)
		continue;
	    if (!(targtype & (isBase(j->p_ship->s_type)
			      ? TARG_BASE
			      : TARG_SHIP)))
		continue;
	    dist = hypot((double) (g_x - j->p_x), (double) (g_y - j->p_y));
	    if (dist <= closedist) {
		_target.o_type = PLAYERTYPE;
		_target.o_num = i;
		closedist = dist;
	    }
	}
    }
    if (closedist == blk_gwidth) {	/* Didn't get one.  bad news */
	_target.o_type = PLAYERTYPE;
	_target.o_num = me->p_no;	/* Return myself.  Oh well... */
	return (&_target);
    } else {
	return (&_target);
    }
}

char *
team_bit_string(int mask)
{
    static char visitorstring[16];	/* better not have more than 16 teams */
    int     i;

    for (i = 0; i < number_of_teams; i++) {
	visitorstring[i] = (mask & (1 << i)) ? teaminfo[i].letter : ' ';
    }
    visitorstring[i] = 0;
    return visitorstring;
}

/* getTargetID returns an id stuct containing then name of the object, the
   type of object, the number of the object, the
   team the object belongs to, and .  Used a lot in the macro code.
   [BDyess] */

struct id *
getTargetID(W_Window ww, int x, int y, int targtype)
{
    struct obtype *target;
    static struct id buf;
    struct player *j;
    struct planet *k;

    target = gettarget(ww, x, y, targtype);
    if (target->o_type == PLAYERTYPE) {
	buf.type = PLAYERTYPE;
	j = &players[target->o_num];
	buf.name = j->p_name;
	buf.number = target->o_num;
	buf.team = j->p_teami;
	buf.mapstring[0] = j->p_mapchars[0];
	buf.mapstring[1] = j->p_mapchars[1];
	buf.mapstring[2] = 0;
    } else if (target->o_type == PLANETTYPE) {
	buf.type = PLANETTYPE;
	k = &planets[target->o_num];
	buf.name = k->pl_name;
	buf.number = target->o_num;
	buf.team = mask_to_idx(k->pl_owner);
	if (0 == strncmp(k->pl_name, "New ", 4)) {
	    strncpy(buf.mapstring, k->pl_name + 4, 3);
	} else if (0 == strncmp(k->pl_name, "Planet ", 7)) {
	    strncpy(buf.mapstring, k->pl_name + 7, 3);
	} else
	    strncpy(buf.mapstring, k->pl_name, 3);
	buf.mapstring[3] = 0;
    } else {			/* tried to find something that doesn't exist */
	return NULL;
    }
    return &buf;
}
