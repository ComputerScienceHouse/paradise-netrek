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

#include <stdlib.h>
#include "config.h"
#include "proto.h"
#include "tool-util.h"
#include "data.h"
#include "shmem.h"
#include "structdesc.h"

extern struct field_desc ship_fields[];

/* ----------------[ prototypes because I like main first ]---------------- */
void dump_ship_sysdef P((void));
void dump_ship_Ccode P((void));
void dump_ships_to_table P((void));
void describe_ship P((int ship));
void usage P((char *name));

/* internal data */
static char *st[] =
{
  "SCOUT", "DESTROYER", "CRUISER", "BATTLESHIP", "ASSAULT", "STARBASE",
  "ATT", "JUMPSHIP", "FRIGATE", "WARBASE", "LIGHTCRUISER", "CARRIER",
  "UTILITY", "PATROL", NULL
};

#define IS_SINGLE_BIT_FLAG(x) ( ((((x) - 1) ^ (x)) + 1) == ((x) << 1) )

/* --[ rather than duplicate it 3 times make the macro from hell (shrug) ]-- */
/* or what the hell make it a function as long as we're calling it
   from three difference places */
void Print_value(int place, void *temp, int emit_C_code)
{
    switch (ship_fields[place].type) {
        case FT_CHAR:
            printf("%c", *(char *) temp);
            break;
        case FT_SHORT:
            printf("%d", *(short *) temp);
            break;
        case FT_INT:
            printf("%d", *(int *) temp);
            break;
        case FT_LONG:
            printf("%ld", *(long *) temp);
            break;
        case FT_FLOAT:
            printf("%g", *(float *) temp);
            break;
        case FT_STRING:
            printf("%s", (char *) temp);
            break;
        case FT_LONGFLAGS:
            {
                int     zz = 0;
		struct longflags *lfd;
                long    flag = *(long *) temp;
                int     first = 1;
		int     equal = 0;
		char   *sep = (emit_C_code ? " | " : ", ");
		char   *pref = "";
		
		lfd = (struct longflags *)ship_fields[place].aux;

		if(emit_C_code) pref = lfd->prefix;
		
                for (zz = 0; lfd->lfd[zz].name; zz++)
		{
                    if (flag == lfd->lfd[zz].bitvalue)
		    {
                        printf("%s%s%s", first ? "" : sep, pref,
			       lfd->lfd[zz].name);
			equal = 1;
                        first = 0;
                    }
                }
		if(!equal)
		{
		    for (zz = 0; 
		         lfd->lfd[zz].name &&
		           IS_SINGLE_BIT_FLAG(lfd->lfd[zz].bitvalue); zz++)
		    {
		        if(flag & lfd->lfd[zz].bitvalue)
			{
			    printf("%s%s%s", first ? "" : sep, pref,
			           lfd->lfd[zz].name);
			    first = 0;
			}
		    }
		}
            }
            break;
        default:
            printf("unknown type");
            break;
    }
}

/* ==============================[ Functions ]============================== */

int
main(int argc, char **argv)
{
    int     i, droutine = 0, c;
    int     opened = 0;
    char *name;

    name = argv[0];

/* the below is NOT an error - it does not need the braces */
#define OPENMEM if(!opened) openmem(1, 0); opened = 1

    while((c = getopt(argc, argv, "sctv:V")) != -1)
    {
      switch(c)
      {
        case 's':
	    OPENMEM;
	    dump_ship_sysdef();
	    break;
	case 'c':
	    OPENMEM;
	    dump_ship_Ccode();
	    break;
	case 't':
	    OPENMEM;
            dump_ships_to_table();
	    break;
	case 'v':
	    /* check for full ship name first */
	    OPENMEM;
	    for(i = 0; st[i] && strcasecmp(st[i], optarg); i++)
	      ;
	    if(!st[i] && (strlen(optarg) == 2))
	    {
	      int j;

	      for(j = 0; j < NUM_TYPES; j++)
	      {
	        struct ship *shp = &shipvals[j];

	        if(shp->s_desig1 == optarg[0] &&
		   shp->s_desig2 == optarg[1])
		{
		  i = j;
		  break;
		}
	      }
	    }
	    if(!st[i] && isdigit(optarg[0]))
	      i = atoi(optarg);
	    if(i >= 0 && i < NUM_TYPES)
	      describe_ship(i);
	    break;
	case 'V':
	    OPENMEM;
	    for(i = 0; i < NUM_TYPES; i++)
	      describe_ship(i);
	    break;
        default:
	    printf("  %s: Unknown option '-%c'\n", name, c);
	    usage(name);
	    break;
      }
    }

    /* default to .sysdef option */
    if(!opened)
    {
      OPENMEM;
      dump_ship_sysdef();
    }

    exit(0);
}

/* ------------------[ Print ship stats in sysdef format ]------------------ */
void
dump_ship_sysdef(void)
{
    int     j, i;

    for (i = 0; i < NUM_TYPES; i++) {
        struct ship *shp = &shipvals[i];
        printf("SHIP=%d\n", i);
        for (j = 0; ship_fields[j].name; j++) {
            void   *temp = ship_fields[j].offset + (char *) shp;

            printf("%c%c %-24s",
                   shp->s_desig1, shp->s_desig2, ship_fields[j].name);
            Print_value(j, temp, 0);
            printf("\n");
        }
        printf("end\n");
    }
}

/* ----------------[ Print ship stats in a C syntax format ]---------------- */
void
dump_ship_Ccode(void)
{
    int     j, i;

    for (i = 0; i < NUM_TYPES; i++) {
        struct ship *shp = &shipvals[i];
        printf("  /* comprehensive definition of %s */\n", shp->s_name);
        for (j = 0; ship_fields[j].name; j++) {
            void   *temp = ship_fields[j].offset + (char *) shp;

            if (ship_fields[j].type == FT_STRING) {
                printf("  strcpy(shipvals[%s].s_%s, \"%s\")", st[i],
                   ship_fields[j].name, (char *) temp);
            } else {
                printf("  shipvals[%s].s_%s = ",
                       st[i], ship_fields[j].name);
                Print_value(j, temp, 1);
            }
            printf(";\n");
        }
        printf("\n");
    }
}

/* -----------------[ Print ship stats in a table format ]----------------- */
void
dump_ships_to_table(void)
{
    int j, i;

    /* we have to find the max element of the ship fields, this is the
       only way I know of so far (BG) */

    printf("Ship Statistics:\n");
    for (i=0; ship_fields[i].name; i++) {
        printf("%-13s ", ship_fields[i].name);
        for (j=0; j < NUM_TYPES; j++) {
            struct ship *shp = &shipvals[j];
            void   *temp = (ship_fields[i].offset + (char *) shp);

            /* we do this one differently so don't use Print_value() */
            switch (ship_fields[i].type) {
              case FT_CHAR:
                printf("%6c ", *(char *) temp);
                break;
              case FT_SHORT:
                printf("%6d ", *(short *) temp);
                break;
              case FT_INT:
                printf("%6d ", *(int *) temp);
                break;
              case FT_LONG:
                printf("%6ld ", *(long *) temp);
                break;
              case FT_FLOAT:
                printf("%6g ", *(float *) temp);
                break;
              case FT_STRING:
                printf("%6s ", (char *) temp);
                break;
              default:
                break;
            }
        }
        printf("\n");
    }
}


/* -------------------------[ Verbose description ]------------------------- */
void 
describe_ship(int s_no)
{
    struct ship *sp = &shipvals[s_no];
    int     i, j, init = 0, equal;
    struct longflags *lfd;

    for(j = 0; ship_fields[j].name; j++)
    {
      if(ship_fields[j].type == FT_LONGFLAGS)
      {
        long flags = *(long *) ((char *)(sp) + ship_fields[j].offset);

	if(!init)
	{
	  printf("The %s\n", sp->s_name);
	  init = 1;
	}

        equal = 0;
	lfd = (struct longflags *)ship_fields[j].aux;

	for(i = 0; lfd->lfd[i].name; i++)
	{
	  if(flags == lfd->lfd[i].bitvalue)
	  {
	    equal = 1;
	    printf("\t%s\n", lfd->lfd[i].help);
	    break;
	  }
	}

	for(i = 0; lfd->lfd[i].name && !equal; i++)
	{
	  if(flags & lfd->lfd[i].bitvalue)
	    printf("\t%s\n", lfd->lfd[i].help);
	}
      }
    }
}


/* ----------------------------[ Prints usage ]---------------------------- */
void
usage(char *name)
{
    int x;

    char *errmsg = 
        "\n\t'%s [format options]'\n\n"
        "This tool will dump all ship values:\n"
        "\t-s       -- .sysdef format (default)\n"
        "\t-c       -- C code format\n"
        "\t-t       -- table format\n"
        "\t-v SHIP  -- verbose format - specify SHIP by name, abbr. or no.\n"
	"                               (e.g. -v SCOUT, -v DD, -v 4)\n"
	"\t-V       -- verbose format, all ships\n";

    fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
    fprintf(stderr, errmsg, name);

    exit(1);
}
