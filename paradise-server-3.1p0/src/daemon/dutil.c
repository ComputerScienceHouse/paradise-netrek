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

/*--------------------------------KILLMESS--------------------------------*/

/*  This function prints to the messages who killed who.  This function
now adds the way of death onto the end of the kill message.  */


void 
killmess(struct player *victim, struct player *killer)
{
    char    buf[80];		/* to sprintf into */

    sprintf(buf, "%s (%s", victim->p_name, twoletters(victim));

    if (victim->p_armies) {
	if ((victim->p_ship.s_nflags & SFNHASFIGHTERS) && (victim->p_ship.s_type != STARBASE))
	    sprintf(buf + strlen(buf), "+%d fighters",
		    victim->p_ship.s_missilestored);
	else
	    sprintf(buf + strlen(buf), "+%d armies", victim->p_armies);
    }
    strcat(buf, ")");
    if (killer) {
	sprintf(buf + strlen(buf), " was kill %0.2f for %s (%s)",
		killer->p_kills, killer->p_name,
		twoletters(killer));

	if (friendly(victim, killer))
	    strcat(buf, "[no credit]");

    }
    else {
	if (victim->p_whydead == KPLANET) {
	    sprintf(buf + strlen(buf), " was %s by %s (%s)",
		    PL_TYPE(planets[victim->p_whodead]) == PLSTAR ?
		    "burned to a crisp" : "shot down",
		    planets[victim->p_whodead].pl_name,
		    teams[planets[victim->p_whodead].pl_owner].shortname);
	}
	else if (victim->p_whydead == KASTEROID)
	   sprintf(buf + strlen(buf), " was crushed by an asteroid");
	else {
	    strcat(buf, " was killed");
	}
    }
    switch (victim->p_whydead) {/* determine why player died */
    case KTORP:
	strcat(buf, "[torp]");
	break;
    case KPHASER:
	strcat(buf, "[phaser]");
	break;
    case KSHIP:
	strcat(buf, "[explosion]");
	break;
    case KPLASMA:
	strcat(buf, "[plasma]");
	break;
    case KASTEROID:
	strcat(buf, "[asteroid]");
	break;
    case KPLANET:
	switch (PL_TYPE(planets[victim->p_whodead])) {
	case PLSTAR:
	    strcat(buf, "[star]");
	    break;
	case PLPLANET:
	    strcat(buf, "[planet]");
	    break;
	default:
	    strcat(buf, "[space rock]");
	    break;
	}
	break;
    default:
	strcat(buf, "[unknown]");
    }
    pmessage(buf, 0, MALL | MKILLA, "GOD->ALL");
}




/*-------------------------------VISIBLE FUNCTIONS------------------------*/

/*-------------------------------CAUSE_KABOOM------------------------------*/
/*  This function sets the victim to explode and sets the counter for the
explosion views.  */

void 
cause_kaboom(struct player *victim)	/* which ship to blast to tiny frags */
{
    victim->p_status = PEXPLODE;/* set player as exploding */
    victim->p_explode = (short) get_explode_views(victim->p_ship.s_type);
}

/*------------------------------GET_EXPLODE_VIEWS--------------------------*/
/* returns the number of ship views for the given ship type */

int 
get_explode_views(short stype)
{
    switch (stype) {
    case STARBASE:
    case WARBASE:
    case JUMPSHIP:
	return 2 * SBEXPVIEWS / PLAYERFUSE;	/* big kablooey */
    }
    return 10 / PLAYERFUSE;	/* small kablooey */
}

/*------------------------------INFLICT_DAMAGE-----------------------------*/
/*  This function is used to inflict damage on a player.  If the player
dies as a result he will be made to explode.  The function returns a 1 if
the victim was killed and a 0 otherwise.  If the sp parameter is a zero then
the victim was damaged by something other than a ship.  It is the responsibil-
ity of the caller to set the whydead field of the victim.  */

/* args:
    struct player *sp;		 player that inflicted the damage
    struct player *op;		 other player that could be responsible
    struct player *victim;	 which ship to victimize
    int     damage;		 how much damage, should be positive
    int     why;		 the source of the damage */
int 
inflict_damage(struct player *sp, struct player *op, 
               struct player *victim, int damage, int why)
{
    if (damage < 0) {		/* should not be called with - damage */
	fprintf(stderr, "Attempt to inflict negative damage\n");
	return 0;
    }

    if (victim->p_flags & PFSHIELD) {	/* shields up? */
	int     penetrated;
	/*
	   if the server is configured for damage penetration then the
	   shields will not absorb all the damage
	*/
	penetrated = damage * configvals->penetration
	    * (victim->p_ship.s_maxshield - victim->p_shield)
	    / (victim->p_ship.s_maxshield);
	damage -= penetrated;

	victim->p_shield -= damage;	/* damage shields */
	if (victim->p_shield < 0) {	/* we punched through the shield */
	    damage = -victim->p_shield;	/* excess damage will apply to hull */
	    victim->p_shield = 0;	/* and zero the shields */
	}
	else
	    damage = 0;
	damage += penetrated;	/* some of the damage got through the shields */
    }
    if (damage > 0) {
	victim->p_damage += damage;	/* all damage to hull */

	/* does hull damage have a chance of killing armies I'm
	   carrying?  */
	if (configvals->kill_carried_armies > 0.0 &&
	    victim->p_damage > victim->p_ship.s_maxdamage/2 &&
	    victim->p_armies > 0)
	{
	  int ak;
	  int max_cankill =
	    ((victim->p_damage - (victim->p_ship.s_maxdamage / 2)) * 
	      victim->p_armies) / (victim->p_ship.s_maxdamage / 2);

          /* possible # of armies to kill, up to proportional amount */
	  ak = (max_cankill * (lrand48() % 101)) / 100;

          /* if we beat the odds, no armies lost */
	  if(drand48() >= configvals->kill_carried_armies)
	    ak = 0;

	  if(ak > 0)
	  {
	    victim->p_armies -= ak;
            if(sp && !friendly(sp, victim) && sp != victim)
	    {
	      char buf[80];

	      sp->p_dooshes += ak;
	      status->dooshes += ak;
	      sp->p_stats.st_di += 0.02 * 5.0 * (float)victim->p_armies;
	      sp->p_kills += (float)ak / 10.0;

              sprintf(buf, "%s (%s) had a partial doosh of %s (%s+%d armies)",
	              sp->p_name, twoletters(sp), 
		      victim->p_name, twoletters(victim), ak);

              pmessage(buf, 0, MALL | MKILLA, "GOD->ALL");
	    }
	  }
	}

	if (configvals->erosion > 0) {
	    float   chance = damage * configvals->erosion;
	    while (chance >= 0.5) {	/* no matter how much you suffer
					   there's a chance you can avoid
					   permanent damage */
		if (lrand48() & 0x40) {
		    victim->p_ship.s_maxdamage--;	/* apply damage to
							   maximum */
		    victim->p_damage--;	/* instead of current */
		}
		chance -= 0.5;
	    }
	    if (drand48() < chance) {
		victim->p_ship.s_maxdamage--;	/* apply damage to maximum */
		victim->p_damage--;	/* instead of current */
	    }
	}
    }

    if (victim->p_damage >= victim->p_ship.s_maxdamage) {	/* victim dead? */
	cause_kaboom(victim);	/* make him explode */
	if (sp) {
	    victim->p_whydead = why;
	    if (!friendly(sp, victim)
		&& sp != victim	/* hozers were getting credit for killing
				   themselves because they were at war with
		    their own race */ ) {
		/* if a hostile player was responsible */
		tlog_plkill(victim, sp, op);
		if (victim->p_ship.s_type == PATROL)
		    sp->p_kills += .5 + ((float) victim->p_armies + victim->p_kills) / 10.0;
		else
		    sp->p_kills += 1.0 + ((float) victim->p_armies + victim->p_kills) / 10.0;
		killerstats(sp->p_no, victim);	/* adjust everyones stats */
		checkmaxkills(sp->p_no);
		killmess(victim, sp);
		victim->p_whodead = sp->p_no;
	    }
	    else if (op && !friendly(op, victim) && op != victim) {
		/* the primary assassin was friendly, check auxiliary killer */
		tlog_plkill(victim, op, sp);

		if (victim->p_ship.s_type == PATROL)
		    op->p_kills += .5 + ((float) victim->p_armies + victim->p_kills) / 10.0;
		else
		    op->p_kills += 1.0 + ((float) victim->p_armies + victim->p_kills) / 10.0;
		killerstats(op->p_no, victim);	/* adjust everyones stats */
		checkmaxkills(op->p_no);
		killmess(victim, op);
		victim->p_whodead = op->p_no;
	    }
	    else {
		/*
		   give no credit since it was friendly and the auxiliary
		   murderer was friendly too
		*/
		tlog_plkill(victim, sp, (struct player *) 0);
		killmess(victim, sp);
		victim->p_whodead = sp->p_no;
	    }
	}
	loserstats(victim->p_no);
	return 1;		/* victim died */
    }
    else
	return 0;		/* victim lived */
}

/*-------------------------------------------------------------------------*/


#if 0
struct ranksorter {
    int     pno;
    int     rank;
    float   di;
    int     teammates;
};

static struct ranksorter admirals[MAXTEAM];
#endif

int 
enemy_admiral(int tno)
{
    int     teammates;
    int     pno;
    int     i;

    struct 
    {
      int pno;
      int rank;
      float di;
      int teammates;
    } admirals[MAXTEAM];

    for (i = 0; i < MAXTEAM; i++) {
	admirals[i].teammates = 0;
	admirals[i].rank = -1;
    }

    for (i = 0; i < MAXPLAYER; i++) {
	int     team;
	struct player *pl;
	pl = &players[i];

	if (pl->p_status == PFREE ||
	    pl->p_team == tno)
	    continue;

	team = pl->p_team;
	admirals[team].teammates++;

	if (pl->p_stats.st_rank < admirals[team].rank)
	    continue;
	if (pl->p_stats.st_rank > admirals[team].rank ||
	    pl->p_stats.st_di > admirals[team].di) {
	    admirals[team].pno = i;
	    admirals[team].rank = pl->p_stats.st_rank;
	    admirals[team].di = pl->p_stats.st_di;
	}
    }

    teammates = -1;
    pno = 0;
    for (i = 0; i < MAXTEAM; i++) {
	if (admirals[i].teammates > teammates) {
	    pno = admirals[i].pno;
	    teammates = admirals[i].teammates;
	}
    }
    return pno;
}


/*--------END OF FILE-------*/
