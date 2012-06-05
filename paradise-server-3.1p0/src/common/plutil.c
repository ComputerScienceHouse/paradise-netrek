/*------------------------------------------------------------------
  Copyright 1989		Kevin P. Smith
				Scott Silvey

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies.

  NETREK II -- Paradise

  Permission to use, copy, modify, and distribute this software and
  its documentation, or any derivative works thereof,  for any 
  NON-COMMERCIAL purpose and without fee is hereby granted, provided
  that this copyright notice appear in all copies.  No
  representations are made about the suitability of this software for
  any purpose.  This software is provided "as is" without express or
  implied warranty.

	Xtrek Copyright 1986			Chris Guthrie
	Netrek (Xtrek II) Copyright 1989	Kevin P. Smith
						Scott Silvey
	Paradise II (Netrek II) Copyright 1993	Larry Denys
						Kurt Olsen
						Brandon Gillespie
		                Copyright 2000  Bob Glamm

--------------------------------------------------------------------*/

#include <math.h>
#include "config.h"
#include "proto.h"
#include "data.h"
#include "shmem.h"

int 
idx_to_mask(int i)
{
    if (i >= 0)
	return 1 << i;
    else
	return 0;
}

int 
mask_to_idx(int m)
{
    int     i;
    if (!m)
	return -1;
    for (i = 0; m > 1; i++, m >>= 1);
    return i;
}

int 
undock_player(struct player *pl)
{
    struct player *base;

    if (!(pl->p_flags & PFDOCK))
	return 0;

    base = &players[pl->p_docked];
    base->p_docked--;		/* base no longer has as many docked */
    base->p_port[pl->p_port[0]] = VACANT;	/* the port is vacant */

    /* what if the player is being undocked because he died? */
    if (base->p_ship.s_type == JUMPSHIP) {
	pl->p_jsdock = 1;
	pl->p_lastjs = pl->p_docked;
    }
    pl->p_flags &= ~PFDOCK;
    pl->p_docked = -1;
    pl->p_port[0] = VACANT;

    return 1;
}

int 
base_undock(struct player *base, int port_id)
{
    struct player *pl;

    if (base->p_port[port_id] < 0)
	return 0;

    pl = &players[base->p_port[port_id]];

    base->p_docked--;
    base->p_port[port_id] = VACANT;

    /* if the jumpship kicks you off, he doesn't get credited.  -RF */

    pl->p_flags &= ~PFDOCK;
    pl->p_docked = -1;
    pl->p_port[0] = VACANT;
    return 1;
}

void 
enforce_dock_position(struct player *pl)
{
    struct player *base;
    unsigned char angle;
    int     port_id;

    if (!(pl->p_flags & PFDOCK))
	return;

    base = &players[pl->p_docked];
    port_id = pl->p_port[0];

    if ((base->p_ship.s_type == STARBASE) || (base->p_ship.s_type == WARBASE) ||
	(base->p_ship.s_type == JUMPSHIP))
	angle = (port_id * 2 + 1) * 128 / base->p_ship.s_numports;
    else
	angle = base->p_dir + 64 + (128 * port_id);	/* max of two ports,
							   really */
    move_player(pl->p_no, (int) (base->p_x + DOCKDIST * Cos[angle]),
		(int) (base->p_y + DOCKDIST * Sin[angle]), 1);

    pl->p_speed = pl->p_desspeed = 0;
    if ((base->p_ship.s_type == STARBASE) || (base->p_ship.s_type == WARBASE) ||
	(base->p_ship.s_type == JUMPSHIP))
	pl->p_dir = pl->p_desdir = angle + 64;
    else
	pl->p_dir = pl->p_desdir = base->p_dir;
}

void 
dock_to(struct player *pl, int base_num, int port_id)
{
    struct player *base = &players[base_num];

    undock_player(pl);

    base->p_docked++;
    base->p_port[port_id] = pl->p_no;

    pl->p_flags |= PFDOCK;	/* we are docked */
    pl->p_docked = base_num;	/* to this base */
    pl->p_port[0] = port_id;	/* in this docking bay */

    enforce_dock_position(pl);
}

void 
scout_planet(int p_no, int pl_no)
{
    struct player *fred = &players[p_no];
    struct planet *mars = &planets[pl_no];
    int     oldness = status->clock - mars->pl_tinfo[fred->p_team].timestamp;

    if (!status->tourn)
	return;

    if ((mars->pl_owner != fred->p_team) &&
	(oldness > 2)) {
	double  scoutdi = log((double) oldness) * M_LOG2E / 100;
	if (scoutdi > 0.1)
	    scoutdi = 0.1;
	fred->p_stats.st_di += scoutdi;
    }
    mars->pl_hinfo |= fred->p_team;
    if (mars->pl_owner != fred->p_team) {
	mars->pl_tinfo[fred->p_team].flags = mars->pl_flags;
	mars->pl_tinfo[fred->p_team].armies = mars->pl_armies;
	mars->pl_tinfo[fred->p_team].owner = mars->pl_owner;
	mars->pl_tinfo[fred->p_team].timestamp = status->clock;
    }
}

/*
 */

void 
evaporate(struct player *pl)
{
    /* put someone on the outfit screen with no ill effects */

    undock_player(pl);		/* if docked then undock me */

    if (allows_docking(pl->p_ship)) {	/* if ships can dock */
	int     k;
	for (k = 0; k < pl->p_ship.s_numports; k++)
	    base_undock(pl, k);	/* remove all docked ships */
	pl->p_docked = 0;	/* no ships docked anymore */
    }
    if (pl->p_flags & PFORBIT) {/* if orbiting then */
	pl->p_flags &= ~PFORBIT;/* eject from orbit */
    }

    pl->p_status = POUTFIT;
    pl->p_whydead = KPROVIDENCE;
}

void 
explode_everyone(int whydead, int minlive)
{
    int     i;
    for (i = 0; i < MAXPLAYER; i++) {
	if (players[i].p_status != PALIVE
	    && players[i].p_status != POBSERVE)
	    continue;

	if(players[i].p_updates < minlive)
	    continue;

	players[i].p_whydead = whydead;
	players[i].p_whodead = 0;
	players[i].p_status = PEXPLODE;
	players[i].p_explode = 10;
	players[i].p_ntorp = 0;	/* no torps shot */
	players[i].p_nplasmatorp = 0;	/* no plasmas shot */
    }
}


/* round randomly, but weighted by the fractional part */

int 
random_round(double d)
{
    int     rval = floor(d);
    if (drand48() < d - rval)
	rval++;
    return rval;
}


char *
twoletters(struct player *pl)
/* calculate the two letters that form the players designation (e.g. R4) */
{
#define RINGSIZE MAXPLAYER+3
    static char buf[RINGSIZE][3];	/* ring of buffers so that this */
    static int idx;		/* proc can be called several times before
				   the results are used */
    if (idx >= RINGSIZE)
	idx = 0;
    buf[idx][0] = teams[pl->p_team].letter;
    buf[idx][1] = shipnos[pl->p_no];
    buf[idx][2] = 0;
    return buf[idx++];
}
