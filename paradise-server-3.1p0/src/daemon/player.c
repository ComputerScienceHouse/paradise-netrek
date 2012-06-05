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
#include "daemonII.h"
#include "data.h"
#include "shmem.h"

/*-----------------------------NUMBER DEFINES----------------------------*/
 /* defines dealing with ships exploding */
#define MAXDAMDIST 3000		/* range at which shipexplosions do damage */

 /* other defines */
#define YRANGE ((100000)/7)	/* range yellow alert comes on */
#define RRANGE ((100000)/10)	/* range red alert comes on */


#define	DOCKDAMAGE	10	/* the amount of damage inflicted when you
				   lose a docking ring... */

/*-----------------------------------------------------------------------*/

/*---------------------------EXTERNAL GLOBALS-----------------------------*/
/* from daemonII.c */
extern int ticks;
extern int dietime;
/* this doesn't look like it's used BUT check the definition of NotTmode 
   in daemonII.h */
extern int tourntimestamp;

/*---------------------------INTERNAL FUNCTIONS---------------------------*/

/*-----------------------------BEAMMEUPSCOTTY-----------------------------*/
/*  This function does the beamup for a player.  It allows the player
to beam up all the armies on a planet.  */

static void 
beammeupscotty(struct player *j)	/* the player beaming */
{
    struct planet *l;		/* to point to planet beaming from */

    if (j->p_flags & PFORBIT) {	/* if orbiting a planet */
	l = &planets[j->p_planet];	/* get planet player is beaming from */
	if (l->pl_armies == 0	/* cannot beam if no armies */
	/* this prevents you from beaming up outside T-mode in a league game */
	    || (!(configvals->evacuation) && (l->pl_armies < 5))
	    || (status2->league && !status->tourn)
	    || j->p_armies >= j->p_ship.s_maxarmies
	    || ((j->p_ship.s_nflags & SFNARMYNEEDKILL)
	   && (j->p_armies >= (int) (j->p_kills * j->p_ship.s_armyperkill)))
	    ) {
	    j->p_flags &= ~PFBEAMUP;
	    return;		/* max armies for number of kills? */
	}
	if (l->pl_owner != j->p_team)
	    return;		/* no picking up foreign troops */

	tlog_beamup(l, j);

	if (j->p_ship.s_nflags & SFNHASFIGHTERS) {
	    j->p_ship.s_missilestored = j->p_ship.s_missilestored + FAE_RATE;
	    j->p_armies = (int) (j->p_ship.s_missilestored / FAE_RATE);
	}
	else
	    j->p_armies++;	/* pick an army up */

	l->pl_armies--;		/* dec armies on planet */
	l->pl_tinfo[j->p_team].armies = l->pl_armies;	/* update info */
	l->pl_tinfo[j->p_team].timestamp = status->clock;	/* timestamp it */

	if (l->pl_armies == 0) {/* if we beamed them all up then */
	    l->pl_tinfo[l->pl_owner].owner = NOBODY;	/* no longer owner */
	    /* these penalties apply even outside of T-mode */
	    j->p_stats.st_tplanets--;	/* prevent scumming */

	    j->p_stats.st_di -= 0.25;
	    if (j->p_stats.st_di < 0.0)
		j->p_stats.st_di = 0.0;

	    j->p_kills -= 0.25;
	    if (j->p_kills < 0.0)
		j->p_kills = 0.0;

	    tlog_planaban(l, j);

	    l->pl_owner = NOBODY;	/* planet goes to nobody */
	    checkwin(enemy_admiral(j->p_no));	/* check for winner */
	}
	if (j->p_lastman == 2)	/* KAO */
	    /* j->p_flags &= ~PFBEAMUP; */
	    j->p_lastman = 1;

    }
    else if (j->p_flags & PFDOCK) {	/* else if docked to another ship */
       if (players[j->p_docked].p_armies == 0
           || j->p_armies >= j->p_ship.s_maxarmies
           || ((j->p_ship.s_nflags & SFNARMYNEEDKILL)
            && (j->p_armies >= (int)(j->p_kills * j->p_ship.s_armyperkill))))
        {
           j->p_flags &= ~PFBEAMUP;
           return;
        }

	tlog_Bbeamup(&players[j->p_docked], j);

	if (j->p_ship.s_nflags & SFNHASFIGHTERS) {
	    j->p_ship.s_missilestored = j->p_ship.s_missilestored + FAE_RATE;
	    j->p_armies = (int) (j->p_ship.s_missilestored / FAE_RATE);
	}
	else {
	    j->p_armies++;	/* add to armies */
	    players[j->p_docked].p_armies--;	/* subtract from dockee */
	}
    }
}




/*-------------------------------BEAMDOWN---------------------------------*/
/*  This function beams armies down to a planet or to a ship the player
is dowcked on.  This function also has Kurt's mod for beaming while at
different alert statuses.  You get more stats from taking while alert status
is red than green.  */

static void 
beamdown(struct player *j)	/* the player beaming */
{
    char    buf[90];		/* to sprintf into */
    char    buf1[90];
    struct planet *l;		/* the planet beaming to */
    /* int oldowner; */		/*to keep track of old planet owner */

    if (j->p_armies == 0)	/* player cannot beam down if */
	return;			/* he has no armies */

    if (j->p_flags & PFORBIT) {	/* if beaming to planet then */

	l = &planets[j->p_planet];	/* get planet beaming to */
	/* oldowner = l->pl_owner; */	/*record old owner */
	if ((!((j->p_swar | j->p_hostile) & l->pl_owner))
	    && (j->p_team != l->pl_owner) && (l->pl_owner != NOBODY))
	    return;		/* no beaming down if not hostile */

	tlog_beamdown(l, j);

	if (l->pl_owner == j->p_team) {	/* if beaming to own planet */
	    if ((j->p_ship.s_nflags & SFNHASFIGHTERS) &&
		(j->p_ship.s_missilestored >= FAE_RATE)) {
		j->p_ship.s_missilestored = j->p_ship.s_missilestored - FAE_RATE;
		j->p_armies = (int) (j->p_ship.s_missilestored / FAE_RATE);
	    }
	    else
		j->p_armies--;	/* decrease by one army */

	    l->pl_armies++;	/* increase planet armies */
	    l->pl_tinfo[j->p_team].armies = l->pl_armies;
	    l->pl_tinfo[j->p_team].timestamp = status->clock;
	}
	else {			/* else beaming to foreign planet */
	    j->p_swar |= l->pl_owner;	/* start war if necessay */
	    j->p_armies--;	/* beam an army down */
	    /* the defender might get a chance to destroy beamed-down
	       army.  Check configvals. */
	    if(l->pl_armies > 0 && (l->pl_flags & PLRESMASK) && 
	       configvals->army_defend_facilities > 0.0)
	    {
	      if(drand48() > configvals->army_defend_facilities)
	        l->pl_armies--;
	    }
	    else if(l->pl_armies > 0 && !(l->pl_flags & PLRESMASK) &&
	            configvals->army_defend_bare > 0.0)
	    {
	      if(drand48() > configvals->army_defend_bare)
	        l->pl_armies--;
	    }
	    else
	      l->pl_armies--;
	    l->pl_tinfo[j->p_team].armies = l->pl_armies;
	    l->pl_tinfo[j->p_team].timestamp = status->clock;
	    l->pl_tinfo[l->pl_owner].armies = l->pl_armies;

	    credit_armiesbombed(j, 1, l);

	    if (l->pl_armies == 0) {	/* if all armies knocked off */
		(void) sprintf(buf, "%s destroyed by %s (%s)", l->pl_name,
			       j->p_name, twoletters(j));
		(void) sprintf(buf1, "%-3s->%-3s", l->pl_name,
			       teams[l->pl_owner].shortname);
		pmessage(buf, l->pl_owner, MTEAM | MDEST, buf1);
		tlog_plandest(l, j);
		l->pl_tinfo[l->pl_owner].owner = NOBODY;
		l->pl_tinfo[l->pl_owner].timestamp = status->clock;
		l->pl_tinfo[j->p_team].owner = NOBODY;
		l->pl_tinfo[j->p_team].armies = 0;
		l->pl_tinfo[j->p_team].timestamp = status->clock;
		l->pl_owner = NOBODY;
		l->pl_trevolt = 0;	/* stop revolution */
		if (NotTmode(ticks)) {	/* if not t-mode then */
		    rescue(STERMINATOR, j->p_no, l->pl_no);	/* send in the
								   terminators */
		    rescue(STERMINATOR, j->p_no, l->pl_no);
		}
		checkwin(j->p_no);	/* check for genocide */
	    }
	    else if (l->pl_armies < 0) {	/* planet taken over */
		l->pl_armies *= -1;	/* newly taken planet has one army */
		if (status->tourn) {	/* if in t-mode then */
		    j->p_planets++;	/* inc game planets taken */
		    j->p_stats.st_tplanets++;	/* inc t-mode planets taken */
		    j->p_stats.st_di += 0.25;	/* inc DI for player */
		    status->planets++;	/* inc global planets */

		    if ((j->p_jsdock > 0) && (j->p_jsdock < 600)) {	/* give JS credit? */
			struct player *js = &players[j->p_lastjs];	/* yes */
			(void) sprintf(buf1, "%-3s->%s ", l->pl_name,
				       twoletters(js));
			(void) sprintf(buf, "Good assist, %s", js->p_name);
			js->p_stats.st_jsplanets++;
			
			if(configvals->js_assist_credit)
			    js->p_stats.st_tplanets++;

			status->jsplanets++;
			pmessage(buf, j->p_lastjs, MINDIV | MTAKE, buf1);
		    }

		    j->p_kills += 0.25;	/* inc kills for taking planet */
		    checkmaxkills(j->p_no);	/* check max kills */
		}
		(void) sprintf(buf, "%s taken over by %s (%s)", l->pl_name,
			       j->p_name, twoletters(j));
		l->pl_owner = j->p_team;	/* switch owner */
		tlog_plantake(l, j);
		l->pl_hinfo |= j->p_team;	/* set up info */
		l->pl_tinfo[j->p_team].owner = l->pl_owner;
		l->pl_tinfo[j->p_team].armies = l->pl_armies;
		l->pl_tinfo[j->p_team].flags = l->pl_flags;
		l->pl_tinfo[j->p_team].timestamp = status->clock;
		checkwin(j->p_no);	/* check for win */
		(void) sprintf(buf1, "%-3s->%-3s", l->pl_name, teams[l->pl_owner].shortname);
		pmessage(buf, l->pl_owner, MTEAM | MTAKE, buf1);
	    }
	}
    }				/* end of beaming to foreign planets */
    else if (j->p_flags & PFDOCK) {	/* if beaming to ship docked to */
	if (players[j->p_docked].p_team != j->p_team)
	    return;		/* no beaming to foreign dockees */
	if (players[j->p_docked].p_armies ==
	    players[j->p_docked].p_ship.s_maxarmies)
	    return;		/* no beaming over max armies */
	tlog_Bbeamdown(&players[j->p_docked], j);
	j->p_armies--;		/* transfer one army over */
	players[j->p_docked].p_armies++;
    }
}



/*----------------------------------BLOWUP--------------------------------*/
/*  This function does the explosion damage of a ship explosion.  It
inflicts damage on the nearby players.  The damage has been changed to be
based on the amount of fuel the ship has when it explodes.  */

static void 
blowup(struct player *sh)	/* the player that blew up */
{
    register int i;		/* looping vars */
    int     dx, dy;		/* delta coords of a ship */
    double  dist2;		/* for distance of ship (sqared) */
    double  maxdist, maxdist2;	/* to hold max damage dist */
    int     damage;		/* to hold calculated damage */
    register struct player *j;	/* to point to other players */
    double  ft, expl;		/* floating point temp */


    /* moved some of this stuff out of the for loop... */
    maxdist = sqrt((double) MAX(sh->p_fuel, 0) / 6.0) * 20.0;
    if (maxdist > MAXDAMDIST)
	maxdist = MAXDAMDIST;
    maxdist2 = maxdist * maxdist;

    expl = (double) ((sh->p_ship.s_expldam
		      + sqrt(MAX(sh->p_fuel, 0) / (double) sh->p_ship.s_maxfuel) * sh->p_ship.s_fueldam)
		     / get_explode_views(sh->p_ship.s_type));

    if (sh->p_whydead == KGHOST)
	return;			/* no gostbusted ships blowing up */

    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	struct player	*me;
	if ((j->p_status != PALIVE) || (sh == j))	/* player no alive or is
							   same */
	    continue;		/* as exploding ship, then continue */
	me = sh;		/* for friendlyPlayer macro below */
	if ((sh->p_whydead == KQUIT) && (friendlyPlayer(j)))
	    continue;		/* No quiting to blow up on people */
	dx = sh->p_x - j->p_x;	/* delta coords */
	dy = sh->p_y - j->p_y;
	if ((ABS(dx) > (int) maxdist) || (ABS(dy) > (int) maxdist))
	    continue;		/* continue if obviously too far away */
	dist2 = (double) dx *(double) dx + (double) dy *(double) dy;
	if (dist2 >= maxdist2)	/* if too far away to damage */
	    continue;		/* then check next player */

	ft = sqrt(1.0 - dist2 / maxdist2);
	damage = (int) (expl * ft);	/* scale by distance */
	if (damage > 0) {	/* if damage done then */
	    /*
	       inflict damage, and maybe credit the person who killed us if
	       we cause the death of a teammate
	    */
	    inflict_damage(sh, &players[sh->p_whodead], j, damage, KSHIP);
	}
    }
}




/*--------------------------------DOSHIPEXPLODE---------------------------*/
/*  This function makes a ship explode.  It resets various fields in the
players structure and decs the timer before the player's explosion is done
and he is really dead.  */

static void 
doshipexplode(struct player *j)	/* the player to explode */
{
    int     k;			/* another damned looping var */

    j->p_flags &= ~PFCLOAK;	/* the cloaking device is kaputt */

    blowup(j);			/* damage surrounding players */

    if (--j->p_explode <= 0) {	/* dec explode timer until really gone */
	j->p_status = PDEAD;	/* explosion done--make player dead */
	j->p_explode = 600 / PLAYERFUSE;	/* set timer for ghost buster */
    }
    undock_player(j);		/* if player docked then undock him */

    if (allows_docking(j->p_ship)) {	/* if ships can dock */
	for (k = 0; k < j->p_ship.s_numports; k++)	/* remove all docked
							   ships */
	    base_undock(j, k);
	j->p_docked = 0;	/* no ships docked anymore */
    }
    if (j->p_flags & PFORBIT) {	/* if orbiting then */
	j->p_flags &= ~PFORBIT;	/* eject him from orbit */
    }				/* reset ship timers */
    if (j->p_status == PDEAD) {
	/*
	   this is the last time doshipexplode will be called for this
	   player. increment the rebuild timer
	*/
	if ((j->p_whydead == KSHIP) || (j->p_whydead == KTORP)
	    || (j->p_whydead == KPHASER) || (j->p_whydead == KPLASMA)
	    || (j->p_whydead == KPLANET) || (j->p_whydead == KGENOCIDE))
	    /* we have to build ANOTHER one */
	    if (status->tourn
		|| configvals->affect_shiptimers_outside_T)
		teams[j->p_team].s_turns[j->p_ship.s_type] += j->p_ship.s_timer;
    }
}




/*----------------------------------DOORBIT--------------------------------*/
/*  This function makes a player orbit around a planet by adjusting his
coordinates to go around in a circle.   */

static void 
doorbit(struct player *j)	/* the player in orbit */
{
    int     x, y;
    int     angle;

    if (j->p_orbitdir)
	j->p_dir += 2;		/* make player rotate */
    else
	j->p_dir -= 2;		/* make player rotate */

    j->p_desdir = j->p_dir;

    angle = j->p_dir + (j->p_orbitdir ? -64 : 64);
    x = planets[j->p_planet].pl_x + ORBDIST	/* new x coord */
	* Cos[(unsigned char) angle];
    y = planets[j->p_planet].pl_y + ORBDIST	/* new y coord */
	* Sin[(unsigned char) angle];
    move_player(j->p_no, x, y, 1);
}


static void 
repair_docking_ring(struct player *j)
{
    int     i;
    int     damaged = 0;
    for (i = 0; i < j->p_ship.s_numports; i++) {
	if (j->p_port[i] == PDAMAGE)
	    damaged++;
    }
    /* check to see if we've repaired any docking ring damage */
    for (i = 0;
    (damaged - 1) * DOCKDAMAGE + 1 > j->p_damage && i < j->p_ship.s_numports;
	 i++) {
	if (j->p_port[i] == PDAMAGE) {
	    j->p_port[i] = VACANT;
	    damaged--;
	}
    }
}

/*---------------------------------DORESOURCES----------------------------*/
/*  This function adjusts various things on a ship, like the fuel regen-
eration, repair, etc.  Some changes:  fuel cost of cloaking is based on
the speed of the ship.  There is a ver for how fast each ship can take on
fuel from  a planet of other ship  */

static void 
doresources(struct player *j)	/* the player in orbit */
{
    int     factor;
    int     ccost;
    /* Charge for shields */
    if (j->p_flags & PFSHIELD)
	j->p_fuel -= j->p_ship.s_shieldcost;

    /* cool weapons */
    j->p_wtemp -= j->p_ship.s_wpncoolrate;	/* subract from W-temp */
    if (j->p_wtemp < 0)		/* no going below zero */
	j->p_wtemp = 0;
    if (j->p_flags & PFWEP) {	/* if weapon temped */
	if (--j->p_wtime <= 0)	/* then w-tep ends when */
	    j->p_flags &= ~PFWEP;	/* w timer goes to zero */
    }
    else if (j->p_wtemp > j->p_ship.s_maxwpntemp) {	/* weapons too hot? */
	if (!(lrand48() % 40)) {/* chance to turn on W-temp */
	    j->p_flags |= PFWEP;/* W-temp the poor sucker */
	    j->p_wtime = ((lrand48() % 150) + 100) / PLAYERFUSE;
	}
    }
    /* cool engine */
    j->p_etemp -= j->p_ship.s_egncoolrate;	/* cool the engine */
    if (j->p_etemp < 0)		/* no going below zero */
	j->p_etemp = 0;
    if (j->p_flags & PFENG) {	/* if E-temped */
        /* see if timer has expired and we're not hot.  if we don't
         * check for current temp, PFENG will blink on and off and
         * you can actually move in a bursty fashion :) */
	if ( (--j->p_etime <= 0) && !(j->p_etemp > j->p_ship.s_maxegntemp) )
	    j->p_flags &= ~PFENG;	/* turn off E-temp if need be */
    }
    else if (j->p_etemp > j->p_ship.s_maxegntemp) {	/* engine too hot? */
	if (!(lrand48() % 20)) {/* random chance for E-temp */
	    j->p_flags |= PFENG;/* set E-temp flag */
	    j->p_etime = ((lrand48() % 150) + 100) / PLAYERFUSE;	/* set E-tmp timer */
	    j->p_desspeed = 1;	/* desired speed goes to zero (one?) */
	}
    }
    if (configvals->newcloak)	/* cloak cost based on speed */
	ccost = (int) (((float) j->p_ship.s_cloakcost / 10.0) *
			isqrt(j->p_speed + 1));
    else
	ccost = j->p_ship.s_cloakcost / 5;	/* 5 because regens have
						   been doubled since
						   bronco */
    if (j->p_flags & PFCLOAK) {	/* do cloaking cost */
	if (j->p_fuel < ccost)	/* if not enough fuel */
	    j->p_flags &= ~PFCLOAK;	/* then decloak */
	else
	    j->p_fuel -= ccost;
    }
    /* add to fuel */
    if ((j->p_flags & PFORBIT)	/* if orbiting */
	&&(planets[j->p_planet].pl_flags & PLFUEL)	/* a fuel planet */
	&&(!(planets[j->p_planet].pl_owner	        /* and not hostile */
	     & (j->p_swar | j->p_hostile))))  {	        /* or at war */
	j->p_fuel += j->p_ship.s_takeonfuel * 2;	/* then take on fuel */
        if( configvals->helpfulplanets )   {            /* helpful planets? */
            j->p_etemp -= j->p_ship.s_egncoolrate;      /* cool engine too */
            if( j->p_etemp < 0 )
                j->p_etemp = 0;
        }
    }   else if ((j->p_flags & PFORBIT)	                /* if orbiting */
	     &&(!(planets[j->p_planet].pl_owner	        /* and not hostile */
		  & (j->p_swar | j->p_hostile))))  {	/* or at war */
        if( configvals->helpfulplanets )   {            /* helpful planets? */
            j->p_etemp -= j->p_ship.s_egncoolrate;      /* cool engine too */
            if( j->p_etemp < 0 )
                j->p_etemp = 0;
            j->p_fuel += j->p_ship.s_takeonfuel;
	    if( j->p_fuel > j->p_ship.s_maxfuel )
                j->p_fuel = j->p_ship.s_maxfuel;        /* don't let it run over */
        }   else
	    j->p_fuel += j->p_ship.s_takeonfuel / 3;	/* then take on fuel */
    }   else if ((j->p_flags & PFDOCK)	/* if docked */
	     &&(j->p_fuel < j->p_ship.s_maxfuel)) {	/* and room for fuel */
	struct player *base = &players[j->p_docked];
	if ((base->p_fuel > base->p_ship.s_mingivefuel)	/* can't fuel below min */
	    &&(base->p_ship.s_nflags & SFNCANFUEL)
	    && (j->p_fuel < j->p_ship.s_maxfuel)) {
	    j->p_fuel += j->p_ship.s_takeonfuel;	/* suck some fuel off */
	    base->p_fuel -= j->p_ship.s_takeonfuel;
	}
    }
    /* else *//* if in free space */
    else if ((j->p_flags & PFDOCK)	/* if docked */
	     &&(j->p_fuel >= j->p_ship.s_maxfuel) &&
	     (j->p_ship.s_type == PATROL)) {
	if (j->p_ship.s_missilestored < shipvals[PATROL].s_missilestored) {
	    struct player *base = &players[j->p_docked];
	    base->p_fuel -= j->p_ship.s_missile.cost;
	    j->p_ship.s_missilestored++;
	}
    }
    else
	j->p_fuel += j->p_ship.s_recharge;	/* add regen fuel */

    if (j->p_fuel > j->p_ship.s_maxfuel) {	/* over max fuel? */
	if (j->p_flags & PFDOCK) {
	    struct player *base = &players[j->p_docked];
	    if (base->p_fuel < base->p_ship.s_maxfuel) {	/* give excess to base */
		base->p_fuel += j->p_fuel - j->p_ship.s_maxfuel;
		if (base->p_fuel > base->p_ship.s_maxfuel)
		    base->p_fuel = base->p_ship.s_maxfuel;
	    }
	}
	j->p_fuel = j->p_ship.s_maxfuel;	/* set to max */
    }
    if (j->p_fuel < 0) {	/* if below zero */
	j->p_desspeed = 0;	/* come to a stop */
	j->p_flags &= ~PFCLOAK;	/* uncloak */
	j->p_fuel = 0;		/* set it to zero */
    }
    /* repair stuff */
    if (j->p_flags & PFREPAIR)
	factor = 2;
    else
	factor = 1;
    if ((j->p_flags & PFORBIT) && !((j->p_swar |
			   j->p_hostile) & planets[j->p_planet].pl_owner)) {
	switch (planets[j->p_planet].pl_flags & (PLREPAIR | PLSHIPYARD)) {
	case 0:
	    factor += 1;
	    break;
	case PLREPAIR:
	    factor += 4;
	    break;
	case PLSHIPYARD:
	    factor += 2;
	    break;
	case PLREPAIR | PLSHIPYARD:
	    factor += 5;
	    break;
	}
    }
    else if ((j->p_flags & PFDOCK) &&
	     (players[j->p_docked].p_ship.s_nflags & SFNCANREPAIR)) {
	factor += 3;
    }
    if (j->p_shield < j->p_ship.s_maxshield) {	/* if shields damaged */
	/* repair the shields */
	j->p_subshield += j->p_ship.s_repair * factor;	/* add to shields fract */

	if (j->p_subshield / 1000) {	/* if fract over 1000 */
	    j->p_shield += j->p_subshield / 1000;	/* then add to shields */
	    j->p_subshield %= 1000;	/* take mod */
	}
	if (j->p_shield > j->p_ship.s_maxshield) {	/* went over max */
	    j->p_shield = j->p_ship.s_maxshield;	/* then no overflow */
	    j->p_subshield = 0;
	}
    }				/* end of if shields damaged */
    /* do repair of ship */
    if (j->p_damage && !(j->p_flags & PFSHIELD)) {
	j->p_subdamage += j->p_ship.s_repair * factor;	/* add to fract repair */
	if (j->p_subdamage / 1000) {	/* if fract repair too high */
	    j->p_damage -= j->p_subdamage / 1000;	/* take away real damage */
	    j->p_subdamage %= 1000;	/* mod the fract repair */
	}
	if (j->p_damage < 0) {	/* do not want damge < 0 */
	    j->p_damage = 0;	/* set it */
	    j->p_subdamage = 0;	/* zero the fract part too */
	}
    }
    if (allows_docking(j->p_ship))
	repair_docking_ring(j);
}




/*--------------------------------DOBOUNCE--------------------------------*/
/*  This function checks to see if the player has hit a wall.  If he has his
direction and position are adjusted.  */

static void 
dobounce(struct player *j)	/* the player to bounce */
{
    int     x = j->p_x;
    int     y = j->p_y;
    if (j->p_x < 0) {		/* past left wall? */
	x = -j->p_x;		/* set him to right of wall */
	j->p_dir = j->p_desdir = 64 - (j->p_dir - 192);	/* adjust direction */
    }
    else if (j->p_x > configvals->gwidth) {	/* past right wall? */
	x = configvals->gwidth - (j->p_x - configvals->gwidth);
	/* set him left of wall */
	j->p_dir = j->p_desdir = 192 - (j->p_dir - 64);	/* adjust direction */
    }
    if (j->p_y < 0) {		/* past top wall? */
	y = -j->p_y;		/* set inside galactic */
	j->p_dir = j->p_desdir = 128 - j->p_dir;	/* adjust direction */
    }
    else if (j->p_y > configvals->gwidth) {	/* below bottom wall */
	y = configvals->gwidth - (j->p_y - configvals->gwidth);
	/* place on right side of bed */
	j->p_dir = j->p_desdir = 0 - (j->p_dir - 128);	/* adjust position */
    }
    move_player(j->p_no, x, y, 1);
}




/*---------------------------------DOALERT-------------------------------*/
/*  This function sets the player's alert status by checking the other
players in the game and seeing if they are too close.  Should probably be
moved to the code that does visibility.  */

static void 
doalert(struct player *j)	/* the player to check for */
{
    int     k;			/* Oh, no.  Another looping var */
    int     dx, dy, dist;	/* to find distances */

    j->p_flags |= PFGREEN;	/* default is green status */
    j->p_flags &= ~(PFRED | PFYELLOW);	/* clear red and yellow alert */
    for (k = 0; k < MAXPLAYER; k++) {	/* go through all players */
	if ((players[k].p_status != PALIVE)	/* should we check player */
	    ||((!((j->p_swar | j->p_hostile) & players[k].p_team))
	  && (!((players[k].p_swar | players[k].p_hostile) & j->p_team)))) {
	    continue;		/* no, don't waste the time */
	}
	else if (j == &players[k])	/* do not check ourself */
	    continue;
	else {			/* we will check this player */
	    dx = j->p_x - players[k].p_x;	/* take delta coords */
	    dy = j->p_y - players[k].p_y;
	    if (ABS(dx) > YRANGE || ABS(dy) > YRANGE)	/* obviously out of
							   yellow */
		continue;	/* stop checking him */
	    dist = dx * dx + dy * dy;	/* calc dist squared */
	    if ((dist < YRANGE * YRANGE)) {	/* close enough for yellow
						   alert? */
		j->p_flags |= PFYELLOW;
		j->p_flags &= ~(PFGREEN);
	    }
	    if (dist < RRANGE * RRANGE) {	/* if close enough for red
						   alert */
		j->p_flags |= PFRED;	/* set red alert status */
		j->p_flags &= ~(PFGREEN | PFYELLOW);
	    }
	    if (j->p_flags & PFRED)	/* if yellow set then check no */
		break;		/* further */
	}
    }
}




/*--------------------------------CHANGEDIR--------------------------------*/
/*  This function does the turning for a ship.  It changes the players
direction to his desired direction, with the rate of change based on the
player's speed and maneuveribility of his ship.  This function includes
optional use of TC's new-style turn rates.  */

static void 
changedir(struct player *sp)	/* the player to turn */
{
    unsigned int ch_ticks;
    unsigned int min, max;
    int     speed;

    speed = sp->p_speed;	/* get player's speed */
    if (speed == 0) {		/* if player is at speed zero then */
	sp->p_dir = sp->p_desdir;	/* change in direction is instant */
	sp->p_subdir = 0;
    }
    else {			/* else we are moving */
	if (configvals->newturn)/* newstyle turn */
	    sp->p_subdir += sp->p_ship.s_turns / (speed * speed);
	else			/* old style turn */
	    sp->p_subdir += sp->p_ship.s_turns / ((sp->p_speed < 30) ?
					   (1 << sp->p_speed) : 1000000000);
	ch_ticks = sp->p_subdir / 1000;	/* get upper digits of subdir */
	if (ch_ticks) {		/* if more than one then we turn */
	    if (sp->p_dir > sp->p_desdir) {	/* find the min and max of
						   current */
		min = sp->p_desdir;	/* direction and desired direction */
		max = sp->p_dir;
	    }
	    else {
		min = sp->p_dir;
		max = sp->p_desdir;
	    }
	    if ((ch_ticks > max - min) || (ch_ticks > 256 - max + min))	/* can we immediately */
		sp->p_dir = sp->p_desdir;	/* get to desired direction */
	    else if ((unsigned char) ((int) sp->p_dir - (int) sp->p_desdir) > 127)
		sp->p_dir += ch_ticks;	/* else move to the right */
	    else
		sp->p_dir -= ch_ticks;	/* move to the left */
	    sp->p_subdir %= 1000;	/* take off upper digits */
	}
    }
}




static int 
being_tractored(struct player *victim)
{
    int     i;
    for (i = 0; i < MAXPLAYER; i++)
	if (players[i].p_status == PALIVE
	    && (players[i].p_flags & (PFTRACT | PFPRESS))
	    && (players[i].p_tractor == victim->p_no)
	    && (players[i].p_no != victim->p_no)	/* can happen */
	    && (players[i].p_team != victim->p_team)
	    )
	    return 1;		/* DOH! */

    /* we made it! */
    return 0;
}

/*----------------------------------DOMOVE-------------------------------*/
/*  This function calls the change direction function then accellerates or
decellerates the ship if it is needed.  The coordinates of the ship are
then adjusted.  */

static void 
domove(struct player *j)	/* the player to move */
{
    int     maxspeed;		/* to hold max speed */
    int     acc;		/* to hold accelleration */
    int     dcc;		/* to hold decelleration */
    int     fcost;		/* to hold fuel cost */
    int     ecost;		/* to hold E-temp cost */
    float   t;			/* temp float */
    int     k;			/* looping var */

    /* warp drive */
    if (j->p_desspeed <= j->p_speed && j->p_speed <= j->p_ship.s_imp.maxspeed)
	j->p_flags &= ~(PFWARP | PFAFTER);

    if (j->p_warptime > 0
	&& configvals->warpprepstyle == WPS_TABORTNOW
	&& being_tractored(j)) {
	j->p_warptime = 0;	/* abort warp prep now */
	j->p_flags &= ~PFWARPPREP;
	god2player("Tractor beam aborted warp prep immediately", j->p_no);
    }

    if (j->p_warptime > 0 ||
	(configvals->warpprep_suspendable && j->p_flags & PFWPSUSPENDED)) {
	if (j->p_speed == j->p_ship.s_warpprepspeed	/* don't speed */
	    && (configvals->cloakduringwarpprep	/* is cloaking legal? */
		|| !(j->p_flags & PFCLOAK))	/* or is it not engaged? */
	    ) {
	    if (configvals->warpprepstyle == WPS_TSUSPEND && being_tractored(j)) {
		/* whoa, warp prep suspended */
	    }
	    else {
		j->p_warptime--;/* countdown to warp powerup. */
		/* put off warping if warpprep suspended  [BDyess] */
		if (j->p_warptime == 0 && (j->p_flags & PFWPSUSPENDED) &&
		    configvals->warpprep_suspendable)
		    j->p_warptime = 1;
		if (0 == j->p_warptime) {
		    int     success = 1;
		    /* warp drives have finished prepping */
		    j->p_flags &= ~PFWARPPREP;
		    if (configvals->warpprepstyle == WPS_TABORT) {
			if (being_tractored(j)) {
			    god2player("Tractor beam aborted warp engagement", j->p_no);
			    success = 0;
			}
		    }
		    else if (configvals->warpprepstyle == WPS_TPREVENT) {
			if (being_tractored(j)) {
			    j->p_warptime++;
			    success = 0;
			}
		    }
		    if (success) {
			j->p_flags |= PFWARP;
			j->p_desspeed = j->p_warpdesspeed;	/* go however fast he
								   asked for originally
								   [BDyess] */
		    }
		}
	    }
	}
	else {
	    j->p_flags |= PFWARPPREP;
	    j->p_desspeed = j->p_ship.s_warpprepspeed;
	    if (!configvals->cloakduringwarpprep)
		j->p_flags &= ~PFCLOAK;
	}
    }

    /**********************************************************************/
    /*
     *  for some reason this would occasionally be zero and cause an FPE in
     *  the code below
     */
    if (!j->p_ship.s_maxdamage)
	return;

    if ((j->p_dir != j->p_desdir) && (j->p_status != PEXPLODE))
	changedir(j);		/* change direction if needed */

    if((j->p_flags & PFWARP) && !j->p_warptime && configvals->tractabortwarp)
	if(being_tractored(j))
	    j->p_flags &= ~PFWARP;

    /* Alter speed */
    if ((j->p_flags & PFWARP) &&
	j->p_warptime == 0) {	/* make sure we are really warping */
	maxspeed = j->p_ship.s_warp.maxspeed;
	if(configvals->warpzone) {
	  if(j->p_zone > 0)
	    maxspeed = maxspeed * 3 / 2;
	}
	/* get damage adjusted max speed */
	maxspeed -= (int) ((float) maxspeed *
		   (float) j->p_damage / (float) j->p_ship.s_maxdamage);
	maxspeed = (maxspeed < 0) ? 0 : maxspeed;	/* no going backward */
	acc = j->p_ship.s_warp.acc;	/* get accelleration for warp */
	dcc = j->p_ship.s_warp.dec;	/* get decelleration for warp */
	fcost = j->p_ship.s_warp.cost;	/* get fuel for warping */
	ecost = j->p_ship.s_warp.etemp;	/* get e-temp for warping */
    }
    else if (j->p_flags & PFAFTER) {	/* if after burners on */
	maxspeed = j->p_ship.s_after.maxspeed -
	    (int) ((float) j->p_ship.s_after.maxspeed *
		   (float) j->p_damage / (float) j->p_ship.s_maxdamage);
	maxspeed = (maxspeed < 0) ? 0 : maxspeed;	/* no going backward */
	acc = j->p_ship.s_after.acc;	/* get acc for afterburners */
	dcc = j->p_ship.s_after.dec;	/* get decel for afterburners */
	fcost = j->p_ship.s_after.cost;	/* fuel used for after */
	ecost = j->p_ship.s_after.etemp;	/* E-temp for after */
    }
    else {			/* if normal speed */
	maxspeed = j->p_ship.s_imp.maxspeed - (int) ((float) j->p_ship.s_imp.maxspeed *
		       (float) j->p_damage / (float) j->p_ship.s_maxdamage);
	maxspeed = (maxspeed < 0) ? 0 : maxspeed;	/* no going backward */
	acc = j->p_ship.s_imp.acc;	/* accelleration for impulse */
	dcc = j->p_ship.s_imp.dec;	/* decelleration for impulse */
	fcost = j->p_ship.s_imp.cost * j->p_speed;	/* fuel used for impulse */
	ecost = j->p_ship.s_imp.etemp * j->p_speed;	/* E-temp for impulse */
    }
    if ((j->p_desspeed > maxspeed)	/* if we are too damage to go max */
    /* && (j->p_damage < 0.9 * j->p_ship.s_maxdamage) */
	)
	j->p_desspeed = maxspeed;	/* then set a new desired speed */
    if ((j->p_flags & PFENG)	/* if E-temped or repairing */
	&&!(j->p_flags & PFREPAIR))
	j->p_desspeed = 0;	/* speed drops to 0 */
    j->p_fuel -= fcost;		/* suck up fuel */
    j->p_subetemp += ecost;	/* heat the engines */
    if (allows_docking(j->p_ship)) {	/* if ships can dock to this */
	for (k = 0; k < j->p_ship.s_numports; k++)	/* go through all ports */
	    if (j->p_port[k] >= 0) {	/* if ship docked there */
		t = (float) players[j->p_port[k]].p_ship.s_mass /
		    (float) j->p_ship.s_mass;
		j->p_fuel -= (int) (t * (float) fcost);
		j->p_subetemp += (int) (t * (float) ecost);
	    }
    }
    while (j->p_subetemp > 1000) {	/* add on the high part of subetemp */
	j->p_etemp += 1;	/* to etemp */
	j->p_subetemp -= 1000;
    }

    /*
     * check acceleration, only if we have fuel.
     * if we need to slow down, do that, but do it if we are
     * out of fuel as well.
     */
    if ( (j->p_desspeed > j->p_speed) && (j->p_fuel > j->p_ship.s_recharge) ) { 
	j->p_subspeed += acc;	/* add on accelleration */
    }
    /*
     * no fuel?  force him to slow down.
     */
    if( j->p_fuel < 0 )
	j->p_desspeed = 1;
    if ( j->p_desspeed < j->p_speed ) {/* if we need to go slower */
	j->p_subspeed -= dcc;	/* decrease our speed */
    }
    if (j->p_subspeed / 1000) {	/* if fractional part big enough */
	j->p_speed += j->p_subspeed / 1000;	/* change integer speed */
	if ((j->p_subspeed < 0 && j->p_speed < j->p_desspeed) ||
	    (j->p_subspeed > 0 && j->p_speed > j->p_desspeed)) {
	    /* went too far, adjust [BDyess] */
	    j->p_speed = j->p_desspeed;
	    j->p_subspeed = 0;
	}
	else {
	    j->p_subspeed %= 1000;	/* adjust the fractional part */
	    if (j->p_speed < 0)	/* can't go below zero speed */
		j->p_speed = 0;
	}

	if ((!configvals->warpdecel) && j->p_speed > maxspeed)
	    /* can't exceed maxspeed */
	    j->p_speed = maxspeed;

    }

    j->p_x += (double) j->p_speed * Cos[j->p_dir] * WARP1;	/* adjust coords */
    j->p_y += (double) j->p_speed * Sin[j->p_dir] * WARP1;
    move_player(j->p_no, j->p_x, j->p_y, 1);
    if( j->p_fuel < 0 )
        j->p_fuel = 0;
}




/*-------------------------------DOTRACTOR-------------------------------*/
/*  This function handles the tractoring and pressoring for a player.
There is a long set of conditions that can turn off tractors.  */

static void 
dotractor(struct player *j)	/* the player to do */
{
    float   cosTheta, sinTheta;	/* Cos and Sin from me to him */
    int     halfforce;		/* Half force of tractor */
    float   dist;		/* for finding distance */
    struct player *victim;

    if (j->p_flags & PFTRACT) {	/* tractor beam on? */
	victim = &players[j->p_tractor];
	if ((isAlive(victim))
	    && ((j->p_fuel > j->p_ship.s_tractcost) && !(j->p_flags & PFENG))
	    && ((dist = ihypot(j->p_x - victim->p_x, j->p_y - victim->p_y))
		< (TRACTDIST) * j->p_ship.s_tractrng)
	    && (!(j->p_flags & (PFORBIT | PFDOCK)))
	     /* && (!(victim->p_flags & PFDOCK)) */ ) {

	    if (victim->p_flags & PFORBIT) {	/* pullplayer out of */
		victim->p_flags &= ~PFORBIT;	/* orbit */
	    }
	    else if (victim->p_flags & PFDOCK) {
		/* ooOooo, damage the base */
		struct player *base = &players[victim->p_docked];
		int     port_num = victim->p_port[0];
		int     flags;

		undock_player(victim);

		/* we need a better reason than KSHIP */
		flags = victim->p_flags;
		victim->p_flags &= ~PFSHIELD;	/* inflict damage on hull */
		inflict_damage(j, (struct player *) 0, victim, DOCKDAMAGE, KSHIP);
		victim->p_flags = flags;

		flags = base->p_flags;
		base->p_flags &= ~PFSHIELD;	/* inflict damage on hull */
		inflict_damage(j, (struct player *) 0, base, DOCKDAMAGE, KSHIP);
		base->p_flags = flags;
		base->p_port[port_num] = PDAMAGE;	/* disable that docking
							   port */
	    }
	    j->p_fuel -= j->p_ship.s_tractcost;	/* take fuel for tractors */
	    j->p_subetemp += j->p_ship.s_tractetemp;	/* heat engines up */
	    cosTheta = victim->p_x - j->p_x;
	    sinTheta = victim->p_y - j->p_y;
	    if (dist == 0)	/* like groos in the dark */
		dist = 1;	/* avoid the divide by zero */
	    cosTheta /= dist;	/* normalize sin and cos */
	    sinTheta /= dist;
	    halfforce = (WARP1 * j->p_ship.s_tractstr);
	    if (j->p_flags & PFPRESS)	/* if pressors being */
		halfforce = -halfforce;	/* used */
	    /* move the players */

	    j->p_x += cosTheta * halfforce / (j->p_ship.s_mass);
	    j->p_y += sinTheta * halfforce / (j->p_ship.s_mass);
	    move_player(j->p_no, j->p_x, j->p_y, 1);
	    victim->p_x -= cosTheta * halfforce / (victim->p_ship.s_mass);
	    victim->p_y -= sinTheta * halfforce / (victim->p_ship.s_mass);
	    move_player(j->p_tractor, victim->p_x, victim->p_y, 1);
	}
	else			/* else if conditions not met */
	    j->p_flags &= ~(PFTRACT | PFPRESS);	/* for tractor turn off */
    }
}

/*-----------------------------------------------------------------------*/








/*------------------------------VISIBLE FUNCTIONS-------------------------*/

/*----------------------------------LOSERSTATS----------------------------*/
/*  This function is called when a player is killed and his losses need to
be incremented.  */

void 
loserstats(int pl)		/* the dead player's number */
{
    struct player *dude;	/* to point to player */

    if (!status->tourn &&
        (configvals->robot_stats && !(players[pl].p_flags & PFROBOT)))
      return;

    dude = &players[pl];	/* get pointer to player's structure */
    if (dude->p_ship.s_type == STARBASE) {	/* if ship was a SB */
	dude->p_stats.st_sblosses++;	/* then inc starbase losses */
	status->sblosses++;	/* inc global stats */
    }
    if (dude->p_ship.s_type == WARBASE) {	/* if ship was a WB */
	dude->p_stats.st_wblosses++;	/* then inc warbase losses */
	status->wblosses++;	/* inc global stats */
    }
    else {			/* else if normal ship */
	dude->p_stats.st_tlosses++;	/* inc t-mode losses */
	status->losses++;	/* inc games t-mode losses */
    }
}




/*--------------------------------KILLERSTATS-----------------------------*/
/*  This function is called when a player kills another player and his
kills need to be increased.  This function will add partial kills if the
victim was carrying armies.  */

void 
killerstats(int pl, struct player *victim)
{
    struct player *dude;	/* to point to killer's player struct */

    if (!status->tourn && 
        (configvals->robot_stats && !(players[pl].p_flags & PFROBOT)))
	return;

    dude = &players[pl];	/* get killer's player struct */
    if (dude->p_ship.s_type == STARBASE) {	/* if player in SB then */
	dude->p_stats.st_sbkills++;	/* inc SB kills */
	status->sbkills++;	/* inc global SB kills */
    }
    else if (dude->p_ship.s_type == WARBASE) {	/* else if in warbase */
	dude->p_stats.st_wbkills++;	/* inc warbase kills */
	status->wbkills++;	/* inc global WB kills */
    }
    else {			/* else in normal ship */
	dude->p_stats.st_tkills++;	/* inc t-mode kills */
	status->kills++;	/* inc global kills */
    }
    dude->p_stats.st_tdooshes += victim->p_armies;
    dude->p_dooshes += victim->p_armies;
    status->dooshes += victim->p_armies;	/* add to global dooshes */
    dude->p_stats.st_di += 0.02 * 5.0 * (float) victim->p_armies;
    if (victim->p_ship.s_type == STARBASE)
	dude->p_stats.st_di += 3.0;
    if (victim->p_ship.s_type == WARBASE)
	dude->p_stats.st_di += 1.5;
    if (victim->p_ship.s_type == PATROL)
	dude->p_stats.st_di += 0.03;
    else
	dude->p_stats.st_di += 0.04;
    /* give robots additional DI for kills, since it's all they ever do ;)*/
    if (dude->p_flags & PFROBOT && configvals->robot_stats)
	dude->p_stats.st_di += 0.05;
}




/*------------------------------CHECKMAXKILLS-------------------------------*/
/* This function checks to see if a player has exceeded his max kills and if
he has, it records the new max kills.  */

void 
checkmaxkills(int pl)		/* # of player to check */
{
    struct stats *stats;	/* to point to player's struct */
    struct player *dude;	/* to point to his stats struct */

    if (!status->tourn && 
        (configvals->robot_stats && !(players[pl].p_flags & PFROBOT)))
	return;
    dude = &(players[pl]);	/* get player's player struct */
    stats = &(dude->p_stats);	/* get player's stat struct */
    if (dude->p_ship.s_type == STARBASE) {	/* if in starbase then */
	if (stats->st_sbmaxkills < dude->p_kills)	/* check max SB kills */
	    stats->st_sbmaxkills = dude->p_kills;	/* set if new max kills */
    }
    else if (dude->p_ship.s_type == WARBASE) {	/* warbase max kills */
	if (stats->st_wbmaxkills < dude->p_kills)	/* check max WB kills */
	    stats->st_wbmaxkills = dude->p_kills;	/* set if new max kills */
    }
    else if (stats->st_tmaxkills < dude->p_kills) {	/* else normal ship */
	stats->st_tmaxkills = dude->p_kills;	/* set if new max kills */
    }
}




/*----------------------------------BEAM----------------------------------*/
/*  This function goes through all the players and if any are beaming up
or down, the beaming is done here.  */

void 
beam(void)
{
    register int i;		/* looping variable */
    register struct player *j;

    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if ((j->p_status != PALIVE) || !(j->p_flags & (PFORBIT | PFDOCK)))
	    continue;
	if (j->p_flags & PFBEAMUP) {	/* if beaming up */
	    beammeupscotty(j);	/* do the beamup */
	}
	else if (j->p_flags & PFBEAMDOWN) {	/* else if beaming down */
	    beamdown(j);	/* do the beam down */
	}
    }
}




/*---------------------------------UDCLOAK--------------------------------*/
/*  This function incs/decs the cloakphase for the players.  */

void 
udcloak(void)
{
    register int i;		/* looping var */

    for (i = 0; i < MAXPLAYER; i++) {	/* go through all players */
	if (isAlive(&players[i]))	/* if player is alive */
	{
	    if ((players[i].p_flags & PFCLOAK)	/* if cloaking */
		&&(players[i].p_cloakphase < (CLOAK_PHASES - 1)))
		players[i].p_cloakphase++;	/* on to next pahse */
	    else if (!(players[i].p_flags & PFCLOAK)	/* if uncloaking */
		     &&(players[i].p_cloakphase > 0))
		players[i].p_cloakphase--;	/* on to next phase */
	 }
    }
}



/*-------------------------------UDPLAYERS---------------------------------*/
/*  This function updates all the players.  It handles most of the functions
dealing with the player's ship.  If there are no players in the game, then
this function will set the dietime variable that the move function in the
daemon will use to shut down the deamon.  */

void 
udplayers(void)
{
    register int i;		/* looping vars */
    register struct player *j;	/* to point to players */
    int     nplayers;		/* number of open slots */

    nplayers = 0;		/* zero the player count */
    for (i = status->active = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	switch (j->p_status) {	/* test the player's status */
	case POUTFIT:		/* player being ghostbusted */
	    if (++(j->p_ghostbuster) > OUTFITTIME) {	/* if time ran out */
		saveplayer(j);	/* then save the player */
		j->p_status = PFREE;	/* free up the player slot */
		move_player(j->p_no, -1, -1, 1);
	    }
	    break;		/* on to next player */
	case PFREE:		/* if slot free then */
	    nplayers++;		/* inc players not here */
	    j->p_ghostbuster = 0;	/* so not to bust new players */
	    break;		/* on to next player */
	case PDEAD:		/* if player dead */
	    if (--j->p_explode <= 0) {	/* dec exp timer, if below zero then */
		saveplayer(j);	/* player is busted, save him */
		j->p_status = POUTFIT;	/* change status to busted */
	    }
	    break;		/* on to next player */
	case PEXPLODE:		/* if player exploding */
	    doshipexplode(j);	/* do the explosion stuff */
	    /* no break, so this will fall through */
	    /* and the explosion will move */
	case PALIVE:		/* the player is alive */
	    if ((j->p_flags & PFORBIT) && !(j->p_flags & PFDOCK))
		doorbit(j);	/* if player orbiting him */
	    else if (!(j->p_flags & PFDOCK))
		domove(j);	/* move player through space */

	    dobounce(j);	/* bounce off of walls */
	    if (j->p_status == PEXPLODE || j->p_status == PDEAD)
		break;		/* player dead or exploding then stop */
	    if (status->tourn && j->p_status == PALIVE)
		switch (j->p_ship.s_type) {
		case STARBASE:
		    j->p_stats.st_sbticks++;	/* inc SB ticks */
		    status->sbtime++;	/* inc global SB time */
		    break;
		case WARBASE:
		    j->p_stats.st_wbticks++;	/* inc WB ticks */
		    status->wbtime++;	/* inc global WB time */
		    break;
		case JUMPSHIP:
		    j->p_stats.st_jsticks++;	/* inc JS ticks */
		    status->jstime++;	/* inc global JS time */
		    break;
		default:
		    j->p_stats.st_tticks++;	/* inc t-mode ticks */
		    status->timeprod++;	/* and global t-mode ticks */
		    break;
		}
	    else if (j->p_flags & PFROBOT && configvals->robot_stats)
		switch (j->p_ship.s_type) {
		case STARBASE:
		    j->p_stats.st_sbticks++;	/* inc SB ticks */
		    break;
		case WARBASE:
		    j->p_stats.st_wbticks++;	/* inc WB ticks */
		    break;
		case JUMPSHIP:
		    j->p_stats.st_jsticks++;	/* inc JS ticks */
		    break;
		default:
		    j->p_stats.st_tticks++;	/* inc t-mode ticks */
		    break;
		}

	    if (j->p_jsdock > 0)/* inc js dock time */
		j->p_jsdock = (++j->p_jsdock < 1200) ? j->p_jsdock : 0;
	    if (++(j->p_ghostbuster) > GHOSTTIME) {	/* if ghost timer */
		cause_kaboom(j);/* ghost bust him--explode */
		ghostmess(j);	/* go do the ghost shit */
		saveplayer(j);	/* save the sorry bastard */
		j->p_whydead = KGHOST;	/* killed by ghostbusters */
		j->p_whodead = i;	/* killed by himself */
	    }
	    status->active += (1 << i);	/* ??????????? */
	    j->p_updates++;	/* inc time alive */
	    doresources(j);	/* go do various ship things */
	    dotractor(j);	/* do the tractors */

	    enforce_dock_position(j);
	    doalert(j);		/* check alert status */
	    break;

	case POBSERVE:
	    if (++(j->p_ghostbuster) > GHOSTTIME) {	/* if ghost timer */
		saveplayer(j);	/* save the sorry bastard */
		j->p_status = PFREE;
		move_player(j->p_no, -1, -1, 1);
	    }
	    if (j->p_flags & PFPLOCK) {	/* watching a player */
		struct player *watched = &players[j->p_playerl];
		if (watched->p_team == j->p_team) {
		    j->p_x = watched->p_x;
		    j->p_y = watched->p_y;
		}
		else {
		    j->p_flags &= ~(PFPLOCK | PFPLLOCK);
		}
	    }
	    else if (j->p_flags & PFPLLOCK) {	/* watching a planet */
		struct planet *watched = &planets[j->p_planet];
		if (watched->pl_owner == j->p_team) {
		    j->p_x = watched->pl_x;
		    j->p_y = watched->pl_y;
		}
		else {
		    j->p_flags &= ~(PFPLOCK | PFPLLOCK);
		}
	    }
	    else {
		j->p_x = -10000;
		j->p_y = -10000;/* out of the action... */
	    }
	    break;

	default:
	    if (++(j->p_ghostbuster) > GHOSTTIME) {	/* if ghost timer */
		ghostmess(j);	/* go do the ghost shit */
		saveplayer(j);	/* save the sorry bastard */
		j->p_status = PFREE;
		move_player(j->p_no, -1, -1, 1);
	    }
	    break;
	}			/* end of switch */
    }				/* end of for loop through players */
    if (nplayers == MAXPLAYER) {/* if no players playing */
	if (dietime == -1)	/* if daemon die timer not running */
	    dietime = ticks + 600 / PLAYERFUSE;	/* set it for one minute */
	if(status2->starttourn)		/* no players, so reset to make it a fresh */
	    status2->newgalaxy = 1;	/*   galaxy */
    }
    else			/* else stop daemon die timer */
	dietime = -1;

    if (status2->league && status->tourn) {
	int     leaguecount[2];	/* used to bench excess players */

	leaguecount[0] = leaguecount[1] = 0;

	/* count up how many live players on each team */
	for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	    if (players[i].p_status == PALIVE) {
		int     which = (j->p_team == idx_to_mask(status2->away.index));
		leaguecount[which]++;
	    }
	}

	if (leaguecount[0] >= configvals->playersperteam ||
	    leaguecount[1] >= configvals->playersperteam) {
	    /*
	       if we're full of live players then all non-live players must
	       sit on the bench
	    */
	    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
		int     which = (j->p_team == idx_to_mask(status2->away.index));
		if (j->p_status != PALIVE && j->p_status != POBSERVE
		    && leaguecount[which] >= configvals->playersperteam) {
		    j->p_observer = 1;
		}
	    }
	}

	leaguecount[0] = leaguecount[1] = 0;

	/* count up how many playing players on each team */
	for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	    if (players[i].p_status != PFREE &&
		players[i].p_status != POBSERVE) {
		int     which = (j->p_team == idx_to_mask(status2->away.index));
		leaguecount[which]++;
	    }
	}

	if (leaguecount[0] < configvals->playersperteam ||
	    leaguecount[1] < configvals->playersperteam) {
	    /*
	       if a team is short, let an observer who doesn't want to
	       observe come in
	    */
	    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
		int     which = (j->p_team == idx_to_mask(status2->away.index));
		if (j->p_status == POBSERVE
		    && leaguecount[which] < configvals->playersperteam
		    && j->p_observer == 0) {
		    j->p_whydead = KPROVIDENCE;
		    j->p_whodead = -1;
		    j->p_status = POUTFIT;
		    leaguecount[which]++;
		}
	    }
	}

	/* check for refits and unexpected deaths */
	scan_for_unexpected_tourny_events();
    }
}

/*------------------------------------------------------------------------*/





/*----------END OF FILE--------*/
