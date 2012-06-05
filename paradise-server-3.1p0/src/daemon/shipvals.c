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


 /* This defines the core flags for a normal ship */
#define SFNCORE (SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP | SFNHASPHASERS)


/*-------------------------------INTERNAL FUNCTONS------------------------*/

/*---------------------------------GETSHIPDEFAULTS------------------------*/
/*  This function loads the shipvals array with the default values for the
ships.  They can later be changed with the sysdefaults.  */


void 
getshipdefaults(void)
{
    int     i;

    for (i = 0; i < NUM_TYPES; i++)
	shipvals[i].s_type = i;

    /* comprehensive definition of SCOUT */
    shipvals[SCOUT].s_alttype = 0;
    strcpy(shipvals[SCOUT].s_name, "Scout");
    shipvals[SCOUT].s_turns = 570000;
    shipvals[SCOUT].s_imp.acc = 200;
    shipvals[SCOUT].s_imp.dec = 300;	/* was: 270; (BG) */
    shipvals[SCOUT].s_imp.cost = 2;
    shipvals[SCOUT].s_imp.maxspeed = 12;
    shipvals[SCOUT].s_imp.etemp = 1000;
    shipvals[SCOUT].s_after.acc = 700;
    shipvals[SCOUT].s_after.dec = 270;
    shipvals[SCOUT].s_after.cost = 40;
    shipvals[SCOUT].s_after.maxspeed = 14;
    shipvals[SCOUT].s_after.etemp = 35000;
    if (configvals->bronco_shipvals) {
	shipvals[SCOUT].s_warp.acc = 10000;
	shipvals[SCOUT].s_warp.dec = 200;
	shipvals[SCOUT].s_warp.cost = 14;
	shipvals[SCOUT].s_warp.maxspeed = 19;
	shipvals[SCOUT].s_warp.etemp = 9000;
	shipvals[SCOUT].s_warpinitcost = 909;
	shipvals[SCOUT].s_warpinittime = 30;
	shipvals[SCOUT].s_warpprepspeed = 2;
    }
    else {
	shipvals[SCOUT].s_warp.acc = 10000;
	shipvals[SCOUT].s_warp.dec = 200;
	shipvals[SCOUT].s_warp.cost = 13;	/* was: 14; (BG) */
	shipvals[SCOUT].s_warp.maxspeed = 32;	/* was: 27; (BG) */
	shipvals[SCOUT].s_warp.etemp = 9000;
	shipvals[SCOUT].s_warpinitcost = 909;
	shipvals[SCOUT].s_warpinittime = 30;
	shipvals[SCOUT].s_warpprepspeed = 3;
    }
    shipvals[SCOUT].s_mass = 1500;
    shipvals[SCOUT].s_tractstr = 2000;
    shipvals[SCOUT].s_tractrng = 0.7;
    shipvals[SCOUT].s_tractcost = 3;
    shipvals[SCOUT].s_tractetemp = 1000;
    shipvals[SCOUT].s_torp.damage = 25;
    shipvals[SCOUT].s_torp.speed = 16;
    shipvals[SCOUT].s_torp.cost = 175;
    shipvals[SCOUT].s_torp.fuse = 16;
    shipvals[SCOUT].s_torp.wtemp = 7;
    shipvals[SCOUT].s_torp.wtemp_halfarc = 32;
    shipvals[SCOUT].s_torp.wtemp_factor = 9;
    shipvals[SCOUT].s_torp.aux = 0;
    shipvals[SCOUT].s_phaser.damage = 75;
    shipvals[SCOUT].s_phaser.speed = 4500;
    shipvals[SCOUT].s_phaser.cost = 525;
    shipvals[SCOUT].s_phaser.fuse = 10;
    shipvals[SCOUT].s_phaser.wtemp = 52;
    shipvals[SCOUT].s_missile.damage = 0;
    shipvals[SCOUT].s_missile.speed = 0;
    shipvals[SCOUT].s_missile.cost = 0;
    shipvals[SCOUT].s_missile.fuse = 0;
    shipvals[SCOUT].s_missile.wtemp = 0;
    shipvals[SCOUT].s_missile.count = 0;
    shipvals[SCOUT].s_missile.aux = 0;
    shipvals[SCOUT].s_missilestored = 0;
    shipvals[SCOUT].s_plasma.damage = -1;
    shipvals[SCOUT].s_plasma.speed = 0;
    shipvals[SCOUT].s_plasma.cost = 0;
    shipvals[SCOUT].s_plasma.fuse = 0;
    shipvals[SCOUT].s_plasma.wtemp = 50;
    shipvals[SCOUT].s_plasma.aux = 0;
    shipvals[SCOUT].s_maxwpntemp = 1000;
    shipvals[SCOUT].s_wpncoolrate = 3;
    if (configvals->bronco_shipvals)
	shipvals[SCOUT].s_maxegntemp = 1000;
    else
	shipvals[SCOUT].s_maxegntemp = 1500;
    shipvals[SCOUT].s_egncoolrate = 8;
    shipvals[SCOUT].s_maxfuel = 5000;
    shipvals[SCOUT].s_recharge = 16;
    shipvals[SCOUT].s_mingivefuel = 0;
    shipvals[SCOUT].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[SCOUT].s_expldam = 35;	/* was: 40; (BG) */
	shipvals[SCOUT].s_fueldam = 50;	/* was: 45; (BG) */
    }
    else {
	shipvals[SCOUT].s_expldam = 75;
	shipvals[SCOUT].s_fueldam = 0;
    }
    shipvals[SCOUT].s_armyperkill = 2;
    shipvals[SCOUT].s_maxarmies = 2;
    if (configvals->bronco_shipvals)
    {
	shipvals[SCOUT].s_bomb = 10;
	shipvals[SCOUT].s_bombflags = SBOMB_ARMIES;
    }
    else
    {
	shipvals[SCOUT].s_bomb = 0;
	shipvals[SCOUT].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    }
    shipvals[SCOUT].s_repair = 80;
    shipvals[SCOUT].s_maxdamage = 75;
    shipvals[SCOUT].s_maxshield = 75;
    shipvals[SCOUT].s_shieldcost = 2;
    shipvals[SCOUT].s_detcost = 100;
    shipvals[SCOUT].s_detdist = 1750;
    if (configvals->bronco_shipvals) {
	shipvals[SCOUT].s_cloakcost = 85;
	shipvals[SCOUT].s_scanrange = -1;
    }
    else {
	shipvals[SCOUT].s_cloakcost = 50;
	shipvals[SCOUT].s_scanrange = 5000;
    }
    shipvals[SCOUT].s_numports = 0;
    shipvals[SCOUT].s_letter = 's';
    shipvals[SCOUT].s_desig1 = 'S';
    shipvals[SCOUT].s_desig2 = 'C';
    shipvals[SCOUT].s_bitmap = 0;
    shipvals[SCOUT].s_width = 20;
    shipvals[SCOUT].s_height = 20;
    shipvals[SCOUT].s_timer = 0;
    shipvals[SCOUT].s_maxnum = 32;
    shipvals[SCOUT].s_rank = 0;
    shipvals[SCOUT].s_numdefn = 0;
    shipvals[SCOUT].s_numplan = 0;
    if (configvals->warpdrive)
	shipvals[SCOUT].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP |  SFNHASPHASERS;
    else
	shipvals[SCOUT].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL |  SFNHASPHASERS;


    /* comprehensive definition of DESTROYER */
    shipvals[DESTROYER].s_alttype = 1;
    strcpy(shipvals[DESTROYER].s_name, "Destroyer");
    shipvals[DESTROYER].s_turns = 310000;
    shipvals[DESTROYER].s_imp.acc = 200;
    shipvals[DESTROYER].s_imp.dec = 300;
    shipvals[DESTROYER].s_imp.cost = 3;
    shipvals[DESTROYER].s_imp.maxspeed = 10;
    shipvals[DESTROYER].s_imp.etemp = 1000;
    shipvals[DESTROYER].s_after.acc = 700;
    shipvals[DESTROYER].s_after.dec = 270;
    shipvals[DESTROYER].s_after.cost = 100;
    shipvals[DESTROYER].s_after.maxspeed = 12;
    shipvals[DESTROYER].s_after.etemp = 40000;
    if (configvals->bronco_shipvals) {
	shipvals[DESTROYER].s_warp.acc = 10000;
	shipvals[DESTROYER].s_warp.dec = 300;
	shipvals[DESTROYER].s_warp.cost = 22;
	shipvals[DESTROYER].s_warp.maxspeed = 15;
	shipvals[DESTROYER].s_warp.etemp = 8000;
	shipvals[DESTROYER].s_warpinitcost = 1272;
	shipvals[DESTROYER].s_warpinittime = 50;
	shipvals[DESTROYER].s_warpprepspeed = 2;
    }
    else {
	shipvals[DESTROYER].s_warp.acc = 10000;
	shipvals[DESTROYER].s_warp.dec = 300;
	shipvals[DESTROYER].s_warp.cost = 21;	/* was: 22; (BG) */
	shipvals[DESTROYER].s_warp.maxspeed = 27;	/* was: 22; (BG) */
	shipvals[DESTROYER].s_warp.etemp = 8000;
	shipvals[DESTROYER].s_warpinitcost = 1272;
	shipvals[DESTROYER].s_warpinittime = 50;
	shipvals[DESTROYER].s_warpprepspeed = 3;
    }
    shipvals[DESTROYER].s_mass = 1800;
    shipvals[DESTROYER].s_tractstr = 2500;
    shipvals[DESTROYER].s_tractrng = 0.9;
    shipvals[DESTROYER].s_tractcost = 4;
    shipvals[DESTROYER].s_tractetemp = 1000;
    shipvals[DESTROYER].s_torp.damage = 30;
    shipvals[DESTROYER].s_torp.speed = 14;
    shipvals[DESTROYER].s_torp.cost = 210;
    shipvals[DESTROYER].s_torp.fuse = 30;
    shipvals[DESTROYER].s_torp.wtemp = 11;
    shipvals[DESTROYER].s_torp.wtemp_halfarc = 32;
    shipvals[DESTROYER].s_torp.wtemp_factor = 9;
    shipvals[DESTROYER].s_torp.aux = 0;
    shipvals[DESTROYER].s_phaser.damage = 85;
    shipvals[DESTROYER].s_phaser.speed = 5100;
    shipvals[DESTROYER].s_phaser.cost = 595;
    shipvals[DESTROYER].s_phaser.fuse = 10;
    shipvals[DESTROYER].s_phaser.wtemp = 59;
    shipvals[DESTROYER].s_missile.damage = 20;
    shipvals[DESTROYER].s_missile.speed = 8;
    shipvals[DESTROYER].s_missile.cost = 900;
    shipvals[DESTROYER].s_missile.fuse = 100;
    shipvals[DESTROYER].s_missile.wtemp = 100;
    shipvals[DESTROYER].s_missile.count = 2;
    shipvals[DESTROYER].s_missile.aux = 2;
    shipvals[DESTROYER].s_missilestored = 8;
    shipvals[DESTROYER].s_plasma.damage = 75;
    shipvals[DESTROYER].s_plasma.speed = 15;
    shipvals[DESTROYER].s_plasma.cost = 2250;
    shipvals[DESTROYER].s_plasma.fuse = 30;
    shipvals[DESTROYER].s_plasma.wtemp = 217;
    shipvals[DESTROYER].s_plasma.aux = 1;
    shipvals[DESTROYER].s_maxwpntemp = 1000;
    shipvals[DESTROYER].s_wpncoolrate = 2;
    if (configvals->bronco_shipvals)
	shipvals[DESTROYER].s_maxegntemp = 1000;
    else
	shipvals[DESTROYER].s_maxegntemp = 1500;
    shipvals[DESTROYER].s_egncoolrate = 7;
    shipvals[DESTROYER].s_maxfuel = 7000;
    shipvals[DESTROYER].s_recharge = 22;
    shipvals[DESTROYER].s_mingivefuel = 0;
    shipvals[DESTROYER].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[DESTROYER].s_expldam = 65;	/* was: 50; (BG) */
	shipvals[DESTROYER].s_fueldam = 45;	/* was: 50; (BG) */
    }
    else {
	shipvals[DESTROYER].s_expldam = 100;
	shipvals[DESTROYER].s_fueldam = 0;
    }
    shipvals[DESTROYER].s_armyperkill = 2;
    shipvals[DESTROYER].s_maxarmies = 4;
    if (configvals->bronco_shipvals)
    {
	shipvals[DESTROYER].s_bomb = 10;
	shipvals[DESTROYER].s_bombflags = SBOMB_ARMIES;
    }
    else
    {
	shipvals[DESTROYER].s_bomb = 5;
	shipvals[DESTROYER].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    }
    shipvals[DESTROYER].s_repair = 100;
    shipvals[DESTROYER].s_maxdamage = 85;
    shipvals[DESTROYER].s_maxshield = 85;
    shipvals[DESTROYER].s_shieldcost = 3;
    shipvals[DESTROYER].s_detcost = 100;
    shipvals[DESTROYER].s_detdist = 1750;
    if (configvals->bronco_shipvals) {
	shipvals[DESTROYER].s_cloakcost = 105;
	shipvals[DESTROYER].s_scanrange = -1;
    }
    else {
	shipvals[DESTROYER].s_cloakcost = 75;
	shipvals[DESTROYER].s_scanrange = 1000;
    }
    shipvals[DESTROYER].s_numports = 0;
    shipvals[DESTROYER].s_letter = 'd';
    shipvals[DESTROYER].s_desig1 = 'D';
    shipvals[DESTROYER].s_desig2 = 'D';
    shipvals[DESTROYER].s_bitmap = 1;
    shipvals[DESTROYER].s_width = 20;
    shipvals[DESTROYER].s_height = 20;
    shipvals[DESTROYER].s_timer = 0;
    shipvals[DESTROYER].s_maxnum = 32;
    shipvals[DESTROYER].s_rank = 0;
    shipvals[DESTROYER].s_numdefn = 0;
    shipvals[DESTROYER].s_numplan = 0;
    if (configvals->warpdrive)
	shipvals[DESTROYER].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP |  SFNHASPHASERS;
    else
	shipvals[DESTROYER].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL |  SFNHASPHASERS;

    /* comprehensive definition of CRUISER */
    shipvals[CRUISER].s_alttype = 2;
    strcpy(shipvals[CRUISER].s_name, "Cruiser");
    shipvals[CRUISER].s_turns = 170000;
    shipvals[CRUISER].s_imp.acc = 150;
    shipvals[CRUISER].s_imp.dec = 200;
    shipvals[CRUISER].s_imp.cost = 4;
    shipvals[CRUISER].s_imp.maxspeed = 9;
    shipvals[CRUISER].s_imp.etemp = 1000;
    shipvals[CRUISER].s_after.acc = 550;
    shipvals[CRUISER].s_after.dec = 270;
    shipvals[CRUISER].s_after.cost = 200;
    shipvals[CRUISER].s_after.maxspeed = 11;
    shipvals[CRUISER].s_after.etemp = 50000;
    if (configvals->bronco_shipvals) {
	shipvals[CRUISER].s_warp.acc = 10000;
	shipvals[CRUISER].s_warp.dec = 500;
	shipvals[CRUISER].s_warp.cost = 25;
	shipvals[CRUISER].s_warp.maxspeed = 13;
	shipvals[CRUISER].s_warp.etemp = 7000;
	shipvals[CRUISER].s_warpinitcost = 1818;
	shipvals[CRUISER].s_warpinittime = 60;
	shipvals[CRUISER].s_warpprepspeed = 1;
    }
    else {
	shipvals[CRUISER].s_warp.acc = 10000;
	shipvals[CRUISER].s_warp.dec = 500;
	shipvals[CRUISER].s_warp.cost = 24;	/* was: 25; (BG) */
	shipvals[CRUISER].s_warp.maxspeed = 24;	/* was: 19; (BG) */
	shipvals[CRUISER].s_warp.etemp = 7000;
	shipvals[CRUISER].s_warpinitcost = 1818;
	shipvals[CRUISER].s_warpinittime = 60;
	shipvals[CRUISER].s_warpprepspeed = 2;
    }
    shipvals[CRUISER].s_mass = 2000;
    shipvals[CRUISER].s_tractstr = 3000;
    shipvals[CRUISER].s_tractrng = 1.0;
    shipvals[CRUISER].s_tractcost = 4;
    shipvals[CRUISER].s_tractetemp = 1000;
    shipvals[CRUISER].s_torp.damage = 40;
    shipvals[CRUISER].s_torp.speed = 12;
    shipvals[CRUISER].s_torp.cost = 280;
    shipvals[CRUISER].s_torp.fuse = 40;
    shipvals[CRUISER].s_torp.wtemp = 18;
    shipvals[CRUISER].s_torp.wtemp_halfarc = 32;
    shipvals[CRUISER].s_torp.wtemp_factor = 9;
    shipvals[CRUISER].s_torp.aux = 0;
    shipvals[CRUISER].s_phaser.damage = 100;
    shipvals[CRUISER].s_phaser.speed = 6000;
    shipvals[CRUISER].s_phaser.cost = 700;
    shipvals[CRUISER].s_phaser.fuse = 10;
    shipvals[CRUISER].s_phaser.wtemp = 70;
    shipvals[CRUISER].s_missile.damage = 25;
    shipvals[CRUISER].s_missile.speed = 7;
    shipvals[CRUISER].s_missile.cost = 900;
    shipvals[CRUISER].s_missile.fuse = 100;
    shipvals[CRUISER].s_missile.wtemp = 105;
    shipvals[CRUISER].s_missile.count = 3;
    shipvals[CRUISER].s_missile.aux = 2;
    shipvals[CRUISER].s_missilestored = 10;
    shipvals[CRUISER].s_plasma.damage = 100;
    shipvals[CRUISER].s_plasma.speed = 15;
    shipvals[CRUISER].s_plasma.cost = 3000;
    shipvals[CRUISER].s_plasma.fuse = 35;
    shipvals[CRUISER].s_plasma.wtemp = 292;
    shipvals[CRUISER].s_plasma.aux = 1;
    shipvals[CRUISER].s_maxwpntemp = 1000;
    shipvals[CRUISER].s_wpncoolrate = 2;
    if (configvals->bronco_shipvals)
	shipvals[CRUISER].s_maxegntemp = 1000;
    else
	shipvals[CRUISER].s_maxegntemp = 1500;
    shipvals[CRUISER].s_egncoolrate = 6;
    shipvals[CRUISER].s_maxfuel = 10000;
    shipvals[CRUISER].s_recharge = 24;
    shipvals[CRUISER].s_mingivefuel = 0;
    shipvals[CRUISER].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[CRUISER].s_expldam = 75;	/* was: 50; (BG) */
	shipvals[CRUISER].s_fueldam = 40;	/* was: 65; (BG) */
    }
    else {
	shipvals[CRUISER].s_expldam = 100;
	shipvals[CRUISER].s_fueldam = 0;
    }
    shipvals[CRUISER].s_armyperkill = 2;
    shipvals[CRUISER].s_maxarmies = 6;
    shipvals[CRUISER].s_bomb = 10;
    if (configvals->bronco_shipvals)
        shipvals[CRUISER].s_bombflags = SBOMB_ARMIES;
    else
        shipvals[CRUISER].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[CRUISER].s_repair = 110;
    shipvals[CRUISER].s_maxdamage = 100;
    shipvals[CRUISER].s_maxshield = 100;
    shipvals[CRUISER].s_shieldcost = 4;
    shipvals[CRUISER].s_detcost = 100;
    shipvals[CRUISER].s_detdist = 1750;
    if (configvals->bronco_shipvals)
	shipvals[CRUISER].s_cloakcost = 130;
    else
	shipvals[CRUISER].s_cloakcost = 100;
    shipvals[CRUISER].s_scanrange = -1;
    shipvals[CRUISER].s_numports = 0;
    shipvals[CRUISER].s_letter = 'c';
    shipvals[CRUISER].s_desig1 = 'C';
    shipvals[CRUISER].s_desig2 = 'A';
    shipvals[CRUISER].s_bitmap = 2;
    shipvals[CRUISER].s_width = 20;
    shipvals[CRUISER].s_height = 20;
    shipvals[CRUISER].s_timer = 0;
    shipvals[CRUISER].s_maxnum = 32;
    shipvals[CRUISER].s_rank = 0;
    shipvals[CRUISER].s_numdefn = 0;
    shipvals[CRUISER].s_numplan = 0;
    if (configvals->warpdrive)
	shipvals[CRUISER].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP |  SFNHASPHASERS;
    else
	shipvals[CRUISER].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL |  SFNHASPHASERS;

    /* comprehensive definition of BATTLESHIP */
    shipvals[BATTLESHIP].s_alttype = 3;
    strcpy(shipvals[BATTLESHIP].s_name, "Battleship");
    shipvals[BATTLESHIP].s_turns = 75000;
    shipvals[BATTLESHIP].s_imp.acc = 80;
    shipvals[BATTLESHIP].s_imp.dec = 180;
    shipvals[BATTLESHIP].s_imp.cost = 6;
    shipvals[BATTLESHIP].s_imp.maxspeed = 8;
    shipvals[BATTLESHIP].s_imp.etemp = 1000;
    shipvals[BATTLESHIP].s_after.acc = 500;
    shipvals[BATTLESHIP].s_after.dec = 270;
    shipvals[BATTLESHIP].s_after.cost = 100;
    shipvals[BATTLESHIP].s_after.maxspeed = 10;
    shipvals[BATTLESHIP].s_after.etemp = 50000;
    if (configvals->bronco_shipvals) {
	shipvals[BATTLESHIP].s_warp.acc = 10000;
	shipvals[BATTLESHIP].s_warp.dec = 500;
	shipvals[BATTLESHIP].s_warp.cost = 37;
	shipvals[BATTLESHIP].s_warp.maxspeed = 11;
	shipvals[BATTLESHIP].s_warp.etemp = 7000;
	shipvals[BATTLESHIP].s_warpinitcost = 2545;
	shipvals[BATTLESHIP].s_warpinittime = 70;
	shipvals[BATTLESHIP].s_warpprepspeed = 0;
    }
    else {
	shipvals[BATTLESHIP].s_warp.acc = 10000;
	shipvals[BATTLESHIP].s_warp.dec = 500;
	shipvals[BATTLESHIP].s_warp.cost = 35;	/* was: 37; (BG) */
	shipvals[BATTLESHIP].s_warp.maxspeed = 21;	/* was: 16; (BG) */
	shipvals[BATTLESHIP].s_warp.etemp = 7000;
	shipvals[BATTLESHIP].s_warpinitcost = 2545;
	shipvals[BATTLESHIP].s_warpinittime = 70;
	shipvals[BATTLESHIP].s_warpprepspeed = 1;
    }
    shipvals[BATTLESHIP].s_mass = 2300;
    shipvals[BATTLESHIP].s_tractstr = 3700;
    shipvals[BATTLESHIP].s_tractrng = 1.2;
    shipvals[BATTLESHIP].s_tractcost = 4;
    shipvals[BATTLESHIP].s_tractetemp = 1000;
    shipvals[BATTLESHIP].s_torp.damage = 40;
    shipvals[BATTLESHIP].s_torp.speed = 12;
    shipvals[BATTLESHIP].s_torp.cost = 300;
    shipvals[BATTLESHIP].s_torp.fuse = 40;
    shipvals[BATTLESHIP].s_torp.wtemp = 20;
    shipvals[BATTLESHIP].s_torp.wtemp_halfarc = 32;
    shipvals[BATTLESHIP].s_torp.wtemp_factor = 9;
    shipvals[BATTLESHIP].s_torp.aux = 0;
    shipvals[BATTLESHIP].s_phaser.damage = 105;
    shipvals[BATTLESHIP].s_phaser.speed = 6300;
    shipvals[BATTLESHIP].s_phaser.cost = 945;
    shipvals[BATTLESHIP].s_phaser.fuse = 10;
    shipvals[BATTLESHIP].s_phaser.wtemp = 94;
    shipvals[BATTLESHIP].s_missile.damage = 30;
    shipvals[BATTLESHIP].s_missile.speed = 5;
    shipvals[BATTLESHIP].s_missile.cost = 900;
    shipvals[BATTLESHIP].s_missile.fuse = 100;
    shipvals[BATTLESHIP].s_missile.wtemp = 100;
    shipvals[BATTLESHIP].s_missile.count = 4;
    shipvals[BATTLESHIP].s_missile.aux = 3;
    shipvals[BATTLESHIP].s_missilestored = 12;
    shipvals[BATTLESHIP].s_plasma.damage = 130;
    shipvals[BATTLESHIP].s_plasma.speed = 15;
    shipvals[BATTLESHIP].s_plasma.cost = 3900;
    shipvals[BATTLESHIP].s_plasma.fuse = 35;
    shipvals[BATTLESHIP].s_plasma.wtemp = 382;
    shipvals[BATTLESHIP].s_plasma.aux = 1;
    shipvals[BATTLESHIP].s_maxwpntemp = 1000;
    shipvals[BATTLESHIP].s_wpncoolrate = 3;
    if (configvals->bronco_shipvals)
	shipvals[BATTLESHIP].s_maxegntemp = 1000;
    else
	shipvals[BATTLESHIP].s_maxegntemp = 1500;
    shipvals[BATTLESHIP].s_egncoolrate = 6;
    shipvals[BATTLESHIP].s_maxfuel = 14000;
    shipvals[BATTLESHIP].s_recharge = 28;
    shipvals[BATTLESHIP].s_mingivefuel = 0;
    shipvals[BATTLESHIP].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[BATTLESHIP].s_expldam = 85;	/* was: 50; (BG) */
	shipvals[BATTLESHIP].s_fueldam = 35;	/* was: 85; (BG) */
    }
    else {
	shipvals[BATTLESHIP].s_expldam = 10;
	shipvals[BATTLESHIP].s_fueldam = 0;
    }
    shipvals[BATTLESHIP].s_armyperkill = 2;
    shipvals[BATTLESHIP].s_maxarmies = 6;
    shipvals[BATTLESHIP].s_bomb = 20;
    if (configvals->bronco_shipvals)
        shipvals[BATTLESHIP].s_bombflags = SBOMB_ARMIES;
    else
        shipvals[BATTLESHIP].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[BATTLESHIP].s_repair = 125;
    shipvals[BATTLESHIP].s_maxdamage = 130;
    shipvals[BATTLESHIP].s_maxshield = 130;
    shipvals[BATTLESHIP].s_shieldcost = 5;
    shipvals[BATTLESHIP].s_detcost = 100;
    shipvals[BATTLESHIP].s_detdist = 1750;
    shipvals[BATTLESHIP].s_cloakcost = 150;
    shipvals[BATTLESHIP].s_scanrange = -1;
    shipvals[BATTLESHIP].s_numports = 0;
    shipvals[BATTLESHIP].s_letter = 'b';
    shipvals[BATTLESHIP].s_desig1 = 'B';
    shipvals[BATTLESHIP].s_desig2 = 'B';
    shipvals[BATTLESHIP].s_bitmap = 3;
    shipvals[BATTLESHIP].s_width = 20;
    shipvals[BATTLESHIP].s_height = 20;
    shipvals[BATTLESHIP].s_timer = 0;
    shipvals[BATTLESHIP].s_maxnum = 32;
    shipvals[BATTLESHIP].s_rank = 0;
    shipvals[BATTLESHIP].s_numdefn = 0;
    shipvals[BATTLESHIP].s_numplan = 0;
    shipvals[BATTLESHIP].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP |  SFNHASPHASERS;

    /* comprehensive definition of ASSAULT */
    shipvals[ASSAULT].s_alttype = 4;
    strcpy(shipvals[ASSAULT].s_name, "Assault");
    shipvals[ASSAULT].s_turns = 120000;
    shipvals[ASSAULT].s_imp.acc = 100;
    shipvals[ASSAULT].s_imp.dec = 200;
    if (configvals->bronco_shipvals)
	shipvals[ASSAULT].s_imp.cost = 3;
    else
	shipvals[ASSAULT].s_imp.cost = 4;
    shipvals[ASSAULT].s_imp.maxspeed = 8;
    shipvals[ASSAULT].s_imp.etemp = 1000;
    shipvals[ASSAULT].s_after.acc = 550;
    shipvals[ASSAULT].s_after.dec = 270;
    shipvals[ASSAULT].s_after.cost = 25;
    shipvals[ASSAULT].s_after.maxspeed = 10;
    shipvals[ASSAULT].s_after.etemp = 50000;
    if (configvals->bronco_shipvals) {
	shipvals[ASSAULT].s_warp.acc = 10000;
	shipvals[ASSAULT].s_warp.dec = 5000;
	shipvals[ASSAULT].s_warp.cost = 20;
	shipvals[ASSAULT].s_warp.maxspeed = 12;
	shipvals[ASSAULT].s_warp.etemp = 6500;
	shipvals[ASSAULT].s_warpinitcost = 1000;
	shipvals[ASSAULT].s_warpinittime = 80;
	shipvals[ASSAULT].s_warpprepspeed = 0;
    }
    else {
	shipvals[ASSAULT].s_warp.acc = 10000;
	shipvals[ASSAULT].s_warp.dec = 5000;
	shipvals[ASSAULT].s_warp.cost = 20;
	shipvals[ASSAULT].s_warp.maxspeed = 23;	/* was: 18; (BG) */
	shipvals[ASSAULT].s_warp.etemp = 6500;
	shipvals[ASSAULT].s_warpinitcost = 1000;
	shipvals[ASSAULT].s_warpinittime = 80;
	shipvals[ASSAULT].s_warpprepspeed = 1;
    }
    shipvals[ASSAULT].s_mass = 2300;
    shipvals[ASSAULT].s_tractstr = 2500;
    shipvals[ASSAULT].s_tractrng = 0.7;
    shipvals[ASSAULT].s_tractcost = 3;
    shipvals[ASSAULT].s_tractetemp = 1000;
    shipvals[ASSAULT].s_torp.damage = 30;
    shipvals[ASSAULT].s_torp.speed = 16;
    shipvals[ASSAULT].s_torp.cost = 270;
    shipvals[ASSAULT].s_torp.fuse = 30;
    shipvals[ASSAULT].s_torp.wtemp = 17;
    shipvals[ASSAULT].s_torp.wtemp_halfarc = 32;
    shipvals[ASSAULT].s_torp.wtemp_factor = 9;
    shipvals[ASSAULT].s_torp.aux = 0;
    shipvals[ASSAULT].s_phaser.damage = 80;
    shipvals[ASSAULT].s_phaser.speed = 4800;
    shipvals[ASSAULT].s_phaser.cost = 560;
    shipvals[ASSAULT].s_phaser.fuse = 10;
    shipvals[ASSAULT].s_phaser.wtemp = 56;
    shipvals[ASSAULT].s_missile.damage = 0;
    shipvals[ASSAULT].s_missile.speed = 0;
    shipvals[ASSAULT].s_missile.cost = 0;
    shipvals[ASSAULT].s_missile.fuse = 0;
    shipvals[ASSAULT].s_missile.wtemp = 0;
    shipvals[ASSAULT].s_missile.count = 0;
    shipvals[ASSAULT].s_missile.aux = 0;
    shipvals[ASSAULT].s_missilestored = 0;
    shipvals[ASSAULT].s_plasma.damage = -1;
    shipvals[ASSAULT].s_plasma.speed = 0;
    shipvals[ASSAULT].s_plasma.cost = 0;
    shipvals[ASSAULT].s_plasma.fuse = 0;
    shipvals[ASSAULT].s_plasma.wtemp = 5;
    shipvals[ASSAULT].s_plasma.aux = 0;
    shipvals[ASSAULT].s_maxwpntemp = 1000;
    shipvals[ASSAULT].s_wpncoolrate = 2;
    if (configvals->bronco_shipvals)
	shipvals[ASSAULT].s_maxegntemp = 1000;
    else
	shipvals[ASSAULT].s_maxegntemp = 1700;
    shipvals[ASSAULT].s_egncoolrate = 6;
    shipvals[ASSAULT].s_maxfuel = 6000;
    if (configvals->bronco_shipvals)
	shipvals[ASSAULT].s_recharge = 20;
    else
	shipvals[ASSAULT].s_recharge = 24;
    shipvals[ASSAULT].s_mingivefuel = 0;
    shipvals[ASSAULT].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[ASSAULT].s_expldam = 70;	/* was: 50; (BG) */
	shipvals[ASSAULT].s_fueldam = 40;	/* was: 45; (BG) */
    }
    else {
	shipvals[ASSAULT].s_expldam = 100;
	shipvals[ASSAULT].s_fueldam = 0;
    }
    shipvals[ASSAULT].s_armyperkill = 3;
    shipvals[ASSAULT].s_maxarmies = 20;
    if (configvals->bronco_shipvals)
    {
	shipvals[ASSAULT].s_bomb = 25;
	shipvals[ASSAULT].s_bombflags = SBOMB_ARMIES;
    }
    else
    {
	shipvals[ASSAULT].s_bomb = 50;
	shipvals[ASSAULT].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    }
    shipvals[ASSAULT].s_repair = 120;
    shipvals[ASSAULT].s_maxdamage = 200;
    shipvals[ASSAULT].s_maxshield = 80;
    shipvals[ASSAULT].s_shieldcost = 3;
    shipvals[ASSAULT].s_detcost = 100;
    shipvals[ASSAULT].s_detdist = 1750;
    if (configvals->bronco_shipvals)
	shipvals[ASSAULT].s_cloakcost = 85;
    else
	shipvals[ASSAULT].s_cloakcost = 80;
    shipvals[ASSAULT].s_scanrange = -1;
    shipvals[ASSAULT].s_numports = 0;
    shipvals[ASSAULT].s_letter = 'a';
    shipvals[ASSAULT].s_desig1 = 'A';
    shipvals[ASSAULT].s_desig2 = 'S';
    shipvals[ASSAULT].s_bitmap = 4;
    shipvals[ASSAULT].s_width = 20;
    shipvals[ASSAULT].s_height = 20;
    shipvals[ASSAULT].s_timer = 0;
    shipvals[ASSAULT].s_maxnum = 32;
    shipvals[ASSAULT].s_rank = 0;
    shipvals[ASSAULT].s_numdefn = 0;
    shipvals[ASSAULT].s_numplan = 0;
    if (configvals->warpdrive)
	shipvals[ASSAULT].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP |  SFNHASPHASERS;
    else
	shipvals[ASSAULT].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL |  SFNHASPHASERS;

    /* comprehensive definition of STARBASE */
    shipvals[STARBASE].s_alttype = 5;
    strcpy(shipvals[STARBASE].s_name, "Starbase");
    shipvals[STARBASE].s_turns = 50000;
    shipvals[STARBASE].s_imp.acc = 100;
    shipvals[STARBASE].s_imp.dec = 200;
    if (configvals->bronco_shipvals)
	shipvals[STARBASE].s_imp.cost = 10;
    else
	shipvals[STARBASE].s_imp.cost = 4;
    shipvals[STARBASE].s_imp.maxspeed = 3;
    shipvals[STARBASE].s_imp.etemp = 1000;
    shipvals[STARBASE].s_after.acc = 100;
    shipvals[STARBASE].s_after.dec = 100;
    shipvals[STARBASE].s_after.cost = 40;
    shipvals[STARBASE].s_after.maxspeed = 5;
    shipvals[STARBASE].s_after.etemp = 30000;
    if (configvals->bronco_shipvals) {
	shipvals[STARBASE].s_warp.acc = 100;
	shipvals[STARBASE].s_warp.dec = 100;
	shipvals[STARBASE].s_warp.cost = 20;
	shipvals[STARBASE].s_warp.maxspeed = 4;
	shipvals[STARBASE].s_warp.etemp = 1500;
	shipvals[STARBASE].s_warpinitcost = 10909;
	shipvals[STARBASE].s_warpinittime = 100;
	shipvals[STARBASE].s_warpprepspeed = 1;
    }
    else {
	shipvals[STARBASE].s_warp.acc = 100;
	shipvals[STARBASE].s_warp.dec = 100;
	shipvals[STARBASE].s_warp.cost = 20;
	shipvals[STARBASE].s_warp.maxspeed = 6;
	shipvals[STARBASE].s_warp.etemp = 1500;
	shipvals[STARBASE].s_warpinitcost = 10909;
	shipvals[STARBASE].s_warpinittime = 100;
	shipvals[STARBASE].s_warpprepspeed = 2;
    }
    shipvals[STARBASE].s_mass = 5000;
    shipvals[STARBASE].s_tractstr = 8000;
    shipvals[STARBASE].s_tractrng = 1.5;
    shipvals[STARBASE].s_tractcost = 10;
    shipvals[STARBASE].s_tractetemp = 3000;
    shipvals[STARBASE].s_torp.damage = 30;
    shipvals[STARBASE].s_torp.speed = 14;
    shipvals[STARBASE].s_torp.cost = 300;
    if (configvals->bronco_shipvals)
	shipvals[STARBASE].s_torp.fuse = 30;
    else
	shipvals[STARBASE].s_torp.fuse = 45;
    shipvals[STARBASE].s_torp.wtemp = 20;
    shipvals[STARBASE].s_torp.wtemp_halfarc = 0;
    shipvals[STARBASE].s_torp.wtemp_factor = 0;
    shipvals[STARBASE].s_torp.aux = 0;
    shipvals[STARBASE].s_phaser.damage = 120;
    shipvals[STARBASE].s_phaser.speed = 7200;
    shipvals[STARBASE].s_phaser.wtemp = 96;
    shipvals[STARBASE].s_phaser.cost = 960;
    shipvals[STARBASE].s_phaser.fuse = 4;
    shipvals[STARBASE].s_missile.damage = 40;
    shipvals[STARBASE].s_missile.speed = 14;
    shipvals[STARBASE].s_missile.cost = 2000;
    shipvals[STARBASE].s_missile.fuse = 100;
    shipvals[STARBASE].s_missile.wtemp = 120;
    shipvals[STARBASE].s_missile.count = 4;
    shipvals[STARBASE].s_missile.aux = 2;
    shipvals[STARBASE].s_missilestored = -1;
    shipvals[STARBASE].s_plasma.damage = 150;
    shipvals[STARBASE].s_plasma.speed = 15;
    shipvals[STARBASE].s_plasma.cost = 3750;
    if (configvals->bronco_shipvals)
	shipvals[STARBASE].s_plasma.fuse = 25;
    else
	shipvals[STARBASE].s_plasma.fuse = 40;
    shipvals[STARBASE].s_plasma.wtemp = 367;
    shipvals[STARBASE].s_plasma.aux = 1;
    shipvals[STARBASE].s_maxwpntemp = 1300;
    shipvals[STARBASE].s_wpncoolrate = 7;
    if (configvals->bronco_shipvals)
	shipvals[STARBASE].s_maxegntemp = 1000;
    else
	shipvals[STARBASE].s_maxegntemp = 1300;
    shipvals[STARBASE].s_egncoolrate = 10;
    shipvals[STARBASE].s_maxfuel = 60000;
    shipvals[STARBASE].s_recharge = 90;
    shipvals[STARBASE].s_mingivefuel = 10000;
    shipvals[STARBASE].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[STARBASE].s_expldam = 150;	/* was: 100; (BG) */
	shipvals[STARBASE].s_fueldam = 100;
    }
    else {
	shipvals[STARBASE].s_expldam = 200;
	shipvals[STARBASE].s_fueldam = 0;
    }
    shipvals[STARBASE].s_armyperkill = 5;
    shipvals[STARBASE].s_maxarmies = 25;
    shipvals[STARBASE].s_bomb = 50;
    if (configvals->bronco_shipvals)
        shipvals[STARBASE].s_bombflags = SBOMB_ARMIES;
    else
        shipvals[STARBASE].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[STARBASE].s_repair = 170;	/* was: 140; (BG) */
    shipvals[STARBASE].s_maxdamage = 600;
    shipvals[STARBASE].s_maxshield = 500;
    shipvals[STARBASE].s_shieldcost = 10;
    shipvals[STARBASE].s_detcost = 100;
    shipvals[STARBASE].s_detdist = 1800;
    if (configvals->bronco_shipvals) {
	shipvals[STARBASE].s_cloakcost = 375;
	shipvals[STARBASE].s_scanrange = -1;
	shipvals[STARBASE].s_numports = 4;
    }
    else {
	shipvals[STARBASE].s_cloakcost = 750;
	shipvals[STARBASE].s_scanrange = 5000;
	shipvals[STARBASE].s_numports = 6;
    }
    shipvals[STARBASE].s_letter = 'o';
    shipvals[STARBASE].s_desig1 = 'S';
    shipvals[STARBASE].s_desig2 = 'B';
    shipvals[STARBASE].s_bitmap = 5;
    shipvals[STARBASE].s_width = 20;
    shipvals[STARBASE].s_height = 20;
    shipvals[STARBASE].s_timer = 30;
    shipvals[STARBASE].s_maxnum = 1;
    shipvals[STARBASE].s_rank = 4;
    shipvals[STARBASE].s_numdefn = 4;
    if (configvals->bronco_shipvals)
	shipvals[STARBASE].s_numplan = 5;
    else
	shipvals[STARBASE].s_numplan = 7;
    if (configvals->warpdrive)
	shipvals[STARBASE].s_nflags = SFNCANWARP | SFNCANFUEL | SFNCANREPAIR | SFNCANREFIT | SFNHASPHASERS | SFNPLASMASTYLE | SFNPLASMAARMED | SFNHASMISSILE;
    else
	shipvals[STARBASE].s_nflags = SFNCANFUEL | SFNCANREPAIR | SFNCANREFIT | SFNHASPHASERS | SFNPLASMASTYLE | SFNPLASMAARMED | SFNHASMISSILE;
    /* comprehensive definition of ATT */
    shipvals[ATT].s_alttype = 6;
    strcpy(shipvals[ATT].s_name, "AT&T");
    shipvals[ATT].s_turns = 1000000;
    shipvals[ATT].s_imp.acc = 10000;
    shipvals[ATT].s_imp.dec = 9000;
    shipvals[ATT].s_imp.cost = 1;
    shipvals[ATT].s_imp.maxspeed = 90;
    shipvals[ATT].s_imp.etemp = 0;
    shipvals[ATT].s_after.acc = 550;
    shipvals[ATT].s_after.dec = 270;
    shipvals[ATT].s_after.cost = 1;
    shipvals[ATT].s_after.maxspeed = 99;
    shipvals[ATT].s_after.etemp = 5;
    shipvals[ATT].s_warp.acc = 32000;
    shipvals[ATT].s_warp.dec = 32000;
    shipvals[ATT].s_warp.cost = 1;
    shipvals[ATT].s_warp.maxspeed = 99;
    shipvals[ATT].s_warp.etemp = 1;
    shipvals[ATT].s_warpinitcost = 1;
    shipvals[ATT].s_warpinittime = 1;
    shipvals[ATT].s_warpprepspeed = 0;
    shipvals[ATT].s_mass = 6000;
    shipvals[ATT].s_tractstr = 32000;
    shipvals[ATT].s_tractrng = 3;
    shipvals[ATT].s_tractcost = 1;
    shipvals[ATT].s_tractetemp = 3;
    shipvals[ATT].s_torp.damage = 40;
    shipvals[ATT].s_torp.speed = 20;
    shipvals[ATT].s_torp.cost = 1;
    shipvals[ATT].s_torp.fuse = 20;
    shipvals[ATT].s_torp.wtemp = 1;
    shipvals[ATT].s_torp.wtemp_halfarc = 0;
    shipvals[ATT].s_torp.wtemp_factor = 0;
    shipvals[ATT].s_torp.aux = 1;
    shipvals[ATT].s_phaser.damage = 110;
    shipvals[ATT].s_phaser.speed = 32000;
    shipvals[ATT].s_phaser.cost = 1;
    shipvals[ATT].s_phaser.fuse = 5;
    shipvals[ATT].s_phaser.wtemp = 5;
    shipvals[ATT].s_missile.damage = 0;
    shipvals[ATT].s_missile.speed = 0;
    shipvals[ATT].s_missile.cost = 0;
    shipvals[ATT].s_missile.fuse = 0;
    shipvals[ATT].s_missile.wtemp = 0;
    shipvals[ATT].s_missile.count = 0;
    shipvals[ATT].s_missile.aux = 0;
    shipvals[ATT].s_missilestored = 0;
    shipvals[ATT].s_plasma.damage = 150;
    shipvals[ATT].s_plasma.speed = 15;
    shipvals[ATT].s_plasma.cost = 1;
    shipvals[ATT].s_plasma.fuse = 20;
    shipvals[ATT].s_plasma.wtemp = 5;
    shipvals[ATT].s_plasma.aux = 2;
    shipvals[ATT].s_maxwpntemp = 10000;
    shipvals[ATT].s_wpncoolrate = 100;
    shipvals[ATT].s_maxegntemp = 10000;
    shipvals[ATT].s_egncoolrate = 100;
    shipvals[ATT].s_maxfuel = 60000;
    shipvals[ATT].s_recharge = 1000;
    shipvals[ATT].s_mingivefuel = 0;
    shipvals[ATT].s_takeonfuel = 150;
    shipvals[ATT].s_expldam = 500;
    shipvals[ATT].s_fueldam = 500;
    shipvals[ATT].s_armyperkill = 1.5;
    shipvals[ATT].s_maxarmies = 1000;
    shipvals[ATT].s_bomb = -2100;
    shipvals[ATT].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[ATT].s_repair = 30000;
    shipvals[ATT].s_maxdamage = 30000;
    shipvals[ATT].s_maxshield = 30000;
    shipvals[ATT].s_shieldcost = 1;
    shipvals[ATT].s_detcost = 1;
    shipvals[ATT].s_detdist = 3000;
    shipvals[ATT].s_cloakcost = 1;
    shipvals[ATT].s_scanrange = 10000;
    shipvals[ATT].s_numports = 0;
    shipvals[ATT].s_letter = '.';
    shipvals[ATT].s_desig1 = 'A';
    shipvals[ATT].s_desig2 = 'T';
    shipvals[ATT].s_bitmap = 6;
    shipvals[ATT].s_width = 28;
    shipvals[ATT].s_height = 28;
    shipvals[ATT].s_timer = 5;
    shipvals[ATT].s_maxnum = 32;
    shipvals[ATT].s_rank = 0;
    shipvals[ATT].s_numdefn = 0;
    shipvals[ATT].s_numplan = 0;
    shipvals[ATT].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP | SFNHASPHASERS | SFNPLASMASTYLE | SFNPLASMAARMED;

    /* comprehensive definition of JUMPSHIP */
    shipvals[JUMPSHIP].s_alttype = 5;
    strcpy(shipvals[JUMPSHIP].s_name, "Jumpship");
    shipvals[JUMPSHIP].s_turns = 700000;
    if (configvals->bronco_shipvals) {
	shipvals[JUMPSHIP].s_imp.acc = 1000;
	shipvals[JUMPSHIP].s_imp.dec = 500;
    }
    else {
	shipvals[JUMPSHIP].s_imp.acc = 2000;
	shipvals[JUMPSHIP].s_imp.dec = 1000;
    }
    shipvals[JUMPSHIP].s_imp.cost = 1;
    shipvals[JUMPSHIP].s_imp.maxspeed = 20;
    shipvals[JUMPSHIP].s_imp.etemp = 1000;
    shipvals[JUMPSHIP].s_after.acc = 2000;
    shipvals[JUMPSHIP].s_after.dec = 2000;
    shipvals[JUMPSHIP].s_after.maxspeed = 30;
    shipvals[JUMPSHIP].s_after.etemp = 200000;
    shipvals[JUMPSHIP].s_after.cost = 1000;
    shipvals[JUMPSHIP].s_warp.cost = 200;
    shipvals[JUMPSHIP].s_warp.acc = 2000;
    shipvals[JUMPSHIP].s_warp.dec = 3000;
    if (configvals->bronco_shipvals)
	shipvals[JUMPSHIP].s_warp.maxspeed = 32;
    else
	shipvals[JUMPSHIP].s_warp.maxspeed = 45;
    shipvals[JUMPSHIP].s_warp.etemp = 500;
    shipvals[JUMPSHIP].s_warpinitcost = 1;
    shipvals[JUMPSHIP].s_warpinittime = 7;
	shipvals[JUMPSHIP].s_warpprepspeed = 2;
    shipvals[JUMPSHIP].s_mass = 10000;
    shipvals[JUMPSHIP].s_tractstr = 5000;
    shipvals[JUMPSHIP].s_tractrng = 1.5;
    shipvals[JUMPSHIP].s_tractcost = 8;
    shipvals[JUMPSHIP].s_tractetemp = 5000;
    shipvals[JUMPSHIP].s_torp.damage = 5;
    shipvals[JUMPSHIP].s_torp.speed = 18;
    shipvals[JUMPSHIP].s_torp.cost = 1000;
    shipvals[JUMPSHIP].s_torp.fuse = 10;
    shipvals[JUMPSHIP].s_torp.wtemp = 99;
    shipvals[JUMPSHIP].s_torp.wtemp_halfarc = 32;
    shipvals[JUMPSHIP].s_torp.wtemp_factor = 9;
    shipvals[JUMPSHIP].s_torp.aux = 0;
    shipvals[JUMPSHIP].s_phaser.damage = 25;
    shipvals[JUMPSHIP].s_phaser.speed = 3000;
    shipvals[JUMPSHIP].s_phaser.cost = 500;
    shipvals[JUMPSHIP].s_phaser.fuse = 4;
    shipvals[JUMPSHIP].s_phaser.wtemp = 5;
    shipvals[JUMPSHIP].s_missile.damage = 0;
    shipvals[JUMPSHIP].s_missile.speed = 0;
    shipvals[JUMPSHIP].s_missile.cost = 0;
    shipvals[JUMPSHIP].s_missile.fuse = 0;
    shipvals[JUMPSHIP].s_missile.wtemp = 0;
    shipvals[JUMPSHIP].s_missile.count = 0;
    shipvals[JUMPSHIP].s_missile.aux = 0;
    shipvals[JUMPSHIP].s_missilestored = 0;
    shipvals[JUMPSHIP].s_plasma.damage = -1;
    shipvals[JUMPSHIP].s_plasma.speed = 15;
    shipvals[JUMPSHIP].s_plasma.cost = 0;
    shipvals[JUMPSHIP].s_plasma.fuse = 25;
    shipvals[JUMPSHIP].s_plasma.wtemp = 5;
    shipvals[JUMPSHIP].s_plasma.aux = 1;
    shipvals[JUMPSHIP].s_maxwpntemp = 1300;
    shipvals[JUMPSHIP].s_wpncoolrate = 4;
    shipvals[JUMPSHIP].s_maxegntemp = 5000;
    shipvals[JUMPSHIP].s_egncoolrate = 34;
    shipvals[JUMPSHIP].s_maxfuel = 50000;
    shipvals[JUMPSHIP].s_recharge = 200;
    shipvals[JUMPSHIP].s_mingivefuel = 10000;
    shipvals[JUMPSHIP].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[JUMPSHIP].s_expldam = 25;	/* was: 30; */
	shipvals[JUMPSHIP].s_fueldam = 175;	/* was: 160; */
    }
    else {
	shipvals[JUMPSHIP].s_expldam = 150;
	shipvals[JUMPSHIP].s_fueldam = 0;
    }
    shipvals[JUMPSHIP].s_armyperkill = 0;
    shipvals[JUMPSHIP].s_maxarmies = 0;
    shipvals[JUMPSHIP].s_bomb = 0;
    shipvals[JUMPSHIP].s_bombflags = 0;
    shipvals[JUMPSHIP].s_repair = 200;
    shipvals[JUMPSHIP].s_maxdamage = 60;
    shipvals[JUMPSHIP].s_maxshield = 5;
    shipvals[JUMPSHIP].s_shieldcost = 5;
    shipvals[JUMPSHIP].s_detcost = 100;
    shipvals[JUMPSHIP].s_detdist = 1750;
    shipvals[JUMPSHIP].s_cloakcost = 1000;
    shipvals[JUMPSHIP].s_scanrange = -1;
    shipvals[JUMPSHIP].s_numports = 4;
    shipvals[JUMPSHIP].s_letter = 'j';
    shipvals[JUMPSHIP].s_desig1 = 'J';
    shipvals[JUMPSHIP].s_desig2 = 'S';
    shipvals[JUMPSHIP].s_bitmap = 7;
    shipvals[JUMPSHIP].s_width = 20;
    shipvals[JUMPSHIP].s_height = 20;
    shipvals[JUMPSHIP].s_timer = 5;
    shipvals[JUMPSHIP].s_maxnum = 1;
    shipvals[JUMPSHIP].s_rank = 3;
    shipvals[JUMPSHIP].s_numdefn = 3;
    shipvals[JUMPSHIP].s_numplan = 0;
    /* UFL says jumpships shouldn't refit. */
    if (configvals->warpdrive)
	shipvals[JUMPSHIP].s_nflags = SFNCANWARP | SFNCANFUEL | SFNHASPHASERS | SFNCANREFIT;
    else
	shipvals[JUMPSHIP].s_nflags = SFNCANFUEL | SFNHASPHASERS | SFNCANREFIT;

    /* comprehensive definition of FRIGATE */
    shipvals[FRIGATE].s_alttype = 4;
    strcpy(shipvals[FRIGATE].s_name, "Frigate");
    shipvals[FRIGATE].s_turns = 122500;
    shipvals[FRIGATE].s_imp.acc = 115;
    shipvals[FRIGATE].s_imp.dec = 190;
    shipvals[FRIGATE].s_imp.cost = 5;
    shipvals[FRIGATE].s_imp.maxspeed = 9;
    shipvals[FRIGATE].s_imp.etemp = 1000;
    shipvals[FRIGATE].s_after.acc = 525;
    shipvals[FRIGATE].s_after.dec = 270;
    shipvals[FRIGATE].s_after.cost = 150;
    shipvals[FRIGATE].s_after.maxspeed = 10;
    shipvals[FRIGATE].s_after.etemp = 50000;
    shipvals[FRIGATE].s_warp.acc = 10000;
    shipvals[FRIGATE].s_warp.dec = 500;
    shipvals[FRIGATE].s_warp.cost = 30;
    if (configvals->bronco_shipvals)
	shipvals[FRIGATE].s_warp.maxspeed = 12;
    else
	shipvals[FRIGATE].s_warp.maxspeed = 23;
    shipvals[FRIGATE].s_warp.etemp = 7000;
    shipvals[FRIGATE].s_warpinitcost = 2272;
    shipvals[FRIGATE].s_warpinittime = 65;
    if (configvals->bronco_shipvals)
	shipvals[FRIGATE].s_warpprepspeed = 1;
    else
	shipvals[FRIGATE].s_warpprepspeed = 2;
    shipvals[FRIGATE].s_mass = 2150;
    shipvals[FRIGATE].s_tractstr = 3400;
    shipvals[FRIGATE].s_tractrng = 1.1;
    shipvals[FRIGATE].s_tractcost = 4;
    shipvals[FRIGATE].s_tractetemp = 1000;
    shipvals[FRIGATE].s_torp.damage = 40;
    shipvals[FRIGATE].s_torp.speed = 12;
    shipvals[FRIGATE].s_torp.cost = 290;
    shipvals[FRIGATE].s_torp.fuse = 40;
    shipvals[FRIGATE].s_torp.wtemp = 19;
    shipvals[FRIGATE].s_torp.wtemp_halfarc = 32;
    shipvals[FRIGATE].s_torp.wtemp_factor = 9;
    shipvals[FRIGATE].s_torp.aux = 0;
    shipvals[FRIGATE].s_phaser.damage = 102;
    shipvals[FRIGATE].s_phaser.speed = 6150;
    shipvals[FRIGATE].s_phaser.cost = 816;
    shipvals[FRIGATE].s_phaser.fuse = 10;
    shipvals[FRIGATE].s_phaser.wtemp = 80;
    shipvals[FRIGATE].s_missile.damage = 23;
    shipvals[FRIGATE].s_missile.speed = 6;
    shipvals[FRIGATE].s_missile.cost = 850;
    shipvals[FRIGATE].s_missile.fuse = 100;
    shipvals[FRIGATE].s_missile.wtemp = 100;
    shipvals[FRIGATE].s_missile.count = 3;
    shipvals[FRIGATE].s_missile.aux = 2;
    shipvals[FRIGATE].s_missilestored = 11;
    shipvals[FRIGATE].s_plasma.damage = 115;
    shipvals[FRIGATE].s_plasma.speed = 15;
    shipvals[FRIGATE].s_plasma.cost = 3450;
    shipvals[FRIGATE].s_plasma.fuse = 35;
    shipvals[FRIGATE].s_plasma.wtemp = 337;
    shipvals[FRIGATE].s_plasma.aux = 1;
    shipvals[FRIGATE].s_maxwpntemp = 1000;
    shipvals[FRIGATE].s_wpncoolrate = 3;
    shipvals[FRIGATE].s_maxegntemp = 1500;
    shipvals[FRIGATE].s_egncoolrate = 6;
    shipvals[FRIGATE].s_maxfuel = 12500;
    shipvals[FRIGATE].s_recharge = 26;
    shipvals[FRIGATE].s_mingivefuel = 0;
    shipvals[FRIGATE].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[FRIGATE].s_expldam = 77;	/* was: 50; (BG) */
	shipvals[FRIGATE].s_fueldam = 40;	/* was: 72; (BG) */
    }
    else {
	shipvals[FRIGATE].s_expldam = 100;
	shipvals[FRIGATE].s_fueldam = 0;
    }
    shipvals[FRIGATE].s_armyperkill = 2;
    shipvals[FRIGATE].s_maxarmies = 6;
    shipvals[FRIGATE].s_bomb = 15;
    shipvals[FRIGATE].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[FRIGATE].s_repair = 118;
    shipvals[FRIGATE].s_maxdamage = 115;
    shipvals[FRIGATE].s_maxshield = 115;
    shipvals[FRIGATE].s_shieldcost = 5;
    shipvals[FRIGATE].s_detcost = 100;
    shipvals[FRIGATE].s_detdist = 1750;
    if (configvals->bronco_shipvals)
	shipvals[FRIGATE].s_cloakcost = 140;
    else
	shipvals[FRIGATE].s_cloakcost = 125;
    shipvals[FRIGATE].s_scanrange = -1;
    shipvals[FRIGATE].s_numports = 0;
    shipvals[FRIGATE].s_letter = 'f';
    shipvals[FRIGATE].s_desig1 = 'F';
    shipvals[FRIGATE].s_desig2 = 'R';
    shipvals[FRIGATE].s_bitmap = 8;
    shipvals[FRIGATE].s_width = 20;
    shipvals[FRIGATE].s_height = 20;
    shipvals[FRIGATE].s_timer = 0;
    shipvals[FRIGATE].s_maxnum = 32;
    shipvals[FRIGATE].s_rank = 0;
    shipvals[FRIGATE].s_numdefn = 0;
    shipvals[FRIGATE].s_numplan = 0;
    if (configvals->warpdrive)
	shipvals[FRIGATE].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP |  SFNHASPHASERS;
    else
	shipvals[FRIGATE].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL |  SFNHASPHASERS;

    /* comprehensive definition of WARBASE */
    shipvals[WARBASE].s_alttype = 5;
    strcpy(shipvals[WARBASE].s_name, "Warbase");
    shipvals[WARBASE].s_turns = 90000;
    shipvals[WARBASE].s_imp.acc = 100;
    shipvals[WARBASE].s_imp.dec = 200;
    shipvals[WARBASE].s_imp.cost = 4;
    shipvals[WARBASE].s_imp.maxspeed = 3;
    shipvals[WARBASE].s_imp.etemp = 1000;
    shipvals[WARBASE].s_after.acc = 250;
    shipvals[WARBASE].s_after.dec = 100;
    shipvals[WARBASE].s_after.cost = 40;
    shipvals[WARBASE].s_after.maxspeed = 5;
    shipvals[WARBASE].s_after.etemp = 30000;
    shipvals[WARBASE].s_warp.acc = 80;
    shipvals[WARBASE].s_warp.dec = 80;
    shipvals[WARBASE].s_warp.cost = 6;
    if (configvals->bronco_shipvals) {
	shipvals[WARBASE].s_warp.maxspeed = 4;
	shipvals[WARBASE].s_warpprepspeed = 1;
    }
    else {
	shipvals[WARBASE].s_warp.maxspeed = 6;
	shipvals[WARBASE].s_warpprepspeed = 2;
    }
    shipvals[WARBASE].s_warp.etemp = 1500;
    shipvals[WARBASE].s_warpinitcost = 9090;
    shipvals[WARBASE].s_warpinittime = 100;
    shipvals[WARBASE].s_mass = 4000;
    shipvals[WARBASE].s_tractstr = 8000;
    shipvals[WARBASE].s_tractrng = 1.5;
    shipvals[WARBASE].s_tractcost = 10;
    shipvals[WARBASE].s_tractetemp = 3000;
    shipvals[WARBASE].s_torp.damage = 45;
    shipvals[WARBASE].s_torp.speed = 15;
    shipvals[WARBASE].s_torp.cost = 450;
    shipvals[WARBASE].s_torp.fuse = 20;
    shipvals[WARBASE].s_torp.wtemp = 35;
    shipvals[WARBASE].s_torp.wtemp_halfarc = 32;
    shipvals[WARBASE].s_torp.wtemp_factor = 9;
    shipvals[WARBASE].s_torp.aux = 0;
    shipvals[WARBASE].s_phaser.damage = 125;
    shipvals[WARBASE].s_phaser.speed = 7500;
    shipvals[WARBASE].s_phaser.cost = 1000;
    shipvals[WARBASE].s_phaser.fuse = 5;
    shipvals[WARBASE].s_phaser.wtemp = 90;
    shipvals[WARBASE].s_missile.damage = 40;
    shipvals[WARBASE].s_missile.speed = 14;
    shipvals[WARBASE].s_missile.cost = 2000;
    shipvals[WARBASE].s_missile.fuse = 100;
    shipvals[WARBASE].s_missile.wtemp = 120;
    shipvals[WARBASE].s_missile.count = 3;
    shipvals[WARBASE].s_missile.aux = 2;
    shipvals[WARBASE].s_missilestored = -1;
    shipvals[WARBASE].s_plasma.damage = 150;
    shipvals[WARBASE].s_plasma.speed = 15;
    shipvals[WARBASE].s_plasma.cost = 3750;
    shipvals[WARBASE].s_plasma.fuse = 40;
    shipvals[WARBASE].s_plasma.wtemp = 360;
    shipvals[WARBASE].s_plasma.aux = 1;
    shipvals[WARBASE].s_maxwpntemp = 1500;
    shipvals[WARBASE].s_wpncoolrate = 5;
    shipvals[WARBASE].s_maxegntemp = 1000;
    shipvals[WARBASE].s_egncoolrate = 10;
    shipvals[WARBASE].s_maxfuel = 50000;
    if (configvals->bronco_shipvals)
	shipvals[WARBASE].s_recharge = 70;
    else
	shipvals[WARBASE].s_recharge = 80;
    shipvals[WARBASE].s_mingivefuel = 10000;
    shipvals[WARBASE].s_takeonfuel = 150;
    shipvals[WARBASE].s_expldam = 100;
    shipvals[WARBASE].s_fueldam = 100;
    shipvals[WARBASE].s_armyperkill = 0;
    shipvals[WARBASE].s_maxarmies = 0;
    shipvals[WARBASE].s_bomb = 90;
    shipvals[WARBASE].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[WARBASE].s_repair = 170;	/* was: 150; (BG) */
    shipvals[WARBASE].s_maxdamage = 500;
    shipvals[WARBASE].s_maxshield = 250;
    shipvals[WARBASE].s_shieldcost = 10;
    shipvals[WARBASE].s_detcost = 100;
    shipvals[WARBASE].s_detdist = 1800;
    shipvals[WARBASE].s_cloakcost = 1000;
    shipvals[WARBASE].s_scanrange = 5000;
    shipvals[WARBASE].s_numports = 2;
    shipvals[WARBASE].s_letter = 'w';
    shipvals[WARBASE].s_desig1 = 'W';
    shipvals[WARBASE].s_desig2 = 'B';
    shipvals[WARBASE].s_bitmap = 9;
    shipvals[WARBASE].s_width = 20;
    shipvals[WARBASE].s_height = 20;
    shipvals[WARBASE].s_timer = 15;
    shipvals[WARBASE].s_maxnum = 1;
    shipvals[WARBASE].s_rank = 3;
    shipvals[WARBASE].s_numdefn = 3;
    shipvals[WARBASE].s_numplan = 3;
    if (configvals->warpdrive)
	shipvals[WARBASE].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP | SFNCANFUEL | SFNCANREPAIR | SFNHASPHASERS | SFNPLASMASTYLE | SFNPLASMAARMED | SFNHASMISSILE;
    else
	shipvals[WARBASE].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANFUEL | SFNCANREPAIR | SFNHASPHASERS | SFNPLASMASTYLE | SFNPLASMAARMED | SFNHASMISSILE;

    /* comprehensive definition of LIGHTCRUISER */
    shipvals[LIGHTCRUISER].s_alttype = 2;
    strcpy(shipvals[LIGHTCRUISER].s_name, "Light");
    shipvals[LIGHTCRUISER].s_turns = 220000;	/* was 225000 */
    shipvals[LIGHTCRUISER].s_imp.acc = 190;
    shipvals[LIGHTCRUISER].s_imp.dec = 250;
    shipvals[LIGHTCRUISER].s_imp.cost = 3;
    shipvals[LIGHTCRUISER].s_imp.maxspeed = 10;
    shipvals[LIGHTCRUISER].s_imp.etemp = 1000;
    shipvals[LIGHTCRUISER].s_after.acc = 720;
    shipvals[LIGHTCRUISER].s_after.dec = 280;
    shipvals[LIGHTCRUISER].s_after.cost = 80;
    shipvals[LIGHTCRUISER].s_after.maxspeed = 12;
    shipvals[LIGHTCRUISER].s_after.etemp = 35000;
    shipvals[LIGHTCRUISER].s_warp.acc = 10000;
    shipvals[LIGHTCRUISER].s_warp.dec = 400;
    shipvals[LIGHTCRUISER].s_warp.cost = 24;
    if (configvals->bronco_shipvals) {
	shipvals[LIGHTCRUISER].s_warp.maxspeed = 16;
	shipvals[LIGHTCRUISER].s_warpprepspeed = 1;
    }
    else {
	shipvals[LIGHTCRUISER].s_warp.maxspeed = 27;	/* was: 24; (BG) */
	shipvals[LIGHTCRUISER].s_warpprepspeed = 2;
    }
    shipvals[LIGHTCRUISER].s_warp.etemp = 7000;
    shipvals[LIGHTCRUISER].s_warpinitcost = 1550;
    shipvals[LIGHTCRUISER].s_warpinittime = 45;
    shipvals[LIGHTCRUISER].s_mass = 1900;
    shipvals[LIGHTCRUISER].s_tractstr = 2700;
    shipvals[LIGHTCRUISER].s_tractrng = 0.9;
    shipvals[LIGHTCRUISER].s_tractcost = 3;
    shipvals[LIGHTCRUISER].s_tractetemp = 1000;
    shipvals[LIGHTCRUISER].s_torp.damage = 35;
    shipvals[LIGHTCRUISER].s_torp.speed = 13;
    shipvals[LIGHTCRUISER].s_torp.cost = 245;
    shipvals[LIGHTCRUISER].s_torp.fuse = 35;
    shipvals[LIGHTCRUISER].s_torp.wtemp = 16;
    shipvals[LIGHTCRUISER].s_torp.wtemp_halfarc = 32;
    shipvals[LIGHTCRUISER].s_torp.wtemp_factor = 9;
    shipvals[LIGHTCRUISER].s_torp.aux = 0;
    shipvals[LIGHTCRUISER].s_phaser.damage = 90;
    shipvals[LIGHTCRUISER].s_phaser.speed = 5400;
    shipvals[LIGHTCRUISER].s_phaser.cost = 630;
    shipvals[LIGHTCRUISER].s_phaser.fuse = 10;
    shipvals[LIGHTCRUISER].s_phaser.wtemp = 60;
    shipvals[LIGHTCRUISER].s_missile.damage = 22;
    shipvals[LIGHTCRUISER].s_missile.speed = 7;
    shipvals[LIGHTCRUISER].s_missile.cost = 800;
    shipvals[LIGHTCRUISER].s_missile.fuse = 100;
    shipvals[LIGHTCRUISER].s_missile.wtemp = 100;
    shipvals[LIGHTCRUISER].s_missile.count = 3;
    shipvals[LIGHTCRUISER].s_missile.aux = 2;
    shipvals[LIGHTCRUISER].s_missilestored = 9;
    shipvals[LIGHTCRUISER].s_plasma.damage = 90;
    shipvals[LIGHTCRUISER].s_plasma.speed = 15;
    shipvals[LIGHTCRUISER].s_plasma.cost = 2500;
    shipvals[LIGHTCRUISER].s_plasma.fuse = 30;
    shipvals[LIGHTCRUISER].s_plasma.wtemp = 242;
    shipvals[LIGHTCRUISER].s_plasma.aux = 1;
    shipvals[LIGHTCRUISER].s_maxwpntemp = 1000;
    shipvals[LIGHTCRUISER].s_wpncoolrate = 3;
    shipvals[LIGHTCRUISER].s_maxegntemp = 1500;
    shipvals[LIGHTCRUISER].s_egncoolrate = 6;
    shipvals[LIGHTCRUISER].s_maxfuel = 8500;
    shipvals[LIGHTCRUISER].s_recharge = 23;
    shipvals[LIGHTCRUISER].s_mingivefuel = 0;
    shipvals[LIGHTCRUISER].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[LIGHTCRUISER].s_expldam = 67;	/* was: 50; (BG) */
	shipvals[LIGHTCRUISER].s_fueldam = 45;	/* was: 58; (BG) */
    }
    else {
	shipvals[LIGHTCRUISER].s_expldam = 100;
	shipvals[LIGHTCRUISER].s_fueldam = 0;
    }
    shipvals[LIGHTCRUISER].s_armyperkill = 2;
    shipvals[LIGHTCRUISER].s_maxarmies = 3;	/* was 4 */
    shipvals[LIGHTCRUISER].s_bomb = 6;
    shipvals[LIGHTCRUISER].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[LIGHTCRUISER].s_repair = 80;	/* was 90 */
    shipvals[LIGHTCRUISER].s_maxdamage = 90;
    shipvals[LIGHTCRUISER].s_maxshield = 95;
    shipvals[LIGHTCRUISER].s_shieldcost = 4;
    shipvals[LIGHTCRUISER].s_detcost = 100;
    shipvals[LIGHTCRUISER].s_detdist = 1750;
    if (configvals->bronco_shipvals)
	shipvals[LIGHTCRUISER].s_cloakcost = 115;
    else
	shipvals[LIGHTCRUISER].s_cloakcost = 75;
    shipvals[LIGHTCRUISER].s_scanrange = 500;
    shipvals[LIGHTCRUISER].s_numports = 0;
    shipvals[LIGHTCRUISER].s_letter = 'l';
    shipvals[LIGHTCRUISER].s_desig1 = 'C';
    shipvals[LIGHTCRUISER].s_desig2 = 'L';
    shipvals[LIGHTCRUISER].s_bitmap = 10;
    shipvals[LIGHTCRUISER].s_width = 20;
    shipvals[LIGHTCRUISER].s_height = 20;
    shipvals[LIGHTCRUISER].s_timer = 0;
    shipvals[LIGHTCRUISER].s_maxnum = 32;
    shipvals[LIGHTCRUISER].s_rank = 0;
    shipvals[LIGHTCRUISER].s_numdefn = 0;
    shipvals[LIGHTCRUISER].s_numplan = 0;
    if (configvals->warpdrive)
	shipvals[LIGHTCRUISER].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL | SFNCANWARP |  SFNHASPHASERS;
    else
	shipvals[LIGHTCRUISER].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL |  SFNHASPHASERS;

    /* comprehensive definition of CARRIER */
    shipvals[CARRIER].s_alttype = 3;
    strcpy(shipvals[CARRIER].s_name, "Carrier");
    shipvals[CARRIER].s_turns = 60000;
    shipvals[CARRIER].s_imp.acc = 100;
    shipvals[CARRIER].s_imp.dec = 200;
    shipvals[CARRIER].s_imp.cost = 4;
    shipvals[CARRIER].s_imp.maxspeed = 9;
    shipvals[CARRIER].s_imp.etemp = 1000;
    shipvals[CARRIER].s_after.acc = 500;
    shipvals[CARRIER].s_after.dec = 250;
    shipvals[CARRIER].s_after.cost = 100;
    shipvals[CARRIER].s_after.maxspeed = 11;
    shipvals[CARRIER].s_after.etemp = 50000;
    shipvals[CARRIER].s_warp.acc = 10000;
    shipvals[CARRIER].s_warp.dec = 300;
    shipvals[CARRIER].s_warp.cost = 28;
    if (configvals->bronco_shipvals) {
	shipvals[CARRIER].s_warp.maxspeed = 14;
	shipvals[CARRIER].s_warpprepspeed = 0;
    }
    else {
	shipvals[CARRIER].s_warp.maxspeed = 25;	/* was: 20; (BG) */
	shipvals[CARRIER].s_warpprepspeed = 1;
    }
    shipvals[CARRIER].s_warp.etemp = 7000;
    shipvals[CARRIER].s_warpinitcost = 2800;
    shipvals[CARRIER].s_warpinittime = 75;
    shipvals[CARRIER].s_mass = 2500;
    shipvals[CARRIER].s_tractstr = 4000;        /* was 3200 (MDM) */
    shipvals[CARRIER].s_tractrng = 1.3;         /* was 1.1  (MDM) */
    shipvals[CARRIER].s_tractcost = 5;
    shipvals[CARRIER].s_tractetemp = 1000;
    shipvals[CARRIER].s_torp.damage = 30;	/* these are the CVs own
						   torps */
    shipvals[CARRIER].s_torp.speed = 13;	/* fighter-torps are now
						   constant */
    shipvals[CARRIER].s_torp.cost = 210;	/* still used when fighters
						   fire! */
    shipvals[CARRIER].s_torp.fuse = 35;
    shipvals[CARRIER].s_torp.wtemp = 20;	/* still used when fighters
						   fire! */
    shipvals[CARRIER].s_torp.wtemp_halfarc = 32;
    shipvals[CARRIER].s_torp.wtemp_factor = 9;
    shipvals[CARRIER].s_torp.aux = 0;
    shipvals[CARRIER].s_phaser.damage = 95;
    shipvals[CARRIER].s_phaser.speed = 6500;
    shipvals[CARRIER].s_phaser.cost = 570;
    shipvals[CARRIER].s_phaser.fuse = 6;
    shipvals[CARRIER].s_phaser.wtemp = 45;
    shipvals[CARRIER].s_missile.damage = 20;
    shipvals[CARRIER].s_missile.speed = 14;
    shipvals[CARRIER].s_missile.cost = 380;	/* no longer includes
						   torpcost */
    shipvals[CARRIER].s_missile.fuse = 300;
    shipvals[CARRIER].s_missile.wtemp = 35;	/* no longer includes torp
						   wtemp */
    shipvals[CARRIER].s_missile.count = 8;
    shipvals[CARRIER].s_missile.aux = 3;
    shipvals[CARRIER].s_missilestored = 0;
    shipvals[CARRIER].s_plasma.damage = 80;
    shipvals[CARRIER].s_plasma.speed = 15;
    shipvals[CARRIER].s_plasma.cost = 3000;
    shipvals[CARRIER].s_plasma.fuse = 35;
    shipvals[CARRIER].s_plasma.wtemp = 270;
    shipvals[CARRIER].s_plasma.aux = 2;
    shipvals[CARRIER].s_maxwpntemp = 1000;
    shipvals[CARRIER].s_wpncoolrate = 2;
    shipvals[CARRIER].s_maxegntemp = 1500;
    shipvals[CARRIER].s_egncoolrate = 5;
    shipvals[CARRIER].s_maxfuel = 15000;
    shipvals[CARRIER].s_recharge = 25;
    shipvals[CARRIER].s_mingivefuel = 0;
    shipvals[CARRIER].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[CARRIER].s_expldam = 80;	/* was: 55; (BG) */
	shipvals[CARRIER].s_fueldam = 70;	/* was: 80; (BG) */
    }
    else {
	shipvals[CARRIER].s_expldam = 100;
	shipvals[CARRIER].s_fueldam = 0;
    }
    shipvals[CARRIER].s_armyperkill = 25;
    shipvals[CARRIER].s_maxarmies = 3;
    shipvals[CARRIER].s_bomb = 20;
    shipvals[CARRIER].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[CARRIER].s_repair = 105;
    shipvals[CARRIER].s_maxdamage = 150;
    shipvals[CARRIER].s_maxshield = 120;
    shipvals[CARRIER].s_shieldcost = 5;
    shipvals[CARRIER].s_detcost = 50;
    shipvals[CARRIER].s_detdist = 1900;
    if (configvals->bronco_shipvals)
	shipvals[CARRIER].s_cloakcost = 135;
    else
	shipvals[CARRIER].s_cloakcost = 100;
    shipvals[CARRIER].s_scanrange = -1;
    shipvals[CARRIER].s_numports = 0;
    shipvals[CARRIER].s_letter = 'v';
    shipvals[CARRIER].s_desig1 = 'C';
    shipvals[CARRIER].s_desig2 = 'V';
    shipvals[CARRIER].s_bitmap = 11;
    shipvals[CARRIER].s_width = 20;
    shipvals[CARRIER].s_height = 20;
    shipvals[CARRIER].s_timer = 12;
    shipvals[CARRIER].s_maxnum = 1;
    shipvals[CARRIER].s_rank = 3;
    shipvals[CARRIER].s_numdefn = 5;
    shipvals[CARRIER].s_numplan = 3;
    if (configvals->warpdrive)
	shipvals[CARRIER].s_nflags = SFNCANWARP | SFNHASPHASERS | SFNHASMISSILE | SFNHASFIGHTERS;
    else
	shipvals[CARRIER].s_nflags = SFNHASPHASERS | SFNHASMISSILE | SFNHASFIGHTERS;

    /* comprehensive definition of UTILITY */
    shipvals[UTILITY].s_alttype = 4;
    strcpy(shipvals[UTILITY].s_name, "Utility");
    shipvals[UTILITY].s_turns = 80000;
    shipvals[UTILITY].s_imp.acc = 100;
    shipvals[UTILITY].s_imp.dec = 200;
    shipvals[UTILITY].s_imp.cost = 4;
    shipvals[UTILITY].s_imp.maxspeed = 7;
    shipvals[UTILITY].s_imp.etemp = 1000;
    shipvals[UTILITY].s_after.acc = 500;
    shipvals[UTILITY].s_after.dec = 250;
    shipvals[UTILITY].s_after.cost = 40;
    shipvals[UTILITY].s_after.maxspeed = 8;
    shipvals[UTILITY].s_after.etemp = 40000;
    shipvals[UTILITY].s_warp.acc = 10000;
    shipvals[UTILITY].s_warp.dec = 5000;
    shipvals[UTILITY].s_warp.cost = 20;
    if (configvals->bronco_shipvals) {
	shipvals[UTILITY].s_warp.maxspeed = 10;
	shipvals[UTILITY].s_warpprepspeed = 0;
    }
    else {
	shipvals[UTILITY].s_warp.maxspeed = 20;	/* was: 15; (BG) */
	shipvals[UTILITY].s_warpprepspeed = 1;
    }
    shipvals[UTILITY].s_warp.etemp = 5500;
    shipvals[UTILITY].s_warpinitcost = 1200;
    shipvals[UTILITY].s_warpinittime = 50;
    shipvals[UTILITY].s_mass = 2400;
    shipvals[UTILITY].s_tractstr = 3500;
    shipvals[UTILITY].s_tractrng = 1.1;
    shipvals[UTILITY].s_tractcost = 4;
    shipvals[UTILITY].s_tractetemp = 1000;
    shipvals[UTILITY].s_torp.damage = 20;
    shipvals[UTILITY].s_torp.speed = 15;
    shipvals[UTILITY].s_torp.cost = 250;
    shipvals[UTILITY].s_torp.fuse = 25; 
    shipvals[UTILITY].s_torp.wtemp = 18;
    shipvals[UTILITY].s_torp.wtemp_halfarc = 16;
    shipvals[UTILITY].s_torp.wtemp_factor = 4;
    shipvals[UTILITY].s_torp.aux = 0;
    shipvals[UTILITY].s_phaser.damage = 80;
    shipvals[UTILITY].s_phaser.speed = 5600;
    shipvals[UTILITY].s_phaser.cost = 640;
    shipvals[UTILITY].s_phaser.fuse = 8;
    shipvals[UTILITY].s_phaser.wtemp = 85;
    shipvals[UTILITY].s_missile.damage = 30;
    shipvals[UTILITY].s_missile.speed = 8;
    shipvals[UTILITY].s_missile.cost = 800;
    shipvals[UTILITY].s_missile.fuse = 80;
    shipvals[UTILITY].s_missile.wtemp = 60;
    shipvals[UTILITY].s_missile.count = 3;
    shipvals[UTILITY].s_missile.aux = 2;
    shipvals[UTILITY].s_missilestored = 18;
    shipvals[UTILITY].s_plasma.damage = -1;
    shipvals[UTILITY].s_plasma.speed = 0;
    shipvals[UTILITY].s_plasma.cost = 0;
    shipvals[UTILITY].s_plasma.fuse = 0;
    shipvals[UTILITY].s_plasma.wtemp = 0;
    shipvals[UTILITY].s_plasma.aux = 0;
    shipvals[UTILITY].s_maxwpntemp = 1000;
    shipvals[UTILITY].s_wpncoolrate = 2;
    shipvals[UTILITY].s_maxegntemp = 1800;
    shipvals[UTILITY].s_egncoolrate = 5;
    shipvals[UTILITY].s_maxfuel = 16000;
    shipvals[UTILITY].s_recharge = 38;
    shipvals[UTILITY].s_mingivefuel = 4000;
    shipvals[UTILITY].s_takeonfuel = 150;
    if (configvals->fuel_explosions) {
	shipvals[UTILITY].s_expldam = 60;
	shipvals[UTILITY].s_fueldam = 80;
    }
    else {
	shipvals[UTILITY].s_expldam = 100;
	shipvals[UTILITY].s_fueldam = 0;
    }
    shipvals[UTILITY].s_armyperkill = 12;
    shipvals[UTILITY].s_maxarmies = 12;
    shipvals[UTILITY].s_bomb = 0;
    shipvals[UTILITY].s_bombflags = SBOMB_ARMIES | SBOMB_FACILITIES;
    shipvals[UTILITY].s_repair = 120;
    shipvals[UTILITY].s_maxdamage = 220;
    shipvals[UTILITY].s_maxshield = 120;
    shipvals[UTILITY].s_shieldcost = 4;
    shipvals[UTILITY].s_detcost = 50;
    shipvals[UTILITY].s_detdist = 1900;
    if (configvals->bronco_shipvals)
	shipvals[UTILITY].s_cloakcost = 180;
    else
	shipvals[UTILITY].s_cloakcost = 130;  /* was 90 (MDM) */
    shipvals[UTILITY].s_scanrange = 2000;
    shipvals[UTILITY].s_numports = 2;
    shipvals[UTILITY].s_letter = 'u';
    shipvals[UTILITY].s_desig1 = 'U';
    shipvals[UTILITY].s_desig2 = 'T';
    shipvals[UTILITY].s_bitmap = 12;
    shipvals[UTILITY].s_width = 20;
    shipvals[UTILITY].s_height = 20;
    shipvals[UTILITY].s_timer = 7;
    shipvals[UTILITY].s_maxnum = 1;
    shipvals[UTILITY].s_rank = 2;
    shipvals[UTILITY].s_numdefn = 2;
    shipvals[UTILITY].s_numplan = 1;
    if (configvals->warpdrive)
	shipvals[UTILITY].s_nflags = SFNCANWARP | SFNHASPHASERS | SFNCANREPAIR | SFNCANFUEL;
    else
	shipvals[UTILITY].s_nflags = SFNHASPHASERS | SFNCANREPAIR | SFNCANFUEL;

/* Comprehensive definition of Gunboat */
    shipvals[PATROL].s_alttype = 0;
    strcpy(shipvals[PATROL].s_name, "Patrol Ship");
    shipvals[PATROL].s_turns = 1500000;
    shipvals[PATROL].s_imp.acc = 350;
    shipvals[PATROL].s_imp.dec = 400;
    shipvals[PATROL].s_imp.cost = 1;
    shipvals[PATROL].s_imp.maxspeed = 13;
    shipvals[PATROL].s_imp.etemp = 1000;
    shipvals[PATROL].s_after.acc = 800;
    shipvals[PATROL].s_after.dec = 350;
    shipvals[PATROL].s_after.cost = 18;
    shipvals[PATROL].s_after.maxspeed = 15;
    shipvals[PATROL].s_after.etemp = 30000;
    shipvals[PATROL].s_warp.acc = 10000;
    shipvals[PATROL].s_warp.dec = 500;
    if (configvals->bronco_shipvals) {
	shipvals[PATROL].s_warp.maxspeed = 21;
	shipvals[PATROL].s_warpprepspeed = 2;
    }
    else {
	shipvals[PATROL].s_warp.maxspeed = 35;	/* was: 30; (BG) */
	shipvals[PATROL].s_warpprepspeed = 3;
    }
    shipvals[PATROL].s_warp.cost = 22;
    shipvals[PATROL].s_warp.etemp = 35000;
    shipvals[PATROL].s_warpinitcost = 800;
    shipvals[PATROL].s_warpinittime = 20;
    shipvals[PATROL].s_mass = 1000;
    shipvals[PATROL].s_tractstr = 1500;
    shipvals[PATROL].s_tractrng = 0.75;
    shipvals[PATROL].s_tractcost = 2;
    shipvals[PATROL].s_tractetemp = 1000;
    shipvals[PATROL].s_torp.damage = 30;
    shipvals[PATROL].s_torp.speed = 15;
    shipvals[PATROL].s_torp.cost = 180;
    shipvals[PATROL].s_torp.fuse = 18;
    shipvals[PATROL].s_torp.wtemp = 10;
    shipvals[PATROL].s_torp.wtemp_halfarc = 32;
    shipvals[PATROL].s_torp.wtemp_factor = 9;
    shipvals[PATROL].s_torp.aux = 0;
    shipvals[PATROL].s_phaser.damage = 50;
    shipvals[PATROL].s_phaser.speed = 5000;
    shipvals[PATROL].s_phaser.cost = 300;
    shipvals[PATROL].s_phaser.fuse = 8;
    shipvals[PATROL].s_phaser.wtemp = 45;
    shipvals[PATROL].s_missile.damage = 50;
    shipvals[PATROL].s_missile.speed = 17;
    shipvals[PATROL].s_missile.cost = 450;
    shipvals[PATROL].s_missile.fuse = 50;
    shipvals[PATROL].s_missile.wtemp = 50;
    shipvals[PATROL].s_missile.count = 6;
    shipvals[PATROL].s_missile.aux = 1;
    shipvals[PATROL].s_missilestored = 6;
    shipvals[PATROL].s_plasma.damage = -1;
    shipvals[PATROL].s_plasma.speed = 0;
    shipvals[PATROL].s_plasma.cost = 0;
    shipvals[PATROL].s_plasma.fuse = 0;
    shipvals[PATROL].s_plasma.wtemp = 0;
    shipvals[PATROL].s_plasma.aux = 0;
    shipvals[PATROL].s_maxwpntemp = 1000;
    shipvals[PATROL].s_wpncoolrate = 3;
    shipvals[PATROL].s_maxegntemp = 1500;
    shipvals[PATROL].s_egncoolrate = 8;
    shipvals[PATROL].s_maxfuel = 4000;
    shipvals[PATROL].s_recharge = 10;
    shipvals[PATROL].s_mingivefuel = 0;
    shipvals[PATROL].s_takeonfuel = 120;
    if (configvals->fuel_explosions) {
	shipvals[PATROL].s_expldam = 35;
	shipvals[PATROL].s_fueldam = 30;
    }
    else {
	shipvals[PATROL].s_expldam = 60;
	shipvals[PATROL].s_fueldam = 0;
    }
    shipvals[PATROL].s_armyperkill = 1;
    shipvals[PATROL].s_maxarmies = 1;
    shipvals[PATROL].s_bomb = 0;
    shipvals[PATROL].s_bombflags = 0;
    shipvals[PATROL].s_repair = 50;
    shipvals[PATROL].s_maxdamage = 40;
    shipvals[PATROL].s_maxshield = 50;
    shipvals[PATROL].s_shieldcost = 2;
    shipvals[PATROL].s_detcost = 100;
    shipvals[PATROL].s_detdist = 1750;
    if (configvals->bronco_shipvals)
	shipvals[PATROL].s_cloakcost = 40;
    else
	shipvals[PATROL].s_cloakcost = 30;
    shipvals[PATROL].s_scanrange = -1;
    shipvals[PATROL].s_numports = 0;
    shipvals[PATROL].s_letter = 'p';
    shipvals[PATROL].s_desig1 = 'P';
    shipvals[PATROL].s_desig2 = 'T';
    shipvals[PATROL].s_bitmap = 13;
    shipvals[PATROL].s_width = 20;
    shipvals[PATROL].s_height = 20;
    shipvals[PATROL].s_timer = 0;
    shipvals[PATROL].s_maxnum = 32;
    shipvals[PATROL].s_rank = 0;
    shipvals[PATROL].s_numdefn = 0;
    shipvals[PATROL].s_numplan = 0; /* 1; was 1 but everybody bitched (BG) */
    if (configvals->warpdrive)
	shipvals[PATROL].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL |  SFNHASPHASERS | SFNHASMISSILE | SFNMASSPRODUCED | SFNCANWARP;
    else
	shipvals[PATROL].s_nflags = SFNCANORBIT | SFNARMYNEEDKILL |  SFNHASPHASERS | SFNHASMISSILE | SFNMASSPRODUCED;
}

/*----------END OF FILE-----*/
