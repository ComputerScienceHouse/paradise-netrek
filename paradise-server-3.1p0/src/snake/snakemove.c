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

#include <signal.h>
#include "config.h"
#include "proto.h"
#include "data.h"
#include "shmem.h"

#define SYSWIDTH      (GWIDTH/5)/* width of a system */

/* distance on either side of center */
#define EYEWIDTH		100	/* 100 */
/* which torp the eyes are positioned alongside */
#define EYETORP			0	/* 0 */
/* angle from EYETORP */
#define EYEANGLE		64	/* 64 */
#define TORPSEP			WARP1*12
#define PLANRAD  900
#define PLANRATE 16

/* threshold to avoid galactic boundaries */
#define MD		4500
/* overlap threshold on team boundaries */
#define TMD		2250

/* how often to check for bombers in cyles (1/10th second) */
#define BOMBCHECK	20

/* how often to check for players in the game in cyles (1/10th second) */
#define PLAYERCHECK	100

#define WARHOSTILE(p)		((((p)->p_swar | (p)->p_hostile)& \
				perfs[0]->p_team) || ((perfs[0]->p_swar | \
				perfs[0]->p_hostile) & (p)->p_team))

#define SNAKETORPDAMAGE		30
#define SNAKEPLASMADAMAGE	100

static struct torp *thead, *eyet, *eyetp;
static int lx, ly;
static struct plasmatorp *fl, *fr;
static int explode, explodetorps, tfuse;

extern struct player *perfs[2];
extern int num_perfs;
extern int debug;
extern int lastm;
extern int tno;
extern int target;
extern int berserk;
extern int patrol;
extern int noSmush;
extern int plan_guard;		/* KAO */
extern int planet1, planet2;	/* KAO */
extern int team1, team2;
extern int length;

static int plan_count = 0;
static int s_clock;
static int _move(void);
static int defenders[ALLTEAM];

static struct planet *homeworld = NULL;
static struct planet *homestar = NULL;

static struct planet *
star_of(struct planet *pl)
{
    int     i;
    if (pl->pl_system < 1)
	return 0;

    for (i = 0; i < NUMPLANETS; i++) {
	if (!(planets[i].pl_flags & PLSTAR))
	    continue;
	if (planets[i].pl_system == pl->pl_system)
	    return &planets[i];
    }

    return 0;
}



RETSIGTYPE
snakemove(int unused)
{
    s_clock++;

    _move();
}

static void
snake_torp(struct torp *t, int n, struct player *p)
{
    t->t_no = n;
    t->t_status = TMOVE;
    t->t_owner = p->p_no;
    t->t_team = p->p_team;
    t->t_dir = 128;
    if (plan_guard == 0)
	t->t_damage = SNAKETORPDAMAGE;
    else
	t->t_damage = SNAKETORPDAMAGE * 10;
    t->t_speed = 0;
    if (berserk)
	t->t_war = FED | ROM | ORI | KLI;
    else if (target == -1)
	t->t_war = 0;
    else
	t->t_war = perfs[0]->p_swar | perfs[0]->p_hostile;
    t->t_fuse = INT_MAX;
    t->t_turns = 0;
    p->p_ntorp++;
}

static void
startsnake(void)
{
    register int i, l;
    register struct player *j;
    register struct torp *k;
    struct player *p1 = perfs[0], *p2 = perfs[1];
    int     first = 1;
    int     px, py;

    for (i = 0; i < NUMPLANETS; i++) {
	if (planets[i].pl_flags & PLHOME && planets[i].pl_owner == 1 << tno) {
	    homeworld = &planets[i];
	    break;
	}
	if (!homeworld && planets[i].pl_owner == 1 << tno)
	    homeworld = &planets[i];
    }

    if (!homeworld) {
	/* ouch */
	homeworld = &planets[lrand48() % NUMPLANETS];
	fprintf(stderr, "snake: My race (%d) has no planets.  Picking one at random: %s\n", tno, homeworld->pl_name);
    }
    homestar = star_of(homeworld);
    if (!homestar)
	homestar = homeworld;

    px = (lrand48() % 10000) - 5000;
    py = (lrand48() % 10000) - 5000;

    p1->p_ntorp = p2->p_ntorp = 0;
    p1->p_nplasmatorp = p2->p_nplasmatorp = 0;

    /* body */

    for (j = perfs[0]; j; j = (j == perfs[1] ? NULL : perfs[1])) {

	for (l = 0, i = j->p_no * MAXTORP, k = &torps[i];
	     i < j->p_no * MAXTORP + length; i++, k++, l++) {

	    if (l == EYETORP && j == perfs[0]) {
		/* eye location */
		eyet = k;
	    }
	    if (l == ((EYETORP == 0) ? 1 : (EYETORP - 1)) && j == perfs[0]) {
		/* perpendicular torp offset */
		eyetp = k;
	    }
	    /* torps are free here */
	    snake_torp(k, i, j);
	    /* note: have to be same team for this to work */
	    k->t_x = homeworld->pl_x + px;
	    k->t_y = homeworld->pl_y + py - (l + (j == perfs[1] ? length : 0)) * TORPSEP;
	    /*
	       if(debug) fprintf(stderr, "k->t_x %d, k->t_y %d\n", k->t_x,
	       k->t_y);
	    */
	    if (first) {
		thead = k;
		first = 0;
	    }
	}
    }

    /* eyes */
    fl = &plasmatorps[perfs[0]->p_no * MAXPLASMA];
    fl->pt_no = perfs[0]->p_no * MAXPLASMA;
    fl->pt_status = PTMOVE;
    fl->pt_owner = perfs[0]->p_no;
    fl->pt_team = perfs[0]->p_team;
    fl->pt_x = eyet->t_x - EYEWIDTH;
    fl->pt_y = eyet->t_y;
    if (plan_guard == 0)
	fl->pt_damage = SNAKEPLASMADAMAGE;
    else
	fl->pt_damage = SNAKEPLASMADAMAGE * 10;
    fl->pt_speed = 0;
    fl->pt_war = 0;
    fl->pt_fuse = INT_MAX;
    fl->pt_turns = 0;

    perfs[0]->p_nplasmatorp++;

    fr = &plasmatorps[perfs[1]->p_no * MAXPLASMA];
    fr->pt_no = perfs[1]->p_no * MAXPLASMA;
    fr->pt_status = PTMOVE;
    fr->pt_owner = perfs[1]->p_no;
    fr->pt_team = perfs[1]->p_team;	/* doesn't work */
    fr->pt_x = eyet->t_x + EYEWIDTH;
    fr->pt_y = eyet->t_y;
    fr->pt_damage = SNAKEPLASMADAMAGE;
    fr->pt_speed = 0;
    fr->pt_war = 0;
    fr->pt_fuse = INT_MAX;
    fr->pt_turns = 0;

    perfs[1]->p_nplasmatorp++;

    if (debug)
	fprintf(stderr, "started\n");
}

/*
 * start planet areas are places where people will likely enter the game
 * (only considers 1 start planet at this time)
 */

static int
sp_area(int x, int y)
{
    register int i,  px, py;

    for (i = 0; i < NUMPLANETS; i++) {
	struct planet *pl = &planets[i];
	if (!(pl->pl_flags & (PLSHIPYARD | PLHOME)))
	    continue;
	px = pl->pl_x;
	py = pl->pl_y;
	/* printf("checking %s (%d,%d)\n", pl->pl_name, i*10,j); */

	if (!(x < px - 5300 || x > px + 5300 ||
	      y < py - 5300 || y > py + 5300)) {
	    return 1;
	}
    }

    return 0;
}


static void
doeyes(void)
{
    unsigned char c = getcourse(eyetp->t_x, eyetp->t_y, eyet->t_x,
				 eyet->t_y);
    int     ew = EYEWIDTH, war_ok;

    /* c + 64 -- fl c - 64 -- fr */

    if (plan_guard) {
	fl->pt_x = planets[planet1].pl_x + Cos[255 - plan_count] * PLANRAD;
	fl->pt_y = planets[planet1].pl_y + Sin[255 - plan_count] * PLANRAD;
	fr->pt_x = planets[planet2].pl_x + Cos[255 - plan_count] * PLANRAD;
	fr->pt_y = planets[planet2].pl_y + Sin[255 - plan_count] * PLANRAD;
	return;
    }
    if (fl->pt_war)
	ew += 15;

    fl->pt_x = eyet->t_x + (double) (ew) * Cos[(unsigned char) (c - EYEANGLE)];
    fl->pt_y = eyet->t_y + (double) (ew) * Sin[(unsigned char) (c - EYEANGLE)];

    if (fr->pt_war)
	ew += 15;

    fr->pt_x = eyet->t_x + (double) (ew) * Cos[(unsigned char) (c + EYEANGLE)];
    fr->pt_y = eyet->t_y + (double) (ew) * Sin[(unsigned char) (c + EYEANGLE)];

    /* toggle war */
    if ((s_clock % 6) == 0) {
	/*
	   your home planet and shipyards are always safe from your own snake
	   unless you are a target.
	*/
	war_ok = !sp_area(thead->t_x, thead->t_y);

	if (!fr->pt_war && war_ok)
	    fr->pt_war = (FED | ROM | KLI | ORI) & ~(war_ok ? 0 : (1 << tno));
	else
	    fr->pt_war = 0;

	if (!fl->pt_war && war_ok)
	    fl->pt_war = (FED | ROM | KLI | ORI) & ~(war_ok ? 0 : (1 << tno));
	else
	    fl->pt_war = 0;
    }
}

/*
 * FED 1
 * ROM 2
 * KLI 4
 * ORI 8
 */

/*
 *
 *    256/0
 * 192  +   64
 *     128
 */

static void
check_tboundary(int teams, unsigned char *mycrs, int range, int x, int y)
{
    int     lx = homestar->pl_x - SYSWIDTH / 2, rx = homestar->pl_x + SYSWIDTH / 2, ty = homestar->pl_y - SYSWIDTH / 2,
            by = homestar->pl_y + SYSWIDTH / 2;
    int     r = 0, l = 0;
    unsigned char crs = *mycrs;

    if (x < lx && crs >= 128) {
	if (crs >= 192)
	    *mycrs += range;
	else
	    *mycrs -= range;
	l = 1;
    }
    else if (x > rx && crs < 128) {
	if (crs > 64)
	    *mycrs += range;
	else
	    *mycrs -= range;
	r = 1;
    }
    if (y < ty && (crs >= 192 || crs < 64)) {
	if (crs >= 192 && !l)
	    *mycrs -= range;
	else
	    *mycrs += range;
    }
    else if (y > by && (crs < 192 && crs >= 64)) {
	if (crs < 128 && !r)
	    *mycrs -= range;
	else
	    *mycrs += range;
    }
}

RETSIGTYPE
exitSnake(int sig)
{
    register int i;
    register struct player *j;
    register struct torp *k;
    register struct plasmatorp *f;

    r_signal(SIGALRM, SIG_DFL);

    if (debug)
	fprintf(stderr, "snake exiting\n");

    for (j = perfs[0]; j; j = (j == perfs[1] ? NULL : perfs[1])) {
	for (i = j->p_no * MAXTORP, k = &torps[i];
	     i < j->p_no * MAXTORP + length; i++, k++) {
	    /* torps are free here */
	    if (k->t_status != TFREE)
		k->t_status = TOFF;
	}
	f = &plasmatorps[j->p_no * MAXPLASMA];
	f->pt_status = PTFREE;
	/* plasma */
    }

    memset(perfs[0], 0, sizeof(struct player));
    memset(perfs[1], 0, sizeof(struct player));
    perfs[0]->p_stats.st_tticks = 1;
    perfs[1]->p_stats.st_tticks = 1;

    /*
       note: if we had a segmentation violation or bus error we want a core
       file to debug
    */
    if (sig == SIGBUS || sig == SIGSEGV)
	abort();
    else
	exit(0);
}

static void
make_war(struct player *p, int plteam)
{
    register int i;
    register struct player *j;
    register struct torp *k;

    perfs[0]->p_team = plteam;
    perfs[0]->p_swar = 0;
    perfs[0]->p_hostile = 0;
    perfs[1]->p_team = plteam;
    perfs[1]->p_swar = 0;
    perfs[1]->p_hostile = 0;

    perfs[0]->p_swar |= p->p_team;
    perfs[0]->p_hostile |= p->p_team;
    perfs[1]->p_swar |= p->p_team;
    perfs[1]->p_hostile |= p->p_team;

    /* update our torps war status */
    for (j = perfs[0]; j; j = (j == perfs[1] ? NULL : perfs[1])) {
	for (i = j->p_no * MAXTORP, k = &torps[i];
	     i < j->p_no * MAXTORP + length; i++, k++) {

	    k->t_war = perfs[0]->p_swar | perfs[0]->p_hostile;
	}
    }
}

static int
bombcheck(int team1, int team2)
{
    register int i;
    register struct player *j;
    struct planet *p;

    for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
	if (j->p_status == PALIVE) {
	    if ((j->p_flags & PFBOMB) && (j->p_flags & PFORBIT)) {
		p = &planets[j->p_planet];
		if (!((j->p_swar | j->p_hostile) & p->pl_owner))
		    continue;

		if (defenders[p->pl_owner] >= configvals->tournplayers)
		    continue;

		if (!team1 && !team2) {
		    /* any planet */
		    printf("snake found bomber: targeting %c%c\n",
			   teams[j->p_team].letter, shipnos[j->p_no]);
		    fflush(stdout);
		    make_war(j, p->pl_owner);
		    return j->p_no;
		}
		else {
		    if (team1) {
			if (p->pl_owner == team1) {
			    printf("found bomber: targeting %c%c\n",
				 teams[j->p_team].letter, shipnos[j->p_no]);
			    fflush(stdout);
			    make_war(j, p->pl_owner);
			    return j->p_no;
			}
		    }
		    if (team2) {
			if (p->pl_owner == team2) {
			    printf("found bomber: targeting %c%c\n",
				 teams[j->p_team].letter, shipnos[j->p_no]);
			    fflush(stdout);
			    make_war(j, p->pl_owner);
			    return j->p_no;
			}
		    }
		}
	    }
	}
    }
    return -1;
}

/* return rnd between -range & range */
static int
rrnd(int range)
{
    return lrand48() % (2 * range) - range;
}

static void
movesnake(void)
{
    register int i, px, py;
    register struct player *j;
    register struct torp *k /* , *prev = thead */ ;
    unsigned char tc;
    struct player *tr;
    static
    int     dir = 8;
    int     ok;

    if ((s_clock % PLAYERCHECK) == 0) {
	/* every x seconds make sure there's people in the game */
	ok = 0;
	defenders[FED] = defenders[ROM] = defenders[KLI] = defenders[ORI] = 0;
	for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
	    if ((j->p_status != PFREE) && !(j->p_flags & PFROBOT)) {
		ok = 1;
		defenders[j->p_team]++;
	    }
	}
	if (!ok)
	    exitSnake(0);
    }
    if (patrol && (target == -1) && (s_clock % BOMBCHECK) == 0) {
	target = bombcheck(team1, team2);
    }
    if ((s_clock % (4 + (lrand48() % 4))) == 0)
	dir = -dir;

    thead->t_dir += dir + rrnd(8);

    if (target > -1) {
	tr = &players[target];
	if (tr->p_status == PALIVE) {
	    int     nd, td;
	    tc = getcourse(tr->p_x, tr->p_y, thead->t_x, thead->t_y);
	    nd = angdist(thead->t_dir, tc);
	    if (nd > 8) {
		td = tc + nd;
		if (td == thead->t_dir)
		    thead->t_dir -= 8;
		else
		    thead->t_dir += 8;
	    }
	}
    }
    if (target == -1) {
	if (!patrol)
	    check_tboundary(ALLTEAM, &thead->t_dir, 8, thead->t_x, thead->t_y);
	else
	    check_tboundary(team1 | team2, &thead->t_dir, 8, thead->t_x, thead->t_y);
    }
    lx = thead->t_x;
    ly = thead->t_y;

    if (!plan_guard) {
	/* NOTE: we aren't letting the daemon move the torp head */
	thead->t_x += (double) TORPSEP *Cos[thead->t_dir];
	thead->t_y += (double) TORPSEP *Sin[thead->t_dir];
    }
    for (j = perfs[0]; j; j = (j == perfs[1] ? NULL : perfs[1])) {
	if (plan_guard) {
	    int     temp;
	    thead = &torps[j->p_no * MAXTORP];
	    if (j == perfs[0])
		temp = planet1;
	    else
		temp = planet2;
	    lx = thead->t_x;
	    ly = thead->t_y;
	    thead->t_x = planets[temp].pl_x + Cos[255 - plan_count] * PLANRAD;
	    thead->t_y = planets[temp].pl_y + Sin[255 - plan_count] * PLANRAD;
	}
	for (i = j->p_no * MAXTORP, k = &torps[i];
	     i < j->p_no * MAXTORP + length; i++, k++) {

	    if (k->t_status == TFREE)
		/* got exploded.  x & y location will remain however */
		snake_torp(k, i, j);

	    if (k == thead)
		continue;

	    px = k->t_x;
	    py = k->t_y;

	    k->t_x = lx;
	    k->t_y = ly;

	    lx = px;
	    ly = py;
	}
    }
    if (plan_guard) {
	thead = &torps[perfs[0]->p_no * MAXTORP];
	plan_count = (plan_count + PLANRATE) % 256;
    }
    lx = thead->t_x;
    ly = thead->t_y;
    doeyes();
}

static int
crash_killer(struct player *p)
{
    register int i, px, py;
    register struct player *j;
    register struct torp *k;
    unsigned char tc;
    int     active = 0;

    tfuse++;

    tc = getcourse(p->p_x, p->p_y, thead->t_x, thead->t_y);
    thead->t_dir = tc;

    lx = thead->t_x;
    ly = thead->t_y;

    thead->t_x += (double) (12 * WARP1) * Cos[thead->t_dir];
    thead->t_y += (double) (12 * WARP1) * Sin[thead->t_dir];

    for (j = perfs[0]; j; j = (j == perfs[1] ? NULL : perfs[1])) {
	for (i = j->p_no * MAXTORP, k = &torps[i];
	     i < j->p_no * MAXTORP + length; i++, k++) {

	    /* move the head up if it exploded */
	    if (k != thead && thead->t_status == TFREE)
		thead = k;
	    else if (k == thead)
		continue;

	    if (k->t_status != TFREE)
		active++;

	    px = k->t_x;
	    py = k->t_y;

	    k->t_x = lx;
	    k->t_y = ly;

	    lx = px;
	    ly = py;
	}
    }
    return active;
}

static void
restore_eye(void)
{
    if (fl->pt_status != PTMOVE) {
	/* eyes */
	fl = &plasmatorps[perfs[0]->p_no * MAXPLASMA];
	fl->pt_no = perfs[0]->p_no * MAXPLASMA;
	fl->pt_status = PTMOVE;
	fl->pt_owner = perfs[0]->p_no;
	fl->pt_team = perfs[0]->p_team;
	fl->pt_x = eyet->t_x - EYEWIDTH;
	fl->pt_y = eyet->t_y;
	if (plan_guard == 0)
	    fl->pt_damage = SNAKEPLASMADAMAGE;
	else
	    fl->pt_damage = SNAKEPLASMADAMAGE * 10;
	fl->pt_speed = 0;
	fl->pt_war = 0;
	fl->pt_fuse = INT_MAX;
	fl->pt_turns = 0;
	perfs[0]->p_nplasmatorp++;
    }
    if (fr->pt_status != PTMOVE) {
	fr = &plasmatorps[perfs[1]->p_no * MAXPLASMA];
	fr->pt_no = perfs[1]->p_no * MAXPLASMA;
	fr->pt_status = PTMOVE;
	fr->pt_owner = perfs[1]->p_no;
	fr->pt_team = perfs[1]->p_team;	/* doesn't work */
	fr->pt_x = eyet->t_x + EYEWIDTH;
	fr->pt_y = eyet->t_y;
	fr->pt_damage = SNAKEPLASMADAMAGE;
	fr->pt_speed = 0;
	fr->pt_war = 0;
	fr->pt_fuse = INT_MAX;
	fr->pt_turns = 0;
	perfs[1]->p_nplasmatorp++;
    }
}

static struct player *
whokilledme(struct plasmatorp *pt)
{
    register int i;
    register struct phaser *j;

    for (i = 0, j = &phasers[i]; i < MAXPLAYER; i++, j++) {
	if (j->ph_status == PHHIT2) {
	    if (debug) {
		fprintf(stderr, "found PHHIT2 from %d at %d,%d\n", 
			players[i].p_no, j->ph_x, j->ph_y);
		fprintf(stderr, "plasma is at %d,%d\n", pt->pt_x, pt->pt_y);
		fprintf(stderr, "fl is at %d,%d\n", fl->pt_x, fl->pt_y);
		fprintf(stderr, "fr is at %d,%d\n", fr->pt_x, fr->pt_y);
	    }
	    if (j->ph_x == pt->pt_x && j->ph_y == pt->pt_y) {
		return &players[i];
	    }
	}
    }
    return NULL;
}

/* NOTE: the procedure could be writing shared memory variables at the same time
   as the daemon.  This may produce unpredicatable results.  A better
   implementation would mark the "snake plasma" and have the daemon do the
   awarding. */
static void
award(struct player *win)
{
    char    buf[80];
    char    addrbuf[10];

    if (!win)
	return;

    if (target == -1 && !(win->p_flags & PFROBOT) && !WARHOSTILE(win)) {
	strcpy(buf, "Snake eyes!");

	/* what do we have for our big winner today, fred? */

	if (((100 * win->p_damage) / win->p_ship.s_maxdamage) > 50 ||
	    ((100 * win->p_shield) / win->p_ship.s_maxshield) < 50) {
	    win->p_damage = 0;
	    win->p_shield = win->p_ship.s_maxshield;
	    strcat(buf, " You win free repairs!");
	}
	else if (((100 * win->p_fuel) / win->p_ship.s_maxfuel) < 50) {
	    win->p_fuel = win->p_ship.s_maxfuel;
	    strcat(buf, " You win free fuel!");
	}
	else if (((100 * win->p_etemp) / win->p_ship.s_maxegntemp) > 60) {
	    win->p_etemp = 0;
	    strcat(buf, " You win free engine cooling!");
	}
	else if (((100 * win->p_wtemp) / win->p_ship.s_maxwpntemp) > 60) {
	    win->p_wtemp = 0;
	    strcat(buf, " You win free weapons cooling!");
	}
	else {
	    win->p_damage = 0;
	    win->p_shield = win->p_ship.s_maxshield;
	    win->p_fuel = win->p_ship.s_maxfuel;
	    win->p_etemp = 0;
	    win->p_wtemp = 0;
	    strcat(buf, " You feel healthy!");
	}

	/* ... */

	sprintf(addrbuf, "%s->%c%c", SERVNAME,
		teams[win->p_team].letter, shipnos[win->p_no]);
	pmessage2(buf, win->p_no, MINDIV, addrbuf, 255);
    }
    sprintf(buf, "%s (%c%c) slew the vile space serpent!",
	    win->p_name, teams[win->p_team].letter, shipnos[win->p_no]);
    pmessage2(buf, 0, MALL | MKILLA, MSERVA, 255);

    /* and get .5 kills */

    win->p_kills += 0.5;
    if (win->p_ship.s_type == STARBASE) {
	if (win->p_stats.st_sbmaxkills < win->p_kills) {
	    win->p_stats.st_sbmaxkills = win->p_kills;
	}
    }
    else if (win->p_stats.st_tmaxkills < win->p_kills) {
	win->p_stats.st_tmaxkills = win->p_kills;
    }
}

static void
check_explode(void)
{
    register int i, l;
    register struct player *j;
    register struct torp *k;
    static struct player *killer;

    if (fl->pt_status == PTDET || fr->pt_status == PTDET ||
      fl->pt_status == PTEXPLODE || fr->pt_status == PTEXPLODE || explode) {
	if (plan_guard) {
	    restore_eye();
	    return;
	}			/* KAO */
	if (debug)
	    fprintf(stderr, "snake exploding\n");
	if (!explode) {
	    /* do once */
	    explode = s_clock;
	    tfuse = 0;
	    if (fl->pt_status == PTDET || fl->pt_status == PTEXPLODE) {
		killer = whokilledme(fl);
		if (killer)
		    award(killer);
	    }
	    if (fr->pt_status == PTDET || fr->pt_status == PTEXPLODE) {
		killer = whokilledme(fr);
		if (killer)
		    award(killer);
	    }
	}
	if (fl->pt_status != PTFREE && fl->pt_status != PTEXPLODE) {
	    fl->pt_war = FED | ROM | KLI | ORI;	/* make sure its at war when
						   it explodes */
	    fl->pt_status = PTEXPLODE;
	    fl->pt_fuse = 10;
	}
	if (fl->pt_status != PTFREE && fr->pt_status != PTEXPLODE) {
	    fl->pt_war = FED | ROM | KLI | ORI;
	    fr->pt_status = PTEXPLODE;
	    fr->pt_fuse = 10;
	}

	/*
	   now for some fancy stuff. If killer is our target and is hostile
	   or at war with our team then snake torp head makes a beeline for
	   him.  This lasts until killer dies or torps have been chasing
	   killer for 40 cycles
	*/
	if (killer  /* someone killed it */
	    && WARHOSTILE(killer)
	    && (noSmush < 1)
	    && (killer->p_status == PALIVE)
	    && tfuse < 40
	    && (killer->p_no == target || noSmush < 0)) {
	    crash_killer(killer);
	}
	else {
	    /* explode all torps in sequence, 1 per cycle until all gone */
	    for (j = perfs[0]; j; j = (j == perfs[1] ? NULL : perfs[1])) {
		for (l = (j == perfs[0]) ? 0 : length, i = j->p_no * MAXTORP, 
		     k = &torps[i]; i < j->p_no * MAXTORP + length; 
		     i++, k++, l++) {
		    if (l == explodetorps && k->t_status != TEXPLODE &&
			k->t_status != TFREE) {
			k->t_status = TEXPLODE;
			k->t_fuse = 10;
			explodetorps++;
			return;
		    }
		    else if (l == explodetorps)
			explodetorps++;
		}
	    }
	}
    }
}



static 
int _move(void)
{
    if (!perfs[0] || !perfs[1])
	exitSnake(0);

    /* keep ghostbuster away */
    perfs[0]->p_ghostbuster = 0;
    perfs[1]->p_ghostbuster = 0;

    if (s_clock == 5)
	startsnake();
    else if (s_clock > 5) {
	check_explode();
	if (!explode) {
	    movesnake();
	}
	else if ((perfs[0]->p_ntorp == 0 && perfs[1]->p_ntorp == 0 &&
	    perfs[0]->p_nplasmatorp == 0 && perfs[1]->p_nplasmatorp == 0) ||
	    /* xx -- sometimes above doesn't work? */
		 s_clock - explode > 56)
	    exitSnake(0);
    }
    return 1;
}
