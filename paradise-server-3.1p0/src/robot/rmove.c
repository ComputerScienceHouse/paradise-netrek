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
#include <signal.h>
#include "config.h"
#include "proto.h"
#include "data.h"
#include "shmem.h"
#include "weapons.h"

/* from robot/robotII.c */
void save_robot P((void));
void config P((void));

#define SIZEOF(s)		(sizeof (s) / sizeof (*(s)))
#define AVOID_TIME		4
#define AVOID_CLICKS		200

/*
   STARTDELTA was moved from rmove() - no idea why it was there
   In fact, I have no idea where this #define is used - 04/11/00 JWW
*/
#define STARTDELTA 5000		/* ships appear +/- delta of home planet */

#define BOREDOM_TIME		1200	/* 10 minutes since last torp fired
					   => become hostile to all 4/13/92
					   TC */

#define AVOID_STAR 8000		/* Distance from star within which collision
				   avoidance is invoked. - MAK, 16-Jun-93 */
#define AVOID_MELT 2000		/* Distance from star within which collision
				   prevention is invoked. - MAK, 16-Jun-93 */

#define NORMALIZE(d) 		(((d) + 256) % 256)

#define E_INTRUDER	0x01
#define E_TSHOT		0x02
#define E_PSHOT 	0x04
#define E_TRACT 	0x08
#define NOENEMY (struct Enemy *) -1

struct Enemy {
    int     e_info;
    int     e_dist;
    unsigned char e_course;	/* course to enemy */
    unsigned char e_edir;	/* enemy's current heading */
    unsigned char e_tcourse;	/* torpedo intercept course to enemy */
    unsigned int e_flags;
    int     e_tractor;		/* who he's tractoring/pressoring */
    int     e_phrange;		/* his phaser range */
    unsigned int e_hisflags;	/* his pflags. bug fix: 6/24/92 TC */
};

static int avoidTime;

#define MESSFUSEVAL 200		/* maint: removed trailing ';' 6/22/92 TC */
static int messfuse = MESSFUSEVAL;
static char rmessage[110];
static char tmessage[110];
static char *rmessages[] = {
    /* I got bored and actually made it say something: 'DIE SCUM' */
    "1000100 1001001 1000101 100000 1010011 1000011 1010101 1001101 !"
    "Crush, Kill, DESTROY!!",
    "Run coward!",
    "Hey!  Come on you Hoser!  I dare you!",
    "Resistance is useless!",
    "Dry up and blow away you weenie!",
    "Go ahead, MAKE MY DAY!",
    "Just hit '<shift> Q' ... and spare both of us the trouble",
    "Is that REALLY you Kurt?",
    "I have you now!",
    "By the way, any last requests?",
    "Dropping your shields is considered a peaceful act.",
    "Drop your shields, and I MAY let you live ...",
    "Don't you have midterms coming up or something?",
    "Ya wanna Tango baby?",
    "Hey! Outta my turf you filthy $t!",
    "I seeeee you...",
    "I claim this space for the $f.",
    "Give up fool ... you're meat.",
    "And another one bites the dust ...",
    "Surrender ... NOW!",
    "Hey!  Have you heard about the clever $t?  No?.. Me neither.",
    "You have been registered for termination.",
    "This'll teach you to mind your own business!",
    "Man, you stupid $ts never learn!",
    "ALL RIGHT, enough goofing off ... you're toast buster!",
    "You pesky humans just NEVER give up, do you?",
    "You're actually paying money so you can play this game?",
    "My job is to put over-zealous newbies like you out of your misery.",
    "How can you stand to be $T?!?  What a hideous life!",
    "All $ts are losers!",
    "$ts all suck.  Come join the $f!",
    "The $f shall crush all filthy $ts like you!",
    "TWINK",
    "How can your friends stand to let you live?",
    "Happy, Happy, Joy, Joy."
};

static char   *termie_messages[] = {
    "$n:  Hasta la vista, Baby.",
    "Come quietly, or there will be trouble. [thump] [thump]",
    "Make your peace with GOD.",
    "$n:  Say your prayers",
    "This galaxy isn't big enough for the two of us, $n.",
    "$n, I have a warrant for your arrest.",
    "$n, self-destruct now.",
    "Coming through.  Out of my way.",
    "You feel lucky, punk?",
    "Killing is our business.  Business is good.",
    "$n, Die.  You have 10 seconds to comply.",
    "You can run $n, but you can't hide!",
    "Sorry, I am out of cool quotes.  Just die.",
    "$n: duck."
};

extern int debug;
extern int hostile;
extern int sticky;
extern int practice;
extern int polymorphic;

extern int startplanet;		/* CRD feature - MAK,  2-Jun-93 */

extern int target;		/* robotII.c 7/27/91 TC */
extern int phrange;		/* robotII.c 7/31/91 TC */
extern int trrange;		/* robotII.c 8/2/91 TC */

extern int dogslow;		/* robotII.c 8/9/91 TC */
extern int dogfast;
extern int runslow;
extern int runfast;
extern int closeslow;
extern int closefast;

/* This function means that the robot has nothing better to do.
   If there are hostile players in the game, it will try to get
   as close to them as it can, while staying in its own space.
   Otherwise, it will head to the center of its own space.
*/
/* CRD feature: robots now hover near their start planet - MAK,  2-Jun-93 */

#define GUARDDIST 8000

static void
go_home(struct Enemy *ebuf)
{
    int     x, y;
    double  dx, dy;
    struct player *j;

    if (ebuf == 0) {		/* No enemies */
/*	if (debug)
	    fprintf(stderr, "%d) No enemies\n", me->p_no);*/
	if (target >= 0) {
	    /* First priority, current target (if any) */
	    j = &players[target];
	    x = j->p_x;
	    y = j->p_y;
	}
	else if (startplanet == -1) {
	    /* No start planet, so go to center of galaxy */
	    x = (GWIDTH / 2);
	    y = (GWIDTH / 2);
	}
	else {
	    /* Return to start planet */
	    x = planets[startplanet].pl_x + (lrand48() % 2000) - 1000;
	    y = planets[startplanet].pl_y + (lrand48() % 2000) - 1000;
	}
    }
    else {			/* Let's get near him */
	j = &players[ebuf->e_info];
	x = j->p_x;
	y = j->p_y;

	if (startplanet != -1) {
	    /* Get between enemy and planet */
	    int     px, py;
	    double  theta;

	    px = planets[startplanet].pl_x;
	    py = planets[startplanet].pl_y;
	    theta = atan2((double) (y - py), (double) (x - px));
	    x = px + GUARDDIST * cos(theta);
	    y = py + GUARDDIST * sin(theta);
	}
    }
/*    if (debug)
	fprintf(stderr, "%d) moving towards (%d/%d)\n",
	    me->p_no, x, y);*/

    /*
       Note that I've decided that robots should never stop moving. * It
       makes them too easy to kill
    */

    me->p_desdir = getcourse(x, y, me->p_x, me->p_y);
    if (angdist(me->p_desdir, me->p_dir) > 64)
	set_speed(dogslow, 1);
    else if (me->p_etemp > 900)	/* 90% of 1000 */
	set_speed(runslow, 1);
    else {
	dx = x - me->p_x;
	dy = y - me->p_y;
	set_speed((ihypot((int)dx, (int)dy) / 5000) + 3, 1);
    }
    cloak_off();
}

static int
phaser_plasmas(void)
{
    register struct plasmatorp *pt;
    register int i;
    int     myphrange;

    myphrange = phrange;	/* PHASEDIST * me->p_ship.s_phaserdamage /
				   100; */
    for (i = 0, pt = &plasmatorps[0]; i < MAXPLASMA * MAXPLAYER; i++, pt++) {
	if (pt->pt_status != PTMOVE)
	    continue;
	if (i == me->p_no)
	    continue;
	if (!(pt->pt_war & me->p_team) && !(me->p_hostile & pt->pt_team))
	    continue;
	if (abs(pt->pt_x - me->p_x) > myphrange)
	    continue;
	if (abs(pt->pt_y - me->p_y) > myphrange)
	    continue;
	if (ihypot(pt->pt_x - me->p_x, pt->pt_y - me->p_y) > myphrange)
	    continue;
	repair_off();
	cloak_off();
	phaser(getcourse(pt->pt_x, pt->pt_y, me->p_x, me->p_y));
	return 1;		/* was this missing? 3/25/92 TC */
	break;
    }
    return 0;			/* was this missing? 3/25/92 TC */
}

static int
projectDamage(int eNum, int *dirP)
{
    register int i, j, numHits = 0, mx, my, tx, ty, dx, dy;
    double  tdx, tdy, mdx, mdy;
    register struct torp *t;

    *dirP = 0;
    for (i = 0, t = &torps[eNum * MAXTORP]; i < MAXTORP; i++, t++) {
	if (t->t_status == TFREE)
	    continue;
	tx = t->t_x;
	ty = t->t_y;
	mx = me->p_x;
	my = me->p_y;
	tdx = (double) t->t_speed * Cos[t->t_dir] * WARP1;
	tdy = (double) t->t_speed * Sin[t->t_dir] * WARP1;
	mdx = (double) me->p_speed * Cos[me->p_dir] * WARP1;
	mdy = (double) me->p_speed * Sin[me->p_dir] * WARP1;
	for (j = t->t_fuse; j > 0; j--) {
	    tx += tdx;
	    ty += tdy;
	    mx += mdx;
	    my += mdy;
	    dx = tx - mx;
	    dy = ty - my;
	    if (ABS(dx) < EXPDIST && ABS(dy) < EXPDIST) {
		numHits++;
		*dirP += t->t_dir;
		break;
	    }
	}
    }
    if (numHits > 0)
	*dirP /= numHits;
    return (numHits);
}

static int
isTractoringMe(struct Enemy *enemy_buf)
{
    return ((enemy_buf->e_hisflags & PFTRACT) &&	/* bug fix: was using */
	    !(enemy_buf->e_hisflags & PFPRESS) &&	/* e_flags 6/24/92 TC */
	    (enemy_buf->e_tractor == me->p_no));
}

static struct Enemy ebuf;

static struct Enemy * get_nearest(void) {
    int     pcount = 0;
    register int i;
    register struct player *j;
    int     intruder = 0;
    int     tdist;
    double  dx, dy;
    double  vxa, vya, l;	/* Used for trap shooting */
    double  vxt, vyt;
    double  vxs, vys;
    double  dp;

    /* Find an enemy */
    ebuf.e_info = me->p_no;
    ebuf.e_dist = GWIDTH + 1;

    pcount = 0;			/* number of human players in game */

    if (target >= 0) {
	j = &players[target];
	if (!((me->p_swar | me->p_hostile) & j->p_team))
	    declare_war(players[target].p_team);	/* make sure we're at
							   war 7/31/91 TC */

	/* We have an enemy */
	/* Get his range */
	dx = j->p_x - me->p_x;
	dy = j->p_y - me->p_y;
	tdist = ihypot((int)dx, (int)dy);

	if (j->p_status != POUTFIT) {	/* ignore target if outfitting */
	    ebuf.e_info = target;
	    ebuf.e_dist = tdist;
	    ebuf.e_flags &= ~(E_INTRUDER);
	}

	/* need a loop to find hostile ships within tactical range */
	for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	    if ((j->p_status != PALIVE) || (j == me) ||
		((j->p_flags & PFROBOT) && (!hostile)))
		continue;
	    else
		pcount++;	/* Other players in the game */
	    if (((j->p_swar | j->p_hostile) & me->p_team)
		|| ((me->p_swar | me->p_hostile) & j->p_team)) {
		/* We have an enemy */
		/* Get his range */
		dx = j->p_x - me->p_x;
		dy = j->p_y - me->p_y;
		tdist = ihypot((int)dx, (int)dy);

		/* if target's teammate is too close, mark as nearest */

		if ((tdist < ebuf.e_dist) && (tdist < 15000)) {
		    ebuf.e_info = i;
		    ebuf.e_dist = tdist;
		    ebuf.e_flags &= ~(E_INTRUDER);
		}
	    }			/* end if */
	}			/* end for */
    }
    else {			/* no target */
	/* avoid dead slots, me, other robots (which aren't hostile) */
	for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	    if ((j->p_status != PALIVE) || (j == me) ||
		((j->p_flags & PFROBOT) && (!hostile)))
		continue;
	    else
		pcount++;	/* Other players in the game */
	    if (((j->p_swar | j->p_hostile) & me->p_team)
		|| ((me->p_swar | me->p_hostile) & j->p_team)) {
		/* We have an enemy */
		/* Get his range */
		dx = j->p_x - me->p_x;
		dy = j->p_y - me->p_y;
		tdist = ihypot((int)dx, (int)dy);

		/* Check to see if ship is near our planet. */
		if (startplanet != -1) {
		    int     px, py;
		    px = planets[startplanet].pl_x;
		    py = planets[startplanet].pl_y;

		    intruder = (ihypot(j->p_x - px, j->p_y - py)
				< GUARDDIST);
		}
		if (tdist < ebuf.e_dist) {
		    ebuf.e_info = i;
		    ebuf.e_dist = tdist;
		    if (intruder)
			ebuf.e_flags |= E_INTRUDER;
		    else
			ebuf.e_flags &= ~(E_INTRUDER);
		}
	    }			/* end if */
	}			/* end for */
    }				/* end else */
    if (pcount == 0) {
	return (NOENEMY);	/* no players in game */
    }
    else if (ebuf.e_info == me->p_no) {
	return (0);		/* no hostile players in the game */
    }
    else {
	j = &players[ebuf.e_info];

	/* Get torpedo course to nearest enemy */
	ebuf.e_flags &= ~(E_TSHOT);

	vxa = (j->p_x - me->p_x);
	vya = (j->p_y - me->p_y);
	l = ihypot((int)vxa, (int)vya);	/* Normalize va */
	if (l != 0) {
	    vxa /= l;
	    vya /= l;
	}
	vxs = (Cos[j->p_dir] * j->p_speed) * WARP1;
	vys = (Sin[j->p_dir] * j->p_speed) * WARP1;
	dp = vxs * vxa + vys * vya;	/* Dot product of (va vs) */
	dx = vxs - dp * vxa;
	dy = vys - dp * vya;
	l = hypot(dx, dy);	/* Determine how much speed is required */
	dp = (float) (me->p_ship.s_torp.speed * WARP1);
	l = (dp * dp - l * l);
	if (l > 0) {
	    double  he_x, he_y, area;

	    /* Only shoot if within distance */
	    he_x = j->p_x + Cos[j->p_dir] * j->p_speed * 50 * WARP1;
	    he_y = j->p_y + Sin[j->p_dir] * j->p_speed * 50 * WARP1;
	    area = 50 * me->p_ship.s_torp.speed * WARP1;
	    if (ihypot(he_x - me->p_x, he_y - me->p_y) < area) {
		l = sqrt(l);
		vxt = l * vxa + dx;
		vyt = l * vya + dy;
		ebuf.e_flags |= E_TSHOT;
		ebuf.e_tcourse = getcourse((int) vxt + me->p_x, (int) vyt + me->p_y, me->p_x, me->p_y);
	    }
	}
	/* Get phaser shot status */
	if (ebuf.e_dist < 0.8 * phrange)
	    ebuf.e_flags |= E_PSHOT;
	else
	    ebuf.e_flags &= ~(E_PSHOT);

	/* Get tractor/pressor status */
	if (ebuf.e_dist < trrange)
	    ebuf.e_flags |= E_TRACT;
	else
	    ebuf.e_flags &= ~(E_TRACT);


	/* get his phaser range */
	ebuf.e_phrange = PHASEDIST * j->p_ship.s_phaser.damage / 100;

	/* get course info */
	ebuf.e_course = getcourse(j->p_x, j->p_y, me->p_x, me->p_y);
	ebuf.e_edir = j->p_dir;
	ebuf.e_hisflags = j->p_flags;
	ebuf.e_tractor = j->p_tractor;
	/*
	   if (debug) fprintf(stderr, "Set course to enemy is %d (%d)\n",
	   (int)ebuf.e_course, (int) ebuf.e_course * 360 / 256);
	*/

	/* check to polymorph */

	if ((polymorphic) && (j->p_ship.s_type != me->p_ship.s_type) &&
	    (j->p_ship.s_type != ATT)) {	/* don't polymorph to ATT
						   4/8/92 TC */
	    int     old_shield;
	    int     old_damage;
	    old_shield = me->p_ship.s_maxshield;
	    old_damage = me->p_ship.s_maxdamage;

	    getship(&(me->p_ship), j->p_ship.s_type);
	    config();
	    if (me->p_speed > me->p_ship.s_imp.maxspeed)
		me->p_speed = me->p_ship.s_imp.maxspeed;
	    me->p_shield = (me->p_shield * (float) me->p_ship.s_maxshield)
		/ (float) old_shield;
	    me->p_damage = (me->p_damage * (float) me->p_ship.s_maxdamage)
		/ (float) old_damage;
	}
	return (&ebuf);
    }
}

static struct planet *
get_nearest_planet(void)
{
    register int i;
    register struct planet *l;
    register struct planet *nearest;
    int     dist = GWIDTH;	/* Manhattan distance to nearest planet */
    int     ldist;

    nearest = &planets[0];
    for (i = 0, l = &planets[i]; i < NUMPLANETS; i++, l++) {
	if ((ldist = (abs(me->p_x - l->pl_x) + abs(me->p_y - l->pl_y))) <
	    dist) {
	    dist = ldist;
	    nearest = l;
	}
    }
    return nearest;
}

static int
do_repair(void)
{
/* Repair if necessary (we are safe) */

    register struct planet *l;
    int     dx, dy;
    int     dist;

    l = get_nearest_planet();
    dx = abs(me->p_x - l->pl_x);
    dy = abs(me->p_y - l->pl_y);

    if (me->p_damage > 0) {
	if ((me->p_swar | me->p_hostile) & l->pl_owner) {
	    if (l->pl_armies > 0) {
		if ((dx < PFIREDIST) && (dy < PFIREDIST)) {
		    if (debug)
			fprintf(stderr, "%d) on top of hostile planet (%s)\n", me->p_no, l->pl_name);
		    return (0);	/* can't repair on top of hostile planets */
		}
		if (ihypot((int)dx, (int)dy) < PFIREDIST) {
		    if (debug)
			fprintf(stderr, "%d) on top of hostile planet (%s)\n", me->p_no, l->pl_name);
		    return (0);
		}
	    }
	    me->p_desspeed = 0;
	}
	else {			/* if friendly */
	    if ((l->pl_flags & PLREPAIR) &&
		!(me->p_flags & PFORBIT)) {	/* oh, repair! */
		dist = ihypot((int)dx, (int)dy);

		if (debug)
		    fprintf(stderr, "%d) locking on to planet %d\n",
			    me->p_no, l->pl_no);
		cloak_off();
		shield_down();
		me->p_desdir = getcourse(l->pl_x, l->pl_y, me->p_x, me->p_y);
		lock_planet(l->pl_no);
		me->p_desspeed = 4;
		if (dist - (ORBDIST / 2) < (11500 * me->p_speed * me->p_speed) /
		    me->p_ship.s_imp.dec) {
		    if (me->p_desspeed > 2) {
			set_speed(2, 1);
		    }
		}
		if ((dist < ENTORBDIST) && (me->p_speed <= 2)) {
		    me->p_flags &= ~PFPLLOCK;
		    orbit();
		}
		return (1);
	    }
	    else {		/* not repair, so ignore it */
		me->p_desspeed = 0;
	    }
	}
	shield_down();
	if (me->p_speed == 0)
	    repair();
	if (debug)
	    fprintf(stderr, "%d) repairing damage at %d\n",
		    me->p_no,
		    me->p_damage);
	return (1);
    }
    else {
	return (0);
    }
}

static char *
robo_message(struct player *enemy)
{
    int     i;
    char   *s, *t;

    i = (lrand48() % (sizeof(rmessages) / sizeof(rmessages[0])));

    for (t = rmessages[i], s = rmessage; *t != '\0'; s++, t++) {
	switch (*t) {
	case '$':
	    switch (*(++t)) {
	    case '$':
		*s = *t;
		break;
	    case 'T':		/* "a Fed" or "a Klingon" ... */
		if (enemy->p_team != me->p_team &&
		    enemy->p_team == ORI) {
		    strcpy(s, "an ");
		}
		else {
		    strcpy(s, "a ");
		}
		s = s + strlen(s);
	    case 't':		/* "Fed" or "Orion" */
		if (enemy->p_team != me->p_team) {
		    switch (enemy->p_team) {
		    case FED:
			strcpy(s, "Fed");
			break;
		    case ROM:
			strcpy(s, "Romulan");
			break;
		    case KLI:
			strcpy(s, "Klingon");
			break;
		    case ORI:
			strcpy(s, "Orion");
			break;
		    }
		}
		else {
		    strcpy(s, "TRAITOR");
		}
		s = s + strlen(s) - 1;
		break;
	    case 'f':
		switch (me->p_team) {
		case FED:
		    strcpy(s, "Federation");
		    break;
		case ROM:
		    strcpy(s, "Romulan empire");
		    break;
		case KLI:
		    strcpy(s, "Klingon empire");
		    break;
		case ORI:
		    strcpy(s, "Orion empire");
		    break;
		}
		s = s + strlen(s) - 1;
		break;
	    default:
		*s = *t;
	    }
	    break;
	default:
	    *s = *t;
	    break;
	}
    }
    *s = '\0';
    return (rmessage);
}

static char *
termie_message(struct player *enemy)
{
    int     i;
    char   *s, *t;

    i = (lrand48() % (sizeof(termie_messages) / sizeof(termie_messages[0])));

    for (t = termie_messages[i], s = tmessage; *t != '\0'; s++, t++) {
	switch (*t) {
	case '$':
	    switch (*(++t)) {
	    case '$':
		*s = *t;
		break;
	    case 'T':		/* "a Fed" or "a Klingon" ... */
		if (enemy->p_team != me->p_team &&
		    enemy->p_team == ORI) {
		    strcpy(s, "an ");
		}
		else {
		    strcpy(s, "a ");
		}
		s = s + strlen(s);
	    case 't':		/* "Fed" or "Orion" */
		if (enemy->p_team != me->p_team) {
		    switch (enemy->p_team) {
		    case FED:
			strcpy(s, "Fed");
			break;
		    case ROM:
			strcpy(s, "Romulan");
			break;
		    case KLI:
			strcpy(s, "Klingon");
			break;
		    case ORI:
			strcpy(s, "Orion");
			break;
		    }
		}
		else {
		    strcpy(s, "TRAITOR");
		}
		s = s + strlen(s) - 1;
		break;
	    case 'f':
		switch (me->p_team) {
		case FED:
		    strcpy(s, "Federation");
		    break;
		case ROM:
		    strcpy(s, "Romulan empire");
		    break;
		case KLI:
		    strcpy(s, "Klingon empire");
		    break;
		case ORI:
		    strcpy(s, "Orion empire");
		    break;
		}
		s = s + strlen(s) - 1;
		break;
	    case 'n':		/* name 8/2/91 TC */
		strcpy(s, enemy->p_name);
		s = s + strlen(s) - 1;
		break;
	    default:
		*s = *t;
	    }
	    break;
	default:
	    *s = *t;
	    break;
	}
    }
    *s = '\0';
    return (tmessage);

}

static void
messAll(char *buf)
{
    static char addrbuf[20];

    sprintf(addrbuf, " %s->ALL", twoletters(me));
    pmessage2(buf, 0, MALL, addrbuf, me->p_no);
}

static void
exitRobot(void)
{
    static char buf[80];

    r_signal(SIGALRM, SIG_IGN);
    if (me != NULL && me->p_team != ALLTEAM) {
	if (target >= 0) {
	    strcpy(buf, "I'll be back.");
	    messAll(buf);
	}
	else {
	    sprintf(buf, "%s %s (%s) leaving the game (%.16s@%.16s)",
		ranks[me->p_stats.st_rank].name,
		me->p_name,
		twoletters(me),
		me->p_login,
		me->p_monitor);
	    messAll(buf);
	}
    }

    if(configvals->robot_stats)
      save_robot();

    if (debug)
	fprintf(stderr, "%s is exiting.  Have a nice day.\n", twoletters(me));

    /* Set flags and move robot before getting rid of it. 04/11/00 JWW */
    me->p_status = PFREE;
    move_player(me->p_no, -1,-1, 1);

    /* 
       Something about Terminators hangs up the slot when a human
       tries to log in on that slot, so...
    */
    strcpy(buf, me->p_name);
    memset(me, 0, sizeof(struct player));	/* confusion 8/5/91 TC */
    strcpy(me->p_name, buf);

    /*
       all right, so zeroing out p_stats.st_tticks has undesireable side
       effects when the client tries to compute ratings...
    */
    me->p_stats.st_tticks = 1;	/* quick fix 3/15/92 TC */
    exit(0);
}

RETSIGTYPE
rmove(int unused)
{
    register struct player *j;
    register struct planet *l;
    register int i;
    register int burst;
    register int numHits, tDir;
    int     avDir;
    struct Enemy *enemy_buf;
    struct player *enemy=NULL;
    static int clock = 0;
    static int avoid[2] = {-32, 32};
    int     no_cloak;
    char    towhom[80];
    int     timer;
    static int lastTorpped = 0;	/* when we last fired a torp 4/13/92 TC */

    clock++;
    /* Check that I'm alive */
    if (me->p_status == PEXPLODE) {
	r_signal(SIGALRM, SIG_IGN);
	if (debug)
	    fprintf(stderr, "Robot: Augh! exploding.\n");

	/* hack:  get rid of robot processes! */
	if (1) {
	    sleep(3);
	}
	else {
	    while (me->p_status == PEXPLODE);
	}
	if (debug)
	    fprintf(stderr, "Robot: done exploding.\n");
	if (1) {
	    sleep(3);
	}
	else {
	    while (me->p_ntorp > 0);
	}
	if (debug)
	    fprintf(stderr, "Robot: torps are gone.\n");
	exitRobot();
    }
    if (me->p_status == PDEAD) {
	r_signal(SIGALRM, SIG_IGN);
	exitRobot();
    }
    /* keep ghostbuster away */
    me->p_ghostbuster = 0;

    timer = 0;
    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if ((j->p_status != PFREE) && !(j->p_flags & PFROBOT))
	    timer = 1;
    }
    if (!timer && !sticky) {
	r_signal(SIGALRM, SIG_IGN);
	exitRobot();
    }

    /* 
       if I'm a Terminator, quit if he quits, and quit if he dies and
       I'm not "sticky" (-s)
    */
    if (target >= 0) {
	/* he went away */
	if (players[target].p_status == PFREE) {
	    me->p_status = PEXPLODE;
	    return;
	}
	/* he died */
	if ((!sticky) && (players[target].p_status != PALIVE)) {
	    me->p_status = PEXPLODE;
	    return;
	}
    }

    /*
       If it's been BOREDOM_TIME updates since we fired a torp, become
       hostile to all races, if we aren't already, and if we're not a
       practice robot (intended for guardian bots). 4/13/92 TC
    */
    if ((clock - lastTorpped > BOREDOM_TIME) && (!practice) && (!hostile) &&
	(me->p_team != 0)) {
	messAll("Bored, Bored, Bored.");
	hostile++;
	declare_war(ALLTEAM);
    }

    /* Our first priority is to phaser plasma torps in nearby vicinity... */
    /* If we fire, we aren't allowed to cloak... */
    no_cloak = phaser_plasmas();

    /* Find an enemy */
    enemy_buf = get_nearest();

    /* Someone to kill */
    if ((enemy_buf != NULL) && (enemy_buf != NOENEMY)) {
	enemy = &players[enemy_buf->e_info];
	if (((lrand48() % messfuse) == 0) &&
	    (ihypot(me->p_x - enemy->p_x, me->p_y - enemy->p_y) < GWIDTH/10)) {
	    messfuse = MESSFUSEVAL;
	    /* change 5/10/21 TC ...neutral robots don't message */
	    if (me->p_team != 0) {
		sprintf(towhom, " %s->%s",
		    twoletters(&players[me->p_no]),
		    twoletters(&players[enemy->p_no]));
		pmessage2(robo_message(enemy), enemy->p_no,
		    MINDIV, towhom, me->p_no);
	    }
	    else if (target >= 0) {
		/*
		   MAK, 28-Apr-92 - send termie messages only to target
		   messAll(termie_message(enemy));
		*/
		sprintf(towhom, " %s->%s",
		    twoletters(&players[me->p_no]),
		    twoletters(&players[enemy->p_no]));
		pmessage2(termie_message(enemy), enemy->p_no,
		    MINDIV, towhom, me->p_no);
	    }
	}
	else if (--messfuse == 0)
	    messfuse = 1;
	timer = 0;
	if (debug)
	    fprintf(stderr, "%d) noticed %d\n", me->p_no, enemy_buf->e_info);
    }
    else if (enemy_buf == NOENEMY) {	/* no more players. wait 1 minute. */
	if (do_repair()) {
	    return;
	}
	go_home(0);
	if (debug)
	    fprintf(stderr, "%d) No players in game.\n", me->p_no);
	return;
    }
    else if (enemy_buf == 0) {	/* no one hostile */
	if (debug)
	    fprintf(stderr, "%d) No hostile players in game.\n", me->p_no);
	if (do_repair()) {
	    return;
	}
	go_home(0);
	timer = 0;
	return;
    }
/* Note a bug in this algorithm:
** Once someone dies, he is forgotten.  This makes robots particularly easy
**  to kill on a suicide run, where you aim to where you think he will turn
**  as you die.  Once dead, the robot will ignore you and all of your
**  active torpedoes!
**/

/* Algorithm:
** We have an enemy.
** First priority: shoot at target in range.
** Second: Dodge stars, torps, and plasma torps.
** Third: Get away if we are damaged.
** Fourth: repair.
** Fifth: attack.
*/

/*
** If we are a practice robot, we will do all but the second.  One
** will be modified to shoot poorly and not use phasers.
**/

    /* Fire weapons!!!
       get_nearest() has already determined if torpedoes and phasers
       will hit.  It has also determined the courses which torps and 
       phasers should be fired.  If so we will go ahead and shoot here.
       We will lose repair and cloaking for the rest of this interrupt.
       if we fire here.
    */
    if (practice) {
	no_cloak = 1;
	if (enemy_buf->e_flags & E_TSHOT) {
	    if (debug)
		fprintf(stderr, "%d) firing torps\n", me->p_no);
	    for (burst = 0; (burst < 3) && (me->p_ntorp < MAXTORP); burst++) {
		ntorp(enemy_buf->e_tcourse, TMOVE);
	    }
	}
    }
    else {
	if (enemy_buf->e_flags & E_TSHOT) {
	    if (debug)
		fprintf(stderr, "%d) firing torps\n", me->p_no);
	    for (burst = 0; (burst < 2) && (me->p_ntorp < MAXTORP); burst++) {
		repair_off();
		cloak_off();
		ntorp(enemy_buf->e_tcourse, TMOVE);
		   /* was TSTRAIGHT 8/9/91 TC */
		no_cloak++;
		lastTorpped = clock;	/* record time of firing 4/13/92 TC */
	    }
	}
	if (enemy_buf->e_flags & E_PSHOT) {
	    if (debug)
		fprintf(stderr, "%d) phaser firing\n", me->p_no);
	    no_cloak++;
	    repair_off();
	    cloak_off();
	    phaser(enemy_buf->e_course);
	}
    }

    /* auto pressor 7/27/91 TC */

    /* tractor/pressor rewritten on 5/1/92... glitches galore :-| TC */

    /*
       whoa, too close for comfort, or he's tractoring me, or headed in for
       me, or I'm hurt
    */

    /* a little tuning -- 0.8 on phrange and +/- 90 degrees in for pressor */

    /*
       pressor_player(-1); this didn't do anything before, so we'll let the
       pressors disengage by themselves 5/1/92 TC
    */
    if (enemy_buf->e_flags & E_TRACT) {	/* if pressorable */
	if (((enemy_buf->e_dist < 0.8 * enemy_buf->e_phrange) &&
	     (angdist(enemy_buf->e_edir, enemy_buf->e_course) > 64)) ||
	    (isTractoringMe(enemy_buf)) ||
	    (me->p_damage > 0)) {
	    if (!(enemy->p_flags & PFCLOAK)) {
		if (debug)
		    fprintf(stderr, "%d) pressoring %d\n", me->p_no,
			enemy_buf->e_info);
		pressor_player(enemy->p_no);
		no_cloak++;
		repair_off();
		cloak_off();
	    }
	}
    }

    /* auto tractor 7/31/91 TC */

    /* 
       Tractor if not pressoring and tractor if in range, not too close, 
       and not headed +/- 90 degrees of me, and I'm not hurt
    */
    if ((!(me->p_flags & PFPRESS)) &&
	(enemy_buf->e_flags & E_TRACT) &&
	(angdist(enemy_buf->e_edir, enemy_buf->e_course) < 64) &&
	(enemy_buf->e_dist > 0.7 * enemy_buf->e_phrange)) {
	if (!(me->p_flags & PFTRACT)) {
	    if (debug)
		fprintf(stderr, "%d) tractoring %d\n", me->p_no,
		    enemy_buf->e_info);
	    tractor_player(enemy->p_no);
	    no_cloak++;
	}
    }
    else
	tractor_player(-1);	/* otherwise don't tractor */

    /* Avoid stars - MAK, 16-Jun-93 */
    /*
       This section of code allows robots to avoid crashing into stars.
       Within a specific range (AVOID_STAR), they will check to see if their
       current course will take them close to the star.  If so, course will
       be adjusted to avoid the star. Within a smaller range (AVOID_MELT),
       course is adjusted directly away from the star. Collision avoidance is
       all they will do for this round, other than shooting.
    */
    for (i = 0, l = &planets[0]; i < NUMPLANETS; i++, l++) {
	if (PL_TYPE(*l) == PLSTAR) {
	    int     dx, dy, stardir, newcourse = -1;
	    dx = ABS(l->pl_x - me->p_x);
	    dy = ABS(l->pl_y - me->p_y);
	    /* Avoid over box rather than circle to speed calculations */
	    if (dx < AVOID_STAR && dy < AVOID_STAR) {
		stardir = getcourse(l->pl_x, l->pl_y, me->p_x, me->p_y);

		if (dx < AVOID_MELT && dy < AVOID_MELT) {
		    /* Emergency! Way too close! */
		    newcourse = NORMALIZE(stardir - 128);
		    if (debug) {
			fprintf(stderr, "Steering away from star %s, dir = %d\n",
			    l->pl_name, newcourse);
		    }
		}
		else if (angdist(me->p_dir, stardir) < 16) {
		    /* Heading towards star, so adjust course. */
		    newcourse =
			NORMALIZE((me->p_dir < stardir) ? stardir - 32 : stardir + 32);
		    if (debug) {
			fprintf(stderr, "Adjusting course away from star %s, dir = %d\n",
			    l->pl_name, newcourse);
		    }
		}
		if (newcourse != -1) {	/* Change course and speed */
		    me->p_desdir = newcourse;
		    if (angdist(me->p_desdir, me->p_dir) > 64)
			me->p_desspeed = dogslow;
		    else
			me->p_desspeed = dogfast;
		    return;
		}
	    }
	}
    }

    /* Avoid torps */
    /*
       This section of code allows robots to avoid torps. Within a
       specific range they will check to see if any of the 'closest'
       enemies torps will hit them.  If so, they will evade for four
       updates.  Evading is all they will do for this round, other than
       shooting.
    */
    if (!practice) {
	if ((enemy->p_ntorp < 5)) {
	    if ((enemy_buf->e_dist < 15000) || (avoidTime > 0)) {
		numHits = projectDamage(enemy->p_no, &avDir);
		if (debug) {
		    fprintf(stderr, "%d hits expected from %d from dir = %d\n", numHits, enemy->p_no, avDir);
		}
		if (numHits == 0) {
		    if (--avoidTime > 0) {	/* we may still be avoiding */
			if (angdist(me->p_desdir, me->p_dir) > 64)
			    me->p_desspeed = dogslow;
			else
			    me->p_desspeed = dogfast;
			return;
		    }
		}
		else {
		    /* Actually avoid Torps */
		    avoidTime = AVOID_TIME;
		    tDir = avDir - me->p_dir;
		    /* put into 0->255 range */
		    tDir = NORMALIZE(tDir);
		    if (debug)
			fprintf(stderr, "mydir = %d avDir = %d tDir = %d q = %d\n",
			    me->p_dir, avDir, tDir, tDir / 64);
		    switch (tDir / 64) {
		    case 0:
		    case 1:
			set_course(NORMALIZE(avDir + 64));
			break;
		    case 2:
		    case 3:
			set_course(NORMALIZE(avDir - 64));
			break;
		    }
		    if (!no_cloak)
			cloak_on();

		    if (angdist(me->p_desdir, me->p_dir) > 64)
			me->p_desspeed = dogslow;
		    else
			me->p_desspeed = dogfast;

		    shield_up();
		    detothers();	/* hmm */
		    if (debug)
			fprintf(stderr, "evading to dir = %d\n", me->p_desdir);
		    return;
		}
	    }
	}
	/*
	   Trying another scheme.  Robot will keep track of the number of
	   torps a player has launched.  If they are greater than say four,
	   the robot will veer off immediately.  Seems more humanlike to me.
	*/
	else if (enemy_buf->e_dist < 15000) {
	    if (--avoidTime > 0) {	/* we may still be avoiding */
		if (angdist(me->p_desdir, me->p_dir) > 64)
		    me->p_desspeed = dogslow;
		else
		    me->p_desspeed = dogfast;
		return;
	    }
	    if (lrand48() % 2) {
		me->p_desdir = NORMALIZE(enemy_buf->e_course - 64);
		avoidTime = AVOID_TIME;
	    }
	    else {
		me->p_desdir = NORMALIZE(enemy_buf->e_course + 64);
		avoidTime = AVOID_TIME;
	    }
	    if (angdist(me->p_desdir, me->p_dir) > 64)
		me->p_desspeed = dogslow;
	    else
		me->p_desspeed = dogfast;
	    shield_up();
	    return;
	}
    }

    /* Run away
       The robot has taken damage.  He will now attempt to run away from
       the closest player.  This obviously won't do him any good if there
       is another player in the direction he wants to go.  Note that the
       robot will not run away if he dodged torps, above.  The robot will
       lower his shields in hopes of repairing some damage.
    */
    if (me->p_damage > 0 && enemy_buf->e_dist < 13000) {
	if (me->p_etemp > 900)	/* 90% of 1000 */
	    me->p_desspeed = runslow;
	else
	    me->p_desspeed = runfast;
	if (!no_cloak)
	    cloak_on();
	repair_off();
	shield_down();
	set_course(enemy_buf->e_course - 128);
	if (debug)
	    fprintf(stderr, "%d(%d)(%d/%d) running from %s %16s damage (%d/%d) dist %d\n",
	        me->p_no,
		(int) me->p_kills,
		me->p_damage,
		me->p_shield,
		twoletters(enemy),
		enemy->p_login,
		enemy->p_damage,
		enemy->p_shield,
		enemy_buf->e_dist);
	return;
    }

    /* Repair if necessary (we are safe) */
    /*
       The robot is safely away from players.  It can now repair in peace.
       It will try to do so now.
    */
    if (do_repair()) {
	return;
    }

    /* Attack. */
    /*
       The robot has nothing to do.  It will check and see if the nearest
       enemy fits any of its criterion for attack.  If it does, the robot
       will speed in and deliver a punishing blow.  (Well, maybe)
    */
    if ((enemy_buf->e_flags & E_INTRUDER) || (enemy_buf->e_dist < 15000)
	|| (hostile)) {
	if ((!no_cloak) && (enemy_buf->e_dist < 10000))
	    cloak_on();
	shield_up();
	if (debug)
	    fprintf(stderr, "%d(%d)(%d/%d) attacking %s %16s damage (%d/%d) dist %d\n",
		me->p_no,
		(int) me->p_kills,
		me->p_damage,
		me->p_shield,
		twoletters(enemy),
		enemy->p_login,
		enemy->p_damage,
		enemy->p_shield,
		enemy_buf->e_dist);

	if (enemy_buf->e_dist < 15000) {
	    set_course(enemy_buf->e_course +
		       avoid[(clock / AVOID_CLICKS) % SIZEOF(avoid)]);
	    if (angdist(me->p_desdir, me->p_dir) > 64)
		set_speed(closeslow, 1);
	    else
		set_speed(closefast, 1);
	}
	else {
	    me->p_desdir = enemy_buf->e_course;
	    if (angdist(me->p_desdir, me->p_dir) > 64)
		set_speed(closeslow, 1);
	    if (target >= 0)	/* 7/27/91 TC */
		set_speed(12, 1);
	    else if (me->p_etemp > 900)	/* 90% of 1000 */
		set_speed(runslow, 1);
	    else
		set_speed(runfast, 1);
	}
    }
    else {
	go_home(enemy_buf);
    }
}
