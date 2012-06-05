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

/*-------------------------------INTERNAL FUNCTONS------------------------*/

/*----------------------------------GETSHIP--------------------------------*/
/*  This function gets the particular ship type the player wants.  It takes
the ship values from the shipvals array.  */

/* args:
   shipp: ship structure to load
   s_type: type of ship to get */
void 
getship(struct ship *shipp, int s_type)
{
    memcpy((char *) shipp, (char *) &(shipvals[s_type]), sizeof(struct ship));
}



/*------------------------------------------------------------------------*/





void 
get_ship_for_player(struct player *me, int s_type)
{
    getship(&me->p_ship, s_type);

    me->p_shield = me->p_ship.s_maxshield;	/* shields are at max */
    me->p_damage = 0;		/* no damage to ship */
    me->p_subdamage = 0;	/* no fractional damage either */
    me->p_subshield = 0;	/* no fractional damage to shield */

    me->p_fuel = me->p_ship.s_maxfuel;	/* fuel is at max */
    me->p_etemp = 0;		/* engines are ice cold */
    me->p_etime = 0;		/* not counting down for E-temp */
    me->p_warptime = 0;		/* we are not preparing for warp */
    me->p_wtemp = 0;		/* weapons cool too */
    me->p_wtime = 0;		/* not counting down for W-temp */

    if (allows_docking(me->p_ship)) {	/* if ship can be docked to */
	int     i;
	me->p_docked = 0;	/* then set all ports as */
	for (i = 0; i < MAXPORTS; i++)	/* vacant */
	    me->p_port[i] = VACANT;
    }
    if (weaponsallowed[WP_MISSILE] &&
	(me->p_ship.s_missilestored > 0) &&
	(me->p_kills >= configvals->mskills)) {
	me->p_ship.s_nflags |= SFNHASMISSILE;	/* arm ship with missile
						   launcher */
    }
    if (!(me->p_ship.s_nflags & SFNHASMISSILE)) {
	me->p_ship.s_missilestored = 0;	/* no missiles if no launcher */
    }
    if (weaponsallowed[WP_PLASMA] &&
	(me->p_ship.s_plasma.cost > 0) &&
	(me->p_kills >= configvals->plkills)) {
	me->p_ship.s_nflags |= SFNPLASMAARMED;	/* arm ship with plasma
						   launcher */
    }
    me->p_specweap = 0;

    /*
     * fix this now since we can't do it right in a
     * conf.sysdef/default/pl_gen ordering... sigh.
     */
    if( !configvals->warpdrive )
        me->p_ship.s_nflags &= ~SFNCANWARP;
}


/*----------END OF FILE-----*/
