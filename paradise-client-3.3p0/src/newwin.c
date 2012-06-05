/*
 * newwin.c
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "images.h"
#include "packets.h"
#include "gppackets.h"

#define NRHEADERS	4
/* elapsed time in outfit window [BDyess] */
int     elapsed;

int     newMotdStuff = 0;	/* set to 1 when new motd packets arrive */
static struct piclist *motdPics = NULL;
/*static struct page *currpage = NULL;
static struct page *motddata = NULL;*/

#define LINESPERPAGE	38

#define BOXSIDE		((mapside - 10*BORDER) / 5)
#define TILESIDE	16
#define MESSAGESIZE	20
#define STATSIZE	(MESSAGESIZE * 2 + BORDER*2)
#define YOFF		0

/* Prototypes */
static void saveimages P((void));
static int teamRequest P((int team, int ship));
static int numShips P((int owner));
static int checkBold P((char *line));
static void showPics P((W_Window win));
static void getResources P((char *prog));
static void redrawTeam P((W_Window win, int teamNo, int *lastnum));
static void redrawQuit P((void));
static void showTimeLeft P((int t, int max));

/* from dashboard.c: */
void light_erase P((void));

extern int helpmessages;

void
newwin(char *hostmon, char *progname)
{
    int     i;
    W_Image *image;

    /* W_Initialize(hostmon); */
    getResources(progname);

    /* the x,y passwd to makewindow are the x,y of the top left corner of
       the border.  The actual window starts x+BORDER,y+BORDER down.  The
       given width is the width of the window WITHOUT the border, ie. 
       useable space.  [BDyess] */
    baseWin = W_MakeWindow("netrek", 0, YOFF, WINSIDE + MAPSIDE + 6 * BORDER,
			   WINSIDE + 9 * BORDER + 2 * MESSAGESIZE + 
			   23 * W_Textheight + 2*WIN_EDGE, NULL, 
			   "bomb here", /*no border*/ 0, gColor);
    image = getImage(I_ICON);
    iconWin = W_MakeWindow("netrek_icon", 0, 0, image->width,image->height,NULL,
			   (char *) 0, BORDER, gColor);
    W_SetIconWindow(baseWin, iconWin);
    w = W_MakeWindow("local", BORDER, BORDER, WINSIDE, WINSIDE, baseWin, 
                     (char *) 0, BORDER, foreColor);
    mapw = W_MakeWindow("map", WINSIDE + 3*BORDER, BORDER, MAPSIDE, MAPSIDE, 
                        baseWin, (char *) 0, BORDER, foreColor);
    tstatw = W_MakeWindow("tstat", BORDER, winside+3*BORDER, winside, 
                          STATSIZE, baseWin, "xterm", BORDER, foreColor);
/*    if(xpm) W_SetWindowBackgroundImage(tstatw, getImage(I_DASHBOARD_BG));*/
    warnw = W_MakeWindow("warn", winside+3*BORDER, mapside+3*BORDER, 
    			 mapside, MESSAGESIZE, baseWin, "xterm", BORDER,
			 foreColor);
    messagew = W_MakeWindow("message", winside+3*BORDER, 
               mapside + 5*BORDER + MESSAGESIZE, mapside, MESSAGESIZE, baseWin,
	       "xterm", BORDER, foreColor);
    planetw = W_MakeTextWindow("planet", 10, 10, 75, (MAXPLANETS + 16) / 2, w, 
                               (char *) 0, 2);
    planetw2 = W_MakeTextWindow("planet2", 10, 10, 75, (MAXPLANETS + 16) / 2, 
                                mapw, (char *) 0, 2);
    rankw = W_MakeTextWindow("rank", 50, 100, 65, nranks2 + 8, w, (char*) 0, 2);
    playerw = W_MakeTextWindow("Player", winside + 3*BORDER, 
                               mapside + 7 * BORDER + 2 * MESSAGESIZE,
			       (mapside-2*WIN_EDGE)/W_Textwidth/*82*/, 23,
			       baseWin, (char *) 0, 2);
    helpWin = W_MakeTextWindow("HELP!", 0, 
                               YOFF + winside + 2 * BORDER + 2 * MESSAGESIZE,
		               160, helpmessages / 4 + 1, NULL, (char *) 0, 
			       BORDER);

    initMessageWindows();

    pStats = W_MakeWindow("Network Statistics", 500, 4, pStatsWidth(), pStatsHeight(),
			  NULL, (char *) 0, 1, foreColor);
    udpWin = W_MakeMenu("UDP", winside + 10, -BORDER + 10, 40, UDP_NUMOPTS,
			NULL, 2);

    spWin = W_MakeMenu("network", winside + 10, -BORDER + 10, 40, SPK_NUMFIELDS,
		       NULL, 2);

  toolsWin = W_MakeScrollingWindow("tools", winside + BORDER, BORDER,
				   80, TOOLSWINLEN, NULL, "xterm", BORDER);

    motdWin = W_MakeWindow("Motd"
			   ,-BORDER, -BORDER, winside, winside, NULL,
			   (char *) 0, BORDER, foreColor);

    for (i = 0; i < 4; i++) {
	teamWin[i] = W_MakeWindow(teaminfo[i].shortname, i * (BOXSIDE+2*BORDER),
	                  0, BOXSIDE, BOXSIDE, mapw, (char *) 0, BORDER, 
			  foreColor);
    }
    qwin = W_MakeWindow("quit", 4 * (BOXSIDE+2*BORDER), 0, BOXSIDE, BOXSIDE, 
                        mapw, "pirate", BORDER, foreColor);

/*    statwin = W_MakeWindow("Stats", 422, 13, 160, 95, NULL, (char*)0,
      5, foreColor);*/
    statwin = W_MakeWindow("stats", 422, 13, 160, 80, NULL,
			   (char *) 0, 5, foreColor);
    newstatwin = W_MakeWindow("NewStats", 422, 13, 100 + 2*BORDER, 400+2*BORDER,
                              NULL, (char *) 0, BORDER, foreColor);

#define WARHEIGHT 2
#define WARWIDTH 20
#define WARBORDER BORDER

    war = W_MakeMenu("war", winside + 10, 10, WARWIDTH, 6, baseWin,
		     WARBORDER);


    /* needed to update constants now that local and map window sizes
       are dynamic [BDyess] */
    recalcWindowConstants();
    saveimages();
}

void
mapAll(void)
{
    initinput();
    W_MapWindow(mapw);
    W_MapWindow(tstatw);
    W_MapWindow(warnw);
    W_MapWindow(messagew);
    W_MapWindow(w);
    W_MapWindow(baseWin);

    /*
       since we aren't mapping windows that have root as parent in
       x11window.c (since that messes up the TransientFor feature) we have to
       map them here. (If already mapped, W_MapWindow returns)
    */

    if (checkMapped("planet"))
	W_MapWindow(planetw);
    if (checkMapped("planet2"))
	W_MapWindow(planetw2);
    if (checkMapped("rank"))
	W_MapWindow(rankw);
    if (checkMapped("help"))
	W_MapWindow(helpWin);
    if (checkMapped("Motd"))
	W_MapWindow(motdWin);
    if (checkMapped("review_all"))
	W_MapWindow(messWin[WALL].window);
    if (checkMapped("review_team"))
	W_MapWindow(messWin[WTEAM].window);
    if (checkMapped("review_your"))
	W_MapWindow(messWin[WINDIV].window);
    if (checkMapped("review_kill"))
	W_MapWindow(messWin[WKILL].window);
    if (checkMapped("review_phaser"))
	W_MapWindow(messWin[WPHASER].window);
    if (booleanDefault("player.mapped", 1))
	W_MapWindow(playerw);
    if (booleanDefault("review.mapped", 1))
	W_MapWindow(messWin[WREVIEW].window);
    if (checkMapped("UDP"))
	udpwindow();

}

static void
saveimages(void)
{
    load_default_teamlogos();
}

void
get_N_dispatch_outfit_event(int *team, int *s_type, int *lastplayercount)
{
    W_Event event;
    int     validshipletter = 0;
    static int resetting = 0;
    int     oldresetting;
    int     i;

    oldresetting = resetting;

    W_NextEvent(&event);
    switch ((int) event.type) {
    case W_EV_KEY:
	{
	    struct shiplist *shipscan;
	    validshipletter = 0;
	    shipscan = shiptypes;
	    while (shipscan) {
		if (shipscan->ship->s_letter == event.key) {
		    *s_type = shipscan->ship->s_type;
		    validshipletter = 1;
		    break;
		}
		shipscan = shipscan->next;
	    }
	}

	if (me->p_status == PTQUEUE) {
	    for (i = 0; i < WNUM; i++) {
		if (event.Window == messWin[i].window) {
		    messageWinEvent(&event);
		    break;
		}
	    }
	    if (i != WNUM)
		break;
	    if (event.Window == messagew ||
		event.Window == tstatw ||
		event.Window == warnw)
		smessage(event.key);
	}
	if (event.Window == motdWin) {
	    motdWinEvent(&event);
	    break;
	} else if (event.Window == playerw || event.Window == infow) {
	    /* allow 'i' 'I' and '^i' in playerw [BDyess] */
	    playerwEvent(&event);
	    break;
	} else if (event.Window == w || event.Window == mapw) {
	    switch (event.key) {
#ifdef Q_OUTFITTING
	    case 'q':
		*team = number_of_teams;
		me->p_status = PFREE;
		break;
#endif				/* Q_OUTFITTING */
	    case 'R':
		warning("Are you sure you want to reset your stats?");
		resetting = 1;
		break;
	    case 'y':
		if (resetting) {
		    sendResetStatsReq('Y');
		    warning("OK, your stats have been reset.");
		    resetting = 0;
		}
		break;
	    case 'n':
		if (resetting) {
		    warning("I didn't think so.");
		    resetting = 0;
		}
		break;

	    case 'f':		/* Scroll motd forward */
		if (currpage == NULL)
		    currpage = motddata;
		if (currpage == NULL || currpage->next == NULL)
		    break;
		currpage->next->prev = currpage;
		currpage = currpage->next;
		showMotd(w);
		resetting = 0;
		break;
	    case 'b':		/* Scroll motd backward */
		if (currpage == NULL || currpage->prev == NULL)
		    break;
		currpage = currpage->prev;
		showMotd(w);
		resetting = 0;
		break;
		/* ok, let's have some info windows available on the TQ */

	    default:		/* hmm, something that doesn't have to do
				   with the MOTD, maybe it's an info window
				   request */
		switch (doKeymap(&event)) {
		case 'U':	/* U = Rank list */
		    if (W_IsMapped(rankw)) {
			W_UnmapWindow(rankw);
		    } else {
			W_MapWindow(rankw);
		    }
		    break;
		case 'P':	/* P = Planet list */
		    if (W_IsMapped(planetw)) {
			W_UnmapWindow(planetw);
			W_UnmapWindow(planetw2);
		    } else {
			W_MapWindow(planetw);
			W_MapWindow(planetw2);
		    }
		    break;
		case 'h':	/* h = Map help window */
		    if (W_IsMapped(helpWin)) {
			W_UnmapWindow(helpWin);
		    } else {
			W_MapWindow(helpWin);
		    }
		    if (optionWin)
			optionredrawtarget(helpWin);
		    break;
		case 'O':	/* O = options Window */
		    if (optionWin != NULL && W_IsMapped(optionWin))
			optiondone();
		    else
			optionwindow();
		    break;
		case 'w':	/* w = map war stuff */
		    if (W_IsMapped(war))
			W_UnmapWindow(war);
		    else
			warwindow();
		    break;
		case '&':
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
		}
		break;
	    }

	    break;		/* switch event type */
	}
	if (event.Window == qwin)
	    return;		/* normal keypresses can't make you quit */

	if (event.Window == optionWin) {
	    optionaction(&event);
	    return;
	}
	if (!validshipletter)
	    break;
	/*
	   it wasn't the main window, see if they hit the key in a team
	   window to choose their ship... falling through
	*/
    case W_EV_BUTTON:

	for (i = 0; i < number_of_teams; i++)
	    if (event.Window == teamWin[i]) {
		*team = i;
		break;
	    }
	if (event.Window == qwin) {
	    *team = number_of_teams;
	    me->p_status = PFREE;
	    break;
	}
	/* allow message scrollback [BDyess] */
	for (i = 0; i < WNUM; i++) {
	    if (event.Window == messWin[i].window) {
		messageWinEvent(&event);
		break;
	    }
	}
	/* allow bozo selection in playerw [BDyess] */
	if (event.Window == playerw) {
	    playerwEvent(&event);
	    break;
	} else if (event.Window == war)
	    waraction(&event);
	else if (event.Window == optionWin)
	    optionaction(&event);
	else if (event.Window == motdWin)
	    motdWinEvent(&event);
	else if (event.Window == w) {
	    W_Window tmp = motdWin;
	    motdWin = w;
	    motdWinEvent(&event);
	    motdWin = tmp;
	}
	if (*team != -1 && !teamRequest(*team, *s_type)) {
	    *team = -1;
	}
	break;
    case W_EV_EXPOSE:
	for (i = 0; i < number_of_teams; i++)
	    if (event.Window == teamWin[i]) {
		lastplayercount[i] = -1;	/* force update */
		redrawTeam(teamWin[i], i, &lastplayercount[i]);
		break;
	    }
	if (event.Window == qwin)
	    redrawQuit();
	else if (event.Window == w)
	    showMotd(w);
	else if (event.Window == mapw) {
		showValues(mapw);
	    redraw_death_messages();
	} else
	    /* let the normal expose handler figure out who to redraw */
	    dispatch_W_expose_event(&event);
	break;
    case W_EV_KILL_WINDOW:	/* WM_DESTROY_WINDOW support [BDyess] */
        if(event.Window == baseWin) exit(0);
	else W_UnmapWindow(event.Window);
	break;
    }

    if (oldresetting && resetting) {
	resetting = 0;
	warning("Resetting of stats cancelled");
    }
}

void
new_entrywindow(int *team, int *s_type)
{
    int     i;
    int     lastplayercount[4];	/* number of players on each team */
    int     okayMask, lastOkayMask;	/* teams you're allowed to choose */
    char    buf[100];

    /* OUTFIT timeout stuff */
    long    startTime = -1;
    long    lasttime = -1;
    int     spareTime = 0;

    if (fastQuit) {
	*team = -1;
	return;
    }
    lastOkayMask = okayMask = tournMask;

    /* erase packet lights to make Bob happy [BDyess] */
    light_erase();

    /*
       map all team selection windows, and stripe out those that are
       unchoosable
    */
    for (i = 0; i < number_of_teams; i++) {
	W_MapWindow(teamWin[i]);
	lastplayercount[i] = -1;
    }
    W_MapWindow(qwin);

    /* no team selected yet */
    *team = -1;
    /*
       set to team index (0..n-1) to choose a team. set to n if you want to
       quit
    */

    /* I don't know why this restriction is in place - RF */
    if (me->p_whydead != KWINNER && me->p_whydead != KGENOCIDE) {
	showMotd(w);
	W_ClearWindow(mapw);
	showValues(mapw);
	redraw_death_messages();
    }
    do {

	/* set team to n if you want to quit */
	while (!W_EventsPending() && (me->p_status == POUTFIT ||
				      me->p_status == PTQUEUE)) {
	    /* no window events, just process socket stuff */
	    fd_set  mask;

	    light_erase();

	    readFromServer();

	    if (me->p_status == POUTFIT || me->p_status == PTQUEUE) {
		/* wait up to a half-second for input from the window system */
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 500000;

		FD_ZERO(&mask);
		FD_SET(W_Socket(), &mask);
		select(W_Socket() + 1, &mask, 0, 0, &tv);
	    }

#if SHOW_MAP_AT_MOTD_DATA_ITEM_IMPLEMENTED
	    if(showMapAtMotd) {
		map();
	    }
#endif
	    redraw_death_messages();

	    if (me->p_status == PTQUEUE)
		startTime = -1;

	    if (me->p_status == POUTFIT) {
		/* time only elapses in OUTFIT mode */

		if (startTime == -1) {	/* we were on the tqueue */
		    /* I hate this [BDyess] */
		    W_Deiconify(baseWin);	/* we changed status.  alert
						   the user */
		    startTime = time(0);
		    spareTime = 480;	/* Allow them extra time, as long */
		    /* as they are active */
		}
		elapsed = time(0) - startTime;

		if (elapsed > autoQuit) {
		    printf("Auto-Quit.\n");
		    *team = number_of_teams;
		    break;
		}
	    }
	    if (lasttime != time(0)) {
		if (W_IsMapped(playerw))
		    playerlist2();

		if (newMotdStuff) {
		    showMotd(w);
			showValues(mapw);
		    redraw_death_messages();
		}
		if (me->p_status == POUTFIT) {
		    showTimeLeft(elapsed, autoQuit);
		}
		lasttime = time(0);
	    }
	    okayMask = tournMask;

	    /* redraw those windows whose choosable status has changed */
	    for (i = 0; i < number_of_teams; i++) {
		if ((okayMask ^ lastOkayMask) & (1 << i)) {
		    lastplayercount[i] = -1;	/* force update */
		}
		redrawTeam(teamWin[i], i, &lastplayercount[i]);
	    }
	    lastOkayMask = okayMask;
	}

	if (playback)  /* silly.  Shouldn't even be mapping team windows. */
	    break;
	/* they quit or ran out of time */
	if (*team == number_of_teams) {
	    me->p_status = PFREE;	/* exit outer while loop */
	    break;
	}
	/*
	   this makes them eventually run out of time no matter how awake
	   they are.  Only affects the OUTFIT screen.
	*/
	if (me->p_status == POUTFIT && startTime != -1) {
	    if (time(0) - startTime <= spareTime) {
		spareTime -= time(0) - startTime;
		startTime = time(0);
	    } else {
		startTime += spareTime;
		spareTime = 0;
	    }
	}
	if (!W_EventsPending())
	    continue;

	/* ok, there's a window event pending */

	/* thiswill set p_status to PFREE if they decide to quit */
	get_N_dispatch_outfit_event(team, s_type, lastplayercount);

    } while ((me->p_status == POUTFIT ||
	      me->p_status == PTQUEUE)
	     && (!pb_update)
	);

    if (*team >= 0) {
	strcpy(buf, "Welcome aboard ");
	if (paradise)
	    strcat(buf, ranks2[me->p_stats2.st_rank].name);
	else
	    strcat(buf, ranks[me->p_stats.st_rank].name);
	sprintf(buf, "Welcome aboard %s!", get_players_rank_name(me));
	warning(buf);
    }
    if (playback) {
	extern int lastTeamReq;
	*team = me->p_teami = lastTeamReq;
    } else
	/* if they quit or ran out of time */
    if (me->p_status == PFREE)
	*team = -1;
    else if (me->p_status == PALIVE ||
	     me->p_status == POBSERVE)
	if (*team == -1)
	    *team = me->p_teami;
	else
	    me->p_teami = *team;


    for (i = 0; i < number_of_teams; i++)
	W_UnmapWindow(teamWin[i]);
    W_UnmapWindow(qwin);
}

/* Attempt to pick specified team & ship */
static int
teamRequest(int team, int ship)
{
    int     lastTime;

    extern int lastTeamReq;

    if (!playback)
	lastTeamReq = team;
    pickOk = -1;
    sendTeamReq(team, ship);
    lastTime = time(NULL);
    while (pickOk == -1) {
	if (lastTime + 3 < time(NULL)) {
	    sendTeamReq(team, ship);
	    lastTime = time(NULL);
	}
	socketPause(0, 20000);
	readFromServer();
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
	    pickOk = 0;
	    break;
	}
    }

    if (pickOk) {
	me->p_status = PALIVE;	/* we got a ship.  We must be alive */
	timeBank[T_SHIP] = time(NULL);
    }
    return (pickOk);
}

static int
numShips(int owner)
{
    int     i, num = 0;
    struct player *p;

    for (i = 0, p = players; i < nplayers; i++, p++)
	if ((p->p_status == PALIVE || p->p_status == PTQUEUE)
	    && p->p_teami == owner)
	    num++;
    return (num);
}

/* Determine if that line should be highlighted on sign-on screen */
/* Which is done when it is the players own score being displayed */
static int
checkBold(char *line)
{
    char   *s, *t;
    int     i;
    int     end = 0;

    if ((int) strlen(line) < 60)
	return (0);
    s = line + 4;
    if (!me)
	return (0);
    t = me->p_name;

    for (i = 0; i < 16; i++) {
	if (!end) {
	    if (*t == '\0')
		end = 1;
	    else if (*t != *s)
		return (0);
	}
	if (end) {
	    if (*s != ' ')
		return (0);
	}
	s++;
	t++;
    }
    return (1);
}

struct list {
    char    bold;
    struct list *next;
    char   *data;
};
static struct list *sysdefptr = NULL;

void
showMotd(W_Window win)
{
    int     i;
    struct list *data;
    int     count;
    int     headernum;

    newMotdStuff = 0;		/* clear the flag */

    if (currpage == NULL)
	currpage = motddata;
    if (currpage == NULL)
	return;
    if (!W_IsMapped(win))
	return;

    headernum = currpage->page % NRHEADERS;
    W_ClearWindow(win);
    if(xpm) 
      W_DrawImageNoClip(win, 0, 0, 0, getImage(I_HEADER), 0);
    else 
    {
      W_Image *headerA, *headerB, *header1;
      headerA = getImage(I_HEADERA);
      headerB = getImage(I_HEADERB);
      header1 = getImage(I_HEADER1);
      W_DrawImageNoClip(win, 0, 0, 0, headerA, foreColor);
      W_DrawImageNoClip(win, headerA->width, 0, 0, 
                        headerB, foreColor);
      W_DrawImageNoClip(win, headerA->width, 
                       headerB->height,
		       0,
		       getImage(I_HEADER1 + headernum),
		       foreColor);
      if (headernum == 2) {	/* fill in client: */
	  /* note: font dependant */
      } else if (headernum == 3) {/* fill in server: */
	  ;
      }
    }
    if (currpage->first) {
	currpage->first = 0;
	data = currpage->text;
	while (data != NULL) {
	    data->bold = checkBold(data->data);
	    data = data->next;
	}
    }
    data = currpage->text;
    count = LINESPERPAGE;	/* Magical # of lines to display */
    i = getImage(I_HEADERA)->height / (paradise ? 10 : W_Textheight) + 1;
    while (count > 0) {
	if (data == NULL)
	    break;

	if (data->bold) {
	    W_WriteText(win, 20, i * (paradise ? 10 : W_Textheight), textColor, data->data,
			strlen(data->data), W_BoldFont);
	} else {
	    W_WriteText(win, 20, i * (paradise ? 10 : W_Textheight), textColor, data->data,
			strlen(data->data), W_RegularFont);
	}
	data = data->next;
	count--;
	i++;
    }
    if (win == w) {
	count = (int)W_StringWidth(blk_refitstring, W_RegularFont) / 2;
	W_WriteText(mapw, 250 - count, 480, textColor, blk_refitstring,
		    strlen(blk_refitstring), W_RegularFont);
    }
    showPics(win);
/*    showValues(mapw); Should be handled in event loop now RF */

    /* keep the warning visible [BDyess] */
    if(hudwarning && warncount) {
      W_MaskText(w, center - (warncount / 2) * W_Textwidth, HUD_Y, W_Green,
                  warningbuf, warncount, W_RegularFont);
    }
    /* flush buffer if one exists [BDyess] */
    if(W_IsBuffered(win)) W_DisplayBuffer(win);	
}

static void
showPics(W_Window win)
{
    struct piclist *temp;
    int     page;

    page = currpage->page;
    temp = motdPics;

    while (temp != NULL) {
	if (page == temp->page) {
	    if (temp->thepic)
		W_DrawImage(win, temp->x, temp->y, 0, temp->thepic, foreColor);
	    else {
		W_MakeLine(win, temp->x, temp->y,
			   temp->x + temp->width - 1, temp->y + temp->height - 1, W_Grey);
		W_MakeLine(win, temp->x, temp->y + temp->height - 1,
			   temp->x + temp->width - 1, temp->y, W_Grey);
		W_MakeLine(win, temp->x, temp->y,
			   temp->x + temp->width - 1, temp->y, W_Grey);
		W_MakeLine(win, temp->x, temp->y,
			   temp->x, temp->y + temp->height - 1, W_Grey);
		W_MakeLine(win, temp->x, temp->y + temp->height - 1,
			   temp->x + temp->width - 1, temp->y + temp->height - 1, W_Grey);
		W_MakeLine(win, temp->x + temp->width - 1, temp->y + temp->height - 1,
			   temp->x + temp->width - 1, temp->y, W_Grey);
	    }
	}
	temp = temp->next;
    }
}

/*
 * ATM: show the current values of the .sysdef parameters.
 */
void
showValues(W_Window win)
{
    int     i;
    struct list *data;

    /* try to find the start of the info */
    data = sysdefptr;

    for (i = 12; i < 50; i++) {
	if (data == NULL)
	    break;
	if (data->data[0] == '+')	/* quick boldface hack */
	    W_WriteText(win, 20, i * W_Textheight, textColor, data->data + 1,
			strlen(data->data) - 1, W_BoldFont);
	else
	    W_WriteText(win, 20, i * W_Textheight, textColor, data->data,
			strlen(data->data), W_RegularFont);
	data = data->next;
    }
    W_DisplayBuffer(win);
}

#define	BETWEEN_PAGES	0
#define	IN_PAGE		1
#define	IN_SYSDEF	3

static int motdlinestate = BETWEEN_PAGES;
static int pagecount = 0;
static struct list **temp = NULL;
static struct page **ptemp = NULL;
static int linecount = 0;
static struct piclist **motd_buftail = &motdPics;

void
erase_motd(void)
{
    struct piclist *temppic;
    struct page *temppage;
    struct list *templist;

    while (motdPics) {
	temppic = motdPics;
	motdPics = temppic->next;
	if (temppic->thepic)
	    W_FreeImage(temppic->thepic);
	free(temppic);
    }
    motd_buftail = &motdPics;

    while (motddata) {
	temppage = motddata;
	motddata = temppage->next;
	while (temppage->text) {
	    templist = temppage->text;
	    temppage->text = templist->next;
	    free(templist->data);
	    free(templist);
	}
	free(temppage);
    }
    motdlinestate = BETWEEN_PAGES;
    currpage = 0;
    pagecount = 0;
    temp = 0;
    ptemp = 0;
    linecount = 0;

    while (sysdefptr) {
	templist = sysdefptr;
	sysdefptr = templist->next;
	free(templist->data);
	free(templist);
    }
}

void
newMotdPic(int x, int y, int width, int height, char *bits, int page)
{
    struct piclist *tmp;

    {
	struct motd_pic_spacket dummy;
	if ((width + 7) / 8 * height > sizeof(dummy.bits) && bits) {
	    fprintf(stderr, "MOTD picture from server is too big!  %dx%d couldn't possibly fit in the %d data bytes of the packet\n",
		    width, height, (int) sizeof(dummy.bits));
	    return;
	}
    }
    if ((currpage && page == currpage->page) || page == 0)
	newMotdStuff = 1;	/* set flag for event loop */

    tmp = (*motd_buftail) = (struct piclist *) malloc(sizeof(struct piclist));
    tmp->next = NULL;
    tmp->x = x;
    tmp->y = y;
    tmp->width = width;
    tmp->height = height;
    tmp->thepic = bits ? W_BitmapToImage(width, height, bits) : 0;
    tmp->page = page;
    motd_buftail = &(tmp->next);
}

void
newMotdLine(char *line)
{

    /*
       Do this first.  That way we don't even have to worry about it at all.
    */

    if (strncmp("BLK: ", line, 5) == 0) {
	blk_parsemotd(line);
	return;
    }
    if (strncmp("\t@@@", line, 4) == 0 && motdlinestate != IN_SYSDEF) {
	motdlinestate = IN_SYSDEF;
	temp = &sysdefptr;
    }
    if (strncmp("\t@@b", line, 4) == 0 && motdlinestate == IN_PAGE)
	motdlinestate = BETWEEN_PAGES;

    if (motdlinestate == BETWEEN_PAGES ||
	(motdlinestate == IN_PAGE && linecount >= LINESPERPAGE)) {
	if (motddata == NULL)
	    ptemp = &motddata;
	(*ptemp) = (struct page *) malloc(sizeof(struct page));
	(*ptemp)->next = NULL;
	(*ptemp)->first = 1;
	(*ptemp)->prev = NULL;
	(*ptemp)->page = pagecount++;
	temp = &((*ptemp)->text);
	(*ptemp)->text = NULL;
	ptemp = &((*ptemp)->next);
	motdlinestate = IN_PAGE;
	linecount = 0;
    }
    if (strncmp("\t@@", line, 3) == 0)
	return;

    if (!currpage ||
	(pagecount - 1) == currpage->page ||
	motdlinestate == IN_SYSDEF)
	newMotdStuff = 1;	/* set flag for event loop */

    (*temp) = (struct list *) malloc(sizeof(struct list));
    (*temp)->next = NULL;
    (*temp)->data = (char *) malloc(strlen(line) + 1);
    strcpy((*temp)->data, line);
    temp = &((*temp)->next);

    if (motdlinestate == IN_PAGE)
	linecount++;
}

/*ARGSUSED*/
static void
getResources(char *prog)
{
    getColorDefs();
}

static void
redrawTeam(W_Window win, int teamNo, int *lastnum)
{
    char    buf[BUFSIZ];
    int     num = numShips(teamNo);

    /* Only redraw if number of players has changed */
    if (*lastnum == num)
	return;

    drawIcon();

    W_ClearWindow(win);
    W_DrawImageNoClip(teamWin[teamNo], 0, 0, 0, teaminfo[teamNo].shield_logo,
                shipCol[teamNo + 1]);
    (void) sprintf(buf, "%d", num);
    W_MaskText(win, 5, 46, shipCol[teamNo + 1], buf, strlen(buf),
	       W_BigFont);
    if (!(tournMask & (1 << teamNo)))
      W_DrawImage(win, 0, 0, 0, getImage(I_NOENTRY), W_Red);
    *lastnum = num;
}

static void
redrawQuit(void)
{
    /* W_WriteText(qwin, 5, 5, textColor, "Quit xtrek", 10, W_RegularFont); */
    if (me->p_status == PTQUEUE) {
	W_ClearArea(qwin, 0, 0, BOXSIDE, BOXSIDE);
	W_DrawImageNoClip(qwin, 0, 0, 0, getImage(I_SAFE), foreColor);
    }
}

void
drawIcon(void)
{
    if (!iconified) {
	me_messages = 0;
	team_messages = 0;
	all_messages = 0;
    }
	iconified = 1;
    if (!infoIcon) {
        W_DrawImageNoClip(iconWin, 0, 0, 0, getImage(I_ICON), W_White);
    } else {			/* code for information icon 1/15 [BDyess] */
	int     side, bottom, top, digits, x, i;
	char    buf[50];
	W_Image *iconimage = getImage(I_ICON);

	W_ClearWindow(iconWin);
	side = iconimage->width / number_of_teams;
	bottom = 0 + side;
	top = 0;
	W_MakeLine(iconWin, 0, bottom, iconimage->width,bottom,W_White);
	for (i = 0; i <= number_of_teams; i++) {	/* draw the vertical
							   lines */
	    x = i * side;
	    x = (x > iconimage->width) ? iconimage->width : x;
	    W_MakeLine(iconWin, x, bottom, x, top, W_White);
	}
	for (i = 0; i < number_of_teams; i++) {
	    sprintf(buf, "%d", numShips(i));
	    digits = strlen(buf);
	    W_WriteText(iconWin, i * side + side / 2 - digits * W_Textwidth / 2,
			bottom - side / 2 - W_Textheight / 2,
			shipCol[i + 1], buf, digits, W_RegularFont);
	}
	if (me->p_status == PALIVE) {
#define TOP iconimage->height-10
	    if (me->p_flags & PFGREEN)
		W_FillArea(iconWin, 0, TOP, iconimage->width,
			   iconimage->height, W_Green);
	    else if (me->p_flags & PFYELLOW)
		W_FillArea(iconWin, 0, TOP,
			   iconimage->width, iconimage->height,
			   W_Yellow);
	    else if (me->p_flags & PFRED)
		W_FillArea(iconWin, 0, TOP,
			   iconimage->width, iconimage->height,
			   W_Red);
	}
	if (me_messages) {
	    sprintf(buf, "Personal: %d", me_messages);
	    W_WriteText(iconWin, 1, bottom + 2, W_White, buf, strlen(buf),
			W_RegularFont);
	}
	if (team_messages) {
	    sprintf(buf, "Team:     %d", team_messages);
	    W_WriteText(iconWin, 1, bottom + 2 + W_Textheight, W_White, buf,
			strlen(buf), W_RegularFont);
	}
	if (all_messages) {
	    sprintf(buf, "All:      %d", all_messages);
	    W_WriteText(iconWin, 1, bottom + 2 + 2 * W_Textheight, W_White, buf,
			strlen(buf), W_RegularFont);
	}
	if (me->p_status == POUTFIT) {
	    sprintf(buf, "Time left: %d", autoQuit - elapsed);
	    W_WriteText(iconWin, 1, bottom + 2 + W_Textheight, W_White, buf,
			strlen(buf), W_RegularFont);
	}
    }
}

#define CLOCK_WID	BOXSIDE
#define CLOCK_HEI	BOXSIDE
#define CLOCK_BDR	0

static void
showTimeLeft(int t, int max)
{
    char    buf[BUFSIZ];
    int     cx, cy, ex, ey, tx, ty;

    if ((max - t) < 10 && t & 1) {
	W_Beep();
	W_Deiconify(baseWin);
    }
    if (iconified)
	drawIcon();
    /* XFIX */
    W_ClearArea(qwin, 0, 0, BOXSIDE, BOXSIDE);

    W_DrawImageNoClip(qwin, 0, 0, 0, getImage(I_CLOCK), foreColor);

    cx = BOXSIDE / 2;
    cy = BOXSIDE / 2 - 6;
    ex = cx - 35 * Sin[((255 * t) / max + 64) % 256];
    ey = cy - 35 * Cos[((255 * t) / max + 64) % 256];
    W_MakeLine(qwin, cx, cy, ex, ey, foreColor);

    sprintf(buf, "%d", max - t);
    cy = BOXSIDE / 2 - 1;
    tx = cx - (int)W_StringWidth(buf, W_RegularFont) / 2;
    ty = cy - W_Textheight;
    W_WriteText(qwin, tx, ty, textColor, buf, strlen(buf), W_RegularFont);
}


void
do_refit(int type)
{
    sendRefitReq(type);
    localflags &= ~PFREFIT;
}
