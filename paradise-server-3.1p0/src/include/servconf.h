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

#ifndef SERVCONF_H
#define SERVCONF_H

/*
// This file is used to define various different options in the server,
// which are not included in the sysdefs (.sysdef/sysdefaults.c), for
// optimization purposes.
//
// Simply adjust the comments for options you wish/dont wish to use.
// 
// Most of the #defines in this file have either
//   1) been eliminated, either making code default or eliminating code, or
//   2) been re-implemented as sysdefaults.
*/

/*
// Cluechecking.
//
// Cluechecker #1 is Robs
//  ** supports clue checking with words from the MOTD with MOTD_SUPPORT
// Cluechecker #2 is Brandon's (hacked from Robs)
*/

#undef CLUECHECK1
#undef MOTD_SUPPORT
#undef CLUECHECK2

#endif /* SERVCONF_H */
