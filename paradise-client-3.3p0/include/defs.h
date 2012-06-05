/*
 * defs.h
 */

#ifndef DEFS_H
#define DEFS_H

#include "copyright.h"
#include "config.h"

#define MAXPLANETS 60		/* maximum planets any server will send us */
#define MAX_PLAYER 257		/* Maximum number of players we can configure
				   the game for, not the server's max
				   players.  */

/* defs for HUD warnings [BDyess] */
#define HUD_Y	4

/* defs for hockey [BDyess] */
#define NUM_HOCKEY_LINES 13

/* redefine EXIT as print totals followed by a normal exit [BDyess] */
#define EXIT(x) {print_totals();exit(x);}

/* defs for new message window data structure [BDyess] */
#define WREVIEW		0
#define WTEAM		1
#define WINDIV		2
#define WKILL		3
#define WPHASER		4
#define WALL		5
#define WNUM		6
/* message window allow mask */
#define WA_TEAM		1
#define WA_INDIV	2
#define WA_KILL		4
#define WA_PHASER	8
#define WA_ALL		16
#define WA_REVIEW	32
#define WA_MACRO        64

/* defs for updatePlayer [BDyess] */
#define NO_UPDATE	0
#define SMALL_UPDATE	1	/* update non-blk_bozo players */
#define LARGE_UPDATE	2	/* update blk_bozo players     */
#define ALL_UPDATE	(SMALL_UPDATE|LARGE_UPDATE)	/* update both */

/* defs for terrain */
#define TERRAIN_STARTED 1
#define TERRAIN_DONE    2

/* defs for timer [BDyess] */
#define T_NONE		0	/* no timer */
#define T_DAY		1	/* time of day */
#define T_SERVER	2	/* time on server */
#define T_SHIP		3	/* time in ship */
#define T_USER		4	/* user reset timer */
#define T_TOTAL		5	/* number of T_ defs */
#define TIMESTRLEN	10	/* used in db_timer(), timeString() */

/* defs for mapmode */
#define GMAP_NEVER	0
#define GMAP_FREQUENT 	1
#define GMAP_INFREQUENT	2

/* metaserver window defs */
#define LINE 80
#define METASERVERADDRESS "metaserver.netrek.org"	/* new metaserver */
#define METAPORT 3521		/* HAVE to use nicely formated version */

#define MAX_PLANETS 257

#define WINSIDE 500	/* Size of strategic and tactical windows */
#define MAPSIDE 500
#define BORDER 2		/* border width for option windows */
#define WIN_EDGE 4		/* border on l/r edges of text windows */
#define MENU_PAD 4		/* border on t/b edges of text windows */
#define MENU_BAR 2		/* width of menu bar */

#define PSEUDOSIZE 16
#if 0
#define CLOAK_PHASES 7		/* number of drawing phases in a cloak
				   engage/disengage */
#endif /*0*/
#define NUMRANKS 9		/* old netrek ranks */
/*#define NUMRANKS2 18*/

/* These are configuration definitions */

#define GWIDTH 200000		/* galaxy is 200000 spaces on a side */
#define SCALE 40		/* Window will be one pixel for 20 spaces */
#define WARP1 20		/* warp one will move 20 spaces per update */
#define EXPDIST 350		/* At this range a torp will explode */
#define GRIDSIZE 33333

#define DETDIST 1600		/* At this range a player can detonate a torp */

#define PHASEDIST 6000		/* At this range a player can do damage with
				   phasers */
#define ENTORBDIST 900		/* At this range a player can orbit a planet */
#define ORBDIST 800		/* A player will orbit at this radius */
#define ORBSPEED 2		/* This is the fastest a person can go into
				   orbit */
#define PFIREDIST 1500		/* At this range a planet will shoot at a
				   player */
#define UPDATE 100000		/* Update time is 100000 micro-seconds */

#define AUTOQUIT 60		/* auto logout in 60 secs */

#define VACANT -1		/* indicates vacant port on a starbase */
#define DOCKDIST 600
#define DOCKSPEED 2		/* If base is moving, there will be some
				   finesse involved to dock */
#define NUMPORTS 4
#define SBFUELMIN 10000		/* If starbase's fuel is less than this, it
				   will not refuel docked vessels */
#define TRACTDIST   6000	/* maximum effective tractor beam range */
#define TRACTEHEAT  5		/* ammount tractor beams heat engines */
#define TRACTCOST   20		/* fuel cost of activated tractor beam */

/* These are memory sections */
#define PLAYER 1
#define MAXMESSAGE 50
#define MAXREVIEWMESSAGE 20

#define rosette(x, ndiv)   (( (((x)&0xff) + 0x100/(2*(ndiv))) * (ndiv)/0x100 ) % ndiv)

/* These are the teams */
/* Note that I used bit types for these mostly for messages and
   war status.  This was probably a mistake.  It meant that Ed
   had to add the 'remap' area to map these (which are used throughout
   the code as the proper team variable) into a nice four team deep
   array for his color stuff.  Oh well.
*/
#define INDi -1
#define FEDi 0
#define ROMi 1
#define KLIi 2
#define ORIi 3
 /* #define ALLTEAMi 4 *//* replaced by number_of_teams */
#define NOBODY 0x0
#define FEDm 0x1
#define ROMm 0x2
#define KLIm 0x4
#define ORIm 0x8
#define ALLTEAM ( (1<<number_of_teams) - 1)
 /*#define MAXTEAM 3 *//* number_of_teams -1 */
 /*#define NUMTEAM 4 *//* number_of_teams */
/*
** These are random configuration variables
*/
#define VICTORY 3		/* Number of systems needed to conquer the
				   galaxy */
#define WARNTIME 30		/* Number of updates to have a warning on the
				   screen */
#define MESSTIME 30		/* Number of updates to have a message on the
				   screen */

/* Flags for gettarget */
#define TARG_SHIP	(1<<0)
#define TARG_BASE	(1<<1)
#define TARG_PLANET	(1<<2)
#define TARG_CLOAK	(1<<3)	/* Include cloaked ships in search */
#define TARG_SELF	(1<<4)
#define TARG_ENEMY	(1<<5)	/* enemy ships/planets only */
#define TARG_FRIENDLY   (1<<6)	/* friendly ships/planets only */
#define TARG_TEAM	(1<<7)	/* same team */
#define TARG_STAR	(1<<8)
#define TARG_NEBULA	(1<<9)
#define TARG_BLACKHOLE  (1<<10)

#define TARG_PLAYER	(TARG_SHIP|TARG_BASE)
#define TARG_ASTRAL	(TARG_PLANET|TARG_STAR|TARG_NEBULA|TARG_BLACKHOLE)

#define DEFAULT_SERVER	"tanya.ucsd.edu"
#define DEFAULT_PORT	2592

#define ABS(a)			/* abs(a) */ (((a) < 0) ? -(a) : (a))
#ifndef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#endif

/* translates a server coord. system point into a local window coord. system
   point [BDyess] */
#define scaleLocal(pt)		((pt) / scale + center)
#define unScaleLocal(pt)	(((pt) - center) * scale)
/* same for server -> galactic [BDyess] */
#define scaleMap(pt)		((pt) / mapscale)
#define scaleMapX(pt)		(((pt) - offsetx) / mapscale)
#define scaleMapY(pt)		(((pt) - offsety) / mapscale)
#define unScaleMapX(pt)		((pt) * mapscale + offsetx)
#define unScaleMapY(pt)		((pt) * mapscale + offsety)

#define myPlasmaTorp(t)		(me->p_no == (t)->pt_owner)
#define myTorp(t)		(me->p_no == (t)->t_owner)
#define friendly_to(warmask, team, pl) \
  (!((team) & ((pl)->p_swar | (pl)->p_hostile)))
#define friendlyPlasmaTorp(t)	friendly_to((t)->pt_war, (t)->pt_team, me)
#define friendlyTorp(t)		friendly_to((t)->t_war, (t)->t_team, me)
#define friendlyThingy(t) \
  friendly_to((t)->t_war, idx_to_mask(players[(t)->t_owner].p_teami), me)

#define myPhaser(p)		(&phasers[me->p_no] == (p))
#define friendlyPhaser(p)	(me->p_teami == players[(p) - phasers].p_teami)
#define myPlayer(p)		(me == (p))
#define myPlanet(p)		(me->p_teami == mask_to_idx((p)->pl_owner))
#define friendlyPlayer(p)	((!(idx_to_mask(me->p_teami) & \
				    ((p)->p_swar | (p)->p_hostile))) && \
				    (!(idx_to_mask((p)->p_teami) & \
				    (me->p_swar | me->p_hostile))))
#define isAlive(p)		((p)->p_status == PALIVE)
#define isPlaying(p)		((p)->p_status != PFREE)
#define isBase(num)		((num)==STARBASE || (num)==WARBASE || \
				 (num)==JUMPSHIP)
#define friendlyPlanet(p)	((p)->pl_info & idx_to_mask(me->p_teami) && \
			     !((p)->pl_owner & (me->p_swar | me->p_hostile)))

#define isLockPlanet(p)		((me->p_flags & PFPLLOCK) && (me->p_planet == p->pl_no))
#define isLockPlayer(p)		((me->p_flags & PFPLOCK) && (me->p_playerl == p->p_no))
#define PtOutsideWin(x, y)      	(x < 0 || x > winside || y < 0 || y > winside)	/* TSH */

#define torpColor(t)		\
	(myTorp(t) ? myColor : shipCol[1+players[(t)->t_owner].p_teami])
#define droneColor(t)		\
	(myTorp(t) ? myColor : shipCol[1+players[(t)->t_owner].p_teami])
#define plasmatorpColor(t)		\
	(myPlasmaTorp(t) ? myColor : shipCol[1+players[(t)->pt_owner].p_teami])
#define phaserColor(p)		\
	(myPhaser(p) ? myColor : shipCol[1+players[(p) - phasers].p_teami])
/*
 * Cloaking phase (and not the cloaking flag) is the factor in determining
 * the color of the ship.  Color 0 is white (same as 'myColor' used to be).
 */
#define playerColor(p)		\
	(myPlayer(p) ? myColor : shipCol[1+(p)->p_teami])
#define planetColor(p)		\
	(((p)->pl_info & idx_to_mask(me->p_teami)) ? shipCol[1+mask_to_idx((p)->pl_owner)] : unColor)

#define planetFont(p)		\
	(myPlanet(p) ? W_BoldFont : friendlyPlanet(p) ? W_UnderlineFont \
	    : W_RegularFont)
#define shipFont(p)		\
	(myPlayer(p) ? W_BoldFont : friendlyPlayer(p) ? W_UnderlineFont \
	    : W_RegularFont)
#define bombingRating(p)	\
	((float) (p)->p_stats.st_tarmsbomb * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->armsbomb))
#define planetRating(p)		\
	((float) (p)->p_stats.st_tplanets * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->planets))
#define offenseRating(p)	\
	((float) (p)->p_stats.st_tkills * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->kills))
#define defenseRating(p)	\
	((float) (p)->p_stats.st_tticks * status->losses / \
	 ((p)->p_stats.st_tlosses!=0 ? \
	  ((float) (p)->p_stats.st_tlosses * status->timeprod) : \
	  (status->timeprod)))

#define sendTorpReq(dir) sendShortPacket(CP_TORP, RotateDirSend(dir))
#define sendPhaserReq(dir) sendShortPacket(CP_PHASER, RotateDirSend(dir))
#define sendDirReq(dir) sendShortPacket(CP_DIRECTION, RotateDirSend(dir))
#define sendPlasmaReq(dir) sendShortPacket(CP_PLASMA, RotateDirSend(dir))

#define sendSpeedReq(speed) sendShortPacket(CP_SPEED, speed)
#define sendShieldReq(state) sendShortPacket(CP_SHIELD, state)
#define sendOrbitReq(state) sendShortPacket(CP_ORBIT, state)
#define sendRepairReq(state) sendShortPacket(CP_REPAIR, state)
#define sendBeamReq(state) sendShortPacket(CP_BEAM, state)
#define sendCopilotReq(state) sendShortPacket(CP_COPILOT, state)
#define sendDetonateReq() sendShortPacket(CP_DET_TORPS, 0)
#define sendCloakReq(state) sendShortPacket(CP_CLOAK, state)
#define sendBombReq(state) sendShortPacket(CP_BOMB, state)
#define sendPractrReq() sendShortPacket(CP_PRACTR, 0)
#define sendWarReq(mask) sendShortPacket(CP_WAR, mask)
#define sendRefitReq(ship) sendShortPacket(CP_REFIT, ship)
#define sendPlaylockReq(pnum) sendShortPacket(CP_PLAYLOCK, pnum)
#define sendPlanlockReq(pnum) sendShortPacket(CP_PLANLOCK, pnum)
#define sendCoupReq() sendShortPacket(CP_COUP, 0)
#define sendQuitReq() sendShortPacket(CP_QUIT, 0)
#define sendByeReq() sendShortPacket(CP_BYE, 0)
#define sendDockingReq(state) sendShortPacket(CP_DOCKPERM, state)
#define sendResetStatsReq(verify) sendShortPacket(CP_RESETSTATS, verify)
#define sendScanReq(who) sendShortPacket(CP_SCAN, who)

/* This macro allows us to time things based upon # frames / sec.
 */
#define ticks(x) ((x)*200000/timerDelay)

/*
 * UDP control stuff
 */
#define UDP_NUMOPTS    10
#define UDP_CURRENT     0
#define UDP_STATUS      1
#define UDP_DROPPED     2
#define UDP_SEQUENCE    3
#define UDP_SEND	4
#define UDP_RECV	5
#define UDP_DEBUG       6
#define UDP_FORCE_RESET	7
#define UDP_UPDATE_ALL	8
#define UDP_DONE        9
#define COMM_TCP        0
#define COMM_UDP        1
#define COMM_VERIFY     2
#define COMM_UPDATE	3
#define COMM_MODE	4
#define SWITCH_TCP_OK   0
#define SWITCH_UDP_OK   1
#define SWITCH_DENIED   2
#define SWITCH_VERIFY   3
#define CONNMODE_PORT   0
#define CONNMODE_PACKET 1
#define STAT_CONNECTED  0
#define STAT_SWITCH_UDP 1
#define STAT_SWITCH_TCP 2
#define STAT_VERIFY_UDP 3
#define MODE_TCP        0
#define MODE_SIMPLE     1
#define MODE_FAT	2
#define MODE_DOUBLE     3

#define UDP_RECENT_INTR 300
#define UDP_UPDATE_WAIT	5

/* client version of UDPDIAG */
#define UDPDIAG(x)      { if (udpDebug) { printf("UDP: "); printf x; }}
#define V_UDPDIAG(x)    UDPDIAG(x)

#define RSA_VERSION "RSA v2.0 CLIENT"	/* string must begin with characters
					   "RSA v" */
#define KEY_SIZE 32
#define RESERVED_SIZE 16
#define MSG_LEN 80
#define NAME_LEN 16
#define KEYMAP_LEN 96

#define RotateDirSend(d)        (rotate?d-rotate_deg:d)

#define         SPK_VOFF        0	/* variable packets off */
#define         SPK_VON         1	/* variable packets on */
#define         SPK_MOFF        2	/* message packets off */
#define         SPK_MON         3	/* message packets on */
#define         SPK_M_KILLS     4	/* send kill mesgs */
#define         SPK_M_NOKILLS   5	/* don't send kill mesgs */
#define         SPK_THRESHOLD   6	/* threshold */
#define         SPK_M_WARN      7	/* warnings */
#define         SPK_M_NOWARN    8	/* no warnings */
#define SPK_SALL 9		/* only planets,kills and weapons */
#define         SPK_ALL 10	/* Full Update - SP_STATS */

#define         SPK_NUMFIELDS   7

#define         SPK_VFIELD      0
#define         SPK_MFIELD      1
#define         SPK_KFIELD      2
#define         SPK_WFIELD      3
#define         SPK_TFIELD      4
#define         SPK_WHYFIELD    5
#define         SPK_DONE        6

#define TOOLSWINLEN 25

#define LITE_PLAYERS_MAP	0x01
#define LITE_PLAYERS_LOCAL	0x02
#define LITE_SELF		0x04
#define LITE_PLANETS		0x08
#define LITE_SOUNDS		0x10
#define LITE_COLOR              0x20

#define sock_write		write
#define sock_close		close
#define sock_ioctl		ioctl

#define PB_REDALERT -1
#define PB_YELLOWALERT -2
#define PB_DEATH -3

#define MAX_CLIENT_VERSION_STRING 100

#endif
