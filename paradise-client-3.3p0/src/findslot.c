/*
 * findslot.c
 *
 * Kevin Smith 03/23/88
 *
 */
#include "copyright2.h"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#define WAITWIDTH 180
#define WAITHEIGHT 60
#define WAITTITLE 15		/* height of title for wait window */
#define WAITICONHEIGHT 50
#define WAITICONWIDTH  50

/* Prototypes */
static void mapWaitCount P((W_Window countWin, unsigned int count));
static void mapWaitQuit P((W_Window quitwin));
static void mapWaitWin P((W_Window waitWin));
static void mapMotdButtonWin P((W_Window motdButtonWin));
static void mapWaitIcon P((W_Window waitIcon, int count, 
			   int *motdMapLater));

extern int newMotdStuff;	/* from newwin.c */


int
findslot(void)
{
    int     oldcount = -1;
    W_Window waitWin, quitwin, countWin, motdButtonWin, waitIcon;
    W_Event event;
    int     motdMapLater = 0;

    /* Wait for some kind of indication about in/not in */
    while (queuePos == -1) {
	socketPause(1, 0);
	if (isServerDead()) {
	    printf("Augh!  Ghostbusted!\n");
	    EXIT(0);
	}
	readFromServer();
	if (me != NULL) {
	    /* We are in! */
	    printf("*** socket %d, player %d ( -s %d -G %d [-2] ) ***\n",
		   nextSocket, me->p_no,
		   nextSocket, me->p_no);
	    return (me->p_no);
	}
    }

    /* We have to wait.  Make appropriate windows, etc... */
    waitWin = W_MakeWindow("wait", 0, 0, WAITWIDTH, WAITHEIGHT, NULL, 
                           (char *) 0, BORDER, foreColor);
    quitwin = W_MakeWindow("waitquit", 0, WAITTITLE, WAITWIDTH / 3 - BORDER*2,
		           WAITHEIGHT - WAITTITLE - BORDER*2, waitWin, 
			   (char *) 0, BORDER,foreColor);
    countWin = W_MakeWindow("count", WAITWIDTH / 3, WAITTITLE, 
                           WAITWIDTH / 3 - BORDER*2, 
			   WAITHEIGHT - WAITTITLE - BORDER*2, 
			   waitWin, (char *) 0, BORDER, foreColor);
    motdButtonWin = W_MakeWindow("motd_select", 2 * WAITWIDTH / 3, WAITTITLE,
                           WAITWIDTH/3 - 2*BORDER, 
			   WAITHEIGHT - WAITTITLE - 2*BORDER, waitWin, 
			   (char *) 0, BORDER, foreColor);
    waitIcon = W_MakeWindow("wait_icon", 0, 0, WAITICONWIDTH, WAITICONHEIGHT,
			    NULL, NULL, BORDER, foreColor);
    W_SetIconWindow(waitWin, waitIcon);
    /* showMotdWin(); */
    W_MapWindow(waitWin);
    W_MapWindow(countWin);
    W_MapWindow(quitwin);
    W_MapWindow(motdButtonWin);
    for (;;) {
	socketPause(0, 10000);
	readFromServer();
	if (isServerDead()) {
	    printf("We've been ghostbusted!\n");
	    EXIT(0);
	}
	if (newMotdStuff)
	    showMotd(motdWin);
	while (W_EventsPending()) {
	    W_NextEvent(&event);
	    switch ((int) event.type) {
	    case W_EV_KEY:
		if (event.Window == motdWin) {
		    motdWinEvent(&event);
		}
	    case W_EV_BUTTON:	/* fall through */
		if (event.Window == quitwin) {
		    printf("OK, bye!\n");
		    EXIT(0);
		} else if (event.Window == motdButtonWin) {
		    showMotdWin();
		} else if (event.Window == waitIcon) {
		    mapWaitIcon(waitIcon, queuePos, NULL);
		} else if (event.Window == motdWin) {
		    motdWinEvent(&event);
		}
		break;
	    case W_EV_EXPOSE:
		if (event.Window == waitWin) {
		    if (motdMapLater) {
			showMotd(motdWin);
			motdMapLater = 0;
		    }
		    mapWaitWin(waitWin);
		} else if (event.Window == quitwin) {
		    mapWaitQuit(quitwin);
		} else if (event.Window == countWin) {
		    mapWaitCount(countWin, queuePos);
		} else if (event.Window == motdWin) {
		    showMotd(motdWin);
		} else if (event.Window == motdButtonWin) {
		    mapMotdButtonWin(motdButtonWin);
		} else if (event.Window == waitIcon) {
		    mapWaitIcon(waitIcon, queuePos, &motdMapLater);
		}
		break;
	    default:
		break;
	    }
	}
	if (queuePos != oldcount) {
	    mapWaitCount(countWin, queuePos);
	    mapWaitIcon(waitIcon, queuePos, NULL);
	    oldcount = queuePos;
	}
	if (me != NULL) {
	    W_DestroyWindow(waitWin);
	    printf("*** socket %d, player %d ( -s %d -G %d [-2] ) ***\n",
		   nextSocket, me->p_no,
		   nextSocket, me->p_no);
	    return (me->p_no);
	}
    }
}

static void
mapWaitWin(W_Window waitWin)
{
    char   *s = "Netrek: Game is full.";

    W_WriteText(waitWin, 15, 5, textColor, s, (int)strlen(s), W_RegularFont);
}

static void
mapWaitQuit(W_Window quitwin)
{
    char   *s = "Quit";

    W_WriteText(quitwin, 15, 15, textColor, s, (int)strlen(s), W_RegularFont);
}

static void
mapWaitCount(W_Window countWin, unsigned int count)
{
    char   *s = "Wait";
    char   *t = "Queue";
    char    buf[10];
    register int len;

    W_WriteText(countWin, 15, 5, textColor, s, (int)strlen(s), W_RegularFont);
    W_WriteText(countWin, 20, 15, textColor, t, (int)strlen(t), W_RegularFont);
    sprintf(buf, "%d    ", count);
    len = strlen(buf);
    if (count == -1)
	strcpy(buf, "?");
    W_WriteText(countWin, WAITWIDTH / 6 - len * W_Textwidth / 2, 25, textColor, buf,
		len, W_RegularFont);
}

static void
mapMotdButtonWin(W_Window motdButtonWin)
{
    char   *s = "MOTD";

    W_WriteText(motdButtonWin, 15, 15,textColor,s,(int)strlen(s),W_RegularFont);
}

static void
mapWaitIcon(W_Window waitIcon, int count, int *motdMapLater)
{
    char    buf[5];
    int     len;

    sprintf(buf, "%d", count);
    len = strlen(buf);
    if (motdMapLater && W_IsMapped(motdWin)) {
	*motdMapLater = 1;
	showMotdWin();
    }
    W_WriteText(waitIcon, WAITICONWIDTH / 2 - 10, W_Textheight, textColor, buf, len,
		W_BigFont);
}
