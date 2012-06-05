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

#define PLANETS 1
#define GRID 0			/* for space grid */

#include <math.h>
#include "config.h"

#ifdef LOADABLE_PLGEN
#include <dlfcn.h>
#endif

#include "proto.h"
#include "daemonII.h"
#include "data.h"
#include "shmem.h"

 /* other defines */
#define UCVISIBLE 40000		/* dist for uncloaked visibility */
#define CVISMIN 12000		/* cloakers always visible distance */
#define CVISSPEED 2000		/* cloak dist added per warp point */
#define CVISIBLE 25000		/* dist for cloaked visibility */
#define PLFIREDIST 1500		/* distance planets fire at players */
#define PLVISDIST 5000		/* dist planets sense enemy players */
#define REVOLT 200		/* chance out of 1000 of revolt start */
#define STARTREV 8		/* for revolt timer */
/*-------------------------------------------------------------------------*/


/*-----------------------------EXTERNAL GLOBALS----------------------------*/
/* from daemonII.c */
extern int plfd;
extern int glfd;


/*-----------------------------MODULE VARIABLES----------------------------*/

#ifndef LOADABLE_PLGEN
typedef void (*galaxy_generator)(void);

galaxy_generator generators[] = 
{
  NULL,
  gen_galaxy_1,
  gen_galaxy_2,
  gen_galaxy_3,
  gen_galaxy_4,
  gen_galaxy_5,
  gen_galaxy_6,
  gen_galaxy_7,
  gen_galaxy_8,
  gen_galaxy_9
};

int ngenerators = sizeof(generators) / sizeof(galaxy_generator);
#endif

 /* the list of all possible planet names */
static char *pnames[] =
{
    /* Federation planets */
    "Rigel", "Canopus", "Beta Crucis", "Organia", "Deneb",
    "Ceti Alpha V", "Altair", "Vega", "Alpha Centauri",
    /* Romulan worlds */
    "Eridani", "Aldeberan", "Regulus", "Capella", "Tauri",
    "Draconis", "Sirius", "Indi", "Hydrae",
    /* Klingon worlds */
    "Pleiades V", "Andromeda", "Lalande", "Pollux", "Lyrae",
    "Scorpii", "Mira", "Cygni", "Castor",
    /* Orion worlds */
    "Cassiopia", "El Nath", "Spica", "Procyon", "Polaris",
    "Arcturus", "Ursae Majoris", "Herculis", "Antares",

    /* new worlds */
    "Planet 10", "Bezier", "Sequent", "Ophiuchi", "Lacaille",
    "Luyten", "Pavonis", "Wolf 424", "Ross 882", "Cephei",
    "Kruger", "Groombridge", "Maanen's Star", "Heinlein",
    "Pixel", "Lazarus", "Mycroft", "Asimov", "Varley",
    "Clarke's Star", "Ren", "Stimpy", "Foo", "Jolt Cola",
    "Kelly Bundy", "Tyrell", "Roy", "Deckard", "Vangelis",
    "Orpheus", "Xanth", "Tatooine", "Ludicrous", "Ogg",
    "Scum", "Twink", "Chiapucci", "Bugno", "Hampsten", "Fignon",
    "Paradise", "Azriel", "Gargamel", "Smurf Village",
    "Praxis", "Acherner", "Arrakis", "Caladan", "Giedi Prime",
    "Clue", "Paulina", "Sith", "Salusa", "Ahrain", "Cerranos",
    "Darkurthe", "Dagobah", "Phaze", "Chatsubu", "Lemond",
    "Bronco", "Vulcan", "Eden", "Klein", "Merckx", "Tarot",
    "Mottet", "Roche", "Doorstop", "Shaedron", "Fondriest",

    /* Bob's fave worlds */
    "Wayne's World", "DanjerHaus", "Anvil", /* B-52s */ "Claire",
    "Planet Reebok", "Sony Corp.", "SEGA!", "McWorld",
    "Tokyo", "New York", "D.C.", "Atlanta",	/* places I've never been */
     /* Tony */ "Levin",
    "Planet Woogie", "Nancy", "Wilson",		/* real people */
    "Beavis", "Butthead",
    "Memolo",			/* Matt Memolo was murdered in Miami in July
				   of '93.  He was a really swell guy. "...he
				   would go far out of his way to give you
				   the shirt off his back." - ajc */
    /* names from the T.V. shows */
    "New Berlin",
    /*
       "Bejor", "Cardassia" I'm not including these till I can spell them -
       RF
    */

    /* Mike's fave worlds */
    "Melmac", "Order", "Yosemite", "New Chicago", "Ceptus", "Ork",
    "Levi 501", "Toughskin", "Wonka",

    /* book names */
    "Terminus", "Magrathea", "Trantor", "Synnax", "Coruscant",

    /* Moons, names, etc. */
    "Io ", "Titan", "Europa", "Ganymede", "Charon",
    "Tholia", "Gor", "Kzin", "Aerth",
    "Proxima", "Cellust", "Calamar", "Icarus",
    "New Prague",

    /* How about the solar system? */
    "Mercury", "Venus", "Mars", "Jupiter", "Saturn", "Neptune",
    "Uranus", "Pluto",		/* are Neptune and Uranus in order? */

#if 0
    /* Player's names */
    "Claypigeon", "Maxout", "Fungus", "Lynx",
    "Bubbles", "KnightRaven", "Bolo", "Vladimir",
    "Gajah Mada", "Trippix", "Shrew", "Bob Dobbs", "Wibble",
    "Rogue"
#endif
};

#define MAXNAMES (sizeof(pnames)/sizeof(char *))	/* # of planet names */

#if 0	/* these are never used */
char   *homenames[] =		/* names of the race's home worlds */
{
    " ", "Earth", "Romulus", " ", "Klingus", " ", " ", " ",
    "Orion"
};
#endif


 /*
    This table is used to turn the four bits of resources flags into a three
    bit number.  The shipyard and the repair are ORed together.  All the non
    resource bits need to be masked off with PLRESMASK before this table is
    used
 */
static int restores[16] = {0, 1, 2, 3, 4, 5, 6, 7, 1, 1, 3, 3, 5, 5, 7, 7};


 /*
    This is a matrix that determines the chance a planet has of popping.  The
    chance is expressed as a chance out of 100.  The matrix is accessed with
    the planet's atmosphere type and the resource bits after they have been
    converted to 3 bits with the table above.
 */
static int popchance[4][8] =
/*000   00Z   0f0   0fZ   a00   a0Z   af0   afZ--the resources flags,  Z=r|s*/
{
    {2, 3, 2, 2, 5, 7, 4, 8},	/* poison */
    {3, 5, 2, 4, 9, 11, 7, 12},	/* atmos #3 */
    {5, 7, 4, 6, 12, 14, 10, 15},	/* atmos #2 */
    {8, 12, 7, 10, 20, 24, 18, 23}	/* atmos #1 */
};


 /*
    This is a matrix that determines the multiplier for popping. When popping
    the armies on a planet will be multiplied by this multiplier to get the
    number of extra armies that grow.  A negative number indicates negative
    growth.
 */
static float popmult[4][8] =
/*000      00Z    0f0    0fZ    a00    a0Z    af0   afZ  */
{
    {-0.08, 0.00, -0.10, -0.03, 0.00, 0.05, -0.05, 0.05},	/* poison */
    {0.03, 0.05, 0.02, 0.04, 0.06, 0.07, 0.06, 0.07},	/* atmos #3 */
    {0.05, 0.07, 0.05, 0.06, 0.10, 0.11, 0.08, 0.10},	/* atmos #2 */
    {0.09, 0.12, 0.08, 0.10, 0.17, 0.19, 0.15, 0.18}	/* atmos #1 */
};


 /*
    This is a matrix that determines the maximum army capacity of a planet.
    Once this capacity is reached, no other armies can grow there.
 */
static float popcap[4][8] =
/*000   00Z   0f0   0fZ   a00   a0Z   af0    afZ--the resources flags, Z=r|s*/
{
    {5, 10, 7, 10, 15, 15, 10, 18},	/* poison */
    {9, 14, 11, 14, 20, 20, 14, 22},	/* atmos #3 */
    {12, 18, 15, 18, 25, 25, 19, 27},	/* atmos #2 */
    {15, 22, 18, 25, 40, 35, 27, 40}	/* atmos #1 */
};

/*-------------------------------------------------------------------------*/

/*------------------------------INTERNAL FUNCTIONS-------------------------*/

/* used by the qsort() in sortnames() below */
static int
comp_pl_name(a, b)
    void *a, *b;
{
    return strcasecmp(((struct planet *)a)->pl_name,
			((struct planet *)b)->pl_name);
}

/*--------------------------------SORTNAMES---------------------------------*/
/*  This function sorts the planet into a alphabeticly increasing list.  It
operates on the global planets structure.  */

static void 
sortnames(void)
{
    int     i;

    qsort(planets, NUMPLANETS, sizeof(struct planet), comp_pl_name);
    for (i = 0; i < NUMPLANETS; i++)	        /* go through the planets */
	planets[i].pl_no = i;			/* and fix their pl_no value */
}


/*--------------------------------INITPLANETS-------------------------------*/
/*  This function generates the names and initializes the fields in the
planets structure.  The planet names need to be sorted afterthe planets
have been placed.  */

void 
initplanets(void)
{
    int     i, j;		/* looping vars */
    int     nused[MAXNAMES];	/* to mark which names are used */

    for (i = 0; i < MAXNAMES; i++)	/* go through all possible names */
	nused[i] = 0;		/* mark name is not used yet */
    for (i = 0; i < NUMPLANETS; i++) {
	planets[i].pl_no = i;	/* set planet number */
	planets[i].pl_flags = PLPARADISE;	/* Paradise planet */
	planets[i].pl_owner = NOBODY;	/* no owner yet */
	planets[i].pl_x = 0;	/* just to intialize x and y */
	planets[i].pl_y = 0;
	j = lrand48() % MAXNAMES;	/* get a random name */
	do {			/* do until unused name found */
	    j = (j + 1) % MAXNAMES;	/* go on to next name in list */
	} while (nused[j]);	/* until unused name found */
	nused[j] = 1;		/* mark name as used */
	strcpy(planets[i].pl_name, pnames[j]);	/* copy into planet struct */
	planets[i].pl_namelen = strlen(pnames[j]);	/* set name's length */
	planets[i].pl_hostile = 0;	/* planet doesn't hate anyone yet */
	planets[i].pl_tshiprepair = 0;	/* zero the repair growth timer */
	planets[i].pl_tagri = 0;/* zero the agri growth timer */
	planets[i].pl_tfuel = 0;/* zero the fuel growth timer */
	planets[i].pl_armies = 0;	/* no armies yet */
	planets[i].pl_warning = 0;	/* no warning being counted down for */
	planets[i].pl_system = 0;	/* not in a system yet */
	planets[i].pl_hinfo = NOBODY;	/* no race has info on planet */
	for (j = 0; j < MAXTEAM + 1; j++) {	/* go through four races */
	    planets[i].pl_tinfo[j].owner = NOBODY;	/* do not know who owns
							   it */
	    planets[i].pl_tinfo[j].armies = 0;	/* no armies on planet */
	    planets[i].pl_tinfo[j].flags = PLPARADISE;	/* know nothing about
							   flags */
	    planets[i].pl_tinfo[j].timestamp = 0;	/* set the timestamp */
	}
	planets[i].pl_trevolt = 0;	/* revolt timer not counting down */
#if GRID
	planets[i].pl_next = NULL;	/* set fields related to space grid */
	planets[i].pl_previous = NULL;
	planets[i].pl_gridnum = 0;
#endif
    }
}




/*-------------------------------GROWPLANETS------------------------------*/
/*  This function grows resources on planets.  It goes through all planets
and updates the growth timers and checks for growth.  Independent planets
do not grow.  This function also checks to see if any player in a starbase
is orbiting a planet and adjusts the planet's growth rate if so.  */

void 
growplanets(void)
{
    int     i;			/* looping var */
    struct planet *p;		/* to point within planets */
    int     add;		/* number to add to timers */
    char   *sb_op;		/* keep track of orbiting SBs */
    char   sb_orbits[NUMPLANETS];

    if (!status->tourn)
	return;

    if (!configvals->resource_bombing)
	return;

    memset(sb_orbits, 0, NUMPLANETS);

    for (i = 0; i < MAXPLAYER; i++) {	/* go through all players */
	struct player *py = &players[i];	/* and look for orbiting */
	if ((py->p_status != PALIVE) ||	/* starbases */
	    (py->p_ship.s_type != STARBASE) ||
	    !(py->p_flags & PFORBIT))
	    continue;
	p = &planets[py->p_planet];	/* found one, get planet */
	p->pl_tfuel += 20;	/* give growth rate a boost */
	p->pl_tagri += 30;	/* NOTE: change these if PLG consts */
	p->pl_tshiprepair += 50;/* change */
	sb_orbits[py->p_planet] = 1;
    }
    p = &planets[0];		/* start with first planet */
    sb_op = &sb_orbits[0];
    for (i = 0; i < NUMPLANETS; i++, p++, sb_op++) {  /* through all planets */
	if (p->pl_owner == NOBODY)	/* if independent then */
	    continue;		/* no growth */
	add = p->pl_armies / 2;	/* rate based on armies */
	p->pl_tfuel += add;	/* add to fuel timer */
	p->pl_tfuel = (p->pl_tfuel > PLGFUEL) ? PLGFUEL : p->pl_tfuel;
	if ((!(p->pl_flags & PLFUEL))	/* if no fuel */
	    &&(p->pl_flags & PLDILYTH)	/* and dilythium deposits */
	    &&(p->pl_tfuel >= PLGFUEL)) {	/* and timer high enough */
	    p->pl_flags |= (PLFUEL | PLREDRAW);	/* create fuel depot */
	    p->pl_tinfo[p->pl_owner].flags = p->pl_flags;
	    p->pl_tinfo[p->pl_owner].timestamp = status->clock;
	    tlog_res(p, "FUEL");
	}
	p->pl_tagri += add;	/* add to agri timer */
	p->pl_tagri = (p->pl_tagri > PLGAGRI) ? PLGAGRI : p->pl_tagri;
	if ((!(p->pl_flags & PLAGRI))	/* if no agri on planet */
	    &&(p->pl_flags & PLARABLE)	/* and arable */
	    &&(p->pl_tagri >= PLGAGRI)) {	/* and timer high enough */
	    p->pl_flags |= (PLAGRI | PLREDRAW);	/* create agri planet */
	    p->pl_tinfo[p->pl_owner].flags = p->pl_flags;
	    p->pl_tinfo[p->pl_owner].timestamp = status->clock;
	    tlog_res(p, "AGRI");
	}

        /* add to SY timer only on configval set and sb orbiting,
	   otherwise clamp at last value, or RPR if auto-grown */
        if(configvals->shipyard_built_by_sb_only && !(*sb_op) &&
	   p->pl_tshiprepair + add >= PLGREPAIR)
	{
	  /* transition from just below PLGREPAIR -> PLGREPAIR */
	  if(p->pl_tshiprepair < PLGREPAIR &&
	     p->pl_tshiprepair + add >= PLGREPAIR)
	    p->pl_tshiprepair = PLGREPAIR;
	}
	else
	  p->pl_tshiprepair += add;

	if ((!(p->pl_flags & PLREPAIR))	/* if not repair */
	    &&(p->pl_flags & PLMETAL)	/* and metal deposits */
	    &&(p->pl_tshiprepair >= PLGREPAIR)) {	/* and timer high enough */
	    p->pl_flags |= (PLREPAIR | PLREDRAW);	/* create repair station */
	    p->pl_tinfo[p->pl_owner].flags = p->pl_flags;
	    p->pl_tinfo[p->pl_owner].timestamp = status->clock;
	    tlog_res(p, "REPAIR");
	}
	p->pl_tshiprepair = (p->pl_tshiprepair > PLGSHIP) ? PLGSHIP :
	    p->pl_tshiprepair;	/* clamp value to max */
	if ((!(p->pl_flags & PLSHIPYARD))	/* if not repair */
	    &&(p->pl_flags & PLMETAL)	/* and metal deposits */
	    &&(p->pl_tshiprepair >= PLGSHIP)) {	/* and timer high enough */

	    p->pl_flags |= (PLSHIPYARD | PLREDRAW);	/* create repair station */
	    p->pl_tinfo[p->pl_owner].flags = p->pl_flags;
	    p->pl_tinfo[p->pl_owner].timestamp = status->clock;
	    tlog_res(p, "SHIPYARD");
	}
    }
}



/*-------------------------------UDPLANETS----------------------------------*/

static void 
fill_planets(int *plist /* array */, int *count /* scalar */, int owner)
{
    int     i;
    *count = 0;
    for (i = 0; i < configvals->numplanets; i++) {
	if (owner<0 || planets[i].pl_owner == owner)
	    plist[(*count)++] = i;
    }

    for (i = *count - 1; i > 0; i--) {
	int     idx = lrand48() % (i + 1);
	int     temp = plist[idx];
	plist[idx] = plist[i];
	plist[i] = temp;
    }
}


/*----------------------------------PVISIBLE------------------------------*/
/*  This function goes through the players and checks the other playes to see
if an enemy is close enough to see him.  THIS FUNCTION SHOULD EVENTUALLY USE
THE SPACE GRID.  */

static void 
pvisible(void)
{
    struct player *p;		/* to point to a player */
    int     i;			/* looping var */
    int     h;			/* looping var */
    struct player *pl2;		/* to point to other players */
    int     dx, dy;		/* delta coords */
    int     dist;		/* to hold distance */

    for (i = 0, p = &players[0]; i < MAXPLAYER; i++, p++)
	p->p_flags &= ~PFSEEN;	/* clear all players' the seen flags */
    for (i = 0, p = &players[i]; i < MAXPLAYER - 1; i++, p++) {
	if (p->p_status != PFREE &&	/* only do if player alive */
	    p->p_status != POBSERVE) {	/* observers can't augment team
					   scanning */
	    for (h = i + 1, pl2 = &players[h]; h < MAXPLAYER; h++, pl2++) {
		if ((pl2->p_status == PFREE) || (pl2->p_status == POBSERVE)
		    || (pl2->p_flags & PFROBOT)	/* if not alive or robot or */
		    ||(pl2->p_team == p->p_team))	/* same team then
							   continue */
		    continue;
		dx = ABS(pl2->p_x - p->p_x);	/* calc delta coords */
		dy = ABS(pl2->p_y - p->p_y);
		if ((dx > UCVISIBLE) || (dy > UCVISIBLE))	/* if obviously too far
								   away */
		    continue;	/* then don't bother further */
		dist = ihypot(dx, dy);
		if (dist > UCVISIBLE)	/* out of range */
		    continue;	/* on to next ship */
		if ((dist < CVISMIN + CVISSPEED * pl2->p_speed) ||
		    (!(pl2->p_flags & PFCLOAK)))
		    pl2->p_flags |= PFSEEN;	/* if close then visible */
		if ((dist < CVISMIN + CVISSPEED * p->p_speed) ||
		    (!(p->p_flags & PFCLOAK)))
		    p->p_flags |= PFSEEN;	/* second player */
	    }
	}
    }
}



static void 
blast_resource(struct player *p, struct planet *l, int res, double dival)
{
    if (status->tourn) {
	p->p_stats.st_di += dival;
	p->p_stats.st_tresbomb++;
	p->p_resbomb++;
	status->resbomb++;
    }
    l->pl_flags &= ~res;
    l->pl_tinfo[l->pl_owner].flags = l->pl_flags;
    l->pl_tinfo[l->pl_owner].timestamp = status->clock;
    l->pl_tinfo[p->p_team].flags = l->pl_flags;
    l->pl_tinfo[p->p_team].timestamp = status->clock;
    l->pl_flags |= PLREDRAW;	/* slate for redraw */
}

/*-----------------------------------PBOMB---------------------------------*/
/*  This function goes through the players and does the bombing if any
player is bombing.  This will knock resources off of planets.  If a player
is bombing a planet that another team owns, then that team will see the
player by having the PFSEEN flag set.  */

static void 
pbomb(void)
{
    struct player *p;		/* to point to a player */
    int     i;			/* looping var */
    struct planet *l;		/* to point to planet being bombed */
    char    buf[90];		/* to sprintf messages into */
    char    buf1[80];
    int     rnd;		/* to hold armies to knock off */

    for (i = 0, p = &players[0]; i < MAXPLAYER; i++, p++) {	/* go through playrs */
	if ((p->p_status == PALIVE)	/* only do if player is alive */
	    &&(p->p_flags & PFORBIT)	/* and he is orbiting */
	    &&(p->p_flags & PFBOMB)) {	/* and he is bombing */

	    l = &planets[p->p_planet];	/* get planet being bombed */

	    /* if he's non-hostile, quit bombing */
	    if ((!((p->p_swar | p->p_hostile) & l->pl_owner))
		&& (l->pl_owner != NOBODY)) {
		p->p_flags &= ~PFBOMB;
		continue;
	    }

	    if ((l->pl_warning <= 0) && (l->pl_owner != NOBODY) &&
	        ((CAN_BOMB(p,ARMIES) && (l->pl_armies > 4)) ||
		 (CAN_BOMB(p,FUEL) && (l->pl_flags & PLFUEL)) ||
		 (CAN_BOMB(p,REPAIR) && (l->pl_flags & PLREPAIR)) ||
		 (CAN_BOMB(p,SHIPYARD) && (l->pl_flags & PLSHIPYARD)) ||
		 (CAN_BOMB(p,AGRI) && (l->pl_flags & PLAGRI))))
            {
		l->pl_warning = 50 / PLFIGHTFUSE;	/* reset warning timer */
		sprintf(buf, "We are being attacked by %s %s who is %d%% damaged.",
			p->p_name, twoletters(p),
			(100 * p->p_damage) / (p->p_ship.s_maxdamage));
		sprintf(buf1, "%-3s->%-3s", l->pl_name, teams[l->pl_owner].shortname);
		pmessage(buf, l->pl_owner, MTEAM | MBOMB, buf1);	/* send message */

		/* CRD feature: shipyard guardians - MAK,  2-Jun-93 */
		if ((l->pl_flags & PLSHIPYARD) && (!status->tourn)) {
		    rescue(l->pl_owner, 0, l->pl_no);
		}
	    }

	    p->p_swar |= l->pl_owner;	/* set player at war w/ owner */

	    if (CAN_BOMB(p, ARMIES))
	    {
	      rnd = (lrand48() % 50) + p->p_ship.s_bomb;/* pick random number */
	      rnd = (int) ((float) rnd / 33.0 + 0.5);	/* calc armies bombed */
	      if (rnd > 0 && l->pl_armies > 4)	/* can't bomb negative armies */
	      {
		    l->pl_armies -= rnd;	/* kill off armies */
		    tlog_bomb(l, p, rnd);
		    l->pl_armies = (l->pl_armies < 1) ? 1 : l->pl_armies;
		    l->pl_tinfo[l->pl_owner].armies = l->pl_armies;
		    l->pl_tinfo[l->pl_owner].timestamp = status->clock;
		    l->pl_tinfo[p->p_team].armies = l->pl_armies;
		    l->pl_tinfo[p->p_team].timestamp = status->clock;
		    if (l->pl_armies < 5)  /* if planet needs to be redrawn */
			l->pl_flags |= PLREDRAW; /* schedule planet for redrw */
		    if (l->pl_owner != NOBODY) {
			credit_armiesbombed(p, rnd, l);
		    }
	      }
	      if (l->pl_armies > 4)	/* no resources bomb if > 4 armies */
		  continue;	/* on planet or in bronco-mode */
	    }

            if(!configvals->resource_bombing)
	      continue;
            
	    if (CAN_BOMB(p, FUEL))
	    {
	      if(configvals->slow_bomb)
		l->pl_tfuel -= rnd * 5;	/* knock fuel timer down */
	      else
		l->pl_tfuel -= rnd * 8;	/* knock fuel timer down */

	      l->pl_tfuel = (l->pl_tfuel < 0) ? 0 : l->pl_tfuel;
	      if ((l->pl_tfuel == 0) && (l->pl_flags & PLFUEL)) 
	      {
		  blast_resource(p, l, PLFUEL, 0.10);
		  tlog_bres(l, p, "FUEL");
	      }
	    }
	    
            if (CAN_BOMB(p, AGRI))
	    {
	      if(configvals->slow_bomb)
		l->pl_tagri -= rnd * 4;	/* attack the agri timer */
	      else
		l->pl_tagri -= rnd * 6;	/* attack the agri timer */

	      l->pl_tagri = (l->pl_tagri < 0) ? 0 : l->pl_tagri;
	      if ((l->pl_tagri == 0) && (l->pl_flags & PLAGRI)) 
	      {
		  blast_resource(p, l, PLAGRI, 0.25);
		  tlog_bres(l, p, "AGRI");
	      }
	    }

	    if ((CAN_BOMB(p, SHIPYARD) && (l->pl_flags & PLSHIPYARD)) ||
	       (CAN_BOMB(p, REPAIR) && !(l->pl_flags & PLSHIPYARD)) ||
	       (CAN_BOMB(p, REPAIR) && CAN_BOMB(p, SHIPYARD)))
	    {
	      if(configvals->slow_bomb)
		l->pl_tshiprepair -= rnd * 5;	/* knock ship/repr down */
	      else
		l->pl_tshiprepair -= rnd * 8;	/* knock ship/repr down */

	      l->pl_tshiprepair = (l->pl_tshiprepair < 0) ? 0 :
		  l->pl_tshiprepair;
	      if ((l->pl_tshiprepair < PLGREPAIR) && (l->pl_flags & PLSHIPYARD))
	      {
		  blast_resource(p, l, PLSHIPYARD, 0.10);
		  tlog_bres(l, p, "SHIPYARD");
	      }
	      if ((l->pl_tshiprepair == 0) && (l->pl_flags & PLREPAIR)) 
	      {
		  blast_resource(p, l, PLREPAIR, 0.20);
		  tlog_bres(l, p, "REPAIR");
	      }
	    }
	}
    }
}




/*---------------------------------PFIRE-----------------------------------*/
/*  This function goes through the planets and sees if any enemy players are
close enough to be fired upon by the planet.  It also checks to see if
players are close enough to the planet so that the planet should be redrawn.
Enemy planets will 'see' players that are very close to them.  THIS FUNCTION
SHOULD EVENTUALLY USE THE SPACE GRID.  */

static void 
pfire(void)
{
    struct player *p;		/* to point to a player */
    int     i, j;		/* looping vars */
    struct planet *l;		/* to point to the planets */
    int     dx, dy;		/* to hold delta coords */
    int     dist;		/* to hold distance */
    int     dam;		/* to hold damage */
    int     boost;		/* for warp zones, total boost [BDyess] */

    for (j = 0, p = &players[0]; j < MAXPLAYER; j++, p++) {
        if(p->p_status != PALIVE) continue;
	boost = 0;		/* default no bonus or penalty [BDyess] */
	for (i = 0, l = &planets[0]; i < NUMPLANETS; i++, l++) {
	    if ((p->p_team == l->pl_owner) && (PL_TYPE(*l) != PLSTAR)
                 && !configvals->warpzone)
		continue;	/* no, then continue */
	    dx = ABS(l->pl_x - p->p_x);	/* calc delta coorda between */
	    dy = ABS(l->pl_y - p->p_y);	/* planet and player */
	    dist = ihypot(dx, dy);

            if (dist < configvals->warpzone) {  
	      /* within warp boost/block range [BDyess] */
	      if (p->p_team == l->pl_owner)
		boost += configvals->warpzone - dist;
	      else if (l->pl_owner != NOBODY  &&
	              ((p->p_swar | p->p_hostile) & l->pl_owner))
	        boost -= configvals->warpzone - dist;
	    }
	        
	    if (dx < 6 * PLFIREDIST && dy < 6 * PLFIREDIST) /* redraw planet */
		l->pl_flags |= PLREDRAW;	  /* if player is very close */

	    if (dist <= p->p_ship.s_scanrange) {	/* check for scanners */
		scout_planet(p->p_no, l->pl_no);
	    }

	    if (dist > PLVISDIST)
		continue;
	    if ((dist < PLVISDIST) && (l->pl_owner != NOBODY)	/* enemy planet */
		&&((p->p_swar | p->p_hostile) & l->pl_owner))	/* see players */
		p->p_flags |= PFSEEN;	/* let team see enemy */
	    if ((dist > PFIREDIST) ||
		((dist > PFIREDIST/2) &&
		 (PL_TYPE(*l) == PLWHOLE))) /* if not within range */
		continue;	/* go on to next playert */
	    if (((p->p_swar | p->p_hostile) & l->pl_owner)
		|| ((l->pl_owner == NOBODY)
		    && (l->pl_hostile & p->p_team)
		    && (l->pl_armies != 0))
		|| (PL_TYPE(*l) == PLWHOLE) || (PL_TYPE(*l) == PLSTAR)) {
		dam = l->pl_armies / 7 + 2;	/* calc the damage */
		if (PL_TYPE(*l) == PLWHOLE) { /* if a wormhole... */
		   /* ...place outside the new wormhole's radius. */
		   if ((wh_effect[SS_WARP] && !(p->p_flags & PFWARP))
			|| (!wh_effect[SS_WARP])) {
		      p->p_x = l->pl_armies + 
			 (((PLFIREDIST/2)+50)*Cos[p->p_dir]);
		      p->p_y = l->pl_radius + 
			 (((PLFIREDIST/2)+50)*Sin[p->p_dir]);
		   }
		   if ((p->p_speed > (p->p_ship.s_imp.maxspeed/2)) &&
		       (wh_effect[SS_IMPULSE])) {
		      dam = ((p->p_ship.s_mass / 100) * (p->p_speed - 
							 p->p_ship.s_imp.maxspeed/2));
		   } else dam = 0;
		   
		} else
		   if (PL_TYPE(*l) == PLSTAR)	/* if planet is a star */
		      dam = 150;	/* do massive damage */
		/*
		   this needs to be reworked and most of it jammed into
		   inflict_damage
		*/
		p->p_whodead = i;	/* which planet is shooting */
		if (dam > 0 && inflict_damage(0, 0, p, dam, KPLANET)) {
		    struct player *killer = 0;
		    p->p_whydead = KPLANET;	/* set killed by a planet */
		    if (PL_TYPE(*l) == PLSTAR) {	/* killed by star? */
			int     pln;
			for (pln = 0, killer = &players[0];
			     pln < MAXPLAYER;
			     pln++, killer++) {
			    if (killer->p_status != PALIVE)
				continue;
			    if (!friendly(killer, p) && killer->p_tractor == p->p_no
				&& killer->p_flags & (PFTRACT | PFPRESS)) {

				killer->p_kills += 1 + (p->p_armies + p->p_kills) / 10;
				killerstats(killer->p_no, p);
				checkmaxkills(killer->p_no);
				break;
			    }
			}
			if (pln >= MAXPLAYER)
			    killer = 0;
		    }
		    killmess(p, killer);
		    tlog_plankill(p, l, killer);
		}
	    }
	}
    /* found the boost for this player, make adjustments if in warp [BDyess] */
        if (configvals->warpzone && p->p_flags & PFWARP) {
	    if(boost == 0 && p->p_zone > 0) { /* did get warp bonus, lost it */
	        if(p->p_desspeed > p->p_ship.s_warp.maxspeed) 
	            p->p_desspeed = p->p_ship.s_warp.maxspeed;
	    } else if(boost < 0 && p->p_zone >= 0) { /* warp no longer allowed*/
		p->p_desspeed = p->p_ship.s_imp.maxspeed;
		if (!configvals->warpdecel)
		    p->p_flags &= ~PFWARP;
	    }
	}
        p->p_zone = boost;
    }
}




/*---------------------------------REVOLT---------------------------------*/
/*  This function does the revolt of a planet.  It updates the revolt
timer and prints the messages that warn the team.  */

static void 
revolt(struct planet *l)	/* the planet to check */
{
    if (!configvals->revolts)
	return;

    if(!configvals->revolt_with_facilities && 
       l->pl_flags & (PLORESMASK | PLRESMASK))
        return;

    if (l->pl_trevolt > 0) {	/* revolt timer running? */
	char    buf[80];	/* to sprintf into */

	{
	    int	i;
	    for (i=0; i<MAXPLAYER; i++) {
		if (players[i].p_status==PALIVE
		    && (players[i].p_flags&(PFORBIT|PFDOCK)) == PFORBIT
		    && players[i].p_planet==l->pl_no
		    && players[i].p_team==l->pl_owner)
		    return;	/* orbiting ship delays revolt */
	    }
	}

	l->pl_trevolt--;	/* dec the timer */
	if ((l->pl_trevolt == 0) && (l->pl_armies > 2)) {
	    l->pl_trevolt = 0;	/* turn off revolt timer */
	    sprintf(buf, "The revolution on %s has been put down.",
		    l->pl_name);/* get message to display */
	    pmessage(buf, l->pl_owner, MTEAM | MCONQ, "PREFECT->");
	}
	else if ((l->pl_trevolt == 0) && (l->pl_armies == 2)) {
	    l->pl_trevolt = STARTREV;	/* more time til last army */
	    l->pl_armies--;	/* kill one army */
	    l->pl_tinfo[l->pl_owner].armies = l->pl_armies;
	    l->pl_tinfo[l->pl_owner].timestamp = status->clock;
	    sprintf(buf, "We cannot hold out much longer on %s",
		    l->pl_name);/* get message to display */
	    pmessage(buf, l->pl_owner, MTEAM | MCONQ, "PREFECT->");
	}
	else if (l->pl_trevolt == 0) {
	    l->pl_trevolt = 0;	/* revolution succeeded */
	    sprintf(buf, "The planet %s has been lost to revolutionaries",
		    l->pl_name);/* get message to display */
	    pmessage(buf, l->pl_owner, MTEAM | MCONQ, "PREFECT->");

	    tlog_revolt(l);

	    l->pl_tinfo[l->pl_owner].timestamp = status->clock;
	    l->pl_tinfo[l->pl_owner].owner = NOBODY;
	    l->pl_owner = NOBODY;
	    l->pl_flags |= PLREDRAW;	/* slate for redraw */
	    checkwin(enemy_admiral(-1));	/* check for game end */

	}
    }
    else {			/* no revolt timer--check for start */
	if ((l->pl_armies <= 2) && (!(l->pl_armies == 0))
	    && ((lrand48() % 1000) <= REVOLT)) {
	    char    buf[80];	/* to sprintf message into */

	    sprintf(buf, "There is civil unrest on %s", l->pl_name);
	    pmessage(buf, l->pl_owner, MTEAM | MCONQ, "PREFECT>>");
	    l->pl_trevolt = STARTREV;	/* time before revolution */
	}
    }
}

void
check_revolt(void)
{
    static int planetlist[MAXPLANETS];
    static int count = 0;
    int	idx;
    struct planet	*l;

    if(!status->tourn)
	return;

    if (count<1)
	fill_planets(planetlist, &count, -1);

    idx = planetlist[--count];
    l = &planets[idx];

    if (l->pl_armies>0 && l->pl_owner!=NOBODY)
	revolt(l);
}

/*-------------------------------------------------------------------------*/

static void 
build_stars_array(void)
{
    int     i;
    struct planet *pl;

    for (i = 0; i < NUMPLANETS; i++) {
	pl = &planets[i];
	if (PL_TYPE(*pl) == PLSTAR)
	    stars[pl->pl_system] = i;
    }
}

static void 
pl_neworbit(void)
/* rearranges a pre-existing galaxy setup such that planets orbit at more "unique"
   distances from their star.  Asteroids look and probably play bad when all the
   planets are approx. the same dist from the star, so I'd suggest turning this
   on if you plan to have asteroids or orbiting planets on your server.  I'd
   suggest turning it on even if you don't. :)  -MDM */
{
   register int i, j;
   /* register struct planet *p; */
   register int numStars = 0, planetsThisStar = 0;
   int * planetList;

   planetList = (int *) malloc(NUMPLANETS * sizeof(int));

   /* find the number of stars */
   for (i=0; i < MAXPLANETS; i++)
      if (PL_TYPE(planets[i]) == PLSTAR) numStars++;

   /* for each star, find the number of planets, and the average distance */

   for (i=1; i <= numStars; i++) {
      planetsThisStar = 0;
      for (j=0; j < MAXPLANETS; j++)
	 if (planets[j].pl_system == i)
	    planetList[planetsThisStar++] = j;
      
      /* now move the planets such that each planet we came across gets its
	 previous distance from it's star, times (it's position in the 
	 planetList array)/(planetsThisStar/1.6).  also make sure that
         new radius isin't so close to star ship can't orbit it.  Heh.  
         Separate the men from the boyz, eh?  */

      for (j=0; j < planetsThisStar; j++) {
	 planets[planetList[j]].pl_radius =  
	    (int)((float)planets[planetList[j]].pl_radius *
		  ((float)(j+1)/(float)(planetsThisStar/1.6))) + 1500;
	 planets[planetList[j]].pl_x = 
	    planets[stars[i]].pl_x + 
	       (int)((float)cos(planets[planetList[j]].pl_angle) *
		     (float)planets[planetList[j]].pl_radius);
	 planets[planetList[j]].pl_y = 
	    planets[stars[i]].pl_y + 
	       (int)((float)sin(planets[planetList[j]].pl_angle) * 
		     (float)planets[planetList[j]].pl_radius);
      }
   }
   free(planetList);
}

/*------------------------------VISIBLE FUNCTIONS-------------------------*/

/*-------------------------------GEN_PLANETS-------------------------------*/
/*  This function generates a number of random planets.  The planets are
arranged in systems and are also placed so that they are not too close to
any other planets.  The races are then given home planets. */

void 
gen_planets(void)
{
    int     i;
    struct planet *pl;
#ifdef LOADABLE_PLGEN
    char *so_path = NULL, so_path_build[256];
    void *dllib = NULL;
    void (*gen_galaxy)(void) = NULL;
    int (*galaxy_type)(void) = NULL;
#endif

    status->clock = 0;		/* reset the timestamp clock */

#ifndef LOADABLE_PLGEN
    if(configvals->galaxygenerator >= 1 && 
       configvals->galaxygenerator <= ngenerators &&
       generators[configvals->galaxygenerator])
      (*generators[configvals->galaxygenerator])();
    else
      gen_galaxy_1();
#else
    /* build_path() */
    /* PLGEN_PATH */
    sprintf(so_path_build, "%s%s", PLGEN_PATH, configvals->galaxygenerator);
    so_path = build_path(so_path_build);
    dllib = dlopen(so_path, RTLD_LAZY);
    if(!dllib)
    {
      fprintf(stderr, "Daemon cannot open galaxy generator %s: %s\n",
              so_path, dlerror());
      exit(1);
    }
    gen_galaxy = dlfcn(dllib, "gen_galaxy");
    if(!gen_galaxy)
    {
      fprintf(stderr, "Daemon cannot link gen_galaxy() from %s: %s\n",
              so_path, dlerror());
      exit(1);
    }
    galaxy_type = dlfcn(dllib, "galaxy_type");
    (*gen_galaxy)();
#endif

#ifndef LOADABLE_PLGEN
    if (configvals->galaxygenerator != 4)
#else
    /* the commented part allows future expansion - e.g. if structures
       change & we bump the version number in here old versions of the
       galaxy generator can be ruled out.  Right now I'm just appending
       a simple galaxy_type function to the end of each Paradise galaxy
       generator & testing for its existence in the .so; the Bronco
       generator (#4) won't have this function). */
    if (galaxy_type /* && galaxy_type() == 3 */)
#endif
    {
	/* gotta do this before they are sorted */
	for (i = 0; i < NUMPLANETS; i++) {
	    pl = &planets[i];
	    if (PL_TYPE(*pl) == PLSTAR || 
		((pl->pl_system == 0) && (PL_TYPE(*pl) != PLWHOLE))) {
		pl->pl_radius = 0;
		pl->pl_angle = 0;
	    }
	    else if (PL_TYPE(*pl) != PLWHOLE) {
		int  dx, dy;
		dx = pl->pl_x - planets[pl->pl_system - 1].pl_x;
		dy = pl->pl_y - planets[pl->pl_system - 1].pl_y;
		pl->pl_radius = ihypot(dx, dy);
		pl->pl_angle = atan2((double)dy, (double)dx);
	    }
	}
	sortnames();		/* sort the names of planets */
	build_stars_array();
	if (configvals->neworbits) pl_neworbit();
	generate_terrain();
    }

#ifdef LOADABLE_PLGEN
    dlclose(dllib);
#endif
}

void 
moveplanets(void)
{
    register int i;		/* for looping */
    register struct planet *l;	/* to point to individual plaent */

    if (stars[1] < 0)
	build_stars_array();

    /* totally experimental planet moving stuff */
    for (i = 0; i < NUMPLANETS; i++) {
	int     x, y;
	double  r;
	l = &planets[i];
	if (PL_TYPE(*l) == PLSTAR || l->pl_system == 0)
	    continue;		/* write code for them another time */

	r = l->pl_radius;
	l->pl_angle += configvals->planupdspd * 1e2 / sqrt(r * r * r);
	if (l->pl_angle > 2 * M_PI)
	    l->pl_angle -= 2 * M_PI;
	x = planets[stars[l->pl_system]].pl_x + cos(l->pl_angle) * l->pl_radius;
	y = planets[stars[l->pl_system]].pl_y + sin(l->pl_angle) * l->pl_radius;
	move_planet(i, x, y, 1);
    }
}


/* This function pops the armies on planets.  It now uses the tables
   that are in this module.  Negative growth is supported. */

static void 
pop_one_planet1(struct planet *l)
{
    int     r;			/* to hold resource index */
    int     a;			/* to hold atmosphere index */
    int     t;			/* temp var */
    float   ft;			/* another temp */

    r = (l->pl_flags & PLRESMASK) >> PLRESSHIFT;	/* get resource bits */
    r = restores[r];		/* convert to 3 bits */
    a = l->pl_flags & PLATMASK;	/* get atmosphere bits */
    a = a >> PLATSHIFT;		/* shift to lower two bits of int */

    t = popchance[a][r];	/* get pop chance */
    if (t < lrand48() % 100)	/* if planet misses chance */
	return;			/* then on to next planet */
    ft = popmult[a][r];		/* get pop multiplier */
    if (ft >= 0) {		/* positive multiplier */
	int     incr;

	if (l->pl_armies >= popcap[a][r])
	    return;

	if (l->pl_armies < 4)	/* chance of going over four */
	    l->pl_flags |= PLREDRAW;	/* slate planet for redraw */
	if (configvals->new_army_growth)
	    incr = 1 + 0.5 + ft * (float) (l->pl_armies);
	else
	    /* what about agri? */
	    incr = 1 + lrand48() % 3;	/* bronco is always 1-3 armies/pop */
	tlog_pop(l, incr);
	l->pl_armies += incr;	/* add on multiplier armies */
	l->pl_tinfo[l->pl_owner].armies = l->pl_armies;
	l->pl_tinfo[l->pl_owner].timestamp = status->clock;
    }
    else {			/* negative multiplier */
	int     decr;
	if (l->pl_armies < 4)	/* no killing all the armies on */
	    return;		/* a planet */
	l->pl_flags |= PLREDRAW;/* slate planet for redraw */
	decr = 1 + 0.5 - ft * (float) (l->pl_armies);
	tlog_pop(l, -decr);
	l->pl_armies -= decr;	/* subtract armies */
	l->pl_armies = (l->pl_armies < 0) ? 0 : l->pl_armies;
	if (l->pl_armies <= 0) {/* if all died then */
	    l->pl_armies = 0;	/* no negative armies */
	    l->pl_tinfo[l->pl_owner].armies = 0;	/* old owner knows */
	    l->pl_tinfo[l->pl_owner].timestamp = status->clock;
	    l->pl_tinfo[l->pl_owner].owner = NOBODY;
	    l->pl_owner = NOBODY;	/* planet is neutral */
	}
	l->pl_tinfo[l->pl_owner].armies = l->pl_armies;
	l->pl_tinfo[l->pl_owner].timestamp = status->clock;
	if (l->pl_armies == 0)	/* if all armies died */
	    checkwin(enemy_admiral(-1));	/* check if game over */
    }
}
/*
 */

static void 
pop_one_planet2(struct planet *l)
{
    int     delta = 0;
    int     atmosphere = (l->pl_flags & PLATMASK) >> PLATSHIFT;
    int     surface = (l->pl_flags & PLSURMASK) >> PLSURSHIFT;

    if (l->pl_armies < 4 && (l->pl_flags & PLAGRI))
	delta++;

    if (atmosphere == 0) {
	if (l->pl_armies >= 4 && drand48() < 0.5) {
	    delta--;
	}
    }
    else if (atmosphere < 3) {
	if (drand48() <
	    ((atmosphere == 1)
	     ? (l->pl_armies < 4 ? 0.2 : 0.1)
	     : (l->pl_armies < 4 ? 0.5 : 0.2))) {
	    delta++;
	}
    }
    else {
	if (drand48() < ( l->pl_armies<4 ? 0.3 : 0.2)) {
	    static int table[] = {2, 2, 2, 3, 3, 3, 4, 4};
	    int     max = table[surface];
	    delta++;
	    if (l->pl_flags & PLAGRI && l->pl_armies>=4)
		max++;		/* armies<4 handled at top */
	    delta += lrand48() % max;
	}
    }

    if (l->pl_armies==0 && delta>1)
	delta = 1;		/* training that first army is tough */

    tlog_pop(l, delta);

    if (delta > 0 &&
	l->pl_armies > popcap[atmosphere]
	[restores[(l->pl_flags & PLRESMASK) >> PLRESSHIFT]])
	delta = 0;
    l->pl_armies += delta;

    /*
       I doubt this if statement will ever get executed unless someone's
       frobbing shmem
    */
    if (l->pl_armies <= 0) {	/* if all died then */
	l->pl_armies = 0;	/* no negative armies */

	tlog_revolt(l);		/* well, not exactly. */

	l->pl_tinfo[l->pl_owner].armies = 0;	/* old owner knows */
	l->pl_tinfo[l->pl_owner].timestamp = status->clock;
	l->pl_tinfo[l->pl_owner].owner = NOBODY;
	l->pl_owner = NOBODY;	/* planet is neutral */
	checkwin(enemy_admiral(-1));	/* check if game over */
    }

    l->pl_tinfo[l->pl_owner].armies = l->pl_armies;
    l->pl_tinfo[l->pl_owner].timestamp = status->clock;
}

static void 
pop_one_planet(struct planet *l)
{
    switch (configvals->popscheme) {
    case 1:
	pop_one_planet2(l);
	break;
    default:
    case 0:
	pop_one_planet1(l);
	break;
    }
}


static int 
find_other_team(int teammask)
{
    int     counts[NUMTEAM];
    int     i;
    for (i = 0; i < NUMTEAM; i++)
	counts[i] = 0;
    for (i = 0; i < MAXPLAYER; i++) {
	int     teamidx = mask_to_idx(players[i].p_team);
	if (teamidx >= 0 && teamidx < NUMTEAM && !(teammask & (1 << teamidx)))
	    counts[teamidx]++;
    }

    {
	int     max = -1;
	int     rval = 0;
	for (i = 0; i < NUMTEAM; i++) {
	    if (counts[i] > max && !(teammask & (1 << i))) {
		max = counts[i];
		rval = 1 << i;
	    }
	}
	return rval;
    }
}

void 
popplanets(void)
{
    register int i;		/* for looping */
    register struct planet *l;	/* to point to individual plaent */

    if (!status->tourn)		/* check t-mode */
	return;			/* do not pop planets */

    switch (configvals->popchoice) {
    case 0:{
	    static int fuse = 0;
	    if (++fuse < PLANETFUSE)
		return;		/* nothing to do */
	    fuse = 0;
	    for (i = 0, l = &planets[i]; i < NUMPLANETS; i++, l++) {
		if ((l->pl_armies == 0) ||
		    (PL_TYPE(*l) == PLSTAR) ||
		    (PL_TYPE(*l) == PLWHOLE))  /* if no armies to pop */
		    continue;	/* go to next planet */
		pop_one_planet(l);
	    }
	} break;
    case 1:{
	    static int Aplanets[MAXPLANETS];
	    static int Acount = 0, Amask = 0;
	    static int Bplanets[MAXPLANETS];
	    static int Bcount = 0, Bmask = 0;

	    int     INDchance;
	    float   Achance, Bchance;
	    float   f;

	    /* make sure there's planets in the lists */
	    if (Acount < 1) {
		Amask = find_other_team(Bmask);
		fill_planets(Aplanets, &Acount, Amask);
	    }
	    if (Bcount < 1) {
		Bmask = find_other_team(Amask);
		fill_planets(Bplanets, &Bcount, Bmask);
	    }

	    /* figure out the probabilities */
	    INDchance = Achance = Bchance = 0;
	    for (i = 0; i < configvals->numplanets; i++) {
		if (planets[i].pl_owner == Amask)
		    Achance += 1.0;
		else if (planets[i].pl_owner == Bmask)
		    Bchance += 1.0;
		else if (!((PL_TYPE(planets[i]) == PLSTAR) ||
			 (PL_TYPE(planets[i]) == PLWHOLE)))
		   /* if (planets[i].pl_owner == NOBODY) */
		    INDchance += 1.0;
	    }

	    if(configvals->losing_advantage >= 1.0)
	    {
		double	la = configvals->losing_advantage;
		if (Achance < Bchance)
		    Achance *= (la - (la-1)*Achance / Bchance);
		else
		    Bchance *= (la - (la-1)*Bchance / Achance);
	    }

	    f = drand48() * (INDchance + Achance + Bchance);
	    if (f < INDchance) {
	    }
	    else if (f - INDchance < Achance) {
		pop_one_planet(&planets[Aplanets[--Acount]]);
	    }
	    else {
		pop_one_planet(&planets[Bplanets[--Bcount]]);
	    }

	} break;
    }
}





/*----------------------------------PLFIGHT--------------------------------*/
/*  This function does the fighting for a planet.  It decs the warning
timer and clears the REDRAW flag.  It then handles the bombing for the
players.  It then goes on to see if a player is visible to the other team
by checking the closeness of other players.  The planets are then checked
to see if they are close enough to fire on enemy players.  */

void 
plfight(void)
{
    register int h;		/* looping vars */
    register struct planet *l;	/* for looping through planets */

    for (h = 0, l = &planets[h]; h < NUMPLANETS; h++, l++) {
	l->pl_flags &= ~PLREDRAW;	/* clear the PLDRAW flag */
	if (l->pl_warning > 0)	/* check if timer needs dec */
	    l->pl_warning--;	/* dec the warning timer */
    }
    pbomb();			/* go do bombing for players */
    pvisible();			/* check visibility w/ other players */
    pfire();			/* let planets fire on other players */
}




/*-------------------------------SAVE_PLANETS-----------------------------*/
/*  This function saves the planets structure to disk.  The plfd variable
is a handle on the previously opened .planets file.  The function also saves
the status to the glfd file.  */

void 
save_planets(void)
{
    if (plfd >= 0) {		/* if plfd file open */
	(void) lseek(plfd, (long) 0, 0);	/* save the planets */
	(void) write(plfd, (char *) planets, sizeof(struct planet) * MAXPLANETS);
    }
    if (glfd >= 0) {		/* if glfd file open then */
	(void) lseek(glfd, (long) 0, 0);	/* save the status */
	(void) write(glfd, (char *) status, sizeof(struct status));
    }
}

/*-------------------------------------------------------------------------*/



/*----------END OF FILE--------*/
