/*
 * input.c
 *
 * Modified to work as client in socket based protocol
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "packets.h"
#include "gppackets.h"

#define control(x) (x)+128

/* Prototypes */
static void buttonaction P((W_Event * data));
static void keyaction P((W_Event * data));
static void scan P((W_Window window, int x, int y));
/*static void selectblkbozo P((W_Event *data));*/

static int tmodeChange = 0;     /* handles first change of tmode; including */
				/* when a play first joins a tmode game */

void
initinput(void)
{
    /* Nothing doing... */
}

void
dispatch_W_key_event(W_Event *evt)
{
    int     i;

    /* try to speed up response -JR */
    if (!messageon && (evt->Window == w || evt->Window == mapw)) {
	keyaction(evt);
	return;
    }
    for (i = 0; i < WNUM; i++) {
	if (evt->Window == messWin[i].window) {
	    messageWinEvent(evt);
	    return;
	}
    }
    if (evt->Window == optionWin)
	optionaction(evt);
    else if (
	     messageon ||
	     evt->Window == messagew ||
	     evt->Window == tstatw ||
	     evt->Window == warnw) {
	/*
	   if it was a control key, convert it to a representation we're more
	   familiar with.
	*/
	if (evt->key > 128) {
	    evt->key -= 128;
	    if (evt->key >= 0x60 && evt->key < 0x80)
		evt->key -= 0x60;
	    else if (evt->key >= 0x40 && evt->key < 0x60)
		evt->key -= 0x40;
	}
	smessage(evt->key);
    } else if (evt->Window == spWin) {
	spaction(evt);
    } else if (evt->Window == motdWin) {
	motdWinEvent(evt);
    } else if (evt->Window == playerw) {
	playerwEvent(evt);
    } else if (evt->Window == defWin && (evt->key == ' ' || evt->key == 27)) {
	W_UnmapWindow(defWin);
    } else if (evt->Window == toolsWin) {
        if(evt->key == 27) W_UnmapWindow(toolsWin);  /* unmap on ESC [BDyess] */
	smessage_ahead('!', evt->key);
    /* unmap on space or escape [BDyess] */
    } else if (evt->Window == helpWin && (evt->key == ' ' || evt->key == 27)) {
        W_UnmapWindow(helpWin);
    } else if (evt->Window == pStats && (evt->key == ' ' || evt->key == 27)) {
        W_UnmapWindow(pStats);
    } else if (evt->Window == statwin && (evt->key == ' ' || evt->key == 27)) {
        W_UnmapWindow(statwin);
    } else if (evt->Window == newstatwin && (evt->key == ' '||evt->key == 27)) {
        W_UnmapWindow(newstatwin);
    } else if (evt->Window == udpWin && (evt->key == ' ' || evt->key == 27)) {
        W_UnmapWindow(udpWin);
    } else if (evt->Window == rankw && (evt->key == ' ' || evt->key == 27)) {
        W_UnmapWindow(rankw);
    } else if (evt->Window == planetw && (evt->key == ' ' || evt->key == 27)) {
        W_UnmapWindow(planetw);
    } else if (evt->Window == planetw2 && (evt->key == ' ' || evt->key == 27)) {
        W_UnmapWindow(planetw2);
    } else {
	keyaction(evt);
    }
}

void
dispatch_W_button_event(W_Event *evt)
{
    int     i;

    if (evt->Window == w || evt->Window == mapw) {
	buttonaction(evt);
	return;
    }
    for (i = 0; i < WNUM; i++) {
	if (evt->Window == messWin[i].window) {
	    messageWinEvent(evt);
	    return;
	}
    }
    if (evt->Window == war)
	waraction(evt);
    else if (evt->Window == optionWin)
	optionaction(evt);
    else if (evt->Window == udpWin)
	udpaction(evt);		/* UDP */
    else if (evt->Window == spWin)
	spaction(evt);
    else if (evt->Window == playerw)
	selectblkbozo(evt);
    else if (evt->Window == macroWin)
	switchmacros();
    else if (evt->Window == defWin)
	def_action(evt);
    else if (evt->Window == motdWin)
	motdWinEvent(evt);
    else
	buttonaction(evt);
}

void
dispatch_W_expose_event(W_Event *evt)
{
    /*
       if anything but the iconWin is refreshed, turn off the iconified flag.
       [BDyess]
    */
    if (evt->Window != iconWin)
	iconified = 0;
    if (evt->Window == statwin)
	redrawStats();
    else if (evt->Window == newstatwin)
        redrawNewStats();
    else if (evt->Window == tstatw)
	redrawTstats();
    else if (evt->Window == mapw)
	redrawall = 1;
    else if (evt->Window == iconWin)
	drawIcon();
    else if (evt->Window == helpWin)
	fillhelp();
    else if (evt->Window == macroWin)
	fillmacro();
    else if (evt->Window == playerw)
	playerlist();
    else if (evt->Window == planetw)
	planetlist();
    else if (evt->Window == planetw2)
	planetlist();
    else if (evt->Window == rankw)
	ranklist();
    else if (evt->Window == warnw)
	W_ClearWindow(warnw);
    else if (evt->Window == messagew)
	message_expose();
    else if (evt->Window == motdWin) {
        evt->key = 'r';		/* 'r' is refresh */
	motdWinEvent(evt);
    }
    /*
       lag meter?  maybe later - RF else if (evt->Window == lMeter)
       redrawLMeter();
    */
    else if (evt->Window == pStats)
	redrawPStats();
    else if (defWin && (evt->Window == defWin))
	showdef();
}

void
dispatch_W_event(W_Event *evt)
{
    switch ((int) evt->type) {
    case W_EV_KEY:
	dispatch_W_key_event(evt);
	break;
    case W_EV_BUTTON:
	dispatch_W_button_event(evt);
	break;
    case W_EV_EXPOSE:
	dispatch_W_expose_event(evt);
	break;
    case W_EV_KILL_WINDOW:
        if(evt->Window == baseWin) {
	  exit(0);
	}
        W_UnmapWindow(evt->Window);
	break;
    default:
	break;
    }
}

/* this figures out what to set war dec's */
void
autoWarDecl(int scheme)
{
     extern int number_of_teams;
     int i, j, k, *team, enemymask = 0;
     struct player *pptr;

     if((team = (int *)malloc(number_of_teams * sizeof(int))) == NULL) {
        perror("autoWarDecl: malloc error\n");
        return;
     }
     memset(team, 0, sizeof(int)*number_of_teams);
     for(i=0, pptr=&players[i]; i < nplayers; i++, pptr++)
        if(pptr->p_status != PFREE && !(pptr->p_status == POUTFIT &&
                                     pptr->p_teami < 0)) {
             team[pptr->p_teami]++;
        }
     switch(scheme) {
     case 1:
        /* war with all non-zero member team */
        /* peace with teams with 0 players */
        for(i=0; i < number_of_teams; i++) {
             if(i != me->p_teami)
                  enemymask ^= team[i] ? (1<<i) : 0;
             /*printf("team: %i, #: %i\n", i, team[i]);
              */
        }
        /*printf("mask: %i\n", enemymask);
         */
        break;
     case 2:
        /* war with only the largest enemy */
        /* team; peace with everyone else*/
        for(i=0; i < number_of_teams; i++) {
             if ((i != me->p_teami) && (j < team[i])) {
                  j = team[i];
                  k = i;
             }
        }
        enemymask = 0 | (1 << k);
        break;
     }
     sendWarReq(enemymask);
     free(team);
} /* end of autoWarDecl */



/* this new event loop allows more flexibility in state transitions */
void
input(void)
{
    W_Event data;
    fd_set  readfds;
    int     wsock = W_Socket();
    int     old_tourn = paradise ? status2->tourn : status->tourn, new_tourn;
    int     i;
    struct timeval timeout;

    if (playback) {
	pb_input(); /* recorder.c */
	return;
    }
    while (me->p_status == PALIVE ||
	   me->p_status == PEXPLODE ||
	   me->p_status == PDEAD ||
	   me->p_status == POBSERVE) {

	if (keepInfo > 0) {
	    if (infowin_up >= 0 &&
		--infowin_up == -1 &&
		infomapped) {
		destroyInfo();
		infowin_up = -2;
	    }
	}
	exitInputLoop = 0;
	/* make sure drawing is done before starting again [BDyess] */
	W_Sync();
	while (W_EventsPending() && !exitInputLoop) {
	    fastQuit = 0;	/* any event cancel fastquit */
	    W_NextEvent(&data);
	    dispatch_W_event(&data);
	}

	    /* try to reduce the response time by handling input 
	       earlier [BDyess] */
	    FD_ZERO(&readfds);
	    while(1) {
	      FD_SET(wsock, &readfds);
	      FD_SET(sock, &readfds);
	      if (udpSock >= 0)
		FD_SET(udpSock, &readfds);
	      timeout.tv_sec = 0;
	      timeout.tv_usec = 50000;
	      i = select(32, &readfds, (fd_set *) 0, (fd_set *) 0, &timeout);
	      while (W_EventsPending() && !exitInputLoop) {
		  fastQuit = 0;	/* any event cancel fastquit */
		  W_NextEvent(&data);
		  dispatch_W_event(&data);
	      }
	      if (i != 0) {	/* got some data to read [BDyess] */
	        break;
	      }
	    }
	if (FD_ISSET(sock, &readfds) ||
	    (udpSock >= 0 && FD_ISSET(udpSock, &readfds))) {
	    
	    intrupt();
	    if (isServerDead()) {
		printf("Whoops!  We've been ghostbusted!\n");
		printf("Pray for a miracle!\n");
		
		/* UDP fail-safe */
		commMode = commModeReq = COMM_TCP;
		commSwitchTimeout = 0;
		if (udpSock >= 0)
		    closeUdpConn();
		if (udpWin) {
		    udprefresh(UDP_CURRENT);
		    udprefresh(UDP_STATUS);
		}
		connectToServer(nextSocket);
		printf("Yea!  We've been resurrected!\n");
	    }
	    /*
	       beep if tmode status changes.  Idea from Roger Books. [BDyess]
	    */
	    new_tourn = paradise ? status2->tourn : status->tourn;

            /* change war dec's at transitions to */
            /* positive tmode */

            if (!tmodeChange && new_tourn) {
                autoWarDecl(autoSetWar);
                tmodeChange = 1;
            }

	    if (old_tourn != new_tourn) {
		W_Beep();
		old_tourn = new_tourn;
	    }
	    continue;
	}
    }
}


static void
keyaction(W_Event *data)
{
    unsigned char course;
    struct obtype *target;
    int     key = data->key;
    struct shiplist *temp;

    if (data->Window != mapw && data->Window != w
	&& data->Window != planetw && data->Window != planetw2
	&& data->Window != rankw
	&& data->Window != scanw
	)
	return;

    if (playback)
	pb_dokey(data);
    if (localflags & PFREFIT) {
	temp = shiptypes;
	while (temp) {
	    if (temp->ship->s_letter == key) {
		do_refit(temp->ship->s_type);
		return;
	    }
	    temp = temp->next;
	}
    } else {
	key = doKeymap(data);
	if (key == -1)
	    return;
    }

    switch (key) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	set_speed(key - '0');
	localflags &= ~(PFREFIT);
	break;
    case 'e':			/* Turn off docking permission, eject docked
				   vessels. */
	sendDockingReq(!(me->p_flags & PFDOCKOK));
	break;
    case 'r':
	localflags |= PFREFIT;
	warning(blk_refitstring);
	break;
    case control('0'):		/* ctrl 0-9, speed 10-19 */
    case control('1'):
    case control('2'):
    case control('3'):
    case control('4'):
    case control('5'):
    case control('6'):
    case control('7'):
    case control('8'):
    case control('9'):
	set_speed(10 + key - control('0'));
	localflags &= ~(PFREFIT);
	break;
    case control(')'):		/* ctrl-shift 0-9, speed 20-29 */
	set_speed(20);
	localflags &= ~(PFREFIT);
	break;
    case control('!'):		/* ctrl-shift-1, speed 21 */
	set_speed(21);
	localflags &= ~(PFREFIT);
	break;
    case control('@'):		/* ctrl-shift-2, speed 22 */
	set_speed(22);
	localflags &= ~(PFREFIT);
	break;
    case control('#'):		/* ctrl-shift-3, speed 23 */
	set_speed(23);
	localflags &= ~(PFREFIT);
	break;
    case control('$'):		/* ctrl-shift-4, speed 24 */
	set_speed(24);
	localflags &= ~(PFREFIT);
	break;
    case control('%'):		/* ctrl-shift-5, speed 25 */
	set_speed(25);
	localflags &= ~(PFREFIT);
	break;
    case control('^'):		/* ctrl-shift-6, speed 26 */
	set_speed(26);
	localflags &= ~(PFREFIT);
	break;
    case control('&'):		/* ctrl-shift-7, speed 27 */
	set_speed(27);
	localflags &= ~(PFREFIT);
	break;
    case control('*'):		/* ctrl-shift-8, speed 28 */
	set_speed(28);
	localflags &= ~(PFREFIT);
	break;
    case control('('):		/* ctrl-shift-9, speed 29 */
	set_speed(29);
	localflags &= ~(PFREFIT);
	break;
    case '`':			/* afterburners */
	set_speed(98);
	localflags &= ~(PFREFIT);
	break;
    case '-':
	set_speed(99);		/* warp! */
	localflags &= ~(PFREFIT);
	break;
    case control('-'):		/* suspend warp toggle [BDyess] */
	if (me->p_flags & PFWPSUSPENDED)
	    set_speed(96);	/* unsuspend */
	else
	    set_speed(97);	/* suspend */
	localflags &= ~(PFREFIT);
	break;
    case '%':			/* max impulse */
	set_speed(me->p_ship->s_maxspeed);
	localflags &= ~(PFREFIT);
	break;
    case '<':			/* speed -= 1 */
	set_speed(me->p_speed - 1);
	localflags &= ~(PFREFIT);
	break;
    case '>':			/* speed += 1 */
	set_speed(me->p_speed + 1);
	localflags &= ~(PFREFIT);
	break;
    case '#':			/* halfimpulse */
	set_speed((me->p_ship->s_maxspeed + 1) / 2);
	localflags &= ~(PFREFIT);
	break;
    case ':':			/* toggle message logging */
	if (logmess)
	    warning("Message logging disabled");
	else
	    warning("Message logging enabled");
	logmess = !logmess;
	break;
    case '!':
	showKitchenSink = !showKitchenSink;
	warning(showKitchenSink ?
		"Kitchen Sink activated.  Bad guys beware!" :
		"Kitchen Sink deactivated.");
	break;
    case '@':
	timeBank[T_USER] = time(NULL);
	timerType = T_USER;
	break;
    case control('t'):
	timerType++;
	if (timerType >= T_TOTAL)
	    timerType = 0;
	break;
    case 'K':			/* cycle playerlist [BDyess] */
	while (*playerList && *playerList != ',')
	    playerList++;
	if (*playerList == ',')
	    playerList++;
	else if (*playerList == 0)
	    playerList = playerListStart;
	break;
    case 'a':
	if (!W_IsMapped(scanwin)) {
	    scan(data->Window, data->x, data->y);
	} else {
	    if (scanmapped)
		W_UnmapWindow(scanwin);
	    scanmapped = 0;
	}
	break;
    case 'm':			/* new from galaxy -- works here too */
    case '\'':           /* ' starts message to 'T'eam */
	message_on();
	if((key == '\'') && (messpend==0)) {
	    smessage(lowercaset ? 't' : 'T');
	}
	break;
    case 'k':			/* k = set course */
	course = getcourse(data->Window, data->x, data->y);
	set_course(course);
	me->p_flags &= ~(PFPLOCK | PFPLLOCK);
	localflags &= ~(PFREFIT);
	break;
    case 'p':			/* p = fire phasers */
	course = getcourse(data->Window, data->x, data->y);
	sendPhaserReq(course);
	break;
    case 't':			/* t = launch torps */
	course = getcourse(data->Window, data->x, data->y);
	sendTorpReq(course);
	break;
    case 'f':
	/* f = launch plasma torpedos */
	course = getcourse(data->Window, data->x, data->y);
	sendPlasmaReq(course);
	break;
    case 'd':			/* d = detonate other torps */
	sendDetonateReq();
	break;
    case 'D':			/* D = detonate my torps */
	detmine();
	break;
    case '[':
	shield_down();
	break;
    case ']':
	shield_up();
	break;
    case 'u':			/* u = toggle shields */
	shield_tog();
	break;
    case 's':			/* For Serge */
	shield_tog();
	break;
    case 'b':			/* b = bomb planet */
	bomb_planet();
	break;
    case 'z':			/* z = beam up */
	    beam_up();
	break;
    case 'x':			/* x = beam down */
	    beam_down();
	break;
    case 'X':			/* X = enter macro mode */
	macroState = 1;
	warning("Macro mode");
	break;
    case 'R':			/* R = Go into repair mode */
	sendRepairReq(1);
	break;
    case 'y':
    case 'T':
	if (me->p_flags & (PFTRACT | PFPRESS)) {
	    sendTractorReq(0, me->p_no);
	    break;
	} else
	/* FALLTHRU */
    case '_':			/* _ = turn on tractor beam */
	/* can go from tract to press without turning anything off, but can't
	   go from press back to tract.  Instead of always turning off T/P,
	   instead only turn it off when absolutely necessary (press on, going
	   to tract).  This reduces the chance that a dropped packet will
	   cause the tractor to be off.  Plus, it reduces the number of packets
	   sent to the server (big deal). [BDyess] */
        if(me->p_flags & PFPRESS)
	  sendTractorReq(0, me->p_no);
	/* FALLTHRU */
    case '^':			/* ^ = turn on pressor beam */
	target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
	me->p_tractor = target->o_num;
	if (key == 'T' || key == '_') {
	    sendTractorReq(1, target->o_num);
	} else {
	    sendRepressReq(1, target->o_num);
	}
	break;
    case '$':			/* turn off tractor/pressor beam */
	sendTractorReq(0, me->p_no);
	break;
    case 'o':			/* o = dock at nearby starbase or orbit
				   nearest planet */
	sendOrbitReq(1);
	break;
    case 'O':			/* O = options Window */
	if (optionWin != NULL && W_IsMapped(optionWin))
	    optiondone();
	else
	    optionwindow();
	break;
    case 'Q':
	sendQuitReq();
	break;
    case 'q':			/* fastquit */
	fastQuit = 1;
	sendQuitReq();
	break;
    case 'V':
	{
	    char *s;
	    char current[BUFSIZ];

	    /* configurable showlocal rotation sequence [BDyess] */
            if (*(showlocal + showLocalLen) == ',' &&
	        *(showlocal + showLocalLen + 1) != 0) { /* not at the end yet */
	      showlocal = showlocal + showLocalLen + 1;
	    } else {	/* at the end, reset to the beginning */
	      showlocal = showLocalSequence;
	    }
	    /* update the length [BDyess] */
	    for(s=showlocal;*s && *s != ',';s++) /*NULL*/;
	    showLocalLen = s - showlocal;
	    sprintf(current,"ShowLocal %*.*s",showLocalLen,
	                     showLocalLen,showlocal);
	    warning(current);
	}
	break;
    case 'B':
	{
	    char *s;
	    char current[BUFSIZ];

	    /* configurable showgalactic rotation sequence [BDyess] */
            if (*(showgalactic + showGalacticLen) == ',' &&
	        *(showgalactic + showGalacticLen + 1) != 0) { 
	        /* not at the end yet */
	      showgalactic = showgalactic + showGalacticLen + 1;
	    } else {	/* at the end, reset to the beginning */
	      showgalactic = showGalacticSequence;
	    }
	    /* update the length [BDyess] */
	    for(s=showgalactic;*s && *s != ',';s++) /*NULL*/;
	    showGalacticLen = s - showgalactic;
	    redrawall = 1;
	    sprintf(current,"ShowGalactic %*.*s",showGalacticLen,
	                     showGalacticLen,showgalactic);
	    warning(current);
	}
	break;
    case '?':			/* ? = Redisplay all message windows */
	if (!W_IsMapped(messWin[WREVIEW].window)) {
	    if (W_IsMapped(messWin[WALL].window)) {
		int     i;
		for (i = 0; i < WNUM; i++) {
		    if (W_IsMapped(messWin[i].window))
			W_UnmapWindow(messWin[i].window);
		}
	    } else {
		W_MapWindow(messWin[WREVIEW].window);
	    }
	} else {
	    W_UnmapWindow(messWin[WREVIEW].window);
	    W_MapWindow(messWin[WALL].window);
	    W_MapWindow(messWin[WTEAM].window);
	    W_MapWindow(messWin[WINDIV].window);
	    W_MapWindow(messWin[WKILL].window);
	    W_MapWindow(messWin[WPHASER].window);
	}
	if (optionWin) {
	    optionredrawtarget(messWin[WREVIEW].window);
	    optionredrawtarget(messWin[WALL].window);
	    optionredrawtarget(messWin[WKILL].window);
	    optionredrawtarget(messWin[WTEAM].window);
	    optionredrawtarget(messWin[WINDIV].window);
	    optionredrawtarget(messWin[WPHASER].window);
	}
	break;
    case 'c':			/* c = cloak */
	cloak();
	break;
    case '{':			/* { = cloak, no toggle */
	sendCloakReq(1);
	break;
    case '}':			/* } = uncloak, no toggle */
	sendCloakReq(0);
	break;
    case 'C':			/* C = coups */
	sendCoupReq();
	break;
    case ';':			/* ; = lock onto planet/base */
	target = gettarget(data->Window, data->x, data->y,
			   TARG_BASE | TARG_PLANET);
	if (target->o_type == PLAYERTYPE) {
	    sendPlaylockReq(target->o_num);	/* a base */
	    me->p_playerl = target->o_num;
	} else {		/* It's a planet */
	    sendPlanlockReq(target->o_num);
	    me->p_planet = target->o_num;
	    planets[target->o_num].pl_flags |= PLREDRAW;
	}
	break;
    case 'l':			/* l = lock onto */
	target = gettarget(data->Window, data->x, data->y,
			   TARG_PLAYER | TARG_ASTRAL);
	if (target->o_type == PLAYERTYPE) {
	    sendPlaylockReq(target->o_num);
	    me->p_playerl = target->o_num;
	} else {		/* It's a planet */
	    sendPlanlockReq(target->o_num);
	    me->p_planet = target->o_num;
	    planets[target->o_num].pl_flags |= PLREDRAW;
	}
	break;
    case '/':			/* toggle sorted player list */
	sortPlayers = !sortPlayers;
	break;
    case '*':			/* send in practice robot */
	sendPractrReq();
	break;
	/* Start of display functions */
    case ' ':			/* ' ' = clear special windows */
	W_UnmapWindow(planetw);
	W_UnmapWindow(planetw2);
	W_UnmapWindow(rankw);
	if (infomapped)
	    destroyInfo();
	W_UnmapWindow(helpWin);
	W_UnmapWindow(war);
	if (optionWin)
	    optiondone();
	if (scanmapped) {
	    W_UnmapWindow(scanwin);
	    scanmapped = 0;
	}
	if (udpWin)
	    udpdone();
	if (defWin)
	    W_UnmapWindow(defWin);
	break;
    case 'E':			/* E = send emergency call */
	if (F_gen_distress)
	    rcd(generic, data);
	else
	    emergency();
	break;
    case 'F':			/* F = send carry report */
	if (F_gen_distress)
	    rcd(carrying, data);
	else
	    carry_report();
	break;
    case 'L':			/* L = Player list */
	if (W_IsMapped(playerw)) {
	    W_UnmapWindow(playerw);
	} else {
	    W_MapWindow(playerw);
	}
	break;
    case 'P':			/* P = Planet list */
	if (W_IsMapped(planetw)) {
	    W_UnmapWindow(planetw);
	    W_UnmapWindow(planetw2);
	} else {
	    W_MapWindow(planetw);
	    W_MapWindow(planetw2);
	}
	break;
    case 'U':			/* U = Rank list */
	if (W_IsMapped(rankw)) {
	    W_UnmapWindow(rankw);
	} else {
	    W_MapWindow(rankw);
	}
	break;
    case 'S':			/* S = toggle stat mode */
	if (W_IsMapped(statwin)) {
	    W_UnmapWindow(statwin);
	} else {
	    W_MapWindow(statwin);
	}
	break;
    case 'Z':                   /* A = toggle new stat mode */
        if (W_IsMapped(newstatwin)) {
            W_UnmapWindow(newstatwin);
        } else {
            W_MapWindow(newstatwin);
        }
        break;
    case 'M':			/* map the motd window */
	showMotdWin();
	break;
    case 'N':			/* N = Toggle Name mode */
	namemode = !namemode;
	if (optionWin)
	    optionredrawoption(&namemode);
	break;
    case 'i':			/* i = get information */
    case 'I':			/* I = get extended information */
    case control('i'):		/* ^i = info on a planet [BDyess] */
	if (!infomapped)
	    inform(data->Window, data->x, data->y, key);
	else
	    destroyInfo();
	break;
    case 'j':			/* j = plotter line toggle [BDyess] */
        plotter = !plotter;
	if(plotter == 0) clearplotter = 1;
        break;
    case 'h':			/* h = Map help window */
	if (W_IsMapped(helpWin)) {
	    W_UnmapWindow(helpWin);
	} else {
	    W_MapWindow(helpWin);
	}
	if (optionWin)
	    optionredrawtarget(helpWin);
	break;
    case 'w':			/* w = map war stuff */
	if (W_IsMapped(war))
	    W_UnmapWindow(war);
	else
	    warwindow();
	break;
    case '+':			/* UDP: pop up UDP control window */
	if (udpWin != NULL && W_IsMapped(udpWin))
	    udpdone();
	else {
	    char    buf[80];
	    udpwindow();
	    sprintf(buf, "UDP client version %.1f",
		    (float) UDPVERSION / 10.0);
	    warning(buf);
	}
	if (optionWin)
	    optionredrawtarget(udpWin);
	break;
    case '=':			/* UDP: request for full update */
	sendUdpReq(COMM_UPDATE);
	break;

    case 9:			/* tab */
    case control('m'):		/* because you can't remap to tab */
	                        /* actually, you can, put a literal
				   TAB in a ckeymap entry and it works.
				   So should we keep this? -JR */
	if(paradise) {
	    blk_zoom = !blk_zoom;
	    redrawall = 1;
	    auto_zoom_timer = udcounter+autoZoomOverride;
	    if (optionWin)
		optionredrawoption(&blk_zoom);
	}
	break;
    case '~':
	if (spWin != NULL && W_IsMapped(spWin))
	    spdone();
	else
	    spwindow();
	if (optionWin)
	    optionredrawtarget(spWin);
	break;
    case '\\':
	sendShortReq(SPK_SALL);
	break;
    case '|':
	sendShortReq(SPK_ALL);
	break;
    case ',':
	if (W_IsMapped(pStats)) {
	    W_UnmapWindow(pStats);
	} else {
	    W_MapWindow(pStats);
	    redrawPStats();
	}
	if (optionWin)
	    optionredrawtarget(pStats);
	break;
    case '&':			/* reread defaults file */
	if (defaultsFile) {
	    char    buf[150];
	    sprintf(buf, "Reading defaults from %s", defaultsFile);
	    warning(buf);
	    freeDefaults();
	    defaultsFile = initDefaults(defaultsFile);
	    resetDefaults();
	} else {
	    warning("No defaults file to read from!");
	}
	break;
    case '(':
	rotate--;
	if (rotate < 0)
	    rotate = 3;
	if (optionWin)
	    optionredrawoption(&rotate);
	rotate_all();
	break;
    case ')':
	rotate++;
	if (rotate > 3)
	    rotate = 0;
	if (optionWin)
	    optionredrawoption(&rotate);
	rotate_all();
	break;

    case control('r'):
	stopRecorder();
	break;

    case '\"':
	showToolsWin();
	break;
    default:
	W_Beep();
	break;
    }
}

static void
buttonaction(W_Event *data)
{
    unsigned char course;

    if (messageon)
	message_off();		/* ATM */

    if (data->Window != w && data->Window != mapw
	&& data->Window != scanwin)
	return;

    data->key--;
    if (data->key >= 0 && data->key < 12) {
	if (myship->s_buttonmap[data->key] != '\0') {
	    data->key = myship->s_buttonmap[data->key];
	    keyaction(data);
	    return;
	} else
	    data->key = data->key % 3;
	/* if alt key is pressed, do default */
    } if (data->key > 11)
	data->key -= 12;
    data->key++;
    if (data->key == W_RBUTTON) {
	course = getcourse(data->Window, data->x, data->y);
	set_course(course);
    } else if (data->key == W_LBUTTON) {
	course = getcourse(data->Window, data->x, data->y);
	sendTorpReq(course);
    } else if (data->key == W_MBUTTON) {
	course = getcourse(data->Window, data->x, data->y);
	sendPhaserReq(course);
    }
}

/*
 * changed from unsigned char to irint() for precise rounding (from Leonard
 * Dickens)
 *
 *  changed from irint (which ULTRIX doesn't have in its math.h header) to
 * floor(x+0.5) for portability.
 */



int
getcourse(W_Window ww, int x, int y)
{
    if (ww == mapw) {
	int     me_x, me_y;

	me_x = scaleMapX(me->p_x);
	me_y = scaleMapY(me->p_y);

	return (unsigned char)(int)
	  floor(0.5 + atan2((double) (x - me_x),
			    (double) (me_y - y))
		         / 3.14159 * 128.);


    } else {

      double	result = atan2((double) (x - center),
			       (double) (center - y)) / 3.14159 * 128.;

      return (unsigned char) (int) floor (result + 0.5);
    }
}

static void
scan(W_Window window, int x, int y)
{
}
