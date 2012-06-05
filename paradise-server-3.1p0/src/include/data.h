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

#ifndef DATA_H
#define DATA_H

#define SBEXPVIEWS 		7

extern char *argv0;

extern struct player *me;
extern struct ship *myship;
extern struct stats *mystats;

extern int oldalert;
extern int remap[];
extern int selfdest;
extern int lastm;
extern int delay;
extern int rdelay;
extern int mustexit;
extern int keeppeace;
extern char *shipnos;
extern int sock;
extern int xtrekPort;
extern int shipPick;
extern int tmpPick;
extern int teamPick;
extern int repCount;
extern char namePick[];
extern char passPick[];
extern int inputMask;
extern int nextSocket;
extern char *host;
extern int noressurect;
extern int userVersion, userUdpVersion;
extern int timerDelay;
extern char testdata[];
extern int RSA_Client;
extern char RSA_client_type[256];	/* LAB 4/1/93 */
extern int testtime;
extern char *galaxyValid;

extern int F_feature_packets; 	/* allow feature packets */
extern int F_allowViewBox;	/* allow view box */
extern int F_allowShowAllTractorPressor;	/* allow all tracts/presses */
extern int F_allowContinuousMouse;	/* allow continuous mouse */
extern int F_UseNewMacro;	/* allow new macros */
extern int F_UseSmartMacro;	/* Allow smart macros */
extern int F_multiline_enabled;	/* Allow multiline macros */
extern int F_why_dead;		/* Allow why_dead reporting */
extern int F_cloakerMaxWarp;	/* Allow cloaker to go maxwarp */
extern int F_gen_distress;	/* No RCDs (not yet implemented in server) */
extern int F_allow_beeplite;	/* No RCDs ==> no allowed beeplite */
extern unsigned char F_beeplite_flags;	/* flags for beeplite */
extern int F_terrain;		/* Client isn't capable of terrain by default */
extern unsigned char F_terrain_major;	/* Major terrain version client can handle */
extern unsigned char F_terrain_minor;	/* Minor terrain version client can handle */
extern int F_gz_motd;		/* Client can't handle GZipped MOTD packets */
extern unsigned char F_gz_motd_major;	/* Major gzipped format client can handle */
extern unsigned char F_gz_motd_minor;	/* Minor gzipped format client can handle */
extern int F_armies_shipcap;

extern int chaos;
extern int topgun;		/* added 12/9/90 TC */
extern int hourratio;
extern int blk_flag;		/* added 1/19/93 KAO */

extern int udpSock;		/* UDP */
extern int commMode;		/* UDP */
extern int blk_metaserver;

extern double oldmax;
extern double *Sin, *Cos;

#define VIEWS 16
extern char pseudo[PSEUDOSIZE];
extern char login[PSEUDOSIZE];

#ifdef STATIC_RANKS
extern struct rank ranks[NUMRANKS];
extern struct royalty royal[NUMROYALRANKS];
#else
extern struct rank *ranks;
extern struct royalty *royal;
extern int NUMRANKS;
extern int NUMROYALRANKS;
#endif

extern int ping;
extern long packets_sent;
extern long packets_received;
extern int ping_ghostbust;
#if 0
extern int ping_freq;
extern int ping_iloss_interval;
extern int ping_allow_ghostbust;
extern int ping_ghostbust_interval;
#endif


/* miscellaneous string constants that don't need to be scattered all
   over the place */

extern char *PARAVERS;
extern char MCONTROL[];
extern char UMPIRE[];
extern char MSERVA[];
extern char SERVNAME[];

#endif /* DATA_H */
