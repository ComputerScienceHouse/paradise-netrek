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
#include "defs.h"
#include "struct.h"

#ifndef SHMEMP_H
#define SHMEMP_H

struct memory {
    int     shmem_size;		/* sizeof(struct memory) sort of a magic
				   number so that we don't connect an
				   obsolete program to the shmem. */
    struct player players[MAXPLAYER];
    struct torp torps[MAXPLAYER * MAXTORP];
    struct missile missiles[MAXPLAYER * NPTHINGIES];
    struct thingy thingies[NGTHINGIES];
    struct plasmatorp plasmatorps[MAXPLAYER * MAXPLASMA];
    struct status status;
    struct status2 status2;
    struct planet planets[MAXPLANETS];
    struct t_unit terrain_grid[(MAX_GWIDTH / TGRID_GRANULARITY) * 
			       (MAX_GWIDTH / TGRID_GRANULARITY)];
    struct phaser phasers[MAXPLAYER];
    int     stars[MAXPLANETS + 1];	/* indices of the stars in the game,
					   indexed on system number */
    struct mctl mctl;
    struct message messages[MAXMESSAGE];
    struct team teams[MAXTEAM + 1];

    struct ship shipvals[NUM_TYPES];
    struct configuration configvals;
    char   cluephrase_storage[CLUEPHRASE_SIZE];
    char   galaxyValid[MAXPLAYER];
};

#endif /* SHMEMP_H */
