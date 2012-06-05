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
#include "daemonII.h"
#include "data.h"
#include "shmem.h"

void 
credit_armiesbombed(struct player *plyr, int armies, struct planet *plan)
{
    double  factor;

    if (!status->tourn)
	return;			/* nothing counts outside T-mode */

    plyr->p_armsbomb += armies;	/* inc armies bombed */

    /* no honor in beating up on someone who isn't there */
    factor = (plan->pl_owner == NOBODY) ? 0.5 : 1.0;

    plyr->p_stats.st_di += 0.02 * armies * factor;	/* inc players DI */

    if(plyr->p_ship.s_type == WARBASE && configvals->wb_bombing_credit)
      plyr->p_stats.st_di += 0.02 * armies * factor;
    plyr->p_kills += 0.02 * armies * factor;	/* increase players kills */

    armies = random_round(factor * armies);

    status->armsbomb += armies;
    plyr->p_stats.st_tarmsbomb += armies;	/* increase player stats */

    if(plyr->p_ship.s_type == WARBASE && configvals->wb_bombing_credit)
      plyr->p_stats.st_tarmsbomb += armies;

    checkmaxkills(plyr->p_no);	/* check if over max kills */
}
