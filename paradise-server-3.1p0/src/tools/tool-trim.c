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

/*
 *  Kevin P. Smith      12/05/88
 *
 *  Modified for Paradise by Rob Forsman 1993
 *  Cleaned up by Brandon Gillespie Sept 17 1994
 */

#include "config.h"
#include "proto.h"
#include "tool-util.h"
#include "data.h"

static struct statentry kplayer;

/* prototypes */
void trimblanks2 P((char *));
void trimblanks P((char *));
void usage P((char *));

int
main(int argc, char **argv)
{
    struct statentry plstats;
    int     i;
    int     harsh = 10;        /* How strict we will be with player trimming */
    char   *me;
    FILE   *infile = stdin;   /* these used to actually read in from the
                                 actual file, Rob changed them to stdin/out,
                                 and I don't mind them that way (it actually
                                 makes it a little easier */
    FILE   *outfile = stdout;

    me = *argv;

    if (argc == 2) {
        if (*argv[1] == '-')
            usage(me);
        else
            harsh = atoi(argv[1]);
    }

    if (argc > 2 || harsh <= 0)
        usage(me);

    fprintf(stderr, "%s: If you do not know how to use this program, break now and type '%s -h'\n", me, me);

    i = 0;
    while (1 == fread(&plstats, sizeof(plstats), 1, infile)) {
    /* Player 0 is always saved. */
    /*
       This formula reads: If (deadtime - (10 + rank^2 + playtime/2.4)*n days > 0), nuke him.
    */
    if (i != 0
        && harsh < 100
        && ((time(NULL) - plstats.stats.st_lastlogin) >
        (10 +
         plstats.stats.st_rank * plstats.stats.st_rank +
         (plstats.stats.st_tticks +
          plstats.stats.st_sbticks +
          plstats.stats.st_wbticks +
          plstats.stats.st_jsticks) / 2.4) * harsh * 24 * 60 * 60)
        ) {
        fprintf(stderr,
                "%-16.16s %7.2f   %4d   %4d   %4d   %4d\n",
                plstats.name,
                plstats.stats.st_tticks / 36000.0,
                plstats.stats.st_tplanets,
                kplayer.stats.st_tarmsbomb,
                kplayer.stats.st_tkills,
                kplayer.stats.st_tlosses);
        continue;
    }
    if (outfile) {
        fwrite(&plstats, sizeof(plstats), 1, outfile);
    }
        i++;
    }
    exit(0);
}

void
usage(char *me)
{
    int x;
    char message[][255] = {
        "\n\t'%s [n] < old-playerdb > new-playerdb'\n",
        "\nThis program trims a player database file.  The level of niceness is\n",
        "determined by 'n' (default: 10).  The old player database is read from stdin,\n",
        "the new player database is written to stdout, a quick description of players\n",
        "who were removed is written to stderr.  It will remove any character who\n",
        "has not played for n*10 days, as well as giving some other consideration\n",
        "to their varied statistics.  The actual formula is:\n\n",
        "\tIf (deadtime - (10 + rank^2 + playtime/2.4)*n days > 0), nuke him.\n\n"
    };

    fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
    for (x=0; *message[x] != '\0'; x++)
        fprintf(stderr, message[x], me);
        
    exit(0);
}

void
trimblanks2(char *str)
{
    *str = 0;
    str--;
    while (*str == ' ') {
        *str = 0;
        str--;
    }
    if (*str == '_')
        *str = 0;
}

void
trimblanks(char *str)
{
    *str = 0;
    str--;
    while (*str == ' ') {
        *str = 0;
        str--;
    }
}
