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

#ifndef SHMEM_H
#define SHMEM_H

#include "config.h"
#include "defs.h"
#include "struct.h"

enum spec_weapons_e {
    WP_PLASMA,
    WP_TRACTOR,
    WP_MISSILE,
    WP_FIGHTER,
    WP_MAX
};

enum ships_systems {		
    /* Terrain could, I suppose, affect any of these */
    /* I may have forgotten a couple, feel free to add.. */
    SS_PLASMA,
    SS_TRACTOR,
    SS_MISSILE,
    SS_FIGHTER,
    SS_PHOTON,
    SS_PHASER,
    SS_SHIELD,
    SS_REPAIR,
    SS_CLOAK,
    SS_SCANNER,
    SS_SENSOR,
    SS_WARP,
    SS_IMPULSE,
    SS_DOCK,
    SS_NAVIGATION,
    SS_COMMUNICATION,
    SHIPS_SYSTEMS
};

struct configuration {
    int     tournplayers;	/* how many players necessary for T-mode */
    int     ntesters;		/* number of slots reserved for robots */

    char    binconfirm;		/* binary confirmation */
    float   maxload;		/* maximum load */
    char    udpAllowed;		/* is UDP allowed */
    int     min_upd_delay;	/* minimum update delay */
    int     min_observer_upd_delay;	/* minimum observer update delay */

    /* planet things */
#ifdef LOADABLE_PLGEN
    char    galaxygenerator[32];	/* which module to gen galaxy with */
#else
    int     galaxygenerator;	/* which method to generate the galaxy with */
#endif
    int     numplanets;		/* number of planets */
    float   planupdspd;		/* planet movement speed */
    char    resource_bombing;	/* have growable and bombable resources? */
    char    revolts;		/* are revolts allowed? */
    unsigned char popscheme;	/* individual planet popping scheme */
    unsigned char popchoice;	/* how to choose a planet for popping */
    int     popspeed;		/* 100 is normal speed */
    char    evacuation;		/* allow beaming up below 5 armies? */
    char    new_army_growth;	/* base army growth on current pop? */
    int     planetsinplay;	/* Max # of planets in play */
    int     planetlimittype;	/* how planet taking is limited */
    char    beamlastarmies;	/* if you can, or cannot beam up the last
				   armies. */
    char    justify_galaxy;     /* Do we want to ensure that the galaxy is
                                   "fair"? */
    int     num_wormpairs;      /* The number of wormhole pairs --
                                   DON'T SET ABOVE (NUMPLANETS/2) */
    int     num_nebula;         /* the number of nebulas in the galaxy. */
    int     nebula_density;     /* not yet used. Will be used for funky-shaped nebs */
    int     nebula_subclouds;   /* the number of nebulous subclouds */
    int     num_asteroid;       /* number of asteroid belts in the galaxy */
    float   asteroid_thickness; /* the thickness of an asteroid belt. */
    int     asteroid_density;   /* the density (% chance) of an asteroid field */
    int     asteroid_radius;    /* distance of a belt from its star */
    float   asteroid_thick_variance; /* float value -- useful range 0.0 - 10.0 or so */
    int     asteroid_dens_variance; /* int value -- useful range 0 - 200 */
    int     asteroid_rad_variance; /* int value -- useful range 0 - 255 */

    float   plkills;		/* how many kills to allow plasmas */
    float   mskills;		/* how many kills to allow missiles */
    float   erosion;		/* hull erosion factor */
    float   penetration;	/* shield penetration factor */
    int     newturn;		/* TC's new style turns */
    int     hiddenenemy;	/* enemies hidden in T-mode */
    int     gwidth;		/* galactic width */
    char    bronco_shipvals;	/* use bronco ship values? */
    char    afterburners;	/* can people use afterburners? */
    char    warpdrive;		/* is warping allowed? */
    char    fuel_explosions;	/* are explosions based on gas? */
    char    newcloak;		/* use new cloaking? */
    char    bronco_ranks;	/* use bronco ranking? NYI, kinda */

    char    warpdecel;		/* non-instant deceleration from warp */
    char    affect_shiptimers_outside_T;	/* should ship destruction
						   outside of T-mode affect
						   the team ship timers? */
    /* NYTested */
    char    durablescouting;	/* if 0, then you have to keep scouting
				   planets to get up-to-date info.  If 1,
				   then you only need to scout it once. */

    /* NYI */
    char    facilitygrowth;	/* if 0, then you can't bomb or grow
				   facilities.  if 1, then you can. */
    char    repair_during_warp_prep;
    char    repair_during_warp;	/* can you repair during warp (prep)? */
    char    fireduringwarpprep;	/* can you fire during warp prep? */
    char    fireduringwarp;	/* can you fire while warping? */
    char    firewhiledocked;	/* can you fire while docked? */

#define WPS_NOTRACT	0	/* tractors do not affect warp prep */
#define WPS_TABORT	1	/* tractors cause an abort at the end */
#define WPS_TPREVENT	2	/* tractors prevent entering warp  */
#define WPS_TABORTNOW	3	/* tractors immediately abort warp prep  */
#define WPS_TSUSPEND	4	/* tractors suspend countdown */
    char    warpprepstyle;

    char    baserankstyle;	/* if nobody on the team has enough rank */
				/* to get a special ship; and this is set */
				/* to 1, then the player with the highest */
				/* rank on the team may get the spec. ship */

    char    cloakduringwarpprep;
    char    cloakwhilewarping;
    char    tractabortwarp;	/* this many tractors will abort warp */

    float   orbitdirprob;	/* probability that the player will orbit
				   clockwise */
    char    neworbits;          /* New, incremental planet distances from
                                   the parent star -- important for games
                                   with terrain. */

    int     sun_effect[SHIPS_SYSTEMS];	/* suns affect these systems */
    int     ast_effect[SHIPS_SYSTEMS];	/* asteroids affect these systems */
    int     neb_effect[SHIPS_SYSTEMS];	/* a nebula affects these */
    int     wh_effect[SHIPS_SYSTEMS];   /* wormhole effects */
    int     improved_tracking[SHIPS_SYSTEMS];	/* which weapons use IT */
    int     shipsallowed[NUM_TYPES];	/* which ships are allowed */
    int     weaponsallowed[WP_MAX];	/* which special weapons are allowed */
    /* league configuration stuff */
    int     timeouts;		/* timeouts per team in league play */
    int     regulation_minutes;	/* minutes in regulation play */
    int     overtime_minutes;	/* minutes in overtime play */
    int     playersperteam;	/* maximum number of players per team */
    /* ping stuff */
    int     ping_period;	/* ping period in seconds */
    int     ping_iloss_interval;/* in terms of ping_period */
    int     ping_allow_ghostbust; /* allow ghostbust detection from
				     ping_ghostbust (cheating possible) */
    int     ping_ghostbust_interval; /* in terms of ping_period, when to
					ghostbust (if allowed) */

    char    cluecheck;		/* should we check clue? */
    int	    cluedelay;		/* maximum time between clue checks */
    int	    cluetime;		/* time player has to respond to a clue check*/
    int	    cluesource;		/* source of clue phrase */
#define	CC_COMPILED_IN_PHRASE_LIST	0
#define	CC_MOTD				1
#define	CC_PHRASE_LIST_FILE		2

    int     variable_warp;	/* allow variable warp or not [BDyess] */
    int     warpprep_suspendable;	/* allow warp prep to be suspended
					   [BDyess] */
    int     warpzone;		/* radius of warp zone [BDyess] */

    /* tmode stuff */
    int     nopregamebeamup;	/* whether to disallow beamup before a game */
    int     gamestartnuke;	/* whether everyone is nuked at game start */
    int     nottimeout;		/* # minutes after loss of tmode before */
				/*  galaxy resets (0 disables) */

    /* put planet growth timer constants here */
    struct plgrow_ {
	int     fuel, agri, repair, shipyard;
    }       plgrow;

#define PLGFUEL configvals->plgrow.fuel
#define PLGAGRI configvals->plgrow.agri
#define PLGREPAIR configvals->plgrow.repair
#define PLGSHIP configvals->plgrow.shipyard
#define PLGSHIPYARD PLGSHIP

    int     helpfulplanets;	/* planets help fuel/etmep */

    char    wb_bombing_credit;	/* double stats for WB bombing? */
    char    js_assist_credit;	/* JS gets real stats for assists? */
    char    butttorp_penalty;	/* penalty for butt torpers? */
    char    slow_bomb;		/* slow bombing? */
    char    robot_stats;	/* robots accumulate statistics? */

    float   losing_advantage;	/* give losing team an army advantage? */

    int     victory_planets;	/* if enemy planets <= victory planets
                                   you win; victory_planets = 0 is original
				   "take everything" genocide code. */
    char    revolt_with_facilities;	/* can planets with facilities
                                           revolt? */
    float   kill_carried_armies;	/* is there a % chance of killing
                                           carried armies when damaged beyond
					   50%? */
    char    shipyard_built_by_sb_only;	/* can only SB build a shipyard? */
    char    can_bomb_own_shipyard;	/* Players can bomb their own SYS? */

    int     surrstart;			/* # planets where surrender starts */
    int     surrend;			/* # planets where surrender stops */
    int     surrlength;			/* # minutes to achive surrend */

    float   army_defend_facilities;	/* chance army beats off enemy if
    					   defending facilities */
    float   army_defend_bare;		/* chance army beats off enemy if
    					   defending without facilities */
};

#ifndef SHMEM_C
extern struct player *players;
extern struct torp *torps;
extern struct missile *missiles;
extern struct thingy *thingies;
extern struct plasmatorp *plasmatorps;
extern struct status *status;
extern struct status2 *status2;
extern struct planet *planets;
extern struct t_unit *terrain_grid;
extern struct phaser *phasers;
extern int *stars;
extern struct mctl *mctl;
extern struct message *messages;
extern struct team *teams;

extern struct ship *shipvals;
extern struct configuration *configvals;
extern int *shipsallowed;	/* points inside configvals */
extern int *weaponsallowed;	/* ditto */
extern int *sun_effect;		/* how do suns affect ship systems? */
extern int *ast_effect;		/* asteroids */
extern int *neb_effect;		/* nebulae */
extern int *wh_effect;          /* wormholes */
extern int *num_nebula;         /* number of nebulae */
extern int *nebula_density;     /* funkyness of nebulae -- NYI */
extern int *nebula_subclouds;   /* number of subclouds per nebula */
extern int *num_asteroid;       /* number of asteroid fields */
extern float *asteroid_thickness;/* thickness of an asteroid belt */
extern int *asteroid_density;   /* density of asteroid belts */
extern int *asteroid_radius;    /* distance from owning star */
extern float *asteroid_thick_variance;
extern int *asteroid_dens_variance;
extern int *asteroid_rad_variance;
extern int *improved_tracking;	/* smarter tracking algorithm */
extern char *galaxyValid;	/* does galaxy go invalid?  0 if so */
extern char *cluephrase_storage;
#endif

#define CLUEPHRASE_SIZE	1024
#define NUMPLANETS configvals->numplanets
#define GWIDTH configvals->gwidth
#define WORMPAIRS configvals->num_wormpairs

#endif /* SHMEM_H */
