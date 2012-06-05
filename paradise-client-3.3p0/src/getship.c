/*
 * getship.c for client of socket protocol.
 *
 * This file has been mangled so it only sets the ship characteristics needed.
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "str.h"

#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/* Prototypes */
static void getship_default P((struct ship * shipp, int s_type));

void
init_shiptypes(void)
{
    int     i;
    struct shiplist *temp;

    /* start at -1, the default shiptype */
    for (i = -1; i < nshiptypes; i++) {
	temp = (struct shiplist *) malloc(sizeof(struct shiplist));
	temp->ship = (struct ship *) malloc(sizeof(struct ship));
	getship_default(temp->ship, i);
	temp->next = shiptypes;
	if (temp->next)
	    temp->next->prev = temp;
	temp->prev = NULL;
	shiptypes = temp;
    }
}

void
init_galaxy_class(void)
{
}

/* now returns a pointer to where the ship data is located.  This way
   if the data is later changed by the server everybody gets updated.
   Plus as a bonus it's more efficient :)   [Bill Dyess] */
struct ship *
getship(int s_type)
{
    struct shiplist *temp, *new;

    temp = shiptypes;
    while (temp) {
	if (temp->ship->s_type == s_type) {
	    return temp->ship;
	}
	temp = temp->next;
    }
    /*
       ok, that shiptype is unheard of.  Assume a new shiptype, and get the
       values for CA.  Also add the ship to the list so if it gets updated by
       the server later everyone stays happy.  [Bill Dyess]
    */
    printf("Error:  getship of unknown ship type %d, using CA defaults\n", s_type);
    temp = shiptypes;
    while (temp) {
	if (temp->ship->s_type == DEFAULT) {
	    printf("Adding ship type %d\n", s_type);
	    /* now add the new ship to the list */
	    new = (struct shiplist *) malloc(sizeof(struct shiplist));
	    new->ship = (struct ship *) malloc(sizeof(struct ship));
	    new->next = shiptypes;
	    new->prev = NULL;
	    if (shiptypes)
		shiptypes->prev = new;
	    shiptypes = new;
	    memmove(new->ship, temp->ship, sizeof(struct ship));
	    new->ship->s_type = s_type;
	    return new->ship;
	}
	temp = temp->next;
    }
    return temp->ship;

}

/* fill in ship characteristics */

static void
getship_default(struct ship *shipp, int s_type)
{
    switch (s_type) {
    case SCOUT:
    case PUCK:
	shipp->s_torpspeed = 16;
	shipp->s_phaserrange = 4500;
	shipp->s_maxspeed = 12;
	shipp->s_maxfuel = 5000;
	shipp->s_maxarmies = 2;
	shipp->s_maxshield = 75;
	shipp->s_maxdamage = 75;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1000;
	if(s_type == PUCK) {
	  shipp->s_type = PUCK;
	  shipp->s_letter = 'k';
	  shipp->s_desig[0] = 'P';
	  shipp->s_desig[1] = 'U';
	  shipp->s_bitmap = PUCK;
	} else {
	  shipp->s_type = SCOUT;
	  shipp->s_letter = 's';
	  shipp->s_desig[0] = 'S';
	  shipp->s_desig[1] = 'C';
	  shipp->s_bitmap = SCOUT;
	}
	break;
    case DESTROYER:
	shipp->s_type = DESTROYER;
	shipp->s_torpspeed = 14;
	shipp->s_phaserrange = 5100;
	shipp->s_maxspeed = 10;
	shipp->s_maxfuel = 7000;
	shipp->s_maxarmies = 5;
	shipp->s_maxshield = 85;
	shipp->s_maxdamage = 85;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1000;
	shipp->s_letter = 'd';
	shipp->s_desig[0] = 'D';
	shipp->s_desig[1] = 'D';
	shipp->s_bitmap = DESTROYER;
	break;
    default:
    case DEFAULT:
    case CRUISER:
	shipp->s_type = s_type;
	shipp->s_torpspeed = 12;
	shipp->s_phaserrange = 6000;
	shipp->s_maxspeed = 9;
	shipp->s_maxfuel = 10000;
	shipp->s_maxarmies = 10;
	shipp->s_maxshield = 100;
	shipp->s_maxdamage = 100;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1000;
	shipp->s_letter = 'c';
	shipp->s_desig[0] = 'C';
	shipp->s_desig[1] = 'A';
	shipp->s_bitmap = CRUISER;
	break;
    case BATTLESHIP:
	shipp->s_type = BATTLESHIP;
	shipp->s_torpspeed = 12;
	shipp->s_phaserrange = 6300;
	shipp->s_maxspeed = 8;
	shipp->s_maxfuel = 14000;
	shipp->s_maxarmies = 6;
	shipp->s_maxshield = 130;
	shipp->s_maxdamage = 130;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1000;
	shipp->s_letter = 'b';
	shipp->s_desig[0] = 'B';
	shipp->s_desig[1] = 'B';
	shipp->s_bitmap = BATTLESHIP;
	break;
    case ASSAULT:
	shipp->s_type = ASSAULT;
	shipp->s_torpspeed = 16;
	shipp->s_phaserrange = 4800;
	shipp->s_maxspeed = 8;
	shipp->s_maxfuel = 6000;
	shipp->s_maxarmies = 20;
	shipp->s_maxshield = 80;
	shipp->s_maxdamage = 200;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1200;
	shipp->s_letter = 'a';
	shipp->s_desig[0] = 'A';
	shipp->s_desig[1] = 'S';
	shipp->s_bitmap = ASSAULT;
	break;
    case STARBASE:
	shipp->s_type = STARBASE;
	shipp->s_torpspeed = 14;
	shipp->s_phaserrange = 7200;
	shipp->s_maxspeed = 2;
	shipp->s_maxfuel = 60000;
	shipp->s_maxarmies = 25;
	shipp->s_maxshield = 500;
	shipp->s_maxdamage = 600;
	shipp->s_maxwpntemp = 1300;
	shipp->s_maxegntemp = 1000;
	shipp->s_letter = 'o';
	shipp->s_desig[0] = 'S';
	shipp->s_desig[1] = 'B';
	shipp->s_bitmap = STARBASE;
	break;
    case ATT:			/* or GALAXY */
	if (paradise) {
	    shipp->s_type = ATT;
	    shipp->s_torpspeed = 20;
	    shipp->s_phaserrange = 6000;
	    shipp->s_maxspeed = 90;
	    shipp->s_maxfuel = 60000;
	    shipp->s_maxarmies = 1000;
	    shipp->s_maxshield = 30000;
	    shipp->s_maxdamage = 30000;
	    shipp->s_maxwpntemp = 10000;
	    shipp->s_maxegntemp = 10000;
	    shipp->s_letter = 'X';
	    shipp->s_desig[0] = 'A';
	    shipp->s_desig[1] = 'T';
	    shipp->s_bitmap = ATT;
	} else {
	    shipp->s_type = GALAXY;
	    shipp->s_torpspeed = 13;
	    shipp->s_phaserrange = 6000;	/* this is a guess */
	    shipp->s_maxspeed = 9;
	    shipp->s_maxfuel = 12000;
	    shipp->s_maxarmies = 12;
	    shipp->s_maxshield = 140;
	    shipp->s_maxdamage = 120;
	    shipp->s_maxwpntemp = 1000;
	    shipp->s_maxegntemp = 1000;
	    shipp->s_letter = 'g';
	    shipp->s_desig[0] = 'G';
	    shipp->s_desig[1] = 'A';
	    shipp->s_bitmap = FLAGSHIP;
	}
	break;
    case FLAGSHIP:
	shipp->s_type = FLAGSHIP;
	shipp->s_torpspeed = 14;
	shipp->s_phaserrange = 5750;
	shipp->s_maxspeed = 9;
	shipp->s_maxfuel = 14500;
	shipp->s_maxarmies = 8;
	shipp->s_maxshield = 115;
	shipp->s_maxdamage = 115;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1500;
	shipp->s_letter = 'f';
	shipp->s_desig[0] = 'F';
	shipp->s_desig[1] = 'L';
	shipp->s_bitmap = FLAGSHIP;
	break;
    case JUMPSHIP:
	shipp->s_type = JUMPSHIP;
	shipp->s_torpspeed = 18;
	shipp->s_phaserrange = 3000;
	shipp->s_maxspeed = 20;
	shipp->s_maxfuel = 50000;
	shipp->s_maxarmies = 0;
	shipp->s_maxshield = 5;
	shipp->s_maxdamage = 60;
	shipp->s_maxwpntemp = 1300;
	shipp->s_maxegntemp = 5000;
	shipp->s_letter = 'j';
	shipp->s_desig[0] = 'J';
	shipp->s_desig[1] = 'S';
	shipp->s_bitmap = JUMPSHIP;
	break;
    case WARBASE:
	shipp->s_type = WARBASE;
	shipp->s_torpspeed = 15;
	shipp->s_phaserrange = 6000;
	shipp->s_maxspeed = 3;
	shipp->s_maxfuel = 50000;
	shipp->s_maxarmies = 0;
	shipp->s_maxshield = 150;
	shipp->s_maxdamage = 500;
	shipp->s_maxwpntemp = 1500;
	shipp->s_maxegntemp = 1000;
	shipp->s_letter = 'w';
	shipp->s_desig[0] = 'W';
	shipp->s_desig[1] = 'B';
	shipp->s_bitmap = WARBASE;
	break;

    case LIGHTCRUISER:
	shipp->s_type = LIGHTCRUISER;
	shipp->s_torpspeed = 13;
	shipp->s_phaserrange = 6000;
	shipp->s_maxspeed = 10;
	shipp->s_maxfuel = 9000;
	shipp->s_maxarmies = 4;
	shipp->s_maxshield = 95;
	shipp->s_maxdamage = 90;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1500;
	shipp->s_letter = 'l';
	shipp->s_desig[0] = 'C';
	shipp->s_desig[1] = 'L';
	shipp->s_bitmap = LIGHTCRUISER;
	break;

    case CARRIER:
	shipp->s_type = CARRIER;
	shipp->s_torpspeed = 10;
	shipp->s_phaserrange = 4500;
	shipp->s_maxspeed = 10;
	shipp->s_maxfuel = 15000;
	shipp->s_maxarmies = 3;
	shipp->s_maxshield = 100;
	shipp->s_maxdamage = 150;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1000;
	shipp->s_letter = 'v';
	shipp->s_desig[0] = 'C';
	shipp->s_desig[1] = 'V';
	shipp->s_bitmap = CARRIER;
	break;
    case UTILITY:
	shipp->s_type = UTILITY;
	shipp->s_torpspeed = 16;
	shipp->s_phaserrange = 5000;
	shipp->s_maxspeed = 7;
	shipp->s_maxfuel = 12000;
	shipp->s_maxarmies = 12;
	shipp->s_maxshield = 110;
	shipp->s_maxdamage = 180;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1800;
	shipp->s_letter = 'u';
	shipp->s_desig[0] = 'U';
	shipp->s_desig[1] = 'T';
	shipp->s_bitmap = UTILITY;
	break;

    case PATROL:
	shipp->s_type = PATROL;
	shipp->s_torpspeed = 15;
	shipp->s_phaserrange = 5000;
	shipp->s_maxspeed = 11;
	shipp->s_maxfuel = 4000;
	shipp->s_maxarmies = 1;
	shipp->s_maxshield = 50;
	shipp->s_maxdamage = 40;
	shipp->s_maxwpntemp = 1000;
	shipp->s_maxegntemp = 1500;
	shipp->s_letter = 'p';
	shipp->s_desig[0] = 'P';
	shipp->s_desig[1] = 'T';
	shipp->s_bitmap = PATROL;
	break;
    }
    buildShipKeymap(shipp);
}
