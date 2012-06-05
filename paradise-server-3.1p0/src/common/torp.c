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
#include "data.h"
#include "shmem.h"


/*-----------------------------MODULE VARIABLES----------------------------*/

static struct timeval lasttp;


#define UGAP 100000		/* microseconds */

/*-------------------------------------------------------------------------*/








/*----------------------------VISIBLE FUNCTIONS------------------------*/

/*---------------------------------NTORP--------------------------------*/
/* If a set of given conditions are met, fire a single torp in direction
** course.  Type is used because robots are allowed to shoot straight.
** Torps sent with status TMOVE wobble a bit.  TSTRAIGHT torps (fired
** by robots) move in a straight line.
**
** torp->t_fuse is the life of the torpedo.  It is set here based on
** a random function.  Torps currently live two to five seconds.
*/

/* args 
    unsigned char course;	 direction of torp travel 
    int     type;		 type of torp flight */
void 
ntorp(unsigned char course, int type)
{
    register int i;		/* looping var */
    register struct torp *k;	/* to point to a torp */
    register unsigned char buttangle;

    if (me->p_flags & PFWEP) {	/* no firing while */
	warning("Torpedo launch tubes have exceeded maximum safe temperature!");
	return;
    }

    if (!temporally_spaced(&lasttp, UGAP))
	return;

    if (me->p_ntorp == MAXTORP) {	/* can't fire more than MAXTORPs */
	warning("Our computers limit us to having 8 live torpedos at a time captain!");
	return;
    }
    if (me->p_fuel < myship->s_torp.cost) {	/* have to have enough fuel */
	warning("We don't have enough fuel to fire photon torpedos!");
	return;
    }
    if (me->p_flags & PFREPAIR) {	/* cannot be in reapair mode */
	warning("We cannot fire while our vessel is in repair mode.");
	return;
    }
    if ((me->p_nthingys > 0)
	&& (myship->s_nflags & SFNHASFIGHTERS)) {	/* cannot fire if
							   fighters can */
	warning("Fire control systems guiding fighters.  No torps available.");
	return;
    }
    if ((me->p_cloakphase) && (me->p_ship.s_type != ATT)) {
	warning("We are unable to fire while in cloak, captain!");
	return;			/* no firing while cloaked unless in an ATT */
    }
    if (!check_fire_warp()
	|| !check_fire_warpprep()
	|| !check_fire_docked())
	return;

    me->p_ntorp++;		/* inc torps in the air */
    me->p_fuel -= myship->s_torp.cost;	/* dec the fuel */

    if(configvals->butttorp_penalty)
    {
      /* figure out absolute difference of arc between rear of ship and torp */
      if((buttangle = course - me->p_dir - 128) > 128)
        buttangle = 256 - buttangle;

      /* Check if in penalty limit.  Ships with no "front" are exempt, of
         course */
      if(myship->s_type == WARBASE || myship->s_type == STARBASE || 
         myship->s_type == JUMPSHIP || TORP_PENALTY_HALFARC < buttangle) 
      {
        me->p_wtemp += myship->s_torp.wtemp;
      }  
      else
      {
        /* You call that dogfighting?  Bad dog!  No biscuit!  Bad dog! */
        me->p_wtemp += myship->s_torp.wtemp +
        (       myship->s_torp.wtemp *
                (TORP_PENALTY_HALFARC - buttangle) *
                TORP_PENALTY_FACTOR *
                me->p_speed
        ) / (TORP_PENALTY_HALFARC * myship->s_imp.maxspeed);
      }
    }
    else
        me->p_wtemp += myship->s_torp.wtemp;

    for (i = me->p_no * MAXTORP, k = &torps[i];	/* Find a free torp */
	 i < me->p_no * MAXTORP + MAXTORP; i++, k++) {
	if (k->t_status == TFREE)
	    break;
    }
    if ((type > TSTRAIGHT) || (type < TFREE))	/* straight torp */
	type = TMOVE;
    k->t_no = i;		/* set torp's number */
    k->t_status = type;		/* set torp's type */
    k->t_owner = me->p_no;	/* set torp's owner */
    k->t_team = me->p_team;	/* set team of owner */

    move_torp(i, me->p_x, me->p_y, 0);

    k->t_dir = course;		/* set course of torp */
    k->t_damage = myship->s_torp.damage;	/* set its damage */
    k->t_speed = myship->s_torp.speed;	/* set its speed */
    k->t_war = me->p_hostile | me->p_swar;	/* get its war mask */
    k->t_fuse = myship->s_torp.fuse + (lrand48() % 20);	/* set fuse */
    k->t_turns = myship->s_torp.aux;	/* set how torp tracks */
}

/*-------------------------------------------------------------------------*/






/*-------END OF FILE--------*/
