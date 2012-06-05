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

/*------------------------------NUMBER DEFINES-----------------------------*/
#define INDEP (GWIDTH/3)	/* width of region in the center of galaxy */
				/* that independents can join in */

#define MIN_DIST_FROM_STAR 3000.0 /* min distance from a star that enter() */
				/* will place you */
/*-------------------------------------------------------------------------*/


static int 
find_start_planet(int team, int flag)
{
    int     valid[MAXPLANETS];
    int     nvalid;
    int     i;

    nvalid = 0;
    for (i = 0; i < NUMPLANETS; i++) {
	struct planet *pl = &planets[i];
	if (pl->pl_owner == team && (!flag || pl->pl_flags & flag))
	    valid[nvalid++] = i;
    }
    if (nvalid < 1)
	return -1;

    return valid[lrand48() % nvalid];
}

/*------------------------------VISIBLE FUNCTIONS-------------------------*/

#define BOSSMAN		NUMROYALRANKS-1
#define SIDEKICK	NUMROYALRANKS-2
#define AMBASSADOR	NUMROYALRANKS-3

#ifdef UFL
#undef SIDEKICK
#define SIDEKICK	NUMROYALRANKS-1
#undef AMBASSADOR
#define AMBASSADOR	NUMROYALRANKS-1
#endif

/*--------------------------------PEANUT_GALLERY--------------------------*/
/*  This function checks too see if certain "important" people have logged
in, and displays a special message to the rest of the players.  As the chief
Paradise hack, I forbid removal of any of the code in this function.  Other
names may be added.  If you choose to disregard this, a malefiction will be
leveled upon your head and all of your progeny will be born with their noses
two times too large.  */

#define AT_LEAST(rank) if (me->p_stats.st_royal<rank) me->p_stats.st_royal = rank;


static void 
peanut_gallery(void)
{
    char    buf[90];

    if (strcmp(me->p_name, "a fungusamongus") == 0) {	/* that's me! */
	pmessage2("  ", 0, MALL, "LDDENYS > ", me->p_no);
	pmessage2("Yikes.  EEK!  Help.  There seems to be a fungus among us!", 0, MALL, "LDDENYS > ", me->p_no);
	pmessage2("  ", 0, MALL, "LDDENYS > ", me->p_no);

    }
    else if (strcmp(me->p_name, "Lynx") == 0) {
	pmessage2("\"GAME OVER MAN, GAME OVER!\"", 0, MALL, "Lynx->ALL ", me->p_no);

    }
    else if (strcmp(me->p_name, "Hammor") == 0) {
	pmessage2("Please don't hurt 'em, Hammor!", 0, MALL, "GOD->ALL", me->p_no);

    }
    else if (strcmp(me->p_name, "Bubbles") == 0) {
	pmessage2("Whoa!", 0, MALL, "KAOLSEN > ", me->p_no);
	pmessage2("Dudes.", 0, MALL, "KAOLSEN > ", me->p_no);
	pmessage2("Cool.", 0, MALL, "KAOLSEN > ", me->p_no);

    }
    else if (strcmp(me->p_name, "KnightRaven") == 0) {
	pmessage2("Insert Heiji quote here", 0, MALL, "KAOLSEN > ", me->p_no);
	AT_LEAST(AMBASSADOR);

    }
    else if (strcmp(me->p_name, "wibble") == 0) {
	pmessage2("No mountain is unclimbable, no river uncrossable, no client RSA"
		 ,0, MALL, "EGO->wibble", me->p_no);
	pmessage2("key unbreakable.  We can just make it bloody difficult!",
		 0, MALL, "EGO->wibble", me->p_no);

    }
    else if (strcmp(me->p_name, "Key") == 0) {
	time_t  curtime;
	struct tm *tmstruct;
	int     hour;

	(void) time(&curtime);
	tmstruct = localtime(&curtime);
	if (!(hour = tmstruct->tm_hour % 12))
	    hour = 12;

	sprintf(buf, "It's %d:%02d%s, time [for me] to die.", hour,
		tmstruct->tm_min, tmstruct->tm_hour >= 12 ? "pm" : "am");
	pmessage2(buf, 0, MALL, "GOD->ALL", me->p_no);

    }
    else if (strcmp(me->p_name, "MikeL") == 0) {
	pmessage2("<This space for rent>", 0, MALL, "GOD->ALL", me->p_no);


    }
    else if (strcmp(me->p_name, "Bolo") == 0) {
	pmessage2("Bolo Mk. MCLXVII On-line.", 0, MALL, MSERVA, me->p_no);
    }
}

static void 
advertise_tourney_queue(void)
{
    char    buf[80], addrbuf[10];
    int     count = 0;
    int     i;

    if (status->tourn)
	return;

    for (i = 0; i < MAXPLAYER; i++) {
	if (players[i].p_status == PTQUEUE)
	    count++;
    }

    sprintf(addrbuf, "%s->YOU ", SERVNAME);
    sprintf(buf, "There %s %d player%s on the tournament queue",
	    (count == 1) ? "is" : "are", count, (count == 1) ? "" : "s");
    pmessage (buf, me->p_no, MINDIV, addrbuf);
}

/*--------------------------------AUTOPEACE------------------------------*/
/*  This function set the player as hostile to all teams with at least one
player on them if it is t-mode.  Otherwise if it is not t-mode the player
is set as hositle to everyone.  */

static void 
auto_peace(void)
{
    int     i, num[MAXTEAM + 1];/* to hold team's player counts */
    struct player *p;		/* looping var */

    num[0] = num[FED] = num[ROM] = num[KLI] = num[ORI] = 0;	/* zero counts */
    for (i = 0, p = players; i < MAXPLAYER; i++, p++)	/* loop all players */
	if (p->p_status != PFREE)	/* ince the count of the team the */
	    num[p->p_team]++;	/* player is on */
    if (status->tourn)		/* if t-mode then make player hostile */
	me->p_hostile =		/* make all teams with a player on */
	    ((FED * (num[FED] >= configvals->tournplayers)) |
	     (ROM * (num[ROM] >= configvals->tournplayers)) |
	     (KLI * (num[KLI] >= configvals->tournplayers)) |
	     (ORI * (num[ORI] >= configvals->tournplayers)));
    else			/* else if not t-mode then */
	me->p_hostile = FED | ROM | KLI | ORI;	/* hostile to everyone */
}

/*------------------------------PLACEINDEPENDENT---------------------------*/
/*  This function places an independent player in the game so he is not
near any other players.  */

static void 
placeIndependent(void)
{
    int     i;			/* ye olde looping var */
    struct player *p;		/* to point to players */
    int     good, failures;	/* flag for success, count of tries */

    failures = 0;		/* have started loops yet */
    while (failures < 10) {	/* only try 10 times */
	me->p_x = GWIDTH / 2 + (lrand48() % INDEP) - INDEP / 2;	/* middle 9th of */
	me->p_y = GWIDTH / 2 + (lrand48() % INDEP) - INDEP / 2;	/* galaxy */
	good = 1;
	for (i = 0, p = players; i < MAXPLAYER; i++, p++) {
	    if ((p->p_status != PFREE) && (p != me)) {
		if ((ABS(p->p_x - me->p_x) < 2 * TRACTDIST) &&
		    (ABS(p->p_y - me->p_y) < 2 * TRACTDIST)) {
		    failures++;	/* found a player too close */
		    good = 0;	/* position not good */
		    break;	/* try another positon */
		}
	    }
	}
	if (good)		/* if good placement found then */
	    return;		/* return */
    }
    fprintf(stderr, "Couldn't place the bot successfully.\n");
}

static int
findtestslot(void)
{
    register int i;

    for (i = MAXPLAYER - configvals->ntesters; i < MAXPLAYER; i++) {
        if (players[i].p_status == PFREE) {     /* We have a free slot */
            players[i].p_status = POUTFIT;      /* possible race code */
            break;
        }
    }
    if (i == MAXPLAYER) {
        return -1;              /* no room in tester slots */
    }
    memset(&players[i].p_stats, 0, sizeof(struct stats));
    players[i].p_stats.st_tticks = 1;
    return (i);
}

int
findrslot(void)
{
    register int i;

    /* look for tester slot first */
    i = findtestslot();
    if (i > -1)
        return i;

    for (i = 0; i < MAXPLAYER; i++) {
        if (players[i].p_status == PFREE) {     /* We have a free slot */
            players[i].p_status = POUTFIT;      /* possible race code */
            break;
        }
    }
    if ((i == MAXPLAYER) || (i == -1)) {
        return -1;
    }
    memset(&players[i].p_stats, 0, sizeof(struct stats));
    players[i].p_stats.st_tticks = 1;
    return (i);
}

/*------------------------------------ENTER-------------------------------*/
/*  This function places a player into the game.  It initializes fields in
the player structure and his ship.  It determines where in the galaxy to
put the player.  This function is also used to place robots and independents.
If tno = 4, then the player is independent.  If it is 5 then the player is
a robot.  */

/* CRD feature: starting planet (for robots) - MAK,  2-Jun-93
 * Also added starting planet number as a useful return value.
 */

/* args:
   tno - team player is on
   disp - "not used, so I used it 7/27/91 TC"
   pno - player's number
   s_type - player's ship type
   startplanet - planet to enter near (or -1) */

int 
enter(int tno, int disp, int pno, int s_type, int startplanet)
{
    static int lastteam = -1;	/* to hold team of last enter */
    static int lastrank = -1;	/* to hold rank of last enter */
    char    buf[80];		/* to sprintf into */
    char    buf2[80];		/* to sprintf into */
    char    addrbuf[10];	/* to hold address */

    if ((startplanet < 0) || (startplanet >= NUMPLANETS))
	startplanet = -1;

    (void) strncpy(me->p_name, pseudo, sizeof(me->p_name));	/* get name */
    me->p_name[sizeof(me->p_name) - 1] = '\0';	/* delimiet just in case */

    me->p_kills = 0.0;		/* have no kills yet */
    get_ship_for_player(me, s_type);	/* get the player's ship type */

    if (me->p_ship.s_nflags & SFNHASFIGHTERS)
	me->p_ship.s_missilestored = 0;

    me->p_docked = 0;		/* not docked to anyone */
    me->p_updates = 0;		/* start updating immediately */
    me->p_flags = PFSHIELD;
    if (allows_docking(me->p_ship))
	me->p_flags |= PFDOCKOK;/* enable docking */
    me->p_dir = 0;		/* direction angle of zero */
    me->p_desdir = 0;		/* desired direction of zero */
    me->p_speed = 0;		/* speed of zero */
    me->p_desspeed = 0;		/* desired speed of zero */
    me->p_subspeed = 0;		/* fractional part of speed zero */

/*  Gunboat docked to ships stuff */
    if ((me->p_ship.s_nflags & SFNMASSPRODUCED) &&
	(shipvals[shipPick].s_numports)) {
	int     i;

	me->p_team = (1 << tno);
	for (i = 0; i < MAXPLAYER; i++)
	    if ((players[i].p_ship.s_type == shipPick) &&
		(players[i].p_team == me->p_team) &&
		(players[i].p_status == PALIVE) &&	/* if all this stuff... */
		(players[i].p_flags & PFDOCKOK) &&
		(players[i].p_docked < players[i].p_ship.s_numports) &&
		(players[i].p_ship.s_missilestored)) {
		me->p_x = players[i].p_x;	/* ...dock em on */
		me->p_y = players[i].p_y;
		newdock(i);
		if (players[i].p_ship.s_missilestored != -1)
		   players[i].p_ship.s_missilestored--;
		if (players[i].p_flags & PFCLOAK) {
		   me->p_flags |= PFCLOAK;
		   me->p_cloakphase = players[i].p_cloakphase;
		} else me->p_cloakphase = 0;
		break;
	    }
    }
/* End Gunboat stuff */

    if ((tno == 4) || (tno == 5)) {	/* if player indep or a robot */
	me->p_team = 0;		/* he has no team */
    }
    else {			/* else player is normal player--find planet */
	me->p_team = (1 << tno);/* set players team number */

	/* forgive me father, for I have used short-circuiting to
	   emulate control flow */
	(startplanet >= 0	/* start planet already specified? */
	 || (((startplanet = me->p_lastrefit) >= 0	/* we've got a home
							   planet */
	      && (planets[startplanet].pl_flags & PLSHIPYARD)	/* and it's a yard */
	      &&(planets[startplanet].pl_owner == me->p_team))	/* and it's ours */
	     ||0 < (me->p_lastrefit = -1))	/* oops, no more home
						   shipyard, F */
	 ||(startplanet = find_start_planet(me->p_team, PLSHIPYARD)) != -1
	 || (startplanet = find_start_planet(me->p_team, PLREPAIR)) != -1
	 || (startplanet = find_start_planet(me->p_team, PLAGRI)) != -1
	 || (startplanet = find_start_planet(me->p_team, PLFUEL)) != -1
	 || (startplanet = find_start_planet(me->p_team, PLHOME)) != -1
	 || (startplanet = find_start_planet(me->p_team, 0)) != -1
	    );
    }
    if (startplanet == -1) {
	placeIndependent();
    }
    else {
	int i;
	double dx, dy;
	struct planet *l;

	/* we have a planet */
	/* use p_x and y as scratch registers */
	while(1) {
	    me->p_x = planets[startplanet].pl_x + (lrand48() % 10000) - 5000;
	    me->p_y = planets[startplanet].pl_y + (lrand48() % 10000) - 5000;
	    if (me->p_x < 0)	/* can't come in outside of borders */
		me->p_x = 0;	/* now can we?  */
	    if (me->p_y < 0)
		me->p_y = 0;
	    if (me->p_x >= GWIDTH)
		me->p_x = GWIDTH-1;
	    if (me->p_y >= GWIDTH)
		me->p_y = GWIDTH-1;
	    for(i = 0, l = &planets[0]; i < NUMPLANETS; i++, l++) {
		if(PL_TYPE(*l) == PLSTAR) {
		    dx = ABS(l->pl_x - me->p_x);
		    dy = ABS(l->pl_y - me->p_y);
		    if(dx * dx + dy * dy <
				MIN_DIST_FROM_STAR * MIN_DIST_FROM_STAR)
			break;
		}
	    }
	    if(i == NUMPLANETS)
		break;
	}
	/* legitimize p_x and p_y */
	move_player(me->p_no, me->p_x, me->p_y, 0);
    }
    me->p_ntorp = 0;		/* have not fired torps yete */
    if (!((me->p_ship.s_nflags & SFNMASSPRODUCED) &&
	(shipvals[shipPick].s_numports)))
       me->p_cloakphase = 0;	/* no cloaking phase--not cloaked */
    me->p_nplasmatorp = 0;	/* no plasma torps */
    me->p_swar = 0;		/* at war with nobody */
    me->p_armies = 0;		/* carrying no armies */
    tmpPick = 0;

    switch_special_weapon();

    if (!keeppeace)		/* if keep peace mode then */
	auto_peace();		/* set do automatic peace */
    me->p_hostile &= ~me->p_team;	/* hostile to all but own team */
    me->p_swar &= ~me->p_team;
    if ((lastteam != tno) || (lastrank != mystats->st_rank)) {
	if ((lastteam > 0) && (lastteam < NUMTEAM) && (lastteam != tno))
	    declare_war((1 << lastteam) | me->p_hostile);	/* if changing then
								   adjust war stat */
	lastteam = tno;		/* store team number in static */
	lastrank = mystats->st_rank;	/* store rank in static */
	sprintf(addrbuf, " %s->ALL", twoletters(me));
	if ((tno == 4) && (strcmp(me->p_monitor, "Nowhere") == 0)) {
	    time_t  curtime;	/* if a robot and independent */
	    struct tm *tmstruct;/* to hold time */
	    int     queuesize;	/* to hold queue size */
	    int     hour;	/* to hold hour */

	    time(&curtime);	/* get the time */
	    tmstruct = localtime(&curtime);	/* convert to local time */
	    if (!(hour = tmstruct->tm_hour % 12))	/* get the hour */
		hour = 12;
	    sprintf(buf, "It's %d:%02d%s, time to die.", hour, tmstruct->tm_min,
		    tmstruct->tm_hour >= 12 ? "pm" : "am");
	    if ((queuesize = status->count - status->wait) > 0) {
		/* lint: queuesize set but not used in function enter */
		sprintf(buf2, "  Approximate queue size: %d.", status->answer);
		strcat(buf, buf2);	/* get queue size if queue */
	    }
	    pmessage2(buf, 0, MALL, addrbuf, me->p_no);	/* display message */
	}
	else if (tno == 5) {	/* if a robot then */
	    if (players[disp].p_status != PFREE) {
		sprintf(buf2, "%s has been targeted for termination.",
			twoletters(&players[disp]));
		pmessage2(buf2, 0, MALL, addrbuf, me->p_no);
	    }
	}

	if (!status2->league)	/* no peanut messages in a league game */
	    peanut_gallery();	/* check for important people */

	if (me->p_stats.st_royal == 0)
	    sprintf(buf, "%s %.16s is now %2.2s (%.16s@%.32s)",
		    ranks[me->p_stats.st_rank].name, me->p_name,
		    twoletters(me), me->p_login, me->p_full_hostname);
	else
	    sprintf(buf, "%s %.16s is now %2.2s (%.16s@%.32s)",
		    royal[me->p_stats.st_royal].name, me->p_name,
		    twoletters(me), me->p_login, me->p_full_hostname);

	pmessage2(buf, 0, MALL | MJOIN, addrbuf, me->p_no);

	advertise_tourney_queue();

    }
    delay = 0;
    return startplanet;
}

/*----------END OF FILE--------*/
