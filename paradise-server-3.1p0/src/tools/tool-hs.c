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
  This is probably broken in anything but the default config
*/

#include <sys/stat.h>
#define PLAYER_EDITOR
#include "config.h"
#include "proto.h"
#include "tool-util.h"
#include "data.h"

static int     topn = 1;
static char  **names = NULL;	/* if we want stats for a set of names */
static int     nplayers;
static struct statentry *database;

struct highscore {
    char    name[32];
    int     di, tkills, tlosses, tarmsbomb, tresbomb, tdooshes, tplanets;
    int     ticks;
    struct highscore *next;
};

static struct highscore *scores;
static int     scoresize, nscores;

void 
subbrag(char *name, int stuff, int time, char *title, char *descr, int num)
{
    char    full[64];
    char    line[80];
    int     len, tlen;
    int     i;
    double  rate;

    if (title) {
	sprintf(full, "%s: %s", title, name);

	len = strlen(full);
	tlen = title ? strlen(title) : 0;

	for (i = len; i - tlen < 10; i++)
	    full[i] = ' ';
	full[i] = 0;
    }
    else {
	strcpy(full, name);
    }

    if (topn != 1)
	sprintf(line, "%15s %3d over ", full, stuff);
    else
	sprintf(line, "%-30s (%2d) %3d %s, over ", full, num, stuff, descr);

    if (time / 10.0 > 3600)
	sprintf(line, "%s%1.2f hours ", line, time / 36000.0);
    else
	sprintf(line, "%s%1.2f minutes ", line, time / 600.0);

    if (topn == 1)
	sprintf(line, "%s\n%40s", line, "");

    rate = stuff / (time / 600.0);

    if (rate < 1) {
	printf(line);
	printf("(%1.2f minutes per)\n", 1 / rate);
    }
    else if (rate > 60) {
	printf(line);
	printf("(%1.2f per hour)\n", rate / 60);
    }
    else {
	printf(line);
	printf("(%1.2f per minute)\n", rate);
    }
}

void 
brag(char *title, char *descr, int offset)
{
    int     i, j;

    if (names) {
	for (i = 0; i < nscores; i++) {
	    for (j = 0; names[j]; j++) {
	        if(!strcmp(scores[i].name, names[j])) {
                    printf("#%5d: ", i + 1);
		    subbrag("", *(int *) (offset + (char *) &scores[i]),
		            scores[i].ticks, title, descr, i);
		    break;	/* out of j loop */
		}
	    }
	}
    }
    else {
	if (topn != 1)
	    printf("\n%s (%s)\n", title, descr);
	for (i = 0; i < topn && i < nscores; i++) {
            printf("%10s", "");
	    subbrag(scores[i].name, *(int *) (offset + (char *) &scores[i]),
		    scores[i].ticks, topn == 1 ? title : (char *) 0, descr, i);
	}
    }
}

#if __STDC__
#define COMPUTE(STN) \
    do { \
      currscore->STN = currplayer.stats.st_ ## STN - \
	   prevplayer->stats.st_ ## STN; \
    } while (0)


#define COMPARE(STN) \
int \
cmp_raw ## STN(const void *aa, const void *bb) \
{ \
  const struct highscore *a = (const struct highscore *)aa; \
  const struct highscore *b = (const struct highscore *)bb; \
  int	diff = a->STN - b->STN; \
 \
  if (diff<0) \
    return 1; \
  else if (diff==0) \
    return 0; \
  else \
    return -1; \
} \
 \
int \
cmp_per ## STN(const void *aa, const void *bb) \
{ \
  const struct highscore *a = (const struct highscore *)aa; \
  const struct highscore *b = (const struct highscore *)bb; \
  double	diff = a->STN/(double)a->ticks - b->STN/(double)b->ticks; \
 \
  if (diff<0) \
    return 1; \
  else if (diff==0) \
    return 0; \
  else \
    return -1; \
}
#else
#define COMPUTE(STN) \
    do { \
      currscore->STN = currplayer.stats.st_/**/STN - \
	   prevplayer->stats.st_/**/STN; \
    } while (0)


#define COMPARE(STN) \
int \
cmp_raw/**/STN(const void *aa, const void *bb) \
{ \
  const struct highscore *a = (const struct highscore *)aa; \
  const struct highscore *b = (const struct highscore *)bb; \
  int	diff = a->STN - b->STN; \
 \
  if (diff<0) \
    return 1; \
  else if (diff==0) \
    return 0; \
  else \
    return -1; \
} \
 \
int \
cmp_per/**/STN(const void *aa, const void *bb) \
{ \
  const struct highscore *a = (const struct highscore *)aa; \
  const struct highscore *b = (const struct highscore *)bb; \
  double	diff = a->STN/(double)a->ticks - b->STN/(double)b->ticks; \
 \
  if (diff<0) \
    return 1; \
  else if (diff==0) \
    return 0; \
  else \
    return -1; \
}
#endif

COMPARE(di)
COMPARE(tkills)
COMPARE(tlosses)
COMPARE(tarmsbomb)
COMPARE(tresbomb)
COMPARE(tdooshes)
COMPARE(tplanets)

int
cmp_ticks(const void *aa, const void *bb)
{
    const struct highscore *a = (const struct highscore *)aa;
    const struct highscore *b = (const struct highscore *)bb;
    int     diff = a->ticks - b->ticks;

    if (diff < 0)
	return 1;
    else if (diff == 0)
	return 0;
    else
	return -1;
}

static struct statentry zeroplayer;

int 
different(struct highscore *one, struct highscore *two)
{
    return 0 != strcmp(one->name, two->name);
}

void
usage(char *name)
{
    char *message = 
	"\nHigh Scores, created by comparing two databases.\n"
	"\n\t'%s -n <num> -c <num> [-name <name>] <old db> [new db]'\n\n"
	"Options:\n"
	"\t-n num        How many high scores to print (default 10)\n"
	"\t-c num        Which category (0 is all (default), max available is 15)\n"
	"\t-p string     print ranking for a particular player\n"
	"\nExample:\t'%1$s -n 5 -c 1 etc/players.db.old '\n\n";

    fprintf(stderr, "--- NetrekII (Paradise), %s ---\n", PARAVERS);
    fprintf(stderr, message, name);

    exit(1);
}

int 
main(int argc, char **argv)
{
    struct stat fstats;
    FILE   *fp;
    int     i, c, pn = 0;
    int     code = 0;
    struct statentry currplayer;
    char  **av;
    float mintime=30.0;

    while((c = getopt(argc, argv, "n:c:p:")))
    {
      switch(c)
      {
        case 'n':
	  topn = atoi(optarg);
	  if(topn < 1)
	    topn = 10;
	  break;
	case 'c':
	  code = atoi(optarg);
	  if(code < 0 || code > 15)
	    usage(argv[0]);
	  break;
        case 'p':
	  pn++;
	  break;
	default:
	  usage(argv[0]);
	  break;
      }
    }

    if((optind == argc) || (argc - optind > 2))
      usage(argv[0]);

    if(pn > 0)
    {
      names = (char **)malloc(sizeof(char *) * (pn + 1));
      pn = 0;
      while((c = getopt(argc, argv, "n:c:p:")))
      {
        if(c == 'p')
	  names[pn++] = optarg;
      }
      names[pn] = NULL;
    }

    av = argv + optind;

    fp = fopen(av[0], "r");
    if (fp == 0) {
	fprintf(stderr, "Couldn't open file %s for read", av[0]);
	perror("");
	exit(1);
    }

    if (fstat(fileno(fp), &fstats) < 0) {
	fprintf(stderr, "Couldn't fstat file %s", av[0]);
	perror("");
	exit(1);
    }

    nplayers = fstats.st_size / sizeof(*database);
    database = (struct statentry *) malloc(sizeof(*database) * nplayers);

    i = fread(database, sizeof(*database), nplayers, fp);

    if (i == 0) {
	fprintf(stderr, "failed to read any player records from file %s\n", av[0]);
	exit(1);
    }
    if (i != nplayers) {
	fprintf(stderr, "failed to read all player records from file %s (%d of %d)\n", av[0], i, nplayers);
	nplayers = i;
    }

    fclose(fp);

    fp = fopen((argc - optind == 2 ? av[1] : build_path(PLAYERFILE)), "r");
    if (fp == 0) {
	fprintf(stderr, "Couldn't open file %s for read", av[1]);
	perror("");
	exit(1);
    }

    scores = (struct highscore *) malloc(sizeof(*scores) * (scoresize = 256));
    nscores = 0;

    while (1) {
	int     dt;
	struct statentry *prevplayer;
	struct highscore *currscore;

	i = fread(&currplayer, sizeof(currplayer), 1, fp);
	if (i < 0) {
	    fprintf(stderr, "error reading player record, aborting loop\n");
	    perror("");
	}
	if (i <= 0)
	    break;

	for (i = 0; i < nplayers; i++) {
	    if (0 == strcmp(database[i].name, currplayer.name))
		break;
	}
	if (i < nplayers)
	    prevplayer = &database[i];
	else
	    prevplayer = &zeroplayer;

	dt = currplayer.stats.st_tticks - prevplayer->stats.st_tticks;

	if (dt < mintime /* minutes */ * 60 * 10)
	    continue;

	if (nscores >= scoresize) {
	    scores = (struct highscore *) realloc(scores, sizeof(*scores) * (scoresize *= 2));
	}
	currscore = &scores[nscores++];
	strcpy(currscore->name, currplayer.name);
	currscore->ticks = dt;

	COMPUTE(di);
	COMPUTE(tkills);
	COMPUTE(tlosses);
	COMPUTE(tarmsbomb);
	COMPUTE(tresbomb);
	COMPUTE(tdooshes);
	COMPUTE(tplanets);
    }


#define offset(field) ( (int)&(((struct highscore*)0)->field) )

    if (!code || code == 1) {
	qsort(scores, nscores, sizeof(*scores), cmp_rawdi);
	brag("Lord of Destruction", "most destruction inflicted", offset(di));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 2) {
	qsort(scores, nscores, sizeof(*scores), cmp_perdi);
	brag("BlitzMeister", "fastest destruction inflicted", offset(di));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 3) {
	qsort(scores, nscores, sizeof(*scores), cmp_rawtkills);
	brag("Hitler", "most opponents defeated", offset(tkills));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 4) {
	qsort(scores, nscores, sizeof(*scores), cmp_pertkills);
	brag("Terminator", "fastest opponents defeated", offset(tkills));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 5) {
	qsort(scores, nscores, sizeof(*scores), cmp_rawtlosses);
	brag("Kamikaze", "most times down in flames", offset(tlosses));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 6) {
	qsort(scores, nscores, sizeof(*scores), cmp_pertlosses);
	brag("Speed Kamikaze", "fastest times down in flames", offset(tlosses));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 7) {
	qsort(scores, nscores, sizeof(*scores), cmp_rawtarmsbomb);
	brag("Carpet Bomber", "most armies bombed", offset(tarmsbomb));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 8) {
	qsort(scores, nscores, sizeof(*scores), cmp_pertarmsbomb);
	brag("NukeMeister", "fastest armies bombed", offset(tarmsbomb));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 9) {
	qsort(scores, nscores, sizeof(*scores), cmp_rawtresbomb);
	brag("Terrorist", "most resources leveled", offset(tresbomb));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 10) {
	qsort(scores, nscores, sizeof(*scores), cmp_pertresbomb);
	brag("Democrat", "fastest resources leveled", offset(tresbomb));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 11) {
	qsort(scores, nscores, sizeof(*scores), cmp_rawtdooshes);
	brag("Executioner", "most armies dooshed", offset(tdooshes));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 12) {
	qsort(scores, nscores, sizeof(*scores), cmp_pertdooshes);
	brag("DooshMeister", "fastest armies dooshed", offset(tdooshes));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 13) {
	qsort(scores, nscores, sizeof(*scores), cmp_rawtplanets);
	brag("Diplomat", "most planets taken", offset(tplanets));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

    if (!code || code == 14) {
	qsort(scores, nscores, sizeof(*scores), cmp_pertplanets);
	brag("speed Diplomat", "fastest planets taken", offset(tplanets));
    }

    if (!code && topn > 5)
	printf("\t@@b\n");

#if 0
    if (!code || code == 15) {
	qsort(scores, nscores, sizeof(*scores), cmp_ticks);
	if (name[0] != 0) {
	    for (i = 0; i < nscores; i++) {
		if (0 == strcmp(scores[i].name, name)) {
		    printf("#%5d:%30s with %1.2f hours\n", i + 1, scores[i].name,
			   scores[i].ticks / 36000.0);
		    break;
		}
	    }
	}
	else if (topn > 1) {
	    int     i;
	    printf("Addicts:\n");
	    for (i = 0; i < topn && i < nscores; i++) {
		printf("%30s with %1.2f hours\n", scores[i].name, scores[i].ticks / 36000.0);
	    }
	}
	else {
	    printf("Addict: %s with %1.2f hours\n", scores[0].name, scores[0].ticks / 36000.0);
	}
    }
#endif

    exit(0);
}
