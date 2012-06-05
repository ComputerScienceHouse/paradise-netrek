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

void
usage(char *name)
{
    char *message[255] = {
        "\nSend messages (accepted from stdin).\n",
        "\n\t'%s <recipient> [options]'\n\n",
        "Recipients can be:\n",
        "\ta number from 0-9 or a letter from a-j to send to one player\n",
        "\ta team letter [FRKO] to send to that team\n",
        "\tthe letter 'A' to send to ALL.\n",
        "\nOptions:\n",
        "\t-w          without the 'who' part of the message ('GOD->ALL')\n",
        "\t-n string   from somebody other than 'GOD'\n\n",
        ""
    };
    int x;

    fprintf(stderr, "-- Netrek II (Paradise), %s --\n", PARAVERS);
    for (x=0; *message[x] != '\0'; x++)
        fprintf(stderr, message[x], name);

    exit(1);
}

int
main(int argc, char **argv)
{
    char    ch;
    char    to[80];
    char    from[80];
    char    buf2[80];
    int     target;
    int     flag;
    char    name[32];

    strcpy(from, "GOD");
    strcpy(to, "ALL");

    strcpy(name, argv[0]);

    if (argc == 1)
	usage(name);

    if ((target = letter_to_pnum(argv[1][0])) >= 0) {	/*--[ personal ]--*/
	flag = MINDIV;
/*      r = &players[target];
      printf("debug: 1");
      to[0] = team_to_letter(r->p_team);
      to[1] = 0; */
	strcpy(to, argv[1]);
    }
    else
	switch (ch = argv[1][0]) {	/*--[ better be a team ]--*/
	case 'A':
	    target = 0;
	    flag = MALL;
	    break;
	case 'F':
	    target = FED;
	    flag = MTEAM;
	    strcpy(to, "FED");
	    break;
	case 'R':
	    target = ROM;
	    flag = MTEAM;
	    strcpy(to, "ROM");
	    break;
	case 'K':
	    target = KLI;
	    flag = MTEAM;
	    strcpy(to, "KLI");
	    break;
	case 'O':
	    target = ORI;
	    flag = MTEAM;
	    strcpy(to, "ORI");
	    break;
	case 'S':
	    printf("+> This options is not compatable yet");
	    exit(1);
	    flag=0;
	    break;
	default:
	    usage(name);
	    flag=0;
	    break;
	}

    /*
       =========================[ check for options
       ]=========================
    */

    if (argc >= 3) {
	if (strcmp(argv[2], "-w") == 0) {	/*-----[ without to/from ]-----*/
	    from[0] = 0;
	}
	else if (strcmp(argv[2], "-n") == 0) {	/*--[ different from name ]---*/
	    if (argc <= 2)	/*--[ other than GOD ]--------*/
		usage(name);
	    if (argc >= 3) {
		strcpy(from, argv[3]);
	    }
	    else {
		usage(name);
	    }
	}
	else if (strcmp(argv[1], "-u") == 0 || strcmp(argv[1], "-h") == 0) {
	    usage(name);
	}
    }

    /*
       ===[ merge the to/from strings, make sure they are the same length
       ]===
    */

    if (from[0] == 0 || to[0] == 0) {
	strcpy(buf2, "");
    }
    else {
	if (strlen(from) <= 2) {
	    strcpy(buf2, " ");
	}
	strcat(buf2, from);
	strcat(buf2, "->");
	strcat(buf2, to);

	while (strlen(buf2) < 9) {	/*---[ if it's too short, extend it ]---*/
	    strcat(buf2, " ");
	}
	strcat(buf2, " ");	/*---[ throw in an extra space ]---*/
    }
    /*
       =========================[ send it on its way
       ]========================
    */

    openmem(0, 0);

    while (1) {
	char    buf[80];
	int     len;

    printf(buf2);
	if (0 == fgets(buf, sizeof(buf), stdin))
	    break;
	len = strlen(buf);
	if (buf[len - 1] == '\n')
	    buf[--len] = 0;
	pmessage(buf2, target, flag, buf);
    }

   exit(0);
}
