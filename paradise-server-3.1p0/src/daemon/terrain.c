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

#define MAXALTITUDE 255
#define MAXSTARS 20
#define X 0
#define Y 1

/* Values inbetween MAXALTITUDE and minalt are considered nebulous terrain.
   Tries to cluster seeds in groups based on num_nebula and num_seeds.
   ...the number of seeds per nebula being num_seeds. 
   10/26/94 MM */
static void 
place_nebula(int num_nebula, int num_seeds, int minalt)
{
    int i = 0,j = 0,x,y,dx,dy, dist, lowdist = 2 * TGRID_SIZE;
    int *seeds1=NULL, *seeds2=NULL;
 
    seeds1 = (int *)malloc(num_nebula * sizeof(int));
    if (num_seeds)
       seeds2 = (int *)malloc(num_seeds * sizeof(int) * num_nebula);
 
    /* find a local minimum, and place a "seed" */
    while (i < num_nebula) {
       j = (int)lrand48()%(TGRID_SIZE*TGRID_SIZE);
       if (j == 0) j = 1;
       while ((j < (TGRID_SIZE * TGRID_SIZE)) &&
 	     ((terrain_grid[j-1].alt1 < terrain_grid[j].alt1) ||
 	      (terrain_grid[j+1].alt1 < terrain_grid[j].alt1))) j++;
       seeds1[i] = j;
       terrain_grid[seeds1[i]].alt2 = MAXALTITUDE;
       i++;
    }
    /* group num_seeds more "sub seeds" around each seed */
    /* (there are a couple bugs in this algorithm yet -- theres a wierd wraparound
        occasionally.  MDM, 8/23/95) */
    
    for (i=0; i < num_nebula; i++)
       for (j=0; j < num_seeds; j++) {
          dx = (int)(lrand48() % ((MAXALTITUDE-minalt)*3)) - 
 	    (int)(lrand48() % (int)((MAXALTITUDE-minalt)*(1.5)));
 	 dy = (int)(lrand48() % ((MAXALTITUDE-minalt)*3)) - 
 	    (int)(lrand48() % (int)((MAXALTITUDE-minalt)*(1.5)));
 	 if (seeds1[i]/TGRID_SIZE + dx < 0) 
 	    dx -= (seeds1[i]/TGRID_SIZE + dx);
 	 if (seeds1[i]/TGRID_SIZE + dx >= TGRID_SIZE) 
 	    dx -= (seeds1[i]/TGRID_SIZE + dx) - (TGRID_SIZE - 1);
 	 if (seeds1[i]/TGRID_SIZE + dy < 0) 
 	    dy -= (seeds1[i]/TGRID_SIZE + dy);
 	 if (seeds1[i]/TGRID_SIZE + dy >= TGRID_SIZE) 
 	    dy -= (seeds1[i]/TGRID_SIZE + dy) - (TGRID_SIZE - 1);
 	 seeds2[i*num_seeds + j] = (seeds1[i]/TGRID_SIZE + dx) * TGRID_SIZE +
 	                           (seeds1[i]%TGRID_SIZE + dy);
 	 terrain_grid[seeds2[i*num_seeds + j]].alt2 = MAXALTITUDE;
       }
 
    /* assign random-ish values, from a distance-from-seed base value (density
       is the randomness -- low density values are smooth, high are rough) */
    /* randomness NYI */ 
    /* these values could be used in combination with the alt1 values to do
       other funky terrain "shapes, but I'm putting in the ABS() speedup
       stuff in for now anyway.  It'll result in alt2 values of 0 for
       any spot outside a nebulous radius -- MDM */
    for (x=0; x < TGRID_SIZE; x++)
       for (y=0; y < TGRID_SIZE; y++) {
 	 for (i=0; i < num_nebula; i++) {
 	    dx = (seeds1[i]/TGRID_SIZE) - x;
 	    dy = (seeds1[i]%TGRID_SIZE) - y;
 	    /* loop speedup */
 	    if ((ABS(dx) <= MAXALTITUDE-minalt) && 
 		(ABS(dy) <= MAXALTITUDE-minalt)) {
 	       dist = (int)sqrt(dx * dx + dy * dy);
 	       if (dist < lowdist) lowdist = dist;
 	    }
 	    if (num_seeds)
 	       for (j=0; j < num_seeds; j++) {
 		  dx = seeds2[i*num_seeds + j]/TGRID_SIZE-x;
 		  dy = seeds2[i*num_seeds + j]%TGRID_SIZE-y;
 		  /* loop speedup */
 		  if ((ABS(dx) <= MAXALTITUDE-minalt) && 
 		      (ABS(dy) <= MAXALTITUDE-minalt)) {
 		     dist = (int)sqrt(dx * dx + dy * dy);
 		     if (dist < lowdist) lowdist = dist;
 		  }
 	       }
 	 }
 	 terrain_grid[x*TGRID_SIZE + y].alt2 = MAXALTITUDE - lowdist;
 	 lowdist = 2*TGRID_SIZE;
       }
 
    /* give each spot with a high enuf alt value the nebulous terrain flag. */
    for (i=0; i < TGRID_SIZE; i++)
       for (j=0; j < TGRID_SIZE; j++) 
 	 if (terrain_grid[i * TGRID_SIZE + j].alt2 >= minalt)
 	    terrain_grid[i * TGRID_SIZE + j].types |= T_NEBULA;
  
    free(seeds1);
    free(seeds2);
}

/* Marks terrain grid locations within density of altitude as asteroid
   fields.  I may make the chance of such a grid location becoming a
   field random (like 90% or something), so that fields will appear less
   uniform, and holes may exist in them.  Makes for interesting terrain, IMO.
   10/26/94 MM */
static void 
place_asteroids(int altitude)
{
  int x,y,i,j,numstars=0;
  int *systems_with_asteroids;
  int *star_numbers;
  int *varied_rad;
  int *varied_dens;
  float *varied_thick;

  star_numbers = (int *)malloc( (NUMPLANETS)*sizeof( int ) );
  for (i=0; i < NUMPLANETS; i++)
     if (PL_TYPE(planets[i]) == PLSTAR) {
	star_numbers[numstars] = i;
	numstars++;
     }
  systems_with_asteroids = (int *)malloc(numstars*sizeof(int));
  varied_rad = (int *)malloc(numstars*sizeof(int));
  varied_dens = (int *)malloc(numstars*sizeof(int));
  varied_thick = (float *)malloc(numstars*sizeof(float));
  for (i=0; i < numstars; i++) systems_with_asteroids[i] = 0;

/* assign what stars have asteroid belts -- I could just start with system #1,
   since systems are placed randomly, but I might as well pick a random system
   to start with.  The only prereq is that the system does NOT belong to a
   race. (prereq NYI) */

  if (*num_asteroid > numstars) {
	*num_asteroid = numstars;
  }


  for (i = 0, j = lrand48()%numstars; i < *num_asteroid; i++) {
     systems_with_asteroids[j] = 1;
     if ((*asteroid_rad_variance) == 0) varied_rad[j] = altitude; 
     else varied_rad[j] = altitude - ((*asteroid_rad_variance) / 2) +
	                             lrand48()%(*asteroid_rad_variance);
     if ((*asteroid_dens_variance) == 0) varied_dens[j] = *asteroid_density;
     else varied_dens[j] = (*asteroid_density) - ((*asteroid_dens_variance) / 2) +
	                                         lrand48()%(*asteroid_dens_variance);
     varied_thick[j] = (*asteroid_thickness) - ((*asteroid_thick_variance)/2.0)+
	               drand48() * (*asteroid_thick_variance);
     if (j == numstars-1) j = 0;
     else j++;
  }

  for (i=0; i < numstars; i++)
     if (systems_with_asteroids[i])

  for (x=0; x < TGRID_SIZE; x++)
    for (y=0; y < TGRID_SIZE; y++)
       for (i=0; i < numstars; i++) {
	  /* if the tgrid locale is within a certain distance of a system
	     that is supposed to have an asteroid belt, then, AST_CHANCE
	     percent of the time, mark that locale as having asteroids */
	  if (systems_with_asteroids[i] &&
	      terrain_grid[x * TGRID_SIZE + y].alt1 >= varied_rad[i]-(varied_thick[i])&& 
	      terrain_grid[x * TGRID_SIZE + y].alt1 <= varied_rad[i]+(varied_thick[i])&&
	      (ABS(x*TGRID_GRANULARITY - planets[star_numbers[i]].pl_x) <
	       (MAXALTITUDE-(varied_rad[i]-(varied_thick[i]+1.0))) * TGRID_GRANULARITY)&&
	      (ABS(y*TGRID_GRANULARITY - planets[star_numbers[i]].pl_y) <
	       (MAXALTITUDE-(varied_rad[i]-(varied_thick[i]+1.0))) * TGRID_GRANULARITY)&&
	      lrand48()%100 < varied_dens[i])	{
	     terrain_grid[x * TGRID_SIZE + y].types |= T_ASTEROIDS;
	}
       }
}

/* Generates terrain based on specs in the system configuration.
   Places a number of "seeds" into the terrain grid, and then generates an
   "altitude" map based on those seeds' positions (which correspond to
   star positions).  Place_nebula generates another altitude map for 
   nebulae -- the first map is used for asteroids and generating the 
   second map.

   This function is called within the galaxy generation stuff.

   10/26/94 MM */
void 
generate_terrain(void)
{
  int i/*,j,k*/;  /* counters */
  int x,y;    /* more counters, different purpose */
  double dist, val;   /* for distance calculations */
  int num_seeds = 0;
  int seed_xy[MAXSTARS][2];

/* place seeds -- this would be easy to change if you just had a number
   of seeds you wanted to place, instead of basing it on stars.  I won't
   bother doing it, even though it might be cool to see it work in a bronco
   game...  MM*/

  for (i=0; i < NUMPLANETS; i++)
    if (PL_TYPE(planets[i]) == PLSTAR) {
      terrain_grid[(planets[i].pl_x/TGRID_GRANULARITY) * TGRID_SIZE +
	           planets[i].pl_y/TGRID_GRANULARITY].alt1 = MAXALTITUDE;
      
      seed_xy[num_seeds][X] = planets[i].pl_x/TGRID_GRANULARITY;
      seed_xy[num_seeds][Y] = planets[i].pl_y/TGRID_GRANULARITY;
      num_seeds++;
    }

/* generate terrain -- simple, stupid version. */

  
  for (x=0; x < TGRID_SIZE; x++)
    for (y=0; y < TGRID_SIZE; y++) 
      if (terrain_grid[x*TGRID_SIZE + y].alt1 != MAXALTITUDE) {
	val = 0.0;
	for (i=0; i < num_seeds; i++) {
	  dist = (double)MAXALTITUDE -
	    sqrt((double)((x - seed_xy[i][X]) * (x - seed_xy[i][X]) +
		 (y - seed_xy[i][Y]) * (y - seed_xy[i][Y])));
	  if (dist > val) val = dist;
	}
	/* reset any previous terrain values */
	terrain_grid[x * TGRID_SIZE + y].types = 0x00;

	terrain_grid[x * TGRID_SIZE + y].alt1 = (int)val;
	terrain_grid[x * TGRID_SIZE + y].alt2 = 0;
      }
  
/* place nebula */
  if (num_nebula)    place_nebula(*num_nebula, *nebula_subclouds, *nebula_density);
/* place asteroids */
  if (num_asteroid)  place_asteroids(MAXALTITUDE-(*asteroid_radius));
}

#define TERRAIN_TYPE(X,Y) terrain_grid[X*TGRID_SIZE + Y].types

/* apply terrain effects to players 

   I REALLY wish I could add a "skill" element to many of the effects, but 
   it's tough.  Asteroid damage is the most notable example where 
   skill *should* be a factor, but isn't. MDM */
void
doTerrainEffects(void)
{
   struct player *p;
   int i,j,dam;
   
   for (i=0; i < MAXPLAYER; i++) {
      p = &(players[i]);
      if (p->p_status != PALIVE) continue;
      if (TERRAIN_TYPE((p->p_x)/TGRID_GRANULARITY, (p->p_y)/TGRID_GRANULARITY) &
	  T_ASTEROIDS) {
	 j = lrand48()%100;
	 /* the player is in an asteroid location */
	 if (p->p_speed != 0) {
	    if (ast_effect[SS_IMPULSE] &&
		(j < (100 - ((p->p_ship.s_turns/(p->p_speed * p->p_speed))/200)) ||
		 (j < MIN_AST_HIT))) {
	       dam = lrand48()%(VAR_AST_DAMAGE * p->p_speed) + MIN_AST_DAMAGE;
	       if( inflict_damage(0, 0, p, dam, KASTEROID) == 1 )
		p->p_whydead = KASTEROID;
	    }
	 }
      }
   }    
}
