/*
 * dmessage.c
 *
 * for the client of a socket based protocol.
 * code for message window scrollback added by Bill Dyess 12/7/93
 */
#include "copyright.h"

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#define MESSAGESIZE	20
#define YOFF		0

/* Prototypes */
void CheckFeatures P((char *m));
void print_message P((char *message, unsigned int flags, unsigned int from,
		      unsigned int to));
void evalFlags P((int type, char *flagList));

void
initMessageWindows(void)
{
    int     i;
    char   *s;
    int	    width = (mapside-2*WIN_EDGE) / W_Textwidth;

    if(width < 80) width = 80;

    messWin[WALL].window = W_MakeScrollingWindow("review_all", 
    				winside + BORDER,
			        YOFF + winside + 3 * BORDER + 2 * MESSAGESIZE, 
				width, 10, 0, "xterm", BORDER);
    messWin[WTEAM].window = W_MakeScrollingWindow("review_team", 
    				winside + BORDER,
      				YOFF + winside + 4 * BORDER + 2 * MESSAGESIZE +
				10 * W_Textheight + 8,
			 	width, 5, 0, "xterm", BORDER);
    messWin[WINDIV].window = W_MakeScrollingWindow("review_your", 
    				winside + BORDER,
     				YOFF + winside + 5 * BORDER + 2 * MESSAGESIZE +
				15 * W_Textheight + 16,
				width, 4, 0, "xterm", BORDER);
    messWin[WKILL].window = W_MakeScrollingWindow("review_kill", 
    				winside + BORDER,
     				YOFF + winside + 6 * BORDER + 2 * MESSAGESIZE +
				19 * W_Textheight + 24,
				width, 6, 0, "xterm", BORDER);
    messWin[WPHASER].window = W_MakeScrollingWindow("review_phaser", 
    				winside + BORDER, 
				YOFF + winside + 7 * BORDER + 2 * MESSAGESIZE +
				24 * W_Textheight + 32, 
				width, 3, 0, "xterm", BORDER);
    messWin[WREVIEW].window = W_MakeScrollingWindow("review", 
    				BORDER,
			      	winside + 7 * BORDER + 2*MESSAGESIZE,
				width, 23, baseWin, "xterm", BORDER);

    /*
       initialize the 'allow' flags.  Any window can display any message.
       [BDyess]
    */
    s = stringDefault("review_all.allow", "MA");
    evalFlags(WALL, s);
    free(s);
    s = stringDefault("review_team.allow", "T");
    evalFlags(WTEAM, s);
    free(s);
    s = stringDefault("review_your.allow", "I");
    evalFlags(WINDIV, s);
    free(s);
    s = stringDefault("review_kill.allow", "K");
    evalFlags(WKILL, s);
    free(s);
    s = stringDefault("review_phaser.allow", "P");
    evalFlags(WPHASER, s);
    free(s);
    s = stringDefault("review.allow", "MATIKP");
    evalFlags(WREVIEW, s);
    free(s);

    for (i = 0; i < WNUM; i++) {
	messWin[i].head = messWin[i].curHead = NULL;
    }
}

void
evalFlags(int type, char *flagList)
{
    messWin[type].flags = 0;
    while (*flagList) {
	switch (*flagList) {
	case 'A':
	    messWin[type].flags |= WA_ALL;
	    break;
	case 'T':
	    messWin[type].flags |= WA_TEAM;
	    break;
	case 'I':
	    messWin[type].flags |= WA_INDIV;
	    break;
	case 'K':
	    messWin[type].flags |= WA_KILL;
	    break;
	case 'P':
	    messWin[type].flags |= WA_PHASER;
	    break;
	case 'M':
	    messWin[type].flags |= WA_MACRO;
	    break;
	default:
	    printf("Invalid allow flag '%c' in allow list, ignoring\n",
		   *flagList);
	    break;
	}
	flagList++;
    }
}

/* handles the mouse and keyboard events that happen in one of the message
   windows.  For key events, if the event is within the team or all window then
   it will automatically start the message for you, with destination to the
   player's team or ALL, respectively.  If it's a button event, it will scroll
   the message window back, forward, or to start (left, right, and middle
   button respectively).  It beeps if there is no messages to scroll (ie at
   the top or bottom already). [BDyess] 12/07/93 */
void
messageWinEvent(W_Event *evt)
{
    W_Window window = evt->Window;
    int     key = evt->key;

    if (!W_IsMapped(window))
	return;

    if (evt->type == W_EV_KEY) {
	if (key == ('e' + 128)) {	/* erase window [BDyess] */
	    W_ClearWindow(window);
	} else if (window == messWin[WALL].window) {
	    smessage_ahead('A', key);
	} else if (window == messWin[WTEAM].window) {
	    smessage_ahead('T', key);
	} else {
	    smessage(key);
	}
    }
}

void
rsvp_borg_call(char *message, int from)
{
    char   *chk;

    if (strlen(message) < 15)
	return;
    for (chk = message + 10; *chk && *chk == ' '; chk++)
	 /* empty body */ ;
    if (*chk)
	return;
    {
        char   *type = "Paradise Client";
	struct  utsname un;
	char    buf[sizeof(type) + MAX_CLIENT_VERSION_STRING + 2*SYS_NMLN + 10];

        uname(&un);
	sprintf(buf, "%s %s (%s %s)", type, CLIENTVERS, un.sysname, un.release);
	if (from == 255) {
	    sendMessage(buf, MGOD, 0);
	} else
	    sendMessage(buf, MINDIV, from);
    }
}

/* logs the given message if the 'logmess' variable is set to one.  It send the
   message to stdout by default, or to a file if one is defined. [BDyess] */
void
logit(char *message)
{
    if (!logmess)
	return;
    if (logfilehandle && logFile) {
	fprintf(logfilehandle, "%s\n", message);
	fflush(logfilehandle);
    } else {
	printf("%s\n", message);
    }
}

static void
writeMessage(char *message, W_Color color, int len, int type)
{
    struct messageWin *j;

    /*printf("writeMessage: message = %s, len = %d\n",message,len);*/

    for (j = &messWin[0]; j < &messWin[WNUM]; j++) {
	if (!(j->flags & type))
	    continue;
	W_WriteText(j->window, 0, 0, color, message, len, 0);
    }
}

/* this function determines the color that the given message should be and
   then passes it to writeMessage [BDyess] */
void
print_message(char *message, unsigned int flags, unsigned int from, 
              unsigned int to)
{
    register int len;
    W_Color color;
    W_Window targwin;

#define    take  MTEAM + MTAKE + MVALID
#define    destroy  MTEAM + MDEST + MVALID
#define    kill  MALL + MKILL + MVALID
#define    killp  MALL + MKILLP + MVALID
#define    killa  MALL + MKILLA + MVALID
#define    bomb  MTEAM + MBOMB + MVALID
#define    team  MTEAM + MVALID
#define    conq  MALL + MCONQ + MVALID

    len = strlen(message);
    if (from == 254) {		/* client passing info around */
	switch (showPhaser) {
	case 0:
	    break;
	case 1:
	    writeMessage(message, textColor, len, WA_KILL | WA_REVIEW);
	    break;
	case 2:
	    writeMessage(message, textColor, len, WA_REVIEW | WA_PHASER);
	    break;
	case 3:
	    writeMessage(message, textColor, len, WA_REVIEW);
	    break;
	}
	return;
    }
    if (from == 255) {
	if (flags == MCONFIG + MINDIV + MVALID) {
	    CheckFeatures(message);
	    return;
	}
	/* From God */
	color = textColor;
    } else {
	/* kludge to fix the occasional incorrect color message */
	if (*message == ' ' && from != me->p_no) {
	    /* XXX fix to handle funky teams */
	    switch (*(message + 1)) {
	    case 'F':
		color = W_Yellow;
		break;
	    case 'R':
		color = W_Red;
		break;
	    case 'K':
		color = W_Green;
		break;
	    case 'O':
		color = W_Cyan;
		break;
	    case 'I':
		color = W_Grey;
		break;
	    default:
		color = playerColor(&(players[from]));
	    }
	} else
	    color = playerColor(&(players[from]));
    }

    /* added/modified to fit with the scrollback feature 1/94 -JR */
    if (!paradise && niftyNewMessages) {
	if (flags == conq) {
	    /* output conquer stuff to stdout in addition to message window */
	    fprintf(stdout, "%s\n", message);
	    if (strstr(message, "kill")) {
		fprintf(stdout, "NOTE: The server here does not properly set message flags\n");
		fprintf(stdout, "You should probably pester the server god to update....\n");
	    }
	}
	if ((flags == team) || (flags == take) || (flags == destroy)) {
	    writeMessage(message, color, len, WA_TEAM | WA_REVIEW);
	    targwin = messWin[WTEAM].window;
	} else if ((flags == kill) || (flags == killp) || (flags == killa) || (flags == bomb)) {
	    writeMessage(message, color, len, 
			 WA_KILL | (reportKills ? WA_REVIEW : 0));
	    targwin = messWin[WKILL].window;
	} else if (flags & MINDIV) {
	    writeMessage(message, color, len, (WA_INDIV | WA_REVIEW));
	    targwin = messWin[WINDIV].window;
	} else if (flags == (MMACRO | MALL)) {
	    writeMessage(message, color, len, (WA_MACRO | WA_REVIEW));
	    targwin = messWin[WALL].window;
	} else {
	    /*
	       if we don't know where the message belongs by this time, stick
	       it in the all board...
	    */
	    writeMessage(message, color, len, (WA_ALL | WA_REVIEW));
	    targwin = messWin[WALL].window;
	}
    } else {

	/*
	   Kludge stuff for report kills...
	*/
	if ((strncmp(message, "GOD->ALL", 8) == 0 &&
	     (strstr(message, "was kill") ||
	      strstr(message, "killed by"))) ||
	      strstr(message, "burned to a crisp by") ||
	      strstr(message, "shot down by") ||
	     (*message != ' ' && strstr(message, "We are being attacked"))) {

	    /* strip off the useless GOD->ALL by adding 9 to message [BDyess] */
	    message += 9;
	    writeMessage(message, color, len, 
			 WA_KILL | (reportKills ? WA_REVIEW : 0));

	    return;
	}
	/*
	   note: messages are kept track of even if the associated window is
	   not mapped.  This allows the window to be later mapped and have
	   all the past messages. [BDyess]
	*/
	if (flags & MTEAM) {
	    writeMessage(message, color, len, WA_TEAM | WA_REVIEW);
	    targwin = messWin[WTEAM].window;
	} else if (flags & MINDIV) {
	    writeMessage(message, color, len, WA_INDIV | WA_REVIEW);
	    targwin = messWin[WINDIV].window;
	} else if (flags == (MMACRO | MALL)) {
	    writeMessage(message, color, len, WA_MACRO | WA_REVIEW);
	    targwin = messWin[WALL].window;
	} else {
	    writeMessage(message, color, len, WA_ALL | WA_REVIEW);
	    targwin = messWin[WALL].window;
	}
    }
    /*
       send warnings to warning or message window, if player doesn't have
       messag es mapped
    */
    if ((use_msgw && (targwin == messWin[WINDIV].window || targwin == messWin[WTEAM].window)) ||
	(!W_IsMapped(targwin) && !W_IsMapped(messWin[WREVIEW].window))) {
	if (!messpend && messagew) {	/* don't collide with messages being
					   written! */
	    W_ClearWindow(messagew);
	    W_WriteText(messagew, 5, 5, color, message, len, W_RegularFont);
	} else
	    warning(message);
    }
}

/* prints the given message by going though several subroutines.  Here, it first
   creates the data structure that holds the message info and logs the message,
   then sends it on it's subroutine path to print it to the correct window(s).
   This is a good place to handle any special-functions that occur due to
   incoming messages.  The data structure for the scroll back is like this:
   each window has a head pointer that points to the top of its message
   list.  Each list only contains the messages that go to it's window.  The
   lists have pointers to the message itself, so that only one copy of the
   message exists even if it is displayed on several windows.  Each list also
   has a current head pointer that points to the record that is at the bottom
   of that current window.  If the current head pointer and the head pointer
   are different, then the window must be scrolled back.  In such a case, new
   messages are still received but not printed.  [BDyess] 12/07/93 */
/* NOT [BDyess] 8/22/95 */
void
dmessage(char *message, unsigned int flags, unsigned int from, unsigned int to)
{
    struct distress dist;
    int len;
    /*char dead[20], alive[20];*/

    /* aha! A new type distress/macro call came in. parse it appropriately */
    if (F_gen_distress && (flags == (MTEAM | MDISTR | MVALID))) {
	if (paradise)
	    printf("RCD: %s\n", message);
	HandleGenDistr(message, from, to, &dist);
	len = makedistress(&dist, message, distmacro[dist.distype].macro);
	if (UseLite)
	    rcdlite(&dist);
	if (len <= 0)
	    return;
	flags ^= MDISTR;
    }
    /*
       keep track of how many queued messages there are for use with the
       infoIcon [BDyess]
    */
    if (infoIcon) {
	if (to == me->p_no && flags & MINDIV) {	/* personal message */
	    me_messages++;
	} else if (flags & MTEAM) {	/* team message */
	    team_messages++;
	} else {		/* message for all */
	    all_messages++;
	}
	if (iconified)
	    drawIcon();
    }
    logit(message);

    /*
       fix for upgrade bug.  Forced UDP would resend numbers, (thinking them
       speed changes) screwing up upgrading on those twinkish sturgeon
       servers. [BDyess]
    */
    if (strncmp(message, "UPG->", 5) == 0)
	upgrading = 1;
    if (upgrading && !(me->p_flags & PFORBIT))
	upgrading = 0;

    if ((from != me->p_no) || pigSelf)
	rsvp_borg_call(message, from);

    /* beep when a personal message is sent while iconified [BDyess] */
    if (to == me->p_no && (flags & MINDIV) && iconified) {
	W_Beep();
    }
    if (from == 255 &&
	strcmp(message, "Tractor beam aborted warp engagement") == 0) {
	me->p_flags &= ~PFWARPPREP;
    }
    /* want a warning for personal kills, so check here [BDyess] */
    /* and personal deaths [BDyess] */
    if(strncmp(message,"GOD->ALL",8) && (0 == strcmp(message,pseudo))) {
      hwarning(message+9);
    }
    print_message(message, flags, from, to);
}

/* I don't know if this works correctly or not.  It is ripped from BRM and has
   been modified a bit to fit.  Right now it doesn't do anything.  [BDyess] */
void
CheckFeatures(char *m)
{
    char    buf[BUFSIZ];
    char   *pek = &m[10];

    if ((int) strlen(m) < 11)
	return;

    while ((*pek == ' ') && (*pek != '\0'))
	pek++;

    strcpy(buf, "Paradise Client: ");

    if (!strcmp(pek, "NO_VIEW_BOX")) {
	allowViewBox = 0;
	strcat(buf, pek);
    }
    if (!strcmp(pek, "NO_CONTINUOUS_MOUSE")) {
	allowContinuousMouse = 0;
	strcat(buf, pek);
    }
    if (!strcmp(pek, "NO_SHOW_ALL_TRACTORS")) {
	allowShowAllTractorPressor = 0;
	strcat(buf, pek);
    }
    if (!strcmp(pek, "HIDE_PLAYERLIST_ON_ENTRY")) {
	allowPlayerlist = 0;
	strcat(buf, pek);
    }
    if (!strcmp(pek, "NO_NEWMACRO")) {
/*      UseNewMacro = 0;*/
	strcat(buf, pek);
    }
    if (!strcmp(pek, "NO_SMARTMACRO")) {
/*      UseSmartMacro = 0;*/
	strcat(buf, pek);
    }
    if (!strcmp(pek, "WHY_DEAD")) {
	why_dead = 1;
	strcat(buf, pek);
    }
    if (!strcmp(pek, "RC_DISTRESS")) {
        F_gen_distress = 1;
/*      distmacro = dist_prefered;*/
	strcat(buf, pek);
    }
    /* what the hell is this? - jmn */
    if (!strncmp(pek, "INFO", 4)) {
	strcat(buf, pek);
    }
    if (strlen(buf) == strlen("Paradise Client: ")) {
	strcat(buf, "UNKNOWN FEATURE: ");
	strcat(buf, pek);
    }
    buf[79] = '\0';

    printf("%s\n", buf);

    W_WriteText(messWin[WREVIEW].window, 0, 0, W_White, buf,(int)strlen(buf),0);
    W_WriteText(messWin[WALL].window, 0, 0, W_White, buf, (int)strlen(buf), 0);
}

void
sendVersion(void)
{
    static int version_sent = 0;
    char    buf[80];

    if (!version_sent) {
	version_sent = 1;
	sprintf(buf, "@ Paradise %s", CLIENTVERS);
	pmessage(buf, me->p_no, MINDIV | MCONFIG);
    }
}
