/*
 * smessage.c
 */
#include "copyright.h"
#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#define MSGLEN 80
#define ADDRLEN 10
#define M_XOFF	5
#define M_YOFF	5

/* XFIX */
#define BLANKCHAR(col, n) W_ClearArea(messagew, M_XOFF+W_Textwidth*(col), \
	M_YOFF, W_Textwidth * (n), W_Textheight);
#define DRAWCURSOR(col) W_WriteText(messagew, M_XOFF+W_Textwidth*(col), \
	M_YOFF, textColor, &cursor, 1, W_RegularFont);

static int lcount;
static char buf[80];
static char cursor = '_';

char    addr, *addr_str;

/* Prototypes */
static void smessage_first P((int ichar));

void
message_expose(void)
{
    if (!messpend)
	return;

    W_WriteText(messagew, M_XOFF, M_YOFF, textColor, addr_str,
		strlen(addr_str), W_RegularFont);
    W_WriteText(messagew, M_XOFF + ADDRLEN * W_Textwidth, M_YOFF, textColor,
		buf, lcount - ADDRLEN, W_RegularFont);
    DRAWCURSOR(lcount);
}

void
smessage_ahead(char head, char ichar)
{
    if (messpend == 0) {
	smessage_first(head);
    }
    smessage(ichar);
}

static void
smessage_first(int ichar)
{
    messpend = 1;

    /* clear out message window in case messages went there */
    W_ClearWindow(messagew);
    if (mdisplayed) {
	BLANKCHAR(0, lastcount);
	mdisplayed = 0;
    }
    /* Put the proper recipient in the window */
    if (/*(ichar == 't') || */
    /* that's a player number! */
    /* YUCK! */
	(ichar == 'T'))
	addr = teaminfo[me->p_teami].letter;
    else
	addr = ichar;
    addr_str = getaddr(addr);
    if (addr_str == 0) {
	/* print error message */
	messpend = 0;
	message_off();
    } else {
	W_WriteText(messagew, M_XOFF, M_YOFF, textColor, addr_str,
		    strlen(addr_str), W_RegularFont);
	lcount = ADDRLEN;
	DRAWCURSOR(ADDRLEN);
    }
}

void
smessage(int ichar)
{
    register int i;
    char    twochar[2], *delim;

    if (messpend == 0) {
	if(lowercaset && ichar == 't')
	    ichar='T';
	else if(lowercaset && (ichar == 'T'))
	    ichar='t';
	smessage_first(ichar);
	return;
    }
    switch ((unsigned char) ichar & ~(0x80)) {
    case '\b':			/* character erase */
    case '\177':
	if (--lcount < ADDRLEN) {
	    lcount = ADDRLEN;
	    break;
	}
	BLANKCHAR(lcount + 1, 1);
	DRAWCURSOR(lcount);
	break;

    case '\027':		/* word erase */
	i = 0;
	/* back up over blanks */
	while (--lcount >= ADDRLEN &&
	       isspace((unsigned char) buf[lcount - ADDRLEN] & ~(0x80)))
	    i++;
	lcount++;
	/* back up over non-blanks */
	while (--lcount >= ADDRLEN &&
	       !isspace((unsigned char) buf[lcount - ADDRLEN] & ~(0x80)))
	    i++;
	lcount++;

	if (i > 0) {
	    BLANKCHAR(lcount, i + 1);
	    DRAWCURSOR(lcount);
	}
	break;

    case '\025':		/* kill line */
    case '\030':
	if (lcount > ADDRLEN) {
	    BLANKCHAR(ADDRLEN, lcount - ADDRLEN + 1);
	    lcount = ADDRLEN;
	    DRAWCURSOR(ADDRLEN);
	}
	break;

    case '\033':		/* abort message */
	BLANKCHAR(0, lcount + 1);
	mdisplayed = 0;
	messpend = 0;
	message_off();
	break;

    case '\r':			/* send message */
	buf[lcount - ADDRLEN] = '\0';
	messpend = 0;
	sendCharMessage(buf, addr);
	BLANKCHAR(0, lcount + 1);
	mdisplayed = 0;
	lcount = 0;
	break;

    default:			/* add character */
	if (lcount >= 79) {	/* send mesg & continue mesg */
             if (addr == 'M')
                  if ((delim = strchr(buf, '>') + 1) == NULL) {
                       W_Beep();
                       break;
                  }
                  else {
                       i = delim - buf;
                       buf[lcount - ADDRLEN + 1] = '\0';
                       sendCharMessage(buf, addr);
                       memset(delim, '\0', sizeof(buf) - i);
                       BLANKCHAR(i, lcount + 1);
                       W_WriteText(messagew, M_XOFF, M_YOFF, textColor,
                                   addr_str, strlen(addr_str), W_RegularFont);
                       W_WriteText(messagew, 
		                   M_XOFF+(strlen(addr_str) + 1) * W_Textwidth,
                                   M_YOFF, textColor, buf, strlen(buf), 
				   W_RegularFont);
                       lcount = i + ADDRLEN;
                       DRAWCURSOR(i + ADDRLEN);
                  }
             else {
                  buf[lcount - ADDRLEN + 1] = '\0';
                  sendCharMessage(buf, addr);
                  BLANKCHAR(0, lcount + 1);
                  W_WriteText(messagew, M_XOFF, M_YOFF, textColor, addr_str,
                              strlen(addr_str), W_RegularFont);
                  lcount = ADDRLEN;
                  DRAWCURSOR(ADDRLEN);
             }
	}
	if (iscntrl((unsigned char) ichar & ~(0x80)))
	    break;
	twochar[0] = ichar;
	twochar[1] = cursor;
	W_WriteText(messagew, M_XOFF + W_Textwidth * lcount, M_YOFF,
		    textColor, twochar, 2, W_RegularFont);
	buf[(lcount++) - ADDRLEN] = ichar;
	break;
    }
}

void
sendCharMessage(char *buffer, int ch)
{
    char tmp[MSGLEN], *delim;
    int i, count;
/* uses ch to find out what kind of message it is and then sends
   it there */
    switch ((char) ch) {
    case 'A':
	pmessage(buffer, 0, MALL);
	break;
    case 'F':
	pmessage(buffer, FEDm, MTEAM);
	break;
    case 'R':
	pmessage(buffer, ROMm, MTEAM);
	break;
    case 'K':
	pmessage(buffer, KLIm, MTEAM);
	break;
    case 'O':
	pmessage(buffer, ORIm, MTEAM);
	break;
    case 'G':
	pmessage(buffer, 0, MGOD);
	break;
        case '!':
          pmessage(buffer, 0, MTOOLS);
          break;
    case 'T':
	pmessage(buffer, idx_to_mask(me->p_teami), MTEAM);
	break;
    case 'M':
        /* mcast format; hit M and list of target addresses followed */
	/* by greater than(>) and message. the entire mesg including */
	/* addresses will be sent to each address */
	if((delim = strchr(buf, '>')) != NULL) {
	    count = delim - buf; /* number of addresses */
	    strncpy(tmp, buf, count);
	    if(strchr(tmp, ' ') == NULL)
	        for(i = 0; i < count; i++) { /* dont do tmp[0] is M */
		    sendCharMessage(buf, tmp[i]);
		}
	}
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
	pmessage(buffer, ch - '0', MINDIV);
	break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
	pmessage(buffer, ch - 'a' + 10, MINDIV);
	break;
    default:
	{
	    int     i;
	    for (i = 0; i < number_of_teams; i++) {
		if (ch == teaminfo[i].letter)
		    break;
	    }
	    if (i < number_of_teams) {
		pmessage(buffer, idx_to_mask(i), MTEAM);
		break;
	    }
	}
	warning("Not legal recipient");
    }
}

void
pmessage(char *str, int recip, int group)
{
    char    newbuf[100];

    strcpy(lastMessage, str);
    switch(group) {
    case MTOOLS:
	sendTools(str);
	break;
    default:
	sendMessage(str, group, recip);
    }
    if ((group == MTEAM && recip != idx_to_mask(me->p_teami)) ||
	(group == MINDIV && recip != me->p_no)) {
	sprintf(newbuf, "%s  %s",
		getaddr2(group, (group == MTEAM) ?
			 mask_to_idx(recip) : recip),
		str);
	newbuf[79] = 0;
	dmessage(newbuf, (unsigned)group, (unsigned)me->p_no, (unsigned)recip);
    }
    message_off();
}

/*static */
char *
getaddr(int who)
{
    switch (who) {
    case 'A':
	return (getaddr2(MALL, 0));
    case 'F':
	return (getaddr2(MTEAM, FEDi));
    case 'R':
	return (getaddr2(MTEAM, ROMi));
    case 'K':
	return (getaddr2(MTEAM, KLIi));
    case 'O':
	return (getaddr2(MTEAM, ORIi));
    case 'G':
	return (getaddr2(MGOD, 0));
    case '!':
      return (getaddr2(MTOOLS, 0));
    case 'M':
      return (getaddr2(MCAST, 0));
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
	if (isPlaying(&players[who - '0'])) {
	    return (getaddr2(MINDIV, who - '0'));
	} else {
	    warning("Player is not in game");
	    return (0);
	}
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
	if (who - 'a' + 10 > nplayers) {
	    warning("Invalid player number");
	    return (0);
	} else if (isPlaying(&players[who - 'a' + 10])) {
	    return (getaddr2(MINDIV, who - 'a' + 10));
	} else {
	    warning("Player is not in game");
	    return (0);
	}
    default:
	{
	    int     i;
	    for (i = 0; i < number_of_teams; i++) {
		if (addr == teaminfo[i].letter)
		    break;
	    }
	    if (i < number_of_teams) {
		return getaddr2(MTEAM, i);
	    }
	}
	warning("Not legal recipient");
	return (0);
    }
}

/*static*/
char *
getaddr2(int flags, int recip)
{
    static char addrmesg[ADDRLEN];

    (void) sprintf(addrmesg, " %c%c->", teaminfo[me->p_teami].letter, shipnos[me->p_no]);
    switch (flags) {
    case MALL:
	(void) sprintf(&addrmesg[5], "ALL");
	break;
    case MTEAM:
	(void) sprintf(&addrmesg[5], teaminfo[recip].shortname);
	break;
    case MINDIV:
	(void) sprintf(&addrmesg[5], "%c%c ",
		   teaminfo[players[recip].p_teami].letter, shipnos[recip]);
	break;
    case MGOD:
	(void) sprintf(&addrmesg[5], "GOD");
	break;
    case MTOOLS:
	(void) sprintf(addrmesg, "  Shell>");
	break;
    case MCAST:
        (void) sprintf(&addrmesg[5], "MCAS");
    }

    return (addrmesg);
}

/* Used in NEWMACRO, useful elsewhere also */
int
getgroup(int address, int *recip)
{
    *recip = 0;

    switch (address) {
    case 'A':
	*recip = 0;
	return (MALL);
    case 'T':			/* had to add this...why didn't COW-lite need
				   it?? -JR */
	*recip = idx_to_mask(me->p_teami);
	return (MTEAM);
    case 'F':
	*recip = FEDm;
	return (MTEAM);
    case 'R':
	*recip = ROMm;
	return (MTEAM);
    case 'K':
	*recip = KLIm;
	return (MTEAM);
    case 'O':
	*recip = ORIm;
	return (MTEAM);
    case 'G':
	*recip = 0;
	return (MGOD);
    case '!':
	*recip = 0;
	return (MTOOLS);
    case 'M':
	*recip = 0;
	return (MMOO);
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
	if (players[address - '0'].p_status == PFREE) {
	    warning("That player left the game. message not sent.");
	    return 0;
	}
	*recip = address - '0';
	return (MINDIV);
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
	if (players[address - 'a' + 10].p_status == PFREE) {
	    warning("That player left the game. message not sent.");
	    return 0;
	}
	*recip = address - 'a' + 10;
	return (MINDIV);
    default:
	warning("Not legal recipient");
    }
    return 0;
}





/*-------------------------------EMERGENCY--------------------------------*/
/*  This function sends a distress message out to the player's team.  */
void
emergency(void)
{
    char    ebuf[120];		/* to sprintf into */
    char    buf2[20];

    sprintf(ebuf, "Distress  %c%c",	/* get team and end */
	    teaminfo[me->p_teami].letter, shipnos[me->p_no]);
    switch (myship->s_type) {
    case STARBASE:
	strcat(ebuf, " (Starbase): ");
	break;
    case WARBASE:
	strcat(ebuf, " (Warbase): ");
	break;
    case JUMPSHIP:
	strcat(ebuf, " (Jumpship): ");
	break;
    default:
	strcat(ebuf, ": ");
    }
    if (me->p_damage != 0) {
	if (me->p_damage > me->p_ship->s_maxdamage - 30)
	    sprintf(buf2, "DEAD MAN  ");
	else
	    sprintf(buf2, "damg: %d%%  ", (100 * me->p_damage) / me->p_ship->s_maxdamage);
	strcat(ebuf, buf2);
    }
    if (me->p_shield < me->p_ship->s_maxshield) {
	if (me->p_shield < 5)
	    sprintf(buf2, "NO SHIELDS  ");
	else
	    sprintf(buf2, "shld: %d%%  ", (100 * me->p_shield) / me->p_ship->s_maxshield);
	strcat(ebuf, buf2);
    }
    if (me->p_wtemp > 0) {
	if (me->p_flags & PFWEP)
	    sprintf(buf2, "WTEMP  ");
	else
	    sprintf(buf2, "W %d%%  ", (100 * me->p_wtemp) / me->p_ship->s_maxwpntemp);
	strcat(ebuf, buf2);
    }
    if (me->p_etemp > 0) {
	if (me->p_flags & PFENG)
	    sprintf(buf2, "ETEMP  ");
	else
	    sprintf(buf2, "E %d%%  ", (100 * me->p_etemp) / me->p_ship->s_maxegntemp);
	strcat(ebuf, buf2);
    }
    if (me->p_fuel < me->p_ship->s_maxfuel) {
	if (me->p_fuel < 400)
	    sprintf(buf2, "NO FUEL  ");
	else
	    sprintf(buf2, "F %d%%  ", (100 * me->p_fuel) / me->p_ship->s_maxfuel);
	strcat(ebuf, buf2);
    }
    if ((int) strlen(ebuf) < 12)
	strcat(ebuf, "  perfect health  ");
    if (me->p_armies > 0) {
	sprintf(buf2, "%d ARMIES!", me->p_armies);
	strcat(ebuf, buf2);
    }
    pmessage(ebuf, idx_to_mask(me->p_teami), MTEAM);
}



void
carry_report(void)
{
    char    ebuf[80], *pntr;
    double  dist, closedist;
    struct planet *k, *p = NULL;

    closedist = blk_gwidth;

    for (k = &planets[0]; k < &planets[nplanets]; k++) {
	dist = hypot((double) (me->p_x - k->pl_x),
		     (double) (me->p_y - k->pl_y));
	if (dist < closedist) {
	    p = k;
	    closedist = dist;
	}
    }
    if(myship->s_type == STARBASE)
        sprintf(ebuf, "Your Starbase is carrying %d armies.  ", me->p_armies);
    else
        sprintf(ebuf, "I am carrying %d armies.  ", me->p_armies);
    for (pntr = ebuf; *pntr; pntr++);
    if (paradise) {
	sprintf(pntr, "Sector: %d-%d  ", (me->p_x / GRIDSIZE) + 1,
		(me->p_y / GRIDSIZE) + 1);
	for (; *pntr; pntr++);
    }
    sprintf(pntr, "%sear %s", (me->p_flags & PFCLOAK) ? "Cloaked n" : "N",
	    p->pl_name);
    pmessage(ebuf, idx_to_mask(me->p_teami), MTEAM);
}

void
message_on(void)
{
    if (warp) {
	W_WarpPointer(messagew,5,5);
    }
    else {
	messageon = 1;
	W_DefineTextCursor(w);
	W_DefineTextCursor(mapw);
    }
}

void
message_off(void)
{
    if (!warp) {
	messageon = 0;
	W_RevertCursor(w);
	W_RevertCursor(mapw);
    }
}
