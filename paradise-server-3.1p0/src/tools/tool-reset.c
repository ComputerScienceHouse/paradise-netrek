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

#define STEP 10

static void
usage(char *name)
{
    char *message = 
	"\n\t'%s [-n seconds]'\n\n"
        "Where n is the seconds until the galaxy is reset (default: 10)\n"
        "\nThis tool resets the galaxy.\n";

    fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
    fprintf(stderr, message, name);

    exit(1);
}

static void
zap(void)
{
    int     player;

    for (player = 0; player < MAXPLAYER; player++) {
	if (players[player].p_status == PFREE)
	    continue;
	players[player].p_whydead = KPROVIDENCE;
	players[player].p_whodead = 0;
	players[player].p_status = PEXPLODE;
	players[player].p_explode = 10;
	players[player].p_ntorp = 0;
	players[player].p_nplasmatorp = 0;
    }
}

int
main(int argc, char **argv)
{
    int     i;
    char    buf[1000];
    int     seconds = 10, part;
    int     c;

    while((c = getopt(argc, argv, "n:")) != -1)
    {
      switch(c)
      {
        case 'n':
	  seconds = atoi(optarg);
	  if(seconds < 0)
	    usage(argv[0]);
	  break;
        default:
	  usage(argv[0]);
          break;
      }
    }

    openmem(0, 0);

    part = seconds % STEP;
    if (part)
	sleep(part);

    for (seconds -= part; seconds > 0; seconds -= STEP) {
	sprintf(buf, "->ALL  ** Attention: The galaxy will be reset in %d seconds.", seconds);
	pmessage(buf, 0, MALL, SERVNAME);
	sleep(STEP);
    }

    sprintf(buf, "%s->ALL  ** Manual galaxy reset **", SERVNAME);
    pmessage(buf, 0, MALL, SERVNAME);

    zap();

    /* *newgalaxy = 1;*/
    status2->newgalaxy = 1;
    exit(0);
}
