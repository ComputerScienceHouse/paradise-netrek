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
 * Robert Forsman
 *
 * Scans the score file for royalty.
 */

#define PLAYER_EDITOR
#include "config.h"
#include "proto.h"
#include "tool-util.h"
#include "data.h"

struct person {
  int	royal;
  char	name[16];
  struct person *next;
};

void
usage(char *name)
{
    char *errmsg =
        "\n\t'%s [options]'\n\n"
	"This tool will scan & dump royalty in the player database:\n"
	"\t-f FILE   -- use FILE as the player datbase\n";
    
    fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
    fprintf(stderr, errmsg, name);

    exit(1);
}

int
compare_people(struct person *a, struct person *b)
{
    if (a->royal > b->royal)
	return 1;
    else
	return -1;
}

int
main(int argc, char **argv)
{
    struct statentry plstats;
    struct person	*head=0;
    int	royalty, c;
    char *fn, *pfs = NULL;
    FILE *pf;

    while((c = getopt(argc, argv, "f:")) != -1)
    {
      switch(c)
      {
        case 'f':
	  pfs = optarg;
	  break;
	default:
	  usage(argv[0]);
	  break;
      }
    }

    fn = build_path(RANKS_FILE);
    init_data(fn);

    fn = (pfs ? pfs : build_path(PLAYERFILE));
    pf = fopen(fn, "r");
    if(!pf)
    {
      char buf[512];

      sprintf(buf, "Cannot open player file %s", fn);
      perror(buf);
      exit(1);
    }

    while (1 == fread(&plstats, sizeof(plstats), 1, pf)) {
	if (plstats.stats.st_royal>0) {
	    /* royalty!  insert into linked list. */
	    struct person **scan;
	    struct person *dude;
	    dude = (struct person*)malloc(sizeof(*dude));
	    dude->royal = plstats.stats.st_royal;
	    strncpy(dude->name, plstats.name, sizeof(dude->name));
	    for (scan=&head;
		 *scan && 0>compare_people(dude, *scan);
		 scan = &(*scan)->next)
		;
	    dude->next = *scan;
	    *scan = dude;
	}
    }
    fclose(pf);


    royalty = -1;
    while (head) {
	struct person	*temp;
	if (royalty != head->royal) {
	    royalty = head->royal;
	    printf("%s:\n", royal[royalty].name);
	}
	printf("  %s\n", head->name);
	temp = head;
	head = head->next;
	free(temp);
    }

    exit(0);
}
