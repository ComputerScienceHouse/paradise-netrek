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
#include "weapons.h"

/*------------------------------VISIBLE FUNCTIONS-------------------------*/

/*---------------------------------HOSTILE_TO-----------------------------*/
/*  This returns whether an object is hostile to another object.  It returns
1 if we are hostile or at war with the object  */


static int 
hostile_to(int warmask, int team, struct player *pl)
{
    return (warmask & pl->p_team) || (team & (pl->p_swar | pl->p_hostile));
}




/*----------------------------------EXPLODE-------------------------------*/
/*  This function decides which players to inflict damage on when a torp
explodes.  */

static void 
explode_damage(struct basetorp *torp, int radius, int why)
{
    register int i;		/* looping var */
    int     dx, dy, dist;	/* to calc distance from torp to players */
    int     damage;		/* to hold damage inflicted */
    register struct player *j;	/* to point to players */

    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if (j->p_status != PALIVE)	/* no need to check players not alive */
	    continue;

	if (j->p_no ==  torp->bt_owner) /* General Vote, September, 1995. */
	   continue;                    /* Torps no longer damage owner. */

	if ((torp->bt_status == TDET)	/* if torp was detted then */
	    &&((j->p_no == torp->bt_owner) ||	/* cannot damage firing
						   player */
	       ((j->p_no != torp->bt_whodet)	/* cannot damage players on
						   same team */
		&&(j->p_team == players[torp->bt_whodet].p_team)))
	    )			/* except the detter */
	    continue;

	dx = torp->bt_x - j->p_x;	/* calc delta x and y */
	dy = torp->bt_y - j->p_y;
	if (ABS(dx) > radius || ABS(dy) > radius)
	    continue;		/* continue if obviously too far */

	dist = dx * dx + dy * dy;	/* calc distnace squared */
	if (dist > radius * radius)	/* if not within damage distance */
	    continue;		/* then continue */
	if (dist > EXPDIST * EXPDIST)	/* if not direct hit */
	    damage = torp->bt_damage * (radius - sqrt((double)dist)) /
		(radius - EXPDIST);
	else			/* else if direct hit */
	    damage = torp->bt_damage;	/* do torp damage */

	if (damage > 0) {	/* if damage was done then */
	    if (players[torp->bt_owner].p_hostile & j->p_team)	/* start war if */
		players[torp->bt_owner].p_swar |= j->p_team;	/* necessary */
	    inflict_damage(&players[torp->bt_owner], &players[torp->bt_whodet],
			   j, damage, why);
	}
    }
}

static void
explode(struct basetorp *torp)	/* pointer to exploding torp's struct */
{
    explode_damage(torp, DAMDIST, KTORP);
    torp->bt_status = TEXPLODE;	/* set the torp to explode */
    torp->bt_fuse = 10 / TORPFUSE;
}




/*---------------------------------PEXPLODE-------------------------------*/
/*  This function does the explosion of a plasma torp.  It goes through all
players and damages them if they are close enough to get damaged*/

static void
pexplode(struct plasmatorp *plasmatorp)	/* ptr to plasma to explode */
{
    explode_damage(&plasmatorp->pt_base, PLASDAMDIST, KPLASMA);
    plasmatorp->pt_status = PTEXPLODE;	/* set the plasma to explode */
    plasmatorp->pt_fuse = 10 / PLASMAFUSE;
}




/*-------------------------------UDPHASER----------------------------------*/
/*  This function goes through all players and calcs the damage from a
phaser hit if the phaser hit a target.  */

void
udphaser(void)
{
    register int i;		/* looping var */
    register struct phaser *j;	/* to point to phaser being fired */
    register struct player *victim;	/* to point to the poor victim */

    for (i = 0, j = &phasers[i]; i < MAXPLAYER; i++, j++) {	/* all players */
	switch (j->ph_status) {	/* check player's phaser status */
	case PHFREE:		/* if not beging fired */
	    continue;		/* then continue */
	case PHMISS:		/* if it missed */
	case PHHIT2:		/* or ????? */
	    if (j->ph_fuse-- == 1)	/* dec count of phaser */
		j->ph_status = PHFREE;	/* free it up if count done */
	    break;
	case PHHIT:		/* if it hit someone then */
	    if (j->ph_fuse-- == players[i].p_ship.s_phaser.fuse) {
		victim = &players[j->ph_target];	/* get the victim */
		if (players[i].p_hostile & victim->p_team)	/* start war if */
		    players[i].p_swar |= victim->p_team;	/* necessary */
		if (victim->p_status == PALIVE)	/* only damage if alive */
		    inflict_damage(&players[i], 0, victim, j->ph_damage, KPHASER);
	    }			/* end of if phaser hit someone */
	    if (j->ph_fuse == 0)
		j->ph_status = PHFREE;
	    break;
	}			/* end of switch */
    }				/* end of for loop through players */
}



/*----------------------------WEAP_NEAR_OBJECT-----------------------------*/
/*   This function checks for objects within *dist* of a weapon.           */
/* this function will use the space grid */

static int
weap_near_object(struct basetorp *torp, int type, int dist)
{
    register struct planet *pl;	/* to point to the planets */
    int     dx, dy, i;

    for (i = 0, pl = &planets[i]; i < NUMPLANETS; i++, pl = &planets[i]) {
	if (PL_TYPE(*pl) != type)
	    /* if not of the type specified, don't bother */
	    continue;
	dx = torp->bt_x - pl->pl_x;
	dy = torp->bt_y - pl->pl_y;
	if (ABS(dx) > dist || ABS(dy) > dist)
	    continue;		/* the obvious thing */
	if (dx * dx + dy * dy < dist * dist)
	    return 1;		/* yep, it hit.  return a 1. */
    }
    return 0;			/* return that it should continue */
}

static int
near_player(struct basetorp *torp, int dist)
{
    register int i;		/* looping var */
    int     dx, dy;		/* to calc torp-player distance */
    register struct player *j;	/* to point to players */

    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if (j->p_status != PALIVE)
	    continue;		/* don't check players not alive */
	if (j->p_no == torp->bt_owner)
	    continue;		/* no exploding on self */
	if (!hostile_to(torp->bt_war, torp->bt_team, j))
	    continue;		/* disregard if both teams not at war */
	dx = torp->bt_x - j->p_x;	/* calc delta coords */
	dy = torp->bt_y - j->p_y;
	if (ABS(dx) > dist || ABS(dy) > dist)
	    continue;		/* disregard if obviously too far */
	if (dx * dx + dy * dy < dist * dist) {
	    torp->bt_whodet = i;/* this guy is the detonator */
	    return 1;		/* if close enough to explode then return 1 */
	}
    }
    return 0;			/* return that torp should continue */
}

/* the fighter landing function */
static int
f_land(struct missile *mis)	/* the fighter to check for */
{
    int     dx, dy;		/* to calc fighter-player distance */
    register struct player *j;	/* to point to players */

    j = &players[mis->ms_owner];

    if (!(j->p_armies < j->p_ship.s_maxarmies))
	return 0;		/* no room! */
    dx = mis->ms_x - j->p_x;	/* calc delta coords */
    dy = mis->ms_y - j->p_y;
    if (ABS(dx) > EXPDIST || ABS(dy) > EXPDIST)
	return 0;		/* obviously too far */
    if (dx * dx + dy * dy < EXPDIST * EXPDIST)
	return 1;		/* if close enough to land then return 1 */

    return 0;			/* return that fighter should continue */
}


/*----------------------------------PNEAR----------------------------------*/
/*  This function goes through the players and sees if there is a player
close enough for a plasma torp to explode on.  This function returns a 1
if the plasma should explode and a 0 if it should continue to travel through
spac.  */

static int
pnear(struct plasmatorp *plasmatorp)	/* the plasma torp to check for */
{
    return near_player(&plasmatorp->pt_base, EXPDIST);
}

/*--------------------------------GET_BEARING------------------------------*/
/*  This function takes two set of coordinates and a direction.  One set
of coordinates is the current coords of a trop.  The other set is some other
coords you want to get a change in direction for.  The direction is the
current direction of the torp.  The function returns the angle that the dir
need to be changed by to travel to the new points.  */

/* args:
    int     dx, dy;		delta x, y coords
    int     dir;		current direction travelling */
unsigned char 
get_bearing(int dx, int dy, int dir)
{
    int     phi;		/* to hold angle */

    phi = (int) (atan2((double) dx, (double) -dy) / 3.14159 * 128.0);
    if (phi < 0)		/* make phi positive */
	phi = 256 + phi;
    if (phi >= dir)
	return ((unsigned char) (phi - dir));
    else
	return ((unsigned char) (256 + phi - dir));
}

/* args:
    int	w;		 speed of torp
    int	dx,dy;		 distance to target
    int	s, dir;		 speed of target */
static int
anticipate_impact(int w, int dx, int dy, int s, int dir)
{
    float	sdx, sdy;
    float	a, b, c, d;
    float	t;
    float	tdx, tdy;
    float	theta;

				/* mathematically, these affect t, but
				   not the return value */
    s *= WARP1 * TICKSPERSEC;
    w *= WARP1 * TICKSPERSEC;

    sdx = s*Cos[dir];
    sdy = s*Sin[dir];

    a = s*(float)s - w*(float)w;
    b = 2*(sdx*(float)dx + sdy*(float)dy);
    c = dx*(float)dx + dy*(float)dy;

    if (a==0) {
	t = -c/b;
    } else {

	d = b*b - 4*a*c;
	if (d<0)
	    return -1;
	d = sqrt(d);

	if (a<0) {
	    a = -a;
	    b = -b;
	}

	t = (-b - d)/(2*a);

	if (t<0)
	    t = (-b + d)/(2*a);
    }

    if (t<0)
      return -1;

    if (t>10)
      return -1;

    tdx = dx/t + sdx;
    tdy = dy/t + sdy;

    theta = atan2(tdx, -tdy);
    return (unsigned char) (int) (theta * 128 / 3.14159);
}


/*----------------------------TORP_TRACK_OPPORTUNITY----------------------*/
/*  This function finds the closest ship to a torp and returns -1 if the
ship is to the left of the torp on its current heading and a 1 if the ship
is to the right.  If no target is found then a 0 is return indicating the
torp should go straight.  */

/* args:
    struct basetorp *torp;	the torp to check for
    int     turnspeed;
    int	    smart;		which tracking algorithm? */
static int 
torp_track_opportunity(struct basetorp *torp, int turnspeed, int smart)
{
    int     i;			/* looping var */
    int     closest;		/* to hold closest player */
    int	    clbearing=0;	/* bearing to closest player */
    int     min_distsq;		/* to hold closest distance */
    int     war_mask;		/* who torp own is at war with */
    int     x, y;		/* to hold torps x, y coords */
    int     bearing;		/* to get bearing to hit player */
    int     dir;		/* to hold torps direction */
    int     range;

    closest = -1;		/* initialize closest player--no plyr */
    x = torp->bt_x;		/* get the coords of torp */
    y = torp->bt_y;
    dir = torp->bt_dir;		/* get torp's directions */
    war_mask = torp->bt_war;	/* and who he as war with */

    range = torp->bt_fuse * torp->bt_speed * WARP1;

    min_distsq = range*range * 4; /* intialize closest player distance */

    for (i = 0; i < MAXPLAYER; i++) {	/* check all other players */
	int     dx, dy;
	if (!(isAlive(&players[i]) &&
	      hostile_to(war_mask, torp->bt_team, &players[i])))
	    continue;		/* only do if player alive and at war */

	dx = players[i].p_x - x;
	dy = players[i].p_y - y;

	if (ABS(dx) > range || ABS(dy) > range)
	    continue;		/* clearly out of range */

	if (smart) {
	    bearing = anticipate_impact(torp->bt_speed, dx, dy,
				    players[i].p_speed, players[i].p_dir);
	    if (bearing<0)
	      bearing = get_bearing(dx, dy, dir);
	    else 
	      bearing = (unsigned char)(bearing-dir);
	} else {
	    bearing = get_bearing(dx, dy, dir);
	}
	/* torps will only track to targets they have a reasonable chance */
	/* of hitting */
	if ((turnspeed * torp->bt_fuse > 127) ||
	    (bearing < ((unsigned char) turnspeed * torp->bt_fuse)) ||
	  (bearing > ((unsigned char) (256 - turnspeed * torp->bt_fuse)))) {
	    int	distsq;
	    distsq = dx*dx + dy*dy;
	    if (distsq < min_distsq) { /* record it if it is */
		min_distsq = distsq; /* less than current closest */
		closest = i;	/* player */
		clbearing = bearing;
	    }
	}
    }
    if (closest >= 0) {		/* if a target found then */
	if (clbearing > 128)
	    return (-1);	/* Target is on the left */
	else
	    return (1);		/* Target is on the right */
    }
    return (0);			/* No target ... go straight. */
}


/*--------------------------------UDTORPS----------------------------------*/
/*  This function updates the torps.  It goes through all torps and checks
to see if they need to be updated.  It they are mving they track toward the
nearest target.  this function also handles the explosion and makes sure
that torps do not go off the edge of the galaxy.  */

void
udtorps(void)
{
    register int i;		/* looping var--to loop through torps */
    int     turn;		/* to get whether to go right or left */
    int     heading;		/* to hold torps heading */
    register struct torp *j;	/* to point to torps */

    for (i = 0, j = &torps[i]; i < MAXPLAYER * MAXTORP; i++, j++) {
	switch (j->t_status) {	/* check status of torp */
	case TFREE:		/* if torp not active then */
	    continue;		/* go on to next torp */
	case TMOVE:
	case TSTRAIGHT:	/* if torp moving then */
	    if (j->t_turns > 0) {	/* if torp can turn then */
		turn = torp_track_opportunity
		    (&j->t_base, j->t_turns,
		     configvals->improved_tracking[SS_PHOTON]);
		/* should we go right or left */
		if (turn < 0) {	/* we will go left */
		    heading = ((int) j->t_dir) - j->t_turns;	/* turn left */
		    if( heading < 0 )
			j->t_dir = heading + 256;
		    /* j->t_dir = ((heading < 0) ? ((unsigned char) (256 + heading)) :
				((unsigned char) heading));	* no underflow */
		}
		else if (turn > 0) {	/* we will go right */
		    heading = ((int) j->t_dir) + j->t_turns;	/* turn right */
		    if( heading > 255 )
			j->t_dir = heading - 256;
		    /* j->t_dir = ((heading > 255) ? ((unsigned char) (heading - 256)) :
				((unsigned char) heading));	* no overflow */
		}
	    }
	    j->t_x += (double) j->t_speed * Cos[j->t_dir] * WARP1;
	    j->t_y += (double) j->t_speed * Sin[j->t_dir] * WARP1;

	    move_torp(i, j->t_x, j->t_y, 1);

	    if (j->t_status == TMOVE)	/* if a TMOVE torp then */
		j->t_dir += (lrand48() % 3) - 1;	/* make the torp wobble */
	    if (j->t_fuse-- <= 0) {	/* dec torp's life and see if dead */
		j->t_status = TFREE;	/* dead, free the torp */
		move_torp(i, -1, -1, 1);
		players[j->t_owner].p_ntorp--;	/* let player fire another */
		break;		/* no more torp processing */
	    }
	    if ((sun_effect[SS_PHOTON] && weap_near_object(&j->t_base, PLSTAR, ORBDIST))
		||
	       (wh_effect[SS_PHOTON] && weap_near_object(&j->t_base, PLWHOLE, ORBDIST))){
		/* did it hit a star or wormhole? */
		j->t_whodet = j->t_owner;
		explode(&j->t_base);
		break;
	    }

	    if ((terrain_grid[(int)(j->t_x)/TGRID_GRANULARITY * TGRID_SIZE +
		             (int)(j->t_y)/TGRID_GRANULARITY].types 
		 & T_ASTEROIDS) &&
		ast_effect[SS_PHOTON])
	      if (TORP_HIT_AST > (lrand48()%100)) {
		explode(&j->t_base);
		break;
	      }
	
	    if (near_player(&j->t_base, EXPDIST)) {
		/* if torp near enough to hit */
		explode(&j->t_base);	/* let torp explode on player */
	    }

	    break;
	case TDET:		/* if torp was detted */
	    explode(&j->t_base);/* make it explode */
	    break;		/* on to next torp */
	case TEXPLODE:		/* if torp exploding */
	    if (j->t_fuse-- <= 0) {	/* dec explosion timer */
		j->t_status = TFREE;	/* if torp done, free it up */
		move_torp(i, -1, -1, 1);
		players[j->t_owner].p_ntorp--;	/* let player fire another */
	    }
	    break;		/* on to next torp */
	case TOFF:
	    j->t_status = TFREE;
	    move_torp(i, -1, -1, 1);
	    players[j->t_owner].p_ntorp--;
	    break;
	default:		/* Shouldn't happen */
	    j->t_status = TFREE;
	    break;
	}			/* end of switch */
    }				/* end of for */
}

/*------------------------------------------------------------------------*/
/*--------------------------FIGHTER_TRACK_TARGET--------------------------*/
/*  This function finds the closest ship to a fighter and returns -1 if the
ship is to the left of the fighter on its current heading and a 1 if the ship
is to the right.  If no target is found then a 0 is return indicating the
fighter should go straight.  Also returns fighters to the CV.
    If the player is locked onto an enemy ship, that's the only ship that
gets checked.  */

static int
fighter_track_target(struct missile *mis, int turnspeed)
{
    int     i;			/* looping var */
    int     closest;		/* to hold closest player */
    int     min_dist;		/* to hold closest distance */
    int     dist;		/* temp var to hold distance */
    int     war_mask;		/* who fighter own is at war with */
    int     x, y;		/* to hold fighters x, y coords */
    int     owner;		/* to hold fighters owner */
    int     bearing;		/* to get bearing to hit player */
    int     dir;		/* to hold fighters direction */
    int     range;
    int     dx, dy;

    min_dist = GWIDTH * 2;	/* intialize closest player distance */
    closest = -1;		/* initialize closest player--no plyr */
    x = mis->ms_x;		/* get the coords of torp */
    y = mis->ms_y;
    dir = mis->ms_dir;		/* get fighter's directions */
    owner = mis->ms_owner;	/* get the fighter's owner */
    war_mask = mis->ms_war;	/* and who he as war with */

    range = mis->ms_fuse * mis->ms_speed * WARP1;

    for (i = 0; i < MAXPLAYER; i++) {	/* check all other players */
	if (mis->ms_status == TRETURN) {
	    if (!(isAlive(&players[i])) || (owner != i))
		continue;
	}			/* if returning, only check owning player */
	else if ((players[owner].p_flags & PFPLOCK) &&
		 (players[owner].p_playerl != i))
	    continue;		/* if player is locked onto a player, only
				   check that player */
	else if (!(isAlive(&players[i]) &&
		   hostile_to(war_mask, mis->ms_team, &players[i])))
	    continue;		/* only do if player alive and at war */

	dx = players[i].p_x - x;
	dy = players[i].p_y - y;

	if (ABS(dx) > range || ABS(dy) > range)
	    continue;		/* clearly out of range */

	bearing = get_bearing(dx, dy, dir);
	if ((turnspeed * mis->ms_fuse > 127) ||
	    (bearing < ((unsigned char) turnspeed * mis->ms_fuse)) ||
	    (bearing > ((unsigned char) (256 - turnspeed * mis->ms_fuse)))) {
	    dist = ihypot(dx, dy);
	    if (dist < min_dist) {	/* record it if it is */
		min_dist = dist;/* less than current closest */
		closest = i;	/* player */
	    }
	}
    }
    if (closest >= 0) {		/* if a target found then */
	if (get_bearing(players[closest].p_x - x,
			players[closest].p_y - y, dir) > 128) {
	    return (-1);	/* Target is on the left */
	}
	else
	    return (1);		/* Target is on the right */
    }
    return (0);			/* No target ... go straight. */
}


/*--------------------------------------------------------------------------*/
/*------------------------------------F_TORP--------------------------------*/
/*   Checks to see if a valid target is within a certain forward firing angle*/
/* then fires a torpedo at that target.  A return value of 1 indicates firing*/

static int 
f_torp(struct missile *mis)
{
    register int i;
    int     torp2fire = -1, targetdist = FSTRIKEDIST + 1, tdist, target;
    unsigned char bearing;
    register struct torp *k;
    int     dx, dy;
    register struct player *j;	/* to point to players */

    for (i = mis->ms_owner * MAXTORP, k = &torps[i];	/* Find a free torp */
	 i < mis->ms_owner * MAXTORP + MAXTORP; i++, k++)
	if (k->t_status == TFREE) {
	    torp2fire = i;
	    break;
	}
    if (torp2fire == -1)
	return 0;


    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if (j->p_status != PALIVE)
	    continue;		/* don't check players not alive */
	if (j->p_no == mis->ms_owner)
	    continue;		/* no firing on self */
	if (!hostile_to(mis->ms_war, mis->ms_team, j))
	    continue;		/* disregard if both teams not at war */
	if ((players[mis->ms_owner].p_flags & PFPLOCK) &&
	    (players[mis->ms_owner].p_playerl != i))
	    continue;		/* ignore if this isn't the target */

	dx = mis->ms_x - j->p_x;/* calc delta coords */
	dy = mis->ms_y - j->p_y;
	if (ABS(dx) > FSTRIKEDIST || ABS(dy) > FSTRIKEDIST)
	    continue;		/* disregard if obviously too far */

	tdist = ihypot(dx, dy);
	if (tdist < FSTRIKEDIST) {
	    bearing = (int) get_bearing(dx, dy, mis->ms_dir);
	    targetdist = tdist;	/* record the target ship */
	    target = i;
	}
    }

    if (targetdist < FSTRIKEDIST) {
	j = &players[mis->ms_owner];
	k = &torps[torp2fire];
	k->t_no = torp2fire;
	k->t_status = TMOVE;
	k->t_owner = mis->ms_owner;
	k->t_team = mis->ms_team;

	move_torp(torp2fire, mis->ms_x, mis->ms_y, 0);

	k->t_damage = FTORP_DAMAGE;
	k->t_speed = FTORP_SPEED;
	k->t_war = j->p_hostile |
	    j->p_swar;
	k->t_fuse = FTORP_FUSE + (lrand48() % 20);
	k->t_turns = FTORP_TRACK;

	/*
	   here's the biggie -- what angle do I fire this torp at, so I have
	   a reasonable chance of hitting?  Especially since I only get one
	   shot. But, then, I have a bunch of buddies, too...
	*/

	if ((mis->ms_no % MAXPLAYER % 3) == 0)
	    k->t_dir = mis->ms_dir;
	else if ((mis->ms_no % MAXPLAYER % 3) == 1)
	    k->t_dir = mis->ms_dir - 8;
	else if ((mis->ms_no % MAXPLAYER % 3) == 2)
	    k->t_dir = mis->ms_dir + 8;
	return 1;
    }
    return 0;
}


void 
udmissiles(void)
{
    int     i;
    int     x, y, turn;
    struct missile *mis;
    struct player *j;

    for (i = 0; i < MAXPLAYER * NPTHINGIES; i++) {
	mis = &missiles[i];
	switch (mis->ms_status) {
	case TFREE:
	    break;
	case TLAND:
	    j = &players[mis->ms_owner];
	    j->p_ship.s_missilestored++;
	    j->p_armies = (int) (j->p_ship.s_missilestored / FAE_RATE);
	    mis->ms_status = TFREE;
	    j->p_nthingys--;
	    break;
	case TRETURN:
	case TMOVE:
	case TSTRAIGHT:

	    if (mis->ms_fuse-- <= 0) {
		mis->ms_status = TFREE;
		move_missile(i, -1, -1, 1);
		break;
	    }

	    if (terrain_grid[(int)(mis->ms_x)/TGRID_GRANULARITY * TGRID_SIZE +
		             (int)(mis->ms_y)/TGRID_GRANULARITY].types 
		 & T_ASTEROIDS)
	    {
	      if ((mis->ms_type == FIGHTERTHINGY) &&
		  ast_effect[SS_FIGHTER] &&
		  (FIGHTER_HIT_AST > (lrand48()%100))) {
		mis->ms_whodet = mis->ms_owner;
		explode(&mis->ms_base);
		break;
	      } else if ((MISSILE_HIT_AST > (lrand48()%100)) &&
			 ast_effect[SS_MISSILE]) {
		mis->ms_whodet = mis->ms_owner;
		explode(&mis->ms_base);
		break;
	      }
	    }
	
	    if ((((sun_effect[SS_MISSILE] && mis->ms_type == MISSILETHINGY)
		 || (sun_effect[SS_FIGHTER] && mis->ms_type == FIGHTERTHINGY))
		&& weap_near_object(&mis->ms_base, PLSTAR, ORBDIST))
		||
		(((wh_effect[SS_MISSILE] && mis->ms_type == MISSILETHINGY)
		  || (wh_effect[SS_FIGHTER] && mis->ms_type == FIGHTERTHINGY))
		 && weap_near_object(&mis->ms_base, PLWHOLE, ORBDIST))){
		/* did it hit a star? */
		explode(&mis->ms_base);
		break;
	    }

	    j = &players[mis->ms_owner];

	    if (mis->ms_type == FIGHTERTHINGY &&
		!(j->p_ship.s_nflags & SFNHASFIGHTERS)) {
		mis->ms_type = MISSILETHINGY;	/* If the player no longer
						   has em, */
		mis->ms_status = TMOVE;	/* make his fighters kamikazes */
		mis->fi_hasfired = 0;
		break;
	    }

	    if ((mis->ms_type == FIGHTERTHINGY)
		&& ((mis->ms_fuse < .6 * j->p_ship.s_missile.fuse)
		    || (mis->fi_hasfired))
		&& mis->ms_status != TRETURN)
		mis->ms_status = TRETURN;

	    if (mis->ms_turns > 0) {
		if (mis->ms_type == FIGHTERTHINGY) {
		    turn = fighter_track_target(&mis->ms_base, mis->ms_turns);
		}
		else {
		    turn = torp_track_opportunity
			(&mis->ms_base, mis->ms_turns,
			 configvals->improved_tracking[SS_MISSILE]);
		}
		mis->ms_dir = (unsigned char) (mis->ms_dir + turn * mis->ms_turns);
	    }
	    x = mis->ms_x + mis->ms_speed * Cos[mis->ms_dir] * WARP1;
	    y = mis->ms_y + mis->ms_speed * Sin[mis->ms_dir] * WARP1;

	    move_missile(i, x, y, 1);

	    if (mis->ms_status != TSTRAIGHT)
		mis->ms_dir += (lrand48() % 3) - 1;

	    if (mis->ms_type == MISSILETHINGY
		&& near_player(&mis->ms_base, EXPDIST)) {
		explode(&mis->ms_base);
	    }
	    else if (mis->ms_type == FIGHTERTHINGY
		     && near_player(&mis->ms_base, FSTRIKEDIST)
		     && !mis->fi_hasfired) {
		if (f_torp(mis))
		    mis->fi_hasfired = 1;	/* if within strike range,
						   fire a torp */
	    }

	    if (mis->ms_status == TRETURN &&
		f_land(mis)) {
		mis->ms_status = TLAND;
		move_missile(i, -1, -1, 1);
	    }

	    break;

	case TDET:
	    explode(&mis->ms_base);
	    break;

	case TEXPLODE:
	    if (mis->ms_fuse-- <= 0) {
		mis->ms_status = TFREE;
		players[mis->ms_owner].p_nthingys--;
		move_missile(i, -1, -1, 1);
	    }
	    break;
	default:
	    mis->ms_status = TFREE;
	    break;
	}
    }
}


/*------------------------------UDPLASMATORPS-----------------------------*/
/*  This function updates the plasma torps.  It goes through all the plasma
torps and if they are alive it adjusts their heading to track players.  It
then moves the plasma torp and checks to see if the plasma should explode.
It will ensure that a plasma torp explodes when it hits the edge of the
galaxy.  */

void
udplasmatorps(void)
{
    register int i;		/* looping var--loop through plasmas */
    int     turn;		/* to get whether to go left or right */
    int     heading;		/* to hold plasma heading */
    struct plasmatorp *j;	/* to point to a plasma */

    for (i = 0, j = &plasmatorps[i]; i < MAXPLAYER * MAXPLASMA; i++, j++) {
	switch (j->pt_status) {	/* check torp's status */
	case PTFREE:		/* if plasma not being fired */
	    continue;		/* go to next plasma */
	case PTMOVE:		/* if plasma moving */
	    turn = torp_track_opportunity
		(&j->pt_base, j->pt_turns,
		 configvals->improved_tracking[SS_PLASMA]);
	    /* should we go right or left */
	    if (turn < 0) {	/* if left then */
		heading = ((int) j->pt_dir) - j->pt_turns;
		j->pt_dir = ((heading < 0) ? ((unsigned char) (256 + heading)) :
			     ((unsigned char) heading));	/* no rollunder */
	    }
	    else if (turn > 0) {/* else if right */
		heading = ((int) j->pt_dir) + j->pt_turns;
		j->pt_dir = ((heading > 255) ? ((unsigned char) (heading - 256)) :
			     ((unsigned char) heading));	/* no rollover */
	    }
	    j->pt_x += (double) j->pt_speed * Cos[j->pt_dir] * WARP1;

	    j->pt_y += (double) j->pt_speed * Sin[j->pt_dir] * WARP1;

	    if (j->pt_fuse-- <= 0) {	/* dec the torp fuse. if torp done */
		j->pt_status = PTFREE;	/* free it up */
		players[j->pt_owner].p_nplasmatorp--;	/* dec p-torps fired */
		break;
	    }

	    if ((terrain_grid[(int)(j->pt_x)/TGRID_GRANULARITY * TGRID_SIZE +
		             (int)(j->pt_y)/TGRID_GRANULARITY].types 
		 & T_ASTEROIDS) &&
		ast_effect[SS_PLASMA])
	      if (PLASMA_HIT_AST > (lrand48()%100)) {
		pexplode(j);
		break;
	      }
	    
	    if ((sun_effect[SS_PLASMA] && weap_near_object(&j->pt_base, PLSTAR, ORBDIST))
		 ||
	       (sun_effect[SS_PLASMA] && weap_near_object(&j->pt_base, PLWHOLE, ORBDIST))
	    /* did it hit a star? */
		|| pnear(j) /* or a player */ ) {
		pexplode(j);
	    }
	    break;		/* on to next torp */
	case PTDET:		/* if torp was detted */
	    pexplode(j);	/* make it explode */
	    break;		/* on to next torp */
	case PTEXPLODE:	/* if torp is exploding */
	    if (j->pt_fuse-- <= 0) {	/* dec the timer until torp dead */
		j->pt_status = PTFREE;	/* set the torp free is timer zero */
		players[j->pt_owner].p_nplasmatorp--;	/* dec ptorps fired by
							   player */
	    }
	    break;
	default:		/* Shouldn't happen */
	    j->pt_status = PTFREE;	/* free torp if it got screwed */
	    break;		/* on to next torp */
	}
    }
}

/*----------END OF FILE--------*/
