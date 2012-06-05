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

static void 
mapWaitCount(unsigned int count)
{
    if (count == -1)
	return;

    sendQueuePacket((short) count);
    blk_flag = 1;
    updateMOTD();
    blk_flag = 0;

    undeferDeferred();		/* send the MOTD through the TCP buffers */
    flushSockBuf();
}

static int
allocate_slot(enum HomeAway homeaway, int overload, int allocate)
{
    int	i;
    if (overload) {
	for (i=MAXPLAYER-1; i>=0; i--) {
	    if (players[i].p_status==PFREE) {
		if (allocate) {
		    players[i].p_status = POUTFIT;
		    players[i].p_team = NOBODY;
		}
		return i;
	    }
	}
    } else {
	if (status2->league) {
	    /* for league play, make sure one team doesn't crowd out
               the other. */
	    int	count=0;
	    for (i=0; i<MAXPLAYER; i++) {
		if (players[i].p_status==PFREE)
		    continue;
		if (players[i].p_homeaway==homeaway)
		    count++;
	    }
	    if (count*2 >= MAXPLAYER-configvals->ntesters)
		return -1;		/* our team is full */
	}
	for (i=0; i< MAXPLAYER - configvals->ntesters; i++) {
	    if (players[i].p_status==PFREE) {
		if (allocate) {
		    players[i].p_status = POUTFIT;
		    players[i].p_team = NOBODY;
		}
		return i;
	    }
	}
    }
    return -1;
}

/*
 * The following code for grabslot() is really bizarre, and needs an
 *   explaination.
 *
 * Basically, the queue works like this:  Each process that needs to wait
 *   takes a number, and when that number comes up, the process enters
 *   the game.  status->count is the next number to take for a new process,
 *   and status->wait is the current process being served.
 *
 * However, this is not enough to determine exactly how many people are
 *   waiting, because people may drop out.  So, 3 more variables are added,
 *   status->request posts a request of some sort to all of the other
 *   processes, and status->number is the process posting the request.
 *   Also, status->answer is available for any kind of response needed from
 *   the other processes.  (Needless to say, only one process can make
 *   a request at any one time).
 *
 * Every process will wait for a second (sleep(1)), and then check the queue
 *   status, do what is appropriate, and repeat.
 *
 * Above and beyond this, processes may die, and not report it (for whatever
 *   reason, and every process waiting on the queue watches the behavior of
 *   every other supposed process to make sure it is still alive).
 *
 * When a space opens up, and the person who holds the number currently
 *   being served does not respond, then the processes argue over who deserves
 *   to get in next.  The first process to decide that no one is going
 *   to take the free slot says that his number is the next on to be served.
 *   He waits for anyone to disagree with him.  Any other processes which
 *   notice that their number is lower than the number being served claim that
 *   their number is the next one to be served.  They also wait for anyone to
 *   disagree with them.  Eventually, everyone is done waiting, and the process
 *   with the lowest count will be served.
 *
 * Variables:
 *   status->wait:  	Number being served.
 *   status->count:	Next number to take.
 *
 *   status->request:	Process request.
 *   (status->number)
 *   (status->answer)
 *	REQFREE:	No requests pending.
 *	REQWHO:		How many people are before me?  In this case, every
 *			process in from of this process (whose position is
 *			recorded in status->number) increments status->answer.
 *	REQDEAD:	I am leaving the queue (Either to quit, or enter
 *			game).  Any process whose position is higher than
 *			this process will note that they are closer to the
 *			top of the queue.
 *
 * (Local)
 * waitWin:	The window.
 * qwin:	The quit half of waitWin.
 * countWin:	The count half of waitWin.
 * count:	My number (Position in queue).
 * pseudocount: Number of people in front of me (-1 means I don't know).
 * myRequest:	This keeps track of requests I've made.  If it is non zero,
 *		then this is the number of times I will leave it there before
 *		I get my answer (if there is one), and reset status->request.
 * idie:	This is set to one if I need to leave the queue.  When I
 *		get a chance, I will submit my request, and leave.
 * wearein:	This is the slot we grabbed to enter the game.  If it is -1,
 *		then we haven't got a slot yet.  If we can grab a slot, then
 *		wearein is set to the slot #, and idie is set to 1.
 *
 * Because we need to monitor other processes, the following variables also
 *   exist:
 *
 * oldcount:	The number that was being served last time I looked.
 * waits:	Number of times I've seen oldcount as the number being served.
 *		If a position opens in the game, and no one takes it for a
 *		while, we assume that someone died.
 * lastRequest:	The last request I have seen.
 * lastNumber:	The process making this request.
 * reqCount:	Number of times I've seen this request.  If I see this request
 *		9 times, I assume it is obsolete, and I reset it.
 */

/* overload - indicates request for tester's slot */
static int
grabslot(int overload, enum HomeAway homeaway)
{
    int     count;		/* My number */
    int     oldcount;		/* Number that was being served last check */
    int     i;
    int     waits;		/* # times I have waited for someone else to
				   act */
    int     oldwait;		/* Number being served last check */
    int     pseudocount = -1;	/* Count on queue for sake of person waiting */
    int     myRequest = 0;	/* To keep track of any request I'm making */
    int     lastRequest = 0;	/* Last request I've seen */
    int     lastNumber = 0;	/* Last person making request */
    int     reqCount = 0;	/* Number of times I've seen this */
    int     idie = 0;		/* Do I want to die? */
    int     wearein = -1;	/* Slot we got in the game */
    int     rep = 0;

    /* If other players waiting, we get in line behind them */
    if (!overload && status->wait != status->count) {
	count = status->count++;
    }
    else {
	/* Get in game if posible */
	i = allocate_slot(homeaway, overload, 1);
	if (i>=0)
	    return i;
	/* Game full.  We will wait. */
	count = status->count++;
    }
    waits = 0;
    oldwait = -1;
    oldcount = status->wait;

    /* For count = 0,1,2  I know that it is right */
    if (count - status->wait < 1) {
	pseudocount = count - status->wait;
    }
    for (;;) {
	/* Send packets occasionally to see if he is accepting... */
	if (rep++ % 10 == 0) {
	    mapWaitCount(pseudocount);
	}
	if (isClientDead()) {
	    if (count == status->count - 1) {
		status->count--;
		exit(0);
	    }
	    /* If we are at top, decrease it */
	    if (count == status->wait) {
		status->wait++;
	    }
	    idie = 1;
	    /* break;		warning, function has return e and return */
	    exit(0);
	}
	if (status->wait != oldcount) {
	    mapWaitCount(pseudocount);
	    oldcount = status->wait;
	}
	/* To mimize process overhead and aid synchronization */
	sleep(1);
	/* Message from daemon that it died */
	if (status->count == 0)
	    exit(0);
	/* I have a completed request? */
	if (myRequest != 0 && --myRequest == 0) {
	    if (idie && status->request == REQDEAD) {
		status->request = REQFREE;
		/* Out of queue, into game */
		if (wearein != -1) {
		    return (wearein);
		}
		exit(0);
	    }
	    pseudocount = status->answer;
	    status->request = REQFREE;
	    if (pseudocount > 18)
		idie = 1;
	    mapWaitCount(pseudocount);
	}
	/* Tell the world I am going bye bye */
	if (idie && status->request == REQFREE) {
	    status->request = REQDEAD;
	    status->number = count;
	    myRequest = 4;
	}
	/* Should I request a count for # of people waiting? */
	if (pseudocount == -1 && status->request == REQFREE) {
	    status->request = REQWHO;
	    status->number = count;
	    status->answer = 0;
	    myRequest = 4;
	    /* I give people 4 seconds to respond */
	}
	/* Is someone else making a request? */
	if (status->request != REQFREE && myRequest == 0) {
	    if (status->request == lastRequest &&
		status->number == lastNumber) {
		reqCount++;
		/* 9 occurances of the same request implies that the process */
		/* died.  I will reset request. */
		if (reqCount > 8) {
		    status->request = REQFREE;
		}
	    }
	    else {
		lastRequest = status->request;
		lastNumber = status->number;
		reqCount = 1;
		if (lastRequest == REQWHO) {
		    if (count < lastNumber) {
			status->answer++;
		    }
		}
		else if (lastRequest == REQDEAD) {
		    if (count > lastNumber && pseudocount != -1) {
			pseudocount--;
			mapWaitCount(pseudocount);
		    }
		}
	    }
	}
	/* If someone raised wait too high, I claim that I * am next in line */
	if (status->wait > count && !idie) {
	    status->wait = count;
	    /* Give people a chance to correct me */
	    sleep(2);
	}
	if (idie)
	    continue;
	if (count==status->wait) {
	    i = allocate_slot(homeaway, overload, 1);
	    if (i<0)
		continue;
	    status->wait++;
	    wearein = i;
	    idie = 1;
	} else {
	    i = allocate_slot(homeaway, overload, 0);
	    if (i<0)
		continue;
	    if (oldwait == status->wait) {
		waits++;
	    }
	    else {
		oldwait = status->wait;
		waits = 1;
	    }
	    /* If this is our fifth wait (5 sec), then something is */
	    /* wrong.  We assume someone died, and fix this problem */
	    if (waits == 5 && !idie) {
		/* I want to be next in line, so I say so. */
		status->wait = count;
		/* And I allow someone to correct me if I'm wrong */
		sleep(2);
		waits = 0;
	    }
	    
	}
	/* this location is skipped if we didn't find a slot */
    }
}

int 
findslot(int overload, enum HomeAway homeaway)
{
    int     i;

    i = grabslot(overload, homeaway);
    players[i].p_pos = -1;
    memset(&players[i].p_stats, 0, sizeof(struct stats));
    players[i].p_stats.st_tticks = 1;
    players[i].p_stats.st_flags = ST_INITIAL;
    return (i);
}
