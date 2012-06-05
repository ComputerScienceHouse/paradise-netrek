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
#include "data.h"
#include "shmem.h"

/*----------------------------------NUMSHIPS------------------------------*/
/*  This function counts the number of players on a team.  */

static int 
numShips(int owner)		/* the team to count for */
{
    int     i;			/* looping var */
    int     num;		/* to hold player count */
    struct player *p;		/* to point to players */

    num = 0;			/* zero the count */
    for (i = 0, p = players; i < MAXPLAYER; i++, p++) {	/* go throgh players */
	if (p->p_status != PFREE && p->p_team == owner)	/* if alive then */
	    num++;		/* inc player count */
    }
    return (num);		/* return number of ships */
}




/*------------------------------NUMPLANETS--------------------------------*/
/*  This function counts the number of planets a team has.  */

int 
numPlanets(int owner)		/* the team to check for */
{
    int     i;			/* looping var */
    int     num;		/* to hold count */
    struct planet *p;		/* to point to a planet */

    num = 0;			/* no planets found yet */
    for (i = 0, p = planets; i < MAXPLANETS; i++, p++) {
	if (p->pl_owner == owner)	/* if planet owned by team then inc */
	    num++;
    }
    return (num);		/* return number of planets */
}

/*-------------------------------------------------------------------------*/

/*
   Suspends warp prep [BDyess]
*/
static void 
suspendPrep(void)
{
    /* check to see if the server allows warp prep to be suspended [BDyess] */
    if (!configvals->warpprep_suspendable)
	return;
    if (me->p_flags & PFWARPPREP) {
	me->p_flags |= PFWPSUSPENDED;	/* turn on suspended flag */
	warning("Warp prep suspended.");
    }
    else {
	warning("Not in warp prep!");
    }
}

/*
   Unsuspends warp prep [BDyess]
*/
static void
unsuspendPrep(void)
{
    /* check to see if the server allows warp prep to be suspended [BDyess] */
    if (!configvals->warpprep_suspendable)
	return;
    if (me->p_flags & PFWARPPREP) {
	me->p_flags &= ~PFWPSUSPENDED;	/* turn off suspended flag */
	warning("Warp prep continued.");
    }
    else {
	warning("Not in warp prep!");
    }
}

/*-----------------------------VISIBLE FUNCTIONS---------------------------*/

/*--------------------------------SETSPEED---------------------------------*/
/*  This function takes the desired speed from the client and translates
it into impulse, afterburners, or warp.  99 = warp.  98 = afterburners.
97 = suspend warp prep, 96 = resume warp prep,
0-x = impulse.  This function is used for both requests to set a ships
speed from the client and from the server.  The client requests are requests
that are received directly from the client.  A speed of 99 is treated as
a request for warp speed.  A request of 98 is a request for afterburners. */


/* args:
    int     speed;		 the desired speed 
    int     type;		 type of request. 0=server 1=client */
void 
set_speed(int speed, int type)
{
    if (me->p_flags & PFDOCK) {	/* if docked then */
	if (players[me->p_docked].p_speed > 4) {	/* you cannot get off >=
							   4 */
	    warning("You are going too fast to disengage safely");
	    return;
	}
	else
	    undock_player(me);
    }
    speed = (speed < 0) ? 0 : speed;	/* no negative speeds */
    /* suspend warp  [BDyess] */
    if (speed == 97 && configvals->warpprep_suspendable) {	/* suspend warp */
	suspendPrep();
    }
    else if (speed == 96 && configvals->warpprep_suspendable) {
	/* unsuspend warp [BDyess] */
	unsuspendPrep();
    }
    else if ((speed == 98) && (configvals->afterburners)) {  /* afterburners */
	/* turn off warp, warpprep, and warp suspended flags */
	me->p_flags &= ~(PFWARP | PFWARPPREP | PFWPSUSPENDED);
	me->p_warptime = 0;
	me->p_flags |= PFAFTER;	/* turn on after burners */
	me->p_desspeed = me->p_ship.s_after.maxspeed;	/* set speed of
							   afterburn */
	/*
	   go to warp if speed 99 is sent, OR if the speed sent is greater
	   than max impulse and the VARIABLE_WARP sysdef is set, AND the ship
	   can warp [BDyess]
	*/
    }
    else if ( (speed == 99
	       || (configvals->variable_warp
		   && speed > me->p_ship.s_imp.maxspeed
		   && me->p_flags & (PFWARPPREP|PFWARP|PFWPSUSPENDED))
	       )
	     && (me->p_ship.s_nflags & SFNCANWARP)) {
	int maxspeed = me->p_ship.s_warp.maxspeed;
	int preptime = myship->s_warpinittime;	/* timer before warping */

	if(configvals->warpzone) {
	  if(me->p_zone < 0) {
	    warning("The enemy is neutralizing our warp field, sir!");
	    return;
	  } else if(me->p_zone > 0) {
	    maxspeed = maxspeed * 3 / 2;  /* can go faster with boost [BDyess]*/
	    if(maxspeed > 99) maxspeed = 99;
	    preptime /= 2;  /* takes less time to build up when you have help */
	    if(preptime == 0) preptime = 1;
	    warning("warp boost!");
	  }
	}
	    
	speed = (speed > maxspeed)
	    ? maxspeed
	    : speed;		/* cap the request at max warp */

	me->p_warpdesspeed = speed;	/* remember how fast to warp for
					   later [BDyess] */
	if (me->p_warptime > 0) {
	    warning("Hold your horses, Speed Racer.  These things take time.");
	    return;
	}
	if (me->p_flags & PFWARP) {
	    me->p_desspeed = speed;
	    return;
	}
	if (me->p_damage > me->p_ship.s_maxdamage / 2) {
	    warning("We are too damaged to risk warp speeds, Captain.");
	    return;
	}
	if (me->p_fuel < me->p_ship.s_warpinitcost) {
	    warning("Not enough fuel for warp, Captain!");
	    return;
	}
	me->p_desspeed = myship->s_warpprepspeed;	/* slow ship down */
	me->p_flags |= PFWARPPREP;	/* set warp prep flag [BDyess] */
	if (!configvals->cloakduringwarpprep)
	    cloak_off();

	if (me->p_speed < me->p_desspeed)
	    me->p_speed = me->p_desspeed;

	/* turn off after burners and warpprep suspended flag [BDyess] */
	me->p_flags &= ~(PFAFTER | PFWPSUSPENDED);
	me->p_warptime = preptime;	/* timer before warping */
	me->p_fuel -= me->p_ship.s_warpinitcost;	/* subtract off fuel */
	warning("Warp drive engaged!");	/* let him know its working */
    }
    else if (type == 0) {	/* else if server request */
	me->p_desspeed = speed;	/* grant speed request */
    }
    else {			/* else client request for impulse */

	if (!configvals->warpdecel)
	    me->p_flags &= ~(PFWARP | PFAFTER);

	me->p_warptime = 0;
	me->p_flags &= ~(PFWARPPREP|PFWPSUSPENDED);	/* turn off warpprep flag [BDyess] */
	speed = (speed > me->p_ship.s_imp.maxspeed) ? me->p_ship.s_imp.maxspeed :
	    speed;
	me->p_desspeed = speed;	/* set the speed */
    }
    me->p_flags &= ~(PFREPAIR | PFBOMB | PFORBIT | PFDOCK | PFBEAMUP | PFBEAMDOWN);
    me->p_lastman = 0;
}




/*--------------------------------SET_COURSE-------------------------------*/
/*  Set the players course.  This function takes a direction and sets the
players direction to it.  Certain flags are cleared.  */

void 
set_course(unsigned char dir)
{
    me->p_desdir = dir;		/* set the desired direction */
    if (me->p_flags & PFDOCK) {	/* if player is docked then he cannot */
	if (players[me->p_docked].p_speed > 4) {	/* undock if dockee
							   going fast */
	    warning("It's unsafe to disengage from bases while moving more that warp 4.");
	    return;
	}
	else
	    undock_player(me);

    }
    me->p_flags &= ~(PFBOMB | PFORBIT | PFDOCK | PFBEAMUP | PFBEAMDOWN);
    me->p_lastman = 0;		/* not beamin all armies up */
}




/*----------------------------------SHIELD_UP-----------------------------*/
/*  This function raises the players shields.  */

void
shield_up(void)
{
    me->p_flags |= PFSHIELD;	/* some flags go up and some down */
    me->p_flags &= ~(PFBOMB | PFREPAIR | PFBEAMUP | PFBEAMDOWN);
    me->p_lastman = 0;		/* not beaming up all armies */
}




/*---------------------------------SHIELD_DOWN----------------------------*/
/*  This function lowers the players shields.  */

void
shield_down(void)
{
    me->p_flags &= ~PFSHIELD;	/* shield flag clear */
}



/*--------------------------------BOMB_PLANET------------------------------*/
/*  This function sets the bomb flag if certain conditions are met, such as
you are bombing enemy planets.  */

void 
bomb_planet(void)
{
    static int bombsOutOfTmode = 0;	/* confirm bomb out of t-mode */
    int     owner;		/* owner of planet */

    owner = planets[me->p_planet].pl_owner;	/* get planet's owner */
    if (!(me->p_flags & PFORBIT)) {	/* can't bomb anything in */
	warning("Must be orbiting to bomb");	/* open space, now can you? */
	return;
    }
    if (me->p_team == planets[me->p_planet].pl_owner) {
	warning("Can't bomb your own armies.  Have you been reading Catch-22 again?");	/* can't bomb own armies */
	return;
    }
    if (!((me->p_swar | me->p_hostile) & owner) && (owner != 0)) {
	warning("Must declare war first (no Pearl Harbor syndrome allowed here).");
	return;			/* can't bomb friendlies */
    }
    if ((!status->tourn) && (bombsOutOfTmode == 0)) {
	warning("Bomb out of T-mode?  Please verify your order to bomb.");
	bombsOutOfTmode++;	/* warning about bombing out of T */
	return;
    }
    if ((!status->tourn) && (bombsOutOfTmode == 1)) {
	warning("Hoser!");	/* if he really wants to bomb, then */
	bombsOutOfTmode++;	/* let him bomb out of T.  Hoser */
    }				/* warning */
    if (!numShips(planets[me->p_planet].pl_owner)
	&& (planets[me->p_planet].pl_owner != NOBODY)) {
	warning("You can't break the peace treaty, Captain");
	return;
    }
    if (me->p_ship.s_bombflags == 0) {
        warning("This ship is not equipped with any bombing equipment");
	return;
    }
    if (status->tourn)		/* reset the bombs out of t-mode */
	bombsOutOfTmode = 0;	/* variable */
    me->p_flags |= PFBOMB;	/* set the bomb flag */
    planets[me->p_planet].pl_hostile |= me->p_team;
    me->p_flags &= ~(PFSHIELD | PFREPAIR | PFBEAMUP | PFBEAMDOWN);
}




/*---------------------------------BEAM_UP---------------------------------*/
/*  This function set the beam up flag if it is allowable to beam men up.  */

void 
beam_up(void)
{
    if(!status2->starttourn && configvals->nopregamebeamup) {
	warning("You can't beam up armies before a game starts");
	return;
    }
    if (!(me->p_flags & (PFORBIT | PFDOCK))) {
	warning("Must be orbiting or docked to beam up.");
	return;			/* have to be orbiting or docked */
    }
    if (me->p_flags & PFORBIT) {/* if orbiting and not owner */
	if (me->p_team != planets[me->p_planet].pl_owner) {
	    warning("Those aren't our men.");
	    return;
	}
	if (me->p_lastman == 0)	/* if we have not signalled to pick */
	    me->p_lastman = 1;	/* up all armies */
	else if (planets[me->p_planet].pl_armies <= 4) {	/* else if less than 5 */
	    if (numPlanets((int) me->p_team && configvals->beamlastarmies) == 1) {
		warning("You can't take those armies, they're on your only planet!");
		return;
	    }
	    me->p_lastman = 2;	/* set status to beaming up all */
	}
    }
    else if (me->p_flags & PFDOCK) {	/* if docked must be to own team ship */
	if (me->p_team != players[me->p_docked].p_team) {
	    warning("Comm Officer: Uhhh, they aren't our men!");
	    return;
	}
    }
    me->p_flags |= PFBEAMUP;	/* set the beam up flag */
    me->p_flags &= ~(PFSHIELD | PFREPAIR | PFBOMB | PFBEAMDOWN);
}




/*--------------------------------BEAM_DOWN--------------------------------*/
/*  This function sets the beam up flag if certain conditions are met*/

void 
beam_down(void)
{
    int     i;			/* looping var */
    struct planet *pl;		/* to access planets */
    int     total;		/* for total number of planets */
    char    buf[80];

    if (!(me->p_flags & (PFORBIT | PFDOCK))) {
	warning("Must be orbiting or docked to beam down.");
	return;			/* have to be docked or orbiting */
    }
    if ((!((me->p_swar | me->p_hostile) & planets[me->p_planet].pl_owner))
	&& (me->p_team != planets[me->p_planet].pl_owner)
	&& (planets[me->p_planet].pl_owner != NOBODY)) {
	warning("You can't beam down, you have not declared war");
	return;			/* no beaming down if not hostile */
    }
    if (!numShips(planets[me->p_planet].pl_owner)
	&& (planets[me->p_planet].pl_owner != NOBODY)) {
	warning("You can't break the peace treaty, Captain");
	return;
    }
    if (me->p_flags & PFDOCK) {
	if (me->p_team != players[me->p_docked].p_team) {
	    warning("Comm Officer: Starbase refuses permission to beam our troops over.");	/* no beaming to foreign
												   bases */
	    return;
	}
    }

    else if(configvals->planetlimittype) {	/* new planet limit */
	total = numPlanets(me->p_team);

	if(total >= configvals->planetsinplay
	    && planets[me->p_planet].pl_owner == NOBODY
	    && planets[me->p_planet].pl_armies) {
	    sprintf(buf, "You may only take enemy planets when your team has %d or more planets.",
		configvals->planetsinplay);
	    warning(buf);
	    me->p_flags &= ~(PFBOMB);
	    return;
	}
    } else {
	int ns[MAXTEAM];
	for(i = 0; i < NUMTEAM; i++)
	    ns[1<<i] = numShips(1<<i);

	total = 0;
	for (i = 0, pl = &planets[i]; i < NUMPLANETS; i++, pl++) {
	    if (pl->pl_owner && ns[pl->pl_owner] > configvals->tournplayers - 1)
		total++;
	}
	if ((total >= configvals->planetsinplay)
            && (!(me->p_flags & PFDOCK))
	    && (planets[me->p_planet].pl_owner == NOBODY)) {
	    sprintf(buf, "Sorry, there are already %d planets in play.",
		    configvals->planetsinplay);
	    warning(buf);
	    me->p_flags &= ~(PFBOMB);
	    return;
	}
    }
    me->p_flags |= PFBEAMDOWN;	/* set beam down flag */
    me->p_flags &= ~(PFSHIELD | PFREPAIR | PFBOMB | PFBEAMUP);
    me->p_lastman = 0;
}




/*---------------------------------REPAIR---------------------------------*/
/*  This function sets the reapair flag and turns off other flags.  You cannot
repair while in warp.  */

void 
repair(void)
{
    if (me->p_flags & PFWARP && !configvals->repair_during_warp) 
    {	/* no repairing in warp */
	warning("You cannot repair while in warp!");
	return;
    }
    if (me->p_flags & PFWARPPREP && !configvals->repair_during_warp_prep)
    {   /* no repairing in warp prep */
        warning("You cannot repair while in warp prep!");
	return;
    }
    me->p_desspeed = 0;		/* speed goes to zero */
    me->p_warptime = 0;		/* turn off warp prep */
    me->p_flags |= PFREPAIR;	/* reair flag goes up */
    me->p_flags &= ~(PFSHIELD | PFBOMB | PFBEAMUP | PFBEAMDOWN | PFPLOCK | PFPLLOCK | PFWARP | PFAFTER | PFWARPPREP | PFWPSUSPENDED);
    me->p_lastman = 0;		/* not beaming all armies */
}




/*--------------------------------REPAIR_OFF------------------------------*/
/*  This function takes the repair flag off.  */

void 
repair_off(void)
{
    me->p_flags &= ~PFREPAIR;
}




/*---------------------------------CLOAK_ON-------------------------------*/
/*  This function turns cloak on.  Tractors and pressors are turned off.  */

void
cloak_on(void)
{
    if (me->p_warptime && !configvals->cloakduringwarpprep) {
	warning("Can't cloak during warp preparation.");
	return;
    }
    if (me->p_flags & PFWARP && !configvals->cloakwhilewarping) {
	warning("Can't cloak while warp engines are engaged.");
	return;
    }
    me->p_flags |= PFCLOAK;
    me->p_flags &= ~(PFTRACT | PFPRESS);
}




/*---------------------------------CLOAK_OFF------------------------------*/
/*  This function turns the cloak off.  */

void 
cloak_off(void)
{
    me->p_flags &= ~PFCLOAK;
}




/*-------------------------------LOCK_PLANET-----------------------------*/
/*  This function locks the player onto a planet.  */

void 
lock_planet(int planet)		/* planet locking onto */
{
    char    buf[80];		/* to sprintf into */

    if ((planet < 0) || (planet >= NUMPLANETS))	/* bad planet number? */
	return;

    if (me->p_status == POBSERVE) {
	if (planets[planet].pl_owner != me->p_team) {
	    warning("Unable to observe enemy planets.");
	    me->p_flags &= ~(PFPLLOCK | PFPLOCK);
	    return;
	}
	else if (!(planets[planet].pl_owner & me->p_teamspy)) {
	    warning("That team has barred you from observing.");
	    me->p_flags &= ~(PFPLLOCK | PFPLOCK);
	    return;
	}
    }

    me->p_flags |= PFPLLOCK;	/* turn on the lock */
    me->p_flags &= ~(PFPLOCK | PFORBIT | PFBEAMUP | PFBEAMDOWN | PFBOMB);
    me->p_lastman = 0;
    me->p_planet = planet;	/* set planet locked onto */
    sprintf(buf, "Locking onto %s", planets[planet].pl_name);
    warning(buf);		/* send message to player */
}




/*-------------------------------LOCK_PLAYER-------------------------------*/
/*  This function locks the player onto another player.  */

void 
lock_player(int player)		/* player locking onto */
{
    char    buf[80];

    if ((player < 0) || (player >= MAXPLAYER))	/* bad planet num? */
	return;
    if (players[player].p_status != PALIVE)	/* player not alive? */
	return;
    if (players[player].p_flags & PFCLOAK)	/* player is cloaked */
	return;

    if (me->p_status == POBSERVE && players[player].p_team != me->p_team) {
	sprintf(buf, "Unable to observe enemy players.");
	warning(buf);
	me->p_flags &= ~(PFPLLOCK | PFPLOCK);
	return;
    }

    me->p_flags |= PFPLOCK;	/* set player lock flag */
    me->p_flags &= ~(PFPLLOCK | PFORBIT | PFBEAMUP | PFBEAMDOWN | PFBOMB);
    me->p_lastman = 0;		/* turn off taking all armies */
    me->p_playerl = player;	/* set player loked oto */
    /* player locking onto own base */
    if ((players[player].p_team == me->p_team) &&
	allows_docking(players[player].p_ship))
	sprintf(buf, "Locking onto %s (%s) (docking is %s)",
		players[player].p_name, twoletters(&players[player]),
	     (players[player].p_flags & PFDOCKOK) ? "enabled" : "disabled");
    else			/* message for locking onto player */
	sprintf(buf, "Locking onto %s (%s)", players[player].p_name,
		twoletters(&players[player]));
    warning(buf);
}




/*-------------------------------TRACTOR_PLAYER----------------------------*/
/*  This function does the tractoring for the player.  There are a few
times when the tractor cannot be used.  */

void 
tractor_player(int player)
{
    struct player *victim;

    if (weaponsallowed[WP_TRACTOR] == 0)	/* tractors allowed?  */
	return;
    if ((player < 0) || (player > MAXPLAYER)) {	/* out of bounds = cancel */
	me->p_flags &= ~(PFTRACT | PFPRESS);
	return;
    }
    if (me->p_flags & PFCLOAK)	/* no tractoring while */
	return;			/* cloaked */
    if (player == me->p_no)	/* can't tractor yourself */
	return;
    victim = &players[player];	/* get player number */
    if (victim->p_flags & PFCLOAK)
	return;
    if (ihypot(me->p_x - victim->p_x, me->p_y - victim->p_y) <
	(TRACTDIST) * me->p_ship.s_tractrng) {
	if (me->p_flags & PFDOCK && players[me->p_docked].p_speed > 4) {
	    warning("It's unsafe to tractor while docked and base is moving more then warp 4.");
	    return;
	}
	me->p_tractor = player;	/* set victim number */
	me->p_flags |= PFTRACT;	/* set tractor flag */
    }
}




/*---------------------------------PRESSOR--------------------------------*/
/*  This function activates the tractors for the player.  There are a
number of conditions where tractors will not work.  */

void 
pressor_player(int player)	/* the player to pressor */
{
    int     target;
    struct player *victim;

    if (weaponsallowed[WP_TRACTOR] == 0) {	/* tractors allowed? */
	warning("Pressor beams haven't been invented yet.");
	return;
    }
    target = player;		/* save target */
    if ((target < 0) || (target > MAXPLAYER)) {	/* out of bounds = cancel */
	me->p_flags &= ~(PFTRACT | PFPRESS);
	return;
    }
    if (me->p_flags & PFPRESS)	/* already pressoring? */
	return;
    if (me->p_flags & PFCLOAK) {/* cloaked? */
	warning("Weapons's Officer:  Cannot pressor while cloaked, sir!");
	return;
    }
    victim = &players[target];	/* get victim's struct */
    if (victim->p_flags & PFCLOAK)	/* victim cloaked */
	return;
    if (ihypot(me->p_x - victim->p_x, me->p_y - victim->p_y) <
	(TRACTDIST) * me->p_ship.s_tractrng) {
	if (me->p_flags & PFDOCK && players[me->p_docked].p_speed > 4) {
	    warning("It's unsafe to pressor while docked and base is moving more then warp 4.");
	    return;
	}
	me->p_tractor = target;	/* set the target */
	me->p_flags |= (PFTRACT | PFPRESS);	/* set the tract and press */
    }
    /* flags */
    else
	warning("Weapon's Officer:  Vessel is out of range of our pressor beam.");
}


/*-----------------------------------SENDWARN------------------------------*/
/*  This function sends the warning message to the other players when a
player switches his war status with that team.  */

/* args:
    char   *string;		 the name of the team changing status
				   towards 
    int     atwar;		 1 if declarig war 0 if decalring peace 
    int     team;		 name of team we are switching in regards
				   to */
static void
sendwarn(char *string, int atwar, int team)
{
    char    buf[BUFSIZ];	/* to hold sprintfed message */
    char    addrbuf[10];	/* to hold ship number and team letter */

    if (atwar) {		/* if decalring war */
	(void) sprintf(buf, "%s (%s) declaring war on the %s",
		       me->p_name, twoletters(me), string);
    }
    else {			/* else decalring peace */
	(void) sprintf(buf, "%s (%s) declaring peace with the %s",
		       me->p_name, twoletters(me), string);
    }
    (void) sprintf(addrbuf, " %s->%-3s", twoletters(me), teams[team].shortname);
    pmessage2(buf, team, MTEAM, addrbuf, me->p_no);
}



/*-------------------------------DECLARE_WAR-----------------------------*/
/*  This function changes the war mask of a player.  */

void 
declare_war(int mask)		/* who are we declaring war against */
{
    int     changes;		/* to hold changes in war mask */
    int     i;			/* looping var */

    mask &= ALLTEAM;		/* mask off extraneous bits */
    mask &= ~me->p_team;	/* mask out our team bit */
    mask |= me->p_swar;
    changes = mask ^ me->p_hostile;	/* determine changes */
    if (changes == 0)		/* if no changes then we are done */
	return;
    if (changes & FED)		/* if Fed status changed then warn them */
	sendwarn("Federation", mask & FED, FED);
    if (changes & ROM)		/* if Rom status changed then warn them */
	sendwarn("Romulans", mask & ROM, ROM);
    if (changes & KLI)		/* if Kli status changed then warn them */
	sendwarn("Klingons", mask & KLI, KLI);
    if (changes & ORI)		/* if Orion status changed then warn them */
	sendwarn("Orions", mask & ORI, ORI);
    if (me->p_flags & PFDOCK) {	/* if docked and now at war with */
	if (players[me->p_docked].p_team & mask) {	/* dockee then undock */
	    undock_player(me);
	}
    }
    else if (allows_docking(me->p_ship)) {	/* if ship is dockable then */
	if (me->p_docked > 0) {	/* go through ports and */
	    for (i = 0; i < me->p_ship.s_numports; i++) {	/* kick off warring
								   ships */
		if (me->p_port[i] >= 0 &&
		    (mask & players[me->p_port[i]].p_team)) {
		    base_undock(me, i);
		}
	    }
	}
    }
    if (mask & ~me->p_hostile) {/* no sudden changing of */
	me->p_flags |= PFWAR;	/* war status */
	delay = me->p_updates + 100;
	warning("Pausing ten seconds to re-program battle computers.");
    }
    me->p_hostile = mask;	/* new hostile mask */
}


/* switches the special weapon of the player */

void 
switch_special_weapon(void)
{
    int     mask = 0;
    int     currflag;
    int     i, j;

    mask = me->p_ship.s_nflags & (SFNHASMISSILE | SFNPLASMAARMED);

    if (!mask) {
	warning("This ship is not armed with any special weapons");
	me->p_specweap = 0;
	return;
    }
    for (i = 0; i < sizeof(int) * 8; i++) {
	currflag = 1 << i;
	if (!mask & currflag)
	    continue;
	if (me->p_specweap & currflag) {
	    i++;
	    break;
	}
    }

    for (j = 0; j < sizeof(int) * 8; i++, j++) {
	if (i > sizeof(int) * 8)
	    i = 0;
	currflag = 1 << i;
	if (mask & currflag) {
	    me->p_specweap = currflag;
	    break;
	}
    }

    if (me->p_specweap & SFNHASMISSILE) {
	if (me->p_ship.s_nflags & SFNHASFIGHTERS)
	    warning("Fighters ready");
	else
	    warning("Missiles armed");
    }
    else if (me->p_specweap & SFNPLASMAARMED) {
	warning("Plasmas armed");
    }
}

/*-------------------------------DO_REFIT---------------------------------*/
/*  This function will attempt to refit a player to another ship.  There
are all sorts of nasty reasons why it might fail. */

void 
do_refit(int type)		/* type of ship to refit to */
{
    int     i;			/* looping var */

    if (type < 0 || type >= NUM_TYPES)	/* ship type select out of range */
	return;
    if (me->p_flags & PFORBIT) {/* if orbiting must be a shipyard */
	if (!(planets[me->p_planet].pl_flags & PLSHIPYARD)) {
	    warning("You can change ships at a planet with a shipyard on it.");
	    return;
	}
    }
    else if (me->p_flags & PFDOCK) {	/* if docked then */
	if (allows_docking(shipvals[type])) {	/* cannot refit to a starbase */
	    warning("Can only refit to a base in a shipyard");
	    return;
	}
	if (players[me->p_docked].p_team != me->p_team) {
	    warning("You must dock with YOUR base to apply for command reassignment!");	/* no refitting on enemy
											   SB's */
	    return;
	}
	if (!(players[me->p_docked].p_ship.s_nflags & SFNCANREFIT)) {
	    warning("This base can not provide you with a new ship.");
	    return;
	}
    }
    else {			/* planet does not have shipyard */
	warning("Must orbit a shipyard or dock with your base to apply for command reassignment!");
	return;
    }
    if ((me->p_damage > ((float) me->p_ship.s_maxdamage) * .75) ||
	(me->p_shield < ((float) me->p_ship.s_maxshield) * .75) ||
	(me->p_fuel < ((float) me->p_ship.s_maxfuel) * .75)) {
	warning("Central Command refuses to accept a ship in this condition!");
	return;			/* ship must be in good condition */
    }
    if ((me->p_armies > 0)) {	/* no refitting with armies on board */
	warning("You must beam your armies down before moving to your new ship");
	return;
    }

    {
	int     oldt, oldd, oldp, allowed;
	oldt = me->p_ship.s_type;
	oldd = me->p_ship.s_numdefn;
	oldp = me->p_ship.s_numplan;
	me->p_ship.s_type = -1;
	me->p_ship.s_numdefn = 0;
	me->p_ship.s_numplan = 0;
	allowed = allowed_ship(me->p_team, me->p_stats.st_rank,
			       me->p_stats.st_royal, type);
	me->p_ship.s_type = oldt;
	me->p_ship.s_numdefn = oldd;
	me->p_ship.s_numplan = oldp;
	if (!allowed)
	    return;
    }

    if (me->p_ship.s_type == STARBASE ||
	me->p_ship.s_type == WARBASE ||
	me->p_ship.s_type == CARRIER)
	me->p_kills = 0;	/* reset kills to zero if coming */
    /* from a base */

    if (allows_docking(me->p_ship)) {	/* if ship dockable then */
	for (i = 0; i < me->p_ship.s_numports; i++) {	/* dump all ship out of
							   dock */
	    base_undock(me, i);
	}
    }
    get_ship_for_player(me, type);	/* place ship in player strct */

    switch_special_weapon();

    if (me->p_flags & PFORBIT) {
	char    buf[120];
	me->p_lastrefit = me->p_planet;
	sprintf(buf, "%s is now your home port", planets[me->p_lastrefit].pl_name);
	warning(buf);
    }

    me->p_flags &= ~(PFREFIT);
    me->p_flags |= PFREFITTING;
    rdelay = me->p_updates + 50;
    warning("You are being transported to your new vessel .... ");
}


/* Is a person with this rank on this team able
   to requisition a ship of this type? */
int 
allowed_ship(int team, int rank, int royal, int type)
{
    struct ship *tempship;
    int     shipcount;
    int     used_teamstr;	/* people required to support flying bases */
    int     used_planstr;	/* planets required to support flying bases */
    int     i, maxrank = 0;

    if (!shipsallowed[type]) {
	warning("That ship hasn't been designed yet.");
	return 0;
    }
    tempship = &shipvals[type];

    if (status2->league == 1)
	return 1;

    if (!status2->league) {
	if(configvals->baserankstyle) {
	    for(i = 0; i < MAXPLAYER; i++)
		if ((players[i].p_status == PALIVE ||
		     players[i].p_status == POUTFIT)
		      && players[i].p_team == team)
		    maxrank = MAX(rank, players[i].p_stats.st_rank);
	} else
	    maxrank = NUMRANKS;

        if((rank < MIN(tempship->s_rank, maxrank) && !(royal > GODLIKE)) &&
	    !(shipvals[tmpPick].s_nflags & SFNMASSPRODUCED)) {
	    char    buf[100];
	    sprintf(buf, "Sorry %s, a rank of %s is required to command that ship.",
		    ranks[rank].name, ranks[tempship->s_rank].name);
	    warning(buf);
	    return 0;
	}
    }
    used_teamstr = used_planstr = 0;
    shipcount = 0;		/* How many ships of that type */
    /* are on our team already? */
    for (i = 0; i < MAXPLAYER; i++) {
	if (players[i].p_status == PALIVE && players[i].p_team == team) {
	    if (players[i].p_ship.s_type == type)
		shipcount++;
	    used_teamstr += players[i].p_ship.s_numdefn;
	    used_planstr += players[i].p_ship.s_numplan;
	}
    }
    /*
       if you are in a base, and near the limits, you will sometimes have to
       switch into a normal ship before changing into another base
    */

    if ((shipcount >= tempship->s_maxnum) && !(shipvals[tmpPick].s_nflags & SFNMASSPRODUCED)) {
	char    buf[100];
	sprintf(buf, "We already have %d %ss.", shipcount, tempship->s_name);
	warning(buf);
	return 0;
    }
    if ((teams[team].s_turns[type] >
	 (tempship->s_maxnum - shipcount - 1) * tempship->s_timer) &&
	!(shipvals[tmpPick].s_nflags & SFNMASSPRODUCED)) {
	char    buf[100];
	int     turnsleft = teams[team].s_turns[type] -
	(tempship->s_maxnum - shipcount - 1) * tempship->s_timer;
	if (teams[team].s_turns[type] > 1) {
	    sprintf(buf, "We are still building that ship (%d minutes until finished.)", turnsleft);
	    warning(buf);
	}
	else
	    warning("We are still building that ship (1 minute until finished.)");
	return 0;
    }
    if ((tempship->s_numdefn &&
	 numShips(team) < tempship->s_numdefn + used_teamstr) &&
	!(shipvals[tmpPick].s_nflags & SFNMASSPRODUCED)) {
	char    buf[100];
	sprintf(buf, "Your team is not strong enough to defend a %s (%d+%d players needed).",
		tempship->s_name, tempship->s_numdefn, used_teamstr);
	warning(buf);
	return 0;
    }
    if ((tempship->s_numplan &&
	 numPlanets(team) < tempship->s_numplan + used_planstr) &&
	!(shipvals[tmpPick].s_nflags & SFNMASSPRODUCED)) {
	char    buf[100];
	sprintf(buf, "Your team's struggling economy cannot support a %s (%d+%d planets needed).", tempship->s_name, tempship->s_numplan, used_planstr);
	warning(buf);
	return 0;		/* have enough planets? */
    }
    return 1;
}

/*-------END OF LINE----*/
