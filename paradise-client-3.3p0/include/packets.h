/*--------------------------------------------------------------------------
NETREK II -- Paradise 2.1                    FILE: packets.h

Permission to use, copy, modify, and distribute this software and its
documentation for any NON-COMMERCIAL purpose (following the terms of
the GNU General Public License (read the file 'COPYING')) and without
fee is hereby granted, provided that this copyright notice appear in all
copies.  No representations are made about the suitability of this
software for any purpose.  This software is provided "as is" without
express or implied warranty.

    Xtrek Copyright 1986                            Chris Guthrie
    Netrek (Xtrek II) Copyright 1989                Kevin P. Smith
                                                    Scott Silvey
    Paradise II (Netrek II) Copyright 1993          Larry Denys
                                                    Kurt Olsen
                                                    Brandon Gillespie
    Paradise II by:
       Larry Denys, Kurt Olsen, Brandon Gillespie, and Rob Forsman
    and:
       Heath Kehoe, Mike Lutz, Mike McGrath, Ted Hadley, and Mark Kolb
--------------------------------------------------------------------------*/

/*
 * Include file for socket I/O xtrek.
 *
 * Kevin P. Smith 1/29/89
 */

#ifndef PACKETS_H
#define PACKETS_H

#define STATUS_TOKEN	"\t@@@"	/* ATM */

/* the following typedefs allow portability to machines without the
   ubiquitous 32-bit architecture (KSR1, Cray, DEC Alpha) */

typedef unsigned int CARD32;
typedef unsigned short CARD16;
typedef unsigned char CARD8;

typedef int INT32;
typedef short INT16;
/* This can be replaced by __CHAR_UNSIGNED__; if __CHAR_UNSIGNED__ is 
   defined, the compiler *has* to have support for the 'signed' keyword,
   otherwise, how do you generate a signed char?  Note that this also takes
   care of _every_ case so INT8 will be correct no matter what. */
#ifdef __CHAR_UNSIGNED__
typedef signed char INT8;
#else
typedef char INT8;
#endif

/*
 * TCP and UDP use identical packet formats; the only difference is that,
 * when in UDP mode, all packets sent from server to client have a sequence
 * number appended to them.
 *
 * (note: ALL packets, whether sent on the TCP or UDP channel, will have
 * the sequence number.  Thus it's important that client & server agree on
 * when to switch.  This was done to keep critical and non-critical data
 * in sync.)
 */

/* the various pad members of the structures are used for explicit
   data alignment.  They do not contain any data.  All structures are
   aligned to 4 bytes.  If your compiler forces 8 byte alignment, you
   will not be adhering to the netrek protocol.
   */

/* packets sent from xtrek server to remote client */
#define SP_MESSAGE 	1
#define SP_PLAYER_INFO 	2	/* general player info not elsewhere */
#define SP_KILLS	3	/* # kills a player has */
#define SP_PLAYER	4	/* x,y for player */
#define SP_TORP_INFO	5	/* torp status */
#define SP_TORP		6	/* torp location */
#define SP_PHASER	7	/* phaser status and direction */
#define SP_PLASMA_INFO	8	/* player login information */
#define SP_PLASMA	9	/* like SP_TORP */
#define SP_WARNING	10	/* like SP_MESG */
#define SP_MOTD		11	/* line from .motd screen */
#define SP_YOU		12	/* info on you? */
#define SP_QUEUE	13	/* estimated loc in queue? */
#define SP_STATUS	14	/* galaxy status numbers */
#define SP_PLANET 	15	/* planet armies & facilities */
#define SP_PICKOK	16	/* your team & ship was accepted */
#define SP_LOGIN	17	/* login response */
#define SP_FLAGS	18	/* give flags for a player */
#define SP_MASK		19	/* tournament mode mask */
#define SP_PSTATUS	20	/* give status for a player */
#define SP_BADVERSION   21	/* invalid version number */
#define SP_HOSTILE	22	/* hostility settings for a player */
#define SP_STATS	23	/* a player's statistics */
#define SP_PL_LOGIN	24	/* new player logs in */
#define SP_RESERVED	25	/* for future use */
#define SP_PLANET_LOC	26	/* planet name, x, y */

#define SP_SCAN		27	/* scan packet */
#define SP_UDP_REPLY	28	/* notify client of UDP status */
#define SP_SEQUENCE	29	/* sequence # packet */
#define SP_SC_SEQUENCE	30	/* this trans is semi-critical info */
#define SP_RSA_KEY	31	/* RSA data packet */

#define SP_MOTD_PIC     32	/* motd bitmap pictures */
#define SP_STATS2	33	/* new stats packet */
#define SP_STATUS2	34	/* new status packet */
#define SP_PLANET2	35	/* new planet packet */
#define SP_NEW_MOTD     36	/* New MOTD info notification uses */
#define SP_THINGY	37	/* thingy location */
#define SP_THINGY_INFO	38	/* thingy status */
#define SP_SHIP_CAP	39	/* ship capabilities */

#define SP_S_REPLY      40	/* reply to send-short request */
#define SP_S_MESSAGE    41	/* var. Message Packet */
#define SP_S_WARNING    42	/* Warnings with 4  Bytes */
#define SP_S_YOU        43	/* hostile,armies,whydead,etc .. */
#define SP_S_YOU_SS     44	/* your ship status */
#define SP_S_PLAYER     45	/* variable length player packet */

#define SP_PING         46	/* ping packet */

#define SP_S_TORP       47	/* variable length torp packet */
#define SP_S_TORP_INFO  48	/* SP_S_TORP with TorpInfo */
#define SP_S_8_TORP     49	/* optimized SP_S_TORP */
#define SP_S_PLANET     50	/* see SP_PLANET */

/* variable length packets */
#define VPLAYER_SIZE    4
#define SHORTVERSION    11	/* other number blocks, like UDP Version */
#define OLDSHORTVERSION	10

#define SP_GPARAM	51	/* game params packet */

/* the following is a family of packets with the same type, but a
   discriminating subtype */
#define SP_PARADISE_EXT1	52
#define SP_PE1_MISSING_BITMAP	0
#define SP_PE1_NUM_MISSILES	1
/* end of packet 52 subtypes */
#define SP_TERRAIN2	53	/* Terrain packets */
#define SP_TERRAIN_INFO2 54	/* Terrain info */

/* SP2 */
#define SP_S_SEQUENCE	56
#define SP_S_PHASER	57
#define SP_S_KILLS	58
#define SP_S_STATS	59

/* feature_spacket, response to feature_cpacket requests. identical structures */
#define SP_FEATURE              60

/* special type tells us when to update the display on playback.
   Not sent or received, only placed in the recorder file */
#define REC_UPDATE 127

/* packets sent from remote client to xtrek server */
#define CP_MESSAGE      1	/* send a message */
#define CP_SPEED	2	/* set speed */
#define CP_DIRECTION	3	/* change direction */
#define CP_PHASER	4	/* phaser in a direction */
#define CP_PLASMA	5	/* plasma (in a direction) */
#define CP_TORP		6	/* fire torp in a direction */
#define CP_QUIT		7	/* self destruct */
#define CP_LOGIN	8	/* log in (name, password) */
#define CP_OUTFIT	9	/* outfit to new ship */
#define CP_WAR		10	/* change war status */
#define CP_PRACTR	11	/* create practice robot? */
#define CP_SHIELD	12	/* raise/lower sheilds */
#define CP_REPAIR	13	/* enter repair mode */
#define CP_ORBIT	14	/* orbit planet/starbase */
#define CP_PLANLOCK	15	/* lock on planet */
#define CP_PLAYLOCK	16	/* lock on player */
#define CP_BOMB		17	/* bomb a planet */
#define CP_BEAM		18	/* beam armies up/down */
#define CP_CLOAK	19	/* cloak on/off */
#define CP_DET_TORPS	20	/* detonate enemy torps */
#define CP_DET_MYTORP	21	/* detonate one of my torps */
#define CP_COPILOT	22	/* toggle copilot mode */
#define CP_REFIT	23	/* refit to different ship type */
#define CP_TRACTOR	24	/* tractor on/off */
#define CP_REPRESS	25	/* pressor on/off */
#define CP_COUP		26	/* coup home planet */
#define CP_SOCKET	27	/* new socket for reconnection */
#define CP_OPTIONS	28	/* send my options to be saved */
#define CP_BYE		29	/* I'm done! */
#define CP_DOCKPERM	30	/* set docking permissions */
#define CP_UPDATES	31	/* set number of usecs per update */
#define CP_RESETSTATS	32	/* reset my stats packet */
#define CP_RESERVED	33	/* for future use */

#define CP_SCAN		34	/* ATM: request for player scan */

#define CP_UDP_REQ	35	/* request UDP on/off */
#define CP_SEQUENCE	36	/* sequence # packet */
#define CP_RSA_KEY	37	/* request MOTD */
#define CP_ASK_MOTD	38	/* request MOTD */

#define CP_PING_RESPONSE 42	/* client response */

#define CP_S_REQ                43
#define CP_S_THRS               44
#define CP_S_MESSAGE    45	/* vari. Message Packet */
#define CP_S_RESERVED       46
#define CP_S_DUMMY      47

#define CP_FEATURE   60

#define SOCKVERSION 	4
#define UDPVERSION	10	/* changing this blocks other */
 /* versions */

struct packet_handler {
    void    (*handler) ();
};


/*
 * These are server --> client packets
 */

struct mesg_spacket {
    INT8    type;		/* SP_MESSAGE */
    CARD8   m_flags;
    CARD8   m_recpt;
    CARD8   m_from;
    char    mesg[80];
};

struct plyr_info_spacket {
    INT8    type;		/* SP_PLAYER_INFO */
    INT8    pnum;
    INT8    shiptype;
    INT8    team;
};

struct kills_spacket {
    INT8    type;		/* SP_KILLS */
    INT8    pnum;
    INT8    pad1;
    INT8    pad2;
    CARD32  kills;		/* where 1234=12.34 kills and 0=0.00 kills */
};

struct player_spacket {
    INT8    type;		/* SP_PLAYER */
    INT8    pnum;
    CARD8   dir;
    INT8    speed;
    INT32   x, y;
};

struct torp_info_spacket {
    INT8    type;		/* SP_TORP_INFO */
    INT8    war;
    INT8    status;		/* TFREE, TDET, etc... */
    INT8    pad1;		/* pad needed for cross cpu compatibility */
    INT16   tnum;
    INT16   pad2;
};

struct torp_spacket {
    INT8    type;		/* SP_TORP */
    CARD8   dir;
    INT16   tnum;
    INT32   x, y;
};

/* Shapes of thingys.  It would be best to add to the end of this list and
   try to coordinate your additions with other hackers. */
enum thingy_types {
    SHP_BLANK, SHP_MISSILE, SHP_BOOM, SHP_TORP, SHP_PLASMA, SHP_MINE,
    SHP_PBOOM, SHP_FIGHTER, SHP_WARP_BEACON, SHP_FBOOM, SHP_DBOOM
};

struct thingy_info_spacket {
    INT8    type;		/* SP_THINGY_INFO */
    INT8    war;
    INT16   shape;		/* a thingy_types */
    INT16   tnum;
    INT16   owner;
};

struct thingy_spacket {
    INT8    type;		/* SP_THINGY */
    CARD8   dir;
    INT16   tnum;
    INT32   x, y;
};

struct phaser_spacket {
    INT8    type;		/* SP_PHASER */
    INT8    pnum;
    INT8    status;		/* PH_HIT, etc... */
    CARD8   dir;
    INT32   x, y;
    INT32   target;
};

struct plasma_info_spacket {
    INT8    type;		/* SP_PLASMA_INFO */
    INT8    war;
    INT8    status;		/* TFREE, TDET, etc... */
    INT8    pad1;		/* pad needed for cross cpu compatibility */
    INT16   pnum;
    INT16   pad2;
};

struct plasma_spacket {
    INT8    type;		/* SP_PLASMA */
    INT8    pad1;
    INT16   pnum;
    INT32   x, y;
};

struct warning_spacket {
    INT8    type;		/* SP_WARNING */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
    char    mesg[80];
};

struct motd_spacket {
    INT8    type;		/* SP_MOTD */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
    char    line[80];
};

struct you_spacket {
    INT8    type;		/* SP_YOU */
    INT8    pnum;		/* Guy needs to know this... */
    INT8    hostile;
    INT8    swar;
    INT8    armies;
    INT8    tractor;		/* ATM - visible tractor (was pad1) */
    CARD8   pad2;
    CARD8   pad3;
    CARD32  flags;
    INT32   damage;
    INT32   shield;
    INT32   fuel;
    INT16   etemp;
    INT16   wtemp;
    INT16   whydead;
    INT16   whodead;
};

struct queue_spacket {
    INT8    type;		/* SP_QUEUE */
    INT8    pad1;
    INT16   pos;
};

struct status_spacket {
    INT8    type;		/* SP_STATUS */
    INT8    tourn;
    INT8    pad1;
    INT8    pad2;
    CARD32  armsbomb;
    CARD32  planets;
    CARD32  kills;
    CARD32  losses;
    CARD32  time;
    CARD32  timeprod;
};

struct planet_spacket {
    INT8    type;		/* SP_PLANET */
    INT8    pnum;
    INT8    owner;
    INT8    info;
    INT16   flags;
    INT16   pad2;
    INT32   armies;
};

/* terrain info for Paradise terrain */
/* 5/16/95 rpg */

struct terrain_info_packet2 {
    CARD8   type;		/* SP_TERRAIN_INFO2 */
    CARD8   pad;
    CARD16  pad2;
    CARD16  xdim;
    CARD16  ydim;
};

struct terrain_packet2 {
    CARD8   type;		/* SP_TERRAIN2 */
    CARD8   sequence;
    CARD8   total_pkts;
    CARD8   length;
    CARD8   terrain_type[128];	/* Ugh... this needs to be fixed 5/16/95 rpg */
 /* CARD16  terrain_alt1[128]; */
 /* CARD16  terrain_alt2[128]; */
};

struct pickok_spacket {
    INT8    type;		/* SP_PICKOK */
    INT8    state;
    INT8    pad2;
    INT8    pad3;
};

struct login_spacket {
    INT8    type;		/* SP_LOGIN */
    INT8    accept;		/* 1/0 */
    INT8    pad2;
    INT8    pad3;
    INT32   flags;
    char    keymap[96];
};

struct flags_spacket {
    INT8    type;		/* SP_FLAGS */
    INT8    pnum;		/* whose flags are they? */
    INT8    tractor;		/* ATM - visible tractors */
    INT8    pad2;
    CARD32  flags;
};

struct mask_spacket {
    INT8    type;		/* SP_MASK */
    INT8    mask;
    INT8    pad1;
    INT8    pad2;
};

struct pstatus_spacket {
    INT8    type;		/* SP_PSTATUS */
    INT8    pnum;
    INT8    status;
    INT8    pad1;
};

struct badversion_spacket {
    INT8    type;		/* SP_BADVERSION */
    INT8    why;
    INT8    pad2;
    INT8    pad3;
};

struct hostile_spacket {
    INT8    type;		/* SP_HOSTILE */
    INT8    pnum;
    INT8    war;
    INT8    hostile;
};

struct stats_spacket {
    INT8    type;		/* SP_STATS */
    INT8    pnum;
    INT8    pad1;
    INT8    pad2;
    INT32   tkills;		/* Tournament kills */
    INT32   tlosses;		/* Tournament losses */
    INT32   kills;		/* overall */
    INT32   losses;		/* overall */
    INT32   tticks;		/* ticks of tournament play time */
    INT32   tplanets;		/* Tournament planets */
    INT32   tarmies;		/* Tournament armies */
    INT32   sbkills;		/* Starbase kills */
    INT32   sblosses;		/* Starbase losses */
    INT32   armies;		/* non-tourn armies */
    INT32   planets;		/* non-tourn planets */
    INT32   maxkills;		/* max kills as player * 100 */
    INT32   sbmaxkills;		/* max kills as sb * 100 */
};

struct plyr_login_spacket {
    INT8    type;		/* SP_PL_LOGIN */
    INT8    pnum;
    INT8    rank;
    INT8    pad1;
    char    name[16];
    char    monitor[16];
    char    login[16];
};

struct reserved_spacket {
    INT8    type;		/* SP_RESERVED */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
    INT8    data[16];
};

struct planet_loc_spacket {
    INT8    type;		/* SP_PLANET_LOC */
    INT8    pnum;
    INT8    pad2;
    INT8    pad3;
    INT32   x;
    INT32   y;
    char    name[16];
};

struct scan_spacket {		/* ATM */
    INT8    type;		/* SP_SCAN */
    INT8    pnum;
    INT8    success;
    INT8    pad1;
    INT32   p_fuel;
    INT32   p_armies;
    INT32   p_shield;
    INT32   p_damage;
    INT32   p_etemp;
    INT32   p_wtemp;
};

struct udp_reply_spacket {	/* UDP */
    INT8    type;		/* SP_UDP_REPLY */
    INT8    reply;
    INT8    pad1;
    INT8    pad2;
    INT32   port;
};

struct sequence_spacket {	/* UDP */
    INT8    type;		/* SP_SEQUENCE */
    INT8    pad1;
    CARD16  sequence;
};
struct sc_sequence_spacket {	/* UDP */
    INT8    type;		/* SP_CP_SEQUENCE */
    INT8    pad1;
    CARD16  sequence;
};

/*
 * Game configuration.
 * KAO 1/23/93
 */

struct ship_cap_spacket {	/* Server configuration of client */
    INT8    type;		/* screw motd method */
    INT8    operation;		/* 0 = add/change a ship, 1 = remove a ship */
    INT16   s_type;		/* SP_SHIP_CAP */
    INT16   s_torpspeed;
    INT16   s_phaserrange;
    INT32   s_maxspeed;
    INT32   s_maxfuel;
    INT32   s_maxshield;
    INT32   s_maxdamage;
    INT32   s_maxwpntemp;
    INT32   s_maxegntemp;
    INT16   s_width;
    INT16   s_height;
    INT16   s_maxarmies;
    INT8    s_letter;
    INT8    s_armies;
    char    s_name[16];
    INT8    s_desig1;
    INT8    s_desig2;
    INT16   s_bitmap;
};

struct motd_pic_spacket {
    INT8    type;		/* SP_MOTD_PIC */
    INT8    pad1;
    INT16   x, y, page;
    INT16   width, height;
    INT8    bits[1016];
};


 /* This is used to send paradise style stats */
struct stats_spacket2 {
    INT8    type;		/* SP_STATS2 */
    INT8    pnum;
    INT8    pad1;
    INT8    pad2;

    INT32   genocides;		/* number of genocides participated in */
    INT32   maxkills;		/* max kills ever * 100  */
    INT32   di;			/* destruction inflicted for all time * 100 */
    INT32   kills;		/* Kills in tournament play */
    INT32   losses;		/* Losses in tournament play */
    INT32   armsbomb;		/* Tournament armies bombed */
    INT32   resbomb;		/* resources bombed off */
    INT32   dooshes;		/* armies killed while being carried */
    INT32   planets;		/* Tournament planets conquered */
    INT32   tticks;		/* Tournament ticks */
    /* SB/WB/JS stats are entirely separate */
    INT32   sbkills;		/* Kills as starbase */
    INT32   sblosses;		/* Losses as starbase */
    INT32   sbticks;		/* Time as starbase */
    INT32   sbmaxkills;		/* Max kills as starbase * 100 */
    INT32   wbkills;		/* Kills as warbase */
    INT32   wblosses;		/* Losses as warbase */
    INT32   wbticks;		/* Time as warbase */
    INT32   wbmaxkills;		/* Max kills as warbase * 100 */
    INT32   jsplanets;		/* planets assisted with in JS */
    INT32   jsticks;		/* ticks played as a JS */
    INT32   rank;		/* Ranking of the player */
    INT32   royal;		/* royaly, specialty, rank */
};

 /* status info for paradise stats */
struct status_spacket2 {
    INT8    type;		/* SP_STATUS2 */
    INT8    tourn;
    INT8    pad1;
    INT8    pad2;
    CARD32  dooshes;		/* total number of armies dooshed */
    CARD32  armsbomb;		/* all t-mode armies bombed */
    CARD32  resbomb;		/* resources bombed */
    CARD32  planets;		/* all t-mode planets taken */
    CARD32  kills;		/* all t-mode kills made */
    CARD32  losses;		/* all t-mode losses */
    CARD32  sbkills;		/* total kills in SB's */
    CARD32  sblosses;		/* total losses in Sb's */
    CARD32  sbtime;		/* total time in SB's */
    CARD32  wbkills;		/* kills in warbases */
    CARD32  wblosses;		/* losses in warbases */
    CARD32  wbtime;		/* total time played in wb's */
    CARD32  jsplanets;		/* total planets taken by jump ships */
    CARD32  jstime;		/* total time in a jump ship */
    CARD32  time;		/* t mode time in this game */
    CARD32  timeprod;		/* t-mode ship ticks--sort of like */
};


 /* planet info for a paradise planet */
struct planet_spacket2 {
    INT8    type;		/* SP_PLANET2 */
    INT8    pnum;		/* planet number */
    INT8    owner;		/* owner of the planet */
    INT8    info;		/* who has touched planet */
    INT32   flags;		/* planet's flags */
    INT32   timestamp;		/* timestamp for info on planet */
    INT32   armies;		/* armies on the planet */
};

struct obvious_packet {
    INT8    type;		/* SP_NEW_MOTD */
    INT8    pad1;		/* CP_ASK_MOTD */
};

struct rsa_key_spacket {
    INT8    type;		/* SP_RSA_KEY */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
    CARD8   data[KEY_SIZE];
};


struct ping_spacket {
    INT8    type;		/* SP_PING */
    CARD8   number;		/* id (ok to wrap) */
    CARD16  lag;		/* delay of last ping in ms */

    CARD8   tloss_sc;		/* total loss server-client 0-100% */
    CARD8   tloss_cs;		/* total loss client-server 0-100% */

    CARD8   iloss_sc;		/* inc. loss server-client 0-100% */
    CARD8   iloss_cs;		/* inc. loss client-server 0-100% */
};

struct paradiseext1_spacket {
    INT8    type;
    CARD8   subtype;
    INT16   pad;
};

struct pe1_missing_bitmap_spacket {
    INT8    type;
    CARD8   subtype;

    INT16   page;

    INT16   x, y;
    INT16   width, height;
};

struct pe1_num_missiles_spacket {
    INT8    type;		/* SP_PARADISE_EXT1 */
    CARD8   subtype;		/* SP_PE1_NUM_MISSILES */

    INT16   num;		/* number of missiles */
};

/*
 * These are the client --> server packets
 */

struct mesg_cpacket {
    INT8    type;		/* CP_MESSAGE */
    INT8    group;
    INT8    indiv;
    INT8    pad1;
    char    mesg[80];
};

struct speed_cpacket {
    INT8    type;		/* CP_SPEED */
    INT8    speed;
    INT8    pad1;
    INT8    pad2;
};

struct dir_cpacket {
    INT8    type;		/* CP_DIRECTION */
    CARD8   dir;
    INT8    pad1;
    INT8    pad2;
};

struct phaser_cpacket {
    INT8    type;		/* CP_PHASER */
    CARD8   dir;
    INT8    pad1;
    INT8    pad2;
};

struct plasma_cpacket {
    INT8    type;		/* CP_PLASMA */
    CARD8   dir;
    INT8    pad1;
    INT8    pad2;
};

struct torp_cpacket {
    INT8    type;		/* CP_TORP */
    CARD8   dir;		/* direction to fire torp */
    INT8    pad1;
    INT8    pad2;
};

struct quit_cpacket {
    INT8    type;		/* CP_QUIT */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
};

struct login_cpacket {
    INT8    type;		/* CP_LOGIN */
    INT8    query;
    INT8    pad2;
    INT8    pad3;
    char    name[16];
    char    password[16];
    char    login[16];
};

struct outfit_cpacket {
    INT8    type;		/* CP_OUTFIT */
    INT8    team;
    INT8    ship;
    INT8    pad1;
};

struct war_cpacket {
    INT8    type;		/* CP_WAR */
    INT8    newmask;
    INT8    pad1;
    INT8    pad2;
};

struct practr_cpacket {
    INT8    type;		/* CP_PRACTR */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
};

struct shield_cpacket {
    INT8    type;		/* CP_SHIELD */
    INT8    state;		/* up/down */
    INT8    pad1;
    INT8    pad2;
};

struct repair_cpacket {
    INT8    type;		/* CP_REPAIR */
    INT8    state;		/* on/off */
    INT8    pad1;
    INT8    pad2;
};

struct orbit_cpacket {
    INT8    type;		/* CP_ORBIT */
    INT8    state;		/* on/off */
    INT8    pad1;
    INT8    pad2;
};

struct planlock_cpacket {
    INT8    type;		/* CP_PLANLOCK */
    INT8    pnum;
    INT8    pad1;
    INT8    pad2;
};

struct playlock_cpacket {
    INT8    type;		/* CP_PLAYLOCK */
    INT8    pnum;
    INT8    pad1;
    INT8    pad2;
};

struct bomb_cpacket {
    INT8    type;		/* CP_BOMB */
    INT8    state;
    INT8    pad1;
    INT8    pad2;
};

struct beam_cpacket {
    INT8    type;		/* CP_BEAM */
    INT8    state;
    INT8    pad1;
    INT8    pad2;
};

struct cloak_cpacket {
    INT8    type;		/* CP_CLOAK */
    INT8    state;
    INT8    pad1;
    INT8    pad2;
};

struct det_torps_cpacket {
    INT8    type;		/* CP_DET_TORPS */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
};

struct det_mytorp_cpacket {
    INT8    type;		/* CP_DET_MYTORP */
    INT8    pad1;
    INT16   tnum;
};

struct copilot_cpacket {
    INT8    type;		/* CP_COPLIOT */
    INT8    state;
    INT8    pad1;
    INT8    pad2;
};

struct refit_cpacket {
    INT8    type;		/* CP_REFIT */
    INT8    ship;
    INT8    pad1;
    INT8    pad2;
};

struct tractor_cpacket {
    INT8    type;		/* CP_TRACTOR */
    INT8    state;
    INT8    pnum;
    INT8    pad2;
};

struct repress_cpacket {
    INT8    type;		/* CP_REPRESS */
    INT8    state;
    INT8    pnum;
    INT8    pad2;
};

struct coup_cpacket {
    INT8    type;		/* CP_COUP */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
};

struct socket_cpacket {
    INT8    type;		/* CP_SOCKET */
    INT8    version;
    INT8    udp_version;	/* was pad2 */
    INT8    pad3;
    CARD32  socket;
};

struct options_cpacket {
    INT8    type;		/* CP_OPTIONS */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
    CARD32  flags;
    INT8    keymap[96];
};

struct bye_cpacket {
    INT8    type;		/* CP_BYE */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
};

struct dockperm_cpacket {
    INT8    type;		/* CP_DOCKPERM */
    INT8    state;
    INT8    pad2;
    INT8    pad3;
};

struct updates_cpacket {
    INT8    type;		/* CP_UPDATES */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
    CARD32  usecs;
};

struct resetstats_cpacket {
    INT8    type;		/* CP_RESETSTATS */
    INT8    verify;		/* 'Y' - just to make sure he meant it */
    INT8    pad2;
    INT8    pad3;
};

struct reserved_cpacket {
    INT8    type;		/* CP_RESERVED */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
    INT8    data[16];
    INT8    resp[16];
};

struct scan_cpacket {		/* ATM */
    INT8    type;		/* CP_SCAN */
    INT8    pnum;
    INT8    pad1;
    INT8    pad2;
};

struct udp_req_cpacket {	/* UDP */
    INT8    type;		/* CP_UDP_REQ */
    INT8    request;
    INT8    connmode;		/* respond with port # or just send UDP
				   packet? */
    INT8    pad2;
    INT32   port;		/* compensate for hosed recvfrom() */
};

struct sequence_cpacket {	/* UDP */
    INT8    type;		/* CP_SEQUENCE */
    INT8    pad1;
    CARD16  sequence;
};

struct rsa_key_cpacket {
    INT8    type;		/* CP_RSA_KEY */
    INT8    pad1;
    INT8    pad2;
    INT8    pad3;
    CARD8   global[KEY_SIZE];
    CARD8   public[KEY_SIZE];
    CARD8   resp[KEY_SIZE];
};

/* the CP_ASK_MOTD packet is the same as temp_spacket */

struct ping_cpacket {
    INT8    type;		/* CP_PING_RESPONSE */
    CARD8   number;		/* id */
    INT8    pingme;		/* if client wants server to ping */
    INT8    pad1;

    INT32   cp_sent;		/* # packets sent to server */
    INT32   cp_recv;		/* # packets recv from server */
};


/* feature_cpacket and _spacket are identical! */
struct feature_cpacket {	/* CP_FEATURE */
    INT8    type;
    char    feature_type;	/* either 'C' or 'S' */
    CARD8   arg1;               /* could be INT8 depending on feature */
    CARD8   arg2;               /* but only BEEP_LITE uses it, for now */
    INT32   value;
    char    name[80];
};

struct feature_spacket {	/* SP_FEATURE */
    INT8    type;
    char    feature_type;	/* either 'C' or 'S' */
    CARD8   arg1;
    CARD8   arg2;
    INT32    value;
    char    name[80];
};

/*
 * short stuff
 */

struct shortreq_cpacket {	/* CP_S_REQ */
    INT8    type;
    INT8    req;
    INT8    version;
    INT8    pad2;
};

struct threshold_cpacket {	/* CP_S_THRS */
    INT8    type;
    INT8    pad1;
    CARD16  thresh;
};

struct shortreply_spacket {	/* SP_S_REPLY */
    INT8    type;
    INT8    repl;
    CARD16  winside;
    INT32   gwidth;
};

struct youshort_spacket {	/* SP_S_YOU */
    INT8    type;

    INT8    pnum;
    INT8    hostile;
    INT8    swar;

    INT8    armies;
    INT8    whydead;
    INT8    whodead;

    INT8    pad1;

    CARD32  flags;
};

struct youss_spacket {		/* SP_S_YOU_SS */
    INT8    type;
    INT8    pad1;

    CARD16  damage;
    CARD16  shield;
    CARD16  fuel;
    CARD16  etemp;
    CARD16  wtemp;
};

#define VPLANET_SIZE 6

struct planet_s_spacket {	/* body of SP_S_PLANET  */
    INT8    pnum;
    INT8    owner;
    INT8    info;
    CARD8   armies;		/* more than 255 Armies ? ...  */
    INT16   flags;
};
struct warning_s_spacket {	/* SP_S_WARNING */
    INT8    type;
    CARD8   whichmessage;
    INT8    argument, argument2;/* for phaser  etc ... */
};

struct player_s_spacket {
    INT8    type;		/* SP_S_PLAYER Header */
    INT8    packets;		/* How many player-packets are in this packet
				   ( only the first 6 bits are relevant ) */
    CARD8   dir;
    INT8    speed;
    INT32   x, y;		/* To get the absolute Position */
};

struct player_s2_spacket
  {
    char    type;                                /* SP_S_PLAYER Header */
    char    packets;                             /* How many player-packets *
                                                  *
                                                  * * are in this packet  ( *
                                                  * * only the firs t 6 bits *
                                                  * * are relevant ) */
    unsigned char dir;
    char    speed;
    short   x, y;                                /* absolute position / 40 */
    unsigned int flags;                          /* 16 playerflags */
  };


/* The format of the body:
struct player_s_body_spacket {	Body of new Player Packet
	CARD8 pnum;	 0-4 = pnum, 5 local or galactic, 6 = 9. x-bit, 7 9. y-bit
	CARD8 speeddir;	 0-3 = speed , 4-7 direction of ship
	CARD8 x;	 low 8 bits from X-Pixelcoordinate
	CARD8 y;	 low 8 bits from Y-Pixelcoordinate
};
*/

struct torp_s_spacket {
    INT8    type;		/* SP_S_TORP */
    CARD8   bitset;		/* bit=1 that torp is in packet */
    CARD8   whichtorps;		/* Torpnumber of first torp / 8 */
    CARD8   data[21];		/* For every torp 2*9 bit coordinates */
};

struct mesg_s_spacket {
    INT8    type;		/* SP_S_MESSAGE */
    CARD8   m_flags;
    CARD8   m_recpt;
    CARD8   m_from;
    CARD8   length;		/* Length of whole packet */
    INT8    mesg;
    INT8    pad2;
    INT8    pad3;
    INT8    pad[76];
};

struct mesg_s_cpacket {
    INT8    type;		/* CP_S__MESSAGE */
    INT8    group;
    INT8    indiv;
    INT8    length;		/* Size of whole packet   */
    INT8    mesg[80];
};

struct kills_s_spacket
  {
    char    type;                                /* SP_S_KILLS */
    char    pnum;                                /* How many kills in packet */
    unsigned short kills;                        /* 6 bit player numer   */
    /* 10 bit kills*100     */
    unsigned short mkills[32];
    	/* NOTE: this must be identical to MAXPLAYER in Vanilla server */
  };

struct phaser_s_spacket
  {
    char    type;                                /* SP_S_PHASER */
    char    status;                              /* PH_HIT, etc... */
    unsigned char pnum;                          /* both bytes are used for *
                                                  *
                                                  * * more */
    unsigned char target;                        /* look into the code   */
    short   x;                                   /* x coord /40 */
    short   y;                                   /* y coord /40 */
    unsigned char dir;
    char    pad1;
    char    pad2;
    char    pad3;
  };

struct stats_s_spacket
  {
    char    type;                                /* SP_S_STATS */
    char    pnum;
    unsigned short tplanets;                     /* Tournament planets */
    unsigned short tkills;                       /* Tournament kills */
    unsigned short tlosses;                      /* Tournament losses */
    unsigned short kills;                        /* overall */
    unsigned short losses;                       /* overall */
    unsigned int tticks;                         /* ticks of tournament play
                                                  * * * time */
    unsigned int tarmies;                        /* Tournament armies */
    unsigned int maxkills;
    unsigned short sbkills;                      /* Starbase kills */
    unsigned short sblosses;                     /* Starbase losses */
    unsigned short armies;                       /* non-tourn armies */
    unsigned short planets;                      /* non-tourn planets */
    unsigned int sbmaxkills;                     /* max kills as sb * 100 */
  };


#endif
