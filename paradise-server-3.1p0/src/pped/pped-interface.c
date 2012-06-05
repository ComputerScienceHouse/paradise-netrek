/*
 * interface.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <curses.h>
#include <term.h>
#include "config.h"
#include "common.h"
#include "db.h"
#include "interface.h"
#include "intfdesc.h"
#include "file.h"
#include "ppeddata.h"
#include "main.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"


/*
static char *shrt_royal_names[] = {
	"",
	"Wes",
	"Cen",
	"Pra",
	"Emp"
};
*/

/*
static char *royal_names[] = {
	"none",
	"Wesley",
	"Centurion",
	"Praetor",
	"Emperor"
};
*/

/*
static char *shrt_rank_names[] = {
	"Recr", "Spec", "Cadt", "Mids", "EnJr", "Ensi", "LtJr",
	"Lieu", "LtCm", "Cmdr", "HiCm", "Capt", "FlCp", "Comd",
	"Adml", "RAdm", "Moff", "GrMo"
};
*/

/*
static char *rank_names[] = {
	"Recruit",     "Specialist", "Cadet",        "Midshipman",
	"Ensn. Jr.",   "Ensign",     "Lt. Jnr. Gr.", "Lieutenant",
	"Lt. Cmdr.",   "Commander",  "High Cmdr.",   "Captain",
	"Fleet Capt.", "Commodore",  "Admiral",      "Rear Adml.",
	"Moff",        "Grand Moff"
};
*/

static char *
short_royal(int n)
{
  static char buf[4];

  strncpy(buf, royal[n].name, 3);
  buf[3] = 0;
  return(buf);
}

static char *
short_rank(int n)
{
  static char buf[5];
  char *p;

  buf[4] = 0;
  p = strchr(ranks[n].name, ' ');
  if(!p)
    strncpy(buf, ranks[n].name, MAX(strlen(ranks[n].name), 4));
  else
  {
    if(!strncasecmp(p+1, "j.g.", 4))
    {
      strncpy(buf, ranks[n].name, 2);
      buf[2] = 'J';
      buf[3] = 'G';
    }
    else if(!strncasecmp(ranks[n].name, "Lt", 2))
    {
      buf[0] = 'L';
      buf[1] = 't';
      strncpy(buf+2, p+1, MAX(strlen(p+1), 2));
    }
    else
    {
      strncpy(buf, ranks[n].name, 1);
      strncpy(buf+1, p+1, MAX(strlen(p+1), 3));
    }
  }
  return(buf);
}

RETSIGTYPE
getTTYinfo(int unused)
{
	struct winsize ws;
	char *name, bp[1024], area[1024], *ap = area;

	signal(SIGWINCH, getTTYinfo);

	/* determine # of lines */
	ioctl(0, TIOCGWINSZ, &ws);
	numLines = ws.ws_row;

	/* find terminal sequence to clear the screen */
	if((name = getenv("TERM")))
		if(tgetent(bp, name) > 0)
			clrStr = tgetstr("cl", &ap);
}

void
cls(void)
{
	printf("%s", clrStr); fflush(stdout);
}

void
Interface(void)
{
	struct plnode *p;
	int nlines;
	int top;
	int i;
	int num;
	char buf[18], *c;

	top = 0;
	for(;;) {
		nlines = numLines - 4;
		if(nlines < 5) err_fatal("not enough tty lines!");

		cls();
		printf("\
 C  #  Rnk  Ryl  Name                  C  #  Rnk  Ryl  Name\n");
		printf("\
 - --- ---- ---- -----------------     - --- ---- ---- -----------------\n");

		for(i = 0; i < nlines; i++) {
			if(top + i > numDBentries - 1) {
				printf("\n");
				continue;
			}
			p = GetNode(top + i);
			if(!p) {
				printf("\n");
				continue;
			}
			strncpy(buf, p->player.name, 16);
			strcat(buf, "_");
			printf(" %c%4d %-4s %-4s %-17.17s    ",
				p->status ? '*':' ', top + i,
				short_rank(p->player.stats.st_rank),
				short_royal(p->player.stats.st_royal),
				Strip(buf));

			if(top + i + nlines > numDBentries - 1) {
				printf("\n");
				continue;
			}
			p = GetNode(top + i + nlines);
			if(!p) {
				printf("\n");
				continue;
			}
			strncpy(buf, p->player.name, 16);
			strcat(buf, "_");
			printf(" %c%4d %-4s %-4s %s\n",
				p->status ? '*':' ', top + i + nlines,
				short_rank(p->player.stats.st_rank),
				short_royal(p->player.stats.st_royal),
				Strip(buf));
		}
		printf("\nIndex: Command (? for help) -->"); fflush(stdout);
		if(fgets(buf, 18, stdin) == NULL)
			continue;

		switch(buf[0]) {
			case '\n':
			case 'n':
				top += (nlines * 2);
				if(top >= numDBentries)
					top = 0;
				break;
			case 'p':
				if(!top) {
					if(numDBentries - 2 * nlines > 0)
						top = numDBentries - 2 * nlines;
					else
						top = 0;
				} else {
					top -= (nlines * 2);
					if(top < 0) top = 0;
				}
				break;
			case 'Q':
				GoAway(CheckChanged());
				break;
			case 's':
				DoSave(1);
				break;
			case 'e':
				printf("Enter player name to edit -->");
				fflush(stdout);
				if(fgets(buf, 18, stdin) == NULL)
					break;
				if((c = strrchr(buf, '\n')))
					*c = (char)0;
				num = GetByName(buf);
				if(num < 0) {
					Report("Couldn't find any players by that name.");
					break;
				}
				Edit(num);
				break;
			case '?':
				Report("\n\
[num] : Edit player [num]\n\
  e   : Edit player (by name)\n\
  n   : Next page\n\
[ret] : Next page\n\
  p   : Previous page\n\
  s   : Save to file\n\
  Q   : Quit pped\n");
				break;
			default:
				i = sscanf(buf, "%d", &num);
				if(!i || (num < 0) || (num >= numDBentries))
					break;
				Edit(num);
		}
	}
}

void
Edit(int pnum)
{
	struct plnode *p, player;
	char buf[18], txt[80];
	int i, num, nlines, top;

	p = GetNode(pnum);
	if(!p) return;
	player = *p;
	player.status = 0;
	
	top = 0;

	for(;;) {
		nlines = numLines - 2;
		cls();
		Display(&player, top, top+nlines);

		printf("\nEdit: Command (? for help) -->"); fflush(stdout);
		if(fgets(buf, 18, stdin) == NULL)
			continue;

		switch(buf[0]) {
			case '?':  /* help */
				Report("\n\
  n   : Next page\n\
  p   : Previous page\n\
[num] : Change value [num]\n\
  s   : Save and return to index\n\
  w   : Save\n\
[ret] : Return to index, don't save\n\
  x   : Same as [ret]\n\
  D   : Delete this entry and return to index\n\
  r   : Revert (undo changes since last save)\n\
  Q   : Quit pped\n");
				break;

			case 'n':  /* page forward */
				top += nlines;
				if(top > NUMDESC + 1) top = 0;
				continue;

			case 'p':  /* page backward */
				top -= nlines;
				if(top < 0) top = 0;
				continue;

			case 'D':  /* delete (and exit) */
				sprintf(txt, "delete entry for player %s", p->player.name);
				if(Verify(txt)) {
					DeleteNode(pnum);
					Report("Player deleted.");
					return;
				} else {
					Report("Canceled.");
				}
				break;

			case 'Q':  /* quit */
				GoAway(player.status || CheckChanged());
				break;

			case 's':  /* save and exit */
				if(player.status) {
					*p = player;
					player.status = 0;
					Report("Player saved.");
				}
				return;
				break;

			case 'w':  /* save */
				if(player.status) {
					*p = player;
					player.status = 0;
					Report("Player saved.");
				} else {
					Report("No changes to save.");
				}
				break;

			case 'r':  /* revert */
				if(player.status) {
					sprintf(txt, "undo changes for player %s?", p->player.name);
					if(Verify(txt)) {
						player = *p;
						player.status = 0;
						Report("Changes discarded.");
					} else {
						Report("Canceled.");
					}
				} else {
					Report("No changes to undo.");
				}
				break;

			case 0x1B:  /* (escape) exit, no changes */
			case '\n':
			case 'x':
				if(player.status)
					if(!Verify("exit?  There are unsaved changes."))
						break;
				return;

			default:
				i = sscanf(buf, "%d", &num);
				if(!i || (num < 0) || (num >= NUMDESC))
					break;
				Change(num, &player);
		}
	}
}

int
Verify(char *str)
{
	char buffer[18];

	for(;;) {
		printf("Verify %s (y/n) -->", str); fflush(stdout);
		if(fgets(buffer, 18, stdin) == NULL)
			continue;
		switch(buffer[0]) {
			case 'Y':
			case 'y':
				return 1;
			case 'N':
			case 'n':
				return 0;
		}
	}
}

void
Report(char *str)
{
	char buffer[18];

	printf("%s\n", str);
	printf("Press return to continue -->"); fflush(stdout);

	fgets(buffer, 18, stdin);
}

void
Change(int num, struct plnode *p)
{
	struct inter_desc *it;
	void *ptr;
	char str16[16], *c;
	char buffer[80];
	int intnum, i, col;
	float floatnum;
	char pwd[16];

	if(num < 0 || num >= NUMDESC) return;
	it = &idesc_tab[num];

	ptr = (void *)((int)(&p->player) + it->offset);

	switch(it->type) {
		case DT_CHAR16:
			printf("Current value for %s: \"%s\"\n", it->name, (char *)ptr);
			printf("Enter new value -->"); fflush(stdout);
			str16[0] = 0;
			fgets(str16, 16, stdin);
			if((c = strrchr(str16, '\n')))
				*c = (char)0;

			sprintf(buffer, "\"%s\" as new value for %s.", str16, it->name);
			if(Verify(buffer)) {
				if(strncmp((char *)ptr, str16, 16)) {
					strncpy((char *)ptr, str16, 16);
					p->status = 1; /* something changed */
				}
			}
			break;

		case DT_PWD:
			printf("Current value for %s: \"%s\"\n", it->name, (char *)ptr);
			printf("Enter new value -->"); fflush(stdout);
			pwd[0] = 0;
			fgets(pwd, 16, stdin);
			if((c = strrchr(pwd, '\n')))
				*c = (char)0;

			sprintf(buffer, "\"%s\" as new value for %s.", pwd, it->name);
			if(Verify(buffer)) {
				if(strncmp((char *)ptr, pwd, 16)) {
					strncpy((char *)ptr, pwd, 16);
					p->status = 1; /* something changed */
				}
			}
			break;

		case DT_INT:
		case DT_TICKS:
			printf("Current value for %s: %d\n", it->name, *((int *)ptr));
			printf("Enter new value -->"); fflush(stdout);
			fgets(buffer, 30, stdin);
			intnum = atoi(buffer);
			sprintf(buffer, "%d as new value for %s.", intnum, it->name);
			if(Verify(buffer)) {
				if(intnum != *((int *)ptr)) {
					*((int *)ptr) = intnum;
					p->status = 1;
				}
			}
			break;

		case DT_FLOAT:
			printf("Current value for %s: %f\n", it->name, *((float *)ptr));
			printf("Enter new value -->"); fflush(stdout);
			fgets(buffer, 30, stdin);
			floatnum = (float)atof(buffer);
			sprintf(buffer, "%f as new value for %s.", floatnum, it->name);
			if(Verify(buffer)) {
				if(floatnum != *((float *)ptr)) {
					*((float *)ptr) = floatnum;
					p->status = 1;
				}
			}
			break;

		case DT_RANK:
			for(i = 0, col = 0; i < NUMRANKS; i++) {
				printf(" %2d: %-7s ", i, short_rank(i));
				if(++col == 4) {
					printf("\n");
					col = 0;
				}
			}
			printf("\nCurrent value for %s: %d (%s)\n", it->name,
				  *((int *)ptr), ranks[*((int *)ptr)].name);
			printf("Enter new value -->"); fflush(stdout);
			fgets(buffer, 30, stdin);
			intnum = atoi(buffer);
			sprintf(buffer, "%d (%s) as new value for %s.", intnum,
				  ranks[intnum].name, it->name);
			if(Verify(buffer)) {
				if(intnum != *((int *)ptr)) {
					*((int *)ptr) = intnum;
					p->status = 1;
				}
			}
			break;

		case DT_ROYAL:
			for(i = 0; i < NUMROYALRANKS; i++)
				printf(" %2d: %s\n", i, royal[i].name);

			printf("Current value for %s: %d (%s)\n", it->name,
				  *((int *)ptr), royal[*((int *)ptr)].name);
			printf("Enter new value -->"); fflush(stdout);
			fgets(buffer, 30, stdin);
			intnum = atoi(buffer);
			sprintf(buffer, "%d (%s) as new value for %s.", intnum,
				  royal[intnum].name, it->name);
			if(Verify(buffer)) {
				if(intnum != *((int *)ptr)) {
					*((int *)ptr) = intnum;
					p->status = 1;
				}
			}
			break;
		default:
			Report("Error in case statement in Change()");
			break;
	}
}

void
Display(struct plnode *n, int from, int to)
{
	int i, hour, dt = 0;
	char c;
	void *ptr;
	struct inter_desc *it;
	struct tm *stm;
	time_t tt;

	if(to > NUMDESC) {
		to = NUMDESC;
		dt = 1;
	}

	for(i = from; i < to; i++) {
		it = &idesc_tab[i];
		ptr = (void *)((int)(&n->player) + it->offset);

		switch(it->type) {
			case DT_CHAR16:
				printf("  (%2d) %16s : \"%s\"\n", i, it->name, (char *)ptr);
				break;
			case DT_PWD:
				printf("  (%2d) %16s : \"%s\"\n", i, it->name, (char *)ptr);
				break;
			case DT_INT:
				printf("  (%2d) %16s : %d\n", i, it->name, *((int *)ptr));
				break;
			case DT_FLOAT:
				printf("  (%2d) %16s : %.2f\n", i, it->name, *((float *)ptr));
				break;
			case DT_TICKS:
				printf("  (%2d) %16s : %d (%.2f hours)\n",
					i, it->name, *((int *)ptr), (*((int *)ptr))/36000.0);
				break;
			case DT_RANK:
				printf("  (%2d) %16s : %d (%s)\n", i, it->name, *((int *)ptr),
					ranks[*((int *)ptr)].name);
				break;
			case DT_ROYAL:
				printf("  (%2d) %16s : %d (%s)\n", i, it->name, *((int *)ptr),
					royal[*((int *)ptr)].name);
				break;
			default:
				printf("Yikes! Unknown it->type in Display()\n");
				break;
		}
	}
	if(dt) {
		tt = n->player.stats.st_lastlogin;
		stm = localtime(&tt);
		hour = stm->tm_hour;
		c = 'A';
		if(!hour) {
			hour = 12;
		} else if(hour > 12) {
			hour -= 12;
			c = 'P';
		}
		printf("        Last login: %2d:%02d %cM  %02d/%02d/%d\n",
		  hour, stm->tm_min, c, stm->tm_mon+1, stm->tm_mday, stm->tm_year);
	}
}

int
CheckChanged(void)
{
	struct plnode *p;

	if(dbDirty) return(1);

	p = firstEnt;
	while(p) {
		if(p->status) return(1);
		p = p->next;
	}
	return(0);
}

void
ClearChanged(void)
{
	struct plnode *p;

	dbDirty = 0;

	p = firstEnt;
	while(p) {
		p->status = 0;
		p = p->next;
	}
}

/* Strip: convert non-printable control chars to ^L notation */
char
*Strip(char *str)
{
	static char buff[36], *o;

	o = buff;
	while(*str) {
		*str &= 0x7f;
		if((int)*str < (int)' ') {
			*o++ = '^';
			*o++ = *str + (char)64;
		} else if ((int)*str == 127) {
			*o++ = '^';
			*o++ = '?';
		} else {
			*o++ = *str;
		}
		str++;
	}
	*o = 0;
	return(buff);
}

