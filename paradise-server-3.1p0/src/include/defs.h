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

#ifndef DEFS_H
#define DEFS_H

#include "config.h"
#include <limits.h>

#ifdef SVR4			/* to get it to work under Solaris [BDyess] */
#define sigsetmask sigset
#endif				/* SVR4 */

#define MAXPLAYER 32

#define MAXPLANETS 60		/* A new variable called NUMPLANETS takes the
				   place of this #define in many parts of the
				   code -- this one just sets the initial
				   array sizes, so it is, literally, a
				   maximum */

#define MAX_GWIDTH 200000       /* GWIDTH is used in most of the code --
                                   this is a max value. */
#define TGRID_GRANULARITY 800   /* The width and height, in the same units as 
                                   GWIDTH, of one terrain_grid unit. */
#define NTERRAIN 128		/* number of bytes of zipped terrain data to send */
				/* at once */
#define LOG2NTERRAIN 7		/* well, you should be able to figure this out */
#define TERRAIN_MASK (NTERRAIN-1) /* NTERRAIN should always be a power of 2, otherwise */
				/* updateTerrain() in socket.c will break */
/* NOTE!  NTERRAIN can *NEVER* be greater than 128 due to the way the structure
   and socket code work.  This may eventually be a bummer when higher bandwidths
   are around, but for now with me & my linux box, this is just fine.  Besides,
   preliminary tests show that the gzipped terrain grid to be usually less than
   350 bytes anyway. */

#define TGRID_SIZE (MAX_GWIDTH/TGRID_GRANULARITY)

#define MAXTORP 8

/* Parnes's (Raynfala's) Butt Torp Code */
/* HALFARC = (0-128), FACTOR = (0-16) */

#define TORP_PENALTY_HALFARC myship->s_torp.wtemp_halfarc
#define TORP_PENALTY_FACTOR myship->s_torp.wtemp_factor

#define NPTHINGIES 8
#define NGTHINGIES 40
#define TOTALTHINGIES (MAXPLAYER*NPTHINGIES + NGTHINGIES)
#define MAXPLASMA 1
#define WINSIDE 500		/* Size of strategic and tactical windows */
#define BORDER 4		/* border width for option windows */
#define PSEUDOSIZE 16
#define CLOAK_PHASES 7		/* number of drawing phases in a cloak
				   engage/disengage */
#ifdef STATIC_RANKS
#define NUMRANKS	18
#ifdef CASSIUS_ROYALTY
#define NUMROYALRANKS	10
#else
#define NUMROYALRANKS	5
#endif
#endif

#define GODLIKE		(NUMROYALRANKS-2)

/* These are configuration definitions */
/* GWIDTH was once a #define, now it's a variable in the configvals */

#define WARP1 20		/* warp one will move 20 spaces per update */
#define SCALE 40		/* Window will be one pixel for 20 spaces */
#define SHIPDAMDIST 3000	/* At this range, an exploding ship does damage */
#define DETDIST 1800    /* At this range a torp can be detonated */ 
                        /* was 1600. 6/29/92 TC */	
                        /* was 1800 8/1/92 TC */
#define NEWDETDIST 1800

#define PHASEDIST 6000		/* At this range a player can do damage with
				   phasers -- outdated */
#define ENTORBDIST 900		/* At this range a player can orbit a planet */
#define ORBDIST 800		/* A player will orbit at this radius */
#define FORBDIST 7500           /* The radius at which fighters patrol */
#define ORBSPEED 2		/* This is the fastest a person can go into
				   orbit */
#define PFIREDIST 1500		/* At this range a planet will shoot at a
				   player */

#define MIN_AST_DAMAGE    10    /* the minimum damage caused by an asteroid */
#define VAR_AST_DAMAGE    5    /* the max additional damage caused by an 
                                   asteroid per speed factor */
#define MIN_AST_HIT 2 /* the minimum chance of hitting an asteroid */

#define TORP_HIT_AST      8
#define PLASMA_HIT_AST    12     /* percent for asteroid collisions. */
#define FIGHTER_HIT_AST   3
#define MISSILE_HIT_AST   5

#define VACANT -1		/* indicates vacant port on a starbase */
#define PDAMAGE -2		/* indicates damaged port on a starbase */
#define DOCKDIST 600

#define TRACTDIST   6000	/* maximum effective tractor beam range */

#define TICKSPERSEC 10		/* clock ticks per second */
#define UPDATE (1000000/TICKSPERSEC)	/* Update time is 100000
					   micro-seconds */
#define AUTOQUIT 60		/* auto logout in 60 secs */
#define SECONDS(s)	(TICKSPERSEC*s)
#define MINUTES(s)	(60*SECONDS(s))

/* These are memory sections */
#define PLAYER 1
#define MAXMESSAGE 50
#define MAXREVIEWMESSAGE 20

#define rosette(x)   ((((x) + 8) / 16) & 15)
/* #define rosette(x)   ((((x) + 256/VIEWS/2) / (256/VIEWS) + VIEWS) % VIEWS) */
/*                      (((x + 8) / 16 + 16)  %  16)  */

/* These are the teams */
/* Note that I used bit types for these mostly for messages and
   war status.  This was probably a mistake.  It meant that Ed
   had to add the 'remap' area to map these (which are used throughout
   the code as the proper team variable) into a nice four team deep
   array for his color stuff.  Oh well.
*/
#define NOBODY 0x0
#define FED 0x1
#define ROM 0x2
#define KLI 0x4
#define ORI 0x8
#define ALLTEAM (FED|ROM|KLI|ORI)
#define MAXTEAM (ORI)		/* was ALLTEAM (overkill?) 6/22/92 TMC */
#define NUMTEAM 4
/*
** These are random configuration variables
*/
#define VICTORY 3		/* Number of systems needed to conquer the
				   galaxy */
#define WARNTIME 30		/* Number of updates to have a warning on the
				   screen */
#define MESSTIME 30		/* Number of updates to have a message on the
				   screen */

#define BUILD_SB_TIME   30	/* Minutes to rebuild an SB */
#define BUILD_JS_TIME   15	/* Minutes to rebuild an JS 1-24 bjg */
#define BUILD_WA_TIME   45	/* Minutes to rebuild an WA 1-24 bjg */

#define TARG_PLAYER	0x1	/* Flags for gettarget */
#define TARG_PLANET	0x2
#define TARG_CLOAK	0x4	/* Include cloaked ships in search */
#define TARG_SELF	0x8

/* Data files to make the game play across daemon restarts. */

#ifdef PLAYER_EDITOR
#define PLAYERFILE      "etc/db.players"
#else
#define PLAYERFILE	(status2->league ? "/tmp/tourney.players":"etc/db.players")
#endif
#define GLOBAL		"etc/db.global"
#define PLFILE		"etc/planets"
#define MOTD		"etc/motd"
#define WCMOTD		"etc/motd.wc"	  /* wrong client message */
#define CLOSEDMOTD	"etc/motd.closed" /* if doesn't exist, MOTD is used. */
#define PICS		"etc/conf.pics"
#define RANKS_FILE	"etc/ranks.conf"
#define RSA_EXEMPTION_FILE "etc/rsa-exemption"
#define NTSERV          "bin/ntserv"
#define DAEMON		"bin/daemonII"
#define ROBOT		"bin/robotII"
#define SNAKE		"bin/snake"
#define LOGFILENAME     "logs/server.log"
#define CONQFILE	"logs/conquer"
#define SYSDEF_FILE     "etc/conf.sysdef"
#define RSA_KEY_FILE	"etc/rsa.keys"
#define GODLOGFILE 	"logs/god.log"
#define CLUEPHRASEFILE	"etc/cluephrases"
#define MAILCLUECHECK	"bin/mailcluecheck"

#ifdef LOADABLE_PLGEN
#define PLGEN_PATH      "lib/galaxygen-"
#endif

/* If this isn't defined, the "Count: n players" messages will go into
   logfile, as before.  Otherwise, they'll go into this file: */
/* #define COUNTFILENAME	"logs/countfile" */

/* Listen stuff */
#define METASERVER "metaserver.netrek.org"
#define PORT 2592 /* port to listen on */

/* Other stuff that Ed added */

#define ABS(a)			/* abs(a) */ (((a) < 0) ? -(a) : (a))
#ifndef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#endif

#define myPlasmaTorp(t)		(me->p_no == (t)->pt_owner)
#define myTorp(t)		(me->p_no == (t)->t_owner)
#define myMissile(t)		(me->p_no == (t)->ms_owner)
#define friendlyPlasmaTorp(t)	((!(me->p_team & (t)->pt_war)) || (myPlasmaTorp(t)))
#define friendlyTorp(t)		(((!(me->p_team & (t)->t_war)) && \
				    (!(t->t_team & (me->p_swar | me->p_hostile)))) || \
				    (myTorp(t)))
#define friendlyMissile(t)		(((!(me->p_team & (t)->ms_war)) && \
				    (!(t->ms_team & (me->p_swar | me->p_hostile)))) || \
				    (myMissile(t)))
#define myPhaser(p)		(&phasers[me->p_no] == (p))
#define friendlyPhaser(p)	(me->p_team == players[(p) - phasers].p_team)
#define myPlayer(p)		(me == (p))
#define myPlanet(p)		(me->p_team == (p)->pl_owner)
#define friendly(fred,bart) \
                       (!(fred->p_team & (bart->p_swar|bart->p_hostile)) && \
                        !(bart->p_team & (fred->p_swar|fred->p_hostile)))
#define friendlyPlayer(p)       friendly(me, (p))
#if 0
#define friendlyPlayer(p)	((!(me->p_team & \
				    ((p)->p_swar | (p)->p_hostile))) && \
				    (!((p)->p_team & \
				    (me->p_swar | me->p_hostile))))
#endif
#define isAlive(p)		((p)->p_status == PALIVE)
#define friendlyPlanet(p)	((p)->pl_info & me->p_team && \
			     !((p)->pl_owner & (me->p_swar | me->p_hostile)))
#define CAN_BOMB(pl,t) (pl->p_ship.s_bombflags & SBOMB_##t)

#if 0
#define torpColor(t)		\
	(myTorp(t) ? myColor : shipCol[remap[players[(t)->t_owner].p_team]])
#define plasmatorpColor(t)		\
	(myPlasmaTorp(t) ? myColor : shipCol[remap[players[(t)->pt_owner].p_team]])
#define phaserColor(p)		\
	(myPhaser(p) ? myColor : shipCol[remap[players[(p) - phasers].p_team]])
/*
 * Cloaking phase (and not the cloaking flag) is the factor in determining
 * the color of the ship.  Color 0 is white (same as 'myColor' used to be).
 */
#define playerColor(p)		\
	(myPlayer(p) ? 		\
	    (cloak_pixels[0][me->p_cloakphase])	\
	    : (cloak_pixels[remap[(p)->p_team]][(p)->p_cloakphase]))
#define planetColor(p)		\
	(((p)->pl_info & me->p_team) ? shipCol[remap[(p)->pl_owner]] : unColor)

#define planetFont(p)		\
	(myPlanet(p) ? bfont : friendlyPlanet(p) ? ifont : dfont)
#define shipFont(p)		\
	(myPlayer(p) ? bfont : friendlyPlayer(p) ? ifont : dfont)
#endif

/* This macro allows us to time things based upon the SIGALRM signal.
 * Given a number of 1/5 seconds, it will return the number of SIGALRMs we
 *  will receive in that period.
 */
#define efticks(x) ((x)*200000/timerDelay)

/*
 * UDP control stuff
 */
#ifdef GATEWAY
#define UDP_NUMOPTS	11
#define UDP_GW		UDP_NUMOPTS-1
#else
#define UDP_NUMOPTS	10
#endif
#define UDP_CURRENT	0
#define UDP_STATUS	1
#define UDP_DROPPED	2
#define UDP_SEQUENCE	3
#define UDP_SEND	4
#define UDP_RECV	5
#define UDP_DEBUG	6
#define UDP_FORCE_RESET	7
#define UDP_UPDATE_ALL	8
#define UDP_DONE	9
#define COMM_TCP	0
#define COMM_UDP	1
#define COMM_VERIFY	2
#define COMM_UPDATE	3
#define COMM_MODE	4	/* put this one last */
#define SWITCH_TCP_OK	0
#define SWITCH_UDP_OK	1
#define SWITCH_DENIED	2
#define SWITCH_VERIFY	3
#define CONNMODE_PORT	0
#define CONNMODE_PACKET	1
#define STAT_CONNECTED	0
#define STAT_SWITCH_UDP	1
#define STAT_SWITCH_TCP	2
#define STAT_VERIFY_UDP	3
#define MODE_TCP	0
#define MODE_SIMPLE	1
#define MODE_FAT	2
#define MODE_DOUBLE	3	/* put this one last */

#define UDP_RECENT_INTR	300
#define UDP_UPDATE_WAIT 5	/* 5 second wait */

/* server version of UDPDIAG */
/* (change these to "#define UDPDIAG(x) <return>" for smaller & faster code) */
#define UDPDIAG(x)	{ if (configvals->udpAllowed > 1) { printf("UDP: "); printf x; }}
#define V_UDPDIAG(x)	{ if (configvals->udpAllowed > 2) { printf("UDP: "); printf x; }}

#define FAE_RATE 8		/* The Fighter-Army exchange rate */
#define FTORP_DAMAGE 50
#define FTORP_SPEED 12
#define FTORP_FUSE 40
#define FTORP_TRACK 1

#define RSA_VERSION "RSA v2.0 SERVER"	/* String must begin with "RSA v" */
#define KEY_SIZE 32
#define RESERVED_SIZE 16
#define MSG_LEN 80
#define NAME_LEN 16
#define KEYMAP_LEN 96

/* random number stuff */
#if 0
#ifdef HAVE_RAND48

#define MAXRAND INT_MAX

#else				/* HAVE_RAND48 */

#ifdef HAVE_RANDOM

#define MAXRAND INT_MAX

#else				/* HAVE_RANDOM */

#define MAXRAND INT_MAX /* (RAND_MAX+1) */
#define random rand
#define srandom srand

#endif				/* HAVE_RANDOM */

#define lrand48 random
#define srand48(s) srandom((int)s)
#define drand48() ((double)random() / (double)MAXRAND)

#endif				/* HAVE_RAND48 */
#endif


#endif				/* DEFS_H */
