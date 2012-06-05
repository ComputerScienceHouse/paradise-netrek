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

#define COIN_TOSS (lrand48() % 2)
#define SYMMETRIC_VARIANCE(k) \
  (lrand48() % (2 * (k) + 1)) - ((k)/2)

#define GALAXY_WIDTH 200000
#define RACE_DISTANCE_FROM_CENTER (GWIDTH/2 - GWIDTH/6)
#define RACE_SYSTEM_VARIANCE (GWIDTH/250)
#define MIN_PLANET_ORBIT_DISTANCE 8000
#define MAX_PLANET_ORBIT_DISTANCE \
  (3 * GWIDTH / 40 <= MIN_PLANET_ORBIT_DISTANCE ? \
    MIN_PLANET_ORBIT_DISTANCE + 1 : \
    3 * GWIDTH / 40)
#define PLANET_ANGULAR_VARIANCE 20

#define IND_SYSTEM_VARIANCE (GWIDTH/250)
#define IND_DISTANCE_FROM_CENTER (GWIDTH/8)

#define HOME_PLANET_ARMIES 20
#define HOME_MARS_ARMIES 14
#define HOME_FUEL_ARMIES 8
#define HOME_BARREN_ARMIES 5
#define MAX_INDEP_ARMIES 3

typedef void (*planet_function)(struct planet *);

/* helper functions to populate independent's planets */
static void
ind_developed(struct planet *p)
{
  if(COIN_TOSS == 1)
    p->pl_armies = (lrand48() % (MAX_INDEP_ARMIES-1)) + 1;
}

/* note that the construction timers on IND planets are set to 
   "quarter built" in the beginning */
static void
populate_ind_type1(struct planet *p)
{
  /* place resources */
  p->pl_flags |= (PLPLANET | PLATYPE1 | PLMETAL);

  if(p->pl_armies)
  {
    p->pl_flags |= (PLREPAIR);
    p->pl_tshiprepair = PLGREPAIR / 4;
  }
}

static void
populate_ind_type2(struct planet *p)
{
  /* place resources */
  p->pl_flags |= (PLPLANET | PLDILYTH);

  if(COIN_TOSS == 1)
    p->pl_flags |= PLPOISON;
  else
    p->pl_flags |= PLATYPE3;

  if(p->pl_armies)
  {
    p->pl_flags |= (PLFUEL);
    p->pl_tfuel = PLGFUEL / 4;
  }
}

static void
populate_ind_type3(struct planet *p)
{
  p->pl_flags |= (PLPLANET | PLPOISON);
}

static void
populate_ind_type4(struct planet *p)
{
  p->pl_flags |= (PLPLANET | PLATYPE3 | PLARABLE);

  if(p->pl_armies)
  {
    p->pl_flags |= (PLAGRI);
    p->pl_tagri = PLGAGRI / 4;
  }
}

/* place independent system with k stars, n planets, at about x, y */
static int
place_ind(int p, int k, int planets_to_populate, int x, int y)
{
  int xvar, yvar, maxyvar;
  planet_function pl_funcs[] = {
    populate_ind_type1, populate_ind_type2, populate_ind_type3,
    populate_ind_type4, populate_ind_type3
  };
  int ang, ang_var, dist, i, ppick, j;
  int sys;

  sys = 0;

  xvar = SYMMETRIC_VARIANCE(IND_SYSTEM_VARIANCE);
  maxyvar = (int)sqrt((double)(IND_SYSTEM_VARIANCE * IND_SYSTEM_VARIANCE * 4 - 
                               xvar * xvar));
  yvar = SYMMETRIC_VARIANCE(maxyvar);
  
  planets[p].pl_armies = 0;
  planets[p].pl_owner = NOBODY;
  planets[p].pl_flags |= PLSTAR;
  planets[p].pl_radius = 0;
  planets[p].pl_angle = 0;
  planets[p].pl_system = sys;
  planets[p].pl_hinfo = ALLTEAM;
  for(j = 0; j < MAXTEAM+1; j++)
  {
    planets[p].pl_tinfo[j].owner = NOBODY;
    planets[p].pl_tinfo[j].armies = 0;
    planets[p].pl_tinfo[j].flags = planets[p].pl_flags;
  }
  if(k == 1)
    move_planet(p, x + xvar, y + yvar, 0);
  else
    move_planet(p, x + xvar - 1000, y + yvar + 500, 0);

  if(k == 2)
  {
    p++;
    planets[p].pl_armies = 0;
    planets[p].pl_owner = NOBODY;
    planets[p].pl_flags |= PLSTAR;
    planets[p].pl_radius = 0;
    planets[p].pl_angle = 0;
    planets[p].pl_system = sys;
    planets[p].pl_hinfo = ALLTEAM;
    for(j = 0; j < MAXTEAM+1; j++)
    {
      planets[p].pl_tinfo[j].owner = NOBODY;
      planets[p].pl_tinfo[j].armies = 0;
      planets[p].pl_tinfo[j].flags = planets[p].pl_flags;
    }
    move_planet(p, x + xvar + 1000, y + yvar - 500, 0);
  }

  /* each planet:
     p->pl_owner = (1 << team);
     p->pl_tinfo[team].owner = p->pl_owner;
     p->pl_hinfo |= (1 << team);
     p->pl_system = team + 1;
  */

  for(i = 0; i < planets_to_populate; i++)
  {
    ang = ((256 * i) / planets_to_populate) + 
            SYMMETRIC_VARIANCE(PLANET_ANGULAR_VARIANCE);

    if(ang < 0)
      ang += 256;

    if(ang >= 256)
      ang -= 256;

    dist = (lrand48()%(MAX_PLANET_ORBIT_DISTANCE-MIN_PLANET_ORBIT_DISTANCE)) +
             MIN_PLANET_ORBIT_DISTANCE;

    planets[p+i+1].pl_owner = NOBODY;
    planets[p+i+1].pl_hinfo |= NOBODY;
    planets[p+i+1].pl_system = sys;
    planets[p+i+1].pl_radius = dist;
    planets[p+i+1].pl_angle = ang;

    ind_developed(planets + p + i + 1);

    /* pick type of race planet */
    ppick = (lrand48() % (planets_to_populate - i));
    /* populate this type of planet */
    pl_funcs[ppick](planets + p + i + 1);
    
    /* place planet */
    move_planet(p + i + 1, 
                planets[p].pl_x + (int)((double)(dist) * Cos[ang]),
		planets[p].pl_y + (int)((double)(dist) * Sin[ang]),
		0);

    /* remove this type from available selection */
    for(j = ppick + 1; j < (planets_to_populate - i); j++)
      pl_funcs[j-1] = pl_funcs[j];
  }

  return(planets_to_populate + k);
}

/* helper functions to populate race's planets */
static void
populate_race_home_planet(struct planet *p)
{
  /* place resources */
  p->pl_flags |= (PLPLANET | PLATYPE1 | PLDILYTH | PLMETAL | PLARABLE) |
                 /* and facilities */
		 (PLFUEL | PLREPAIR | PLSHIPYARD | PLAGRI);
  p->pl_armies = HOME_PLANET_ARMIES;
  p->pl_tfuel = PLGFUEL;
  p->pl_tagri = PLGAGRI;
  p->pl_tshiprepair = PLGSHIPYARD;
}

static void
populate_race_marslike_planet(struct planet *p)
{
  /* place resources and facilities */
  p->pl_flags |= (PLPLANET | PLATYPE2 | PLMETAL | PLARABLE) | 
                 (PLREPAIR | PLAGRI);
  p->pl_armies = HOME_MARS_ARMIES;
  p->pl_tshiprepair = PLGREPAIR;
}

static void
populate_race_fuel_planet(struct planet *p)
{
  /* place resources & facilities */
  p->pl_flags |= (PLPLANET | PLATYPE3 | PLDILYTH) | (PLFUEL);
  p->pl_armies = HOME_FUEL_ARMIES;
  p->pl_tfuel = PLGFUEL;
}

static void
populate_race_barren_planet(struct planet *p)
{
  /* place resources & facilities */
  p->pl_flags &= (PLORESMASK | PLRESMASK | PLATMASK);
  p->pl_flags |= (PLPLANET | PLPOISON);
  p->pl_armies = HOME_BARREN_ARMIES;
}

/* this function places a star + five planets for the given team at
   about the given location starting with planet p */
static int
place_race(int p, int team, int x, int y)
{
  int xvar, yvar, maxyvar;
  planet_function pl_funcs[] = {
    populate_race_home_planet,
    populate_race_marslike_planet,
    populate_race_fuel_planet,
    populate_race_fuel_planet,
    populate_race_barren_planet
  };
  int planets_to_populate = sizeof(pl_funcs) / sizeof(planet_function);
  int ang, ang_var, dist, i, ppick, j;
  int sys;

  sys = 0;
  xvar = SYMMETRIC_VARIANCE(RACE_SYSTEM_VARIANCE);
  maxyvar = (int)sqrt((double)(RACE_SYSTEM_VARIANCE * RACE_SYSTEM_VARIANCE * 4 -
                               xvar * xvar));
  yvar = SYMMETRIC_VARIANCE(maxyvar);

  planets[p].pl_armies = 0;
  planets[p].pl_owner = NOBODY;
  planets[p].pl_flags |= PLSTAR;
  planets[p].pl_radius = 0;
  planets[p].pl_angle = 0;
  planets[p].pl_system = sys;
  planets[p].pl_hinfo = ALLTEAM;
  for(j = 0; j < MAXTEAM+1; j++)
  {
    planets[p].pl_tinfo[j].owner = NOBODY;
    planets[p].pl_tinfo[j].armies = 0;
    planets[p].pl_tinfo[j].flags = planets[p].pl_flags;
  }
  move_planet(p, x + xvar, y + yvar, 0);

  /* each planet:
     p->pl_owner = (1 << team);
     p->pl_tinfo[team].owner = p->pl_owner;
     p->pl_hinfo |= (1 << team);
     p->pl_system = team + 1;
  */

  for(i = 0; i < planets_to_populate; i++)
  {
    ang = ((256 * i) / planets_to_populate) + 
            SYMMETRIC_VARIANCE(PLANET_ANGULAR_VARIANCE);

    if(ang < 0)
      ang += 256;

    if(ang >= 256)
      ang -= 256;

    dist = (lrand48()%(MAX_PLANET_ORBIT_DISTANCE-MIN_PLANET_ORBIT_DISTANCE)) +
             MIN_PLANET_ORBIT_DISTANCE;

    planets[p+i+1].pl_owner = (1 << team);
    planets[p+i+1].pl_hinfo |= (1 << team);
    planets[p+i+1].pl_system = sys;
    planets[p+i+1].pl_radius = dist;
    planets[p+i+1].pl_angle = ang;

    /* pick type of race planet */
    ppick = (lrand48() % (planets_to_populate - i));
    /* populate this type of planet */
    pl_funcs[ppick](planets + p + i + 1);

    planets[p+i+1].pl_tinfo[1 << team].owner = planets[p+i+1].pl_owner;
    planets[p+i+1].pl_tinfo[1 << team].armies = planets[p+i+1].pl_armies;
    planets[p+i+1].pl_tinfo[1 << team].flags = planets[p+i+1].pl_flags;
    
    /* place planet */
    move_planet(p + i + 1, 
                planets[p].pl_x + (int)((double)(dist) * Cos[ang]),
		planets[p].pl_y + (int)((double)(dist) * Sin[ang]),
		0);

    /* remove this type from available selection */
    for(j = ppick + 1; j < (planets_to_populate - i); j++)
      pl_funcs[j-1] = pl_funcs[j];
  }

  return(planets_to_populate + 1);
}

/* Generate a complete galaxy.
   This variation is completely different from anything else.

   Four home-race systems, placed in each compass direction.
   Two or three independent systems placed in the center
   offset from the home-race systems.  There will be a total of
   9 independent planets.  For two indep. systems there will be one
   central system with 4 planets & a binary star.

   Each home-race system has:
     1) home base with AGRI, RPR, SY, FUEL (all resources), plus 20 armies;
     2) A thin world with AGRI, RPR (agri/metal res) plus 14 armies
     3) two fuel planets (TNTD/dilith) plus 8 armies on each
     4) one barren planet (TOXC) with no resources, 5 armies.

   Each independent planet may or may not be developed.  If developed,
     maximum facilities are AGRI and/or FUEL, and up to 3 armies.  If
     not developed, no facilities, no armies.

   Each independent system will have 3, 4, or 5 planets:
     1) one STND, METAL planet
     2) one TNTD/TOXC DILITH planets
     3) one TOXC barren planet
    *4) one TNTD Arable planet (only for 5-planet systems)
    *5) one TOXC barren planet (only for 5-planet systems)

    For a total of (4 * 5) + 4 + 9 + 3 = 36 planets + stars.
    Note that 2 * 5 + 9 = 19 planets should be in play at once.

    Wormholes and justify_galaxy are not applicable for this generator.
   */

void 
#ifdef LOADABLE_PLGEN
gen_galaxy(void)
#else
gen_galaxy_9(void)
#endif
{
    int teams[NUMTEAM], i, ang, x, y, tpick, pnum = 0, j;
    int nindep;
    int sa;
    int pl_x_ctr, pl_y_ctr;

    NUMPLANETS = (4 * 5) + 4 + 9 + 3;
    /* race planets + race stars + ind planets + ind stars = 36 */

    GWIDTH = GALAXY_WIDTH;

    initplanets();		/* initialize planet structures */

    nindep = (lrand48() % 2) + 2;	/* two or three indep. systems */
    sa = (lrand48() % 256);		/* randomly orient inner systems */
    /* place_ind(p, k, n, x, y, sys) */

    if(nindep == 2)
    {
      /* randomly place the binary star */
      int rv = COIN_TOSS;
      
      for(i = 0; i < nindep; i++)
      {
        ang = (((256 * i) / nindep) + sa) % 256;
	x = (GWIDTH/2) + (int)(Cos[ang] * (double)(IND_DISTANCE_FROM_CENTER));
	y = (GWIDTH/2) + (int)(Sin[ang] * (double)(IND_DISTANCE_FROM_CENTER));
	pnum += place_ind(pnum, (rv == i ? 2 : 1), (rv == i ? 4 : 5), x, y);
      }
    }
    else
    {
      for (i = 0; i < nindep; i++)
      {
	ang = (((256 * i) / nindep) + sa) % 256;
	x = (GWIDTH/2) + (int)(Cos[ang] * (double)(IND_DISTANCE_FROM_CENTER));
	y = (GWIDTH/2) + (int)(Sin[ang] * (double)(IND_DISTANCE_FROM_CENTER));
	pnum += place_ind(pnum, 1, 3, x, y);
      }
    }

    /* compute weighted average center */
    pl_x_ctr = pl_y_ctr = 0;
    for(i = 0; i < pnum; i++)
    {
      pl_x_ctr += planets[i].pl_x;
      pl_y_ctr += planets[i].pl_y;
    }
    pl_x_ctr /= pnum;
    pl_y_ctr /= pnum;

    /* place races */
    for(i = 0; i < NUMTEAM; i++)
      teams[i] = i;

    for(i = 0; i < NUMTEAM; i++)
    {
      ang = (((256 * i) / NUMTEAM) + 32) % 256;
      x = (pl_x_ctr) + (int)(Cos[ang] * (double)(RACE_DISTANCE_FROM_CENTER));
      y = (pl_y_ctr) + (int)(Sin[ang] * (double)(RACE_DISTANCE_FROM_CENTER));
      tpick = (lrand48() % (NUMTEAM - i));
      pnum += place_race(pnum, teams[tpick], x, y);

      /* team placed, eliminate them from contention */
      for(j = tpick + 1; j < (NUMTEAM - i); j++)
        teams[j-1] = teams[j];
    }

    for(i = pnum; i < MAXPLANETS; i++)
    {
      planets[i].pl_x = -1;
      planets[i].pl_y = -1;
    }
}

#ifdef LOADABLE_PLGEN
int
galaxy_type(void)
{
  return(3);
}
#endif
