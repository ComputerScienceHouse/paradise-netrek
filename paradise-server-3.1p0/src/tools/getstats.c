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

int
main(int argc, char **argv)
{
    FILE   *f;
    struct statentry s;

    if (argc != 2) {
        printf("Usage: %s <player file>\n", argv[0]);
	exit(1);
    }

    f = fopen(argv[1], "r");
    if (f == NULL) {
	printf("Cannot open players file (%s)\n", argv[1]);
	exit(1);
    }
    while (fread(&s, sizeof(struct statentry), 1, f) == 1) {
	printf("\nPlayer: %s\n", s.name);
	printf("Genocides: %d\n", s.stats.st_genocides);
	printf("Maxkills: %f\n", s.stats.st_tmaxkills);
	printf("DI: %f\n", s.stats.st_di);
	printf("Kills: %d\n", s.stats.st_tkills);
	printf("Losses: %d\n", s.stats.st_tlosses);
	printf("Armies bombed: %d\n", s.stats.st_tarmsbomb);
	printf("Resources bombed: %d\n", s.stats.st_tresbomb);
	printf("Dooshes: %d\n", s.stats.st_tdooshes);
	printf("Planets: %d\n", s.stats.st_tplanets);
	printf("Time: %f\n", (float) s.stats.st_tticks / 36000.0);
	printf("Rank: %d\n", s.stats.st_rank);
	printf("Royalty: %d\n", s.stats.st_royal);

	printf("SB kills: %d\n", s.stats.st_sbkills);
	printf("SB losses: %d\n", s.stats.st_sblosses);
	printf("SB time: %f\n", (float) s.stats.st_sbticks / 36000.0);
	printf("SB maxkills: %f\n", s.stats.st_sbmaxkills);

	printf("WB kills: %d\n", s.stats.st_wbkills);
	printf("WB losses: %d\n", s.stats.st_wblosses);
	printf("WB time: %f\n", (float) s.stats.st_wbticks / 36000.0);
	printf("WB maxkills: %f\n", s.stats.st_wbmaxkills);

	printf("JS planets: %d\n", s.stats.st_jsplanets);
	printf("JS time: %f\n", (float) s.stats.st_jsticks / 36000.0);
    }
    fclose(f);
    exit(0);
}
