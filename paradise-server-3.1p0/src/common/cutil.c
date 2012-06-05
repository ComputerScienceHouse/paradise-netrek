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

/* cutil.c - misc utility fuctions common to all the binaries */

#include "config.h"
#include "proto.h"

#include <signal.h>
#include <sys/types.h>
#include <string.h>

/* r_signal - reliable version of signal()
   the System-V signal() function provides the older, unreliable signal
   semantics.  So, this is an implementation of signal using sigaction. */

void (*r_signal(int sig, void (*func)() )) ()
{
    struct sigaction act, oact;

    act.sa_handler = func;

    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if( sig != SIGALRM )
        act.sa_flags |= SA_RESTART;

    if (sigaction(sig, &act, &oact) < 0)
	return (SIG_ERR);

    return (oact.sa_handler);
}

#ifndef HAVE_STRDUP
char *
strdup(char *str)
{
    char   *s;

    s = (char *) malloc(strlen(str) + 1);
    if(!s)
	return NULL;
    strcpy(s, str);
    return s;
}
#endif
