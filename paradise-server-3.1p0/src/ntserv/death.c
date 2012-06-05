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

/* Figure out ratings for player p.  Computed ratings are filled into r */
void 
compute_ratings(struct player *p, struct rating *r)
{
    struct stats *s;
    float   t, t2;

    s = &p->p_stats;

    r->ratio = (s->st_tlosses != 0) ? (float) s->st_tkills /
	(float) s->st_tlosses : s->st_tkills;

    if( status->timeprod == 0 )
	status->timeprod = 1;

    t = (float) s->st_tticks / (float) status->timeprod;	/* hour ratio */
    if (t == 0.0)
	t = 1.0;

    t2 = t * (float) status->kills;	/* get expected kills */
    t2 /= 2;				/* lower expectation */
    if(t2 == 0.0) t2 = 1.0;
    printf("ticks %f timeprod %lu %f %f %ld\n",
           (float)s->st_tticks,
           status->timeprod,
           t2, t, status->kills);
    r->offrat = s->st_tkills / t2;	/* calc offense rating */

    t2 = t * (float) status->dooshes;	/* expected armies dooshed */
    if(t2 == 0.0) t2 = 1.0;
    r->dooshrat = (float) s->st_tdooshes / t2;	/* doosh rating */

    r->battle = r->offrat + r->dooshrat;	/* get battle rating */

    t2 = t * (float) status->armsbomb;	/* expected armies bombed */
    if(t2 == 0.0) t2 = 1.0;
    r->bombrat = (float) s->st_tarmsbomb / t2;	/* bomb rating */

    t2 = t * (float) status->resbomb;	/* expected resources bombed */
    if(t2 == 0.0) t2 = 1.0;
    r->resrat = (float) s->st_tresbomb / t2;	/* resource bombed rating */

    t2 = t * (float) status->planets;	/* expected planets */
    if(t2 == 0.0) t2 = 1.0;
    r->planetrat = (float) s->st_tplanets / t2;	/* get planet rating */

printf("planetrat %f tplanets %f t2 %f\n", (float)r->planetrat, (float)s->st_tplanets, (float)t2);

    r->strategy = r->bombrat + r->resrat + r->planetrat;	/* strategy rating */

    t2 = (float) status->sbkills / (float) (status->sblosses ? status->sblosses : 1);
    if(t2 == 0.0) t2 = 1.0;
    if (s->st_sblosses == 0)
	r->sbrat = (float) s->st_sbkills / t2;
    else
	r->sbrat = ((float) s->st_sbkills / (float) s->st_sblosses) / t2;

    t2 = (float) status->wbkills / (float) (status->wblosses ? status->wblosses : 1);
    if(t2 == 0.0) t2 = 1.0;
    if (s->st_wblosses == 0)
	r->wbrat = (float) s->st_wbkills / t2;
    else
	r->wbrat = ((float) s->st_wbkills / (float) s->st_wblosses) / t2;

    t = (float) s->st_jsticks / (float) (status->jstime ? status->jstime : 1);
    t2 = t * (float) status->jsplanets;	/* get expected js planets */
    if (t2 == 0.0)
	r->jsrat = 0.0;
    else
	r->jsrat = (float) s->st_jsplanets / t2;	/* js rating */

    r->special = r->sbrat + r->wbrat + r->jsrat;	/* get special ship
							   rating */
}


/*-----------------------------------DEATH---------------------------------*/
/*  This function is called when the player dies.  It checks to see if the
player has been promoted.  */

void 
death(void)
{
    struct stats *s;		/* to point to player's stats */
    struct rating r;		/* filled in by compute_ratings() */
    int     genocides;		/* player's genocides */
    float   di;			/* player's di */

    me->p_status = POUTFIT;	/* Stop the ghost buster */
    switch (me->p_whydead) {	/* determine whether the player */
    case KTORP:		/* should be forced out of the */
    case KPLASMA:		/* game or not */
    case KPHASER:
    case KPLANET:
    case KSHIP:
    case KGENOCIDE:
    case KGHOST:
    case KPROVIDENCE:
    case KASTEROID:
    default:
	break;
    case KQUIT:
    case KDAEMON:
    case KWINNER:
	mustexit = 1;		/* set global var to force player out */
	break;
    }
    me->p_flags &= ~(PFWAR | PFREFITTING);	/* turn off most flags */
    if (me->p_stats.st_rank < NUMRANKS - 1) {	/* should we try to promote */
	s = &(me->p_stats);	/* get player's stat struct */
	genocides = s->st_genocides;	/* get # genocides */
	di = s->st_di;		/* get player's DI */

	compute_ratings(me, &r);

	/*--[ check for promotion ]--*/
	if (configvals->bronco_ranks) {
	    if ((di >= ranks[s->st_rank + 1].di)
		&& (r.battle >= ranks[s->st_rank + 1].battle)
		&& (r.strategy * 3 / 2 >= ranks[s->st_rank + 1].strategy)
		&& (r.special * 3 >= ranks[s->st_rank + 1].specship))
		s->st_rank++;	/* we have a promotion */
	}
	else {
	    if ((genocides >= ranks[s->st_rank + 1].genocides)
		&& (di >= ranks[s->st_rank + 1].di)
		&& (r.battle >= ranks[s->st_rank + 1].battle)
		&& (r.strategy >= ranks[s->st_rank + 1].strategy)
		&& (r.special >= ranks[s->st_rank + 1].specship))
		s->st_rank++;	/* we have a promotion */
	}
    }
    updateClient();		/* update the client */
    savestats();		/* save players stats */
}

/*-------------------------------------------------------------------------*/


/*----END OF FILE-----*/
