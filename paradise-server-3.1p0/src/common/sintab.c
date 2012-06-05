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
#include <stdlib.h>
#include <math.h>
#include "proto.h"

/*---------------------------------DATA------------------------------------*/
/*  The sine table so we do not have to calculate this on the fly.  */
/* This file formerly contained hard-coded sin,cos values.  This has */
/* been replaced with a function that initializes the tables once via  */
/* real math functions instead and avoids duplicating data between Cos */
/* and Sin */

double *Sin = NULL;
double *Cos = NULL;

void 
init_trig(void)
{
  if(!Sin)
  {
    int i;

    Sin = (double *)malloc(sizeof(double) * 320);
    for(i = -64; i < 256; i++)
    {
      double rads, degrees;

      degrees = 360.0 * (double)(i) / 256.0;
      rads = M_PI * degrees / 180.0;
      Sin[i+64] = sin(rads);
    }
    Cos = Sin + 64;
  }
}

/*-------END OF FILE--------*/
