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

#define TLOGNAME "/tmp/tourney.log"
#define MAXTIME 120

#define TOURNEYMODE	(status2->league > 2)

/*---------------------------LOCAL VARIABLES------------------------------*/

static FILE *tlog = NULL;

static enum player_status_e status_cache[MAXPLAYER];
static enum ship_types_e ship_cache[MAXPLAYER];

/*---------------------------LOCAL FUNCTIONS------------------------------*/

static void 
opentlog(void)
{
    tlog = fopen(TLOGNAME, "a");
    if (tlog == NULL)
	fprintf(stderr, "Could not open the tournament logfile");
    else {
#ifdef SETVBUF_REVERSED
	setvbuf(tlog, _IOLBF, NULL, BUFSIZ);
#else
	setvbuf(tlog, NULL, _IOLBF, BUFSIZ);
#endif
	fprintf(tlog, "\n\n\n\nOPENING TOURNAMENT LOG\n\n");
    }
}

/* Return a string identifying the player and his ship.
   Uses a ring of buffers so that it can be used multiple times
   in a printf */
static char *
id_player(struct player *p)
{
    static char bufs[16][80];
    static int ring = 0;
    int     count;

    ring++;
    count = sizeof(bufs) / sizeof(*bufs);
    if (ring >= count)
	ring = 0;

    sprintf(bufs[ring], "%s <%s> %c%c", twoletters(p),
	    p->p_name, p->p_ship.s_desig1, p->p_ship.s_desig2);

    return bufs[ring];
}

/* Return a string identifying the player and his ship.
   Uses a ring of buffers so that it can be used multiple times
   in a printf */
static char *
id_planet(struct planet *p)
{
    static char bufs[16][80];
    static int ring = 0;
    int     count;

    ring++;
    count = sizeof(bufs) / sizeof(*bufs);
    if (ring >= count)
	ring = 0;

    sprintf(bufs[ring], "#%d [%s] %s", p->pl_no, p->pl_name,
	    teams[p->pl_owner].shortname);

    return bufs[ring];
}


/*

   List of player related actions

   */

static void 
tlog_refit(struct player *pl)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    /* we have to print the OLD ship type he was flying */
    fprintf(tlog, "%ld\trefit %s <%s> %c%c %dt\n", status->clock,
	    twoletters(pl), pl->p_name,
	    shipvals[ship_cache[pl->p_no]].s_desig1,
	    shipvals[ship_cache[pl->p_no]].s_desig2,
	    pl->p_updates);

    ship_cache[pl->p_no] = pl->p_ship.s_type;
}

/* player was killed by player.
   call before erasing armies */
void 
tlog_plkill(struct player *victim, struct player *killer1, 
            struct player *killer2)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tkill %s+%da,%dt,%.2fk by %s&%s\n",
	    status->clock,
	    id_player(victim), victim->p_armies, victim->p_updates,
	    victim->p_kills,
    killer1 ? id_player(killer1) : "_", killer2 ? id_player(killer2) : "_");

    status_cache[victim->p_no] = victim->p_status;
}

/* player changed status from ALIVE to
   something else without telling us.
   call before erasing armies */
static void 
tlog_plquit(struct player *victim)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tquit %s+%da,%dt,%.2fk\n",
	    status->clock,
	    id_player(victim), victim->p_armies, victim->p_updates,
	    victim->p_kills);

    status_cache[victim->p_no] = victim->p_status;
}

/* player was killed by planet.
   call before erasing armies */
void 
tlog_plankill(struct player *victim, struct planet *killer1, 
              struct player *killer2)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tkill %s+%da by %s&%s\n",
	    status->clock, id_player(victim), victim->p_armies,
	    id_planet(killer1), killer2 ? id_player(killer2) : "_");
    status_cache[victim->p_no] = victim->p_status;
}

/* planet was destroyed
   call before changing owner to IND */
void 
tlog_plandest(struct planet *pl, struct player *killer)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tpl_destroy %s by %s\n", status->clock, id_planet(pl),
	    id_player(killer));
}

/* planet was taken
   call after changing owner */
void 
tlog_plantake(struct planet *pl, struct player *killer)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tpl_take %s by %s\n", status->clock, id_planet(pl),
	    id_player(killer));
}

/* planet was abandoned
   call before changing owner */
void 
tlog_planaban(struct planet *pl, struct player *killer)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tpl_aban %s by %s\n", status->clock, id_planet(pl),
	    id_player(killer));
}

/* call before transferring the army */
void 
tlog_beamup(struct planet *pl, struct player *carrier)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tbeamup %s from %s@%da\n", status->clock,
	    id_player(carrier), id_planet(pl), pl->pl_armies);
}

/* call after transferring the army */
void 
tlog_beamdown(struct planet *pl, struct player *carrier)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tbeamdown %s to %s@%da\n", status->clock,
	    id_player(carrier), id_planet(pl), pl->pl_armies);
}

/* call before transferring the army */
void 
tlog_Bbeamup(struct player *base, struct player *carrier)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\txfer %s from %s\n", status->clock, id_player(carrier),
	    id_player(base));
}

/* call after transferring the army */
void 
tlog_Bbeamdown(struct player *base, struct player *carrier)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\txfer %s to %s\n", status->clock, id_player(carrier),
	    id_player(base));
}

/* call after destroying armies */
void 
tlog_bomb(struct planet *pl, struct player *killer, int narmies)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    if (!narmies)
	return;

    fprintf(tlog, "%ld\tbomb %s by %s (%da)\n", status->clock,
	    id_planet(pl), id_player(killer), narmies);
}

void 
tlog_bres(struct planet *pl, struct player *killer, char *resource)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tresbomb %s (%s) by %s\n", status->clock,
	    id_planet(pl), resource, id_player(killer));
}


/*

   non-player action stuff

   */

/* call before popping */
void 
tlog_pop(struct planet *pl, int narmies)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tpoparmies %s %+da\n", status->clock,
	    id_planet(pl), narmies);
}

void 
tlog_res(struct planet *pl, char *resource)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\tresgrow %s (%s)\n", status->clock,
	    id_planet(pl), resource);
}

/* call before changing owner */
void 
tlog_revolt(struct planet *pl)
{
    if (!TOURNEYMODE)
	return;
    if (!tlog)
	return;

    fprintf(tlog, "%ld\trevolt %s\n", status->clock, id_planet(pl));
}

void 
scan_for_unexpected_tourny_events(void)
{
    int     i;
    for (i = 0; i < MAXPLAYER; i++) {
	struct player *p = &players[i];
	if (status_cache[i] != PALIVE) {
	    status_cache[i] = p->p_status;
	    ship_cache[i] = -1;
	    continue;
	}

	if (p->p_status != PALIVE) {
	    tlog_plquit(p);
	    status_cache[i] = p->p_status;
	}
	else if (ship_cache[i] < 0) {
	    ship_cache[i] = p->p_ship.s_type;
	}
	else if (ship_cache[i] != p->p_ship.s_type) {
	    tlog_refit(p);
	}
    }
}

/*

   end of T log procs

   */


static void 
closetlog(void)
{
    if (tlog != NULL) {
	fprintf(tlog, "\n\nCLOSING TOURNAMENT LOG\n\n\n\n");
	fclose(tlog);
    }
    tlog = NULL;
}


static void 
tlog_all(void)
{
    int     i;
    struct planet *pl;

    if (!tlog)
	return;

    fprintf(tlog, "The time is: %ld ----------------\n", status->clock);
    for (i = 0, pl = &planets[i]; i < NUMPLANETS; i++, pl++)
	fprintf(tlog, "Planet: %s  %x  %s  -- armies %d\n", pl->pl_name,
		pl->pl_flags, teams[pl->pl_owner].shortname, pl->pl_armies);
}

void 
tlog_conquerline(char *line)
{
    if (!tlog)
	return;

    fprintf(tlog, "C:\t%s\n", line);
}





void 
udtourny(void)
{
    int     trem;
    char    buf[120];
    if (!status2->league)
	return;

    /* server is configured for league play */

    if (status2->league == 1)
	return;			/* we're still prepping */

    trem = --status2->leagueticksleft;


    switch (status2->league) {
    case 2:			/* the 1-minute pre tourney warm up. */
	if (trem == SECONDS(5))
	    pmessage("5 seconds to tournament start", -1, MALL, UMPIRE);
	else if (trem == SECONDS(2))
	    pmessage("Hold on to your hats.  Everybody dies!", -1, MALL, UMPIRE);
	else if (trem <= 0) {
	    starttourn();
	}
	break;

    case 3:			/* full-on T-mode */

	buf[0] = 0;
	if (trem % MINUTES(20) == 0 ||
	    (trem < MINUTES(20) && 0 == trem % MINUTES(5)) ||
	    (trem < MINUTES(5) && 0 == trem % MINUTES(1))) {
	    sprintf(buf, "There are %d minutes remaining in regulation play",
		    trem / MINUTES(1));
	}
	else if (trem < MINUTES(1) && 0 == trem % SECONDS(10)) {
	    sprintf(buf, "There are %d seconds remaining in regulation play",
		    trem / SECONDS(1));
	}
	if (buf[0])
	    pmessage(buf, -1, MALL, UMPIRE);
	if (trem <= 0)
	    /* maybe go into overtime */
	    endtourn();
	break;
    case 4:
	/* overtime, not implemented */
	break;
    }
}



void 
starttourn(void)
{
    int     i, j;
    struct planet *pl;
    char    s[80];

    opentlog();
    for (i = 0, pl = &planets[i]; i < NUMPLANETS; i++, pl++) {
	for (j = 0; j < MAXTEAM + 1; j++) {
	    pl->pl_tinfo[j].timestamp = 0;
	    /* doesn't work? */
	    pl->pl_hinfo = (PL_TYPE(*pl) == PLSTAR) ? ALLTEAM : pl->pl_owner;
	}
    }
    status->clock = 0;

    explode_everyone(KTOURNSTART, 0);

    tlog_all();


    status2->league++;
    status2->leagueticksleft = MINUTES(configvals->regulation_minutes);

    /* version team names configured time date */
    pmessage(" ", 0, MALL, UMPIRE);
    sprintf(s, "Timer started -- %d minutes remaining",
	    status2->leagueticksleft / MINUTES(1));
    pmessage(s, 0, MALL, UMPIRE);
    pmessage(" ", 0, MALL, UMPIRE);
}


void 
endtourn(void)
{
    pmessage(" ", 0, MALL, UMPIRE);
    pmessage("Time is done", 0, MALL, UMPIRE);
    pmessage(" ", 0, MALL, UMPIRE);

    /* print damage assesments */
    endgame_effects(1 << status2->home.index, 1 << status2->away.index, -1);

    scan_for_unexpected_tourny_events();

    tlog_all();

    closetlog();

    /* conquer */

    status2->league = 1;

    /* we should shut the game off here (status->gameup=0) */
}
