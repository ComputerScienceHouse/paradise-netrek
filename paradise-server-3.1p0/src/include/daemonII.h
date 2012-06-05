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
#include "defs.h"
#include "struct.h"

#ifndef DAEMONII_H
#define DAEMONII_H

/*-------------------------------NUMBER DEFINES-----------------------------*/
#define TOURNEXTENSION 15	/* Tmode gone for 15 seconds 8/26/91 TC */

#define PLAYERFUSE	1
#define TORPFUSE	1
#define MISSILEFUSE	1
#define PLASMAFUSE      1
#define PHASERFUSE	1
#define CLOAKFUSE       2
#define TEAMFUSE	5
#define PLFIGHTFUSE	5
#define TERRAINFUSE     1
#define BEAMFUSE	8		/* scott 8/25/90 -- was 10 */
#define PLANETFUSE	SECONDS(15)	/* every 15 seconds */
#define MINUTEFUSE      MINUTES(1)	/* 1 minute, surrender funct etc.
					   4/15/92 TC */
#define SYNCFUSE	MINUTES(5)
#define CHECKLOADFUSE   MINUTES(15)	/* change 1/26/91 -- was 600 */
#define HOSEFUSE	MINUTES(20)	/* 20 min., was 15 minutes 6/29/92 TC */
#define HOSEFUSE2	MINUTES(5)	/* 5 min., was  3 minutes 6/29/92 TC */

#define GHOSTTIME	SECONDS(30)	/* 30 secs */
#define OUTFITTIME	SECONDS(6 * AUTOQUIT)	/* 6 * AQ secs */

#define HUNTERKILLER	(-1)
#define TERMINATOR  	(-2)		/* Terminator */
#define STERMINATOR	(-3)		/* sticky Terminator */
/*--------------------------------------------------------------------------*/

/*---------------------------------MACROS-----------------------------------*/

#define FUSE(X) ((ticks % (X)) == 0)

#define NotTmode(X) (!(status->tourn) && ((X - tourntimestamp)/10 > TOURNEXTENSION))

/*--------------------------------------------------------------------------*/

/*------------------------------PROTOTYPES----------------------------------*/

/* daemon/conquer.c */
void udsurrend P((void));
void endgame_effects P((int, int, int));
void checkwin P((int));

/* daemon/daemonII.c */
void starttimer P((void));
void stoptimer P((void));
void ghostmess P((struct player *));
void saveplayer P((struct player *));
void rescue P((int, int, int));

/* daemon/dutil.c */
void killmess P((struct player *, struct player *));
void cause_kaboom P((struct player *));
int get_explode_views P((short));
int inflict_damage P((struct player *, struct player *, struct player *,
                      int, int));
int enemy_admiral P((int));

/* daemon/misc.c */
void warmessage P((void));
void peacemessage P((void));
int realNumShips P((int));
int tournamentMode P((void));
void god2player P((char *, int));
void parse_godmessages P((void));

/* daemon/planets.c */
void initplanets P((void));
void growplanets P((void));
void check_revolt P((void));
void gen_planets P((void));
void moveplanets P((void));
void popplanets P((void));
void plfight P((void));
void save_planets P((void));

/* daemon/player.c */
void loserstats P((int));
void killerstats P((int, struct player *));
void checkmaxkills P((int));
void beam P((void));
void udcloak P((void));
void udplayers P((void));

/* daemon/galaxygen/galaxygen*.c */
int place_stars P((struct planet *, int, int, int, int, struct planet *, int));
void zero_plflags P((struct planet *, int));
void randomize_atmospheres P((struct planet *, int, int, int, int, int));
void randomize_resources P((struct planet *, int, int, int, int));
void justify_galaxy P((int));

#ifndef LOADABLE_PLGEN
void gen_galaxy_1 P((void));
void gen_galaxy_2 P((void));
void gen_galaxy_3 P((void));
void gen_galaxy_4 P((void));
void gen_galaxy_5 P((void));
void gen_galaxy_6 P((void));
void gen_galaxy_7 P((void));
void gen_galaxy_8 P((void));
void gen_galaxy_9 P((void));
#endif
/* daemon/shipvals.c */
void getshipdefaults P((void));

/* daemon/stats.c */
void credit_armiesbombed P((struct player *, int, struct planet *));

/* daemon/sysdefaults.c */
void readsysdefaults P((void));
int update_sys_defaults P((void));

/* daemon/terrain.c */
void generate_terrain P((void));
void doTerrainEffects P((void));

/* daemon/tourny.c */
void tlog_plkill P((struct player *, struct player *, struct player *));
void tlog_plankill P((struct player *, struct planet *, struct player *));
void tlog_plandest P((struct planet *, struct player *));
void tlog_plantake P((struct planet *, struct player *));
void tlog_planaban P((struct planet *, struct player *));
void tlog_beamup P((struct planet *, struct player *));
void tlog_beamdown P((struct planet *, struct player *));
void tlog_Bbeamup P((struct player *, struct player *));
void tlog_Bbeamdown P((struct player *, struct player *));
void tlog_bomb P((struct planet *, struct player *, int));
void tlog_bres P((struct planet *, struct player *, char *));
void tlog_pop P((struct planet *, int));
void tlog_res P((struct planet *, char *));
void tlog_revolt P((struct planet *));
void scan_for_unexpected_tourny_events P((void));
void tlog_conquerline P((char *));
void udtourny P((void));
void starttourn P((void));
void endtourn P((void));

/* daemon/weapons.c */
void udphaser P((void));
void udtorps P((void));
void udmissiles P((void));
void udplasmatorps P((void));

/*--------------------------------------------------------------------------*/

/*----------END OF FILE--------*/

#endif /* DAEMONII_H */
