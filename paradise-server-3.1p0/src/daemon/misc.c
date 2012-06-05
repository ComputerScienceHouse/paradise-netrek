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

/*------------------------------MODULE VARIABLES--------------------------*/

#if 0
#define	NMESSAGES	16
 /* The list of messages that are sent when t-mode begins.  */
static char *warmessages[] =
{
    "A dark mood settles upon the galaxy as galactic superpowers seek",
    "to colonize new territory.  Border disputes errupt developing",
    "into all out intergalactic WAR!!!",

    "Slick Willy is elected president, the bottom falls out of the",
    "economy, and the pressure to expand rises.",

    "Clinton found with foreign emperor's wife!",
    "All out war ensues.",

    "Taxes on the rich raised to 80%.",
    "The rich flee to neigboring empire.",
    "Democrats demand extradition.",
    "Foreign emporer refuses.",
    "Hillary 'Rob Them' Clinton declares war!",
    "Bill hides out in London.",

    "INTERGALACTIC BEER SHORTAGE!",
    "CHAOS LIKE IT HAS NEVER BEEN SEEN BEFORE ENSUES!",

    "The American Gladiators hit as the number 1 show on the vid.",
    "After a gruesome episode, the masses storm out their homes,",
    "jump in their ships, make a quick stop at their local 7-11",
    "to grab a few beers, then head out into the galaxy to ",
    "KICK SOME ASS.  WAR!!!!!!!!!!!!!!!!!!",

    "Khan is resurrected in a bizzare experiment.  Elected as head of",
    "the Starfleet Council, he removes all pajama-wearing new-age",
    "hippies from Starfleet.  (And you thought the borg were bad news)",

    "Several members of the computing support staff of the Galactic",
    "Council are severely injured in a freak skydiving accident.",
    "The rest are killed.  The network collapses without their",
    "guidance.  Galactic chaos breaks out!",

    /* oldies */
    "A dark mood settles upon the galaxy",
    "Political pressure to expand is rising",
    "Border disputes break out as political tensions increase!",
    "Galactic superpowers seek to colonize new territory!",
    "'Manifest Destiny' becomes motive in galactic superpower conflict!",
    "Diplomat insults foriegn emperor's mother and fighting breaks out!",
    "Dan Quayle declares self as galactic emperor and chaos breaks out!",
    "Peace parties have been demobilized, and fighting ensues.",
};


 /*
    The starting index of the message series and the number of messages in
    series.
 */
int     warm[NMESSAGES][2] = {
    {0, 3},
    {3, 2},
    {5, 2},
    {7, 6},
    {13, 2},
    {15, 5},
    {20, 3},
    {23, 4},
    {27, 1},
    {28, 1},
    {29, 1},
    {30, 1},
    {31, 1},
    {32, 1},
    {33, 1},
    {34, 1},
};


 /* The set of message series that are displayed when t-mode ends. */
static char *peacemessages[] =
{
    "The expansionist pressure subsides, and temporary agreements are",
    "reached in local border disputes.",
    "Peace returns to the galaxy...",

    "Wild mob storms the White House!",
    "Clinton flees.",
    "Order returns to the galaxy.",

    "Slick Willy apologizes about incident with foreign emperor's wife.",
    "Claims he did not penetrate.",
    "Peace ensues.",

    "The economy goes belly up.  The Democrats are kicked",
    "out of office, tarred and feathered, and sent to the",
    "zoo on Rigel 4.  Capitalism is implemented and order",
    "returns to the empire.",

    "Officials sieze hidden beer stockpiles at the Mackenzie brother's",
    "house!  The beer shortage is over.  Peace breaks out.",

    "The people decide they would rather sit home and watch",
    "Al Bundy than fight.  The war comes to an end.",

    "Khan takes a fatal blow to the kidneys when Kirk returns from",
    "retirement and whacks him in the back with his walker.",
    "It looks like the hippies are back in control.",

    "Sole survivors of the skydiving accident that took out the",
    "rest of the computing support staff, Michael McLean and Brad",
    "Spatz are released from intensive care and rebuild the",
    "network.  Peace is restored to the Galaxy.",

    /* oldies */
    "A new day dawns as the oppressive mood lifts",
    "The expansionist pressure subsides",
    "Temporary agreement is reached in local border disputes.",
    "Galactic governments reduce colonization efforts.",
    "'Manifest Destiny is no longer a fad.' says influential philosopher.",
    "Diplomat apologizes to foreign emperor's mother and invasion is stopped!",
    "Dan Quayle is locked up and order returns to the galaxy!",
    "The peace party has reformed, and is rallying for peace",
};


 /*
    The starting index of each message series and the number of messages in
    the series.
 */
static int peacem[NMESSAGES][2] = {
    {0, 3},
    {3, 3},
    {6, 3},
    {9, 4},
    {13, 2},
    {15, 2},
    {17, 3},
    {20, 4},
    {24, 1},
    {25, 1},
    {26, 1},
    {27, 1},
    {28, 1},
    {29, 1},
    {30, 1},
    {31, 1},
};
#else

/* New message format (much more maintainable than the old message
   format).  Instead of keeping track of (offset,count) for each
   message, each message is a single string (ANSI concat rules in
   effect here) with each line in the message (except the last) broken by \n.
   When the message is sent to the client, each \n-terminated segment
   (or \0-terminated segment if the last segment)
   of the message is sent prefixed by WAR-> or PEACE-> (whichever is
   appropriate).

   Sure, we have to do a few strchr's, but what the hell */

 /* The list of messages that are sent when t-mode begins.  */
static char *warmessages[] =
{
    "A dark mood settles upon the galaxy as galactic superpowers seek\n"
    "to colonize new territory.  Border disputes errupt developing\n"
    "into all out intergalactic WAR!!!",

    "Slick Willy is elected president, the bottom falls out of the\n"
    "economy, and the pressure to expand rises.",

    "Clinton found with foreign emperor's wife!\n"
    "All out war ensues.",

    "Taxes on the rich raised to 80%.\n"
    "The rich flee to neigboring empire.\n"
    "Democrats demand extradition.\n"
    "Foreign emporer refuses.\n"
    "Hillary 'Rob Them' Clinton declares war!\n"
    "Bill hides out in London.",

    "INTERGALACTIC BEER SHORTAGE!\n"
    "CHAOS LIKE IT HAS NEVER BEEN SEEN BEFORE ENSUES!",

    "The American Gladiators hit as the number 1 show on the vid.\n"
    "After a gruesome episode, the masses storm out their homes,\n"
    "jump in their ships, make a quick stop at their local 7-11\n"
    "to grab a few beers, then head out into the galaxy to\n"
    "KICK SOME ASS.  WAR!!!!!!!!!!!!!!!!!!",

    "Khan is resurrected in a bizzare experiment.  Elected as head of\n"
    "the Starfleet Council, he removes all pajama-wearing new-age\n"
    "hippies from Starfleet.  (And you thought the borg were bad news)",

    "Several members of the computing support staff of the Galactic\n"
    "Council are severely injured in a freak skydiving accident.\n"
    "The rest are killed.  The network collapses without their\n"
    "guidance.  Galactic chaos breaks out!",

    /* oldies */
    "A dark mood settles upon the galaxy",
    "Political pressure to expand is rising",
    "Border disputes break out as political tensions increase!",
    "Galactic superpowers seek to colonize new territory!",
    "'Manifest Destiny' becomes motive in galactic superpower conflict!",
    "Diplomat insults foriegn emperor's mother and fighting breaks out!",
    "Dan Quayle declares self as galactic emperor and chaos breaks out!",
    "Peace parties have been demobilized, and fighting ensues.",
};

#define NWARMESSAGES (sizeof(warmessages) / sizeof(char *))

 /* The set of message series that are displayed when t-mode ends. */
static char *peacemessages[] =
{
    "The expansionist pressure subsides, and temporary agreements are\n"
    "reached in local border disputes.\n"
    "Peace returns to the galaxy...",

    "Wild mob storms the White House!\n"
    "Clinton flees.\n"
    "Order returns to the galaxy.",

    "Slick Willy apologizes about incident with foreign emperor's wife.\n"
    "Claims he did not penetrate.\n"
    "Peace ensues.",

    "The economy goes belly up.  The Democrats are kicked\n"
    "out of office, tarred and feathered, and sent to the\n"
    "zoo on Rigel 4.  Capitalism is implemented and order\n"
    "returns to the empire.",

    "Officials sieze hidden beer stockpiles at the Mackenzie brother's\n"
    "house!  The beer shortage is over.  Peace breaks out.",

    "The people decide they would rather sit home and watch\n"
    "Al Bundy than fight.  The war comes to an end.",

    "Khan takes a fatal blow to the kidneys when Kirk returns from\n"
    "retirement and whacks him in the back with his walker.\n"
    "It looks like the hippies are back in control.",

    "Sole survivors of the skydiving accident that took out the\n"
    "rest of the computing support staff, Michael McLean and Brad\n"
    "Spatz are released from intensive care and rebuild the\n"
    "network.  Peace is restored to the Galaxy.",

    /* oldies */
    "A new day dawns as the oppressive mood lifts",
    "The expansionist pressure subsides",
    "Temporary agreement is reached in local border disputes.",
    "Galactic governments reduce colonization efforts.",
    "'Manifest Destiny is no longer a fad.' says influential philosopher.",
    "Diplomat apologizes to foreign emperor's mother and invasion is stopped!",
    "Dan Quayle is locked up and order returns to the galaxy!",
    "The peace party has reformed, and is rallying for peace",
};

#define NPEACEMESSAGES (sizeof(peacemessages) / sizeof(char *))

#endif

static int series = 0;		/* the message series that was printed */
 /* when t-mode started.  */

/*------------------------------------------------------------------------*/








/*------------------------------VISIBLE FUNCTIONS-------------------------*/

/*---------------------------------WARMESSAGE----------------------------*/
/*  This function is called when t-mode ensues.  It chooses a message series
and prints it out.  It records the message series in the module's series
variable so that the corresponding peace message series can be printed.  */

static void
tolstoy_message(char *p, char *transition)
{
  char *q;

  do
  {
    q = strchr(p, '\n');
    if(q)
      *q = 0;
    pmessage(p, 0, MALL, transition);
    if(q)
    {
      *q = '\n';
      p = q+1;
    }
  } while(q && *p);
}

void
warmessage(void)
{
  char *msg;
  series = lrand48() % NWARMESSAGES;
  msg = strdup(warmessages[series]);
  if(msg) {
    tolstoy_message(msg, "  WAR->  ");
    free(msg);
  }
}


#if 0
void 
warmessage(void)
{
    int     i;			/* to hold index */
    int     n;			/* number of messages in a series */

    i = lrand48() % NMESSAGES;	/* choose message series */
    series = i;			/* record series number */
    n = warm[i][1];		/* get number of messages in series */
    i = warm[i][0];		/* get index of first message */
    while (n > 0) {		/* print all messages */
	pmessage(warmessages[i], 0, MALL, "  WAR->  ");
	n--;			/* dec messages that need to be printed */
	i++;			/* on to next message */
    }
}
#endif



/*-------------------------------PEACEMESSAGE-----------------------------*/
/*  This function prints a peace message series.  It uses the module
variable series to decide which message series to print.  */

void
peacemessage(void)
{
  if(series >= NPEACEMESSAGES)
    series = lrand48() % NPEACEMESSAGES;
  tolstoy_message(peacemessages[series], " PEACE-> ");
}

#if 0
void 
peacemessage(void)
{
    int     i;			/* to hold index */
    int     n;			/* number of messages in a series */

    n = peacem[series][1];	/* get # messages in series */
    i = peacem[series][0];	/* get starting index */
    while (n > 0) {		/* print all messages */
	pmessage(peacemessages[i], 0, MALL, " PEACE-> ");
	n--;			/* dec messages that need to be printed */
	i++;			/* on to next message */
    }
}
#endif



/*-------------------------------REALNUMSHIPS-----------------------------*/
/*  This function counts the number of ships on a team.  If the race is the
same and the slot is not free, then the ship is counted.  */

int 
realNumShips(int owner)
{
    int     i, num;		/* for looping and counting */
    struct player *p;		/* to point to a player */

    num = 0;			/* zero the count of ships */
    for (i = 0, p = players; i < MAXPLAYER; i++, p++)
	if ((p->p_status != PFREE) && (p->p_team == owner))
	    num++;		/* found a ship on this team, inc count */
    return (num);		/* return number of ships */
}




/*------------------------------TOURNAMENTMODE------------------------------*/
/*  This function counts the number of players on each team and sees if two
or more teams have enough players for t-mode.  It returns a 0 for no team
mode.  */

int 
tournamentMode(void)
{
    if (status2->league) {
	return status2->league > 2;
    } else
      {
	static int wt = -1;	/* warning time due to team unbalance */
	static int lct = -1;	/* last check time (ie last pmessage) */
	char    buf[80];	/* temp buffer for tmode halt warning */
	int     i;		/* looping vars */
	int     t[16];		/* to count players with */
	int     counts[4];
	/* Even though teams go from 0-8, leave */
	/* this at 16...or bad things might happen */
	struct player *p;	/* to point to players */

	for (i = 0; i < 16; i++)/* clear the team players count array */
	    t[i] = 0;
	for (i = 0, p = players; i < MAXPLAYER; i++, p++) {
	    /* through all players */
	    if (p->p_flags&PFROBOT)
		continue;	/* robots don't count */
	    switch (p->p_status) {
	    case PFREE:		/* free slots obviously don't count */
	    case POBSERVE:	/* and neither do observers */
		break;

	    case PEXPLODE:
	    case PDEAD:
	    case PTQUEUE:
		if (p->p_observer)
		    break;	/* observers in these modes don't count */
		/* we can fall through here */
	    case POUTFIT:
	    case PALIVE:
		t[p->p_team]++;
		break;
	    }
	}

	i = 0;			/* zero count of # teams with tournplayers */
	if (t[FED] >= configvals->tournplayers)
	    counts[i++] = t[FED];
	if (t[ROM] >= configvals->tournplayers)
	    counts[i++] = t[ROM];
	if (t[KLI] >= configvals->tournplayers)
	    counts[i++] = t[KLI];
	if (t[ORI] >= configvals->tournplayers)
	    counts[i++] = t[ORI];
	if (i > 1) {
	    i = counts[0] - counts[1];
	    if (i < 0)
		i = -i;
	    if (i > 2) {	/* Too big a team imbalance */
		if (wt == -1) {
		    wt = status->clock;	/* make a timestamp */
		    lct = status->clock;	/* used every time it sends
						   the message */
		}
		if (((status->clock - wt) < 3) && (lct < status->clock)) {
		    /*
		       teams are unblananced, and we havn't said anything for
		       1 minute, warn everybody  Yes these are noisy strings,
		       but I want people to notice.
		    */
		    pmessage("**************************!! Teams are unbalanced !!**************************", 0, MALL, "");
		    sprintf(buf, "********************* TMODE WILL BE HALTED IN %li MINUTES **********************", (wt - status->clock) + 3);
		    pmessage(buf, 0, MALL, "");
		    pmessage("************************ if this problem isn't fixed *************************", 0, MALL, "");
		    lct = status->clock;
		    return (1);
		}
		else if ((status->clock - wt) >= 3) {
		    if (lct < status->clock) {
			pmessage("******************!! TMODE HALTED DUE TO TEAM IMBALANCE !!********************", 0, MALL, "");
			pmessage("*********************** Balance teams to resume tmode ************************", 0, MALL, "");
			lct = status->clock;
		    }
		    return (0);	/* stop tmode */
		}
	    }
	    else {
		wt = -1;
		lct = -1;
	    }
	    return (1);		/* return that we are in team mode */
	}
	return (0);		/* we are NOT in T-mode */
    }
}

/*
   send a message from god to a particular player number
   */

void 
god2player(char *str, int pno)
{
    struct message *cur;	/* to pnt to where to put message */

    if (++(mctl->mc_current) >= MAXMESSAGE)
	mctl->mc_current = 0;
    cur = &messages[mctl->mc_current];

    cur->m_no = mctl->mc_current;
    cur->m_flags = MINDIV;
    cur->m_recpt = pno;
    cur->m_from = 255;
    sprintf(cur->m_data, "%s->%s %s", SERVNAME, twoletters(&players[pno]), str);
    cur->m_flags |= MVALID;
}




/*-------------------------------LOG_MESSAGE------------------------------*/
/*  This function writes a message into the logfile so the server god can
look at it later.  It is used to record messages to god.  */

static void 
log_message(char *who, char *info, char *str)
{
    char   *path;
    FILE   *logfile;
    char   *tstr;
    char    namebuf[24];
    time_t  t;

    path = build_path(GODLOGFILE);
    logfile = fopen(path, "a");
    if (!logfile)
	return;	
    time(&t);
    tstr = ctime(&t);
    tstr[strlen(tstr)-1] = (char) NULL;
    strcpy(namebuf, who);
    strcat(namebuf, "->GOD");
    fprintf(logfile, "[%s/%s]\n  %-22s %s\n", tstr, info, namebuf, str);
    fclose(logfile);
}




/*-----------------------------PARSE_GODMESSAGES----------------------------*/
/*  This function checks all the new messages in the message buffer to see
if the keyword 'GOD' is at the first of the message.  If it is, then the
message is written to the logfile so the server god can read it.  */

void 
parse_godmessages(void)
{
    static int lastparsed = 0;		/* keeps track of last message */

    while (mctl->mc_current != lastparsed) {	/* Is there a new message? */
	struct message *cur;			/* to point to message struct */
	char   *s;				/* to point to string */

	if (++lastparsed >= MAXMESSAGE)		/* rollover after end of */
	    lastparsed = 0;			/* message buffer */
	cur = &messages[lastparsed];		/* get new message's struct */

	s = cur->m_data + 1;			/* get new message */
	while ((*s != ' ') && (*s != 0))	/* go until first space */
	    s++;
	while ((*s == ' ') && (*s != 0))	/* go past spaces */
	    s++;

	if (*s && ((cur->m_flags & MGOD) || strncmp(s, "GOD:", 4) == 0)) {
            if (!(cur->m_flags & MGOD))
                s += 4;
            if (cur->m_from >= 0)
                log_message(players[cur->m_from].p_name,
                            players[cur->m_from].p_login, s);
            else
                log_message("??", "??", s);
        }
    }
}

/*----------END OF FILE--------*/
