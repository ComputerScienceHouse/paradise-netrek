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

#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include "config.h"
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "proto.h"
#include "data.h"
#include "shmem.h"

/* from rmove.c */
RETSIGTYPE rmove();

static struct itimerval udt;
extern int redrawall;		/* maint: missing "extern" 6/22/92 TC */
extern int lastm;		/* maint: missing "extern" 6/22/92 TC */

/* lots of neat flags */
int     hostile;
int     debug;
int     level;
int     fleet;
int     sticky;
int     berserk;
int     practice;
int     nofuel;

int     polymorphic;		/* match opponent's ship class 8/15/91 TC */
int     target;			/* Terminator's target 7/27/91 TC */
int     phrange;		/* phaser range 7/31/91 TC */
int     trrange;		/* tractor range 8/2/91 TC */

int     startplanet = -1;	/* CRD feature - MAK,  2-Jun-93 */

/* velocities 8/9/91 TC */

int     dogslow;		/* dodge speed (hard turn) */
int     dogfast;		/* dodge speed (soft turn) */
int     runslow;		/* run speed (flat etemp) */
int     runfast;		/* run speed (gain etemp) */
int     closeslow;		/* approach speed (hard turn) */
int     closefast;		/* approach speed (soft turn) */

static char *rnames[6] = {
  "M5", "Colossus", "Guardian", "HAL", "DreadPirate Bob", "TERMINATOR"
};

static void do_robot_login(void) {
  int     plfd, position, entries;
  char   *path;
  struct statentry player;
  struct stat buf;

  if(configvals->robot_stats) {
    path = build_path(PLAYERFILE);
    plfd = open(path, O_RDONLY, 0);
    if (plfd < 0) {
	fprintf(stderr, "%s: I cannot open the player file! (errno: %d)\n",
		argv0, errno);
	me->p_pos = -1;
	return;
    }
    position = 0;
    while (read(plfd, (char *) &player, sizeof(struct statentry)) ==
	   sizeof(struct statentry)) {
	if (!strcmp(pseudo, player.name)) {
	    close(plfd);
	    me->p_pos = position;
	    memcpy(&(me->p_stats), &player.stats, sizeof(struct stats));
	    return;
	}
	position++;
    }

    /* Not in there, so create it */
    strcpy(player.name, pseudo);
    strcpy(player.password, "*");	/* an invalid password to prevent
					   logins */
    memset(&player.stats, 0, sizeof(struct stats));
    player.stats.st_tticks = 1;
    player.stats.st_flags = ST_INITIAL;

    plfd = open(path, O_RDWR | O_CREAT, 0644);
    if (plfd < 0) {
	fprintf(stderr, "%s: I cannot open the player file! (errno: %d)\n",
		argv0, errno);
	me->p_pos = -1;
	return;
    } 
    else {
	fstat(plfd, &buf);
	entries = buf.st_size / sizeof(struct statentry);
	lseek(plfd, entries * sizeof(struct statentry), 0);
	write(plfd, (char *) &player, sizeof(struct statentry));
	close(plfd);
	me->p_pos = entries;
	memcpy(&(me->p_stats), &player.stats, sizeof(struct stats));
    }
  }
}

/* yes, there is a warning() in ntserv/warning.c.  However, pulling
   in that version of warning() results in pulling in almost ALL of
   ntserv, which would hose things up pretty good.  Since the robot isn't
   interested in warnings from the server, we define a null function
   here instead. */
void warning(char *t) { }

void save_robot(void) {
  int     fd;
  char   *path;

  if(configvals->robot_stats) {
    if (me->p_pos < 0)
	return;
    path = build_path(PLAYERFILE);
    fd = open(path, O_WRONLY, 0644);
    if (fd >= 0) {
	me->p_stats.st_lastlogin = time(NULL);
	lseek(fd, 32 + me->p_pos * sizeof(struct statentry), 0);
	write(fd, (char *) &me->p_stats, sizeof(struct stats));
	close(fd);
    }
  }
}

void config(void) {
    /* calc class-specific stuff */

    phrange = PHASEDIST * me->p_ship.s_phaser.damage / 100;
    trrange = TRACTDIST * me->p_ship.s_tractrng;

    switch (myship->s_type) {
    case SCOUT:
	dogslow = 5;
	dogfast = 7;
	runslow = 8;
	runfast = 9;
	closeslow = 5;
	closefast = 7;
	break;
    case DESTROYER:
	dogslow = 4;
	dogfast = 6;
	runslow = 6;
	runfast = 8;
	closeslow = 4;
	closefast = 6;
	break;
    case CRUISER:
	dogslow = 4;
	dogfast = 6;
	runslow = 6;
	runfast = 7;
	closeslow = 4;
	closefast = 6;
	break;
    case BATTLESHIP:
	dogslow = 3;
	dogfast = 5;
	runslow = 5;
	runfast = 6;
	closeslow = 3;
	closefast = 4;
	break;
    case FRIGATE:
	dogslow = 3;
	dogfast = 5;
	runslow = 5;
	runfast = 6;
	closeslow = 3;
	closefast = 4;
	break;
    case ASSAULT:
	dogslow = 3;
	dogfast = 5;
	runslow = 6;
	runfast = 7;
	closeslow = 3;
	closefast = 4;
	break;
    case JUMPSHIP:
	dogslow = 2;
	dogfast = 3;
	runslow = 2;
	runfast = 3;
	closeslow = 2;
	closefast = 3;
	break;
    case STARBASE:
	dogslow = 2;
	dogfast = 2;
	runslow = 2;
	runfast = 2;
	closeslow = 2;
	closefast = 2;
	break;
    case WARBASE:
	dogslow = 2;
	dogfast = 2;
	runslow = 2;
	runfast = 2;
	closeslow = 2;
	closefast = 2;
	break;
    case LIGHTCRUISER:
	dogslow = 5;
	dogfast = 6;
	runslow = 6;
	runfast = 7;
	closeslow = 5;
	closefast = 7;
	break;
    case CARRIER:
	dogslow = 3;
	dogfast = 4;
	runslow = 5;
	runfast = 6;
	closeslow = 4;
	closefast = 6;
	break;
    case UTILITY:
	dogslow = 3;
	dogfast = 4;
	runslow = 5;
	runfast = 6;
	closeslow = 4;
	closefast = 5;
	break;
    case PATROL:
	dogslow = 7;
	dogfast = 8;
	runslow = 9;
	runfast = 10;
	closeslow = 8;
	closefast = 9;
	break;
    }

    if (debug)
	printf("My phaser range: %d.\n", phrange);
    if (debug)
	printf("My tractor range: %d.\n", trrange);

    if (!nofuel) {
	myship->s_phaser.cost = 0;
	myship->s_torp.cost = 0;
	myship->s_cloakcost = 0;
    }
    if (target >= 0) {		/* 7/27/91 TC */
	myship->s_imp.maxspeed = 20;	/* was 10, so you can't run */
	myship->s_imp.cost = 1;
	myship->s_egncoolrate = 100;
    }
}

int main(int argc, char **argv) {
    register int i;
    int     team = -1;
    int     bteam=0;
    int     pno;
    int     class;		/* ship class 8/9/91 TC */
    char    *rf;

    argv0 = argv[0];
    srand48(time(NULL) + getpid());

    openmem(0, 0);

    /* Pick a ship for the robot.  Don't allow robot to be invincible (AT),
       a sysconf-disallowed type, or any ship that has a timer requirement. */
    for (;;) {
	class = lrand48() % NUM_TYPES;	/* pick a ship type */
	if (class != ATT && shipsallowed[class] && shipvals[class].s_timer <= 0)
	    break;
    }

    target = -1;		/* no target 7/27/91 TC */

    for (; argc > 1 && argv[1][0] == '-'; argc--, argv++) {
	switch (argv[1][1]) {
	case 'S':
	    startplanet = atoi(&argv[1][2]);
	    if (startplanet >= configvals->numplanets) {
		printf("startplanet insane: %d\n", startplanet);
		startplanet = -1;
	    }
	    break;
	case 'F':
	    nofuel++;
	    break;
	case 'P':
	    polymorphic++;
	    break;
	case 'f':
	    fleet++;
	    break;
	case 's':
	    sticky++;
	    break;
	case 'd':
	    debug++;
	    break;
	case 'h':
	    hostile++;
	    break;
	case 'p':
	    practice++;
	    break;
	case 'b':
	    berserk++;
	    break;
	case 'l':
	    if (argv[1][2] != '\0')
		level = atoi(&argv[1][2]);
	    else
		level = 100;
	    break;
	case 'c':		/* ship class */
	    {
	        char    cstr[NUM_TYPES + 1];

	        for (class = 0; class <= NUM_TYPES; class++) {
		    if (class == NUM_TYPES) {
		        cstr[NUM_TYPES] = 0;
		        fprintf(stderr, 
			    "Unknown ship class, must be one of [%s].\n", cstr);
		        exit(1);
		    }
		    if (argv[1][2] == shipvals[class].s_letter)
		        break;
	            cstr[class] = shipvals[class].s_letter;
		}
	    }
	    break;
	case 'T':		/* team */
	    switch (argv[1][2]) {
	    case 'f':
		team = 0;
		bteam = FED;
		break;
	    case 'r':
		team = 1;
		bteam = ROM;
		break;
	    case 'k':
		team = 2;
		bteam = KLI;
		break;
	    case 'o':
		team = 3;
		bteam = ORI;
		break;
	    case 'n':		/* change 5/10/91 TC neutral */
	    case 'i':
		team = 4;
		bteam = FED | ROM | KLI | ORI;	/* don't like anybody */
		break;
	    case 't':
		{
		    char    c;
		    c = argv[1][3];

		    team = 5;
		    bteam = FED | ROM | KLI | ORI;	/* don't like anybody */
		    target = -1;
		    if (c == '\0') {
			fprintf(stderr, "Must specify target.  e.g. -Tt3.\n");
			exit(1);
		    }
		    if ((c >= '0') && (c <= '9'))
			target = c - '0';
		    else if ((c >= 'a') && (c <= 'v'))
			target = c - 'a' + 10;
		    else {
			fprintf(stderr, "Must specify target.  e.g. -Tt3.\n");
			exit(1);
		    }
		}		/* end case 't' */
		break;
	    default:
		fprintf(stderr, 
		    "Unknown team type.  Usage -Tx where x is [frko]\n");
		exit(1);
	    }			/* end switch argv */
	    break;
	default:
	    fprintf(stderr, 
		"Unknown option '%c' (must be one of: bcdfpsFPT)\n", argv[1][1]);
	    exit(1);
	}			/* end switch argv[1][1] */
    }				/* end for */

    /* init ranks */
    rf = build_path(RANKS_FILE);
    init_data(rf);

    /* init trig tables */
    init_trig();

    if (team < 0 || team >= 6) {/* change 7/27/91 TC was 4 now 6 */
	if (debug)
	    fprintf(stderr, "Choosing random team.\n");
	team = lrand48() % 4;
    }

    pno = findrslot();		/* Robots are entitled to tester's slots. */
    me = &players[pno];
    me->p_no = pno;
    myship = &me->p_ship;
    mystats = &me->p_stats;
    lastm = mctl->mc_current;

    /*
       At this point we have memory set up.  If we aren't a fleet, we don't
       want to replace any other robots on this team, so we'll check the
       other players and get out if there are any on our team.
    */

    if (practice) {
	strcpy(pseudo, "Hozer");
    }
    else {
	strcpy(pseudo, rnames[team]);
    }
    strcpy(login, "Robot");

    (void) strncpy(me->p_login, login, sizeof(me->p_login));
    me->p_login[sizeof(me->p_login) - 1] = '\0';
    (void) strncpy(me->p_monitor, "Nowhere", sizeof(me->p_monitor));
    me->p_monitor[sizeof(me->p_monitor) - 1] = '\0';

    /* repeat "Nowhere" for completeness 4/13/92 TC */
    (void) strncpy(me->p_full_hostname, "Nowhere", sizeof(me->p_full_hostname));
    me->p_full_hostname[sizeof(me->p_full_hostname) - 1] = '\0';

    if (target >= 0) {		/* hack 7/27/91 TC */
	enter(team, target, pno, class, startplanet);
	startplanet = -1;	/* Termies should forget about startplanet */
    }
    else
	startplanet = enter(team, 0, pno, class, startplanet);

    if(configvals->robot_stats)
      do_robot_login();
    else
      me->p_pos = -1;		/* So robot stats don't get saved */

    me->p_flags |= PFROBOT;	/* Mark as a robot */
    if ((berserk) || (team == 4))	/* indeps are hostile */
	me->p_hostile = (FED | ROM | KLI | ORI);	/* unless they are
							   berserk */
    else if (team == 5)
	me->p_hostile = 0;	/* Termies declare war later */
    else if (practice)
	me->p_hostile = bteam;	/* or practice */
    else
	me->p_hostile = 0;	/* robots are peaceful */

    if (practice)
	me->p_flags |= PFPRACTR;/* Mark as a practice robot */

    r_signal(SIGALRM, rmove);
    config();

    if (practice) {
	udt.it_interval.tv_sec = 1;	/* Robots get to move 1/sec */
	udt.it_interval.tv_usec = 000000;
    }
    else {
	udt.it_interval.tv_sec = 0;	/* Robots get to move 2/sec */
	udt.it_interval.tv_usec = 500000;
    }

    udt.it_value.tv_sec = 1;
    udt.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &udt, 0) < 0) {
	perror("setitimer");
	me->p_status = PFREE;	/* Put robot in game */
	move_player(me->p_no, -1, -1, 1);
	exit(1);
    }

    /* allows robots to be forked by the daemon on some systems */
    {
      sigset_t unblock_everything;
      sigfillset(&unblock_everything);
      sigprocmask(SIG_UNBLOCK, &unblock_everything, NULL);
    }

    if (team == 4) {
	int     count = 0;
	for (i = 0; i < MAXPLAYER; i++) {
	    if (players[i].p_status == PALIVE)
		count++;
	}
	if (((count <= 1) && (!fleet)) ||
	    ((count < 1) && (fleet))) {	/* if one or less players, don't show */
	    if (debug)
		fprintf(stderr, "No one to hoze.\n");
	    players[pno].p_status = PFREE;
	    move_player(me->p_no, -1, -1, 1);
	    exit(1);
	}
    }
    if (!fleet) {
	for (i = 0; i < MAXPLAYER; i++) {
	    if ((players[i].p_status == PALIVE) 
		&& (players[i].p_team == bteam)) {
		if (debug)
		    fprintf(stderr, "Galaxy already defended\n");
		players[pno].p_status = PFREE;
		move_player(me->p_no, -1, -1, 1);
		exit(1);
	    }
	}
    }
    me->p_status = PALIVE;	/* Put robot in game */
    while (1) {
	pause();
    }
    return 0;
}
