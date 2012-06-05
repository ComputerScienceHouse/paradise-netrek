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

#ifndef STRUCT_H
#define STRUCT_H

#include "config.h"
#include "defs.h"

/*-----------------------------SHIP STRUCTURE------------------------------*/

enum ship_types_e {
    SCOUT, DESTROYER, CRUISER, BATTLESHIP, ASSAULT, STARBASE, ATT,
    JUMPSHIP, FRIGATE, WARBASE, LIGHTCRUISER, CARRIER, UTILITY, PATROL,
    NUM_TYPES
};

struct drivestat {
    /* various drive type statistics */
    int     acc;		/* acceleration */
    int     dec;		/* decelleration */
    int     cost;		/* fuel cost */
    int     maxspeed;		/* maximum speed */
    int     etemp;		/* engine temperature */
};

struct weaponstat {
    short   damage;		/* damage potential */
    short   speed;		/* speed for missiles, range for beams */
    short   cost;		/* fuel cost */
    short   fuse;		/* how long they last */
    short   wtemp;		/* weapon temperature cost */
    short   wtemp_halfarc;	/* Arc the penalty is calculated from */
    short   wtemp_factor;	/* Penalty Factor 1-16 of wtemp caused */
    short   count;		/* how many we can have airborne */
    short   aux;		/* aux field */
    /* aux is turn rate for torps, plasmas, and missiles */
};

struct ship {

    short   s_type;		/* ship type, defined with the number */
    short   s_alttype;		/* This MUST be a legal vanilla bronco type */
    char    s_name[32];		/* ship's name */

    /* engine characteristics */
    int     s_turns;		/* ship\'s turning */
#undef	s_imp			/* bloody obsolete header files */
    struct drivestat s_imp;	/* impulse drive stats */
    struct drivestat s_after;	/* impulse drive stats */
    struct drivestat s_warp;	/* impulse drive stats */
    int     s_warpinitcost;	/* fuel charge to initialize warp */
    int     s_warpinittime;	/* time to initialize warp */
    int     s_warpprepspeed;	/* speed while initializing warp */

    short   s_mass;		/* to guage affect of tractor beams */

    /* tractor characteristics */
    short   s_tractstr;		/* Strength of tractor beam */
    float   s_tractrng;		/* tractor range */
    int     s_tractcost;	/* fuel used by tractors */
    int     s_tractetemp;	/* e-temp caused by tractors */

    struct weaponstat s_torp;	/* torp characteristics */
    struct weaponstat s_phaser;	/* phaser characteristics */
    struct weaponstat s_missile;/* missile characteristics */
    struct weaponstat s_plasma;	/* plasma characteristics */
    short   s_missilestored;	/* how many missiles can we store? */

/* Raynfala's Butt Torp Code. Gjor's Ship Patch for his code to do it */
/*   for each ship.						       */

    short   s_torp_penalty_halfarc;
    short   s_torp_penalty_factor;

    int     s_maxwpntemp;	/* max W-temp */
    short   s_wpncoolrate;	/* weapon cool rate */

    int     s_maxegntemp;	/* max engine temp */
    short   s_egncoolrate;	/* engine cool rate */

    /* fuel characteristics */
    int     s_maxfuel;		/* ship's maximum fuel */
    short   s_recharge;		/* speed fuel recharges */
    int     s_mingivefuel;	/* ship must have this much to give fuel */
    int     s_takeonfuel;	/* how fast ship takes on fuel */

    short   s_expldam;		/* damage done by ship's explosion (assuming
				   his fuel tank is empty) */
    short   s_fueldam;		/* the amount of _additional_ damage that
				   this ship's explosion does with a full
				   tank */

    /* miscellaneous army stats */
    float   s_armyperkill;	/* the number of armies per kill */
    short   s_maxarmies;	/* max armies ship can carry */
    int     s_bomb;		/* bomb damage ship can do */
    unsigned s_bombflags;	/* what damage bombing can cause */

    /* hull, shield and repair stats */
    short   s_repair;		/* ship's repair rate */
    int     s_maxdamage;	/* maximum damage ship can take */
    int     s_maxshield;	/* maximum shield value */
    int     s_shieldcost;	/* cost in fuel of shields being up */

    short   s_detcost;		/* fuel cost of detting */
    int     s_detdist;		/* fuel cost of detting */
    short   s_cloakcost;	/* base fuel cost of cloaking */

    short   s_scanrange;	/* range of the ship's scanners */

    short   s_numports;		/* how many docking ports do we have? */

    char    s_letter;		/* the letter used to enter as that ship */
    char    s_desig1;		/* the designation used (such as A, in AS */
    char    s_desig2;		/* the designation used (such as S, in AS */

    short   s_bitmap;		/* the bitmap to use */
    short   s_width;		/* width of bitmap */
    short   s_height;		/* height of bitmap */

    /* ship requisition limitations */
    int     s_timer;		/* value to set timer to get another ship */
    int     s_maxnum;		/* maximum number of these per team */
    int     s_rank;		/* minimum rank for this sucker */
    int     s_numdefn;		/* number of player necessary */
    int     s_numplan;		/* number of planets */

    long    s_nflags;		/* attributes for ship type, SFN flags */
};

/* for s_bombflags */
#define SBOMB_NONE     0
#define SBOMB_ARMIES   (1 << 0)
#define SBOMB_FUEL     (1 << 1)
#define SBOMB_AGRI     (1 << 2)
#define SBOMB_REPAIR   (1 << 3)
#define SBOMB_SHIPYARD (1 << 4)
#define SBOMB_FACILITIES (SBOMB_FUEL | SBOMB_AGRI | SBOMB_REPAIR | SBOMB_SHIPYARD)
#define SBOMB_ALL (SBOMB_FACILITIES | SBOMB_ARMIES)

/* ATTENTION!!!
   Changes to these flags should be mirrored in structdesc.c
   */
 /* For s_nflags field */
#if 0
#define SFNDOCKON       (1<< 0)	/* specify a ship to docked on */
#define SFNCANDOCK      (1<< 1)	/* specify a ship can dock */
#else
#define SFNUNDOCKABLE	(1<< 0)	/* this ship can not dock with another */
#define SFNCANORBIT     (1<< 1)	/* specify a ship can orbit */
#endif
#define SFNCANWARP      (1<< 2)	/* this ship can go warp */
#define SFNCANFUEL	(1<< 3)	/* base can give docked ships fuel */
#define SFNCANREPAIR	(1<< 4)	/* base can repair docked ships */
#define SFNCANREFIT	(1<< 5)	/* base allows a refit while docked */
#define SFNARMYNEEDKILL (1<< 6)	/* does ship need kills to carry */

#define SFNHASPHASERS	(1<< 7)	/* this ship has phasers */

#define SFNPLASMASTYLE  (1<< 8)	/* style of firing plasmas */

#define SFNMASSPRODUCED (1<< 9)	/* ship can appear docked to an SB or UT */

#define SFNPLASMAARMED	(1<<10)
#define SFNHASMISSILE	(1<<11)

#define SFNHASFIGHTERS	(1<<12)	/* ship has a fighter bay */

#if 0
#define allows_docking(s_ship)	((s_ship).s_nflags & SFNDOCKON)
#define can_dock(s_ship)	((s_ship).s_nflags & SFNCANDOCK)
#else
#define allows_docking(s_ship)	((s_ship).s_numports>0)
#define can_dock(s_ship)	((s_ship).s_numports==0 && !((s_ship).s_nflags&SFNUNDOCKABLE))
#endif
/*-------------------------------------------------------------------------*/







/*----------------------------STATS STRUCTURE------------------------------*/

struct stats {
    int     st_genocides;	/* number of genocides participated in */
    float   st_tmaxkills;	/* max kills ever */
    float   st_di;		/* total destruction inflicted for all time */
    int     st_tkills;		/* Kills in tournament play */
    int     st_tlosses;		/* Losses in tournament play */
    int     st_tarmsbomb;	/* Tournament armies bombed */
    int     st_tresbomb;	/* resources bombed off */
    int     st_tdooshes;	/* armies killed while being carried */
    int     st_tplanets;	/* Tournament planets conquered */
    int     st_tticks;		/* Tournament ticks */
    /* SB/WB/JS stats are entirely separate */
    int     st_sbkills;		/* Kills as starbase */
    int     st_sblosses;	/* Losses as starbase */
    int     st_sbticks;		/* Time as starbase */
    float   st_sbmaxkills;	/* Max kills as starbase */
    int     st_wbkills;		/* Kills as warbase */
    int     st_wblosses;	/* Losses as warbase */
    int     st_wbticks;		/* Time as warbase */
    float   st_wbmaxkills;	/* Max kills as warbase */
    int     st_jsplanets;	/* planets assisted with in JS */
    int     st_jsticks;		/* ticks played as a JS */
    long    st_lastlogin;	/* Last time this player was played */
    int     st_flags;		/* Misc option flags */
    int	    st_cluesuccess;	/* how many times you passed a clue check */
    char    st_pad[92];		/* space for expansion */
    int     st_rank;		/* Ranking of the player */
    int     st_royal;		/* royaly, specialty, rank */
};

 /*
    These are for the flags field and control the preset options of the
    players client.
 */
#if 0
#define ST_MAPMODE      0x001
#define ST_NAMEMODE     0x002
#define ST_SHOWSHIELDS  0x004
#endif
#define ST_NOBITMAPS	(1<<0)
#define ST_KEEPPEACE    (1<<3)
#if 0
#define ST_SHOWLOCAL    0x010	/* two bits for these two */
#define ST_SHOWGLOBAL   0x040
#endif
#define ST_CYBORG	(1<<8)

 /* initial value to put in flags */
#define ST_INITIAL ST_KEEPPEACE
 /*
    ST_MAPMODE+ST_NAMEMODE+ST_SHOWSHIELDS+ \
    ST_KEEPPEACE+ST_SHOWLOCAL*2+ST_SHOWGLOBAL*2;
 */

/*-------------------------------------------------------------------------*/







/*---------------------------VARIOUS STRUCTURES----------------------------*/

 /*
    This is used to indicate various things about the game and to hold global
    stats about it.  All stats are only updated while in t-mode.
 */
struct status {
    int     active;		/* for interfacing with people who */
    unsigned int wait, count;	/* want to get into the game */
    unsigned int number, request, answer;
    unsigned char tourn;	/* Tournament mode? */
    unsigned long dooshes;	/* total number of armies dooshed */
    unsigned long armsbomb;	/* all t-mode armies bombed */
    unsigned long resbomb;	/* resources bombed */
    unsigned long planets;	/* all t-mode planets taken */
    unsigned long kills;	/* all t-mode kills made */
    unsigned long losses;	/* all t-mode losses */
    unsigned long genocides;	/* number of genocides */
    unsigned long sbkills;	/* total kills in SB's */
    unsigned long sblosses;	/* total losses in Sb's */
    unsigned long sbtime;	/* total time in SB's */
    unsigned long wbkills;	/* kills in warbases */
    unsigned long wblosses;	/* losses in warbases */
    unsigned long wbtime;	/* total time played in wb's */
    unsigned long jsplanets;	/* total planets taken by jump ships */
    unsigned long jstime;	/* total time in a jump ship */
    unsigned long time;		/* t-mode time */
    unsigned long timeprod;	/* t-mode ship ticks--sort of like */
    /* manhours in t-mode */
    unsigned long clock;	/* clock used for timestamping */
    int     nukegame;		/* set to PID of daemon */
    int     gameup;		/* is game up */
};

/* Some of the stuff above belongs in struct status2
   */

struct league_team {
    int     index;		/* team index, not mask. -1 means captain
				   hasn't chosen yet */
    int     captain;		/* player number of captain */
    char    name[32];
    int     timeouts_left;	/* NYI */
    int     ready;
    int     desirepause;	/* do we want to pause the game? */

    struct {
	int     regulation;	/* game length */
	int     overtime;
	int     maxplayers;	/* max players per team */
	int     galaxyreset;	/* do we want to reset the galaxy? */
	int     restart;	/* should we restart team selection? */
    }       desired;
};

struct status2 {
    int     league;		/* are we playing league? 0 means no. 1 means
				   we're prepping (captains choose sides). 2
				   means we're almost there. 3 means we're in
				   regulation league play. 4 means we're in
				   overtime */
    int     leagueticksleft;	/* ticks left in game play */

    struct league_team home, away;

    int     timeout_timeleft;	/* ticks left in a timeout */
    int     awaypassed;		/* does the away team get first choice? this
				   becomes 1 if they PASS */
    int     paused;		/* contains 0 if the game is not paused.
				   contains the number of ticks left before
				   return to normal play if already paused */
    int     pausemsgfuse;	/* a fuse that prints out a reminder every
				   few seconds */
    int     starttourn;
    int     newgalaxy;
    int     nontteamlock;
};


 /*
    used for the request field of status.  These are used for a client
    querying the queue to see if he can get in
 */

enum queue_request_type_e {
    REQFREE, REQWHO, REQDEAD
};

 /*
    This structure is used to hold various info that each team has, such as
    how long before a certain ship type is done with construction.
 */
struct team {
    char    nickname[32];	/* he is now a %s  */
    char    name[32];		/* come join the %s */
    char    letter;		/* 1-letter abbrev */
    char    shortname[4];	/* 3-letter abbrev */

    int     s_turns[NUM_TYPES];	/* turns till another ship is legal */
    int     s_surrender;	/* minutes until this team surrenders */
    int     s_plcount;		/* how many planets this team owns */
    int     s_plcountold;	/* the old planet count */
};


 /*
    this is used for getting a player name and password and fetching a
    players stats
 */
struct statentry {
    char    name[16];		/* player's name */
    char    password[16];	/* player's password */
    struct stats stats;		/* player's stats */
};

/* Used by the function that computes ratings */
struct rating {
    float   battle;
    float   strategy;
    float   special;

    float   bombrat;
    float   planetrat;
    float   resrat;
    float   offrat;
    float   dooshrat;
    float   ratio;

    float   sbrat;
    float   wbrat;
    float   jsrat;
};

/*-------------------------------------------------------------------------*/







/*-----------------------------PLAYER STRUCTURE----------------------------*/

#define MAXPORTS	6	/* maximum number of docking bays a ship type
				   can have */

 /* These are defines for the p_whydead field of the player structure */
enum why_dead_e {
    KNOREASON,
    KQUIT,			/* Player quit */
    KTORP,			/* killed by torp */
    KPHASER,			/* killed by phaser */
    KPLANET,			/* killed by planet */
    KSHIP,			/* killed by other ship */
    KDAEMON,			/* killed by dying daemon */
    KWINNER,			/* killed by a winner */
    KGHOST,			/* killed because a ghost */
    KGENOCIDE,			/* killed by genocide */
    KPROVIDENCE,		/* killed by a hacker */
    KPLASMA,			/* killed by a plasma torpedo */
    KTOURNEND,			/* killed by the end of the tourn */
    KOVER,			/* killed by overtime */
    KTOURNSTART,		/* killed by the start of the tournament */
    KBINARY,			/* killed by improper client */
    KMISSILE,			/* killed by a missile */
    KASTEROID                   /* smashed by an asteroid */
};

 /* These are number defines for the player structs p_status field */
enum player_status_e {
    PFREE,			/* player slot is not filled */
    POUTFIT,			/* player in process of being ghost busted */
    PALIVE,			/* player is alive */
    PEXPLODE,			/* player is in process of exploding */
    PDEAD,			/* player is dead */
    PTQUEUE,			/* player is on the tournament queue */
    POBSERVE			/* player is observing the game */
};


struct player {
    int     p_no;		/* player number in player array */
    int     p_updates;		/* Number of updates ship has survived */
    enum player_status_e p_status;	/* Player status */
    char    p_observer;		/* is the player only an observer? */
    unsigned int p_flags;	/* Player flags */
    char    p_name[16];		/* name of player */
    char    p_login[16];	/* as much of user name and site we can hold */
    char    p_monitor[16];	/* Monitor being played on */
    struct ship p_ship;		/* Personal ship statistics */
    int     p_x;		/* players x coord in the galaxy */
    int     p_y;		/* player y coord in the galaxy */

    unsigned char p_dir;	/* Real direction */
    int     p_subdir;		/* fraction direction change */
    unsigned char p_desdir;	/* desired direction */

    int     p_speed;		/* Real speed */
    int     p_subspeed;		/* Fractional speed */
    short   p_desspeed;		/* Desired speed */
    short   p_warpdesspeed;	/* Desired warp speed, after prep [BDyess] */

    short   p_team;		/* Team I'm on */
    enum HomeAway {
	NEITHER, HOME, AWAY
    }       p_homeaway;
    char    p_spyable;		/* can you watch this player? */
    char    p_teamspy;		/* (mask) what teams can this player watch? */

    int     p_damage;		/* Current damage */
    int     p_subdamage;	/* Fractional damage repair */

    int     p_shield;		/* Current shield power */
    int     p_subshield;	/* Fractional shield recharge */

    short   p_cloakphase;	/* Drawing stage of cloaking engage/disengage */

    short   p_ntorp;		/* Number of torps flying */
    short   p_nplasmatorp;	/* Number of plasma torps active */
    short   p_nthingys;		/* number of thingys we own. */
    long    p_specweap;		/* which special weapons we're packing */

    char    p_hostile;		/* Who my torps will hurt */
    char    p_swar;		/* Who am I at sticky war with */

    float   p_kills;		/* Enemies killed */
    short   p_planet;		/* Planet orbiting or locked onto */
    char    p_orbitdir;		/* direction you are orbiting the planet in */
    short   p_playerl;		/* Player locked onto */
    short   p_armies;		/* armies player is carrying */

    int     p_fuel;		/* fuel player's ship currently has */
    short   p_explode;		/* Keeps track of final explosion */

    int     p_etemp;		/* player's current engine temp */
    int     p_subetemp;		/* fractional part of E-temp */
    short   p_etime;		/* timer for coming out of E-temp */

    int     p_wtemp;		/* current weapon temp */
    short   p_wtime;		/* timer for coming out of W-temp */

    enum why_dead_e p_whydead;	/* Tells you why you died */
    short   p_whodead;		/* Tells you who killed you */
    struct stats p_stats;	/* player statistics */
    short   p_planets;		/* planets taken this game */
    short   p_armsbomb;		/* armies bombed this game */
    short   p_resbomb;		/* resources bombed */
    short   p_dooshes;		/* armies being carried killed */
    int     p_ghostbuster;	/* ??????????????? */
    int     p_docked;		/* If SB, # docked to, else no base host */
    int     p_port[MAXPORTS];	/* If SB, pno of ship docked to that port,
				   else p_port[0] = port # docked to on host. */
    short   p_tractor;		/* What player is in tractor lock */
    int     p_pos;		/* My position in the player file */
    char    p_full_hostname[32];/* full hostname 4/13/92 TC */
#if 0
    int     p_planfrac;		/* for getting fractional credit for */
    int     p_bombfrac;		/* bombing and planet taking */
#endif
    int     p_warptime;		/* timer for warp countdown */
    int     p_jsdock;		/* to keep track of undocking from JS */
    int     p_lastjs;		/* player number of last JS ridden on */
    int     p_lastman;		/* flag used to beam up last men */

    int     p_lastrefit;	/* what shipyard you last refitted at (plno) */
    /* ping stuff */
    int     p_avrt;		/* average round trip time */
    int     p_stdv;		/* standard deviation in round trip time */
    int     p_pkls;		/* packet loss (input/output) */

    /* clue checking goop */
    int		p_cluecountdown;
    int		p_cluedelay;

    int     p_ntspid;		/* proc ID of ntserv in control of this slot */
    int     p_zone;		/* total warp zone bonus/penalty [BDyess] */

    int     gen_distress;	/* generate generic distress messages for client */
};

 /* These are defines that used for the player struct's p_flags field */
#define PFSHIELD	  (1<< 0)	/* shields are raised */
#define PFREPAIR	  (1<< 1)	/* player in repair mode */
#define PFBOMB		  (1<< 2)	/* player is bombing */
#define PFORBIT		  (1<< 3)	/* player is orbiting */
#define PFCLOAK		  (1<< 4)	/* player is cloaked */
#define PFWEP		  (1<< 5)	/* player is weapon temped */
#define PFENG		  (1<< 6)	/* player is engine temped */
#define PFROBOT		  (1<< 7)	/* player is a robot */
#define PFBEAMUP	  (1<< 8)	/* player is beaming up */
#define PFBEAMDOWN	  (1<< 9)	/* player is beaming down */
#define PFSELFDEST	  (1<<10)	/* player is self destructing */
#define PFGREEN		  (1<<11)	/* player at green alert */
#define PFYELLOW	  (1<<12)	/* player is at yellow alert */
#define PFRED		  (1<<13)	/* player is at red alert */
#define PFPLOCK		  (1<<14)	/* Locked on a player */
#define PFPLLOCK	  (1<<15)	/* Locked on a planet */
#define PFCOPILOT	  (1<<16)	/* Allow copilots */
#define PFWAR		  (1<<17)	/* computer reprogramming for war */
#define PFPRACTR	  (1<<18)	/* practice type robot (no kills) */
#define PFDOCK            (1<<19)	/* true if docked to a starbase */
#define PFREFIT           (1<<20)	/* true if about to refit */
#define PFREFITTING	  (1<<21)	/* true if currently refitting */
#define PFTRACT  	  (1<<22)	/* tractor beam activated */
#define PFPRESS  	  (1<<23)	/* pressor beam activated */
#define PFDOCKOK	  (1<<24)	/* docking permission */
#define PFSEEN		  (1<<25)	/* seen by enemy on galactic map? */
#define PFWARPPREP	  (1<<26)	/* in warp prep */
#define PFWARP		  (1<<27)	/* ship warping */
#define PFAFTER		  (1<<28)	/* after burners on */
#define PFWPSUSPENDED	  (1<<29)	/* warp prep suspended [BDyess] */
#define PFSNAKE	          (1<<30)	/* it's a space snake */
#define PFBIRD	          (1<<31)	/* it's a space bird */
/*-------------------------------------------------------------------------*/







/*-----------------------------TORP STRUCTURE-------------------------------*/


 /* For status field of torp structure */
enum torp_status_e {
    TFREE,			/* torp is not being fired */
    TMOVE,			/* torp is moving with wobble */
    TEXPLODE,			/* torp is in the process of exploding */
    TDET,			/* torp is being detted */
    TOFF,			/* torp is off ??? */
    TSTRAIGHT,			/* Non-wobbling torp */
    TRETURN,			/* torp is returning to owner (fighters) */
    TLAND			/* torp gets recovered */
};

struct basetorp {
    int     bt_no;
    enum torp_status_e bt_status;
    int     bt_owner;
    int     bt_x, bt_y;
    unsigned char bt_dir;
    int     bt_damage;
    int     bt_speed;
    int     bt_fuse;
    char    bt_war;
    char    bt_team;
    char    bt_whodet;		/* who detonated... */
};


struct torp {
    struct basetorp t_base;
#define t_no		t_base.bt_no
#define t_status	t_base.bt_status
#define t_owner		t_base.bt_owner
#define t_x		t_base.bt_x
#define t_y		t_base.bt_y
#define t_dir		t_base.bt_dir
#define t_damage	t_base.bt_damage
#define t_speed		t_base.bt_speed
#define t_fuse		t_base.bt_fuse
#define t_war		t_base.bt_war
#define t_team		t_base.bt_team
#define t_whodet	t_base.bt_whodet
    short   t_turns;		/* rate of change of direction if tracking */
};

/*-------------------------------------------------------------------------*/

struct missile {
    struct basetorp ms_base;
#define ms_no		ms_base.bt_no
#define ms_status	ms_base.bt_status
#define ms_owner	ms_base.bt_owner
#define ms_x		ms_base.bt_x
#define ms_y		ms_base.bt_y
#define ms_dir		ms_base.bt_dir
#define ms_damage	ms_base.bt_damage
#define ms_speed	ms_base.bt_speed
#define ms_fuse		ms_base.bt_fuse
#define ms_war		ms_base.bt_war
#define ms_team		ms_base.bt_team
#define ms_whodet       ms_base.bt_whodet
    short   ms_turns;
    short   ms_locked;
    short   ms_type;		/* A missile, fighter, or mine */
    short   fi_hasfired;	/* has the fighter fired a torp? */
};
/* defines for ms_type */
#define MISSILETHINGY 0
#define FIGHTERTHINGY 1
#define MINETHINGY 2

enum thingy_type {
    TT_NONE,
    TT_WARP_BEACON
};

struct warp_beacon {
    int     owner;
    int     x, y;
};

struct thingy {
    enum thingy_type type;
    union {
	struct warp_beacon wbeacon;
    }       u;
};



/*---------------------------PLASMA TORP STRUCTURE-------------------------*/
 /* For the plasma status field */
struct plasmatorp {
    struct basetorp pt_base;
#define pt_no		pt_base.bt_no
#define pt_status	pt_base.bt_status
#define PTFREE	TFREE
#define PTMOVE	TMOVE
#define PTEXPLODE	TEXPLODE
#define PTDET	TDET
#define pt_owner	pt_base.bt_owner
#define pt_x		pt_base.bt_x
#define pt_y		pt_base.bt_y
#define pt_dir		pt_base.bt_dir
#define pt_damage	pt_base.bt_damage
#define pt_speed	pt_base.bt_speed
#define pt_fuse		pt_base.bt_fuse
#define pt_war		pt_base.bt_war
#define pt_team		pt_base.bt_team
#define pt_whodet	pt_base.bt_whodet
    short   pt_turns;		/* ticks turned per cycle */
};


/*-------------------------------------------------------------------------*/







/*-----------------------------PHASER STRUCTURE---------------------------*/

struct phaser {
    int     ph_status;		/* What it's up to */
    unsigned char ph_dir;	/* direction */
    int     ph_target;		/* Who's being hit (for drawing) */
    int     ph_x, ph_y;		/* For when it hits a torp */
    int     ph_fuse;		/* Life left for drawing */
    int     ph_damage;		/* Damage inflicted on victim */
};

 /* for phaser's status field */
#define PHFREE 0x00		/* phaser not being fired */
#define PHHIT  (1<<0)		/* When it hits a person */
#define PHMISS (1<<1)		/* phaser missed */
#define PHHIT2 (1<<2)		/* When it hits a plasma */

/*-------------------------------------------------------------------------*/







/*-----------------------------PLANET STRUCTURE---------------------------*/

 /* This structure is used to hold what each team knows about a planet */
struct teaminfo {		/* to hold what a team knows about a planet */
    int     owner;		/* planet's owner */
    int     armies;		/* number of armies team knows about */
    int     flags;		/* flags team knows about */
    int     timestamp;		/* time info was taken */
};

struct planet {			/* all info about a planet */
    int     pl_no;		/* planet number */
    int     pl_flags;		/* attributes of planet */
    int     pl_owner;		/* the current owner of the planet */

    int     pl_x;		/* planet's coords */
    int     pl_y;		/* use the move_planet function to change
				   them */
    int     pl_radius;		/* distance from sun */
    float   pl_angle;		/* angular position relative to sun */
    int     pl_system;		/* planetary system number, 0 = no system */

    char    pl_name[16];	/* name of the planet */
    char    pl_namelen;		/* name's length--Cuts back on strlen's */
#if 1
    char    pl_hostile;
#else
    char    pl_torbit;		/* teams currently in orbit */
#endif
    int     pl_tshiprepair;	/* repair and shipyard growth timer */
    int     pl_tagri;		/* agri growth timer */
    int     pl_tfuel;		/* fuel depot growth timer */
    int     pl_armies;		/* armies curently on planet */
    int     pl_warning;		/* timer so that planets don't talk too much */
    int     pl_hinfo;		/* which races have info on planet */
    struct teaminfo pl_tinfo[MAXTEAM + 1];	/* to hold information for
						   races */
    int     pl_trevolt;		/* timer for planet revolting */
    /* space grid support stuff */
    int     pl_next, pl_prev;	/* doubly linked list of planet numbers */
    int     pl_gridnum;		/* to hold grid square number */
};

 /* defines for the pl_flags field of planet struct */
#if 0
#define PLHOME 	   0x000100	/* These 4 flags no longer are */
#define PLCOUP     0x000200	/* used in the server */
#define PLCHEAP    0x000400
#define PLCORE     0x000800
#define PLREPAIR   0x001010	/* planet can repair ships */
#define PLFUEL     0x002020	/* planet has fuel depot */
#define PLAGRI     0x004040	/* agricultural thingies built here */
#define PLSHIPYARD 0x008000	/* planet has a shipyard on it */
#define PLORESMASK 0x000070	/* mask for original resource flags */
#define PLRESMASK  0x00F000	/* to mask off all but resource bits */
#define PLRESSHIFT       12	/* bit to shift right by for resources */
#define PLSTAR     0x010000	/* planet is actually a star */
#define PLREDRAW   0x000080	/* Player close for redraw */
#define PLPOISON   0x000000	/* poison atmosphere, no army growth */
#define PLATYPE3   0x020000	/* slightly toxic, very slow army growth */
#define PLATYPE2   0x040000	/* thin atmosphere, slow army growth */
#define PLATYPE1   0x060000	/* normal human atmosphere, normal growth */
#define PLATMASK   0x060000	/* to mask off everything but atmos bits */
#define PLATSHIFT	 17	/* number of right bit shifts for atmos bits */
#define PLBARREN   0X000000	/* rocky barren surface */
#define PLDILYTH   0x080000	/* dilythium deposits on the planet */
#define PLMETAL    0x100000	/* metal deposits on the planet */
#define PLARABLE   0x200000	/* planet has farmland */
#define PLSURMASK  0x380000	/* number of surface combinations */
#define PLSURSHIFT 	 19	/* number of bit shift to surface */
#define PLPARADISE 0x400000	/* Paradise server flag set to 1 for P server */
#else

/*
   pl_flags is an int of 32 bits:

   bits 16 and 23 currently define the type of the planet.  The
   interpretation of the other bits is dependent upon the planet
   type.

   Here is the interpretation for a planet
   bits 0..3			unknown
   bits 4..6			planetary facilities (REPAIR,FUEL,AGRI)
   bit  7			redraw (archaic, recyclable?)
   bits 8..11			old flags (archaic, recyclable?)
   bits 12..15			paradise planetary facilities
				(REPAIR,FUEL,AGRI,SHIPY)
   bit  16			cosmic object type (also bit 23)
   bits 17,18			planet atmosphere type
   bits 19..21			planetary surface properties
				(DILYTH,METAL,ARABLE)
   bit  22			paradise planet flag (why?)
   bit  23			cosmic object type (also bit 16)
   bits 24..31	currently unallocated (8 bits to play with)

   Asteroids are NYI but here is a draft standard:
   bits 12,15			facilities
				(REPAIR,FUEL,SHIPY)
   bit  20			surface properties
				(DILYTH,METAL)
   other bits	currently unallocated

   */

/* facilities, bits 4..6 and 12..15
   valid for planets and asteroids */
#define PLREPAIR   ((1<<12) | (1<<4))	/* planet can repair ships */
#define PLFUEL     ((1<<13) | (1<<5))	/* planet has fuel depot */
#define PLAGRI     ((1<<14) | (1<<6))	/* agricultural thingies built here */
#define PLSHIPYARD ((1<<15))	/* planet has a shipyard on it */
#define PLORESMASK (0x7<<4)	/* mask for original resource flags */
#define PLRESSHIFT       12	/* bit to shift right by for resources */
#define PLRESMASK  (0xF<<PLRESSHIFT)	/* to mask off all but resource bits */

#define PLREDRAW   (1<<7)	/* Player close for redraw */

#define PLHOME 	   (1<< 8)	/* These 4 flags no longer are */
#define PLCOUP     (1<< 9)	/* used in the server */
#define PLCHEAP    (1<<10)
#define PLCORE     (1<<11)

/* cosmic object types, bits 16, 23, and 24 */
#define PLPLANET	0	/* object is a planet */
#define PLSTAR     (1<<16)	/* object is a star */
#define PLAST	   (1<<23)	/* object is an asteroid NYI */
#define PLNEB	   ((1<<16)|(1<<23))	/* object is a nebula NYI */
#define PLBHOLE	                    (1<<24)	/* object is a black hole NYI */
#define PLPULSAR   ((1<<16)|        (1<<24))	/* object is a pulsar NYI */
#define PLUK1	   (        (1<<23)|(1<<24))	/* future expansion NYI */
#define PLWHOLE	   ((1<<16)|(1<<23)|(1<<24))	/* object is a wormhole */
#define PLTYPEMASK ((1<<16)|(1<<23)|(1<<24))	/* mask to extract object
						   type */
#define PL_TYPE(p) ( (p).pl_flags & PLTYPEMASK )

/* Atmosphere Types, bits 17 and 18.
   Valid for planets.
   */
#define PLATSHIFT	 17	/* number of right bit shifts for atmos bits */
#define PLPOISON   (0<<PLATSHIFT)	/* poison atmosphere, no army growth */
#define PLATYPE3   (1<<PLATSHIFT)	/* slightly toxic, very slow army
					   growth */
#define PLATYPE2   (2<<PLATSHIFT)	/* thin atmosphere, slow army growth */
#define PLATYPE1   (3<<PLATSHIFT)	/* normal human atmosphere, normal
					   growth */
#define PLATMASK   (0x3<<PLATSHIFT)	/* to mask off everything but atmos
					   bits */

/* Surface Properties, bits 19..21
   Valid for planets and asteroids.
   */
#define PLBARREN   0		/* rocky barren surface */
#define PLSURSHIFT 	 19	/* number of bit shift to surface */
#define PLDILYTH   (1<<(PLSURSHIFT+0))	/* dilythium deposits on the planet */
#define PLMETAL    (1<<(PLSURSHIFT+1))	/* metal deposits on the planet */
#define PLARABLE   (1<<(PLSURSHIFT+2))	/* planet has farmland */
#define PLSURMASK  (0x7<<PLSURSHIFT)	/* number of surface combinations */


#define PLPARADISE (1<<22)	/* Paradise server flag set to 1 for P server */

#endif
/*-------------------------------------------------------------------------*/


/*----------------------T E R R A I N   S T R U C T S----------------------*/
/* Data */
struct t_unit {
  int alt1;		/* alt1, alt2 are currently unused in the client */
  int alt2;		/* according to MDM (5/16/95) */
  char types;		/* modify the packet code as well if these are put */
};			/* back in, as well as the structs in packets.h */

/* Flags */
#define T_EMPTY_SPACE 0x00
#define T_ASTEROIDS   0x01
#define T_NEBULA      0x02
#define T_RADIATION   0x04
#define T_EXPANSN1    0x08
#define T_EXPANSN2    0x10
#define T_EXPANSN3    0x20
#define T_EXPANSN4    0x40
#define T_EXPANSN5    0x80

/*-------------------------------------------------------------------------*/


/*-----------------------------MESSAGE STRUCTS-----------------------------*/

struct message {
    int     m_no;		/* message number in array of messgs */
    int     m_flags;		/* flags for message type */
    int     m_time;		/* time message sent???? */
    int     m_recpt;		/* who it should be sent to */
    char    m_data[80];		/* the message string */
    int     m_from;		/* who it is from */
};

 /*
    for m_flags field -- lower five bits used for what kind of group the
    message is sent to
 */
#define MVALID (1<<0)		/* the message is valid--has not been sent */
#define MINDIV (1<<1)		/* sent to an individual */
#define MTEAM  (1<<2)		/* sent to a team */
#define MALL   (1<<3)		/* sent to eveyone */

/* order these by importance (0x100 - 0x400) */
/* many of these are not used by the client, so their
 * incorrect value doesn't matter.  however we should
 * note that the packet only has a width of 8 bits for
 * the type field, and many of these flags are > 8 bits
 * wide --eld */

#define	MGOD   ( 1<<4)		/* sent to god -- misc.c, socket.c */
#define MGENO  ( 1<<5)		/* genocide message -- conquer.c */
#define MCONQ  ( 2<<5)		/* conquer message -- planets.c */
#define MTAKE  ( 1<<5)		/* planet taken message -- player.c */
#define MDEST  ( 2<<5)		/* planet destroyed message -- player.c */
#define MKILLA ( 5<<5)		/* a person killed -- dutil.c, snakemove.c */
#define MBOMB  ( 3<<5)		/* bombarding planet -- planets.c */
#define MKILLP ( 7<<5)		/* a person killed -- NOT USED */
#define MKILL  ( 8<<5)		/* a person killed -- NOT USED */
#define MLEAVE ( 9<<5)		/* player leaving game message -- main.c */
#define MJOIN  (10<<5)		/* player joining game -- enter.c */
#define MGHOST (11<<5)		/* player ghostbusted -- daemonII.c */
#define MCOUP1 (12<<5)		/* a coup occured -- NOT USED */
#define MCOUP2 (13<<5)		/* -- NOT USED */
#define MDISTR (14<<5)		/* flag for a distress message */
#define MMASK  (0xf<<5)		/* bits in the flags field should be sent -- */

 /* message control structure */
struct mctl {			/* used to keep track of position in */
    int     mc_current;		/* array we put our last message */
};

/*-------------------------------------------------------------------------*/



struct rsa_key {
    unsigned char client_type[KEY_SIZE];
    unsigned char architecture[KEY_SIZE];
    unsigned char global[KEY_SIZE];
    unsigned char public[KEY_SIZE];
};


/*
 * RCD stuff, from bronco server 2.7pl13.
 */
struct	distress	{
	unsigned char	sender;
	unsigned char	dam, shld, arms, wtmp, etmp, fuelp, sts;
	unsigned char	wtmpflag, etempflag, cloakflag, distype, macroflag, ttype;
	unsigned char	close_pl, close_en, target, tclose_pl, tclose_en, pre_app, i;
	unsigned char	close_j, close_fr, tclose_j, tclose_fr;
	unsigned char	cclist[ 6 ];		/* CC list */
	unsigned char	preappend[ 80 ];	/* text which we pre- or append */
};

/*
 * macro management
 */
struct	dmacro_list	{
	char	c;
	char	*name;
	char	*macro;
};

/*
 * distress types 
 */
enum	dist_type	{
	/* help me do series */
	take = 1, ogg, bomb, space_control, help1, help2, help3, help4,

	/* doing series */
	escorting, ogging, bombing, controlling, doing1, doing2, doing3, doing4,

	/* other info series */
	free_beer,		/* player x is totally hosed now */
	no_gas,			/* player x has no gas */
	crippled,		/* player x is crippled but may have fuel */
	pickup,			/* player x picked up armies */
	pop,			/* there was a pop somewhere */
	carrying,		/* I am carrying */
	other1, other2,

	/* just a generic distress call */
	generic
};

/*
 * targets of messages
 */
enum	target_type	{
	none,
	planet,
	player
};

/* 
 * from bronco server2.7pl3 
 */
/* The General distress has format:

A 'E' will do byte1-8 inclusive. Macros can do more but haven't been
implemented that way yet.

   byte1: xxyzzzzz
        where zzzzz is dist_type, xx is target_type and y is 1 if this is
        a macro and not just a simple distress (a simple distress will ONLY
        send ship info like shields, armies, status, location, etc.)

   byte2: 1fff ffff - f = percentage fuel remaining (0-100)
   byte3: 1ddd dddd - % damage
   byte4: 1sss ssss - % shields remaining
   byte5: 1eee eeee - % etemp
   byte6: 1www wwww - % wtemp
   byte7: 100a aaaa - armies carried
   byte8: (lsb of me->p_status) & 0x80
   byte9: 1ppp pppp - planet closest to me
   byte10: 1eee eeee - enemy closest to me

   Byte 11 and on are only sent if y (from byte 1) is = 1
        although even for simplest case (y=0) we should send byte14+ = 0x80
        and then a null so that in future we can send a cc list and 
	pre/append text
   byte11: 1ttt tttt - target (either player number or planet number)
   byte12: 1ppp pppp - planet closest to target
   byte13: 1eee eeee - enemy closest to target
   byte14+: cc list (each player to cc this message to is 11pp ppp)
        cc list is terminated by 1000 0000 or 0100 0000 )
                                 pre-pend     append
   byte15++: the text to pre or append .. depending on termination above.
             text is null terminated and the last thing in this distress
*/

struct command_handler {
    char *command;
    int tag;
    char *desc;
    int (*handler)();
};

#ifdef VOTING
struct vote_handler {
    char *type;
    int tag;
    int minpass;
    int start;
    char *desc;
    int frequency;
    int (*handler)();
};

#define VC_ALL     0x0001   /* Majority Vote */
#define VC_TEAM    0x0002   /* Team Vote     */
#define VC_GLOG    0x0010   /* Write Votes to God Log */
#define VC_PLAYER  0x0020   /* Each player can be voted on, like eject */


#endif

/*-------------------------SOME CLIENT STRUCTURES--------------------------*/

 /* used to tell what planet or player mouse is pointing to */
struct obtype {
    int     o_type;		/* mouse pointing to a player or planet */
    int     o_num;		/* the player or planet number */
};

 /* used in the o_type structure */
#define PLANETTYPE 1		/* mouse on planet */
#define PLAYERTYPE 2		/* mouse on player */


 /* used to hold a players rank */
struct rank {
    int     genocides;		/* minimum number of genocides */
    float   di;			/* minimum destruction inflicted */
    float   battle;		/* minimum battle ratings */
    float   strategy;		/* minimum strategy ratings */
    float   specship;		/* minimum total ratings in a specialty */
    /* ship  SB + WB + JS */
    char   *name;		/* name of this rank */
};


struct royalty {
    char   *name;		/* name of rank */
};

/*-------------------------------------------------------------------------*/



#endif /* STRUCT_H */

/*---------END OF FILE---------*/
