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
#include "weapons.h"

/*-----------------------------VISIBLE FUNCTIONS---------------------------*/

/*----------------------------------PHASER---------------------------------*/
/*  This function shoots a phaser for a player.  Various conditions are
checked to see if the phaser should be allowed to fire.  If the player can
fire then, a player is found in the direction the phaser was fired.  */


void 
phaser(unsigned char course)
{
    int i;		/* looping var */
    struct player *j, *target=0;	/* to hold player found */
    struct plasmatorp *k, *target2=0;	/* to hold plasma torp found */
    struct missile *d, *target3=0;
    struct phaser *mine;	/* to pnt to player's phaser */
    int     whichtarget;	/* to hold type of target */
    int     target_x=0, target_y=0; /* target's x and y coords */
    unsigned char dir;		/* to get direction of phasr */
    int     range, trange;	/* range of target */
    int     maxangle;		/* potential target range */
    int     myphrange;		/* angle to hit potentl targ */
    char    buf[80];		/* to sprintf warnings into */

    mine = &phasers[me->p_no];	/* get phaser struct */
    if (!(myship->s_nflags & SFNHASPHASERS)) {	/* do we have phasers? */
	warning("Weapons Officer:  This ship is not armed with phasers, captain!");
	return;
    }
    if (mine->ph_status != PHFREE) {	/* is phaser currently being */
	warning("Phasers have not recharged");	/* fired */
	return;
    }
    if (me->p_fuel < myship->s_phaser.cost) {	/* do we have enough fuel */
	warning("Not enough fuel for phaser");
	return;
    }
    if (me->p_flags & PFREPAIR) {	/* cannot fire while in */
	warning("Can't fire while repairing");	/* repair mode */
	return;
    }
    if (me->p_flags & PFWEP) {	/* cannot fire while weapon */
	warning("Weapons overheated");	/* temped */
	return;
    }
    if ((me->p_cloakphase) && (me->p_ship.s_type != ATT)) {
	warning("Cannot fire while cloaked");	/* cannot fire while cloaked */
	return;
    }
    if (!check_fire_warp()
	|| !check_fire_warpprep()
	|| !check_fire_docked())
	return;

    me->p_fuel -= myship->s_phaser.cost;	/* subtract off fuel cost */
    me->p_wtemp += myship->s_phaser.wtemp;	/* add to w temp */
    mine->ph_dir = course;	/* get direction of phaser */
    whichtarget = 0;		/* no target fount yet */
    range = 1000000;		/* Sufficiently big. */
    /* check the players */
    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {	/* loop all players */
	if (((j->p_status != PALIVE)
#ifdef PFBIRD
	     && !(j->p_flags & PFBIRD)
#endif
	     ) || (j == me))	/* only check alive players */
	    continue;
	if ((!((j->p_swar | j->p_hostile) & me->p_team)) &&
	    (!((me->p_swar | me->p_hostile) & j->p_team)) &&
#ifdef PFBIRD
	    !(j->p_flags & PFBIRD)
#endif
	    )
	    continue;		/* only check at war with */
	dir = (unsigned char) (int) (atan2((double) (j->p_x - me->p_x),
			     (double) (me->p_y - j->p_y)) / 3.14159 * 128.);
						      /* get range of target */
	trange = ihypot(j->p_x - me->p_x, j->p_y - me->p_y);
	if (trange == 0)	/* don't want zero in atan */
	    trange = 1;
	maxangle = atan((float) EXPDIST / trange) / 3.14159 * 128.0;
	if (angdist(dir, course) <= maxangle) {	/* if angle within tolerance */
	    if (range > trange) {	/* then check to see if */
		whichtarget = 1;/* this is the closest target */
		target = j;	/* found yet */
		range = trange;	/* record if it is */
	    }
	}
    }				/* check the plasmas */
    for (i = 0, k = &plasmatorps[i]; i < MAXPLASMA * MAXPLAYER; i++, k++) {
	if ((k->pt_status != PTMOVE) || (k->pt_owner == me->p_no))
	    continue;		/* only check live plasmas */
	if ((!(k->pt_war & me->p_team)) &&	/* and unfriendly ones */
	    (!((me->p_swar | me->p_hostile) & k->pt_team)))
	    continue;
	dir = (unsigned char) (int) (atan2((double) (k->pt_x - me->p_x),
			    (double) (me->p_y - k->pt_y)) / 3.14159 * 128.);	/* find direction */
	trange = ihypot(k->pt_x - me->p_x, k->pt_y - me->p_y);
	if (trange == 0)	/* no zeroes in math funcs */
	    trange = 1;
	maxangle = atan((float) EXPDIST / 4 / trange) / 3.14159 * 128.0;
	if (angdist(dir, course) <= (maxangle + 1)) {	/* if we can hit it */
	    if (range > trange) {	/* then check to see if this */
		target_x = k->pt_x;	/* is the closest plasma */
		target_y = k->pt_y;	/* found yet */
		whichtarget = 2;/* and record it if it is */
		target2 = k;
		range = trange;
	    }
	}
    }
    /* check the fighters */
    for (i = 0, d = &missiles[i]; i < NPTHINGIES * MAXPLAYER; i++, d++) {
	if ((d->ms_owner == me->p_no) || (d->ms_type != FIGHTERTHINGY))
	    continue;		/* only check live fighters */
	if ((!(d->ms_war & me->p_team)) &&	/* and unfriendly ones */
	    (!((me->p_swar | me->p_hostile) & d->ms_team)))
	    continue;
	dir = (unsigned char) (atan2((double) (d->ms_x - me->p_x),
				     (double) (me->p_y - d->ms_y))
			       / 3.14159 * 128.);	/* find direction */
	trange = ihypot(d->ms_x - me->p_x, d->ms_y - me->p_y);
	if (trange == 0)	/* no zeroes in math funcs */
	    trange = 1;
	maxangle = atan((float) EXPDIST / 8 / trange) / 3.14159 * 128.0;
	if (angdist(dir, course) <= (maxangle + 1)) {	/* if we can hit it */
	    if (range > trange) {	/* then check to see if this */
		target_x = d->ms_x;	/* is the closest fighter */
		target_y = d->ms_y;	/* found yet */
		whichtarget = 3;/* and record it if it is */
		target3 = d;
		range = trange;
	    }
	}
    }

    mine->ph_fuse = me->p_ship.s_phaser.fuse;	/* set phaser fuse */
    myphrange = me->p_ship.s_phaser.speed;	/* phaser range */
    if ((whichtarget == 0) ||	/* if no target found or all */
	(range > myphrange)) {	/* targets too long */
	mine->ph_status = PHMISS;	/* then we missed */
	warning("Phaser missed!!!");
    }
    else if (whichtarget == 2) {/* if we hit a plasma then */
	warning("You destroyed the plasma torpedo!");
	mine->ph_x = target_x;	/* the set point to shoot at */
	mine->ph_y = target_y;
	mine->ph_status = PHHIT2;	/* we hit a plasma */
	target2->pt_status = PTEXPLODE;	/* Plasmas hurt everyone */
	target2->pt_whodet = me->p_no;
    }
    else if (whichtarget == 3) {/* if we hit a fighter then */
	warning("You shot a fighter!");
	mine->ph_x = target_x;	/* the set point to shoot at */
	mine->ph_y = target_y;
	mine->ph_status = PHHIT2;	/* we hit the fighter */
	target3->ms_status = TDET;	/* det the fighter */
	target3->ms_whodet = me->p_no;
    }
    else {			/* else if we hit player */
	mine->ph_target = target->p_no;	/* set the player number */
	mine->ph_damage = me->p_ship.s_phaser.damage *	/* get damage */
	    (1.0 - (range / (float) myphrange));
	if (mine->ph_damage < 0)/* if damage inflicted */
	    mine->ph_damage = -mine->ph_damage;	/* set the phaser damage */
	mine->ph_status = PHHIT;/* status is a hit */

#ifdef PFBIRD
	if (target->p_flags & PFBIRD) {
	    /* change to PHHIT2 so phaser won't follow bird */
	    mine->ph_status = PHHIT2;
	    mine->ph_x = target->p_x;
	    mine->ph_y = target->p_y;
	    /* xx: slight misuse of fields here */
	    target->p_damage = mine->ph_damage;
	    target->p_whodead = me->p_no;

	    (void) sprintf(buf, "\"AAWWK!\"");
	}
#endif

	(void) sprintf(buf, "Phaser burst hit %s for %d points",
		       target->p_name, mine->ph_damage);
	warning(buf);
    }
}

/*------------------------------------------------------------------------*/





/*-------END OF FILE-------*/
