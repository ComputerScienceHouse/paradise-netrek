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
#include "data.h"
#include "shmem.h"

/*

   This file contains utility procedures useful when laying out the
   galaxy.

 */

/*#define SLOWER*/

int
place_stars(struct planet *first, int count, int border, 
            int minpad, int maxpad, struct planet *othercheck, int ocount)
{
    int	i;
    double	x,y;

    for (i=0; i<count; i++) {
	int	done, attempts;
	int	j;

	attempts = 0;
	done = 0;
#ifndef SLOWER
	x = drand48() * (configvals->gwidth - 2*border) + border;
	y = drand48() * (configvals->gwidth - 2*border) + border;
#endif
	do {

	    attempts++;

#ifdef SLOWER
	    x = drand48() * (configvals->gwidth - 2*border) + border;
	    y = drand48() * (configvals->gwidth - 2*border) + border;
#else
	    x =border + fmod(x + 3574-border,
			     (configvals->gwidth - 2.0*border));
	    y =border + fmod(y + 1034-border,
			     (configvals->gwidth - 2.0*border));
#endif
	    done = 1;
	    /* check to make sure we aren't too close to other stars */
	    for (j=0; j<ocount; j++) {
		double dist = hypot(x-othercheck[j].pl_x,
				    y-othercheck[j].pl_y);
		if (dist<minpad || dist>maxpad) {
		    done = 0;
		    break;
		}
	    }
	    /* check to make sure we're not too close to the current
	       set of stars */
	    if (done) for (j=0; j<i; j++) {
		double dist = hypot(x-first[j].pl_x,
				    y-first[j].pl_y);
		if (dist<minpad || dist>maxpad) {
		    done = 0;
		    break;
		}
	    }
	} while (!done && attempts < 1000);
	if (!done)
	    return 0;

	first[i].pl_owner = NOBODY;
	first[i].pl_system = (&first[i] - planets)+1;
	first[i].pl_flags |= PLSTAR;
	move_planet(&first[i]-planets, (int) x, (int) y, 0);
	first[i].pl_hinfo = ALLTEAM;	/* all teams know its a star */
	for (j = 0; j < MAXTEAM + 1; j++) {	/* go put in info for teams */
	    first[i].pl_tinfo[j].owner = NOBODY;	/* nobody owns it */
	    first[i].pl_tinfo[j].armies = 0;
	    first[i].pl_tinfo[j].flags = first[i].pl_flags;
	}
    }
    return 1;
}

void
zero_plflags(struct planet *first, int count)
{
    int	i;
    for (i=0; i<count; i++) {
	first[i].pl_flags=0;
    }
}

void
randomize_atmospheres(struct planet *first, int count, 
                      int p1, int p2, int p3, int p4)
{
    int	i;
    int	sum=p1+p2+p3+p4;
    for (i=0; i<count; i++) {
	int	val;
	int	atmosphere;
	val = lrand48() % sum;
	if ( (val -= p1) < 0)
	    atmosphere = PLATYPE1;
	else if ( (val -= p2) < 0)
	    atmosphere = PLATYPE2;
	else if ( (val -= p3) < 0)
	    atmosphere = PLATYPE3;
	else 
	    atmosphere = PLPOISON;
	first[i].pl_flags &= !PLATMASK;
	first[i].pl_flags |= atmosphere;
    }
}

/* special note.

   It looks like this function originally wanted to make all Dilithium
   planets toxic, but the code was buggy and if an arable happened to
   be placed on the same planet later, you would get an STND DA.

   I am loath to fix this bug, because it would noticably alter the
   galactic mix.  This must be brought before the PLC.

   -RF

   */
void
randomize_resources(struct planet *first, int count, 
                    int nm, int nd, int na)
{
    for (; count>0; count--, first++) {
	int	val;

	val = lrand48()%count;
	if (val<nm) {
	    nm--;
	    first->pl_flags |= PLMETAL;
	    if (!configvals->resource_bombing)
		first->pl_flags |= PLREPAIR;
	}

	val = lrand48()%count;
	if (val<nd) {
	    nd--;
	    first->pl_flags |= PLDILYTH;
	    first->pl_flags &= ~(PLATMASK|PLARABLE);
	    first->pl_flags |= PLPOISON;
	    if (!configvals->resource_bombing)
		first->pl_flags |= PLFUEL;
	}

	val = lrand48()%count;
	if (val<na) {
	    na--;
	    first->pl_flags |= PLARABLE | PLATYPE1;
	    if (!configvals->resource_bombing)
		first->pl_flags |= PLAGRI;
	}
    }
}

static int
count_planets_in_system(int sysnum)
{
    int       rval=0;
    int       i;

    for (i=0; i < NUMPLANETS; i++) {
      if (PL_TYPE(planets[i])==PLPLANET &&
          planets[i].pl_system==sysnum)
          rval++;
    }
    return rval;
}

static int
pick_metal_planet_from_system(int sysnum, int nplanets)
{
    int       i;

    for (i=0; i < NUMPLANETS; i++) {
      if (PL_TYPE(planets[i])==PLPLANET &&
          planets[i].pl_system==sysnum &&
          (planets[i].pl_flags&PLMETAL) ) {
          if (lrand48()%nplanets==0)
              return i;
          nplanets--;
      }
    }
    return -1;
}

static int
pick_planet_from_system(int sysnum, int nplanets)
{
    int       i;

    if (nplanets<0)
      nplanets = count_planets_in_system(sysnum);

    for (i=0; i < NUMPLANETS; i++) {
      if (PL_TYPE(planets[i])==PLPLANET &&
          planets[i].pl_system==sysnum) {
          if (lrand48()%nplanets==0)
              return i;
          nplanets--;
      }
    }
    return -1;
}


/*  Balances the galaxy to be "fair".  Currently ensures that:
       -> One metal planet exists within each system.
*/
void
justify_galaxy(int numsystems)
{
   int i,j;
   int        *metalcount;
  
   metalcount = malloc(sizeof(*metalcount) * (numsystems+1));

   for (i=0; i <= numsystems; i++)
       metalcount[i] = 0;

   for (i=0; i < NUMPLANETS; i++) {
       switch (PL_TYPE(planets[i])) {
       case PLPLANET:
         {
             int      system = planets[i].pl_system;
             if (system<0 || system > numsystems)
                 break;
             if (planets[i].pl_flags & PLMETAL)
                 metalcount[system]++;
         }
         break;
       default:
	;
         /* don't care about other stuff */
       }
   }

   for (i=1; i <= numsystems; i++) {
       if (metalcount[i]<1) {
         int  to, from;
         int  randbase;

         randbase = lrand48()%(numsystems+1);

         for (j=0; j<=numsystems; j++)
             if (metalcount[(j+randbase)%(numsystems+1)]>1)
	    break;
	 if (j>numsystems) {
             fprintf(stderr, "error stealing metal planet.  Too few!\n");
             return;
	 }
	 j = (j+randbase)%(numsystems+1);
         to = pick_planet_from_system(i, -1);
         from = pick_metal_planet_from_system(j, metalcount[j]);
         planets[to].pl_flags |= PLMETAL;
         planets[from].pl_flags &= ~PLMETAL;
         if (!configvals->resource_bombing) {
             planets[to].pl_flags |= PLREPAIR;
             planets[from].pl_flags &= PLREPAIR;
	 }
	 metalcount[i]++;
         metalcount[j]--;
      }
   }
   free(metalcount);
}
