/*
 * struct.h for the client of an xtrek socket protocol.
 *
 * Most of the unneeded stuff in the structures has been thrown away.
 */

#ifndef STRUCT_H
#define STRUCT_H

#include "copyright.h"
#include "Wlib.h"
#include "defs.h"

struct point {
  int x;
  int y;
};

/* hockey struct [BDyess] */
struct hockeyLine {
  int vertical;		/* vertical or horizontal flag */
  W_Color color;
  int pos, end1, end2;	/* x or y constant and two endpoints */
};

/* struct for keeping tractor line data [BDyess] */
typedef struct tractor { 
  int     sx, sy, d1x, d1y, d2x, d2y; /* source (x,y) dest (x1,y1) (x2,y2) */
  struct tractor *next;
} Tractor;
	                                                                      
/* struct for identifying a unique planet image [BDyess] */
typedef struct {
   unsigned int b_scouted:1;		/* is the planet scouted? */
   unsigned int b_facilities:4;		/* facilities */
   unsigned int b_resources:3;		/* resources */
   unsigned int b_surface:2;		/* surface properties */
   unsigned int b_age:3;		/* planet scout info age */
   unsigned int b_team:4;		/* owning team */
   unsigned int b_type:3;		/* type (planet, asteroid, etc */
   unsigned int b_layers;		/* sets of 3-bit image layers */
} bitstruct;

enum { B_FACILITIES=1, B_RESOURCES, B_SURFACE, B_TEAM, B_AGE };

/* binary tree structure for planet imagelist caching and lookup [BDyess] */
typedef struct _PlanetImageNode {
  bitstruct bits;		/* significant planet status bits */
  W_Image *image;		/* image that the bits represent */
  struct _PlanetImageNode *left, *right;  /* left and right nodes */
} PlanetImageNode;

/* ratings struct [BDyess] */
struct ratings {
    float   r_offrat;		/* offense rating */
    float   r_planetrat;	/* planets rating */
    float   r_bombrat;		/* bombing rating */
    float   r_defrat;		/* defense rating */
    float   r_resrat;		/* resource rating */
    float   r_dooshrat;		/* doosh rating */
    float   r_stratrat;		/* strategy rating */
    float   r_batrat;		/* battle rating */
    float   r_sbrat;		/* sb rating */
    float   r_wbrat;		/* wb rating */
    float   r_jsrat;		/* js rating */
    int     r_jsplanets;	/* js planets */
    int     r_resources;	/* total resources bombed */
    int     r_armies;		/* total armies bombed */
    int     r_planets;		/* total planets taken */
    int     r_dooshes;		/* total armies dooshed */
    float   r_specrat;		/* special ship rating */
    float   r_di;		/* damage inflicted */
    float   r_ratio;		/* ratio, kills/losses */
    int     r_kills;		/* kills */
    int     r_losses;		/* losses */
    float   r_ratings;		/* total ratings */
    float   r_killsPerHour;	/* kills/hour */
    float   r_lossesPerHour;	/* losses/hour */
    float   r_maxkills;		/* max kills */
    int     r_genocides;	/* number of genocides */
};

/* messageWindow structure [BDyess] */
struct messageWin {
    W_Window window;
    int     flags;
    struct messageNode *head, *curHead;
};

/* stuff yanked from COW-lite for rc_distress [BDyess] */
struct distress {
    unsigned char sender;
    unsigned char dam, shld, arms, wtmp, etmp, fuelp, sts;
    unsigned char wtmpflag, etempflag, cloakflag, distype, macroflag;
    unsigned char close_pl, close_en, tclose_pl, tclose_en, pre_app, i;
    unsigned char close_j, close_fr, tclose_j, tclose_fr;
    unsigned char cclist[6];	/* allow us some day to cc a message up to 5
				   people */
    /* sending this to the server allows the server to do the cc action */
    /* otherwise it would have to be the client ... less BW this way */
    char    preappend[80];	/* text which we pre or append */
};

enum dist_type {
    /* help me do series */
    take = 1, ogg, bomb, space_control,
    save_planet,
    base_ogg,
    help3, help4,

    /* doing series */
    escorting, ogging, bombing, controlling,
    asw,
    asbomb,
    doing3, doing4,

    /* other info series */
    free_beer,			/* ie. player x is totally hosed now */
    no_gas,			/* ie. player x has no gas */
    crippled,			/* ie. player x is way hurt but may have gas */
    pickup,			/* player x picked up armies */
    pop,			/* there was a pop somewhere */
    carrying,			/* I am carrying */
    other1, other2,

    /* just a generic distress call */
    generic
};

/* The General distress has format:

   byte1: 00yzzzzz
   where zzzzz is dist_type, and y is 1 if this is a more complicated macro
   and not just a simple distress (a simple distress will ONLY send ship
   info like shields, armies, status, location, etc.). I guess y=1 can be for !
   future expansion.

   byte2: 1fff ffff - f = percentage fuel remaining (0-100)
   byte3: 1ddd dddd - % damage
   byte4: 1sss ssss - % shields remaining
   byte5: 1eee eeee - % etemp
   byte6: 1www wwww - % wtemp
   byte7: 100a aaaa - armies carried
   byte8: (lsb of me->p_status) & 0x80
   byte9: 1ppp pppp - planet closest to me
   byte10: 1eee eeee - enemy closest to me
   byte11: 1ppp pppp - planet closest to target
   byte12: 1eee eeee - enemy closest to target
   byte13: 1ttt tttt - tclose_j
   byte14: 1jjj jjjj - close_j
   byte15: 1fff ffff - tclose_fr
   byte16: 1ccc cccc - close_fr
   byte17+: cc list (each player to cc this message to is 11pp ppp)
   cc list is terminated by 0x80 (pre-pend) or 0100 0000 (append) )
   byte18++: the text to pre or append .. depending on termination above.
   text is null terminated and the last thing in this distress
 */

struct macro_list {
    int     type;
    char    key;
    char    who;
    char   *string;
};

struct dmacro_list {
    unsigned char c;
    char   *name;
    char   *macro;
};
/* end rc_distress stuff [BDyess] */

struct status {
    unsigned char tourn;	/* Tournament mode? */
    /* These stats only updated during tournament mode */
    unsigned int armsbomb, planets, kills, losses, time;
    /* Use long for this, so it never wraps */
    unsigned long timeprod;
};

struct status2 {		/* paradise status struct */
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
    int     gameup;		/* is game up */
    unsigned long clock;	/* clock for planet info timestamp */
};


/* metaserver window struct */
struct servers {
    char    address[LINE];
    int     port;
    int     time;
    int     players;
    int     status;
    int     RSA_client;
    char    typeflag;
    char    hilited;
};

/* MOTD structures */
struct piclist {
    int     page;
    W_Image *thepic;
    int     x, y;
    int     width, height;
    struct piclist *next;
};
struct page {
    struct list *text;
    struct page *next;
    struct page *prev;
    int     first;
    int     page;
};

#define PFREE 0
#define POUTFIT 1
#define PALIVE 2
#define PEXPLODE 3
#define PDEAD 4
#define PTQUEUE 5
#define POBSERVE 6

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
#define PFWARPPREP	  (1<<26)	/* in warp prep [BDyess] */
#define PFWARP		  (1<<27)	/* ship warping */
#define PFAFTER		  (1<<28)	/* after burners on */
#define PFWPSUSPENDED     (1<<29)	/* warp prep suspended [BDyess] */
#define PFSNAKE	          (1<<30)	/* it's a space snake */
#define PFBIRD	          (1<<31)	/* it's a space bird */

enum why_dead {
    KNOREASON, KQUIT, KTORP, KPHASER, KPLANET,
    KSHIP, KDAEMON, KWINNER, KGHOST, KGENOCIDE,
    KPROVIDENCE, KPLASMA, KTOURNEND, KOVER, KTOURNSTART,
    KBADBIN, KMISSILE, KASTEROID
};

#define DEFAULT -1
#define SCOUT 0
#define DESTROYER 1
#define CRUISER 2
#define BATTLESHIP 3
#define ASSAULT 4
#define STARBASE 5
#define ATT 6
#define GALAXY 6		/* galaxy ships now supported - they look
				   extremely similar to flagships :) [BDyess] */
#define JUMPSHIP 7
#define FLAGSHIP 8
#define WARBASE 9
#define LIGHTCRUISER 10
#define CARRIER 11
#define UTILITY 12
#define PATROL 13
#define PUCK 14
/*#define NUM_TYPES 14*/

struct shiplist {
    struct ship *ship;
    struct shiplist *prev, *next;
};

struct ship {
    int     s_phaserrange;
    int     s_maxspeed;
    int     s_maxfuel;
    int     s_maxshield;
    int     s_maxdamage;
    int     s_maxegntemp;
    int     s_maxwpntemp;
    short   s_maxarmies;
    short   s_type;
    int     s_torpspeed;
    char    s_letter;
    char    s_armies;	/* new - gets army carrying cap. from server */
    /* char s_name[16]; */
    char    s_desig[2];
    short   s_bitmap;
    unsigned char s_keymap[256];
    unsigned char s_buttonmap[12];
};

struct stats {
    double  st_maxkills;	/* max kills ever */
    int     st_kills;		/* how many kills */
    int     st_losses;		/* times killed */
    int     st_armsbomb;	/* armies bombed */
    int     st_planets;		/* planets conquered */
    int     st_ticks;		/* Ticks I've been in game */
    int     st_tkills;		/* Kills in tournament play */
    int     st_tlosses;		/* Losses in tournament play */
    int     st_tarmsbomb;	/* Tournament armies bombed */
    int     st_tplanets;	/* Tournament planets conquered */
    int     st_tticks;		/* Tournament ticks */
    /* SB stats are entirely separate */
    int     st_sbkills;		/* Kills as starbase */
    int     st_sblosses;	/* Losses as starbase */
    int     st_sbticks;		/* Time as starbase */
    double  st_sbmaxkills;	/* Max kills as starbase */
    long    st_lastlogin;	/* Last time this player was played */
    int     st_flags;		/* Misc option flags */
#if 0
    unsigned char st_keymap[256];	/* keymap for this player */
#endif
    int     st_rank;		/* Ranking of the player */
};


struct stats2 {			/* paradise stats */
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
    unsigned char st_keymap[256];	/* keymap for this player */
    int     st_rank;		/* Ranking of the player */
    int     st_royal;		/* royaly, specialty, rank */
};

#define ST_NOBITMAPS	(1<<0)
#define ST_KEEPPEACE    (1<<3)

struct player {
    int     p_no;
    int     p_updates;		/* Number of updates ship has survived */
    int     p_status;		/* Player status */
    unsigned int p_flags;	/* Player flags */
    char    p_name[16];
    char    p_login[16];
    char    p_monitor[16];	/* Monitor being played on */
    char    p_mapchars[2];	/* Cache for map window image */
    struct ship *p_ship;	/* Personal ship statistics */
    int     p_x;
    int     p_y;
    unsigned char p_dir;	/* Real direction */
    unsigned char p_desdir;	/* desired direction */
    int     p_subdir;		/* fraction direction change */
    int     p_speed;		/* Real speed */
    short   p_desspeed;		/* Desired speed */
    int     p_subspeed;		/* Fractional speed */
    short   p_teami;		/* Team I'm on */
    int     p_damage;		/* Current damage */
    int     p_subdamage;	/* Fractional damage repair */
    int     p_shield;		/* Current shield power */
    int     p_subshield;	/* Fractional shield recharge */
    short   p_cloakphase;	/* Drawing stage of cloaking
				   engage/disengage. */
    short   p_ntorp;		/* Number of torps flying */
    short   p_ndrone;		/* Number of drones .. why was this missing? */
    short   p_totmissiles;	/* number of total missiles [Bill Dyess] */
    short   p_nplasmatorp;	/* Number of plasma torps active */
    char    p_hostile;		/* Who my torps will hurt */
    char    p_swar;		/* Who am I at sticky war with */
    float   p_kills;		/* Enemies killed */
    short   p_planet;		/* Planet orbiting or locked onto */
    short   p_playerl;		/* Player locked onto */
#ifdef ARMY_SLIDER
    int     p_armies;		/* XXX: for stats */
#else
    short   p_armies;
#endif				/* ARMY_SLIDER */
    int     p_fuel;
    short   p_explode;		/* Keeps track of final explosion */
    int     p_etemp;
    short   p_etime;
    int     p_wtemp;
    short   p_wtime;
    short   p_whydead;		/* Tells you why you died */
    short   p_whodead;		/* Tells you who killed you */
    struct stats p_stats;	/* player statistics */
    struct stats2 p_stats2;	/* Paradise stats */
    short   p_genoplanets;	/* planets taken since last genocide */
    short   p_genoarmsbomb;	/* armies bombed since last genocide */
    short   p_planets;		/* planets taken this game */
    short   p_armsbomb;		/* armies bombed this game */
    int     p_docked;		/* If starbase, # docked to, else pno base
				   host */
    int     p_port[4];		/* If starbase, pno of ship docked to that
				   port, else p_port[0] = port # docked to on
				   host.   */
    short   p_tractor;		/* What player is in tractor lock */
    int     p_pos;		/* My position in the player file */
};

struct statentry {
    char    name[16], password[16];
    struct stats stats;
};

/* Torpedo states */

#define TFREE 0
#define TMOVE 1
#define TEXPLODE 2
#define TDET 3
#define TOFF 4
#define TSTRAIGHT 5		/* Non-wobbling torp */

struct torp {
    int     t_no;
    int     t_status;		/* State information */
    int     t_owner;
    int     t_x;
    int     t_y;
    unsigned char t_dir;	/* direction */
    short   t_turns;		/* rate of change of direction if tracking */
    int     t_damage;		/* damage for direct hit */
    int     t_speed;		/* Moving speed */
    int     t_fuse;		/* Life left in current state */
    char    t_war;		/* enemies */
    char    t_team;		/* launching team */
    char    t_whodet;		/* who detonated... */
    char    frame;		/* frame of animation [BDyess] */
};

struct thingy {
    int     t_no;
    int     t_shape;		/* State information */
    int     t_owner;
    int     t_x;
    int     t_y;
    unsigned char t_dir;	/* direction */
    int     t_speed;		/* Moving speed */
    int     t_fuse;		/* Life left in current state */
    char    t_war;		/* enemies */
};

/* Plasma Torpedo states */

#define PTFREE 0
#define PTMOVE 1
#define PTEXPLODE 2
#define PTDET 3

struct plasmatorp {
    int     pt_no;
    int     pt_status;		/* State information */
    int     pt_owner;
    int     pt_x;
    int     pt_y;
    unsigned char pt_dir;	/* direction */
    short   pt_turns;		/* ticks turned per cycle */
    int     pt_damage;		/* damage for direct hit */
    int     pt_speed;		/* Moving speed */
    int     pt_fuse;		/* Life left in current state */
    char    pt_war;		/* enemies */
    char    pt_team;		/* launching team */
};

#define PHFREE 0x00
#define PHHIT  0x01		/* When it hits a person */
#define PHMISS 0x02
#define PHHIT2 0x04		/* When it hits a photon */
struct phaser {
    int     ph_status;		/* What it's up to */
    unsigned char ph_dir;	/* direction */
    int     ph_target;		/* Who's being hit (for drawing) */
    int     ph_x, ph_y;		/* For when it hits a torp */
    int     ph_fuse;		/* Life left for drawing */
    int     ph_damage;		/* Damage inflicted on victim */
};

/* An important note concerning planets:  The game assumes that
    the planets are in a 'known' order.  Ten planets per team,
    the first being the home planet.
*/

 /* defines for the pl_flags field of planet struct */

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
   bits 23,24			cosmic object type (also bit 16)
   bits 25..31	currently unallocated (7 bits to play with)

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

/* cosmic object types, bits 16 and 23, and 24 */
#define PLPLANET	0	/* object is a planet */
#define PLSTAR     (1<<16)	/* object is a star */
#define PLAST	   (1<<23)	/* object is an asteroid NYI */
#define PLNEB	   ((1<<16)|(1<<23))	/* object is a nebula NYI */
#define PLBHOLE    (1<<24)	/* object is a black hole NYI */
#define PLPULSAR   ((1<<16)|(1<<24))	/* object is a pulsar NYI */
#define PLUK1      ((1<<23)|(1<<24))	/* future expansion */
#define PLWHOLE    ((1<<16)|(1<<23)|(1<<24))	/* object is a wormhole */
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


struct planet {
    int     pl_no;
    int     pl_flags;		/* State information */
    int     pl_owner;
    int     pl_x;
    int     pl_y;
    char    pl_name[16];
    int     pl_namelen;		/* Cuts back on strlen's */
    int     pl_armies;
    int     pl_info;		/* Teams which have info on planets */
    int     pl_deadtime;	/* Time before planet will support life */
    int     pl_couptime;	/* Time before coup may take place */
    int     pl_timestamp;	/* time the info was taken */
};

struct t_unit {
/*  int	    alt1;*/
/*  int     alt2;*/		/* Terrain types. */
    char types;
};

struct _clearzone {
    int     x, y;
    int     width, height;
};

#define MVALID 0x01
#define MGOD   0x10
#define MMOO   0x12

#define MTOOLS 0x14

/* order flags by importance (0x100 - 0x400) */
/* restructuring of message flags to squeeze them all into 1 byte - jmn */
/* hopefully quasi-back-compatible:
   MVALID, MINDIV, MTEAM, MALL, MGOD use up 5 bits. this leaves us 3 bits.
   since the server only checks for those flags when deciding message
   related things and since each of the above cases only has 1 flag on at
   a time we can overlap the meanings of the flags */

#define MINDIV 0x02
/* these go with MINDIV flag */
#ifdef STDBG
#define MDBG   0x20
#endif
#define MCONFIG 0x40		/* config messages from server */
#define MDIST 0x60		/* flag distress messages - client thing
				   really but stick it in here for
				   consistency */

#define MTEAM  0x04
/* these go with MTEAM flag */
#define MTAKE  0x20
#define MDEST  0x40
#define MBOMB  0x60
#define MCOUP1 0x80
#define MCOUP2 0xA0
#define MDISTR 0xC0

#define MALL   0x08
#define MCAST  0x18		/* not an offical msg dest */
/* these go with MALL flag */
#define MGENO  0x20		/* MGENO is not used in INL server but
				   belongs here */
#define MCONQ  0x20		/* not enought bits to distinguish
				   MCONQ/MGENO :-( */
#define MKILLA 0x40
#define MKILLP 0x60
#define MKILL  0x80
#define MLEAVE 0xA0
#define MJOIN  0xC0
#define MGHOST 0xE0
/* MMASK not used in INL server */

/* to flag multi-line macros */
#define MMACRO 0x80

#define MWHOMSK  0x1f		/* mask with this to find who msg to */
#define MWHATMSK 0xe0		/* mask with this to find what message about */

struct message {
    int     m_no;
    int     m_flags;
    int     m_time;
    int     m_recpt;
    char    m_data[80];
};

/* message control structure */

struct mctl {
    int     mc_current;
};

/* This is a structure used for objects returned by mouse pointing */

#define PLANETTYPE 0x1
#define PLAYERTYPE 0x2

struct obtype {
    int     o_type;
    int     o_num;
};

struct id {
    char   *name;
    int     team;
    int     number;
    int     type;
    char    mapstring[4];
};

struct rank {
    float   hours, ratings, defense;
    char   *name;
};

struct rank2 {			/* Paradise ranks */
    int     genocides;		/* minimum number of genocides */
    float   di;			/* minimum destruction inflicted */
    float   battle;		/* minimum battle ratings */
    float   strategy;		/* minimum strategy ratings */
    float   specship;		/* minimum total ratings in a specialty */
    /* ship  SB + WB + JS */
    char   *name;		/* name of this rank */
};


struct royalty {		/* Paradise royalty ranks */
    char   *name;		/* name of rank */
};

struct plupdate {

    int     plu_update;
    int     plu_x, plu_y;
};

#define MACSINGLE 1
#define MACRCD 2
#define MACMULTI 4
struct macro {
    char    flags;		/* WAS isSingleMacro; now, uses above flags */
    char    to;			/* if to team, rom, etc put here or -1 */
    char    specialto;		/* player nearest mouse, etc here */
    struct macro *next;		/* for multi-line macros, points to a struct
				   *distress if MACRCD flag set. -JR */
    char   *string;		/* string to be sent, % escapes intact */
};

struct stringlist {
    char   *string;
    char   *value;
    struct stringlist *next, *prev;
    int searched;
};

#endif
