/*
 * getname.c
 *
 * Kevin P. Smith 09/28/88
 *
 */
#include "copyright2.h"

#include "config.h"
#include <stdlib.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
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

static char tempname[16];
static char password1[16];
static char password2[16];
static int state, autolog;
static char username[32] = "****";
#define ST_GETNAME 0
#define ST_GETPASS 1
#define ST_MAKEPASS1 2
#define ST_MAKEPASS2 3
#define ST_DONE 4

/* Prototypes */
static void adjustString P((char ch, char *str, char *defname));
static void checkpassword P((void));
static void displayStartup P((char *defname));
static void loaddude P((void));
static void makeNewGuy P((void));
static void loginproced P((int ch, char *defname));

void
noautologin(void)
{
    char   *tempstr;

    autolog = 0;
    *defpasswd = *password1 = *password2 = '\0';
    tempstr = "Automatic login failed";
    W_WriteText(w, 100, 100, textColor, tempstr, strlen(tempstr),
		W_BoldFont);
}

/* Let person identify themselves from w */
void
getname(char *defname, char *def_passwd)
{
    W_Event event;
    register int ch = 0;
    int     secondsLeft = 99, laststate;
    char    tempstr[40];
    long    lasttime;
    register int j = 0;

    if (playback)
	return;

    /* shows the credits in the map window [BDyess] */
    showCredits(mapw);

    autolog = (*def_passwd && *defname) ? 1 : 0;

    {
	struct passwd *passwd;

	passwd = getpwuid(getuid());
	if (passwd)		/* believe it or not, getpwuid failed on me -
				   RF */
	    strcpy(username, passwd->pw_name);
    }

    memset(mystats, 0, sizeof(struct stats));
    mystats->st_tticks = 1;
    mystats->st_flags =
	(ST_NOBITMAPS * (!sendmotdbitmaps) +
	 ST_KEEPPEACE * keeppeace +
	 0);
    lasttime = time(NULL);

    if (ghoststart)
	return;

    tempname[0] = '\0';
    password1[0] = '\0';
    password2[0] = '\0';
    laststate = state = ST_GETNAME;
    displayStartup(defname);
    for (;;) {
	if (isServerDead()) {
	    printf("Ack!  We've been ghostbusted!\n");
	    EXIT(0);
	}
	if (lasttime != time(NULL)) {
	    lasttime++;
	    secondsLeft--;
	    if (!autolog) {
		sprintf(tempstr, "Seconds to go: %d ", secondsLeft);
		W_WriteText(w, 150, 400, textColor, tempstr, 
			    strlen(tempstr), W_BoldFont);
		/* flush buffer if one exists [BDyess] */
		if(W_IsBuffered(w)) W_DisplayBuffer(w);	
	    }
	    if (secondsLeft == 0) {
		me->p_status = PFREE;
		printf("Auto-Quit\n");
		/* flush buffer if one exists [BDyess] */
		if(W_IsBuffered(w)) W_DisplayBuffer(w);	
		EXIT(0);
	    }
	}
	if (state == ST_DONE) {
	    W_ClearWindow(w);
	    return;
	}
	readFromServer();	/* Just in case it wants to say something */

	if (autolog) {
	    switch (state) {
	    case ST_GETNAME:
		tempname[0] = '\0';
		ch = 13;
		j = 0;
		break;

	    case ST_GETPASS:
	    case ST_MAKEPASS1:
	    case ST_MAKEPASS2:
		ch = def_passwd[j++];
		if (ch == '\0') {
		    j = 0;
		    ch = 13;
		}
		break;

	    default:
		break;
	    }

	    loginproced(ch, defname);

	}
	laststate = state;

	if (!W_EventsPending())
	    continue;
	W_NextEvent(&event);
	if (event.Window == mapw && (int)event.type == W_EV_EXPOSE)
	    showCredits(mapw);
	if (event.Window != w)
	    continue;
	switch ((int) event.type) {
	case W_EV_EXPOSE:
            displayStartup(defname);
	    break;
	case W_EV_KEY:
	    ch = event.key;
	    if (!autolog)
		loginproced(ch, defname);
	}
    }
}


static void
loginproced(int ch, char *defname)
{
    if (ch > 255)
	ch -= 256;		/* was alt key, ignore it */
    if (ch == 10)
	ch = 13;
    if ((ch == ('d' + 128) || ch == ('D' + 128)) && state == ST_GETNAME && *tempname == '\0') {
	EXIT(0);
    }
    if ((ch < 32 || ch > 127) && ch != 21 && ch != 13 && ch != 8)
	return;
    switch (state) {
    case ST_GETNAME:
	if (ch == 13) {
	    if (*tempname == '\0') {
		strcpy(tempname, defname);
	    }
	    loaddude();
	    displayStartup(defname);
	} else {
	    adjustString(ch, tempname, defname);
	}
	break;
    case ST_GETPASS:
	if (ch == 13) {
	    checkpassword();
	    displayStartup(defname);
	} else {
	    adjustString(ch, password1, defname);
	}
	break;
    case ST_MAKEPASS1:
	if (ch == 13) {
	    state = ST_MAKEPASS2;
	    displayStartup(defname);
	} else {
	    adjustString(ch, password1, defname);
	}
	break;
    case ST_MAKEPASS2:
	if (ch == 13) {
	    makeNewGuy();
	    displayStartup(defname);
	} else {
	    adjustString(ch, password2, defname);
	}
	break;
    }
}

/* Query dude.
 */
static void
loaddude(void)
{
    if (strcmp(tempname, "Guest") == 0 || strcmp(tempname, "guest") == 0) {
	loginAccept = -1;
	sendLoginReq(tempname, "", username, 0);
	state = ST_DONE;
	me->p_pos = -1;
	me->p_stats.st_tticks = 1;	/* prevent overflow */
	strcpy(me->p_name, tempname);
	while (loginAccept == -1) {
	    socketPause(1, 0);
	    readFromServer();
	    if (isServerDead()) {
		printf("Server is dead!\n");
		EXIT(0);
	    }
	}
	if (loginAccept == 0) {
	    printf("Hmmm... The SOB server won't let me log in as guest!\n");
	    EXIT(0);
	}
	return;
    }
    /* Ask about the user */
    loginAccept = -1;
    sendLoginReq(tempname, "", username, 1);
    while (loginAccept == -1) {
	socketPause(1, 0);
	readFromServer();
	if (isServerDead()) {
	    printf("Server is dead!\n");
	    EXIT(0);
	}
    }
    *password1 = *password2 = 0;
    if (loginAccept == 0) {
	state = ST_MAKEPASS1;
    } else {
	state = ST_GETPASS;
    }
}

/* Check dude's password.
 * If he is ok, move to state ST_DONE.
 */
static void
checkpassword(void)
{
    char   *s;

    sendLoginReq(tempname, password1, username, 0);
    loginAccept = -1;
    while (loginAccept == -1) {
	socketPause(1, 0);
	readFromServer();
	if (isServerDead()) {
	    printf("Server is dead!\n");
	    EXIT(0);
	}
    }
    if (loginAccept == 0) {
	if (!autolog) {
	    s = "Bad password!";
	    W_WriteText(w, 100, 100, textColor, s, strlen(s), W_BoldFont);
	    /* flush buffer if one exists [BDyess] */
	    if(W_IsBuffered(w)) W_DisplayBuffer(w);	
	    (void) W_EventsPending();
	    sleep(3);
	    W_ClearWindow(w);
	} else
	    noautologin();
	*tempname = 0;
	state = ST_GETNAME;
	return;
    }
    strcpy(me->p_name, tempname);
    sendmotdbitmaps = !((me->p_stats.st_flags / ST_NOBITMAPS) & 1);
    keeppeace = (me->p_stats.st_flags / ST_KEEPPEACE) & 1;
    state = ST_DONE;
}

/* Make the dude with name tempname and password password1.
 * Move to state ST_DONE.
 */
static void
makeNewGuy(void)
{
    char   *s;

    if (strcmp(password1, password2) != 0) {
	if (!autolog) {
	    s = "Passwords do not match";
	    W_WriteText(w, 100, 120, textColor, s, strlen(s), W_BoldFont);
	    /* flush buffer if one exists [BDyess] */
	    if(W_IsBuffered(w)) W_DisplayBuffer(w);	
	    (void) W_EventsPending();
	    sleep(3);
	    W_ClearWindow(w);
	} else
	    noautologin();
	*tempname = 0;
	state = ST_GETNAME;
	return;
    }
    /* same routine! */
    checkpassword();
}

static void
adjustString(char ch, char *str, char *defname)
{
    if (ch == 21) {
	*str = '\0';
	if (state == ST_GETNAME)
	    displayStartup(defname);
    } else if (ch == 8 || ch == '\177') {
	if ((int) strlen(str) > 0) {
	    str[strlen(str) - 1] = '\0';
	    if (state == ST_GETNAME)
		displayStartup(defname);
	}
    } else {
	if (strlen(str) == 15)
	    return;
	str[strlen(str) + 1] = '\0';
	str[strlen(str)] = ch;
	if (state == ST_GETNAME)
	    displayStartup(defname);
    }
}

/* Draws entry screen based upon state. */
static void
displayStartup(char *defname)
{
    char    s[100];
    char   *t;

    if (state == ST_DONE || autolog)
	return;
    t = "Enter your name.  Use the name 'guest' to remain anonymous.";
    W_WriteText(w, 100, 30, textColor, t, strlen(t), W_BoldFont);
    t = "Type ^D (Ctrl - D) to quit.";
    W_WriteText(w, 100, 40, textColor, t, strlen(t), W_BoldFont);
    sprintf(s, "Your name (default = %s): %s               ", defname, tempname);
    W_WriteText(w, 100, 50, textColor, s, strlen(s), W_BoldFont);
    if (state == ST_GETPASS) {
	t = "Enter password: ";
	W_WriteText(w, 100, 60, textColor, t, strlen(t), W_BoldFont);
    }
    if (state > ST_GETPASS) {
	t = "You need to make a password.";
	W_WriteText(w, 100, 70, textColor, t, strlen(t), W_BoldFont);
	t = "So think of a password you can remember, and enter it.";
	W_WriteText(w, 100, 80, textColor, t, strlen(t), W_BoldFont);
	t = "What is your password? :";
	W_WriteText(w, 100, 90, textColor, t, strlen(t), W_BoldFont);
    }
    if (state == ST_MAKEPASS2) {
	t = "Enter it again to make sure you typed it right.";
	W_WriteText(w, 100, 100, textColor, t, strlen(t), W_BoldFont);
	t = "Your password? :";
	W_WriteText(w, 100, 110, textColor, t, strlen(t), W_BoldFont);
    }
    /* flush buffer if one exists [BDyess] */
    if(W_IsBuffered(w)) W_DisplayBuffer(w);	
}
