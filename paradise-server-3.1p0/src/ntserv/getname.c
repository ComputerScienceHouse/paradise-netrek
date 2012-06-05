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
#include <errno.h>
#include <sys/stat.h>
#include "config.h"
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif
#include "proto.h"
#include "ntserv.h"
#include "data.h"
#include "shmem.h"

/* replace non-printable characters in string with spaces */
static void 
remove_ctl(char *str)
{
    while (*str) {
	if (!isgraph(*str))	/* not a printable character */
	    *str = ' ';		/* replace it with space */
	str++;
    }
}

/* return true if we want a lockout */
static int 
lockout(void)
{
    return (
/*
	  (strncmp (login, "bozo", 4) == 0) ||
*/
	    0
    );
}

static void 
handleLogin(void)
{
    static struct statentry player;
    static int position = -1;
    int     plfd;
    int     entries;
    struct stat buf;
    char   *paths;
    char    pwbuf[16];

    paths = build_path(PLAYERFILE);
    *namePick = '\0';
    inputMask = CP_LOGIN;
    while (*namePick == '\0') {
	/* Been ghostbusted? */
	if (me->p_status == PFREE)
	    exitGame();
	if (isClientDead())
	    exitGame();
	socketPause();
	readFromClient();
    }
    /* Change those annoying control characters to spaces */
    remove_ctl(namePick);
    if ((strcmp(namePick, "Guest") == 0 || strcmp(namePick, "guest") == 0) &&
	!lockout()) {
	hourratio = 5;
	memset(&player.stats, 0, sizeof(struct stats));
	player.stats.st_tticks = 1;
	player.stats.st_flags = ST_INITIAL;
	/*
	   If this is a query on Guest, the client is screwed, but I'll send
	   him some crud anyway.
	*/
	if (passPick[15] != 0) {
	    sendClientLogin(&player.stats);
	    flushSockBuf();
	    return;
	}
	sendClientLogin(&player.stats);

	updateMOTD();		/* added here 1/19/93 KAO */

	flushSockBuf();
	strcpy(me->p_name, namePick);
	me->p_pos = -1;
	memcpy(&(me->p_stats), &player.stats, sizeof(struct stats));
	return;
    }
    hourratio = 1;
    /* We look for the guy in the stat file */
    if (strcmp(player.name, namePick) != 0) {
	for (;;) {		/* so I can use break; */
	    plfd = open(paths, O_RDONLY, 0644);
	    if (plfd < 0) {
		printf("I cannot open the player file!\n");
		strcpy(player.name, namePick);
		position = -1;
		printf("Error number: %d\n", errno);
		break;
	    }
	    position = 0;
	    while (read(plfd, (char *) &player, sizeof(struct statentry)) ==
		   sizeof(struct statentry)) {
		/*
		   this is a temporary thing to remove control chars from
		   existing entries in the player db
		*/
		remove_ctl(player.name);
		if (strcmp(namePick, player.name) == 0) {
		    close(plfd);
		    break;
		}
		position++;
	    }
	    if (strcmp(namePick, player.name) == 0)
		break;
	    close(plfd);
	    position = -1;
	    strcpy(player.name, namePick);
	    break;
	}
    }
    /* Was this just a query? */
    if (passPick[15] != 0) {
	if (position == -1) {
	    sendClientLogin(NULL);
	}
	else {
	    sendClientLogin(&player.stats);
	}
	flushSockBuf();
	return;
    }
    /* A new guy? */
    if ((position == -1) && !lockout()) {
	strcpy(player.name, namePick);
        memset(player.password, 0, 16);
        strncpy(player.password, crypt(passPick, namePick), 15);
	memset(&player.stats, 0, sizeof(struct stats));
	player.stats.st_tticks = 1;
	player.stats.st_flags = ST_INITIAL;
	player.stats.st_royal = 0;
	plfd = open(paths, O_RDWR | O_CREAT, 0644);
	if (plfd < 0) {
	    sendClientLogin(NULL);
	}
	else {
	    fstat(plfd, &buf);
	    entries = buf.st_size / sizeof(struct statentry);
	    lseek(plfd, entries * sizeof(struct statentry), 0);
	    write(plfd, (char *) &player, sizeof(struct statentry));
	    close(plfd);
	    me->p_pos = entries;
	    memcpy(&(me->p_stats), &player.stats, sizeof(struct stats));
	    strcpy(me->p_name, namePick);
	    sendClientLogin(&player.stats);
	    updateMOTD();	/* added here 1/19/93 KAO */
	}
	flushSockBuf();
	return;
    }

    /* An actual login attempt */
    memset(pwbuf, 0, 16);
    strncpy(pwbuf, crypt(passPick, player.password), 15);
    if ((strcmp(player.password, pwbuf) != 0) || lockout()) {
	sendClientLogin(NULL);
    } else {
	strcpy(me->p_name, namePick);
	me->p_pos = position;
	memcpy(&(me->p_stats), &player.stats, sizeof(struct stats));

	sendClientLogin(&player.stats);
	
	updateMOTD();		/* st_flags&ST_NOBITMAPS */
    }
    flushSockBuf();

    /* Try to make the first person in the player database an Emperor */
    if (position==0)
      me->p_stats.st_royal = NUMROYALRANKS-1;

    return;
}

void 
getname(void)
/* Let person identify themselves from w */
{
    *(me->p_name) = 0;
    while (*(me->p_name) == 0) {
	handleLogin();
    }
}

void 
savestats(void)
{
    int     fd;
    char   *paths;

    if (me->p_pos < 0)
	return;

    paths = build_path(PLAYERFILE);

    fd = open(paths, O_WRONLY, 0644);
    if (fd >= 0) {
	me->p_stats.st_lastlogin = time(NULL);
	lseek(fd, 32 + me->p_pos * sizeof(struct statentry), 0);
	write(fd, (char *) &me->p_stats, sizeof(struct stats));
	close(fd);
    }
}
