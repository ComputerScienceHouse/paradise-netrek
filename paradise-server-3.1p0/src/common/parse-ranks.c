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

#include <ctype.h>
#include "config.h"
#include "proto.h"
#include "data.h"

typedef struct tagRANK_NODE
{
  struct rank r;
  struct tagRANK_NODE *next;
} rank_node;

typedef struct tagROYAL_NODE
{
  struct royalty r;
  struct tagROYAL_NODE *next;
} royal_node;

/* build in a default table that can be used if file not available or
   no ranks available */
static char *builtin_ranks[] = 
{
 "RANKS",
 "Recruit:        0,    0, 0.00, 0.0, 0.0",
 "Specialist:     1,   10, 0.30, 0.3, 0.0",
 "Cadet:          2,   25, 0.40, 0.6, 0.0",
 "Midshipman:     3,   45, 0.50, 0.9, 0.0",
 "Ensn., J.G.:    4,   70, 0.70, 1.2, 0.0", 
 "Ensign:         5,  100, 0.90, 1.5, 0.0",
 "Lt., J.G.:      6,  140, 1.10, 2.0, 0.0",
 "Lieutenant:     8,  190, 1.30, 2.5, 0.0",
 "Lt. Cmdr.:     10,  250, 1.50, 3.0, 0.5",
 "Commander:     15,  300, 1.80, 3.5, 0.7",
 "Captain:       20,  350, 2.00, 4.0, 1.0",
 "Fleet Capt.:   25,  400, 2.10, 4.3, 2.5",
 "Commodore:     50,  500, 2.15, 4.8, 3.0",
 "Rear Adml.:    75,  700, 2.20, 5.3, 3.3",
 "Admiral:      100, 1000, 2.25, 5.7, 3.6",
 "ROYALTY",
 "Wesley",
 "Governor",
 "Emperor",
 "Q",
 NULL
};

static int
rank_order_fn(const void *aa, const void *bb)
{
  const struct rank *r1 = (const struct rank *)aa;
  const struct rank *r2 = (const struct rank *)bb;

  if(r1->genocides < r2->genocides)
    return(-1);
  if(r1->genocides > r2->genocides)
    return(1);
  return(0);
}

static char *
get_rank_line(FILE *f)
{
  static int builtin_line = 0;
  char *rv;
  static char buf[512];

  if(!f)
  {
    rv = builtin_ranks[builtin_line++];
    if(rv == NULL)
      builtin_line = 0;
    return(rv);
  }
  return(fgets(buf, 511, f));
}

void
parse_ranks(char *filename)
{
  FILE *f;
  int nlines = 0;
  char *line, *p, *q, *s;
  char nbuf[512];
  enum { RANK_MODE, ROYAL_MODE } mode;
  struct rank k;
  rank_node *rank_head = NULL, *rank_tail = NULL;
  royal_node *royalty_head = NULL, *royalty_tail = NULL;
  int i;

  mode = RANK_MODE;
  f = fopen(filename, "r");

  while((line = get_rank_line(f)))
  {
    int in_name;

    nlines++;

    /* remove comments */
    p = strchr(line, '#');
    if(p)
      *p = 0;
    
    /* and spaces/tabs */
    p = line; q = nbuf;
    in_name = 1;
    while(*p)
    {
      if(*p == ':')
        in_name = 0;
      if(in_name || !isspace(*p))
        *q++ = *p;
      p++;
    }
    *q = 0;

    /* Always strip leading blanks. */
    p = nbuf;
    while(*p && isspace(*p))
      *p++ = 0;
    
    /* strip trailing blanks if already in name */
    if(in_name)
    {
      if(*p)
      {
	q = p + strlen(p) - 1;
	while(q > p && isspace(*q))
	  *q-- = 0;
      }
    }
    else
      q = p + strlen(p) - 1;

    for(s = nbuf; s - nbuf < q - p + 1; s++)
      *s = p[s-nbuf];
    *s = 0;

    /* mode switch? */
    if(!strcasecmp(nbuf, "ranks"))
    {
      mode = RANK_MODE;
      continue;
    }
    if(!strcasecmp(nbuf, "royalty"))
    {
      mode = ROYAL_MODE;
      continue;
    }

    if(mode == ROYAL_MODE)
    {
      /* new royalty? */
      if(strlen(nbuf) > 0)
      {
        royal_node *nr = (royal_node *)malloc(sizeof(royal_node));

        if(!royalty_head)
	  royalty_head = royalty_tail = nr;
        else
	{
	  royalty_tail->next = nr;
	  royalty_tail = nr;
	}

	nr->r.name = strdup(nbuf);
	nr->next = NULL;
	NUMROYALRANKS++;
      }
    }
    else
    {
      rank_node *nr;
      int params;

      /* Anything to process? */
      if(strlen(nbuf) == 0)
        continue;

      /* find name/stats separator */
      p = strchr(nbuf, ':');
      if(!p)
      {
        fprintf(stderr, "Name/stats separator ':' missing at line %d\n", 
	                nlines);
	continue;
      }
      q = p+1;
      *p-- = 0;

      /* strip trailing spaces from rank name */
      while(p >= nbuf && isspace(*p))
        *p-- = 0;
       
      if((params = sscanf(q, "%d,%f,%f,%f,%f", &k.genocides, &k.di, 
                          &k.battle, &k.strategy, &k.specship)) != 5)
      {
        fprintf(stderr, "Not enough stats parameters for rank %s on line %d\n"
	                "  (needed 5, got %d)\n",
	                s, nlines, params);
	continue;
      }

      nr = (rank_node *)malloc(sizeof(rank_node));
      if(!rank_head)
        rank_head = rank_tail = nr;
      else
      {
        rank_tail->next = nr;
	rank_tail = nr;
      }

      memcpy(&nr->r, &k, sizeof(struct rank));
      nr->r.name = strdup(nbuf);
      nr->next = NULL;
      NUMRANKS++;
    }
  }

  ranks = (struct rank *)malloc(NUMRANKS * sizeof(struct rank));
  i = 0;
  while(rank_head)
  {
    rank_node *or = rank_head;

    memcpy(&ranks[i], &or->r, sizeof(struct rank));
    rank_head = or->next;
    free(or);
    i++;
  }
  qsort(ranks, NUMRANKS, sizeof(struct rank), rank_order_fn);

  NUMROYALRANKS++;
  royal = (struct royalty *)malloc(NUMROYALRANKS * sizeof(struct royalty));
  i = 0;
  /* oop, almost forgot index 0 has no royalty */
  royal[i++].name = strdup("none");
  while(royalty_head)
  {
    royal_node *or = royalty_head;

    memcpy(&royal[i], &or->r, sizeof(struct royalty));
    royalty_head = or->next;
    free(or);
    i++;
  }

  if(f)
    fclose(f);
}
