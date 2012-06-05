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

#ifndef WTEXT_H
#define WTEXT_H

/*	Here are all warnings that are send with SP_S_WARNING */
/*		HW 93		*/
/* ab handleTractorReq  socket.c */

#define TEXTE 0
#define PHASER_HIT_TEXT 1
#define BOMB_INEFFECTIVE 2
#define BOMB_TEXT 3
#define BEAMUP_TEXT 4
#define BEAMUP2_TEXT 5
#define BEAMUPSTARBASE_TEXT 6
#define BEAMDOWNSTARBASE_TEXT 7
#define BEAMDOWNPLANET_TEXT 8
#define SBREPORT 9
#define ONEARG_TEXT 10
#define BEAM_D_PLANET_TEXT 11
#define ARGUMENTS 12
#define BEAM_U_TEXT 13
#define LOCKPLANET_TEXT 14
#define LOCKPLAYER_TEXT 15
#define SBRANK_TEXT 16
#define SBDOCKREFUSE_TEXT 17
#define SBDOCKDENIED_TEXT 18
#define SBLOCKSTRANGER 19
#define SBLOCKMYTEAM 20
/*	Daemon messages */
#define DMKILL 21
#define KILLARGS 22
#define DMKILLP 23
#define DMBOMB 24
#define DMDEST 25
#define DMTAKE 26
#define DGHOSTKILL 27
/*	INL	messages		*/
#define INLDMKILLP 28
#define INLDMKILL 29		/* Because of shiptypes */
#define INLDRESUME 30
#define INLDTEXTE 31
/* Variable warning stuff */
#define STEXTE 32		/* static text that the server needs to send
				   to the client first */
#define SHORT_WARNING 33	/* like CP_S_MESSAGE */
#define STEXTE_STRING 34
#define KILLARGS2  35

#define DINVALID 255

#endif /* WTEXT_H */
