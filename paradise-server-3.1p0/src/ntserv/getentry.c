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

#include <sys/types.h>
#include <sys/socket.h>
#include "config.h"
#include "proto.h"
#include "ntserv.h"
#include "data.h"
#include "shmem.h"

extern int overload;		/* 7/31/91 TC let overloaded join any team */

void 
detourneyqueue(void)
{
    me->p_status = POUTFIT;
    me->p_whydead = KPROVIDENCE;
}


/*----------------------------------DEADTEAM-------------------------------*/
/*  This function counts the number of planets that a team owns.  It returns
a 1 if the team has no planets, and a 0 if they have at least one planet. */

static int 
deadTeam(int owner)		/* team to check for */
{
    int     i;			/* looping var */
    struct planet *p;		/* to point to a planets */

    for (i = 0, p = planets; i < NUMPLANETS; i++, p++) {
	if (p->pl_owner & owner)/* if planet owned by team then */
	    return (0);		/* found one planet owned by team */
    }
    return (1);			/* no planets found, team is dead */
}

/*-------------------------------------------------------------------------*/


/*------------------------------TOURNAMENTMASK----------------------------*/
/*  This function takes the players current team and returns a mask of the
teams that the player can join.  */

static int 
leaguemask(int ishome, int idx)
{
    if (status2->league == 1)
	return idx_to_mask(ishome);
    else
	return idx_to_mask(idx);
}

static int
tournamentMask(int team)
{
    int     i;			/* looping var */
    int     team1, team2;	/* for getting two possible teams */
    int     num1, num2;		/* to hold num players on teams */

    if (!blk_flag)		/* not a paradise client */
	return (0);
    if (status->gameup == 0)	/* if game over, can join no team */
	return (0);
    if (overload)		/* if tester slot then can join */
	return (ALLTEAM);	/* any team */
    if (mustexit)		/* should we force player out */
	return (0);
    if (!time_access())		/* if server closed then can join */
	return (0);		/* no team */

    /* league stuff */
    if (status2->league) {
	if (me->p_homeaway == AWAY)
	    return leaguemask(1, status2->away.index);
	else if (me->p_homeaway == HOME)
	    return leaguemask(0, status2->home.index);
	else {
	    warning("You have connected to a league server but aren't on a side.");
	    me->p_homeaway = (lrand48() & 1) + 1;	/* dangerous int->enum
							   cast */
	    return 0;
	}
	/* NOTREACHED */
    }
    if (me->p_homeaway != NEITHER) {
	warning("You have connected to a non-league server with a league ntserv.");
	me->p_homeaway = NEITHER;
    }

    if (!status->tourn) {
	return status2->nontteamlock;
    }

    if (team != ALLTEAM && deadTeam(team))	/* if former team dead */
	team = ALLTEAM;		/* then check all others */
    for (i = 0; i < NUMTEAM; i++) {	/* go through all teams and eliminate */
	if ((team & (1 << i)) && (deadTeam(1 << i)))	/* from team var all
							   teams */
	    team &= ~(1 << i);	/* that are dead */
    }
    team1 = 0;			/* no team in team 1 */
    num1 = 0;			/* 0 players on team 1 */
    team2 = 0;			/* no team in team 2 */
    num2 = 0;			/* 0 players on team 2 */
    for (i = 0; i < NUMTEAM; i++) {	/* go through all teams */
	if (deadTeam(1 << i))	/* if team is dead then */
	    continue;		/* disregard it */
	if (realNumShips(1 << i) >= configvals->tournplayers) {	/* enough players */
	    if (!team1)		/* if no team in team1 yet */
		team1 = (1 << i);	/* then this will be team 1 */
	    else {		/* otherwise its team2 */
		team2 = (1 << i);
		num1 = realNumShips(team1);	/* get num players on two
						   teams */
		num2 = realNumShips(team2);
		if (num1 == num2) {	/* if teams same size then */
		    if (team & (team1 | team2))	/* if player on one team */
			return (team & (team1 | team2));	/* let him join same
								   team */
		    return (team1 | team2);	/* otherwise, he can join
						   either */
		}
		else if ((num1 > num2) && (team != team1))
		    return (team2);	/* if slight imabalance */
		else if ((num1 < num2) && (team != team2))
		    return (team1);	/* if slight imbalance */
		else {
		    if (ABS(num1 - num2) < 2 || (((num1 > num2) && (team == team2)) ||
					    (num2 > num1 && team == team1)))
			return (team);
		    else
			return (team1 | team2);
		}
	    }
	}
    }				/* end of for loop */
    return (team);		/* just in case */
}


void 
getEntry(int *team, int *stype)
{


    int     switching = -1;	/* confirm switches 7/27/91 TC */
    inputMask = CP_OUTFIT;
    for (;;) {
	/* updateShips so he knows how many players on each team */
	if (blk_flag)
	    updateShips();
	sendMaskPacket(tournamentMask(me->p_team));
	if (blk_flag)
	    briefUpdateClient();
	flushSockBuf();
	/* Have we been busted? */
	if (me->p_status == PFREE) {
	    me->p_status = PDEAD;
	    me->p_explode = 600;
	}
	socketPause();
	readFromClient();
	if (isClientDead()) {
	    int     i;

	    if (noressurect)
		exitGame();
	    printf("Ack!  The client went away!\n");
	    printf("I will attempt to resurrect him!\n");

	    /* UDP fail-safe */
	    commMode = COMM_TCP;
	    if (udpSock >= 0)
		closeUdpConn();

	    /* For next two minutes, we try to restore connection */
	    shutdown(sock, 2);
	    sock = -1;
	    for (i = 0;; i++) {
		switch (me->p_status) {
		case PFREE:
		    me->p_status = PDEAD;
		    me->p_explode = 600;
		    break;
		case PALIVE:
		case POBSERVE:
		    me->p_ghostbuster = 0;
		    break;
		case PDEAD:
		    me->p_explode = 600;
		    break;
		default:
		    me->p_explode = 600;
		    me->p_ghostbuster = 0;
		    break;
		}
		sleep(5);
		if (connectToClient(host, nextSocket))
		    break;
		if (i == 23) {
		    printf("Oh well, maybe I'm getting rusty!\n");
		    switch (me->p_status) {
		    case PFREE:
			break;
		    case PALIVE:
		    case POBSERVE:
			me->p_ghostbuster = 100000;
			break;
		    case PDEAD:
			me->p_explode = 0;
			break;
		    default:
			me->p_explode = 0;
			me->p_ghostbuster = 100000;
			break;
		    }
		    exitGame();
		}
	    }
	    printf("A miracle!  He's alive!\n");
	    teamPick = -1;
	    updateSelf();
	    updateShips();
	    flushSockBuf();
	}
	if (teamPick != -1) {
	    if (teamPick < 0 || teamPick > 3) {
		warning("Get real!");
		sendPickokPacket(0);
		teamPick = -1;
		continue;
	    }
	    if (!(tournamentMask(me->p_team) & (1 << teamPick))) {
		warning("I cannot allow that.  Pick another team");
		sendPickokPacket(0);
		teamPick = -1;
		continue;
	    }

	    if (((1 << teamPick) != me->p_team) &&
		(me->p_team != ALLTEAM))	/* switching teams 7/27/91 TC */
		if ((switching != teamPick)
		    && (me->p_whydead != KGENOCIDE)
		    && !status2->league
		    ) {
		    switching = teamPick;
		    warning("Please confirm change of teams.  Select the new team again.");
		    sendPickokPacket(0);
		    teamPick = -1;
		    continue;
		}
	    /* His team choice is ok. */
	    if (shipPick < 0 || shipPick >= NUM_TYPES) {
		warning("That is an illegal ship type.  Try again.");
		sendPickokPacket(0);
		teamPick = -1;
		continue;
	    }
	    if (!allowed_ship(1 << teamPick, mystats->st_rank, mystats->st_royal, shipPick)) {
		sendPickokPacket(0);
		teamPick = -1;
		continue;
	    }
	    if (shipvals[shipPick].s_nflags & SFNMASSPRODUCED) {
		warning("o)utpost, u)tility, or s)tandard?");
		sendPickokPacket(0);
		tmpPick = shipPick;
		continue;
	    }
	    break;
	}
	if (me->p_status == PTQUEUE) {
	    if (status->tourn)
		detourneyqueue();
	    /* You don't time out on the tourney queue */
	    me->p_ghostbuster = 0;
	}
    }
    *team = teamPick;
    if ((shipvals[tmpPick].s_nflags & SFNMASSPRODUCED) &&
	((shipvals[shipPick].s_numports) || (shipPick == SCOUT))) {

	*stype = tmpPick;
	tmpPick = CRUISER;
    }
    else {
	*stype = shipPick;
	tmpPick = CRUISER;
    }
    sendPickokPacket(1);
    flushSockBuf();
}





/*--------------------------------REALNUMSHIPS-----------------------------*/
/*  Count the real number of ships on a team.  This function returns a
count of all the ships on a team that are not PFREE.  */

int 
realNumShips(int owner)		/* the team to check for */
{
    int     i;			/* looping var */
    int     num;		/* to hold ship count */
    struct player *p;		/* to point to players */

    num = 0;			/* initialize count */
    for (i = 0, p = players; i < MAXPLAYER; i++, p++) {
	if ((p->p_status != PFREE) &&
	    !(p->p_flags & (PFSNAKE | PFBIRD)) &&
	    (p->p_team == owner)) {
	    num++;		/* if ship not free and on our team */
	}
    }
    return (num);		/* retun number of ships */
}


/*---------END OF FILE--------*/
