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

#define SYSTEMS		0	/* number of planetary systems */

/*atmosphere chances form a cascade win rand()%100*/
#define PATMOS1		100	/* chance for normal atmosphere */
#define PATMOS2		100	/* chance for thin atmosphere */
#define PATMOS3		100	/* chance for slightly toxic stmos */
#define PPOISON		100	/* chance for poison atmos */

/*defines that deal with planets resources and types*/
#define NMETAL		8	/* number of metal deposits */
#define NDILYTH		12	/* number of dilythium deposits */
#define NARABLE		8	/* number of arable land planets */

#define MINARMY 8		/* min numer of armies on a planet */
#define MAXARMY 15		/* max number of armies on a planet */

 /* other defines */
#define HOMEARMIES 30		/* number of armies on home planets */
#define COLONYARMIES 10		/* number of armies for colony planet */

/*-------------------------------INITBRONCO------------------------------*/
/*  Initializes the planet array the way normaltrek did it -- not much
   variety, but some people dig playing chess from the same setup over
   and over again too. :) */
static void 
initbronco(void)
{
    int     i, j;

    static struct planet pdata[MAXPLANETS] = {
	{0, (FED | PLHOME | PLCORE | PLFUEL | PLREPAIR | PLSHIPYARD | PLMETAL | PLARABLE | PLDILYTH),
	    FED, 20000, 80000, 0, 0, 0, "Earth", 5,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{1, FED | PLCORE, FED, 30000, 90000, 0, 0, 0, "Deneb", 5,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{2, FED | PLCORE, FED, 11000, 75000, 0, 0, 0, "Altair", 6,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{3, FED | PLCORE, FED, 8000, 93000, 0, 0, 0, "Vega", 4,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{4, FED, FED, 10000, 60000, 0, 0, 0, "Rigel", 5,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{5, FED, FED, 25000, 60000, 0, 0, 0, "Canopus", 7,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{6, FED, FED, 44000, 81000, 0, 0, 0, "Beta Crucis", 11,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{7, FED, FED, 39000, 55000, 0, 0, 0, "Organia", 7,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{8, FED, FED, 45000, 66000, 0, 0, 0, "Ceti Alpha V", 12,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{9, FED, FED, 32000, 74000, 0, 0, 0, "Alpha Centauri", 14,
	(ROM | KLI | ORI), 0, 0, 0, 30, 0, FED},
	{10, (ROM | PLHOME | PLCORE | PLFUEL | PLREPAIR | PLSHIPYARD | PLMETAL | PLARABLE | PLDILYTH),
	    ROM, 20000, 20000, 0, 0, 0, "Romulus", 7,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{11, ROM | PLCORE, ROM, 28000, 8000, 0, 0, 0, "Tauri", 5,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{12, ROM | PLCORE, ROM, 28000, 23000, 0, 0, 0, "Draconis", 8,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{13, ROM | PLCORE, ROM, 4000, 12000, 0, 0, 0, "Aldeberan", 9,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{14, ROM, ROM, 45000, 7000, 0, 0, 0, "Eridani", 7,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{15, ROM, ROM, 42000, 44000, 0, 0, 0, "Regulus", 7,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{16, ROM, ROM, 13000, 45000, 0, 0, 0, "Capella", 7,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{17, ROM, ROM, 40000, 25000, 0, 0, 0, "Sirius", 6,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{18, ROM, ROM, 25000, 44000, 0, 0, 0, "Indi", 4,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{19, ROM, ROM, 8000, 29000, 0, 0, 0, "Hydrae", 6,
	(FED | KLI | ORI), 0, 0, 0, 30, 0, ROM},
	{20, (KLI | PLHOME | PLCORE | PLFUEL | PLREPAIR | PLSHIPYARD | PLMETAL | PLARABLE | PLDILYTH),
	    KLI, 80000, 20000, 0, 0, 0, "Klingus", 7,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{21, KLI | PLCORE, KLI, 88000, 12000, 0, 0, 0, "Pollux", 6,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{22, KLI | PLCORE, KLI, 69000, 31000, 0, 0, 0, "Scorpii", 7,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{23, KLI | PLCORE, KLI, 73000, 5000, 0, 0, 0, "Castor", 6,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{24, KLI, KLI, 70000, 40000, 0, 0, 0, "Pleiades V", 10,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{25, KLI, KLI, 60000, 10000, 0, 0, 0, "Andromeda", 9,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{26, KLI, KLI, 54000, 40000, 0, 0, 0, "Lalande", 7,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{27, KLI, KLI, 90000, 37000, 0, 0, 0, "Lyrae", 5,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{28, KLI, KLI, 83000, 48000, 0, 0, 0, "Mira", 4,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{29, KLI, KLI, 54000, 21000, 0, 0, 0, "Cygni", 5,
	(FED | ROM | ORI), 0, 0, 0, 30, 0, KLI},
	{30, (ORI | PLHOME | PLCORE | PLFUEL | PLREPAIR | PLSHIPYARD | PLMETAL | PLARABLE | PLDILYTH),
	    ORI, 80000, 80000, 0, 0, 0, "Orion", 5,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{31, ORI | PLCORE, ORI, 72000, 69000, 0, 0, 0, "Procyon", 7,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{32, ORI | PLCORE, ORI, 91000, 94000, 0, 0, 0, "Ursae Majoris", 13,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{33, ORI | PLCORE, ORI, 85000, 70000, 0, 0, 0, "Antares", 7,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{34, ORI, ORI, 92000, 59000, 0, 0, 0, "Cassiopia", 9,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{35, ORI, ORI, 65000, 55000, 0, 0, 0, "El Nath", 7,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{36, ORI, ORI, 52000, 60000, 0, 0, 0, "Spica", 5,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{37, ORI, ORI, 64000, 80000, 0, 0, 0, "Polaris", 7,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{38, ORI, ORI, 56000, 89000, 0, 0, 0, "Arcturus", 8,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{39, ORI, ORI, 70000, 93000, 0, 0, 0, "Herculis", 8,
	(FED | ROM | KLI), 0, 0, 0, 30, 0, ORI},
	{40, PLSTAR, NOBODY, 50000, 50000, 0, 0, 0, "Cogadh Dearg", 12,
	(FED | ROM | KLI | ORI), 0, 0, 0, 0, 0, 0}
    };
    for (i = 0; i < NUMPLANETS; i++) {
	for (j = 0; j < MAXTEAM + 1; j++) {
	    pdata[i].pl_tinfo[j].owner = pdata[i].pl_owner;
	    pdata[i].pl_tinfo[j].armies = pdata[i].pl_armies;
	    pdata[i].pl_tinfo[j].flags = pdata[i].pl_flags;
	    pdata[i].pl_tinfo[j].timestamp = 0;
	}
	pdata[i].pl_trevolt = 0;
	pdata[i].pl_next = 0;
	pdata[i].pl_prev = 0;
	pdata[i].pl_gridnum = 0;
        pdata[i].pl_flags |= PLPARADISE;
    }
    memcpy((char *) planets, (char *) pdata, MAXPLANETS * sizeof(struct planet));
}
/*-------------------------------GENRESOURCES----------------------------*/
/*  This function goes through the planets structure and determines what
kind of atmosphere and what kind of surface the planets have.  It generates
the stars that will be used as system centers ans then places atmospheres
on the other planets.  It then distributes the resources on the planet
surfaces.  This version's been bronco-ified. :)*/

static void 
genresources_bronco(void)
{
    int     i, j;		/* looping vars */
    int     t;			/* temp var */

    for (i = SYSTEMS; i < NUMPLANETS; i++) {	/* generate atmospheres */
	t = lrand48() % 100;	/* random # 0-99 */
	if (t < PATMOS1)	/* is it atmosphere type 1 */
	    planets[i].pl_flags |= PLATYPE1;
	else if (t < PATMOS2)	/* is it atmosphere type 2 */
	    planets[i].pl_flags |= PLATYPE2;
	else if (t < PATMOS3)	/* is it atmosphere type 3 */
	    planets[i].pl_flags |= PLATYPE3;
	else if (t < PPOISON)	/* is it poison atmosphere */
	    planets[i].pl_flags |= PLPOISON;
    }
    for (i = 0; i < NMETAL; i++) {	/* place the metal deposits */
	t = lrand48() % ((NUMPLANETS - SYSTEMS) / 4) + SYSTEMS +
	    ((i / (NMETAL / 4)) * ((NUMPLANETS - SYSTEMS) / 4));
	if (!(planets[t].pl_flags & PLMETAL & PLSURMASK)) {
	    planets[t].pl_flags |= PLMETAL;	/* OR in the metal flag */
	    planets[t].pl_flags |= PLREPAIR;
	}
	else
	    i--;
    }
    for (i = 0; i < NDILYTH; i++) {	/* place the crystals */
	t = lrand48() % ((NUMPLANETS - SYSTEMS) / 4) + SYSTEMS +
	    ((i / (NDILYTH / 4)) * ((NUMPLANETS - SYSTEMS) / 4));
	if (!(planets[t].pl_flags & PLDILYTH & PLSURMASK)) {
	    planets[t].pl_flags |= PLDILYTH;	/* OR in the dilyth flag */
	    planets[t].pl_flags |= PLFUEL;
	}
	else
	    i--;
    }
    for (i = 0; i < NARABLE; i++) {	/* place the farms */
	t = lrand48() % ((NUMPLANETS - SYSTEMS) / 4) + SYSTEMS +
	    ((i / (NARABLE / 4)) * ((NUMPLANETS - SYSTEMS) / 4));
	if (!(planets[t].pl_flags & PLARABLE & PLSURMASK) &&
	    !(planets[t].pl_flags & PLHOME)) {
	    planets[t].pl_flags |= PLARABLE | PLATYPE1;	/* OR in the arable flag */
	    planets[t].pl_flags |= PLAGRI;
	}
	else
	    i--;
    }
    for (i = 0; i < NUMPLANETS; i++)
	for (j = 0; j < MAXTEAM + 1; j++)
	    if (j == planets[i].pl_owner)
		planets[i].pl_tinfo[j].flags = planets[i].pl_flags;
}



/* Generate a complete galaxy.
   Uses ye old bronco planet setup.
   */

void 
#ifdef LOADABLE_PLGEN
gen_galaxy(void)
#else
gen_galaxy_8(void)
#endif
{
    GWIDTH = 100000;
    NUMPLANETS = 41;
    initbronco();		/* initialize planet structures */
    genresources_bronco();	/* place the resources */
    return;
}


#ifdef LOADABLE_PLGEN
int
galaxy_type(void)
{
  return(3);
}
#endif
