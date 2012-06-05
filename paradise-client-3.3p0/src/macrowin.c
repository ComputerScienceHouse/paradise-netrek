
/*
 * macrowin.c from helpwin.c
 * copyright 1993 Nick Trown
 * copyright 1991 ERic mehlhaff
 * Free to use, hack, etc. Just keep these credits here.
 * Use of this code may be dangerous to your health and/or system.
 * Its use is at your own risk.
 * I assume no responsibility for damages, real, potential, or imagined,
 * resulting  from the use of it.
 * Yeah.....what Eric said...
 */
/* Modified for Paradise, 4/94  -JR */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"


/*
   Fills in macro window with the macros defined in the .xtrekrc.
 */

#define NUMLINES 80

#define MAXMACRO 65
/* maximum length in characters of key explanation */

#define MACROLEN 255
/* length of construction string since we don't know how long a macro can be */


int     lineno = 0;
char    maclines[10][MAXMACRO];
int     maclevel = 0;
int     macrocnt = 0;		/* a global from COW-lite, only needed here
				   for Paradise. */

int
formatline(char *line)
{
    register int end;
    char   *temp;
    int     num = 0;

    if (!line)
	return 0;
    if (strlen(line) <= MAXMACRO) {
	strncpy(maclines[num], line, sizeof(maclines[0]));
	lineno++;
	return 1;
    }
    temp = line;
    while (1) {
	end = MAXMACRO - 1;
	if (end > strlen(temp)) {
	    lineno++;
	    strncpy(maclines[num++], temp, sizeof(maclines[0]));
	    return (num);
	} else
	    for (; temp[end] != '%'; end--);

	lineno++;
	strncpy(maclines[num++], temp, end);

	temp = temp + end;
    }
}


void
filldist(int fill)
{
    register int i;
    register int row;
    register int c;
    int     num;
    char    key[3];

    lineno = 0;
    for (i = 1, row = 5; distmacro[i].macro != '\0'; i++) {
	if (fill) {
	    if (distmacro[i].c < 128)
		sprintf(key, " %c", distmacro[i].c);
	    else
		sprintf(key, "^%c", distmacro[i].c - 128);
	    sprintf(maclines[0], "%-8s %s",
		    key,
		    distmacro[i].name);
	    W_WriteText(macroWin, 2, row++, W_Yellow, maclines[0],
			strlen(maclines[0]), W_RegularFont);
	}
	lineno++;
	num = formatline(distmacro[i].macro);
	if (fill) {
	    for (c = 0; c < num; c++) {
		W_WriteText(macroWin, 8, row++, textColor, maclines[c],
			    strlen(maclines[c]), W_RegularFont);
	    }
	}
	if (lineno > NUMLINES)
	    continue;
    }
}



void
fillmacro(void)
{
    register int row, i;
    char    macromessage[MACROLEN];
    struct macro *m;

    W_ClearWindow(macroWin);
    sprintf(macromessage, "Packages active:  %s%s",
	    (F_UseNewMacro ? ", NEWMACRO" : ""),
	    (F_gen_distress ? ", RCD" : ""));
    /* (UseSmartMacro ? ", SMARTMACRO" : "")); */

    W_WriteText(macroWin, 2, 1, textColor,
		macromessage, strlen(macromessage), W_RegularFont);

    sprintf(macromessage, "Currently showing: %s",
	    (maclevel ? "Macros" : "RCDS"));

    W_WriteText(macroWin, 2, 2, textColor,
		macromessage, strlen(macromessage), W_RegularFont);


    if (maclevel == 0) {
	W_WriteText(macroWin, 2, 4, W_Yellow,
		    "Key     Distress Name", 21, W_RegularFont);
	filldist(1);
	return;
    }
    /* 4 column macro window. This may be changed depending on font size */
    for (row = 4, i = 0; i < 256; i++) {
	for (m = macrotable[i]; m; m = m->next, row++) {
	    if (m->flags & MACRCD)
		break;
	    if (i <= 128)
		sprintf(macromessage, "%c ", i);
	    else
		sprintf(macromessage, "^%c", i - 128);
#ifdef NEWMOUSE
	    if (macro[i].type == NEWMMOUSE) {
		switch (macro[i].who) {
		case MACRO_PLAYER:
		    strcat(macromessage, " PL MS ");
		    break;
		case MACRO_TEAM:
		    strcat(macromessage, " TM MS ");
		    break;
		default:
		    strcat(macromessage, " SELF  ");
		    break;
		}
	    } else {
#endif
		switch (m->to) {
		case 'T':
		    strcat(macromessage, " TEAM  ");
		    break;
		case 'A':
		    strcat(macromessage, " ALL   ");
		    break;
		case 'F':
		    strcat(macromessage, " FED   ");
		    break;
		case 'R':
		    strcat(macromessage, " ROM   ");
		    break;
		case 'K':
		    strcat(macromessage, " KLI   ");
		    break;
		case 'O':
		    strcat(macromessage, " ORI   ");
		    break;
		case 'M':
		    strcat(macromessage, " MOO   ");
		    break;
		case '!':
		    strcat(macromessage, " SHELL ");
		    break;
		case '\0':
		    strcat(macromessage, " SPEC  ");
		    break;
		case -1:
		    switch (m->specialto) {
		    case 'I':
		    case 'C':
			strcat(macromessage, " SELF  ");
			break;
		    case 'U':
		    case 'P':
			strcat(macromessage, " PL MS ");
			break;
		    case 'T':
		    case 'Z':
			strcat(macromessage, " TM MS ");
			break;
		    case 'G':
			strcat(macromessage, " NR FR ");
			break;
		    case 'H':
			strcat(macromessage, " NR EN ");
			break;
		    }
		    break;
		case -2:
		    strcat(macromessage, " QUERY ");
		    break;
		default:
		    strcat(macromessage, " ----  ");
		    break;
		}

#ifdef NEWMOUSE
	    }
#endif
	    strcat(macromessage, m->string);
	    macromessage[MAXMACRO] = '\0';
	    W_WriteText(macroWin, 2, row, textColor,
			macromessage, strlen(macromessage), W_RegularFont);
	}
    }
}

void
switchmacros(void)
{
    int     num, i;
    struct macro *m;

    if (!macroWin)
	return;			/* paranoia? */

    maclevel = abs(maclevel - 1);

    if (maclevel == 0) {
	lineno = 0;
	filldist(0);
	num = lineno + 5;
    } else {
	for (i = 0, macrocnt = 0; i < 256; i++) {
	    for (m = macrotable[i]; m; m = m->next) {
		if (m->flags & MACRCD)
		    break;
		macrocnt++;
	    }
	}
	num = macrocnt + 5;
    }
    W_ResizeText(macroWin, 80, num);
    W_MapWindow(macroWin);
}



void
showMacroWin(void)
{
    int     num, i;
    struct macro *m;

    if (!macroWin) {
	if (maclevel == 0) {
	    lineno = 0;
	    filldist(0);
	    num = lineno + 5;
	} else {
	    for (i = 0, macrocnt = 0; i < 256; i++) {
		for (m = macrotable[i]; m; m = m->next) {
		    if (m->flags & MACRCD)
			break;
		    macrocnt++;
		}
	    }
	    num = macrocnt + 5;
	}

	macroWin = W_MakeTextWindow("macrow", winside, 0,
				    80, num, NULL, (char*)0, BORDER);

	W_ResizeText(macroWin, 80, num);
	W_MapWindow(macroWin);
    } else if (W_IsMapped(macroWin)) {
	W_DestroyWindow(macroWin);
	macroWin = 0;
    } else
	W_MapWindow(macroWin);
}
