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
#include "ntserv.h"
#include "data.h"
#include "shmem.h"

extern char start_login[], start_name[];	/* change 4/14/91 TC */
extern int goAway;		/* change 4/14/91 TC */

#ifdef AUTHORIZE
static void 
check_authentication(void)
{
    /* if (!configvals->binconfirm) testtime=0; */
    if (testtime == -1) {
	struct reserved_spacket sp;

	if (site_rsa_exempt()) {
	    printf("site exempt from RSA authentication\n");
	    strcpy(RSA_client_type, "site/player is RSA exempt");
	    testtime = 0;
	} else
        {
            /*
              Give reasonable period of time to respond to query (and
              test code if they need to)
            */

            testtime = 200;
            makeReservedPacket(&sp);
            memcpy(testdata, sp.data, KEY_SIZE);
            sendClientPacket((struct player_spacket *) & sp);
        } /* RSA EXEMPTION */
    } else if (testtime != 0) {
	testtime--;
	if (testtime == 0) {
	    /* User failed to respond to verification query.  Bye! */
	    if (!configvals->binconfirm)
		me->p_stats.st_flags |= ST_CYBORG;	/* mark for reference
							   7/27/91 TC */
	    else {
		me->p_explode = 10;
		me->p_whydead = KQUIT;
		me->p_status = PEXPLODE;
		fprintf(stderr, "User binary failed to verify\n");
		if (RSA_Client == 1)
		    warning("No customized binaries.  Please use a blessed one.");
		else if (RSA_Client == 2)
		    warning("Wrong Client Version Number!");
		else
		    warning("You need a spiffy new RSA client for this server!");
	    }
	}
    }
}
#endif				/* AUTHORIZE */

static void 
selfdestruct_countdown(void)
{
    char    buf[180];
    if (!(me->p_flags & PFSELFDEST))
	return;

    if ((me->p_updates >= selfdest) ||
	((me->p_flags & PFGREEN) && (me->p_damage == 0)
	 && (me->p_shield == me->p_ship.s_maxshield))) {
	me->p_flags &= ~PFSELFDEST;
	me->p_explode = 10;
	me->p_whydead = KQUIT;
	me->p_status = PEXPLODE;
    }
    else {
	switch ((selfdest - me->p_updates) / 10) {
	case 5:
	case 4:
	    imm_warning("You notice everyone on the bridge is staring at you.");
	    break;
	default:
	    sprintf(buf, "Stand by ... Self destruct in %d seconds",
		    (selfdest - me->p_updates) / 10);
	    imm_warning(buf);
	    break;
	}
    }
}

static void 
warp_powerup(void)
{
    static int sequence = 0;
    int     time = myship->s_warpinittime;

    if (me->p_warptime <= 0 && sequence == 0)
	return;

    if (me->p_speed > myship->s_warpprepspeed && !(me->p_flags & PFWARP)) {
	if (sequence < 1) {
	    warning("Prepping for warp jump");
	    sequence = 1;
	}
    }
    else if (me->p_warptime >= time * 9 / 10 /* 37 */ ) {
	if (sequence < 2) {
	    warning("Starting power buildup for warp");
	    sequence = 2;
	}
    }
    else if (me->p_warptime >= time * 3 / 4 /* 30 */ ) {
	if (sequence < 3) {
	    imm_warning("Warp power buildup at 30%");
	    sequence = 3;
	}
    }
    else if (me->p_warptime >= time * 3 / 5 /* 23 */ ) {
	if (sequence < 4) {
	    imm_warning("Warp power buildup at 60%");
	    sequence = 4;
	}
    }
    else if (me->p_warptime >= time * 2 / 5 /* 16 */ ) {
	if (sequence < 5) {
	    imm_warning("Warp power buildup at 90%");
	    sequence = 5;
	}
    }
    else if (me->p_warptime >= time * 1 / 5 /* 8 */ ) {
	if (sequence < 6) {
	    imm_warning("Commander Hoek: `Prepare to surge to sub-light speed'");
	    sequence = 6;
	}
    }
    else if (me->p_warptime == 0) {
	warning((me->p_flags & PFWARP) ? "Engage" : "Warp drive aborted");
	sequence = 0;
    }
}

static void 
refit_countdown(void)
{
    char    buf[120];
    static int lastRefitValue = 0;	/* for smooth display */

    if (!(me->p_flags & PFREFITTING))
	return;

    if (lastRefitValue != (rdelay - me->p_updates) / 10) {
	lastRefitValue = (rdelay - me->p_updates) / 10;	/* CSE to the rescue? */
	switch (lastRefitValue) {
	case 3:
	case 2:
	    sprintf(buf, "Engineering:  Energizing transporters in %d seconds", lastRefitValue);
	    warning(buf);
	    break;
	case 1:
	    warning("Engineering:  Energize. [ SFX: chimes ]");
	    break;
	case 0:
	    switch (lrand48() % 5) {
	    case 0:
		warning("Wait, you forgot your toothbrush!");
		break;
	    case 1:
		warning("Nothing like turning in a used ship for a new one.");
		break;
	    case 2:
		warning("First officer:  Oh no, not you again... we're doomed!");
		break;
	    case 3:
		warning("First officer:  Uh, I'd better run diagnostics on the escape pods.");
		break;
	    case 4:
		warning("Shipyard controller:  `This time, *please* be more careful, okay?'");
		break;
	    }
	    break;
	}
    }
}

static void 
backstab_countdown(void)
{
    static int lastWarValue = 0;

    if (!(me->p_flags & PFWAR))
	return;

    if (lastWarValue != (delay - me->p_updates) / 10) {
	char   *str = 0;
	lastWarValue = (delay - me->p_updates) / 10;	/* CSE to the rescue? */
	if (lastWarValue == 0) {
	    static char buf[1024];
	    static char *msgs[7] = {
		"tires on the ether",
		"Klingon bitmaps are ugly",
		"Federation bitmaps are gay",
		"all admirals have scummed",
		"Mucus Pig exists",
		"guests advance 5x faster",
		"Iggy has infinite fuel",
	    };
	    sprintf(buf, "First Officer: laws of the universe, like '%s'.",
		    msgs[lrand48() % 7]);
	    str = buf;
	}
	else if (lastWarValue <= 9) {
	    static char *msgs[10] = {
		"First Officer:  Easy, big guy... it's just one of those mysterious",
		"Weapons officer:  Bah! [ bangs fist on inoperative console ]",
		"Weapons officer:  Just to twiddle a few bits of the ship's memory?",
		"Weapons officer:  ...the whole ship's computer is down?",
		"Weapons officer:  Not again!  This is absurd...",
	    };
	    str = msgs[(lastWarValue - 1) / 2];
	}
	else {
	    str = "urk";
	}
	if (str) {
	    warning(str);
	}
    }
}

static void 
bombing_info(void)
{
    char    buf[120];

    if (!(me->p_flags & PFBOMB))
	return;

    if (planets[me->p_planet].pl_armies < 5) {
	if (configvals->facilitygrowth && configvals->resource_bombing &&
	    planets[me->p_planet].pl_flags & (PLRESMASK | PLORESMASK))
	{
	    if(CAN_BOMB(me, FACILITIES))
  	      imm_warning("Weapons Officer: "
	                  "Bombing resources off planet, sir.");
	    else
	      imm_warning("Weapons Officer: "
	                  "We are not equipped to bomb facilities, sir.");
	}
	else {
	    sprintf(buf, "Weapons Officer: "
	                 "Bombing is ineffective. Sensor read %d armies left.",
		    planets[me->p_planet].pl_armies);
	    imm_warning(buf);
	}
    }
    else {
	sprintf(buf, "Weapons Officer: "
	             "Bombing %s...  Sensors read %d armies left.",
		planets[me->p_planet].pl_name,
		planets[me->p_planet].pl_armies);
	imm_warning(buf);
    }
}


static void 
operate_transporters(void)
{
    char    buf[120];
    int     troop_capacity = 0;

    if ((me->p_ship.s_nflags & SFNARMYNEEDKILL) &&
	(me->p_kills * myship->s_armyperkill) < myship->s_maxarmies) {
	troop_capacity = (int) (me->p_kills * myship->s_armyperkill);
    }
    else
	troop_capacity = me->p_ship.s_maxarmies;

    if (me->p_flags & PFBEAMUP) {
	if (me->p_flags & PFORBIT) {
	    if (planets[me->p_planet].pl_armies < 1) {
		sprintf(buf, "%s: Too few armies to beam up",
			planets[me->p_planet].pl_name);
		warning(buf);
		me->p_flags &= ~PFBEAMUP;
	    }
	    else if ((planets[me->p_planet].pl_armies <= 4) &&
		     (me->p_lastman != 2)) {
		strcpy(buf, "Hit beam again to beam up all armies.");
		warning(buf);
		me->p_flags &= ~PFBEAMUP;
	    }
	    else if (me->p_armies == troop_capacity) {
		sprintf(buf, "No more room on board for armies");
		warning(buf);
		me->p_flags &= ~PFBEAMUP;
	    }
	    else {
		sprintf(buf, "Beaming up.  (%d/%d)", me->p_armies, troop_capacity);
		imm_warning(buf);
	    }
	}
	else if (me->p_flags & PFDOCK) {
	    if (players[me->p_docked].p_armies == 0) {
		sprintf(buf, "Starbase %s: Too few armies to beam up",
			players[me->p_docked].p_name);
		warning(buf);
		me->p_flags &= ~PFBEAMUP;
	    }
	    else if (me->p_armies >= troop_capacity) {
		sprintf(buf, "No more room on board for armies");
		warning(buf);
		me->p_flags &= ~PFBEAMUP;
	    }
	    else {
		sprintf(buf, "Beaming up.  (%d/%d)", me->p_armies, troop_capacity);
		imm_warning(buf);
	    }
	}
    }
    if (me->p_flags & PFBEAMDOWN) {
	if (me->p_armies == 0) {
	    if (me->p_flags & PFDOCK) {
		sprintf(buf, "No more armies to beam down to Starbase %s.",
			players[me->p_docked].p_name);
		warning(buf);
	    }
	    else {
		sprintf(buf, "No more armies to beam down to %s.",
			planets[me->p_planet].pl_name);
		warning(buf);
	    }
	    me->p_flags &= ~PFBEAMDOWN;
	}
	else if (me->p_flags & PFORBIT) {
	    sprintf(buf, "Beaming down.  (%d/%d) %s has %d armies left",
		    me->p_armies,
		    troop_capacity,
		    planets[me->p_planet].pl_name,
		    planets[me->p_planet].pl_armies);
	    imm_warning(buf);
	}
	else if (me->p_flags & PFDOCK) {
	    if (players[me->p_docked].p_armies ==
		players[me->p_docked].p_ship.s_maxarmies) {
		sprintf(buf, "Transporter Room:  Starbase %s reports all troop bunkers are full!",
			players[me->p_docked].p_name);
		warning(buf);
		me->p_flags &= ~PFBEAMDOWN;
	    }
	    else {
		sprintf(buf, "Transfering ground units.  (%d/%d) Starbase %s has %d armies left",
			me->p_armies,
			troop_capacity,
			players[me->p_docked].p_name,
			players[me->p_docked].p_armies);
		imm_warning(buf);
	    }
	}
    }
}

static void 
decelerate_at_range(int dist)
{
    int     speed;
    int     maximp = myship->s_imp.maxspeed;

#define FUDGEFACT	1.15

    if (me->p_flags & PFWARP) {
	if (configvals->warpdecel) {
	    /*
	       int impdist = 1000*(maximp*10*WARP1*maximp*10*WARP1)/ (2 *
	       myship->s_imp.dec*10*10*warp1);
	    */

	    int     impdist = FUDGEFACT * (maximp * maximp - 2 * 2) * 500 * WARP1 / myship->s_imp.dec;

	    for (speed = maximp;
		 speed < me->p_desspeed && speed < me->p_speed;
		 speed++) {
		int     decdist;
		decdist = FUDGEFACT * 500 * ((speed * speed) - (maximp * maximp)) * WARP1 /
		    myship->s_warp.dec;
		if (dist - impdist - ENTORBDIST < decdist)
		    break;
	    }
	    if (speed == maximp && speed < me->p_speed)
		warning("Approaching planet, dropping out of warp");
	    if (speed < me->p_desspeed &&
		speed < me->p_speed) {
		/* printf("(W) curr: %d, ideal %d\n", me->p_speed, speed); */
		me->p_desspeed = speed;
	    }
	}
	else {
	    int     impdist;
	    maximp = (3 * maximp + myship->s_warp.maxspeed) / 4;
	    impdist = FUDGEFACT * ((maximp) * (maximp) - 2 * 2) *
		500 * WARP1 / myship->s_imp.dec;
	    if (dist - impdist - ENTORBDIST < 0)
		me->p_flags &= ~PFWARP;
	}
    }
    else {
	for (speed = 2; speed < me->p_desspeed && speed < me->p_speed; speed++) {
	    int     decdist;
	    decdist = FUDGEFACT * 500 * (speed * speed - 2 * 2) * WARP1 / myship->s_imp.dec;
	    if (dist - ENTORBDIST < decdist)
		break;
	}
	if (speed < me->p_desspeed &&
	    speed < me->p_speed) {
	    /* printf("(I) curr: %d, ideal %d\n", me->p_speed, speed); */
	    me->p_desspeed = speed;
	}
    }
}

static void 
follow_player(void)
{
    struct player *pl;
    int     dist;

    if (!(me->p_flags & PFPLOCK))
	return;

    /* set course to player x */

    pl = &players[me->p_playerl];
    if (pl->p_status != PALIVE)
	me->p_flags &= ~PFPLOCK;
    if (allows_docking(pl->p_ship)) {
	dist = ihypot(me->p_x - pl->p_x, me->p_y - pl->p_y);

	decelerate_at_range(dist);

	if ((dist < DOCKDIST) && (me->p_speed <= 2)) {
	    orbit();
	    me->p_flags &= ~PFPLOCK;
	}
    }
    if (me->p_flags & PFPLOCK)
	set_course(getcourse(pl->p_x, pl->p_y, me->p_x, me->p_y));

}

static void 
follow_planet(void)
{				/* follow a planet?  How silly. Will it
				   perhaps outmaneuver you? */
				/* it might if MDM's planet mover code is
				   on 1/10/2000 - rpg ;) */
    struct planet *pln;
    int     dist;
/*  int speed = me->p_speed;*/

    if (!(me->p_flags & PFPLLOCK))
	return;

    /* set course to planet x */

    pln = &planets[me->p_planet];
    dist = ihypot(me->p_x - pln->pl_x, me->p_y - pln->pl_y);

    decelerate_at_range(dist);

    if ((dist < ENTORBDIST) && (me->p_speed <= 2)) {
	orbit();
	me->p_flags &= ~PFPLLOCK;
    }
    else {
	int     course = getcourse(pln->pl_x, pln->pl_y, me->p_x, me->p_y);
	set_course(course);
    }
}

/* These are routines that need to be done on interrupts but
   don't belong in the redraw routine and particularly don't
   belong in the daemon. */

void
auto_features(void)
{
    selfdestruct_countdown();

    warp_powerup();

    /* provide a refit countdown 4/6/92 TC */
    refit_countdown();

    /* provide a war declaration countdown 4/6/92 TC */
    backstab_countdown();

    /* give certain information about bombing or beaming */
    bombing_info();

    operate_transporters();

    if (me->p_flags & PFREPAIR) {
	if ((me->p_damage == 0) && (me->p_shield == me->p_ship.s_maxshield))
	    me->p_flags &= ~PFREPAIR;
    }
    if (me->p_status != POBSERVE) {
	/* these mess up the observer mode */
	follow_player();
	follow_planet();
    }

#if defined(CLUECHECK1) || defined(CLUECHECK2)
    if (
	!status2->league && 
	configvals->cluecheck)
	countdown_clue();
#endif
}

void
intrupt(void)
{
#ifdef AUTHORIZE
    check_authentication();
#endif

    if (me->p_status == PFREE) {
	me->p_ghostbuster = 0;
	me->p_status = PDEAD;
    }
    /* Change 4/14/91 TC: fixing 2 players/1 slot bug here, since this is */
    /* where the ghostbust check is */

#if 0
    if ((strcmp(me->p_login, start_login) != 0) ||
	(strcmp(me->p_name, start_name) != 0)) {
	struct pstatus_spacket pstatus;
	goAway = 1;		/* get rid of this client! */
	warning("Sorry, your slot has been taken by another player.");
	pstatus.type = SP_PSTATUS;
	pstatus.pnum = 0;
	pstatus.status = PDEAD;
	sendClientPacket(&pstatus);
	longjmp(env, 0);
    }
#endif

    if (me->p_status == PEXPLODE || me->p_status == PDEAD) {
	inputMask = 0;
    }
    if (((me->p_status == PDEAD) || (me->p_status == POUTFIT))
	&& (me->p_ntorp <= 0) && (me->p_nplasmatorp <= 0)) {
	death();
    }
    auto_features();
    updateClient();
}
