/* here's pretty much all the macro code.   */
/* This bears little resemblance to the     */
/* BRM code, i.e. it's somewhat organized :)*/
/* Bill Dyess  10/05/93	            [BDyess]*/

#include "copyright.h"

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#include "packets.h"
#include "gppackets.h"

#define MAXMACRO 4096

/* prototypes */
void doMacro2 P((struct macro * m, W_Event * data));
void handle_dollar P((char **locpntr, char **destpntr, W_Event * data));
void handle_special P((char **locpntr, char **destpntr, W_Event * data));
void handle_test P((char **locpntr, char **destpntr, W_Event * data));
void handle_conditional P((char **locpntr, char **destpntr, W_Event * data));
void getTestString P((char *buf, char **locpntr, char **destpntr, W_Event * data));
void getConditionalString P((char **locpntr, char **destpntr, W_Event * data));
void ignoreConditionalString P((char **locpntr));

int     abortflag = 0;

void
initMacros(void)
{
    struct stringlist *s;
    char   *loc;
    unsigned char ch;
    struct macro *m;
    int     i;
    struct dmacro_list *dm;
    struct dmacro_list *dm_def;
    int     notdone;
    unsigned char	c;
    char  *str;

    /* initialize macro lookup tables */
    memset(macrotable, 0, sizeof(struct macro *) * 256);

    /* sizeof doesn't work if it isn't in the same source file, shoot me */
    memcpy(dist_prefered, dist_defaults, sizedist);

    for (s = defaults; s; s = s->next) {
	if (strncasecmp(s->string, "dist.", 5) == 0) {
	    str = (s->string) + 5;
	    if (*str == '^') {
		str++;
		if (*str == '^')
		    c = '^';
		else
		    c = *str + 128;
	    } else
		c = *str;
	    str++;
	    if (*str != '.') {
		str = (s->string) + 4;
		c = '\0';
	    }
	    str++;

	    notdone = 1;
	    for (dm = &dist_prefered[take], dm_def = &dist_defaults[take], i = take;
		 dm->name && notdone; dm++, dm_def++, i++) {
		if (strcasecmp(str, dm->name) == 0) {
		    dm->macro = strdup(s->value);
		    if (c) {
			if (!macrotable[c]) {
			    macrotable[c] = (struct macro *) malloc(sizeof(struct macro));
			    memset(macrotable[c], 0, sizeof(struct macro));
			}
			macrotable[c]->flags |= MACRCD;
			macrotable[c]->to = i;
/*                        printf("dist.%c.%s: %s\n",c,dm->name,dm->macro);*/
			dm->c = c;
			dm_def->c = c;
		    }
		    notdone = 0;
		}
	    }
	}

	else if (strncasecmp(s->string, "lite.", 5) == 0) {
	    int     offset = 5;
	    char  **lt;

	    if (s->string[6] == '.')
		offset = 7;

	    notdone = 1;

	    for (lt = &distlite[take], dm = &dist_prefered[take];
		 dm->name && notdone; dm++, lt++) {
		if (strcasecmp(s->string + offset, dm->name) == 0) {
		    *lt = strdup(s->value);
/*                    printf("lite.%s: %s\n",dm->name,*lt);*/

		    notdone = 0;
		}
	    }
	    if (notdone)
		fprintf(stderr, "Unknown lite %s\n", s->string + offset);
	}
	if (!strncasecmp("mac", s->string, 3)) {
	    if (s->string[3] == '.')
		loc = s->string + 4;
	    else if (strncasecmp("ro.", s->string + 3, 3))
		continue;
	    else
		loc = s->string + 6;
	    if (*loc == '^') {	/* possible control char */
		loc++;
		if (*loc == '^' && *loc)
		    ch = '^';
		else
		    ch = *loc + 128;
	    } else
		ch = *loc;
	    loc++;
	    if (!macrotable[ch]) {
		/*
		   make sure it doesn't already exist.  I've allowed people
		   to have singlemacro: before the macro.*.* statements, so
		   it is possible
		*/
		/*
		   modified to allow multline macros by creating a linked
		   list of macro structures. -JR
		*/
		if (ch == '?') {
		    printf("Can't use '?' as a macro.  It is reserved for the macro window.  Ignoring.\n");
		    continue;
		}
		macrotable[ch] = m = (struct macro *) malloc(sizeof(struct macro));
		memset(m, 0, sizeof(struct macro));
	    } else {
		if (macrotable[ch]->flags & MACRCD) {
		    m = macrotable[ch];
		    m->flags &= ~(MACRCD);	/* in case singleMacro was
						   set */
		    m->next = 0;
		} else
		{
		    m = (struct macro *) malloc(sizeof(struct macro));
		    m->next = macrotable[ch];
		    macrotable[ch] = m;
		    m->next->flags |= MACMULTI;
		    m->flags = m->next->flags;
		}
	    }
	    if (*(loc++) != '.')
		m->to = -2;	/* no destination given */
	    else {
		ch = *loc;
		if (ch == '%') {
		    m->specialto = toupper(*(loc + 1));
		    m->to = -1;
		} else {
		    m->to = ch;
		}
	    }
	    m->string = strdup(s->value);
	} else if (!strncasecmp("singlemacro", s->string, 11)) {
	    loc = s->value;
	    while (*loc) {
		ch = *(loc++);
		if (ch == '^') {/* for control chars */
		    if (*loc != '^' && *loc)
			ch = *loc + 128;
		    loc++;
		}
		if (!macrotable[ch]) {
		    m = macrotable[ch] = (struct macro *) malloc(sizeof(struct macro));
		    memset(m, 0, sizeof(struct macro));
		    m->flags = MACSINGLE;
		} else {
		    for (m = macrotable[ch]; m; m = m->next)
			m->flags |= MACSINGLE;
		}
	    }
	}
    }
    for (i = 0; i < 256; i++) {
	/* eliminate any macros that have (null) macro strings */
	if (macrotable[i] && !(macrotable[i]->flags & MACRCD)) {
	    struct macro *tmp, **scan;

	    scan=&macrotable[i];
	    while (*scan) {
		if ( (*scan)->string ) {
		    scan = &(*scan)->next;
		} else {
		    tmp = (*scan);
		    *scan = tmp->next;
		    free(tmp);
		}
	    }
	    }
	}
    /*
       make macro entries for the default RCD keys, if those keys don't have
       macros defined
    */
    for (dm = &dist_prefered[take], i = take; dm->name; dm++, i++) {
	if (!macrotable[dm->c]) {
	    macrotable[dm->c] = (struct macro *) malloc(sizeof(struct macro));
	    memset(macrotable[dm->c], 0, sizeof(struct macro));
	    macrotable[dm->c]->flags = MACRCD;
	    macrotable[dm->c]->to = i;
	}
    }
}

/* takes a key as input and creates a string that is then sent to smessage*/
void
doMacro(W_Event *data)
{
    static struct macro *m;
    int     key = data->key;

    if (key == '?') {
	showMacroWin();
	macroState = 0;
	return;
    }
    if (macroState != 2)
	m = macrotable[key];
    if (!m) {
	W_Beep();
	warning("No such macro");
	macroState = 0;
	return;			/* no macro */
    }
    if (m->flags & MACRCD) {
	rcd(m->to, data);
	macroState = 0;
	return;
    }
    while (m) {
	if (macroState == 2) {
	    m->to = key;
	    doMacro2(m, data);
	    m->to = -2;
	} else {
	    doMacro2(m, data);
	    if (macroState == 2)
		return;
	}
	m = m->next;
    }
    macroState = 0;
    warning("              ");
}

void
doMacro2(struct macro *m, W_Event *data)
{
    int     group = -1, recip = 0;
    char    buf[MAXMACRO], sourcebuf[MAXMACRO];
    char   *loc, *dest;
    struct obtype *target;

    /* first figure out who I'm going to send it to */
    if ((INT8)(m->to) == -1) {		/* special recipient */
	switch (m->specialto) {
	case 'I':		/* send a message to myself */
	case 'C':
	    group = MINDIV;
	    recip = me->p_no;
	    break;
	case 'U':		/* send message to player nearest mouse */
	case 'P':
	    group = MINDIV;
	    target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
	    recip = target->o_num;
	    break;
	case 'T':		/* send message to team of the player nearest
				   mouse */
	case 'Z':
	    group = MTEAM;
	    target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
	    recip = idx_to_mask(players[target->o_num].p_teami);
	    break;
	case 'G':		/* send message to nearest friendly player to
				   my ship */
	    group = MINDIV;
	    target = gettarget((W_Window) 0, me->p_x, me->p_y,
			       TARG_PLAYER | TARG_FRIENDLY);
	    recip = target->o_num;
	    break;
	case 'H':		/* send message to nearest enemy player to my
				   ship */
	    group = MINDIV;
	    target = gettarget((W_Window) 0, me->p_x, me->p_y,
			       TARG_PLAYER | TARG_ENEMY);
	    recip = target->o_num;
	    break;
	default:
	    warning("Bad macro - incorrect 'to' field");
	    break;
	}
    } else if ((INT8)m->to == -2) {	/* get recipient not provided, so change
				   state to get one */
	macroState = 2;
	warning("Send macro to who?");
	return;
    } else
	recip = m->to;
    /* now parse the macro itself. */
    strcpy(sourcebuf, m->string);
    loc = sourcebuf;
    dest = buf;
    while (*loc) {
	if (*loc == '$') {
	    loc++;
	    handle_dollar(&loc, &dest, data);	/* handle the special escape */
	} else if (*loc == '%') {
	    loc++;
	    if (*loc == '*')
		return;		/* %* means exit macro NOW */
	    handle_special(&loc, &dest, data);	/* handle the special escape */
	} else {
	    *(dest++) = *(loc++);
	}
    }
    *dest = 0;
    if (buf[0] == 0 || abortflag) {	/* abortflag means somewhere there
					   was a %* */
	abortflag = 0;
	macroState = 0;
	return;			/* null message.  If you *really* want to
				   print a null message, use <space> */
    }
    if (group == -1)
	group = getgroup(recip, &recip);
    if (group <= 0)
	return;
    if ((m->flags & MACMULTI) && (F_multiline_enabled || paradise))
	group |= MMACRO;
    pmessage(buf, recip, group);
}

void
handle_special(char **locpntr, char **destpntr, W_Event *data)
{
    char    ch = **locpntr;
    char   *buf = *destpntr;
    struct obtype *target;
    struct macro *m;
    int     targettype = 0;
    struct id *id;
    /* for pingstats */
    extern int ping_tloss_sc;	/* total % loss 0--100, server to client */
    extern int ping_tloss_cs;	/* total % loss 0--100, client to server */
    extern int ping_av;		/* average rt */
    extern int ping_sd;		/* standard deviation */

    switch (ch) {
    case 'a':			/* armies carried by sender */
	sprintf(buf, "%d", me->p_armies);
	break;
    case 'd':			/* sender damage percentage */
	sprintf(buf, "%d", 100 * me->p_damage / me->p_ship->s_maxdamage);
	break;
    case 's':			/* sender shield percentage */
	sprintf(buf, "%d", 100 * me->p_shield / me->p_ship->s_maxshield);
	break;
    case 'f':			/* sender fuel percentage */
	sprintf(buf, "%d", 100 * me->p_fuel / me->p_ship->s_maxfuel);
	break;
    case 'w':			/* sender wtemp percentage */
	sprintf(buf, "%d", 100 * me->p_wtemp / me->p_ship->s_maxwpntemp);
	break;
    case 'e':			/* sender etemp percentage */
	sprintf(buf, "%d", 100 * me->p_etemp / me->p_ship->s_maxegntemp);
	break;
    case 'r':			/* team id character of target player */
	target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
	buf[0] = teaminfo[players[target->o_num].p_teami].letter;
	buf[1] = 0;
	break;
    case 't':			/* team id character of target planet */
	target = gettarget(data->Window, data->x, data->y, TARG_PLANET);
	buf[0] = teaminfo[mask_to_idx(planets[target->o_num].pl_owner)].letter;
	buf[1] = 0;
	break;
    case 'p':			/* id character of target player */
	targettype = TARG_PLAYER;
    case 'g':			/* id character of target friendly player */
	if (!targettype)
	    targettype = TARG_PLAYER | TARG_FRIENDLY;
    case 'h':			/* id char of target enemy player */
	if (!targettype)
	    targettype = TARG_PLAYER | TARG_ENEMY;
	id = getTargetID(data->Window, data->x, data->y, targettype);
	buf[0] = id->mapstring[1];
	buf[1] = 0;
	break;
    case 'P':			/* id character of player nearest sender */
	id = getTargetID((W_Window) 0, me->p_x, me->p_y, TARG_PLAYER);
	buf[0] = id->mapstring[1];
	buf[1] = 0;
	break;
    case 'T':			/* team id character of sender team */
	buf[0] = me->p_mapchars[0];
	buf[1] = 0;
	break;
    case 'c':			/* sender id character */
	buf[0] = me->p_mapchars[1];
	buf[1] = 0;
	break;
    case 'C':			/* 1 if cloaked, 0 if not [BDyess] */
	if (me->p_flags & PFCLOAK)
	    buf[0] = '1';
	else
	    buf[0] = '0';
	buf[1] = 0;
	break;
    case 'n':			/* armies on target planet */
	target = gettarget(data->Window, data->x, data->y, TARG_PLANET);
	sprintf(buf, "%d", planets[target->o_num].pl_info ?
		planets[target->o_num].pl_armies :
		-1);
	break;
    case 'E':			/* 1 if etemped, 0 if not */
	if (me->p_flags & PFENG)
	    buf[0] = '1';
	else
	    buf[0] = '0';
	buf[1] = 0;
	break;
    case 'W':			/* 1 if wtemped, 0 if not */
	if (me->p_flags & PFWEP)
	    buf[0] = '1';
	else
	    buf[0] = '0';
	buf[1] = 0;
	break;
    case 'S':			/* sender two character ship type */
	strncpy(buf, me->p_ship->s_desig, 2);
	buf[2] = 0;
	break;
    case 'G':			/* id char of friendly player nearest sender */
	targettype = TARG_FRIENDLY;
    case 'H':			/* id char of enemy player nearest sender */
	if (!targettype)
	    targettype = TARG_ENEMY;
	id = getTargetID((W_Window) 0, me->p_x, me->p_y,
			 TARG_PLAYER | targettype);
	buf[0] = id->mapstring[1];
	buf[1] = 0;
	break;
    case 'l':			/* three character name of target planet */
    case 'L':
	id = getTargetID(data->Window, data->x, data->y, TARG_PLANET);
	strcpy(buf, id->mapstring);
	buf[0] = tolower(buf[0]);
	break;
    case 'i':			/* sender full player name (16 character max) */
    case 'I':
	strncpy(buf, me->p_name, 16);
	buf[16] = 0;
	break;
    case 'u':			/* full name of target player (16 character
				   max) */
    case 'U':
	id = getTargetID(data->Window, data->x, data->y, TARG_PLAYER);
	strncpy(buf, id->name, 16);
	buf[16] = 0;
	break;
    case 'z':			/* 3 letter team id of target planet */
    case 'Z':
	id = getTargetID(data->Window, data->x, data->y, TARG_PLANET);
	strcpy(buf, teaminfo[id->team].shortname);
	strlower(buf);
	break;
    case 'b':			/* nearest planet to sender */
    case 'B':
	id = getTargetID((W_Window) 0, me->p_x, me->p_y, TARG_PLANET);
	strcpy(buf, id->mapstring);
	buf[0] = tolower(buf[0]);
	break;
    case 'v':			/* average ping round trip time */
	sprintf(buf, "%d", ping_av);
	break;
    case 'V':			/* ping stdev */
	sprintf(buf, "%d", ping_sd);
	break;
    case 'y':			/* packet loss */
	sprintf(buf, "%d", (2 * ping_tloss_sc + ping_tloss_cs) / 3);
	break;
    case 'm':			/* last message */
    case 'M':
	strcpy(buf, lastMessage);
	break;
    case 'o':			/* insert three letter team name */
    case 'O':
	strcpy(buf, teaminfo[me->p_teami].shortname);
	break;
    case ' ':			/* nothing.  This is so you can start a macro
				   with spaces */
	buf[0] = ' ';
	buf[1] = 0;
	break;
    case '%':			/* insert % */
	buf[0] = '%';
	buf[1] = 0;
	break;
    case '?':			/* start test */
	(*locpntr)++;
	handle_test(locpntr, destpntr, data);
	return;
    case '{':			/* conditional */
	handle_conditional(locpntr, destpntr, data);
	return;
    case '*':			/* abort! */
	abortflag = 1;
	return;
    case '2':			/* is paradise?  sorry, ran out of good
				   letters. '2' means, 'is Netrek II?'. */
	buf[0] = paradise + '0';
	buf[1] = 0;
	break;
    case '_':			/* call another macro. Added 1/24/94 [BDyess] */
	(*locpntr)++;
	if (**locpntr == '^') {	/* control char */
	    (*locpntr)++;
	    m = macrotable[**locpntr + (**locpntr == '^') ? 0 : 128];
	} else {
	    m = macrotable[(int) **locpntr];
	}
	if (m) {		/* does the macro exist? */
	    char    temp[MAXMACRO];
	    strcpy(temp, m->string);
	    strcat(temp, *locpntr + 1);
	    strcpy(*locpntr + 1, temp);
	} else {		/* somebody screwed up */
	    printf("Error: called macro ");
	    if (&m - macrotable >= 128)
		putchar('^');
	    printf("%c doesn't exist.\n", **locpntr);
	}
	buf[0] = 0;
	break;
    default:
	sprintf(buf, "Unknown %% escape: %%%c", ch);
	warning(buf);
	buf[0] = 0;
	break;
    }
    if (isupper(ch))
	strupper(buf);
    (*locpntr)++;
    while (**destpntr)
	(*destpntr)++;
    return;
}

void
handle_test(char **locpntr, char **destpntr, W_Event *data)
{
    char    l[MAXMACRO], r[MAXMACRO], condition = 0;
    short   trueflag = 0;

    getTestString(l, locpntr, destpntr, data);
    if (**locpntr != '%') {
	condition = *((*locpntr)++);
	getTestString(r, locpntr, destpntr, data);
    }
    switch (condition) {
    case '=':
	if (!strcmp(l, r))
	    trueflag = 1;
	break;
    case '>':
	if (atoi(l) > atoi(r))
	    trueflag = 1;
	break;
    case '<':
	if (atoi(l) < atoi(r))
	    trueflag = 1;
	break;
    default:
	if (atoi(l))
	    trueflag = 1;
    }
    **destpntr = '0' + trueflag;
    *(*destpntr + 1) = 0;
    (*destpntr)++;
    return;
}

void
handle_conditional(char **locpntr, char **destpntr, W_Event *data)
{
    (*locpntr)++;
    **destpntr = 0;
    (*destpntr)--;
    if (**destpntr == '0') {
	ignoreConditionalString(locpntr);
	getConditionalString(locpntr, destpntr, data);
    } else {
	getConditionalString(locpntr, destpntr, data);
	ignoreConditionalString(locpntr);
    }
    (*locpntr) += 2;
    while (**destpntr)
	(*destpntr)++;
    return;
}

void
ignoreConditionalString(char **locpntr)
{
    int     depth = 0, breakflag = 0;

    while (**locpntr) {
	if (**locpntr == '%') {
	    switch (*(*locpntr + 1)) {
	    case '!':
		if (!depth)
		    breakflag = 1;
		(*locpntr) += 2;
		break;
	    case '}':
		if (depth) {
		    depth--;
		    (*locpntr) += 2;
		} else
		    breakflag = 1;
		break;
	    case '{':
		depth++;
		(*locpntr) += 2;
		break;
	    case '*':
		abortflag = 1;
		return;
	    case 0: 		/* terminator! [BDyess] */
	        return;
	    default:
		(*locpntr)++;
	    }
	    if (breakflag)
		break;
	} else
	    (*locpntr)++;
    }
}

void
getConditionalString(char **locpntr, char **destpntr, W_Event *data)
{
    char   *dest = *destpntr;

    while (**locpntr) {
	if (**locpntr != '%' && **locpntr != '$' && **locpntr)
	    *(dest++) = *((*locpntr)++);
	else if (*(*locpntr + 1) == '!') {
	    (*locpntr) += 2;
	    break;
	} else if (*(*locpntr + 1) == '}')
	    break;
	else if (**locpntr == '%') {
	    (*locpntr)++;
	    handle_special(locpntr, &dest, data);
	    while (*(dest++));
	    dest--;
	} else {		/* **locpntr must equal '$' */
	    if(**locpntr == 0) return;
	    (*locpntr)++;
	    handle_dollar(locpntr, &dest, data);
	    while (*(dest++));
	    dest--;
	}
    }
    *dest = 0;
    return;
}

void
getTestString(char *buf, char **locpntr, char **destpntr, W_Event *data)
{
    char   *dest = buf;

    if (**locpntr == '%') {
	(*locpntr)++;
	handle_special(locpntr, &buf, data);
    } else if (**locpntr == '$') {
	(*locpntr)++;
	handle_dollar(locpntr, &buf, data);
    } else {
	while (**locpntr != '%' && **locpntr != '$' && **locpntr != '<' &&
	       **locpntr != '>' && **locpntr != '=' &&
	       **locpntr)
	    *(dest++) = *((*locpntr)++);
	*dest = 0;
    }
    return;
}

/**********************************************************************/

/*
   start with a $

   field 1:
   (n)earest
   (t)arget
   (s)elf	(doesn't have fields 2 and 3)
   (_) ego	(has no other fields)

   field 2:
   (a)ny
   (t)eammate
   (f)riendly
   (h)ostile

   field 3:
   (a)ny
   (u)ser
   (p)lanet (includes asteroids)
   (s)tar
   (n)ebula
   (b)lack hole
   (^) non-planet
   (*) any stellar object

   field 4: (optional)		NYI
   (U)ppercase
   (C)apitalize
   (L)owercase

   field 5:
   full (n)ame (Hammor, Thought)
   (i)dentifier (e.g. R5, Ka, Can, Sco)
   (#) number (0-9a-z for players, %d for planets)
   (t)eam name (Romulan)
   (s)hort team id (ROM)
   (l)etter of team (R)
   (a)rmies
   (@) sector
   (A)rable, 0=not arable, 1=arable but not AGRI, 2=AGRI
   (M)etal, 0, 1, 2(repair), or 3(sy)
   (D)ilithium, 0, 1 or 2(fuel)

   Any implementation of the paradise $ codes (subset or superset)
   must implement and document the $_ code.      -- Robert Forsman
*/

void
handle_dollar(char **locpntr, char **destpntr, W_Event *data)
{
    char   *buf = *destpntr;
    struct id *target;
    char    ch = *((*locpntr)++);
    W_Window win;
    int     x, y;
    int     flags;
    int     capitalize = 0;

    buf[0] = 0;

    if (ch == '_') {
	strcpy(buf, "Paradise netrek $ codes are orthogonal and make sense.");
	while (**destpntr)
	    (*destpntr)++;
	return;
    } if (ch == 's') {
	target = getTargetID((W_Window) 0, me->p_x, me->p_y, TARG_PLAYER | TARG_SELF);
    } else {
	switch (tolower(ch)) {
	case 'n':
	    win = 0;
	    x = me->p_x;
	    y = me->p_y;
	    break;
	case 't':
	    win = data->Window;
	    x = data->x;
	    y = data->y;
	    break;
	default:
	    printf("Invalid $ code field 1 : `%c'\n", ch);
	    return;
	}

	ch = *((*locpntr)++);
	switch (tolower(ch)) {
	case 'a':
	    flags = 0;
	    break;
	case 't':
	    flags = TARG_TEAM;
	    break;
	case 'f':
	    flags = TARG_FRIENDLY;
	    break;
	case 'h':
	    flags = TARG_ENEMY;
	    break;
	default:
	    printf("Invalid $ code field 2 : `%c'\n", ch);
	    return;
	}

	ch = *((*locpntr)++);
	switch (tolower(ch)) {
	case 'a':
	    flags |= TARG_PLAYER | TARG_ASTRAL;
	    break;
	case 'u':
	    flags |= TARG_PLAYER;
	    break;
	case 'p':
	    flags |= TARG_PLANET;
	    break;
	case 's':
	    flags |= TARG_STAR;
	    break;
	case 'n':
	    flags |= TARG_NEBULA;
	    break;
	case 'b':
	    flags |= TARG_BLACKHOLE;
	    break;
	case '^':
	    flags |= (TARG_ASTRAL & ~TARG_PLANET);
	    /* fall through */
	case '*':
	    flags |= TARG_PLANET;
	    break;
	default:
	    printf("Invalid $ code field 3 : `%c'\n", ch);
	    return;
	}

	target = getTargetID(win, x, y, flags);
    }

    ch = tolower(*((*locpntr)++));
    if (ch == 'l')
	capitalize = -1;
    else if (ch == 'c')
	capitalize = 1;
    else if (ch == 'u')
	capitalize = 2;
    else
	(*locpntr)--;		/* oops, back up and try again */

    ch = *((*locpntr)++);

    switch (ch) {
    case 'n':
	strcpy(buf, target->name);
	break;
    case 'i':
/*    if (target->type == PLANETTYPE) {*/
	strcpy(buf, target->mapstring);
/*
    } else {
      buf[0] = target->mapstring[1];
      buf[1] = 0;
    }*/
	break;
    case '#':
	if (target->type == PLANETTYPE) {
	    sprintf(buf, "%d", target->number);
	} else {
	    buf[0] = target->mapstring[1];
	    buf[1] = 0;
	}
	break;
    case 't':
	strcpy(buf, teaminfo[target->team].name);
	break;
    case 's':
	strcpy(buf, teaminfo[target->team].shortname);
	break;
    case 'l':
	buf[0] = teaminfo[target->team].letter;
	buf[1] = 0;
	break;
    case 'a':
	sprintf(buf, "%d", (target->type == PLANETTYPE) ?
		planets[target->number].pl_armies :
		players[target->number].p_armies);
	break;
    case '@':
	if (!paradise)
	    break;
	if (target->type == PLANETTYPE) {
	    x = planets[target->number].pl_x;
	    y = planets[target->number].pl_y;
	} else {
	    x = players[target->number].p_x;
	    y = players[target->number].p_y;
	}
	rotate_coord(&x, &y, -rotate_deg, blk_gwidth/2, blk_gwidth/2);
	sprintf(buf, "%d-%d", x / GRIDSIZE + 1, y / GRIDSIZE + 1);
	break;
    case 'A':			/* Arable or AGRI */
	buf[0] = '0';
	if (target->type == PLANETTYPE) {
	    if (planets[target->number].pl_flags & PLARABLE)
		buf[0] = '1';
	    if (planets[target->number].pl_flags & PLAGRI)
		buf[0] = '2';
	}
	buf[1] = 0;
	break;
    case 'M':			/* Metal, Repair, or SY */
	buf[0] = '0';
	if (target->type == PLANETTYPE) {
	    if (planets[target->number].pl_flags & PLMETAL)
		buf[0] = '1';
	    if (planets[target->number].pl_flags & PLREPAIR)
		buf[0] = '2';
	    if (planets[target->number].pl_flags & PLSHIPYARD)
		buf[0] = '3';
	}
	buf[1] = 0;
	break;
    case 'D':			/* Dilythium or Fuel */
	buf[0] = '0';
	if (target->type == PLANETTYPE) {
	    if (planets[target->number].pl_flags & PLDILYTH)
		buf[0] = '1';
	    if (planets[target->number].pl_flags & PLFUEL)
		buf[0] = '2';
	}
	buf[1] = 0;
	break;
    default:
	printf("Invalid $ code field 4 : `%c'\n", ch);
	return;
    }

    if (capitalize < 0) {
	char   *s;
	for (s = buf; *s; s++)
	    *s = tolower(*s);
    } else if (capitalize > 1) {
	char   *s;
	for (s = buf; *s; s++)
	    *s = toupper(*s);
    } else if (capitalize) {
	char   *s;
	s = buf;
	*s = toupper(*s);
	for (s++; *s; s++)
	    *s = tolower(*s);
    }
    while (**destpntr)
	(*destpntr)++;
    return;
}
