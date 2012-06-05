/*------------------------------------------------------------------
  Copyright 1989		Kevin P. Smith
				Scott Silvey

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies.

  NETREK II -- Paradise

  Permission to use, copy, modify, and distribute this software and
  its documentation, or any derivative works thereof,  for any 
  NON-COMMERCIAL purpose and without fee is hereby granted, provided
  that this copyright notice appear in all copies.  No
  representations are made about the suitability of this software for
  any purpose.  This software is provided "as is" without express or
  implied warranty.

	Xtrek Copyright 1986			Chris Guthrie
	Netrek (Xtrek II) Copyright 1989	Kevin P. Smith
						Scott Silvey
	Paradise II (Netrek II) Copyright 1993	Larry Denys
						Kurt Olsen
						Brandon Gillespie
		                Copyright 2000  Bob Glamm

--------------------------------------------------------------------*/

#include "config.h"
#include "proto.h"
#include "tool-util.h"
#include "data.h"
#include "shmem.h"

char 
team_to_letter(int t)
{
    switch (t) {
    case FED:
	return 'F';
    case ROM:
	return 'R';
    case KLI:
	return 'K';
    case ORI:
	return 'O';
    default:
	return 'I';
    }
}

int 
letter_to_team(char ch)
{
    switch (ch) {
    case 'I':
    case 'i':
	return 0;
    case 'F':
    case 'f':
	return FED;
    case 'R':
    case 'r':
	return ROM;
    case 'K':
    case 'k':
	return KLI;
    case 'O':
    case 'o':
	return ORI;
    default:
	return -1;
    }
}

int 
letter_to_pnum(char ch)
{
    switch (ch) {
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
	return ch - '0';

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
	return ch - 'a' + 10;
    default:
	return -1;
    }
}

char 
pnum_to_letter(int ch)
{
    switch (ch) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
	return ch + '0';
    default:
	return ch - 10 + 'a';
    }
}


char *
twoletters(struct player *pl)
/* calculate the two letters that form the players designation (e.g. R4) */
{
#define RINGSIZE MAXPLAYER+3
    static char buf[RINGSIZE][3];	/* ring of buffers so that this */
    static int idx;		/* proc can be called several times before
				   the results are used */
    if (idx >= RINGSIZE)
	idx = 0;
    buf[idx][0] = teams[pl->p_team].letter;
    buf[idx][1] = shipnos[pl->p_no];
    buf[idx][2] = 0;
    return buf[idx++];
}
