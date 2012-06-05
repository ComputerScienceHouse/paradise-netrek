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

/* ------------------------------------------------------------------- */
static char *statnames[] = {"F", "O", "A", "E", "D", "Q"};

static char *oldranknames[] = {
              "Ensign", "Lieutenan",
              "Lt. Cmdr.", "Commander",
              "Captain", "Flt. Capt",
              "Commodore", "Rear Adml",
              "Admiral"
};

static char *oldshiptypes[] = {"SC", "DD", "CA", "BB", "AS", "SB"};

/* ------------------------------------------------------------------- */
char *maprank P((struct player *));
char *mapshiptype P((struct player *));
char *mapname P((char *));
char *typestat P((struct player *));

/* -------------------------------[ Main ]----------------------------- */
int
main(int argc, char **argv)
{
    int     mode;
    char    fh[33];
    int     teams[9];
    int     i, count;
    struct  player *j;
    int     usage = 0;
    char    *rf;

    argv0 = argv[0];

    if (argc > 1) {
        if (argv[1][0] != '-') {
            usage++;
        } else {
            switch(argv[1][1]) {
              case 'h':
                usage++;
                break;
              case 'd':
              case 'r':
              case 'l':
              case 'o':
              case 'M':
              case 'p':
                mode = argv[1][1];
                break;
              default:
                usage++;
                break;
            }
        }
    } else {
        mode = '\0';
    }

    if (usage) {
        char *message[255] = {
            "\n\t'%s [option]'\n",
            "\nOptions:\n",
            "\t-h   help (this usage description)\n",
            "\t-d   Damage Status.\n",
            "\t-r   ?\n",
            "\t-l   Lag Statistics\n",
            "\t-o   Old style (NetrekI Ranks/ships)\n",
            "\t-M   Default (Metaserver)\n",
            "\t-p   PID\n\n",
            "\0"
        };

        fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
        for (i=0; *message[i] != '\0'; i++)
            fprintf(stderr, message[i], argv0);

        exit(1);
    }

    rf = build_path(RANKS_FILE);
    init_data(rf);

    openmem(0, 0);

    for (count = 0, i = 0, j = players; i < MAXPLAYER; i++, j++) {
        if (j->p_status == PFREE)
            continue;
        count++;
    }

    if (!count) {
        printf("No one is playing.\n");
        exit(0);
    }

    for (i = 0; i < 9; i++)
        teams[i] = 0;

    for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
        if (j->p_status == PFREE) {
            continue;
        }
        teams[j->p_team]++;
    }

    /* print the header #1 */
    printf("--==[ %d Player%s", count,
           (count == 1 ? " ]=====[" : (count > 9 ? "s ]===[" : "s ]====[")));
    printf(" Feds: %d ]=[ Roms: %d ]=[ Kli: %d ]=[ Ori: %d ]=====--\n",
       teams[FED], teams[ROM], teams[KLI], teams[ORI]);

    /* print the header #2 */
    switch (mode) {
      case 'd':
        printf("--==[ Name ]=========[ Type  Kills Damage Shields Armies   Fuel ]======--\n");
        break;
      case 'r':
        printf("--==[ Name ]=========[ Status Type  Genocides  MaxKills        DI ]====--\n");
        break;
      case 'l':
        printf("--==[ Name ]============[ Average   Stnd Dev   Loss ]==================--\n");
        break;
      case 'M':
      case 'o':
      case '\0':
        printf("--=======[ Rank ]====[ Name ]=========[ Address ]======================--\n");
        break;
      case 'p':
        printf("--========[ PID ]===[ Name ]==========[ Address ]======================--\n");
        break;
    }

    for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
        if (j->p_status == PFREE) {
            continue;
        }
        switch (mode) {
          case 'd':
            printf("  %2s: %-16s   %2s %6.2f %6d %7d %6d %6d\n",
                   twoletters(j), j->p_name,
                   typestat(j),
                   j->p_kills,
                   j->p_damage,
                   j->p_shield,
                   j->p_armies,
                   j->p_fuel);
            break;
          case 'r':
            printf("  %2s: %-16s %s/%-4d  %2s  %7d  %9.1f  %10.2f\n",
                   twoletters(j),
                   j->p_name,
                   statnames[j->p_status],
                   j->p_ghostbuster,
                   typestat(j),
                   j->p_stats.st_genocides,
                   j->p_stats.st_tmaxkills,
                   j->p_stats.st_di);
            break;
          case 'l':
            printf("  %2s: %-16s %7d ms %7d ms %5d%%\n",
                   twoletters(j),
                   j->p_name,
                   j->p_avrt,
                   j->p_stdv,
                   j->p_pkls);
            break;
          case 'o':
            if (i > 19)
              continue;    /* show only first 20 */

            strncpy(fh, j->p_full_hostname, 32);
            fh[32] = 0;
            printf("  %2s:  %2s  %-9.9s   %-16s %s@%s\n",
               twoletters(j), mapshiptype(j), maprank(j),
               mapname(j->p_name), j->p_login, fh);
            break;
          case 'p':
            strncpy(fh, j->p_full_hostname, 32);
            fh[32] = 0;
            printf(" %c%2s:  %2s   %-9d %-16s  %s@%s\n",
                   (j->p_stats.st_flags & ST_CYBORG) ? '*' : ' ',
                   twoletters(j), typestat(j),
                   j->p_ntspid,
                   j->p_name,
                   j->p_login,
                   fh);
              break;
          case 'M':
          case '\0':
              strncpy(fh, j->p_full_hostname, 32);
              fh[32] = 0;
              printf(" %c%2s:  %2s  %-11.11s %-16s %s@%s\n",
                     (j->p_stats.st_flags & ST_CYBORG) ? '*' : ' ',
                     twoletters(j), typestat(j),
                     j->p_stats.st_royal ? royal[j->p_stats.st_royal].name
                     : ranks[j->p_stats.st_rank].name,
                     j->p_name,
                     j->p_login,
                     fh);
              break;
        }
    }

    printf("--==[ NetrekII (Paradise), %s ]==--\n", PARAVERS);

    exit(0);
}

/* ---------------------------[ Functions ]--------------------------- */

/* map rank into one of the original netrek rank names for compatibility
   with the metaserver */
char *
maprank(struct player *p)
{
    int     r;

    r = (int) (10 * (p->p_stats.st_rank / (float) NUMRANKS));
    if (r < 0)
        r = 0;
    if (r > 8)
        r = 8;
    return oldranknames[r];
}

char *
mapshiptype(struct player *p)
{
    int     n;

    n = p->p_ship.s_alttype;
    if (n > 5)
        n = 5;

    return oldshiptypes[n];
}

char *
mapname(char *s)
{
    static char    newname[17];

    if (*s == ' ') {
        strncpy(newname, s, 16);
        newname[0] = '_';
        return newname;
    } else
        return s;
}

char *
typestat(struct player *p)
{
    static char    str[3];

    switch (p->p_status) {
      case PALIVE:
      case PEXPLODE:
        str[0] = p->p_ship.s_desig1;
        str[1] = p->p_ship.s_desig2;
        str[3] = 0;
        return str;
      case PTQUEUE:
        return "tq";
      case POBSERVE:
        return "ob";
      case POUTFIT:
        return "of";
      case PDEAD:
        default:
        return "--";
    }
}
