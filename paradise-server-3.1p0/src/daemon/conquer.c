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

#if 0	/* these are now configvals */
/*----------------------------NUMBER DEFINES-------------------------------*/
#define SURRSTART  4		/* # planets where surrender starts */
#define SURREND	  7		/* # planets where surrender stops */
#define SURRLENGTH 25		/* give them 30 minutes to sustain empire */
/*-------------------------------------------------------------------------*/
#endif

#define SURRSTART (configvals->surrstart)
#define SURREND   (configvals->surrend)
#define SURRLENGTH (configvals->surrlength)

/*----------------------------LOCAL DATA-----------------------------------*/
static char *teamVerbage[9] = 
  {"", "has", "have", "", "have", "", "", "", "have"};
/*-------------------------------------------------------------------------*/


/*----------------------------MODULE VARIABLES-----------------------------*/

 /* used to store players found with the displayBest function */
typedef struct playerlist {
    char    name[16];		/* player's name */
    char    mapchars[2];	/* player's map window image */
    int     planets, armies;	/* planets taken and armies bombed */
    int     resources, dooshes;	/* resources bombed and armies dooshed */
}       Players;

/*-------------------------------------------------------------------------*/


static void 
swap(int *a, int *b)
{
    int     t = *a;
    *a = *b;
    *b = t;
}


/*----------------------------INTERNAL FUNCTIONS---------------------------*/



#define OUT pmessage(buf, 0, MALL | MGENO, " "); \
	if (conqfile) fprintf(conqfile, "  %s\n", buf); \
	tlog_conquerline(buf)



/*-------------------------------DISPLAYBEST-------------------------------*/
/*  This function is used to get a list of players on a team after a
genocide.  It goes through the players and selects the ones that have
inflicted the most damage.  They are then printed to the message window
and to the conquer file.  They are arranged according to how much damage
the players have inflicted on the enemy, with one planet taken counting
as thirty armies bombed.  A resource counts as 8 armies bombed and a
army dooshed counts as five armies bombed.  */

/* args:
    FILE   *conqfile;		 the opened conquer file 
    int     team;		 the winning team */
static void
displayBest(FILE *conqfile, int team)
{
    register int i, k, l;	/* looping vars */
    register struct player *j;	/* to point to players */
    int     planets, armies;	/* # planets and armies for player */
    int     resources, dooshes;	/* resources bombed, armies dooshed */
    Players winners[MAXPLAYER + 1];	/* to hold player's stats */
    char    buf[100];		/* to sprintf into */
    int     number;		/* to hold # players found */

    number = 0;			/* number of players found */
    for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
	if ((j->p_team != team) || (j->p_status == PFREE))	/* player wrong race */
	    continue;		/* or not here, then forget him */
	planets = j->p_planets;	/* get all planets and armies for */
	armies = j->p_armsbomb;	/* entire game */
	resources = j->p_resbomb;	/* get resources and armies dooshed */
	dooshes = j->p_dooshes;
	for (k = 0; k < number; k++) {	/* go find postion in current list */
	    if (30 * winners[k].planets + winners[k].armies + 8 * winners[k].resources
		+ 5 * winners[k].dooshes <
		30 * planets + armies + 8 * resources + 5 * dooshes) {
		break;		/* break when position found */
	    }
	}
	for (l = number; l >= k; l--)	/* move other players to make room */
	    winners[l + 1] = winners[l];	/* for new player */
	number++;		/* we have found one more player */
	winners[k].planets = planets;	/* record his planets and armies */
	winners[k].armies = armies;
	winners[k].resources = resources;
	winners[k].dooshes = dooshes;
	strncpy(winners[k].mapchars, twoletters(j), 2);
	strncpy(winners[k].name, j->p_name, 16);	/* get his name */
	winners[k].name[15] = 0;/* `Just in case' paranoia */
    }
    for (k = 0; k < number; k++) {	/* go through all players found */
	if (winners[k].planets != 0 || winners[k].armies != 0) {	/* damage done? */
	    sprintf(buf, " %16s (%2.2s)  %d planets %d armies %d resources %d dooshes",
		    winners[k].name, winners[k].mapchars, winners[k].planets,
	       winners[k].armies, winners[k].resources, winners[k].dooshes);
	    OUT;
	}
    }
}

/*-------------------------------------------------------------------------*/






/*-----------------------------------GENOCIDE------------------------------*/
/*  This function checks for a genocide.  It also returns the two teams
with the most players.  It first goes through the four teams and finds the
two teams with the greatest number of players.  These two teams are returned
in the parameters team1 and team2.  If one of the teams does not have at
least one planet then a 1 is returned.  Otherwise a zero is returned.  */

static int 
genocide(int *team1, int *team2)	/* where to put first/second team */
{
    int     t1, t2;		/* to hold two teams */
    int     n1, n2;		/* to hold number of ships on teams */
    int     t;			/* temp var */
#ifdef OLD_GENO
    int     i;			/* loop var */
#endif

    t1 = FED;			/* set first team */
    n1 = realNumShips(FED);	/* get players on it */
    t2 = ROM;			/* set second team */
    n2 = realNumShips(ROM);	/* get players on it */
    if (n1 < n2) {		/* place team with least players in t2 */
	swap(&n1, &n2);
	swap(&t1, &t2);
    }
    if (realNumShips(KLI) > n2) {	/* check the klingons */
	t2 = KLI;
	n2 = realNumShips(KLI);
    }
    if (n1 < n2) {		/* place team with least players in t2 */
	swap(&n1, &n2);
	swap(&t1, &t2);
    }
    if (realNumShips(ORI) > n2) {	/* check the orions */
	t2 = ORI;
	n2 = realNumShips(ORI);
    }
    /* no longer necessarily in order */
    *team1 = t1;		/* pass back team 1 */
    *team2 = t2;		/* pass back team 2 */
    t = 0;			/* no genocide detected yet */
#ifndef OLD_GENO
    if(teams[t1].s_plcount <= configvals->victory_planets || 
       teams[t2].s_plcount <= configvals->victory_planets)
      return(1);
    return(0);
#else
    for (i = 0; i < NUMPLANETS; i++) {	/* see if team 1 has a planet */
	if (planets[i].pl_owner == t1) {	/* do they own this */
	    t++;		/* inc t andd then get out of loop */
	    break;
	}
    }
    for (i = 0; i < NUMPLANETS; i++) {	/* see if team 2 has a planet */
	if (planets[i].pl_owner == t2) {	/* do they own this */
	    t++;		/* inc t andd then get out of loop */
	    break;
	}
    }
    if (t != 2)			/* if both teams do not have planets then */
	return (1);		/* return that genocide has occured */
    else			/* else genocide has not occured */
	return (0);
#endif
}




/*------------------------------CHECKSURRENDER-----------------------------*/
/*  This function is called when a teams surrender status may have changed.
It takes a team and uses the s_plcount in the teams structure to check
whether a team has prevented surrender or the surrender process should
begin.  */

static void 
checksurrender(int team, int otherplanets)	/* team - team to check */
{
    char    buf[80];		/* to sprintf into */

    if ((teams[team].s_surrender > 0) &&	/* surrender timer running? */
	((teams[team].s_plcount >= SURREND) ||
	 (teams[team].s_plcount * 2 > otherplanets))) {	/* and enough planets */
	pmessage("", 0, MALL, "");
	sprintf(buf, "The %s %s prevented collapse of the empire.",
		teams[team].name, teamVerbage[team]);
	pmessage(buf, 0, MALL, MSERVA);	/* message to all */
	pmessage("", 0, MALL, "");
	teams[team].s_surrender = 0;	/* stop surrender clock */
    }
    else if ((teams[team].s_surrender == 0) &&	/* start surrender  ? */
	     (teams[team].s_plcount <= SURRSTART) &&
	     (teams[team].s_plcount * 2 <= otherplanets) &&
	     (realNumShips(team) >= configvals->tournplayers) &&
	     (teams[team].s_plcount < teams[team].s_plcountold)) {
	teams[team].s_surrender = SURRLENGTH;
	sprintf(buf, "The %s %s %d minutes before the empire collapses.",
		teams[team].name, teamVerbage[team], SURRLENGTH);
	pmessage("", 0, MALL, "");
	pmessage(buf, 0, MALL, MSERVA);
	sprintf(buf, "%d planets are needed to sustain the empire.",
		SURREND);
	pmessage(buf, 0, MALL, MSERVA);
	pmessage("", 0, MALL, "");
    }
}

/*-------------------------------------------------------------------------*/







/*------------------------------VISIBLE PROCEDURES-------------------------*/

/*---------------------------------UDSURRENDER-----------------------------*/
/*  This function updates the surender stuff.  There is also stuff for
updating the ship construction timers here.  */

void
udsurrend(void)
{
    register int i, t;		/* looping vars */
    char    buf[80];
    struct planet *l;
    struct player *j;

    /* TC's termination of players not on a t-mode team */
    for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
	if ((j->p_status == PALIVE) &&
	    !(j->p_flags & PFROBOT) && (j->p_team != NOBODY) &&
	    (realNumShips(j->p_team) < configvals->tournplayers) &&
	    (lrand48() % 5 == 0))
	    rescue(TERMINATOR, j->p_no, -1);
    }
    for (t = 0; t <= MAXTEAM; t++) {	/* go through all teams */
	if ((teams[t].s_surrender == 0) ||
	    (realNumShips(t) < configvals->tournplayers))
	    continue;		/* suspend if no t-mode */
	if (teams[t].s_plcount > SURRSTART)	/* suspend timer if they have */
	    continue;		/* enough planets */
	teams[t].s_surrender--;	/* dec the surrender timer */
	if (teams[t].s_surrender <= 0) {	/* if timer ran out then */
	    teams[t].s_surrender = -1; /* set it so it isnt 0 (otherwise
                                          it announces surrender again) (BG) */
	    sprintf(buf, "The %s %s surrendered.", teams[t].name,
		    teamVerbage[t]);	/* print surrender message */
	    pmessage("", 0, MALL, "");
	    pmessage(buf, 0, MALL, MSERVA);
	    pmessage("", 0, MALL, "");
	    for (i = 0, l = &planets[i]; i < NUMPLANETS; i++, l++) {
		if (l->pl_owner == t) {	/* neutralize and zero the */
		    l->pl_owner = NOBODY;	/* armies on all loser's
						   planets */
		    l->pl_armies = 0;
		}
	    }
	    checkwin(enemy_admiral(t));	/* go check the win */
	}
	else if ((teams[t].s_surrender % 5) == 0 ||
		 (teams[t].s_surrender < 4)) {
	    sprintf(buf, "The %s %s %d minutes remaining.",
		    teams[t].name, teamVerbage[t], teams[t].s_surrender);
	    pmessage("", 0, MALL, "");	/* blank line */
	    pmessage(buf, 0, MALL, MSERVA);	/* printf counting down */
	    sprintf(buf, "%d planets will sustain the empire.  ", SURREND);
	    pmessage(buf, 0, MALL, MSERVA);
	    sprintf(buf, "%d or half enemy suspends countdown.", SURRSTART + 1);
	    pmessage(buf, 0, MALL, MSERVA);
	    pmessage("", 0, MALL, "");	/* blank line */
	}
    }				/* end for team */
}


/*------------------------------CONQUERMESSAGE-----------------------------*/
/*  This function is called after a race is genocided.  It lists the players
on the winning and losing teams.  The reason should be either GENOCIDE or
SURRENDER.  The list is written to the conquer file.  */

static void
conquerMessage(int winners, int losers, int pno)
 /* the losing and winning teams */
 /* the person who won it */
{
    char    buf[80];		/* to sprintf into */
    FILE   *conqfile;		/* to open conquer file */
    time_t  curtime;		/* to get current time */
    char   *paths;		/* to hold path to directory */

    paths = build_path(CONQFILE);
    conqfile = fopen(paths, "a");	/* open the conquer file */
    if (!conqfile) {
	perror("");
	fprintf(stderr, "failed to open file %s\n", paths);
    }

    time(&curtime);		/* get current time */
    strcpy(buf, "\nConquer! ");	/* this is a genocide */
    strcat(buf, ctime(&curtime));	/* cat on current time */
    if (conqfile)
	fprintf(conqfile, "  %s\n", buf);	/* write it to conquer file */
    pmessage("***********************************************************",
	     0, MALL | MGENO, " ");	/* print enclosure to messages */

    if (pno >= 0) {
	sprintf(buf, "The galaxy has been conquered by %s and the %s!!!!",
		players[pno].p_name, teams[winners].name);
    }
    else {
	sprintf(buf, "Stalemate.");
    }
    OUT;

    sprintf(buf,
	    status2->league ? "The %s(%s):" :
	    "The %s:",
	    teams[winners].name
	    , ((winners == (1 << status2->home.index)) ?
	     (&status2->home) :
	     (&status2->away))->name
	    );
    OUT;
    displayBest(conqfile, winners);	/* go display team 1 */
    sprintf(buf,
	    status2->league ? "The %s(%s):" :
	    "The %s:",
	    teams[losers].name
	    ,((losers == (1 << status2->home.index)) ?
	     (&status2->home) :
	     (&status2->away))->name
	    );
    OUT;
    displayBest(conqfile, losers);	/* go display team 2 */
    pmessage("***********************************************************",
	     0, MALL | MGENO, " ");	/* printf the enclosure */
    if (conqfile) {
	fprintf(conqfile, "\n");/* space between conquering */
	fclose(conqfile);	/* close conquer file */
    }
}


static void 
refresh_team_planetcounts(void)
{
    int     i;
    register struct planet *l;	/* to point to planets */

    for (i = 0; i <= MAXTEAM; i++) {	/* zero out each teams planet count */
	teams[i].s_plcountold = teams[i].s_plcount;	/* save last planet
							   count */
	teams[i].s_plcount = 0;
    }
    for (i = 0, l = &planets[i]; i < NUMPLANETS; i++, l++)	/* recompute */
	teams[l->pl_owner].s_plcount++;	/* planet counts for teams */
}

/* blow everyone up, print the conquer assessments, figure out who won. */
void 
endgame_effects(int t1, int t2, int pno)
{
    int     i;
    struct player *j;

    refresh_team_planetcounts();

    if (teams[t1].s_plcount < teams[t2].s_plcount) {
	int     i;
	i = t1;
	t1 = t2;
	t2 = i;			/* t1 is the winners */
    }
    if (pno >= 0 && players[pno].p_team != t1)
	pno = -1;		/* he can't win the game! */

    if (pno < 0)
	pno = enemy_admiral(t2);

    if (teams[t1].s_plcount == teams[t2].s_plcount) {
	/* tie, nobody wins */
	pno = -1;
    }
    status->genocides++;	/* inc number of genocides */
    conquerMessage(t1, t2, pno);/* print conquer message */

    for (i = 0, j = &players[0]; i < MAXPLAYER; i++, j++) {
	if (j->p_status == PFREE)	/* if player not in game */
	    continue;		/* ignore him */
	if (status->tourn)	/* only inc genos if t-mode */
	    j->p_stats.st_genocides++;	/* inc player's genocides */
	j->p_whydead = KWINNER;	/* because someone won */
	j->p_whodead = pno;	/* set who won the game */
	j->p_status = POUTFIT;	/* send them immediately to outfit screen */
/*
    cause_kaboom (j);
*/
	j->p_ntorp = 0;		/* no torps shot */
	j->p_nplasmatorp = 0;	/* no plasmas shot */
    }
}



/*-------------------------------CHECKWIN---------------------------------*/
/*  This function is called when a planet is taken or beamed to zero.
It recalculates s_plcount for each team.  It then finds the two teams that
are in t-mode.  If either of them has just sustained the empire from
collapse or should start the surrender procedure then that is done.  If
a team has been genocided then the genocide message is printed and the
game is reset. */

void
checkwin(int pno)
{
    int     g;			/* to hold whether genocide occured */
    int     t1, t2;		/* to hold the t-mode teams */

    refresh_team_planetcounts();
    g = genocide(&t1, &t2);	/* check genocide */
    if (teams[t1].s_plcount < teams[t2].s_plcount) {
	int     i;
	i = t1;
	t1 = t2;
	t2 = i;			/* t1 is the winners */
    }
    checksurrender(t1, teams[t2].s_plcount);	/* check for change in */
    checksurrender(t2, teams[t1].s_plcount);	/* surrender status */
    if (!g)			/* if genocide did not occur */
	return;			/* then return from func */
    teams[t1].s_surrender = 0;	/* stop the surrender clock */
    teams[t2].s_surrender = 0;

    stoptimer();

    if (status2->league) {
	endtourn();		/* calls endgame_effects */
    }
    else
	endgame_effects(t1, t2, pno);
    status->gameup = 0;		/* exit the daemon */
    status->count = 0;
    save_planets();
    sleep(2);
    blast_shmem();
    exit(0);
}


/*-----------END OF FILE-----*/
