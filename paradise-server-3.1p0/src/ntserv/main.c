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
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include "proto.h"
#include "ntserv.h"
#include "data.h"
#include "shmem.h"

int     startTkills, startTlosses, startTarms, startTplanets, startTticks;
char    start_login[16];	/* change 1/25/91 TC */
char    start_name[16];		/* change 1/25/91 TC */
int     goAway = 0;		/* change 4/14/91 TC */
int     ignored[MAXPLAYER];	/* change 7/24/91 TC */

int     overload = 0;		/* global 7/31/91 TC */
/* overload indicates a request for a reserved slot */

static int     indie = 0;	/* always be indie 8/28/91 TC */

static void 
atexitfunc(void)
{
    me->p_ntspid = 0;
    me->p_status = PFREE;
}

RETSIGTYPE
reaper(int unused)
{
#ifdef HAVE_WAIT3
    while (wait3((int *) 0, (int) WNOHANG, (struct rusage *) 0) > 0);
#else
    while (waitpid(0, NULL, WNOHANG) > 0);
#endif				/* SVR4 */
}

void
printStats(void)
{
    FILE   *logfile;
    time_t  curtime;
    char   *paths;		/* added 1/18/93 KAO */

    paths = build_path(LOGFILENAME);
    logfile = fopen(paths, "a");
    if (!logfile)
	return;
    curtime = time(NULL);

    fprintf(logfile, "Leaving: %-16s (%s) %3dP %3dA %3dW/%3dL %3dmin %drtt %dsdv %dls %dplan <%s@%s> %s",
	    me->p_name,
	    twoletters(me),
	    me->p_stats.st_tplanets - startTplanets,
	    me->p_stats.st_tarmsbomb - startTarms,
	    me->p_stats.st_tkills - startTkills,
	    me->p_stats.st_tlosses - startTlosses,
	    (me->p_stats.st_tticks - startTticks) / 600,
            me->p_avrt, me->p_stdv, me->p_pkls,
	    numPlanets(me->p_team),

	    me->p_login,
	    me->p_full_hostname,
	    ctime((time_t *) &curtime));

    if (goAway)
	fprintf(logfile, "^^^ 2 players/1 slot.  was %s (%s)\n",
		start_name, start_login);
    fclose(logfile);
}


/*
 * .pics file format:
 *
 * name
 * x y page
 * name
 * x y page
 * etc
 */

void 
doMotdPics(void)
{
    FILE   *ptr, *ftemp;
    char    buf[128], fname[128];
    unsigned char *bits;
    char   *result;
    int     x, y, page, w, h;
    int     bytesperline=0;	/* pad the width to a byte */
    int     linesperblock=0;	/* how many lines in 1016 bytes? */
    int     i;
    char   *paths;

    paths = build_path(PICS);
    ptr = fopen(paths, "r");
    if (ptr == NULL)
	return;
    while (1) {
	result = fgets(fname, 125, ptr);
	if (result == NULL)
	    /* must fclose ptr */
	    break;

	if (fname[strlen(fname) - 1] == '\n')
	    fname[strlen(fname) - 1] = 0;

	paths = build_path(fname);
	ftemp = fopen(paths, "r");
	bits = 0;
	if (ftemp == 0) {
	    fprintf(stderr, "ntserv: couldn't open file %s.  skipping\n", paths);
	}
	else {
	    ParseXbmFile(ftemp, &w, &h, &bits);	/* parsexbm.c */

	    bytesperline = (w - 1) / 8 + 1;
	    linesperblock = 1016 /* packets.h */ / bytesperline;
	}

	fgets(buf, 125, ptr);

	if (3 != sscanf(buf, "%d %d %d", &x, &y, &page)) {
	    printf("Format error in .pics file\n");
	    if (bits)
		free(bits);
	    bits = NULL;
	}

	if (bits) {
	    if (me!=0 && (me->p_stats.st_flags & ST_NOBITMAPS)) {
		sendMotdNopic(x, y, page, w, h);
	    }
	    else
		for (i = 0; i * linesperblock < h; i++) {
		    int     nlines;
		    if ((i + 1) * linesperblock > h)
			nlines = h - i * linesperblock;
		    else
			nlines = linesperblock;
		    sendMotdPic(x, y + i * linesperblock,
				bits + bytesperline * linesperblock * i,
				page, w, nlines);
		}
	    free(bits);
	}
    }
    fclose(ptr);
}

int 
main(int argc, char **argv)
{
    int     team, s_type;
    int     pno;
    int     usage = 0;   /* Flag saying tell usage */
    int     errorc = 0;  /* Error count */
    char   *errorv[5];   /* Error Vector (cannot have more than 5) */
    char   *name, *ptr;
    char   *rank_filename;
    int     callHost = 0;
    long    starttime;
    enum HomeAway homeaway = NEITHER;
    int     observer = 0;

    argv0 = argv[0];

    pno = time(NULL);


    /* read in ranks file */
    rank_filename = build_path(RANKS_FILE);
    init_data(rank_filename);

    /* load_time_acess - read in the hours file, Larry's. */

    load_time_access();

    name = *argv++;
    argc--;
    if ((ptr = strrchr(name, '/')) != NULL)
	name = ptr + 1;
    while (*argv) {
	if (**argv == '-')
	    ++* argv;
	else
	    break;

	argc--;
	ptr = *argv++;
	while (*ptr) {
	    switch (*ptr) {
	    case 'u': /* for old times sake */
            case '-': /* this will help the --help people */
            case 'h':
		usage++;
		break;
	    case 'i':
		indie++;
		break;		/* 8/28/91 TC */
	    case 'R':
		if (getuid() == geteuid())
		    overload++;
		break;
	    case 's':
		xtrekPort = atoi(*argv);
		callHost = 1;
		argv++;
		argc--;
		break;
	    case 'M':
		blk_metaserver = 1;
		break;
	    case 'd':
		host = *argv;
		argc--;
		argv++;
		break;
		/* netrek league stuff */
	    case 'O':
		observer = 1;
		break;
	    case 'H':
		homeaway = HOME;
		break;
	    case 'A':
		homeaway = AWAY;
		break;
	    default: {
		char buffer[100];
		sprintf(buffer, "Unknown option '%c'\n", *ptr);
		errorv[errorc++] = buffer;
		break;
	      }
	    }
            if (usage)
                break;
	    ptr++;
	}
    }

    if (usage || errorc) {
        int x;
        char message[][255] = {
            "\n\t'%s [options] -s <socket number> <display address>'\n\n",
            "Options:\n",
            "\t-h   Help (this usage message)\n",
            "\t-i   Team independant\n",
            "\t-R   Reserved slot\n",
            "\t-s   Socket number\n",
            "\t-M   Metaserver\n",
            "\t-d   Display\n",
            "\t-O   Observer\n",
            "\t-H   Home (League Play)\n",
            "\t-A   Away (League Play)\n",
            "\nNOTE: %s is designed to be launched by the startup process\n\n",
            "\0"
        };

        fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
        for (x=0; x < errorc; x++)
            fprintf(stderr, "\n%s: %s", argv0, errorv[x]);
        for (x=0; *message[x] != '\0'; x++)
            fprintf(stderr, message[x], argv0);

	exit(1);
    }

    /* init the trig tables */
    init_trig();

    openmem(1, homeaway != NEITHER);

    /* compatability */
    if (argc > 0)
	host = argv[0];
    srand48(getpid() + time((time_t *) 0));
    /* this finds the shared memory information */

#if 0
    if (blk_metaserver) {
	FILE   *ptr;
	char   *buf;
	buf = build_path("logs/metaserver.log");
	ptr = fopen(buf, "a");
	fprintf(ptr, "Connection from meta-server\n");
	fclose(ptr);
    }
#endif


    me = NULL;			/* UDP fix (?) */
    if (callHost) {
	if (!connectToClient(host, xtrekPort)) {
	    exit(0);
	}
    }
    else {
	sock = 0;		/* Because we were forked by inetd! */
	checkSocket();
	initClientData();	/* "normally" called by connectToClient() */
    }

    starttime = time(NULL);
    while (userVersion == 0) {
	/*
	   Waiting for user to send his version number. We give him ten
	   seconds to do so...
	*/
	if (starttime + 10 < time(NULL)) {
	    exit(1);
	}
	socketPause();
	readFromClient();
    }
    if (!checkVersion())
	exit(1);

    pno = findslot(overload, homeaway);
    if (pno < 0) {
	/* print some appropriate message */
	exit(1);
    }

    atexit(atexitfunc);		/* register a function to execute at exit */

    me = &players[pno];
    me->p_no = pno;
    galaxyValid[ me->p_no ] = 0;
    me->p_team = NOBODY;
    me->p_stats.st_royal = 0;
    me->gen_distress = 0;	/* default to RCD off */
    me->p_ntspid = getpid();
    myship = &me->p_ship;
    mystats = &me->p_stats;
    lastm = mctl->mc_current;
    me->p_lastrefit = -1;
    me->p_spyable = 1;
    me->p_teamspy = ~0;

/* --------------------------[ CLUECHECK stuff ]-------------------------- */
#ifdef CLUECHECK1
    me->p_cluedelay = 10;
    me->p_cluecountdown = 0;
#endif

#ifdef CLUECHECK2
    me->p_cluedelay = lrand48()%1000;  /* so it doesn't ask them immediately */
    me->p_cluecountdown = 0;
#endif
/* ----------------------------------------------------------------------- */

#ifndef AUTHORIZE
    strcpy(RSA_client_type, "server doesn't support RSA");
#endif

    (void) r_signal(SIGINT, SIG_IGN);
    (void) r_signal(SIGCHLD, reaper);

    /*
       We set these so we won't bother updating him on the location of the
       other players in the galaxy which he is not near.  There is no real
       harm to doing this, except that he would then get more information
       than he deserves. It is kind of a hack, but should be harmless.
    */
    me->p_x = -100000;
    me->p_y = -100000;
    me->p_homeaway = homeaway;
    me->p_observer = observer;

    updateSelf();		/* so he gets info on who he is */
    updateShips();		/* put this back so maybe something will work */
    /* with Andy's meta-server */

    if (!blk_metaserver) {
	updateStatus();
	updatePlanets();
        updateTerrain();
    }
    updateGameparams();

    flushSockBuf();

    /* Get login name */

    (void) strncpy(login, "Bozo", sizeof(login));
    login[sizeof(login) - 1] = '\0';

    strcpy(pseudo, "Guest");

    strcpy(me->p_name, pseudo);
    me->p_team = ALLTEAM;
    getname();
    if (me->p_stats.st_rank >= NUMRANKS)
      me->p_stats.st_rank = NUMRANKS-1;
    if (me->p_stats.st_royal >= NUMROYALRANKS)
      me->p_stats.st_royal = NUMROYALRANKS-1; 
    strcpy(pseudo, me->p_name);
    strcpy(start_name, me->p_name);	/* change 1/25/91 TC */

    sendShipCap();		/* KAO 1/25/93 */

    keeppeace = (me->p_stats.st_flags & ST_KEEPPEACE) == ST_KEEPPEACE;

    /*
       Set p_hostile to hostile, so if keeppeace is on, the guy starts off
       hating everyone (like a good fighter should)
    */
    me->p_hostile = (FED | ROM | KLI | ORI);
    s_type = CRUISER;
    me->p_planets = 0;
    me->p_armsbomb = 0;
    me->p_dooshes = 0;
    me->p_resbomb = 0;
    /* Set up a reasonable default */
    me->p_whydead = KQUIT;

    (void) strncpy(me->p_login, login, sizeof(me->p_login));
    me->p_login[sizeof(me->p_login) - 1] = '\0';
    strcpy(start_login, login);	/* change 1/25/91 TC */
    {
	int     i;
	for (i = 0; i < MAXPLAYER; i++)
	    ignored[i] = 0;
    }

    (void) strncpy(me->p_monitor, host, sizeof(me->p_monitor));
    me->p_monitor[sizeof(me->p_monitor) - 1] = '\0';

    /* assume this is only place p_monitor is set, and mirror accordingly */
    /* 4/13/92 TC */
    (void) strncpy(me->p_full_hostname, host, sizeof(me->p_full_hostname));
    me->p_full_hostname[sizeof(me->p_full_hostname) - 1] = '\0';

    logEntry();			/* moved down to get login/monitor 2/12/92
				   TMC */

    me->p_avrt = -1;		/* ping stats */
    me->p_stdv = -1;
    me->p_pkls = -1;

    startTkills = me->p_stats.st_tkills;
    startTlosses = me->p_stats.st_tlosses;
    startTarms = me->p_stats.st_tarmsbomb;
    startTplanets = me->p_stats.st_tplanets;
    startTticks = me->p_stats.st_tticks;

    r_signal(SIGHUP, exitGame);	/* allows use of HUP to force a clean exit */



    me->p_status = POUTFIT;
    repCount = 0;

    while (1) {
	switch (me->p_status) {
	case POUTFIT:
	case PTQUEUE:
	    /* give the player the motd and find out which team he wants */
	    if (me->p_status != PALIVE) {
		me->p_x = -100000;
		me->p_y = -100000;
		updateSelf();
		updateShips();
		teamPick = -1;
		flushSockBuf();
		getEntry(&team, &s_type);
	    }
	    if (goAway) {	/* change 4/14/91 TC */
		printStats();
		exit(0);
	    }
	    if (team == -1) {
		exitGame();
	    }

	    if (indie)
		team = 4;	/* force to independent 8/28/91 TC */
	    inputMask = -1;	/* Allow all input now */
	    enter(team, 0, pno, s_type, -1);
	    /* for (i = 0; i < NSIG; i++) { r_signal(i, SIG_IGN); } */

	    me->p_status = me->p_observer ? POBSERVE : PALIVE;	/* Put player in game */
	    me->p_ghostbuster = 0;
	    break;
	case PALIVE:
	case PEXPLODE:
	case PDEAD:
	case POBSERVE:
	    /* Get input until the player quits or dies */


	    input();
	    break;
	default:
	    if (tmpPick != PATROL) {
		printf("player status = %d.  exiting\n", me->p_status);
		exitGame();
	    }
	}
    }

    /* NOTREACHED */
    return 1;
}

int     interrupting = 0;

void 
stop_interruptor(void)
{
    struct itimerval udt;

    if (!interrupting)
	return;

    r_signal(SIGALRM, SIG_IGN);	/* set up signals */
    udt.it_interval.tv_sec = 0;
    udt.it_interval.tv_usec = 0;
    udt.it_value.tv_sec = 0;
    udt.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &udt, 0);

    interrupting = 0;
}

void 
start_interruptor(void)
{
    struct itimerval udt;

    if (interrupting)
	return;

    {
	int     min_delay = me->p_observer
	? configvals->min_observer_upd_delay
	: configvals->min_upd_delay;

	if (timerDelay < min_delay)
	    timerDelay = min_delay;
    }

    r_signal(SIGALRM, SIG_IGN);	/* set up signals */
    udt.it_interval.tv_sec = 0;
    udt.it_interval.tv_usec = timerDelay;
    udt.it_value.tv_sec = 0;
    udt.it_value.tv_usec = timerDelay;
    setitimer(ITIMER_REAL, &udt, 0);

    r_signal(SIGALRM, setflag);

    interrupting = 1;
}

void 
exitGame(void)
{
    char    buf[80];
    char    addrbuf[20];

    if (me != NULL && me->p_team != ALLTEAM) {
	sprintf(buf, "%s %s (%s) leaving game (%.16s@%.32s)",
		((me->p_stats.st_royal) ? royal[me->p_stats.st_royal].name
		 : ranks[me->p_stats.st_rank].name),
		me->p_name,
		twoletters(me),
		me->p_login,
		me->p_full_hostname
	    );
	sprintf(addrbuf, " %s->ALL", twoletters(me));
	pmessage2(buf, 0, MALL | MLEAVE, addrbuf, me->p_no);
	me->p_stats.st_flags &= ~ST_CYBORG;	/* clear this flag 8/27/91 TC */
	savestats();
	printStats();
    }
    me->p_status = PFREE;
    move_player(me->p_no, -1, -1, 1);
    exit(0);
}

#define	PLURAL(n)	(((n)==1)?"":"s")

static char *weapon_types[WP_MAX] = {
    "Plasma torpedos",
    "Tractors",
    "Missiles",
    "Fighters",
};

static void 
sendSysDefs(void)
{
    char    buf[200], buf2[200];
    int     i;

    sendMotdLine("\t@@@");

    if (!time_access()) {
        sendMotdLine("** WE ARE CLOSED, CHECK HOURS **");
        sendMotdLine("");
    }

    sendMotdLine("Available ship types:");
    buf[0] = 0;
    for (i = 0; i < NUM_TYPES; i++) {
	struct ship *s = &shipvals[i];
	if (!shipsallowed[i])
	    continue;
	sprintf(buf2, "   %c) %s/%c%c", s->s_letter, s->s_name,
		s->s_desig1, s->s_desig2);
	if (strlen(buf) + strlen(buf2) > 80) {
	    sendMotdLine(buf);
	    strcpy(buf, buf2);
	}
	else {
	    strcat(buf, buf2);
	}
    }
    /* guaranteed to have stuff here */
    sendMotdLine(buf);
    sendMotdLine("");
    sendMotdLine(
		 "SHIP         REQUIRED RANK   BUILD TIME    LIMIT     NUM PLYRS  NUM PLNTS");

    for (i = 0; i < NUM_TYPES; i++) {
	struct ship *s = &shipvals[i];
	if (!shipsallowed[i])
	    continue;
	buf2[0] = 0;
	if (s->s_rank > 0 || s->s_timer > 0 || s->s_maxnum < 16 ||
	    s->s_numdefn || s->s_numplan) {
	    sprintf(buf2, "%-13s%-16s", s->s_name,
		    s->s_rank ? ranks[s->s_rank].name : "none");
	    if (s->s_timer > 0)
		sprintf(buf, "%d minutes", s->s_timer);
	    else
		strcpy(buf, "none");
	    sprintf(buf2 + strlen(buf2), "%-14s", buf);

	    if (s->s_maxnum < 16)
		sprintf(buf, "%d/team", s->s_maxnum);
	    else
		strcpy(buf, "none");
	    sprintf(buf2 + strlen(buf2), "%-12s  %-3d        %-3d", buf,
		    s->s_numdefn, s->s_numplan);
	    sendMotdLine(buf2);
	}
    }
    sendMotdLine("");


    buf2[0] = 0;
    for (i = 0; i < WP_MAX; i++) {
	if (weaponsallowed[i]) {
	    if (buf2[0])
		strcat(buf2, ", ");
	    strcat(buf2, weapon_types[i]);
	}
    }
    sprintf(buf, "Special weapons enabled: %s", buf2[0] ? buf2 : "none");
    sendMotdLine(buf);

    if (weaponsallowed[WP_PLASMA]) {
	sprintf(buf, "You need %.1f kill%s to get plasma torpedos",
		configvals->plkills, PLURAL(configvals->plkills));
	sendMotdLine(buf);
    }
    if (weaponsallowed[WP_MISSILE]) {
	sprintf(buf, "You need %.1f kill%s to get missiles",
		configvals->mskills, PLURAL(configvals->mskills));
	sendMotdLine(buf);
    }
    sendMotdLine("");

    sprintf(buf, "Tournament mode requires %d player%s per team",
	    configvals->tournplayers, PLURAL(configvals->tournplayers));
    sendMotdLine(buf);

    /* We don't blab about newturn */
    sendMotdLine(configvals->binconfirm ?
		 "Only authorized binaries are allowed" :
		 "Non-authorized binaries are detected but not rejected");

    if(configvals->planetlimittype) {
	sprintf(buf, "Independent planets may be taken if your team has fewer than %d planets", configvals->planetsinplay);
	sendMotdLine(buf);
    } else {
	sprintf(buf, "Only %d planets can be in play at once",
		configvals->planetsinplay);
	sendMotdLine(buf);
    }

    if (configvals->planupdspd == 0) {
	sendMotdLine("Planets do not orbit their stars");
    }
    else {
	sprintf(buf, "Planets orbit their stars at a rate of %g",
		configvals->planupdspd);
	sendMotdLine(buf);
    }
    sendMotdLine(configvals->warpdecel ?
		 "New warp deceleration code is in effect" :
		 "Old-style instant warp deceleration is in effect");
    sprintf(buf, "The next galaxy will be generated using method #%d",
	    configvals->galaxygenerator);
    switch (configvals->galaxygenerator) {
        case 1:
            strcat(buf, " (original)");
            break;
        case 2:
            strcat(buf, " (compact galaxy)");
            break;
        case 3:
            strcat(buf, " (compact, 2 team)");
            break;
        case 4:
            strcat(buf, " (bronco emulation)");
            break;
        case 5:
            strcat(buf, " (smaller)");
            break;
        case 6:
            strcat(buf, " (compact, 2 systems, 2 teams)");
            break;
        case 7:
            strcat(buf, " (deepspace galaxy)");
            break;
        case 8:
            strcat(buf, " (bronco style)");
            break;
	case 9:
	    strcat(buf, " (balanced advance)");
	    break;
        default:
            strcat(buf, " (unknown??)");
            break;
    }
    sendMotdLine(buf);
    sendMotdLine
	(configvals->affect_shiptimers_outside_T ?
	 "Ship deaths outside tournament mode affect construction timers" :
	 "Construction timers are not affected by ship deaths outside tournament mode");

    if (configvals->cloakduringwarpprep || configvals->cloakwhilewarping)
    {
      sprintf(buf, "Cloaking during%s%s%s is allowed",
              (configvals->cloakwhilewarping ? " warp" : ""),
	      (configvals->cloakwhilewarping && configvals->cloakduringwarpprep?
	       " and" : ""),
	      (configvals->cloakduringwarpprep ? " warp prep" : ""));
      sendMotdLine(buf);
    }

    sprintf(buf, "Variable warp speed is %sabled",
	    configvals->variable_warp ? "en" : "dis");
    sendMotdLine(buf);

    sprintf(buf, "Warp prep suspension is %sallowed",
	    configvals->warpprep_suspendable ? "" : "not ");
    sendMotdLine(buf);

    switch (configvals->warpprepstyle) {
    case WPS_NOTRACT:
	sendMotdLine("Tractors do not affect warp prep");
	break;
    case WPS_TABORT:
	sendMotdLine("Tractors make warp fail to engage");
	break;
    case WPS_TPREVENT:
	sendMotdLine("Tractors prevent entering warp");
	break;
    case WPS_TABORTNOW:
	sendMotdLine("Tractors abort warp prep countdown");
	break;
    case WPS_TSUSPEND:
	sendMotdLine("Tractors suspend warp prep countdown");
	break;
    }

    sprintf(buf, "There is a %d%% chance that you'll orbit a planet CCW",
	    (int)((1.0 - configvals->orbitdirprob) * 100));
    sendMotdLine(buf);

    sprintf(buf, "Army growth: %d.  Pop choice: %d.  Pop speed: %d%%.",
	    configvals->popscheme,configvals->popchoice,configvals->popspeed);
    sendMotdLine(buf);

    if(configvals->warpzone) {
        sprintf(buf,"Warp zones are enabled with radius %d.",
		configvals->warpzone);
    } else {
        sprintf(buf,"Warp zones are disabled.");
    }
    sendMotdLine(buf);

    if(!(configvals->repair_during_warp_prep) || 
       !(configvals->repair_during_warp))
    {
      sprintf(buf, "Repair is NOT allowed during%s%s%s.",
              (!configvals->repair_during_warp ? " warp" : ""),
	      (!configvals->repair_during_warp && 
	       !configvals->repair_during_warp_prep ? " and" : ""),
	      (!configvals->repair_during_warp_prep ? " warp prep" : ""));
      sendMotdLine(buf);
    }

    sprintf(buf, "WB bombing credit bonus %sabled.  JS assist credit %sabled.",
            (configvals->wb_bombing_credit ? "en" : "dis"),
	    (configvals->js_assist_credit ? "en" : "dis"));
    sendMotdLine(buf);

    sprintf(buf, "Surrender start: %d planets; sustain: %d planets; "
                 "timer: %d min.",
           configvals->surrstart, configvals->surrend,
	   configvals->surrlength);
    sendMotdLine(buf);

    sprintf(buf, "Genocide: %d planets.", configvals->victory_planets);
    sendMotdLine(buf);

    sprintf(buf, "Penetration: %d%%.  Erosion: %d%%.  Army penetration: %d%%",
	    (int)(configvals->penetration * 100.0),
	    (int)(configvals->erosion * 100.0),
	    (int)(configvals->kill_carried_armies * 100.0));
    sendMotdLine(buf);

    sprintf(buf, "Planets with built facilities do%s revolt.",
           (configvals->revolt_with_facilities ? "" : " not"));
    sendMotdLine(buf);

    if(configvals->shipyard_built_by_sb_only)
    {
      sprintf(buf, "Shipyards can only be built by Starbases.");
      sendMotdLine(buf);
    }
    if(configvals->can_bomb_own_shipyard)
    {
      sprintf(buf, "Players can bomb their own shipyard.");
      sendMotdLine(buf);
    }
}

void 
sendMotd(void)
{
    FILE   *motd;
    char    buf[100], buf2[30];	/* big enough... */
    char   *paths;

    time_t  curtime;
    struct tm *tmstruct;
    int     hour/* , tacc */;

    time(&curtime);
    tmstruct = localtime(&curtime);
    if (!(hour = tmstruct->tm_hour % 12))
	hour = 12;
    sprintf(buf, "Netrek Paradise II (3.1p1), connection established at %d:%02d%s.",
            hour,
	    tmstruct->tm_min,
	    tmstruct->tm_hour >= 12 ? "pm" : "am");

/*    if (!(tacc = time_access()))
	strcat(buf, "  WE'RE CLOSED, CHECK HOURS");*/
    sendMotdLine(buf);
    sendMotdLine(" ");

    if (!blk_flag) {
	paths = build_path(WCMOTD);	/* Wrong client message */
	if ((motd = fopen(paths, "r")) != NULL) {
	    while (fgets(buf, sizeof(buf), motd) != NULL) {
		buf[strlen(buf) - 1] = '\0';
		sendMotdLine(buf);
	    }
	    fclose(motd);
	}
	else {			/* default message */
	    sendMotdLine(" ");
	    sendMotdLine(
			 "      ****************************************************************");
	    sendMotdLine(" ");

	    sendMotdLine(
			 "       This is a Paradise server; you need a Paradise client to play!");
	    sendMotdLine(" ");
	    sendMotdLine(
			 "       Paradise clients can be had from");
	    sendMotdLine(
		       "          ftp.cis.ufl.edu    pub/netrek.paradise/");
	    sendMotdLine(
		   "          ftp.reed.edu       mirrors/netrek.paradise/");
	    sendMotdLine(" ");
	    sendMotdLine(
			 "      ****************************************************************");
	}
	return;
    }

     /* if (blk_flag) */ {	/* added 1/19/93 KAO */
	int     i, first = 1;

	strcpy(buf, "BLK: REFIT ");

	for (i = 0; i < NUM_TYPES; i++) {
	    struct ship *s = &shipvals[i];
	    if (!shipsallowed[i])
		continue;

	    if (!first)
		strcat(buf, ", ");
	    else
		first = 0;

	    sprintf(buf2, "%c) %c%c", s->s_letter, s->s_desig1, s->s_desig2);
	    strcat(buf, buf2);
	}
	sendMotdLine(buf);
    }
    /* the following will read a motd */
    if(!time_access()) {
	paths = build_path(CLOSEDMOTD);
	if((motd = fopen(paths, "r")) == NULL) {
	    paths = build_path(MOTD);
	    motd = fopen(paths, "r");
	}
    } else {
	paths = build_path(MOTD);
	motd = fopen(paths, "r");
    }
    if(motd != NULL) {
#ifdef CLUECHECK1
	init_motdbuf(paths);
#endif
	while (fgets(buf, sizeof(buf), motd) != NULL) {
	    buf[strlen(buf) - 1] = '\0';
	    sendMotdLine(buf);
	}
	(void) fclose(motd);
    }
    sendSysDefs();

    /* wait till the end for the pictures */
    if(!blk_metaserver)
	doMotdPics();
}
