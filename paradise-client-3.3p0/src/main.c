/*
 * main.c
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "packets.h"
#include "gppackets.h"

#ifdef UNIX_SOUND
#include "sound.h"
#endif

jmp_buf env;

/* Prototypes */
static void printUsage P((char *prog));
void checkExpire P((int verbose));

extern int xpmORplanes;	/* from x11window.c */

int
main(int argc, char **argv)
{
    int     team, s_type;
    char   *dpyname = NULL;
    int     usage = 0;
    int     err = 0;
    char   *name, *ptr, *cp;
    struct passwd *pwent;
    int     passive = 0;
    int     xpmopt = 1;
    int     useORopt = 0;
    int     useCookieOpt = 0;
    int     dontUseCookieOpt = 0;
/*    char *defaultsFile=NULL;*/

    pseudo[0] = defpasswd[0] = '\0';

    name = *argv++;
    argc--;
    if ((ptr = strrchr(name, '/')) != NULL)
	name = ptr + 1;
    while (*argv) {
	if (**argv != '-') {
	    serverName = *argv;	/* don't abort argument processing */
	    argv++;
	    argc--;
	} else {
	    ++*argv;

	    argc--;
	    ptr = *argv++;
	    while (*ptr) {
		switch (*ptr) {
		case 'C':	/* character name */
		    (void) strncpy(pseudo, *argv, sizeof(pseudo));
		    argv++;
		    argc--;
		    break;

		case 'P':	/* authorization password */
		    (void) strncpy(defpasswd, *argv, sizeof(defpasswd));
		    {
			int     i;
			for (i = 0; (*argv)[i]; i++)
			    (*argv)[i] = 0;
		    }
		    argv++;
		    argc--;
		    break;

		case 'u':
		    usage++;
		    break;
		case 's':
		    if (*argv) {
			xtrekPort = atoi(*argv);
			passive = 1;
			argv++;
			argc--;
		    }
		    break;
		case 'p':
		    if (*argv) {
			xtrekPort = atoi(*argv);
			argv++;
			argc--;
		    }
		    break;
		case 'd':
		    dpyname = *argv;
		    argc--;
		    argv++;
		    break;
		case 'm':
		    usemeta = 1;
		    break;
		case 'h':
		    serverName = *argv;
		    argc--;
		    argv++;
		    break;

		case 't':
		    title = *argv;
		    argc--;
		    argv++;
		    break;
		case 'r':
		    defaultsFile = *argv;
		    argv++;
		    argc--;
		    break;
#ifdef AUTHORIZE
		case 'o':
		    RSA_Client = -1;
		    break;
		case 'R':
		    RSA_Client = -2;
		    break;
#else
		case 'o':
		case 'R':
		    printf("This client does not have binary authorization.\n");
		    break;
#endif
		case 'e':
#ifdef AUTHORIZE
		    checkExpire(1);
#else
		    printf("This client does not RSA verify and will not expire.\n");
#endif
		    exit(0);
		    break;
		case 'f':	/* list ftp sites */
		    fprintf(stderr, "\n\
The newest version of the Paradise client can be found at:\n\
      ftp.netrek.org  in /pub/netrek/paradise/bin/\n");
		    exit(0);
		case 'G':
		    if (*argv) {
			ghoststart++;
			ghost_pno = atoi(*argv);
			printf("Emergency restart being attempted...\n");
			argv++;
			argc--;
		    }
		    break;
		case '2':	/* force paradise */
		    paradise = 1;
		    break;
		case 'F':	/* File playback */
		    if (*argv) {
			playFile = strdup(*argv);
			playback = 1;
			argv++;
			argc--;
		    }
		    break;
		case 'x':
		    xpmopt = 0;
		    break;
		case 'k':		/* cookie mode [BDyess] */
		    useCookieOpt = 1;
		    break;
		case 'K':		/* no-cookies :( [BDyess] */
		    dontUseCookieOpt = 1;
		    break;
		case 'v':
		    verbose_image_loading = 1;
		    break;
		case 'O':		/* turn on GXor image drawing [BDyess]*/
		    useORopt = 1;	
		    break;
                case 'c': 	/* dump .paradiserc defaults [BDyess] */
		    dump_defaults = 1;
		    break;
		default:
		    fprintf(stderr, "%s: unknown option '%c'\n", name, *ptr);
		    err++;
		    break;
		}
		ptr++;
	    }
	}
    }

    inittrigtables();

    initStars();		/* moved from redraw.c at KAO\'s suggestion */

    if (usage || err) {
	printUsage(name);
#ifdef AUTHORIZE
	checkExpire(1);
#endif
	exit(0);
	/* exit(err); Exits from checkExpire */
    }
    defaultsFile = initDefaults(defaultsFile);

    if(xpmopt) xpm = 1;
    else xpm = booleanDefault("xpm",xpm);
    if(xpm) printf("XPM mode enabled.\n");
    /* command line option overrides .paradiserc value [BDyess] */
    if(useORopt) useOR = 1;
    else useOR = booleanDefault("useOR",useOR);
    if(useOR) printf("OR mode enabled.\n");
    if(useOR || !xpm) cookie = 0;	/* default no-cookies unless in XPM
    					   mode w/out OR [BDyess] */
					/* note: need a milk mode :) */
    if(useCookieOpt) cookie = 1;
    else if(dontUseCookieOpt) cookie = 0;
    else cookie = booleanDefault("cookie",cookie);
    if(cookie) printf("Cookie mode enabled.\n");

#ifdef AUTHORIZE
    if (RSA_Client != -1)
	checkExpire(0);
#endif

    /* compatability */
    if (argc > 0)
	serverName = argv[0];

    srandom(getpid() + time((long *) 0));

    if(playback || booleanDefault("playback",0)) {
        defNickName = "playback";
	usemeta=0;
        serverName = "playback";
    } else
    {
        if (serverName) {
	    char    temp[80], *s;
	    sprintf(temp, "server.%s", serverName);
	    if ((s = stringDefault(temp,NULL))) {
		printf("Using nickname \"%s\" for server %s\n", serverName, s);
		defNickName = serverName;
		serverName = s;
		defFlavor = stringDefault("flavor",NULL);
	    }
	}
	if (!serverName) {
	    serverName = stringDefault("server",NULL);
        }
	if (!serverName && !passive) {
	    serverName = DEFAULT_SERVER;
	    usemeta = 1;		/* no server specified, show the menu */
	}
	if (passive)
	    serverName = "passive";	/* newwin gets a wrong title otherwise */

	if (xtrekPort < 0)
	    xtrekPort = intDefault("port", -1);
	if (xtrekPort < 0)
	    xtrekPort = DEFAULT_PORT;
    } /* playback */
    build_default_configuration();

    metaserverAddress = stringDefault("metaserver",
				      "metaserver.netrek.org");
    if (usemeta)
	openmeta();

    W_Initialize(dpyname);

    metaFork = booleanDefault("metaFork", metaFork);
    /* do the metawindow thang */
    if (usemeta) {
	metawindow();
	metainput();
	if (metaFork)
	    W_Initialize(dpyname);
	newwin(dpyname, name);
    } else

	/* this creates the necessary x windows for the game */
	newwin(dpyname, name);

    /* open memory...? */
    openmem();
    if (!startPlayback())
    {
	if (!passive) {
	    serverName = callServer(xtrekPort, serverName);
	} else {
	    connectToServer(xtrekPort);
	}
    }
    sendFeature("FEATURE_PACKETS", 'S', 1, 0, 0);

    timeStart = time(NULL);
    findslot();

    /* sets all the settings from defaults file (.xtrekrc probably) */
    resetDefaults();

#ifdef UNIX_SOUND
    init_sound();
    play_sound(SND_PARADISE);
#endif

    mapAll();
/*    signal(SIGINT, SIG_IGN);*/
    signal(SIGCHLD, reaper);

    /* Get login name */
    if ((pwent = getpwuid(getuid())) != NULL)
	(void) strncpy(login, pwent->pw_name, sizeof(login));
    else
	(void) strncpy(login, "Bozo", sizeof(login));
    login[sizeof(login) - 1] = '\0';

    if (pseudo[0] == '\0') {
	char *freeme;
	strncpy(pseudo, freeme = stringDefault("name",login), sizeof(pseudo));
	free(freeme);
    }
    pseudo[sizeof(pseudo) - 1] = '\0';

    if (defpasswd[0] == '\0') {
	char buf[100];  /* added password by character name -JR */
	sprintf(buf,"password.%s",pseudo);
	if((cp = stringDefault(buf,NULL)) || (cp = stringDefault("password",NULL)))
	    (void) strncpy(defpasswd, cp, sizeof(defpasswd));
    }
    defpasswd[sizeof(defpasswd) - 1] = '\0';

    /*
       sendLoginReq("Gray Lensman", "hh", "sfd", 0); loginAccept = -1; while
       (loginAccept == -1) { socketPause(1,0); readFromServer(); }
    */
    getname(pseudo, defpasswd);
    loggedIn = 1;

    /*
       Set p_hostile to hostile, so if keeppeace is on, the guy starts off
       hating everyone (like a good fighter should)
    */
    me->p_hostile = (1 << number_of_teams) - 1;

    redrawTstats();

    me->p_planets = 0;
    me->p_genoplanets = 0;
    me->p_armsbomb = 0;
    me->p_genoarmsbomb = 0;
    /* Set up a reasonable default */
    me->p_whydead = KNOREASON;
    me->p_teami = -1;
    s_type = defaultShip(CRUISER);	/* from rlb7h 11/15/91 TC */

    if (booleanDefault("netStats", 1))
	startPing();		/* tell the server that we support pings */

    /*
       hack to make galaxy class ships work.  This could be more elegant, but
       the configuration code would have to be modified quite a bit, since
       the client doesn't know if it's on a paradise server until after it
       connects, and it needs the configuration info before it connects.
    */
    init_galaxy_class();

    initkeymap(-1);		/* needs to have ship types initialized -JR */

    setjmp(env);		/* Reentry point of game */

    if (ghoststart) {
	int     i;

	ghoststart = 0;

	for (i = -1; i < 5; i++)
	    if (teaminfo[i].letter == me->p_mapchars[0])
		break;

	me->p_teami = i;

	if (me->p_damage > me->p_ship->s_maxdamage) {
	    me->p_status = POUTFIT;
	} else
	    me->p_status = PALIVE;
    } else
	me->p_status = POUTFIT;

    while (1) {
	switch (me->p_status) {
	case POUTFIT:
	case PTQUEUE:
	    /* give the player the motd and find out which team he wants */
	    new_entrywindow(&team, &s_type);
	    allowPlayerlist = 1;
	    if (W_IsMapped(playerw))
		playerlist();

	    if (!playback)
		if (team == -1) {
		    W_DestroyWindow(w);
		    sendByeReq();
		    sleep(1);
		    printf("OK, bye!\n");
		    EXIT(0);
		}
	    sendVersion();
	    myship = getship(myship->s_type);

	    currentship = myship->s_type;

	    /*
	       sendOptionsPacket(); this would totally blast any flags you
	       had on the server
	    */

	    redrawall = 1;
	    enter();
	    calibrate_stats();
	    W_ClearWindow(w);
	    /*
	       for (i = 0; i < NSIG; i++) { signal(i, SIG_IGN); }
	    */

	    me->p_status = PALIVE;	/* Put player in game */

#ifdef UNIX_SOUND
            kill_sound ();
#endif
            
	    hockeyInit();

	    if (showStats)	/* Default showstats are on. */
		W_MapWindow(statwin);
	    if (showNewStats)	/* default showNewStats are off. [BDyess] */
	        W_MapWindow(newstatwin);

	    if (tryUdp && commMode != COMM_UDP) {
		sendUdpReq(COMM_UDP);
	    }

	    if (tryShort) {
		sendShortReq(SPK_VON);
		tryShort = 0;	/* only try it once */
	    }
	    /* Send request for a full update */
	    if (askforUpdate) {
		if(recv_short)
		    sendShortReq(SPK_SALL);
		else
		    sendUdpReq(COMM_UPDATE);
	    }
	    sendUpdatePacket(1000000 / updateSpeed);

	    W_Deiconify(baseWin);

	    break;
	case PALIVE:
	case PEXPLODE:
	case PDEAD:
	case POBSERVE:

	    /* Get input until the player quits or dies */
	    input();
	    W_ClearWindow(mapw);
	    break;
	default:
	    printf("client has p_status=%d.  how strange\n", me->p_status);
	    me->p_status = POUTFIT;
	}
    }

    /* NOTREACHED */
}

static void
printUsage(char *prog)
{
    fprintf(stderr, "Usage:\n  %s [ options ] [ ntserv-host ]\n\
Where options are\n\
    [-h] host          server host name\n\
    [-p] port          server port number\n\
\n\
    [-x]               disable XPM mode.\n\
    [-k/-K]            turn on/off Cookie mode.\n\
    [-O]               turn on OR display mode when in XPM mode.\n\
\n\
  For emergency restart:\n\
    [-2]               force paradise - use if you were on a paradise server\n\
    [-G] playernum     specify player number to use\n\
    [-s] port          specify socket number to use\n\
\n\
  Other options:\n\
    [-c]               dump .paradiserc defaults\n\
    [-d] display       set Xwindows display\n\
    [-e]               check the expire time on the client\n\
    [-f]               how to get the newest client\n\
    [-m]               check metaserver for active servers\n\
    [-o]               use old (non-RSA) authorization\n\
    [-r] xtrekrc       defaults file to replace ~/.xtrekrc\n\
    [-t] title         window manager title\n\
    [-u]               print usage (this message)\n\
    [-v]               verbose image loading.\n\
    [-C] name          netrek pseudonym\n\
    [-F] file          Replay from file instead of connecting\n\
    [-P] passwd        passwd to use to attempt autologin\n\
    [-R]               use RSA authorization (default)\n\
\n\
Paradise Full-Color Client %s\n\
For more information on how to play Paradise, go to\n\
    http://paradise.netrek.org/\n\n", prog,
	    CLIENTVERS);
}

RETSIGTYPE
reaper(void)
{
#ifndef HAVE_WAIT3
    wait(NULL);
#else
/*    while (wait3((union wait *) 0, WNOHANG, NULL) > 0);*/
    while (wait3(NULL, WNOHANG, NULL) > 0);
#endif
}
