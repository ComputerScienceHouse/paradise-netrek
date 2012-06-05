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


#include <ctype.h>
#include <signal.h>
#include "config.h"
#include "proto.h"
#include "data.h"
#include "shmem.h"


/* from snakemove.c */
RETSIGTYPE snakemove();
RETSIGTYPE exitSnake();

struct player *perfs[2];
static struct itimerval udt;		/* derived from frequency RF */
int     num_perfs = 2;
int     debug = 0;
int     target = -1;
int     berserk = 0;
int     patrol = 0;
int     noSmush = 0;
int     plan_guard = 0;		/* KAO */
int     planet1, planet2;	/* KAO */
int     team1 = 0, team2 = 0;
int     length = MAXTORP;	/* how many pairs of torps in the snake */
int     lastm;
int     tno = 0;


/* sets up team stuff */
static int choose_team(int team) {
    if (tno < 0)
	tno = team;
    if (!team1)
	team1 = 1 << team;
    else if (!team2)
	team2 = 1 << team;
    return 1;
}


/* sets up the configuration for the snake */
static int config(void) {
    /* mostly not used */
    myship->s_phaser.cost = 0;
    myship->s_torp.cost = 0;
    myship->s_cloakcost = 0;
    myship->s_torp.fuse = 32767;
    myship->s_torp.damage = 10;
    myship->s_plasma.damage = 50;
    myship->s_plasma.cost = 0;
    myship->s_plasma.aux = 0;
    myship->s_plasma.speed = 10;
    myship->s_plasma.fuse = 32767;
    myship->s_torp.fuse = 32767;
    myship->s_wpncoolrate = 100;
    myship->s_egncoolrate = 100;
    return 1;
}  /* end config() */


/* prints the usage of snake */
static void printsnakeUsage(void) {
    printf("Usage: snake [options]\n");
    printf("Options:\n\
  -p		-- patrol\n\
  -n		-- noSmush (snake is not vindictive)\n\
  -s		-- Smush (snake is vindictive)\n\
  -g # #	-- guardian: -g <planet1> <planet2>\n\
		   (must be 2 planets listed, by number).\n\
  -b		-- berserk\n\
  -d		-- debug\n\
  -t#		-- target <player number>\n\
  -T[frkoi]	-- team [frkoi]\n\
  -l		-- length (in torps)\n\
  -f		-- frequency\n\
  -u		-- this usage message\n");
}  /* end printsnakeUsage() */


/* 
 * likewise as in robotII.c, we don't need ntserv/warning() but don't
 * want to drag in the entirety of ntserv.  Declare null function here.
 */
void warning(char *t) { }


/* main parses the input and starts the snake */
int main(int argc, char **argv)
{
    register int i;
    int     pno;
    char    tlet;
    int     usage = 0;
    double  frequency = 10;	/* default of 10 updates per second */
    char   *rf;

    argv0 = argv[0];
    tno = -1;

    srand48(getpid() + time((time_t *) 0));
    memset(perfs, 0, sizeof(perfs));

    for (; argc > 1 && argv[1][0] == '-'; argc--, argv++) {
	switch (argv[1][1]) {
	case 'p':
	    patrol++;
	    break;
	case 'n':
	    noSmush = 1;
	    break;
	case 's':
	    noSmush = -1;
	    break;
	case 'g':		/* KAO */
	    plan_guard++;	/* KAO */
	    argv++;		/* KAO */
	    argc--;
	    planet1 = atoi(argv[1]);	/* KAO */
	    argc--;
	    argv++;		/* KAO */
	    planet2 = atoi(argv[1]);	/* KAO */
	    break;		/* KAO */
	case 'b':
	    berserk++;
	    break;
	case 'd':
	    debug++;
	    break;
	case 't':{		/* target */
		char    c;
		c = argv[1][2];
		target = -1;
		if (c == '\0') {
		    fprintf(stderr, "Must specify target - example: "
			"\"snake -t#\"  where # is a player number\n\n");
		    usage++;
		}
		else if ((c >= '0') && (c <= '9')) {
		    target = c - '0';
		}
		else if ((c >= 'a') && (c <= 'z')) {
		    target = c - 'a' + 10;
		}
		else {
		    fprintf(stderr, "Must specify a valid target - example: "
			"\"snake -t#\"  where # is a player number\n\n");
		    usage++;
		}
	    }
	    break;

	case 'T':		/* team */
	    tlet = argv[1][2];
	    if (isupper(tlet))
		tlet = tolower(tlet);
	    switch (tlet) {
	    case 'f':
		choose_team(0);
		break;
	    case 'r':
		choose_team(1);
		break;
	    case 'k':
		choose_team(2);
		break;
	    case 'o':
		choose_team(3);
		break;
	    case 'i':
		tno = 4;
		break;
	    default:
		fprintf(stderr, "Unknown team type - example: "
			"\"snake -Tx\"  where x is [frkoi]\n\n");
		usage++;
		break;
	    }			/* end switch argv */
	    break;

	case 'l':
	    length = atoi(argv[1] + 2);
	    if (length < 1) {
		length = 1;
	    }
	    else if (length > MAXTORP) {
		length = MAXTORP;
	    }
	    break;
	case 'f':
	    frequency = atof(argv[1] + 2);
	    if (frequency < 0) {
		frequency = 1;
	    }
	    break;
	case 'u':
	    usage++;
	    break;
	default:
	    fprintf(stderr, "Unknown option '%c'\n\n", argv[1][1]);
	    usage++;
	    break;
	}			/* end switch argv[1][1] */

	/* detect error conditions and output a usage statement */
    	if (usage > 0) {
	    printsnakeUsage();
	    exit(1);
        }
    }				/* end for */


    /* if -T wasn't specified default to IND */
    if (tno < 0) {
	tno = 4;
    }

    /* XX -- teams imply patrol */
    if (team1 && team2)
	patrol++;

/*   readsysdefaults();*/

    /* init ranks data */
    rf = build_path(RANKS_FILE);
    init_data(rf);

    /* init trig tables */
    init_trig();

    (void) r_signal(SIGHUP, exitSnake);
    (void) r_signal(SIGINT, exitSnake);
    (void) r_signal(SIGBUS, exitSnake);
    (void) r_signal(SIGSEGV, exitSnake);
    openmem(0, 0);

    lastm = mctl->mc_current;

    for (i = 0; i < 2; i++) {	/* two players per snake */
	pno = findrslot();
	if (pno < 0) {
	    fprintf(stderr, "snake: no room in game\n");
	    if (i > 0)
		perfs[0]->p_status = PFREE;
	    exit(1);
	}
	me = &players[pno];

	perfs[i] = me;

	me->p_no = pno;
	myship = &me->p_ship;
	mystats = &me->p_stats;

	strcpy(pseudo, "The Snake");
	strcpy(login, "SnkeChrmr");

	strcpy(me->p_name, pseudo);
	me->p_name[sizeof(me->p_name) - 1] = '\0';

	(void) strncpy(me->p_login, login, sizeof(me->p_login));
	me->p_login[sizeof(me->p_login) - 1] = '\0';
	(void) strncpy(me->p_monitor, "Server", sizeof(me->p_monitor));
	me->p_monitor[sizeof(me->p_monitor) - 1] = '\0';
	/* enter(tno, 0, pno, class, -1); */

	me->p_team = (tno < 4) ? (1 << tno) : 0;
	config();

	me->p_pos = -1;
	me->p_flags |= PFROBOT;	/* Mark as a robot */
	me->p_flags |= PFSNAKE;	/* Mark as snake */
	if (berserk)
	    me->p_hostile = FED | ROM | ORI | KLI;
    }

    r_signal(SIGALRM, snakemove);

    {
	double  period = 1 / frequency;
	udt.it_interval.tv_sec = period;	/* get the whole part */
	period -= udt.it_interval.tv_sec;	/* get the fractional part */
	udt.it_interval.tv_usec = 1e6 * period;
	udt.it_value.tv_sec = 1;
	udt.it_value.tv_usec = 0;
    }

    if (setitimer(ITIMER_REAL, &udt, 0) < 0) {
	perror("setitimer");
	for (i = 0; i < num_perfs; i++) {
	    if (perfs[i])
		perfs[i]->p_status = PFREE;
	}
	exit(1);
    }
    /* allows robots to be forked by the daemon on some systems */
    {
      sigset_t unblock_everything;
      sigfillset(&unblock_everything);
      sigprocmask(SIG_UNBLOCK, &unblock_everything, NULL);
    }

    /* NOTE: snakes do not become alive. */

    while (1) {
	pause();
    }
}

