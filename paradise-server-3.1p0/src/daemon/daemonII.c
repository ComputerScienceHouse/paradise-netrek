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
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "proto.h"
#include "daemonII.h"
#include "data.h"
#include "shmem.h"

#define TellERR(x)     fprintf(stderr, "!  %s: %s\n", argv0, x)
#define TellERRf(x, y) { \
                         sprintf(buf, x, y); \
                         fprintf(stderr, "!  %s: %s\n", argv0, buf); \
                       }

/*---------------------------GLOBAL VARIABLES-----------------------------*/

/* globals that are declared here */
int     dietime = -1;		/* to decide whether the deamon has been */
                                /* inactive for one minute.  Set to -1 so */
                                /* the deamon will not immediately quit */

int     ticks = 0;		/* counting ticks for game timing */

int     plfd;			/* for the planet file */
int     glfd;			/* for the status file */

int     tourntimestamp = 0;	/* ticks since t-mode started */

/*---------------------------LOCAL VARIABLES------------------------------*/

static jmp_buf env;			/* to hold long jump back into main */

static int debug = 0;		/* set if an arg is passed to main on the
				   command line, this var gets set to 1 and
				   debuf info is printed.  */

static int doMove;		/* indicates whether it's time to call move() */

static int tm_robots[MAXTEAM + 1];	/* To limit the number of robots */

/*---------------------------LOCAL FUNCTIONS------------------------------*/

static void
teamtimers(void)
{
    register int i;
    for (i = 0; i <= MAXTEAM; i++) {
	if (tm_robots[i] > 0)
	    tm_robots[i]--;
    }
}

static void
shipbuild_timers(void)
{
    int     i, t;

    for (i = 0; i <= MAXTEAM; i++)	/* go through all teams */
	for (t = 0; t < NUM_TYPES; t++)	/* and each ship type */
	    if (teams[i].s_turns[t] > 0)	/* and if need be, then dec */
		teams[i].s_turns[t]--;	/* the construction timer */
}

/* signal handler for SIGALRM */
static RETSIGTYPE
setflag(int unused)
{
    doMove = 1;
}

static RETSIGTYPE
freemem(int sig)
{
    register int i;
    register struct player *j;

    if (sig) {
	fprintf(stderr, "Daemon: Caught signal %d\n", sig);
	/* U_STACK_TRACE(); */
    }

    /* Blow players out of the game */
    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	j->p_status = POUTFIT;
	j->p_whydead = KDAEMON;
	j->p_ntorp = 0;
	j->p_nplasmatorp = 0;
	j->p_explode = 600 / PLAYERFUSE;	/* ghost buster was leaving
						   players in */
    }
    /* Kill waiting players */
    status->gameup = 0;		/* say goodbye to xsg et. al. 4/10/92 TC */
    status->count = 0;
    save_planets();
    sleep(2);
    blast_shmem();
    exit(0);
}

/* Don't fear the ... */

static RETSIGTYPE
reaper(int unused)
{
    static int status;
    static int pid;

#ifdef HAVE_WAIT3
    while ((pid = wait3((int *) & status, WNOHANG, (struct rusage *) 0)) > 0) {
#else				/* note: no status info */
    while ((pid = waitpid(0, 0, WNOHANG)) > 0) {
#endif				/* SVR4 */
	if (debug) {
	    fprintf(stderr, "Reaping: pid is %d (status: %X)\n",
		    pid, status);
	}
    }
}

/*---------------------[ prints the usage of daemonII ]---------------------*/

static void
printdaemonIIUsage(char *myname)
{
    int x;
    char message[][255] = {
        "\n\t'%s [options]'\n\n",
        "Options:\n",
        "\t-h   help (this usage message)\n",
        "\t-l   configures as a League server (usually run by listen)\n",
        "\t-d   debug\n",
        "\t-a   attach to a crashed daemon's memory segment\n",
        "\nNOTE: %s is designed to be launched by the startup process.\n\n",
        "\0"
    };

    fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
    for (x=0; *message[x] != '\0'; x++)
        fprintf(stderr, message[x], myname);

    exit(1);
}

static void
check_load(void)
{
#ifdef HAVE_UPTIME
    FILE   *fp;
    char    buf[100];
    char   *s;
    float   load;

    fp = popen(UPTIME_PATH, "r");
    if (fp == NULL) {
/*	status->gameup=0;*/
	return;
    }
    fgets(buf, 99, fp);
    s = strrchr(buf, ':');
    if (s == NULL) {
/*	status->gameup=0;*/
	pclose(fp);
	return;
    }
    if (sscanf(s + 1, " %f", &load) == 1) {
	sprintf(buf, "NetrekII (Paradise), %s", PARAVERS);
	pmessage(buf, 0, MALL, MSERVA);
	if (load >= configvals->maxload && status->gameup == 1) {
	    status->gameup = 0;
	    sprintf(buf, "The load is %f, this game is going down", load);
	    pmessage(buf, 0, MALL, MSERVA);
	}
	else if (load < configvals->maxload && status->gameup == 0) {
	    status->gameup = 1;
	    sprintf(buf, "The load is %f, this game is coming up", load);
	    pmessage(buf, 0, MALL, MSERVA);
	}
	else {
	    sprintf(buf, "Load check: %-7.2f", load);
	    buf[strlen(buf) - 1] = '\0';
	    pmessage(buf, 0, MALL, MSERVA);
	}
    }
    else {
/*	status->gameup=0;*/
    }
    r_signal(SIGCHLD, SIG_DFL);
    pclose(fp);
    r_signal(SIGCHLD, reaper);
#endif
}

static void 
handle_pause_goop(void)
{
    if (status2->paused) {
	if (!status2->home.desirepause && !status2->away.desirepause) {
	    /* countdown to game resumption */
	    status2->paused--;
	    if (status2->paused) {
		if (status2->paused % TICKSPERSEC == 0) {
		    char    buf[80];
		    sprintf(buf, "Game will resume in %d seconds",
			    status2->paused / TICKSPERSEC);
		    pmessage(buf, -1, MALL, UMPIRE);
		}
	    }
	    else {
		pmessage("Let the carnage resume!", -1, MALL, UMPIRE);
	    }
	}
	else {
	    status2->pausemsgfuse++;
	    if (status2->pausemsgfuse > SECONDS(15)) {
		status2->pausemsgfuse = 0;
		pmessage("Game is PAUSEd.  Captains `LEAGUE CONTINUE' to resume play.",
			 -1, MALL, UMPIRE);
		if (!status2->home.desirepause)
		    pmessage("The home team wishes to CONTINUE the game.",
			     -1, MALL, UMPIRE);
		if (!status2->away.desirepause)
		    pmessage("The away team wishes to CONTINUE the game.",
			     -1, MALL, UMPIRE);
	    }
	}
    }
    else {
	if (!status2->home.desirepause && !status2->away.desirepause)
	    return;

	status2->pausemsgfuse++;
	if (status2->pausemsgfuse > SECONDS(15)) {
	    char    buf[80];
	    status2->pausemsgfuse = 0;
	    sprintf(buf, "The %s team wishes to PAUSE the game!",
		    status2->home.desirepause ? "home" : "away");
	    pmessage(buf, -1, MALL, UMPIRE);
	}
    }
}


/*---------------------------------MOVE-----------------------------------*/
/*  This is the main loop for the program.  It is called every 1/10th of
a second.  It decides which  functions of the deamon need to be run.  */

static void
move(void)
{
    static int oldtourn = 0;	/* are we in t-mode or not */
    int     i, j;		/* looping vars */
    struct planet *pl;

    if (++ticks == dietime) {	/* no player for 1 minute. kill self */
	if (debug)		/* do not quit if debug mode */
	    fprintf(stderr, "Ho hum.  1 minute, no activity...\n");
	else {			/* quit if not debug mode */
	    fprintf(stderr, "Self-destructing the daemon!\n");
	    freemem(0);
	}
    }

    if ((FUSE(300)) && update_sys_defaults())	/* check to load system
						   defualts */
	/* This message tells players that new defaults have been */
	/* loaded and the message triggers the ntserv processes */
	/* to check new defaults.  */
	pmessage("Loading new server configuration.", 0, MALL, MSERVA);

    if(FUSE(SECONDS(1))) {
	if (tournamentMode()) {	/* are we in tournament mode */
	    if (!oldtourn) {	/* is this a new condition */
		if(!status2->starttourn) {	/* fresh t-mode */
		    if(configvals->gamestartnuke)
			explode_everyone(KTOURNSTART, 20);
		}
		status2->starttourn = configvals->nottimeout ? configvals->nottimeout : -1;
		warmessage();	/* go print war message */
		for (i = 0, pl = &planets[i]; i < NUMPLANETS; i++, pl++)
		    for (j = 0; j < MAXTEAM + 1; j++)
			pl->pl_tinfo[j].timestamp = 0;
		status->clock = 0;
		tourntimestamp = ticks;	/* take a timestamp */
	    }
	    oldtourn = 1;		/* record that we have printed warmsg */
	    status->tourn = 1;	/* set the game status to t-mode */
	    status->time++;		/* inc time in t-mode */
	}
	else {			/* else we are not in t-mode */
	    if (oldtourn) {		/* if previously in t-mode */
		tourntimestamp = ticks;	/* record t-mode ending */
		peacemessage();	/* send peace message */
	    } else {
		static int fuse=0;
		fuse++;
		if(fuse>60 && status2->starttourn > 0) {
		    fuse = 0;
		    status2->starttourn--;
		    switch(status2->starttourn) {
		    case 0:
			status2->newgalaxy = 1;
			break;
		    case 1:
		    case 3:
		    case 5:
		    case 15:
			{
			    static char	buf[120];
			    sprintf(buf, "Warning!!  Galaxy will be reset in %d minute%s due to inactivity.", status2->starttourn, (status2->starttourn==1)?"":"s");
			    pmessage(buf, 0, MALL, MSERVA);
			}
			break;
			pmessage("Warning!!  Galaxy will be reset in one minute due to inactivity.", 0, MALL, MSERVA);
			break;
		    }
		}
	    }
	    oldtourn = 0;		/* set we are not in t-mode */
	    status->tourn = 0;	/* record in stats */
	}
    }

    parse_godmessages();	/* log any messages to god */

    handle_pause_goop();	/* print any messages related to pausing the
				   game */

    if (!status2->paused)
      {
	if (FUSE(PLAYERFUSE))	/* time to update players? */
	    udplayers();

	if (FUSE(TORPFUSE))	/* time to update torps? */
	    udtorps();
	if (FUSE(MISSILEFUSE))	/* time to update missiles? */
	    udmissiles();
	if (FUSE(PLASMAFUSE))	/* time to update plasma? */
	    udplasmatorps();
	if (FUSE(PHASERFUSE))	/* time to update phasers? */
	    udphaser();


	if (FUSE(CLOAKFUSE))	/* time to update cloaking? */
	    udcloak();

	if (FUSE(TEAMFUSE))	/* time to update team timers? */
	    teamtimers();

	if (FUSE(PLFIGHTFUSE))	/* time to update planets? */
	    plfight();

	if (FUSE(TERRAINFUSE))	/* time to do terrain effects? */
	    doTerrainEffects();

	if (FUSE(BEAMFUSE))	/* time to update beaming */
	    beam();

	if (FUSE(SYNCFUSE))	/* time to save planets? */
	    save_planets();


	if (FUSE(topgun ? HOSEFUSE2 : HOSEFUSE)
	    && status->tourn != 1	/* no Iggy during T-mode */
	    && status2->league == 0
	    )	/* no Iggy during league games */
	    rescue(HUNTERKILLER, 0, -1);	/* send in iggy-- no team, no
						   target */

	if (status->tourn) {
	    {
		static int spinner = 0;

		for (spinner += configvals->popspeed; spinner >= 100; spinner -= 100)
		    popplanets(); /* increase population */
	    }

	    if (FUSE(PLANETFUSE)) /* time to grow resources */
		growplanets();

	    {
		/* check for revolts.  Each planet is checked on average
		   once every PLANETFUSE. */
		static int spinner = 0;
		for (spinner += configvals->numplanets;
		     spinner >= PLANETFUSE;
		     spinner -= PLANETFUSE) {
		    check_revolt();
		}
	    }
	}
	/* planet moving */
	if (configvals->planupdspd > 0 && FUSE(4))
	    moveplanets();

	if (FUSE(MINUTEFUSE) && status->tourn) {

	    shipbuild_timers();

	    if (!status2->league)
		udsurrend();	/* update surrender every minute unless
				   playing league */

	    status->clock++;	/* increment the timestamp clock */

	}
	/* update the tournament clock, maybe print messages */
	udtourny();
    }				/* end if !paused */

    if (FUSE(MINUTEFUSE)) {
	int     i, c;
	c = 0;
	for (i = 0; i < MAXPLAYER; i++) {
	    if (players[i].p_status != PFREE)
		c++;
	}
#ifdef COUNTFILENAME
	if (c) {
	    char   *paths;
	    FILE   *logfile;
	    paths = build_path(COUNTFILENAME);
	    logfile = fopen(paths, "a");
	    if (logfile) {
		struct tm *tp;
		char    buf[50];
		time_t  cal;

		cal = time(0);
		tp = localtime(&cal);
		sprintf(buf, "%02d/%02d %02d:%02d", 
		        tp->tm_mon, tp->tm_mday, tm->tm_hour, tm->tm_min);
	        /* was: 
		strftime(buf, 50, "%m/%d %H:%M", tp);*/

		fprintf(logfile, "%s : %2d ", buf, c);
		for (i = 0; i < c; i++) {
		    putc('*', logfile);
		}
		putc('\n', logfile);
		fclose(logfile);
	    }
	}
#endif
    }

    if (status2->newgalaxy) {

	/* Disable the game timer. It'll be set again after the longjmp() */
	stoptimer();

	status2->nontteamlock = ALLTEAM;/* allow all teams again */
	status2->starttourn = 0;	/* fresh galaxy */

	gen_planets();
        
	for( i = 0; i < MAXPLAYER; i++ ){
          galaxyValid[i] = 0;	/* force download of new galaxy map */
	}
	longjmp(env, 0);
    }
}


/*----------------------------------MAIN-----------------------------------*/
/*  Well, this is it.  The big Kahuna.  The main function of daemonII.  If
an arg is passed to main, then debug info is printed.  What is passed in
does not matter.  Personally, I am fond of running it with: daemonII fungus.
But maybe that's just me.
  Important:  An environment variable is read in from the user.  It is
called NETREKDIR.  This variable needs to be a path to where the  '.' (dot)
files are to be found.  Kurt added this to make things a hell of a lot
easier.  */

int
main(int argc, char **argv)
{
    register int i;		/* looping var */
    int     jjk;		/* looping var */
    char    buf[255];           /* Temp buffer */
    char   *paths;		/* to form path with */
    char   *ptr;		/* get get path env var */

    int     x = 0;		/* for delay in debugging messages */
    int     configleague = 0;	/* if nonzero, set configvals->league to 1 */
    int     attach = 0;
    int     nogo = 0;		/* for the usage flag/case */

    argv0 = argv[0];

    i = 1;
    while (argv[i]) {
	if (argv[i][0] == '-') {
	    ptr = &argv[i][1];
	    while (*ptr) {
		switch (*ptr) {
		case 'l':
		    configleague = 1;
		    break;
		case 'd':
		    debug = 1;
		    break;
		case 'a':
		    attach = 1;
		    break;
                case 'h':
                case 'u': /* for old times sake */
                case '-': /* this allows for --help people */
		    nogo++;
		    break;
                case 'v': /* version, what the hell */
                    fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
                    exit(0);
                    break;
		default:
                    TellERRf("Unknown flag '%c'.", *ptr);
                    nogo++;
/*		    fprintf(stderr, "Unknown flag '%c'\n", *ptr); */
		    break;
		}
		ptr++;
	    }
	}
        else {
            TellERRf("Invalid option format '%s'.", argv[i]);
            nogo++;
            break;
        }
	i++;
    }

    if (nogo) {
	printdaemonIIUsage(argv0);
    } else {
	/* log the PID */
        char *fname;
        FILE *fptr;

        fname = build_path("logs/daemonII.pid");
        if((fptr = fopen(fname, "w+")))
	{
          fprintf(fptr, "%d", getpid());
          fclose(fptr);
	}
    }

    /* build the trig tables */
    init_trig();

    fprintf(stderr, "Daemon says 'hello!'\n");	/* say hi */
    srand48(getpid());		/* seed random # gen */

    openmem(attach ? 0 : 2, 0);	/* create shared memory */

    /* my daemonII has been dumping core a lot in readsysdefaults */
    if (!debug) {		/* setup signals if not debugging */
	for (i = 0; i < NSIG; i++)
	    r_signal(i, freemem);
	r_signal(SIGSTOP, SIG_DFL);	/* accept SIGSTOP? 3/6/92 TC */
	r_signal(SIGTSTP, SIG_DFL);	/* accept SIGTSTP? 3/6/92 TC */
	r_signal(SIGCONT, SIG_DFL);	/* accept SIGCONT? 3/6/92 TC */
    }

    if (configleague && !attach) {
	status2->league = 1;	/* configure for league play */
	/* .sysdef will be ignored */

	/* haven't chosen teams yet */
	status2->home.index = status2->away.index = -1;

	/* haven't chosen captains either */
	status2->home.captain = status2->away.captain = -1;

	/* no names for the team */
	status2->home.name[0] = status2->away.name[0] = 0;

	status2->home.ready = status2->away.ready = 0;

	status2->home.desirepause = status2->away.desirepause = 0;

	/* away has NOT passed team choice */
	status2->awaypassed = 0;

	status2->paused = 0;

	/* clear out the temporary player file */
	paths = build_path(PLAYERFILE);
	if (0 != unlink(paths) && errno != ENOENT) {
	    perror("zeroing tourney temporary player file");
	}

	status2->home.desired.galaxyreset = status2->away.desired.galaxyreset
	    = 0;
	status2->home.desired.restart = status2->away.desired.restart
	    = 0;
    }

    if (!attach)
	readsysdefaults();	/* go get sysdefaults */

    if (configleague && !attach) {
	status2->home.timeouts_left = status2->away.timeouts_left =
	    configvals->timeouts;

	status2->home.desired.regulation = status2->away.desired.regulation
	    = configvals->regulation_minutes;
	status2->home.desired.overtime = status2->away.desired.overtime
	    = configvals->overtime_minutes;
	status2->home.desired.maxplayers = status2->away.desired.maxplayers
	    = configvals->playersperteam;
    }


    if (!attach) {
	for (i = 0; i < MAXPLAYER; i++) {	/* go through all players */
	    players[i].p_status = PFREE;	/* set slot free */
	    players[i].p_no = i;/* set his player number */
	    players[i].p_ntspid = 0;
	}
	status2->nontteamlock = ALLTEAM;
	status2->starttourn = 0;
    }				/* !attach */

    paths = build_path(PLFILE);
    plfd = open(paths, O_RDWR, 0744);	/* open planets file */
    if (!attach) {
	gen_planets();		/* generate a new galaxy every time */
	status->time = 0;
    }				/* !attach */
    paths = build_path(GLOBAL);

    glfd = open(paths, O_RDWR, 0744);	/* try to open file */

    if (!attach) {
	if (glfd < 0) {		/* if could not open */
	    fprintf(stderr, "No global file.  Resetting all stats\n");
	    memset((char *) status, 0, sizeof(struct status));
	    glfd = open(paths, O_RDWR | O_CREAT, 0744);	/* try to create file */
	}
	else {
	    int nr;

	    if ((nr = read(glfd, (char *) status, sizeof(struct status))) !=
		sizeof(struct status)) {	/* try to read file */
		fprintf(stderr, "Global file wrong size (read %d, expected %d).  Resetting all stats\n", nr, sizeof(struct status));
		memset((char *) status, 0, sizeof(struct status));
	    }
	}
	if (status->time == 0) {/* do stats need resetting */
	    status->dooshes = 1500;	/* yup, then reset them */
	    status->armsbomb = 4000;	/* set them to something other than */
	    status->resbomb = 1200;	/* zeroes and ones so that the */
	    status->planets = 1000;	/* globals are not totally whacked */
	    status->kills = 1;		/* when we first start */
	    status->losses = 1;
	    status->genocides = 10;
	    status->sbkills = 1200;
	    status->sblosses = 30;
	    status->sbtime = 720000;
	    status->wbkills = 1200;
	    status->wblosses = 40;
	    status->wbtime = 360000;
	    status->jsplanets = 400;
	    status->jstime = 240000;
	    status->time = 1;
	    status->timeprod = 1;
	}

	/* wait queue stuff */
	status->wait = 0;	/* invocation of the */
	status->count = 0;	/* daemon */
	status->request = 0;

    }				/* !attach */
    status->active = 0;		/* set stats that deal with this */
    status->gameup = 1;
    status->nukegame = getpid();
    status->timeprod = 0;
    
    setjmp(env);		/* set the loooong jump */

    r_signal(SIGCHLD, reaper);	/* set reaper and setflag signal */
    r_signal(SIGALRM, setflag);	/* handlers */

    if (!attach) {
	for (i = 0; i <= MAXTEAM; i++) {	/* reset some team vars */
	    teams[i].s_surrender = 0;	/* reset surrender timers */
	    for (jjk = 0; jjk < NUM_TYPES; jjk++)
		teams[i].s_turns[jjk] = 0;	/* reset all ship
						   construction timers */
	}
    }				/* !attach */

    status2->newgalaxy = 0;
    for( i = 0; i < MAXPLAYER; i++ ){
      galaxyValid[i] = 0;
    }
      
    check_load();		/* check the load on machine */

    starttimer();		/* start interval timer */
    doMove = 0;

    while (1) {			/* do forever */
	if (!doMove)
	    pause();		/* wait for signal */

	if (doMove) {		/* if it's time */
	    doMove = 0;		/* reset the flag */
	    move();		/* then do the update */

	    if (debug) {	/* if in debug mode */
		if (!(++x % 50))/* print 'mark' as we wait */
		    printf("Mark %d\n", x);
	    }
	}
    }
}

void 
starttimer(void)
{
    struct itimerval udt;

    udt.it_interval.tv_sec = 0;
    udt.it_interval.tv_usec = UPDATE;
    udt.it_value.tv_sec = 0;
    udt.it_value.tv_usec = UPDATE;
    setitimer(ITIMER_REAL, &udt, (struct itimerval *) 0);
}

void 
stoptimer(void)
{
    struct itimerval udt;

    udt.it_interval.tv_sec = 0;
    udt.it_interval.tv_usec = 0;
    udt.it_value.tv_sec = 0;
    udt.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &udt, (struct itimerval *) 0);
}

void
ghostmess(struct player *victim)
{
    char    buf[80];
    static float ghostkills = 0.0;
    int     i, k;

    ghostkills += 1.0 + victim->p_armies * 0.1 + victim->p_kills * 0.1;
    sprintf(buf, "%s (%s) was kill %0.2f for the GhostBusters",
	    victim->p_name, twoletters(victim),
	    ghostkills);
    pmessage(buf, 0, MALL, MSERVA);
    if (victim->p_armies > 0) {
	k = 10 * (remap[victim->p_team] - 1);
	if (k >= 0 && k <= 30)
	    for (i = 0; i < 10; i++) {
		if (planets[i + k].pl_owner == victim->p_team) {
		    planets[i + k].pl_armies += victim->p_armies;
		    sprintf(buf, "%s's %d armies placed on %s",
			    victim->p_name, victim->p_armies, planets[k + i].pl_name);
		    pmessage(buf, 0, MALL | MGHOST, MSERVA);
		    break;
		}
	    }
    }
}

void
saveplayer(struct player *victim)
{
    int     fd;
    char   *paths;

    if (victim->p_pos < 0)
	return;
    if (victim->p_stats.st_lastlogin == 0)
	return;
    if (victim->p_flags & PFROBOT && !configvals->robot_stats)
	return;

    paths = build_path(PLAYERFILE);
    fd = open(paths, O_WRONLY, 0644);
    if (fd >= 0) {
	lseek(fd, 32 + victim->p_pos * sizeof(struct statentry), 0);
	write(fd, (char *) &victim->p_stats, sizeof(struct stats));
	close(fd);
    }
}


/* Send in a robot to avenge the aggrieved team */
/* -1 for HK, -2 for Terminator, -3 for sticky Terminator */

/* if team in { FED, ROM, KLI, ORI }, a nonzero target means "fleet" mode */
/* CRD feature: number (or -1) for starting planet - MAK,  2-Jun-93 */

void
rescue(int team, int target, int planet)
{
    char   *arg1, argp[5];
    int     pid;
    char   *paths;		/* added 1/18/93 KAO */

    if (status2->league)
	return;			/* no robots during league play */

    sprintf(argp, "-S%d", planet);

    if ((pid = fork()) == 0) {
	/* underscore is just a place holder */
	static char	termbuf[] = "-Tt_";
	if (!debug) {
	    close(0);
	    close(1);
	    close(2);
	}
	r_signal(SIGALRM, SIG_DFL);
	paths = build_path(ROBOT);
	switch (team) {
	case FED:
	    arg1 = "-Tf";
	    break;
	case ROM:
	    arg1 = "-Tr";
	    break;
	case KLI:
	    arg1 = "-Tk";
	    break;
	case ORI:
	    arg1 = "-To";
	    break;
	case HUNTERKILLER:
	    arg1 = "-Ti";	/* -1 means independent robot */
	    if (!debug) {
		execl(paths, "robot", arg1, "-P", argp, 0);
	    }
	    else {
		execl(paths, "robot", arg1, "-P", argp, "-d", 0);
	    }
	    break;
	case TERMINATOR:	/* Terminator */
	    arg1 = termbuf;
	    arg1[3] = twoletters(&players[target])[1];
	    break;
	case STERMINATOR:	/* sticky Terminator */
	    arg1 = termbuf;
	    arg1[3] = twoletters(&players[target])[1];
	    if (!debug)
		execl(paths, "robot", arg1, "-s", argp, 0);
	    else
		execl(paths, "robot", arg1, "-s", argp, "-d", 0);
	    break;
	default:
	    arg1 = "-Ti";
	    break;
	}
	if (target > 0)		/* be fleet 8/28/91 TC */
	    execl(paths, "robot", arg1, "-f", argp, 0);
	else if (!debug) {	/* Make these fleet, too - MAK,  4-Jun-93 */
	    execl(paths, "snake", arg1, argp, 0);
	    /* execl (paths, "robot", arg1, "-f", argp, 0); */
	}
	else {			/* Make these fleet, too - MAK,  4-Jun-93 */
	    execl(paths, "snake", arg1, argp, "-d", 0);
	    /* execl (paths, "snake", arg1, "-f", argp, "-d", 0); */
	}
	/* If we get here, we are hosed anyway */
	fprintf(stderr, "Failed to exec robot %s.\n", paths);
	exit(1);
    }
    else {
	if (debug) {
	    fprintf(stderr, "Forking robot: pid is %d\n", pid);
	}
    }
}
