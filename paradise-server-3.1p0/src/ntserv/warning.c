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
#include "packets.h"

#define WQLEN 8
#define WARN_GAP 300000		/* microseconds */

/*
** The warning in text will be printed in the warning window.
** The message will last WARNTIME/10 seconds unless another message
** comes through and overwrites it.
*/

/* a small buffer of warnings to be sent... we want to space them out,
   since warning() is sometimes called in quick succesison */

static struct warning_spacket w_buf[WQLEN];
static int numw = 0, whichw = 0;

static struct timeval space = {0, 0};

/*-------------------------------VISIBLE FUNCTIONS------------------------*/

/*--------------------------------WARNING---------------------------------*/
/*  This function sends a warning message to the client, or queues the
warning to be sent a little later if we just sent one.  The warning can
be a maximum of 79 characters plus the delimiter.  */


void 
warning(char *text)	/* warning string */
{
    struct warning_spacket warn;/* warning packet to send warning in */
    struct warning_spacket *wp;
    int     t;

    t = temporally_spaced(&space, WARN_GAP);
    if (!numw && t) {
	/* don't need to queue it */
	warn.type = SP_WARNING;	/* set the type */
	strncpy(warn.mesg, text, 80);	/* copy message into packet */
	warn.mesg[79] = '\0';	/* chop message to 79 chars + delimeter */
	sendClientPacket((struct player_spacket *) & warn);	/* send the warning
								   packet */
    }
    else {
	/* first check to see if this warning has already been queued */
	if (!strcmp(text, w_buf[(whichw + numw - 1) % WQLEN].mesg))
	    return;

	wp = &w_buf[(whichw + numw) % WQLEN];

	wp->type = SP_WARNING;
	strncpy(wp->mesg, text, 80);
	wp->mesg[79] = '\0';

	if (++numw == WQLEN)
	    numw = WQLEN - 1;

	/* This is useful especially if updates/sec is set low */
	if (t) {		/* then there's been enough time to send
				   another one */
	    sendClientPacket((struct player_spacket *) & w_buf[whichw]);
	    whichw = (whichw + 1) % WQLEN;
	    numw--;
	}
    }
}


/*---------------------------- UPDATEWARNINGS ---------------------------*/
/* Called by updateClient.  Check to see if there are warnings queued, and
send one when enough time has gone by */

void 
updateWarnings(void)
{
    if (!numw)
	return;			/* nothing queued */

    if (!temporally_spaced(&space, WARN_GAP))
	return;			/* not time yet */

    /* we're assuming that the entry in w_buf has been properly filled in */
    sendClientPacket((struct player_spacket *) & w_buf[whichw]);

    whichw = (whichw + 1) % WQLEN;
    numw--;
}

/*------------------------------ IMM_WARNING ----------------------------*/

/* This sends a warning, bypassing the time-interval check.  Used for
   messages that give continuous updates (like bomb and beam messages)
   that are sent more frequently then the time interval allows. */

void 
imm_warning(char *text)
{
    struct warning_spacket warn;

    warn.type = SP_WARNING;
    strncpy(warn.mesg, text, 80);
    warn.mesg[79] = '\0';
    sendClientPacket((struct player_spacket *) & warn);
}

/*------------------------------------------------------------------------*/


/*-------END OF FILE-------*/
