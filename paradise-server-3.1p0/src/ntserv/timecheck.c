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
#include "ntserv.h"

/*--------------------------------DESCRIPTION-------------------------------
  This module contains the code to see if players are allowed to play
depending on the time of day.  This is done with a matrix that contains
an element for each hour in each day of the week.  If the element is zero,
access is not allow.  If it is not, then access is allowed.  The time_access
function is called to check the time and look up the time in the access
matrix.
  Before the time_access function is called, the load_time_access function
needs to be called.  This function loads the access matrix from a file.
The format of the file is:
udfgaergfhggdfhg		--These are three garbage lines
sdjkhgfsadhghsdghdgh		--They are ignored
fjishgfsdhgdfhjghdahgdtu
SUN 0 0 0 0 1 1 1 ...		--name of day followed by 24 0's or 1's
MON 0 1 1 1 1 ...		-- 0 = closed  1 = open
  The time_access function checks the time access matrix to see if access
should be allowed.  The function makes a provision for overriding the
access matrix.  If the file OVERRIDE exists, then access is automaticly
allowed.  If the file DENY exists then access is disallowed.  If they both
exists then access is allowed.
---------------------------------------------------------------------------*/







/*-------------------------------NUMBER DEFINES----------------------------*/
#define HOURS	 "etc/conf.hours"
#define OVERRIDE "etc/ALLOW"
#define DENY     "etc/DENY"
/*------------------------------------------------------------------------*/







/*-------------------------------MODULE VARIABLES--------------------------*/

 /*
    The access matrix that is looked up to see if the server is open. 0 =
    closed  1 = open
 */
static int timematrix[7][24];

/*-------------------------------------------------------------------------*/






/*------------------------------VISIBLE FUNCTIONS--------------------------*/

/*---------------------------------LOAD_TIME_ACCESS------------------------*/
/*  This function loads the time access matrix from the time access file.
If the file cannot be opened, a warning is printed out and the access matrix
is filled with 1's, allowing access at all times.  */


void 
load_time_access(void)
{
    FILE   *f;			/* to open time access file with */
    char   *filename;		/* to hold full path plus filename */
    int     i, j;		/* looping vars */

    filename = build_path(HOURS);

    if ((f = fopen(filename, "r")) != NULL) {	/* file opened correctly? */
	fgets(filename, 256, f);/* get rid of first three lines */
	fgets(filename, 256, f);
	fgets(filename, 256, f);
	for (i = 0; i < 7; i++) {	/* go through each day */
	    fscanf(f, "%s", filename);	/* get day name */
	    for (j = 0; j < 24; j++)	/* go through each hour */
		fscanf(f, "%d", &(timematrix[i][j]));	/* get access for that
							   hour */
	}
	fclose(f);		/* clsoe time access file */
    }
    else {			/* else no timecheck file */
	fprintf(stderr, "Warning, no .hours file found.");
	fprintf(stderr, "  Allowing access at all hours, all days.\n");
	for (i = 0; i < 7; i++)	/* go through all days */
	    for (j = 0; j < 24; j++)	/* go through all hours */
		timematrix[i][j] = 1;	/* set access okay at this day,hour */
    }
}




/*---------------------------------TIME_ACCESS-----------------------------*/
/*  This function checks to see if the server is open.  It returns a 1 if the
server is open at the current time of day and day of the week.  It returns
a 0 otherwise.  */

int 
time_access(void)
{
    struct tm *tm;		/* points to structure containing local time */
    time_t  secs;		/* to get number of seconds */
    FILE   *fd;			/* to open override file with */
    char   *filename;		/* to hold full path and filename */

    filename = build_path(OVERRIDE);
    if ((fd = fopen(filename, "r")) != NULL) {	/* if override open file */
	fclose(fd);		/* exists then */
	return 1;		/* return server is open */
    }
    filename = build_path(DENY);
    if ((fd = fopen(filename, "r")) != NULL) {	/* if override close file */
	fclose(fd);		/* exists then */
	return 0;		/* return server is closed */
    }
    time(&secs);		/* get calendar time */
    tm = localtime(&secs);	/* convert it to local time */
    return (timematrix[tm->tm_wday][tm->tm_hour]);	/* check time access
							   matrix */
}

/*------------------------------------------------------------------------*/





/*--------END OF FILE----------*/
