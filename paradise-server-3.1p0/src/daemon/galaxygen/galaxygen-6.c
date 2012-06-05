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

#define SYSWIDTH	(GWIDTH/4.5)	/* 5.9 width of a system */

#define SYSTEMS		6	/*  9 number of planetary systems */

/*atmosphere chances form a cascade win rand()%100*/
#define PATMOS1		40	/* chance for normal atmosphere */
#define PATMOS2		70	/* chance for thin atmosphere */
#define PATMOS3		90	/* chance for slightly toxic stmos */
#define PPOISON		100	/* chance for poison atmos */

/*defines that deal with planets resources and types*/
#define NMETAL		13	/* number of metal deposits */
#define NDILYTH		10	/* number of dilythium deposits */
#define NARABLE		15	/* number of arable land planets */
 /* defines that deal with star placement */

#define GW	((float)GWIDTH)	/* size of galaxy in floating point */
#define	STARBORD	(GW*0.27)
#define TEAMBORD	(GW*0.32)
#define STARMIN		(GW/5.6)/* min dist between stars */
#define STARMAX		GW
#define TEAMMIN		(GW/2.8)/* min dist between team stars */
#define TEAMMAX		(GW/1.8)/* max dist between team stars */

 /* defines that deal with systems and their planets */
#define SYSADD		2	/* number possible above min number */
#define SYSBORD		(7000.0 + (float)GWIDTH/200)	/* min distance from
							   border wall */
#define INDBORD		(GW*0.23)
#define SYSMIN		(5500.0 + (float)GWIDTH/200)	/* min distance between
							   objects */
#define SYSMIN2		(SYSMIN*SYSMIN)	/* square of sysmin distance */
#define SYSPLMIN	9	/* 5 min number of planets for system */
#define SYSPLADD	0	/* number of possible extra planets */
#define MINARMY 5 /* 8*/		/* min numer of armies on a planet */
#define MAXARMY 6 /* 15*/		/* max number of armies on a planet */

 /* other defines */
#define HOMEARMIES 30		/* number of armies on home planets */
#define COLONYARMIES 10		/* number of armies for colony planet */


/*-----------------------------PLACESYSTEMS------------------------------*/
/*  This function places the planets in each star's system.  The function
will return the index of the first planet that was not placed in a system.
The coordinates of the planets are placed in the space grid.  */

static int 
placesystems(void)
{
    int     i, j, k;		/* looping vars */
    double  x=0, y=0;		/* to hold star coordinates */
    int     done;		/* flag to indicate done */
    double  dx, dy;		/* delta x and y's */
    int     n;			/* number of planet to place */
    int     np;			/* number of planets in system */
    int     attempts;

    n = SYSTEMS;		/* first planet to place */
    for (i = 0; i < SYSTEMS; i++) {	/* planets for each system */
	np = SYSPLMIN + lrand48() % (SYSPLADD + 1);	/* how many planets */
	for (k = 0; k < np; k++) {	/* go place the planets */
	    attempts = 0;
	    do {		/* do until location found */
		attempts++;
		done = 0;	/* not done yet */
		dx = (drand48() * SYSWIDTH - SYSWIDTH / 2.0);
		dy = (drand48() * SYSWIDTH - SYSWIDTH / 2.0);
		if (dx * dx + dy * dy > (SYSWIDTH / 2.0) * (SYSWIDTH / 2.0))
		    continue;	/* might orbit its way out of the galaxy */
		x = planets[i].pl_x + dx;
		y = planets[i].pl_y + dy;
		if ((x > GW - SYSBORD) || (x < SYSBORD)
		    || (y < SYSBORD) || (y > GW - SYSBORD))
		    continue;	/* too close to border? */

		done = 1;	/* assume valid coord found */
		for (j = 0; j < n; j++) {	/* go through previous
						   planets */
		    dx = fabs(x - (double) planets[j].pl_x);
		    dy = fabs(y - (double) planets[j].pl_y);
		    if (dx * dx + dy * dy < SYSMIN2) {	/* if too close to
							   another star */
			done = 0;	/* we must get another coord */
		    }
		}
	    } while (!done && attempts < 200);	/* do until location found */

	    if (!done)
		return 0;	/* universe too crowded, try again */

	    move_planet(n, (int) x, (int) y, 0);
	    planets[n].pl_system = i + 1;	/* mark the sytem number */
	    planets[n].pl_armies = MINARMY + lrand48() % (MAXARMY - MINARMY);
	    n++;		/* go to next planet */
	}
    }
    return (n);			/* return index of next planet */
}




/*-----------------------------PLACEINDEP------------------------------*/
/*  This function places idependent planets that are not in a system.
They can appear anywhere in the galaxy as long as they are not too close
to another planet.  The coords are put in the space grid.  */

static int 
placeindep(int n)
 /* number of planet to start with */
{
    int     i, j;		/* looping vars */
    double  x, y;		/* to hold star coordinates */
    int     done;		/* flag to indicate done */
    double  dx, dy;		/* delta x and y's */
    int     attempts;

    for (i = n; i < (NUMPLANETS - (WORMPAIRS*2)); i++) 
	 {	/* go through rest of planets */
	x = drand48() * (GW - 2 * INDBORD) + INDBORD;	/* pick initial coords */
	y = drand48() * (GW - 2 * INDBORD) + INDBORD;
	attempts = 0;
	do {			/* do until location found */
	    attempts++;
	    done = 0;		/* not done yet */
	    x = INDBORD + fmod(x + (3574.0 - INDBORD), GW - 2 * INDBORD);	/* offset coords a
										   little */
	    y = INDBORD + fmod(y + (1034.0 - INDBORD), GW - 2 * INDBORD);	/* every loop */
	    done = 1;		/* assume valid coord */
	    for (j = 0; j < n; j++) {	/* go through previous planets */
		dx = fabs(x - (double) planets[j].pl_x);
		dy = fabs(y - (double) planets[j].pl_y);
		if (dx * dx + dy * dy < SYSMIN2) {	/* if planet to close */
		    done = 0;	/* we must get another coord */
		}
	    }
	} while (!done && attempts < 200);	/* do until location found */

	if (!done)
	    return 0;

	move_planet(n, (int) x, (int) y, 0);
	planets[n].pl_system = 0;	/* mark the no sytem */
	planets[n].pl_armies = MINARMY + lrand48() % (MAXARMY - MINARMY);
	n++;			/* go to next planet */
    }
    for (i = n; i < NUMPLANETS; i++) /* now place wormholes */ {
	x = drand48() * GW;  /* pick intial coords */
	y = drand48() * GW;
	attempts = 0;
	do {			/* do until location found */
	    attempts++;
	    done = 0;		/* not done yet */
	    x = fmod(x + 3574.0, GW);	/* offset coords a little */
	    y = fmod(y + 1034.0, GW);	/* every loop */
	    done = 1;		/* assume valid coord */
	    for (j = 0; j < n; j++) {	/* go through previous planets */
		dx = fabs(x - (double) planets[j].pl_x);
		dy = fabs(y - (double) planets[j].pl_y);
		if (dx * dx + dy * dy < SYSMIN2) {	/* if planet to close */
		    done = 0;	/* we must get another coord */
		}
	    }
	} while (!done && attempts < 200);	/* do until location found */

	if (!done)
	    return 0;

	move_planet(n, (int) x, (int) y, 0);
	planets[n].pl_system = 0;	/* mark the no system */
	planets[n].pl_flags |= PLWHOLE; /* mark the planet as a wormhole */
	/* the armies in a wormhole is the other wormhole's x coord */
	/* the radius is the other wormhole's y coord*/
	if (NUMPLANETS%2) {
	   if (!(n%2)) {
	      planets[n].pl_armies = planets[n-1].pl_x;
	      planets[n].pl_radius = planets[n-1].pl_y;
	      planets[n-1].pl_armies = planets[n].pl_x;
	      planets[n-1].pl_radius = planets[n].pl_y;
	   }
	} else {
	   if (n%2) {
	      planets[n].pl_armies = planets[n-1].pl_x;
	      planets[n].pl_radius = planets[n-1].pl_y;
	      planets[n-1].pl_armies = planets[n].pl_x;
	      planets[n-1].pl_radius = planets[n].pl_y;
	   }
	}
	planets[i].pl_owner = NOBODY;	/* no team owns a star */
	planets[i].pl_hinfo = ALLTEAM;	/* all teams know its a star */
	for (j = 0; j < MAXTEAM + 1; j++) {	/* go put in info for teams */
	    planets[i].pl_tinfo[j].owner = NOBODY;	/* nobody owns it */
	    planets[i].pl_tinfo[j].armies = 0;
	    planets[i].pl_tinfo[j].flags = planets[i].pl_flags;
	}
	n++;			/* go to next planet */       
    }
    return 1;
}




/*---------------------------------PLACERACES------------------------------*/
/*  This function places the races in the galaxy.  Each race is placed in
a different system.  The race is given a home world with an Agri and Ship-
yard on it and HOMEARMIES.  They are also given a conoly planet with
dilythium deposits and COLONYARMIES on it.  */

static void
placeraces(void)
{
    int     i, j, k/*, x*/;        /* looping vars */
    int     p;                 /* to hold planet for race */
    int     r[4], t;

    r[0] = r[1] = lrand48() % 4;/* pick two races at random.  They will be */
    while(r[0] == r[1])         /*   the races whose systems are 'optimally' */
        r[1] = lrand48() % 4;   /*   placed. */
    i = 0;
    while(i == r[0] || i == r[1])
        i++;
    r[2] = i++;
    while(i == r[0] || i == r[1])
        i++;
    r[3] = i;

    /* only allow these teams */
    status2->nontteamlock = (1 << r[0]) | (1 << r[1]);

    for (i = 0; i < 4; i++) {    /* go through races */
        t = r[i];    /* which team */
        p = lrand48() % NUMPLANETS;    /* pick random planet */
/*        for (x=0; x <= 1; x++) {*/  /* loop twice for 2 systems */
            while ((planets[p].pl_system != i + 1)
                   || (PL_TYPE(planets[p]) == PLSTAR)
                   || (planets[p].pl_owner != NOBODY))
                p = (p + 1) % NUMPLANETS;    /* go on to next planet */

            planets[p].pl_flags &= ~PLSURMASK;    /* make sure no dilithium */
            planets[p].pl_flags |= (PLMETAL | PLARABLE);    /* metal and arable */
            planets[p].pl_flags |= PLATYPE1;    /* good atmosphere */
            planets[p].pl_flags |= (PLAGRI | PLSHIPYARD | PLREPAIR);
            planets[p].pl_tagri = PLGAGRI;    /* set timers for resources */
            planets[p].pl_tshiprepair = PLGSHIP;
            planets[p].pl_owner = 1 << t;    /* make race the owner */
            planets[p].pl_armies = HOMEARMIES;    /* set the armies */
            planets[p].pl_hinfo = 1 << t;    /* race has info on planet */
            planets[p].pl_tinfo[1 << t].owner = 1 << t;    /* know about owner */
            planets[p].pl_tinfo[1 << t].armies = planets[p].pl_armies;
            planets[p].pl_tinfo[1 << t].flags = planets[p].pl_flags;

            /* find colony planet */
            p = lrand48() % NUMPLANETS;    /* pick random planet */
            while ((planets[p].pl_system != i + 1)
                   || (PL_TYPE(planets[p]) == PLSTAR)
                   || (planets[p].pl_owner != NOBODY))
                p = (p + 1) % NUMPLANETS;    /* go on to next planet */
            planets[p].pl_flags |= PLFUEL;    /* make fuel depot */
            planets[p].pl_tfuel = PLGFUEL;    /* set timer for fuel depot */
            planets[p].pl_flags &= ~PLATMASK;    /* take off previous atmos */
            planets[p].pl_flags |= PLPOISON;    /* poison atmosphere */
            planets[p].pl_flags |= PLDILYTH;    /* dilythium deposits */
            planets[p].pl_owner = 1 << t;    /* make race the owner */
            planets[p].pl_armies = COLONYARMIES;    /* set the armies */
            planets[p].pl_hinfo = 1 << t;    /* race knows about */
            planets[p].pl_tinfo[1 << t].owner = 1 << t;    /* know about owner */
            planets[p].pl_tinfo[1 << t].armies = planets[p].pl_armies;
            planets[p].pl_tinfo[1 << t].flags = planets[p].pl_flags;
            for (j = 0; j < NUMPLANETS; j++) {
                if ((planets[j].pl_system == i + 1)
                    && (PL_TYPE(planets[j]) != PLSTAR)) {
                    for (k = (status2->league ? 0 : t);
                         k < (status2->league ? 4 : t + 1);
                         k++)
        {
            planets[j].pl_owner = 1 << t;
            planets[j].pl_hinfo =
              status2->league ? (1 << 4) - 1 :
              (1 << t);
            planets[j].pl_tinfo[1 << k].owner = 1 << t;
            planets[j].pl_tinfo[1 << k].armies = planets[j].pl_armies;
            planets[j].pl_tinfo[1 << k].flags = planets[j].pl_flags;
        }
        }
    }
    }
}

/* Generate a complete galaxy.
   This variation is similar to gen_galaxy_1; except that it tries
   to place the races at consistent distances from one another.
   */

void 
#ifdef LOADABLE_PLGEN
gen_galaxy(void)
#else
gen_galaxy_6(void)
#endif
{
    int     t;

    NUMPLANETS = 60;            /* planets + wormholes */
    GWIDTH = 200000;

    while (1) {
	initplanets();		/* initialize planet structures */

	/* place the resources */
	zero_plflags(planets, NUMPLANETS);
	randomize_atmospheres(planets+SYSTEMS, NUMPLANETS-SYSTEMS,
			      PATMOS1,PATMOS2,PATMOS3,PPOISON);
	randomize_resources(planets+SYSTEMS, NUMPLANETS-SYSTEMS,
			    NMETAL, NDILYTH, NARABLE);

	/* place system centers */
	t = place_stars(planets, 2,
			(int)TEAMBORD, (int)TEAMMIN, (int)TEAMMAX,
			(struct planet*)0, 0)
	  && place_stars(planets+2, 2,
			 (int)(STARBORD * 0.8), (int)TEAMMIN, (int) STARMAX,
			 planets, 2)
	  && place_stars(planets+4, SYSTEMS-4,
			 (int)STARBORD, (int)STARMIN, (int) STARMAX,
			 planets, 4);

	if (!t)
	    continue;
	t = placesystems();	/* place planets in systems */
	if (!t)
	    continue;
	t = placeindep(t);	/* place independent planets */
	if (t)
	    break;		/* success */
    }
    if (configvals->justify_galaxy)
       justify_galaxy(SYSTEMS);
    placeraces();		/* place home planets for races */
}

#ifdef LOADABLE_PLGEN
int
galaxy_type(void)
{
  return(3);
}
#endif
