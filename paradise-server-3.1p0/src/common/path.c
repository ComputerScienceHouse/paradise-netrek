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

/* This is the default path, you don't have to change this as long as
 * you set the NETREKDIR env
 */
static char netrekpath[256] = "/usr/local/paradise";

char *
build_path(char *suffix)
{
    static char buf[512];
    static int initted = 0;
    int     len;

    if (!initted) {
	char   *ptr;
	initted = 1;
	ptr = (char *) getenv("NETREKDIR");
	if (ptr == NULL) {
	    fprintf(stderr, "Warning, no NETREKDIR envariable.  Using default of %s\n", netrekpath);
	}
	else {
	    strncpy(netrekpath, ptr, 255);
	    netrekpath[255] = '\0';
	}
	len = strlen(netrekpath);
	if (len > 230) {
	    fprintf(stderr, "NETREKDIR enviroment variable too long.\n");
	    fprintf(stderr, "Please change it.\n");
	    exit(1);
	}
	if (netrekpath[len - 1] != '/') {
	    netrekpath[len] = '/';
	    netrekpath[len + 1] = '\0';
	}
    }
    if (*suffix == '/') {	/* absolute path... don't prepend anything */
	strcpy(buf, suffix);
    }
    else {
	strcpy(buf, netrekpath);
	strcat(buf, suffix);
    }

    return buf;
}
