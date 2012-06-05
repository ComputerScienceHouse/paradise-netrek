/*
 * main.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#define PLAYER_EDITOR	/* this gets us a non-status2 version of PLAYERFILE */
#include "config.h"
#include "common.h"
#include "main.h"
#include "db.h"
#include "interface.h"
#include "file.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

static char *myname;
char *playerFile;

int
main(int argc, char *argv[])
{
	char *rf;

	myname = argv[0];

	if(argc > 2) {
		fprintf(stderr, "Usage:  %s [playerfile]\n", myname);
		exit(1);
	}

	signal(SIGWINCH, getTTYinfo);

	rf = build_path(RANKS_FILE);
	init_data(rf);

	if(argc == 2)
		playerFile = argv[1];
	else
		playerFile = build_path(PLAYERFILE);

	getTTYinfo(0);
	if(ReadIt(playerFile)) exit(1);
	Interface();
	exit(0);
}

void
err(char *s, ...)
{
	va_list ap;
	char txt[60];

	va_start(ap, s);
	vsprintf(txt, s, ap);
	fprintf(stderr, "%s: %s\n", myname, txt);
	va_end(ap);
}

void
err_sys(char *s, ...)
{
	va_list ap;
	char txt[60];

	va_start(ap, s);
	vsprintf(txt, s, ap);
	fprintf(stderr, "%s: %s: %s\n", myname, txt, strerror(errno));
	va_end(ap);
}

void
GoAway(int type)
{
	if(type)
		if(!Verify("quit?  There are unsaved changes."))
			return;

	printf("\npped version %s by H. Kehoe\n\n", VERSSTR);
	exit(0);
}

