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
#include "ntserv.h"

#define pm_error(str) \
{\
  fprintf(stderr, "Error in ParseXbmFile(): %s\n", str);\
  if(*dataP) \
  { \
    free(*dataP); \
    *dataP = 0; \
  }\
return(-1); \
}

/* most of the following function came from xbmtopbm.c in the PBMPLUS
   library */

#define MAX_LINE 500

int 
ParseXbmFile(FILE *stream, int *widthP, int *heightP, unsigned char **dataP)
{
    char    line[MAX_LINE], name_and_type[MAX_LINE];
    unsigned char   *ptr;
    char   *t;
    int     raster_length, v;
    register int bytes, bytes_per_line;
    register int c1, c2, value1, value2;
    int     hex_table[256];

    *widthP = *heightP = -1;
    *dataP = 0;

    for (;;) {
	if (fgets(line, MAX_LINE, stream) == NULL)
	    pm_error("EOF / read error");
	if (strlen(line) == MAX_LINE - 1)
	    pm_error("line too long");

	if (sscanf(line, "#define %s %d", name_and_type, &v) == 2) {
	    if ((t = strrchr(name_and_type, '_')) == NULL)
		t = name_and_type;
	    else
		++t;
	    if (!strcmp("width", t))
		*widthP = v;
	    else if (!strcmp("height", t))
		*heightP = v;
	    continue;
	}

	if (sscanf(line, "static char %s = {", name_and_type) == 1 ||
	    sscanf(line, "static unsigned char %s = {", name_and_type) == 1)
	    break;
    }

    if (*widthP == -1)
	pm_error("invalid width");
    if (*heightP == -1)
	pm_error("invalid height");

    bytes_per_line = (*widthP + 7) / 8;

    raster_length = bytes_per_line * *heightP;
    *dataP = (unsigned char *) malloc(raster_length);
    if (*dataP == NULL)
	pm_error("out of memory");

    /* Initialize hex_table. */
    for (c1 = 0; c1 < 256; ++c1)
	hex_table[c1] = 256;

    hex_table['0'] = 0;
    hex_table['1'] = 1;
    hex_table['2'] = 2;
    hex_table['3'] = 3;
    hex_table['4'] = 4;
    hex_table['5'] = 5;
    hex_table['6'] = 6;
    hex_table['7'] = 7;
    hex_table['8'] = 8;
    hex_table['9'] = 9;
    hex_table['A'] = 10;
    hex_table['B'] = 11;
    hex_table['C'] = 12;
    hex_table['D'] = 13;
    hex_table['E'] = 14;
    hex_table['F'] = 15;
    hex_table['a'] = 10;
    hex_table['b'] = 11;
    hex_table['c'] = 12;
    hex_table['d'] = 13;
    hex_table['e'] = 14;
    hex_table['f'] = 15;

    for (bytes = 0, ptr = *dataP; bytes < raster_length; ++bytes) {
	/*
	   * Skip until digit is found.
	*/
	for (;;) {
	    c1 = getc(stream);
	    if (c1 == EOF)
		pm_error("EOF / read error");
	    value1 = hex_table[c1];
	    if (value1 != 256)
		break;
	}
	/*
	   * Loop on digits.
	*/
	for (;;) {
	    c2 = getc(stream);
	    if (c2 == EOF)
		pm_error("EOF / read error");
	    value2 = hex_table[c2];
	    if (value2 != 256) {
		value1 = (value1 << 4) | value2;
		if (value1 >= 256)
		    pm_error("syntax error");
	    }
	    else if (c2 == 'x' || c2 == 'X') {
		if (value1 == 0)
		    continue;
		else
		    pm_error("syntax error");
	    }
	    else
		break;
	}
	*ptr++ = (unsigned char) (value1 ^ 0xFF);
    }
    return (raster_length);
}
