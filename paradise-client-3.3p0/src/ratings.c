/*
 * ratings.c,  2/13/94 Bill Dyess
 */
#include "copyright.h"

#include "config.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/*
   fills the ratings struct pointed to by r with the stats for the player
   pointed to by j [BDyess]
*/
struct ratings *
get_ratings(struct player *j, struct ratings *r)
{
    if (paradise) {
	struct stats2 *s;	/* point to player's paradise stats */
	float   t, t2;		/* temps */

	s = &(j->p_stats2);
	/* fill in kills, losses, and maxkills based on ship type */
	if (j->p_ship->s_type == STARBASE) {
	    r->r_kills = s->st_sbkills;
	    r->r_losses = s->st_sblosses;
	    r->r_maxkills = s->st_sbmaxkills;
	} else if (j->p_ship->s_type == WARBASE) {
	    r->r_kills = s->st_wbkills;
	    r->r_losses = s->st_wblosses;
	    r->r_maxkills = s->st_wbmaxkills;
	} else {
	    r->r_kills = s->st_tkills;
	    r->r_losses = s->st_tlosses;
	    r->r_maxkills = s->st_tmaxkills;
	}
	/* calculate ratio */
	r->r_ratio = (r->r_losses != 0) ? r->r_kills / (float) r->r_losses
	    : r->r_kills;
	/*
	   r->r_ratio = (s->st_tlosses != 0) ? (float) s->st_tkills / (float)
	   s->st_tlosses : s->st_tkills;
	*/
	status2->timeprod = status2->timeprod ? status2->timeprod : 1;
	t = (float) s->st_tticks / (float) status2->timeprod;	/* hour ratio */
	if (t == 0.0)
	    t = 1.0;
	t2 = t * (float) status2->losses;	/* get expected losses */
	if(t2 == 0) t2=1;
	r->r_defrat = s->st_tlosses / t2;	/* calc defense rating */

	t2 = t * (float) status2->kills;	/* get expected kills */
	if(t2 == 0) t2=1;
	r->r_offrat = s->st_tkills / t2;	/* calc offense rating */

	t2 = t * (float) status2->armsbomb;	/* expected armies bombed */
	if(t2 == 0) t2=1;
	r->r_bombrat = (float) s->st_tarmsbomb / t2;	/* bomb rating */

	t2 = t * (float) status2->resbomb;	/* expected resources bmbd */
	if(t2 == 0) t2=1;
	r->r_resrat = (float) s->st_tresbomb / t2;	/* resrce bmbd rating */

	t2 = t * (float) status2->dooshes;	/* expected armies dooshed */
	if(t2 == 0) t2=1;
	r->r_dooshrat = (float) s->st_tdooshes / t2;	/* doosh rating */

	r->r_batrat = r->r_dooshrat + r->r_offrat;	/* get battle rating */

	t2 = t * (float) status2->planets;	/* expected planets */
	if(t2 == 0) t2=1;
	r->r_planetrat = (float) s->st_tplanets / t2;	/* get planet rating */
	/* strategy rating */
	r->r_stratrat = r->r_bombrat + r->r_resrat + r->r_planetrat;
	/* calculate sb rating */
	t2 = (float) status2->sbkills / (float)( (status2->sblosses > 0) ? status2->sblosses : 1);
	if (s->st_sblosses == 0)
	    r->r_sbrat = 0.0;
	else if (t2 == 0)
	    r->r_sbrat = 99.0;
	else
	    r->r_sbrat = ((float) s->st_sbkills / (float) s->st_sblosses) / t2;
	/* calculate wb rating */
	t2 = (float) status2->wbkills / (float)( (status2->wblosses > 0) ? status2->wblosses : 1);
	if (s->st_wblosses == 0)
	    r->r_wbrat = 0.0;
	else if (t2 == 0)
	    r->r_wbrat = 99.0;
	else
	    r->r_wbrat = ((float) s->st_wbkills / (float) s->st_wblosses) / t2;
	/* calculate js rating */
	t = (float) s->st_jsticks / (float)( (status2->jstime > 0) ? status2->jstime : 1);
	t2 = t * (float) status2->jsplanets;	/* get expected js planets */
	if (t2 == 0.0)
	    r->r_jsrat = 0.0;
	else if (t2 == 0)
	    r->r_jsrat = 99.0;
	else
	    r->r_jsrat = (float) s->st_jsplanets / t2;	/* js rating */
	r->r_jsplanets = s->st_jsplanets;	/* store js planets */

	r->r_specrat = r->r_sbrat + r->r_wbrat + r->r_jsrat;	/* get special ship
								   rating */
	/* put the sum of the three major ratings in the 'ratings' slot */
	r->r_ratings = r->r_specrat + r->r_batrat + r->r_stratrat;
	r->r_genocides = s->st_genocides;	/* get # genocides */
	r->r_di = s->st_di;	/* get player's DI */
	t = (s->st_tticks) ? s->st_tticks : 1.0;
	r->r_killsPerHour = r->r_kills * 36000.0 / t;
	r->r_lossesPerHour = r->r_losses * 36000.0 / t;
	r->r_planets = s->st_tplanets;
	r->r_armies = s->st_tarmsbomb;
	r->r_resources = s->st_tresbomb;
	r->r_dooshes = s->st_tdooshes;
	/* r->r_jsplanets = s->st_jsplanets; */
    } else {			/* bronco stats */
	struct stats *s = &j->p_stats;

	r->r_offrat = offenseRating(j);	/* offense */
	r->r_planetrat = planetRating(j);	/* planet */
	r->r_bombrat = bombingRating(j);	/* bombing */
	r->r_offrat = offenseRating(j);	/* offense */
	r->r_defrat = defenseRating(j);	/* defense */
	r->r_resrat = 0;	/* these don't apply */
	r->r_dooshrat = 0;
	r->r_stratrat = 0;
	r->r_batrat = 0;
	r->r_sbrat = 0;
	r->r_wbrat = 0;
	r->r_jsrat = 0;
	r->r_jsplanets = 0;
	r->r_specrat = 0;
	r->r_ratings = r->r_offrat + r->r_planetrat + r->r_bombrat;	/* ratings */
	r->r_di = r->r_ratings * s->st_tticks / 36000.0;	/* di */
	/* fill in kills, losses, and ratio based on ship type */
	if (j->p_ship->s_type == STARBASE) {
	    r->r_kills = s->st_sbkills;
	    r->r_losses = s->st_sblosses;
	    r->r_ratio = (s->st_sblosses != 0)
		? (float) r->r_kills / (float) r->r_losses
		: (float) j->p_stats.st_sbkills;
	    r->r_maxkills = j->p_stats.st_sbmaxkills;
	} else {
	    r->r_kills = s->st_kills + s->st_tkills;
	    r->r_losses = s->st_losses + s->st_tlosses;
	    r->r_ratio = (r->r_losses != 0)
		? (float) r->r_kills / (float) r->r_losses
		: r->r_kills;
	    r->r_maxkills = s->st_maxkills;
	}
	r->r_planets = s->st_tplanets + s->st_planets;
	r->r_armies = s->st_armsbomb + s->st_tarmsbomb;
	/* not recorded in bronco */
	r->r_resources = 0;
	/* r->r_jsplanets = 0; */
	r->r_dooshes = 0;
	r->r_genocides = 0;
    }
    return r;
}
