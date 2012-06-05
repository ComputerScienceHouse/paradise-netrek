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

/*
** Here we have another flaw.  Detonating other players torps can be a
** very quick way to die.  Why?  Because you always take some damage.
** Experienced players never detonate other players' torps.  Balance is
** really hard to obtain with this type of function.  Technically, a
** player could nearly continuously detonate torps (at least faster than
** they could be fired) and never be hurt, if I allowed less damage as
** a possible result.  So here it sits.
*/



static struct timeval lasttp;


#define UGAP 100000		/* microseconds */


/*-----------------------------VISIBLE FUNCTIONS---------------------------*/

/*-------------------------------DETOTHERS---------------------------------*/
/*  This function goes through all the torps in the games and detonates the
torps that are close enough to det.  */

void 
detothers(void)
{
    register int h, i;		/* looping var */
    int     dx, dy;		/* to find distance */
    register struct torp *j;	/* to point to torp */
    register struct missile *drn;	/* to point to torp */

    if (me->p_fuel < myship->s_detcost) {	/* if not enough fuel */
	warning("Not enough fuel to detonate");	/* then print warning */
	return;			/* and get out of dodge */
    }
    if (me->p_flags & PFWEP) {	/* if W-temped then you */
	warning("Weapons overheated");	/* cannot det */
	return;
    }

    if (!temporally_spaced(&lasttp, UGAP))
	return;


    me->p_fuel -= myship->s_detcost;	/* take fuel away */
    me->p_wtemp += myship->s_detcost / 5;	/* increase W-temp */
    for (h = 0; h < MAXPLAYER; h++) {	/* go through all players */
	if ((players[h].p_status == PFREE) || (h == me->p_no))
	    continue;		/* do not det own torps */
	for (i = h * MAXTORP; i < MAXTORP * (h + 1); i++) {
	    j = &torps[i];	/* get a torp */
	    if (friendlyTorp(j))/* if its friendly then */
		continue;	/* disregard it */
	    if ((j->t_status == TMOVE) || (j->t_status == TSTRAIGHT)) {
		dx = j->t_x - me->p_x;	/* if torp moving */
		dy = j->t_y - me->p_y;	/* get delta cords */
		if (ABS(dx) > myship->s_detdist || ABS(dy) > myship->s_detdist)
		    continue;	/* obviously too far away */
		if (dx * dx + dy * dy < myship->s_detdist * myship->s_detdist)
		{ /* close enough? */
		    j->t_whodet = me->p_no;	/* set who detted it */
		    j->t_status = TDET;	/* change status to det */
		}
	    }
	}
    }

    for (h = 0; h < MAXPLAYER * NPTHINGIES; h++) {
	drn = &missiles[h];

	if (friendlyMissile(drn))
	    continue;

	if (drn->ms_status == TMOVE || drn->ms_status == TSTRAIGHT) {
	    dx = drn->ms_x - me->p_x;	/* if torp moving */
	    dy = drn->ms_y - me->p_y;	/* get delta cords */

     	    if (ABS(dx) > myship->s_detdist || ABS(dy) > myship->s_detdist)
	       continue;	/* obviously too far away */
	    if (dx * dx + dy * dy < myship->s_detdist * myship->s_detdist) 
	    { /* close enough? */
	       drn->ms_whodet = me->p_no;	/* set who detted it */
	       drn->ms_status = TDET;	/* change status to det */
	    }
	}
    }
}


/*-------------------------------------------------------------------------*/





/*-------END OF FILE------*/
