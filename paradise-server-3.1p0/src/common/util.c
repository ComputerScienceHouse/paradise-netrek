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

#include <math.h>
#include "config.h"
#include "proto.h"
#include "data.h"
#include "shmem.h"

struct player *me;
struct ship *myship;
struct stats *mystats;

/*----------------------------VISIBLE FUNCTIONS---------------------------*/

unsigned char
getcourse(int x, int y, int xme, int yme)
{
    return(
           (unsigned char) (int) 
             (atan2((double) (x - xme), (double) (yme - y)) / M_PI * 128.0)
          );
}

/*-------------------------------ANGDIST----------------------------------*/
/*  This function provides the proper angular distance between two angles.
The angles are expressed as numbers from 0-255. */

int 
angdist(unsigned char x, unsigned char y)
{
    register unsigned char res;	/* temp var */

    res = x - y;		/* get abs value of difference */
    if (res > 128)		/* if more than 180 degrees */
	return (256 - (int) res);	/* then choose to go other way around */
    return ((int) res);		/* else its just the difference */
}

/*-------------------------------------------------------------------------*/


/* this function checks to see if an occurrence is temporally spaced
   from the previous one.  This is useful in preventing the client
   from firing torps and missiles too quickly and to limit detting to a
   reasonable frequency (detting too fast burns fuel and increases
   wtemp without any benefit).
   */

int 
temporally_spaced(struct timeval *lasttime, int gap)
{
    struct timeval curtp;

    gettimeofday(&curtp, (struct timezone *) 0);
    if ((curtp.tv_sec == lasttime->tv_sec &&
	 curtp.tv_usec < lasttime->tv_usec + gap)
	|| (curtp.tv_sec == lasttime->tv_sec + 1 &&
	    curtp.tv_usec + 1000000 < lasttime->tv_usec + gap))
	return 0;

    lasttime->tv_sec = curtp.tv_sec;
    lasttime->tv_usec = curtp.tv_usec;
    return 1;
}

/*
 *
 */

int 
check_fire_warp(void)
{
    if (configvals->fireduringwarp || !(me->p_flags & PFWARP))
	return 1;

    warning("Can not fire while in warp.");

    return 0;
}

int 
check_fire_warpprep(void)
{
    if (configvals->fireduringwarpprep || !me->p_warptime)
	return 1;

    warning("Can not fire while preparing for warp.");

    return 0;
}

int
check_fire_docked(void)
{
    if (configvals->firewhiledocked || !(me->p_flags & PFDOCK))
	return 1;

    warning("It is unsafe to use weapons while docked.");

    return 0;
}


/*-------END OF FILE--------*/
