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

#include "config.h"
#include "proto.h"
#include "ntserv.h"
#include "data.h"
#include "shmem.h"



/*-----------------------------MODULE VARIABLES----------------------------*/
static struct timeval lasttime;	/* used to prevent players from */
 /* firing too quickly */

#define UGAP 100000		/* gap between missile firings, microseconds */
/*-------------------------------------------------------------------------*/








/*----------------------------VISIBLE FUNCTIONS------------------------*/

/*------------------------------FIRE_MISSILE-----------------------------*/
/* If a set of given conditions are met, fire a single missile in direction
** course.
**
** torp->t_fuse is the life of the missile.
*/

void 
fire_missile_dir(unsigned char course)
{
    register int i;		/* looping var */
    register struct missile *drn;	/* to point to a torp */
    int     max;

    if (weaponsallowed[WP_MISSILE] == 0) {
	warning("Missiles haven't been invented yet.");
	return;
    }
    if ((weaponsallowed[WP_FIGHTER] == 0) && (me->p_ship.s_nflags & SFNHASFIGHTERS)) {
	warning("Fighters haven't been invented yet.");
	return;
    }
    if (0 == shipvals[me->p_ship.s_type].s_missile.count) {
	warning("This ship can't mount a missile rack");
	return;
    }
    if (!(me->p_specweap & SFNHASMISSILE)) {
	warning("Our ship is not equipped with a missile rack");
	return;
    }
    if (me->p_ship.s_missilestored == 0) {
	warning("Our missile rack is empty!");
	return;
    }
    if (me->p_flags & PFWEP) {	/* no firing while */
	warning("Torpedo launch tubes have exceeded maximum safe temperature!");
	return;
    }
    if (me->p_flags & PFREPAIR) {	/* cannot be in repair mode */
	warning("We cannot fire while our vessel is in repair mode.");
	return;
    }
    if ((me->p_cloakphase) && (me->p_ship.s_type != ATT)) {
	warning("We are unable to fire while in cloak, captain!");
	return;			/* no firing while cloaked unless in an ATT */
    }
    if ((me->p_ship.s_nflags & SFNHASFIGHTERS) &&
	(me->p_ntorp > 0)) {
	warning("Flight crews rearming fighters, be patient (wait for torps to die)");
	return;			/* can't do both fighters and torps */
    }

    if (!check_fire_warp()
	|| !check_fire_warpprep()
	|| !check_fire_docked())
	return;

    if (!temporally_spaced(&lasttime, UGAP))
	return;

    max = NPTHINGIES;
    if (me->p_ship.s_missile.count < max)
	max = me->p_ship.s_missile.count;

    for (i = 0, drn = &missiles[me->p_no * NPTHINGIES]; i < max; i++, drn++) {
	if (drn->ms_status == TFREE || drn->ms_status == TLAND)
	    break;
    }

    if (i >= max) {
	char    buf[120];

	if (myship->s_nflags & SFNHASFIGHTERS)
	    sprintf(buf, "Maximum of %d fighters in a squadron.", max);
	else
	    sprintf(buf, "Our computers limit us to %d missiles at a time", max);
	warning(buf);
	return;
    }
    if (me->p_fuel < myship->s_missile.cost) {	/* have to have enough fuel */
	if (myship->s_nflags & SFNHASFIGHTERS)
	    warning("No fuel can be spared to send out fighters!");
	else
	    warning("We don't have enough fuel to launch a missile!");
	return;
    }
    if (me->p_ship.s_missilestored > 0) {
	char    buf[80];
	me->p_ship.s_missilestored--;	/* use up a missile in the rack */
	if (myship->s_nflags & SFNHASFIGHTERS) {
	    me->p_armies = (int) (me->p_ship.s_missilestored / FAE_RATE);
	    sprintf(buf, "Fighter launched. %d left", me->p_ship.s_missilestored);
	}
	else
	    sprintf(buf, "Missile away. %d left", me->p_ship.s_missilestored);
	warning(buf);
    }
    me->p_fuel -= myship->s_missile.cost;	/* dec the fuel */
    me->p_wtemp += myship->s_missile.wtemp;	/* Heat weapons */


    drn->ms_no = drn - missiles;/* set torp's number */
    drn->ms_status = TMOVE;	/* set torp's type */
    drn->ms_owner = me->p_no;	/* set torp's owner */

    move_missile(drn->ms_no, me->p_x, me->p_y, 0);

/* Fighters get launched in "groups" of four --> +/- 30',60' */
/* It might be a good idea to make this launch pattern thing a #define */
/* Maybe even a macro with a pattern, so they can go in groups of 2,3,etc */

    if (myship->s_nflags & SFNHASFIGHTERS) {
	if (((drn->ms_no) % NPTHINGIES) % 4 == 1)
	    drn->ms_dir = me->p_dir + 16;
	else if (((drn->ms_no) % NPTHINGIES) % 4 == 2)
	    drn->ms_dir = me->p_dir - 48;
	else if (((drn->ms_no) % NPTHINGIES) % 4 == 3)
	    drn->ms_dir = me->p_dir + 48;
	else
	    drn->ms_dir = me->p_dir - 16;	/* set course of torp */
    }
    else if (myship->s_nflags & SFNMASSPRODUCED)
	drn->ms_dir = me->p_dir;/* PTs only fire forward */
    else
	drn->ms_dir = course;

    drn->ms_damage = myship->s_missile.damage;	/* set its damage */
    drn->ms_speed = myship->s_missile.speed;	/* set its speed */
    drn->ms_fuse = myship->s_missile.fuse + (lrand48() % 20);	/* set fuse */
    drn->ms_war = me->p_hostile | me->p_swar;	/* get its war mask */
    drn->ms_team = me->p_team;	/* set team of owner */
    drn->fi_hasfired = 0;
    if (myship->s_nflags & SFNHASFIGHTERS) {	/* If sfnhasfighters is set */
	drn->ms_type = FIGHTERTHINGY;	/* the missile is a fighter.  */
    }
    else {
	drn->ms_type = MISSILETHINGY;
    }

    drn->ms_turns = myship->s_missile.aux;	/* set how torp tracks */
    drn->ms_locked = -1;
    me->p_nthingys++;                            /* we own another thingy */
}

/*-------------------------------------------------------------------------*/






/*-------END OF FILE--------*/
