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

/*-----------------------------VISIBLE FUNCTIONS---------------------------*/

/*--------------------------------NPLASMATORP------------------------------*/
/*  This function fires a plasma torp.  It checks a number of conditions to
see if it is allowable to fire a plasma torp.  If so the the plasma if fire.
The two styles of plasma torp are ones that fire in the direction the
ship is going and ones that can fire independent of the ships direction.  */

/* args:
    unsigned char course;	direction plasma should go
    int     type;		type of plasma */
void 
nplasmatorp(unsigned char course, int type)
{
    register int i;		/* looping var */
    register struct plasmatorp *k;	/* to point to plasma torp */

    if (weaponsallowed[WP_PLASMA] == 0) {	/* are plasmas enabled */
	warning("Plasmas haven't been invented yet.");
	return;
    }
    if (me->p_ship.s_plasma.cost <= 0) {	/* ship can have plasma */
	warning("Weapon's Officer:  Captain, this ship can't carry plasma torpedoes!");
	return;
    }
    if (!(me->p_specweap & SFNPLASMAARMED)) {	/* ship equiped with plasma */
	warning("Weapon's Officer:  Captain, this ship is not armed with plasma torpedoes!");
	return;
    }
    if (me->p_flags & PFWEP) {	/* ship not w-temped */
	warning("Plasma torpedo launch tube has exceeded the maximum safe temperature!");
	return;
    }
    if (me->p_nplasmatorp == MAXPLASMA) {
	warning("Our fire control system limits us to 1 live torpedo at a time captain!");
	return;
    }
    if (me->p_fuel < myship->s_plasma.cost) {	/* have enough fuel? */
	warning("We don't have enough fuel to fire a plasma torpedo!");
	return;
    }
    if (me->p_flags & PFREPAIR) {	/* not while in repair mode */
	warning("We cannot fire while our vessel is undergoing repairs.");
	return;
    }
    if ((me->p_cloakphase) && (me->p_ship.s_type != ATT)) {
	warning("We are unable to fire while in cloak, captain!");
	return;			/* not while cloaked */
    }

    if (!check_fire_warp()
	|| !check_fire_warpprep()
	|| !check_fire_docked())
	return;
    me->p_nplasmatorp++;	/* inc plasma torps fired */
    me->p_fuel -= myship->s_plasma.cost;	/* take off the fuel */
    me->p_wtemp += myship->s_plasma.wtemp;	/* do the w-temp */
    for (i = me->p_no * MAXPLASMA, k = &plasmatorps[i];
	 i < (me->p_no + 1) * MAXPLASMA; i++, k++) {
	if (k->pt_status == PTFREE)	/* find a free plasma to fire */
	    break;
    }

    k->pt_no = i;		/* set plasmas number */
    k->pt_status = type;	/* set what type plasma is */
    k->pt_owner = me->p_no;	/* set the owner */
    k->pt_team = me->p_team;	/* set the team */
    k->pt_x = me->p_x;		/* set starting coords */
    k->pt_y = me->p_y;
    if (myship->s_nflags & SFNPLASMASTYLE)	/* depending on type set */
	k->pt_dir = course;	/* any direction */
    else
	k->pt_dir = me->p_dir;	/* or straight ahead of ship */
    k->pt_damage = myship->s_plasma.damage;	/* set the damage it will do */
    k->pt_speed = myship->s_plasma.speed;	/* set its speed */
    k->pt_war = me->p_hostile | me->p_swar;	/* who it doesn't like */
    k->pt_fuse = myship->s_plasma.fuse;	/* how long it will live */
    k->pt_turns = myship->s_plasma.aux;	/* how much will it track */
}

/*-------------------------------------------------------------------------*/





/*------END OF FILE-----*/
