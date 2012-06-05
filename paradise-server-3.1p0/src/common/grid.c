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

/* This function should be called any time a planet is relocated.
   It is in charge of updating the space grid. */

/* args:
    int     pno;		 planet number 
    int     x, y;
    int     isold;		 0 if the planet has not yet been entered
				   into the grid */
void 
move_planet(int pno, int x, int y, int isold)
{
    struct planet *pl = &planets[pno];

    if (isold) {
	/* remove from previous space grid position */
    }
    pl->pl_x = x;
    pl->pl_y = y;

    /* enter into space grid */
}

/* args:
    int     pno;		 player number 
    int     x, y;
    int     isold;		 0 if the player has not yet been entered
				   into the grid */
void 
move_player(int pno, int x, int y, int isold)
{
    struct player *pl = &players[pno];

    if (isold) {
    }
    pl->p_x = x;
    pl->p_y = y;

    /* enter into space grid */
}

/* args:
    int     tno;		 torp number 
    int     x, y;
    int     isold;		 0 if the torp has not yet been entered
				   into the grid */
void 
move_torp(int tno, int x, int y, int isold)
{
    struct torp *t = &torps[tno];

    if (isold) {
    }
    t->t_x = x;
    t->t_y = y;

    /* enter into space grid */
}

/* args:
    int     dno;		 missile number 
    int     x, y;
    int     isold;		 0 if the missile has not yet been entered
				   into the grid */
void 
move_missile(int dno, int x, int y, int isold)
{
    struct missile *d = &missiles[dno];

    if (isold) {
    }
    d->ms_x = x;
    d->ms_y = y;

    /* enter into space grid */
}
