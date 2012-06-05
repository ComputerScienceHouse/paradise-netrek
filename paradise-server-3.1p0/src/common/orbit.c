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

static void 
deactivate_friendly_tractors(void)
{
    int     i;
    struct player *j;
    for (i = 0; i < MAXPLAYER; i++) {
	j = &players[i];
	if (i == me->p_no ||
	    j->p_team != me->p_team ||
	    j->p_tractor != me->p_no)
	    continue;
	/* j is a teammate with a tractor on me */
	j->p_flags &= ~(PFTRACT | PFPRESS);
    }
}

/*------------------------------VISIBLE PROCEDURES-------------------------*/

/*-----------------------------------ORBIT---------------------------------*/
/*  This function is called when the player presses the orbit key.  It
check to make sure the player can orbit or dock.  The function first checks
through the list of players to see whether there is a dockable ship nearby.
If that fails, then the planets are checked.  The orbit key acts as a toggle
for docking and undocking at ships.  */

void 
orbit(void)
{
    register int i;		/* looping var */
    register struct planet *l;	/* to point to planet being orbited */
    unsigned char dir;		/* to hold course direction */
    int     dx, dy;		/* used to get distances */

    if (me->p_flags & PFORBIT)	/* if not orbiting then we do not */
	return;			/* need to do this section of code */
    if (me->p_speed > ORBSPEED) {	/* going to fast to orbit/dock? */
	warning("Helmsman: Captain, the maximum safe speed for docking or orbiting is warp 2!");
	return;
    }				/* turn off flags */
    me->p_flags &= ~(PFORBIT | PFTRACT | PFPRESS |
		     PFWARP | PFAFTER | PFWARPPREP | PFWPSUSPENDED);
    me->p_warptime = 0;
    if ((me->p_flags & PFDOCK) && (players[me->p_docked].p_speed > 4)) {
	warning("It's unsafe to disengage from your base when it's moving faster then warp 4.");	/* cannot disengage from
													   travelling */
	return;			/* ships if they are going too fast */
    }
    else
	undock_player(me);	/* make sure I'm not docked */

    if (!(me->p_flags & PFPLLOCK) &&	/* if we aren't locked onto a planet */
	can_dock(me->p_ship)) {	/* and player can dock then */
	for (i = 0; i < MAXPLAYER; i++) {	/* go through all players */
	    if (me->p_no == i)	/* and look for ships that */
		continue;	/* allow docking */
	    if (!allows_docking(players[i].p_ship))
		continue;
	    if (!isAlive(&players[i]))	/* disregard players not */
		continue;	/* alive */
	    if (!friendlyPlayer(&players[i]))	/* cannot dock on enemy */
		continue;	/* players */
	    dx = ABS(players[i].p_x - me->p_x);	/* get distance */
	    dy = ABS(players[i].p_y - me->p_y);
	    if (dx > DOCKDIST || dy > DOCKDIST)	/* if too far away thenwe */
		continue;	/* cannot dock here */
	    newdock(i);		/* find a port and dock */

	    deactivate_friendly_tractors();

	    me->p_flags &= ~(PFPLOCK | PFPLLOCK);
	    return;
	}
    }				/* not docking with a player try a planet */
    if (!(me->p_flags & PFPLOCK)) {	/* aren't locked onto a player */
	for (i = 0, l = &planets[i]; i < NUMPLANETS; i++, l++) {
	    switch (PL_TYPE(*l)) {
	    case PLSTAR:
	    case PLNEB:
	    case PLBHOLE:
	    case PLPULSAR:
		continue;
	    }

	    dx = ABS(l->pl_x - me->p_x);	/* go through all planets and */
	    dy = ABS(l->pl_y - me->p_y);	/* check their distance */
	    if (dx > ENTORBDIST || dy > ENTORBDIST)
		continue;	/* too far away?  */
	    if (dx * dx + dy * dy > ENTORBDIST * ENTORBDIST)
		continue;

	    if (!(me->p_team & planets[i].pl_owner) &&	/* can player orbit? */
		!(me->p_ship.s_nflags & SFNCANORBIT)) {
		warning("Central Command regulations prohibits you from orbiting foreign planets");
		return;
	    }
	    if (!(me->p_team & planets[i].pl_owner) &&
		!status->tourn) {
		warning("No one is allowed to orbit alien planets outside of T-mode");
		return;
	    }
	    dir = (unsigned char) (int) (atan2((double) (me->p_x - l->pl_x),
			    (double) (l->pl_y - me->p_y)) / 3.14159 * 128.);
	    scout_planet(me->p_no, l->pl_no);
#if 0
	    l->pl_torbit |= me->p_team;	/* place team in orbit */
#endif
	    me->p_orbitdir = drand48() < configvals->orbitdirprob;
	    me->p_dir = dir + (me->p_orbitdir ? 64 : -64);	/* get direction for
								   player */
	    me->p_flags |= PFORBIT;	/* set his orbiting flag */

	    deactivate_friendly_tractors();

	    move_player(me->p_no, (int) (l->pl_x + ORBDIST * Cos[dir]),
			(int) (l->pl_y + ORBDIST * Sin[dir]), 1);

	    me->p_speed = me->p_desspeed = 0;
	    me->p_planet = l->pl_no;	/* set the planet number */
	    me->p_flags &= ~(PFPLOCK | PFPLLOCK);
	    return;
	}
    }
    warning("Helmsman:  Sensors read no valid targets in range to dock or orbit sir!");
}



/*--------------------------------NEWDOCK--------------------------------*/


void 
newdock(int base_num)
{
    char    buf[80];		/* to sprintf into */
    struct player *base = &players[base_num];
    int     port_id, i;
    int     numports;
    long    distsq = 0;		/* will be set by the first matching port */
    int     angle;

    if (!(base->p_flags & PFDOCKOK)) {	/* docking allowed? */
	sprintf(buf, "%s %s refusing us docking permission captain.",
		base->p_ship.s_name, base->p_name);
	warning(buf);		/* if not say so then */
	return;			/* get out of here */
    }
    port_id = -1;
    numports = base->p_ship.s_numports;

    for (i = 0; i < numports; i++) {
	long    ds;
	int     x, y;
	angle = (2 * i + 1) * 128 / numports;

	if (base->p_port[i] != VACANT)
	    continue;

	x = base->p_x + DOCKDIST * Cos[angle];
	y = base->p_y + DOCKDIST * Sin[angle];
	ds = (me->p_x - x) * (me->p_x - x) + (me->p_y - y) * (me->p_y - y);
	if (port_id == -1 || ds < distsq) {
	    port_id = i;
	    distsq = ds;
	}
    }

    if (port_id < 0) {
	sprintf(buf, "Base %s: Permission to dock denied, all ports currently occupied.", base->p_name);
	warning(buf);		/* print message to tell */
	return;
    }				/* player */
    dock_to(me, base_num, port_id);

    sprintf(buf, "Helmsman:  Docking manuever completed Captain.  All moorings secured at port %d.", port_id);
    warning(buf);		/* tell user he's docked */
}

/*------------END OF FILE--------*/
