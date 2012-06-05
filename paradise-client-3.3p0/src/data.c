/*
 * data.c
 */
#include "copyright.h"

#include "config.h"
#include "conftime.h"
#include <stdlib.h>
#include <stdio.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"

int     useExternalImages = 1;	/* uses external images in preference to
                                   compiled in images [BDyess] */

int     rounded_asteroids = 1;	/* draws rounded asteroid belt [BDyess] */

int     scrollSaveLines = 500;	/* default number of scrollback lines [BDyess]*/

int     plotter = 0;		/* galactic map plotting line [BDyess] */
int     clearplotter = 0;	/* flag for plotter state changes [BDyess] */

/* variables needed now that MAPSIDE, WINSIDE, and SCALE are variable [BDyess] */
int	winside = WINSIDE;	/* width of local window [BDyess] */
int	mapside = MAPSIDE;	/* width of galactic window [BDyess] */
int	scale = SCALE;		/* translate server->local coord. [BDyess] */
int	mapscale=GWIDTH/MAPSIDE;/* translate server->map coord. [BDyess] */
int	center = WINSIDE/2;	/* center of local window [BDyess] */
int	fullview=WINSIDE*SCALE; /* width of local win in server units[BDyess] */
int	view = WINSIDE*SCALE/2;	/* half fullview - how far the player can see */
				/* in each direction [BDyess] */

int     showNewStats = 0;	/* new stats window [BDyess] */

char    warningbuf[85];		/* holder of the current warning [BDyess] */
int     warncount = 0;
int     warntimer = -1;
char	hwarningbuf[85];	/* holder of the current high warning [BDyess]*/
int     hwarncount = 0;		/* length of hwarningbuf [BDyess] */
int	hwarntimer = -1;	/* timer for clearing of hwarning [BDyess] */
int	hudwarning = 1;		/* HUD warning on or off [BDyess] */

int     paradise = 0;		/* is the server a paradise server */
int	hockey = 0;		/* is the server a hockey server [BDyess] */

int     planetChill = 1;	/* udcounter is divided by this to find out
                                   the current frame - higher numbers mean
				   slower planet rotation [BDyess] */
int	dump_defaults = 0;	/* if on, all .paradiserc defaults are dumped
				   [BDyess] */

int gwidth;			/* galaxy width, adjusted for zoom [BDyess] */
int offsetx, offsety;		/* offsets when zooming [BDyess] */
Tractor *tracthead = NULL,      /* data for remembering where tractor lines */
        *tractcurrent = NULL,   /* were drawn [BDyess] */
	*tractrunner;

int	xpm = 0;		/* use xpm's or not [BDyess] */
char   *xpmPath = NULL;		/* path prefix used to find pixmaps [BDyess] */
int     cookie = 0;		/* cookie mode [BDyess] */
int	verbose_image_loading = 0; /* image loading logging [BDyess] */
int     useOR = 0;		/* turn Rob's color allocator on so GXor can
                                   be used to draw images [BDyess] */
int     colorEnemies = 1;	/* use .colored for enemies by default */
int     colorFriends = 0;	/* don't use .colored for friends by default */

char   *imagedir = NULL;	/* dir containing image files [BDyess] */
char   *imagedirend = NULL;	/* end of original imagedir [BDyess] */

int     nplayers = 36;
int     nshiptypes = 15;
int     ntorps = 8;
int     npthingies = 20;
int     ngthingies = 0;
int     nplasmas = 1;
int     nphasers = 1;
int     nplanets = MAXPLANETS;		/* get this info dyn. from Pserver */

/* hockey stuff [BDyess] */
int	galacticHockeyLines = 1;  /* draw lines on the galactic? [BDyess] */
int	tacticalHockeyLines = 1;  /* draw lines on the tactical? [BDyess] */
int	cleanHockeyGalactic = 0;  /* don't draw planets on the galactic when
                                     playing hockey [BDyess] */
int	teamColorHockeyLines = 1; /* color hockey lines by team [BDyess] */
int	puckBitmap = 1;		  /* use the puck bitmap [BDyess] */
int	puckArrow = 1;		  /* draw a direction arrow on puck [BDyess] */
int 	puckArrowSize = 3;	  /* size of puckArrow [BDyess] */
struct	hockeyLine hlines[NUM_HOCKEY_LINES];

int     metaFork = 0;		/* allow spawning off of clients from meta-
				   server window [BDyess] */

int     viewBox = 0;		/* flag for viewBox [BDyess] */
int     allowViewBox = 1;	/* allow flag for viewBox [BDyess] */
int	allowShowArmiesOnLocal = 1;	/* allow flag for displaying # of planet armies
				 * on local display */
int     sectorNums = 0;		/* flag for numbering sectors in galactic -TH */
int     lockLine = 0;		/* flag for line in lock for galactic -TH */
int     mapSort = 1;		/* sort player list by team -TH */
int     autoSetWar = 1;         /* do war decl's when tmode starts -TH */

char   *playerList;		/* string of fields for wide playerlist */
char   *playerListStart;	/* comma seperated set of strings for plist */
int     resizePlayerList = 0;

int     packetLights = 0;	/* flag for packetLights [BDyess] */

/* for showgalactic and showlocal rotation sequence [BDyess] */
char   *showGalacticSequence, *showLocalSequence;

/* Lynx wants the playerlist blanked upon entry.  Ok, whatever [BDyess] */
int     allowPlayerlist = 1;

/* message window array [BDyess] */
struct messageWin messWin[WNUM];

/* global counters for number of queued messages. [BDyess] */
int     me_messages = 0, all_messages = 0, team_messages = 0;

/* added 1/94 -JR */
int     niftyNewMessages = 1;	/* on by default */

/* needed for rc_distress [BDyess] */
int     F_gen_distress = 0;	/* generic distress/macro system support */
/* the index into distmacro array should correspond with the correct dist_type */

#define NUM_DIST 27


#define control(x) (x)+128

struct dmacro_list dist_prefered[NUM_DIST];

/* the index into distmacro array should correspond with the correct dist_type */
/* the character specification is ignored now, kept here anyway for reference */
struct dmacro_list dist_defaults[] =
{
    {'X', "no zero", "this should never get looked at"},
    {control('t'), "taking", " %T%c->%O (%S) Carrying %a to %l%?%n>-1%{ @ %n%}\0"},
    {control('o'), "ogg", " %T%c->%O Help Ogg %p at %l\0"},
    {control('b'), "bomb", " %T%c->%O %?%n>4%{bomb %l @ %n%!bomb%}\0"},
    {control('c'), "space_control", " %T%c->%O Help Control at %L\0"},
    {control('1'), "save_planet", " %T%c->%O Help at %l! %?%a>0%{ (have %a arm%?%a=1%{y%!ies%}) %} %s%% shld, %d%% dam, %f%% fuel\0"},
    {control('2'), "base_ogg", " %T%c->%O Sync with --]> %g <[-- OGG ogg OGG base!!\0"},
    {control('3'), "help1", " %T%c->%O Help me! %d%% dam, %s%% shd, %f%% fuel %a armies.\0"},
    {control('4'), "help2", " %T%c->%O Help me! %d%% dam, %s%% shd, %f%% fuel %a armies.\0"},
    {control('e'), "escorting", " %T%c->%O ESCORTING %g (%d%%D %s%%S %f%%F)\0"},
    {control('p'), "ogging", " %T%c->%O Ogging %h\0"},
    {control('m'), "bombing", " %T%c->%O Bombing %l @ %n\0"},
    {control('l'), "controlling", " %T%c->%O Controlling at %l\0"},
    {control('5'), "asw", " %T%c->%O Anti-bombing %p near %b.\0"},
    {control('6'), "asbomb", " %T%c->%O DON'T BOMB %l. Let me bomb it (%S)\0"},
    {control('7'), "doing1", " %T%c->%O (%i)%?%a>0%{ has %a arm%?%a=1%{y%!ies%}%} at %b.  %d%% dam, %s%% shd, %f%% fuel\0"},
    {control('8'), "doing2", " %T%c->%O (%i)%?%a>0%{ has %a arm%?%a=1%{y%!ies%}%} at %b.  %d%% dam, %s%% shd, %f%% fuel\0"},
    {control('f'), "free_beer", " %T%c->%O %p is free beer\0"},
    {control('n'), "no_gas", " %T%c->%O %p @ %l has no gas\0"},
    {control('h'), "crippled", " %T%c->%O %p @ %l crippled\0"},
    {control('9'), "pickup", " %T%c->%O %p++ @ %l\0"},
    {control('0'), "pop", " %T%c->%O %l%?%n>-1%{ @ %n%}!\0"},
     /* F */ {'F', "carrying", " %T%c->%O %?%S=SB%{Your Starbase is c%!C%}arrying %?%a>0%{%a%!NO%} arm%?%a=1%{y%!ies%}.\0"},
    {control('@'), "other1", " %T%c->%O (%i)%?%a>0%{ has %a arm%?%a=1%{y%!ies%}%} at %b. (%d%%D, %s%%S, %f%%F)\0"},
    {control('#'), "other2", " %T%c->%O (%i)%?%a>0%{ has %a arm%?%a=1%{y%!ies%}%} at %b. (%d%%D, %s%%S, %f%%F)\0"},
     /* E */ {'E', "help", " %T%c->%O Help(%S)! %s%% shd, %d%% dmg, %f%% fuel,%?%S=SB%{ %w%% wtmp,%!%}%E%{ ETEMP!%}%W%{ WTEMP!%} %a armies!\0"},
    {'\0', '\0', '\0'},
};

struct dmacro_list *distmacro = dist_defaults;

int     sizedist = sizeof(dist_defaults);


/* continuousmouse kludge */
int     exitInputLoop;

/* ghoststart data */
int     ghoststart = 0;		/* is this a ghostbust restart? */
int     ghost_pno = 0;		/* my p_no if it is */

/* time client connected to server [BDyess] */
time_t  timeStart = 0;

/* dashboard timer data [BDyess] */
int     timerType = T_SHIP;	/* timer defaults to ship timer */
time_t  timeBank[T_TOTAL];	/* array of times	 */

struct macro *macrotable[256];
int     macroState = 0;		/* 0=nothing, 1=in macro mode, 2=in macro,
				   want destination */
char    lastMessage[100] = {0};

/* defaults list */
struct stringlist *defaults = NULL;

/* upgrade kludge flag */
int     upgrading = 0;

/* clearzone data */
int     czsize = (8 + 1 + /* MAXTHINGIES */ 8 + 1 + 1) * 32 + 60;
struct _clearzone *clearzone = 0;
int     clearcount = 0;
int     clearline[4][32 + 2 * 32 + NUM_HOCKEY_LINES];
int     clearlmark[2];
int     clearlmcount;
int     clearlcount;
int     mclearzone[6][32];	/* for map window */

struct player *players;
struct player *me = NULL;
struct torp *torps;
struct thingy *thingies = 0;
struct plasmatorp *plasmatorps;
struct status *status;
struct status2 *status2;
struct ship *myship;
struct shiplist *shiptypes = NULL;
struct stats *mystats;
struct planet *planets;
struct t_unit *terrainInfo;
int received_terrain_info = 0;
int terrain_x;
int terrain_y;
struct phaser *phasers;
struct message *messages;
struct mctl *mctl;

int     logPhaserMissed = 0;	/* default to not log 'phaser missed' type
				   messages [BDyess] */
int     phaserStats = 1;	/* default to keeping phaser stats. -JR */
int     phasFired = 0, phasHits = 0, totalDmg = 0; /* moved here to allow resetting */

int     infoIcon = 0;		/* default to bitmap icon, not info icon
				   [BDyess] */
int     iconified = 0;		/* 1 if the client is iconified [BDyess] */
char   *defaultsFile = NULL;	/* name of defaults file (.xtrekrc usually) */
short  *slot = NULL;		/* array of who's in what slot for playerlist */
char   *defNickName = NULL;
char   *defFlavor = NULL;
char   *cloakchars = NULL;	/* characters used for cloakers */
int     cloakcharslen;		/* length of cloakchars string */
int     oldalert = 0;
/*int             remap[16] =
{0, 1, 2, 0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0};*/
int     messpend;
int     lastcount;
int     mdisplayed;
int     redrawall;
int     nopilot = 1;
int     selfdest;
int     udcounter;
int     showMySpeed = 0;
int     showTractorPressor = 1;
int     showAllTractorPressor = 0;	/* shows _all_ TP's, not just self
					   [BDyess] */
int     allowShowAllTractorPressor = 1;	/* server can disable [BDyess] */
int     showLock = 3;
int     autoKey = 0;
int     extraBorder = 1;
char   *unixSoundPath = NULL;
int     playSounds = 1;
/* udp options */
int     tryUdp = 1;
struct plupdate pl_update[MAX_PLANETS];	/* should be jammed into struct
					   planet */
int     currentship;
int     lastm;
int     delay;			/* delay for decaring war */
int     rdelay;			/* delay for refitting */
int     mapmode = GMAP_INFREQUENT;
int     namemode = 1;
int     showStats;
int     showShields = 1;
int     infomapped = 0;
void   *infothing = NULL;	/* infow contents [BDyess] */
int     infoupdate = 0;		/* update flag for infow [BDyess] */
int     infotype = 0;		/* type of info thing [BDyess] */
int     keepInfo = 0;
int     infowin_up = -2;
int     mustexit = 0;
int     messtime = 5;
int     keeppeace = 0;
int     drawgrid = 1;

/* playerlist settings */
int     sortPlayers = 0;	/* whether to sort playerlist [BDyess]   */
int     hideNoKills = 0;
int     showDead = 1;
int     showPreLogins = 0;
int     sortOutfitting = 1;	/* sorts '--' players to bottom [BDyess]    */
int     robsort = 0;		/* flag for Rob.  Allows changing of the    */
 /* quadrant the playerlist uses for your    */
 /* team. [BDyess]   			    */
int     Dashboard = 3;		/* 0 = old dashboard, 1 = new dashboard,    */
 /* 2 = newdashboard2, 3 = rainbow dashboard */
 /* [BDyess]				    */
int     cup_half_full = 0;	/* setting for new dashboard 2 [BDyess]	    */
int     logmess = 0;		/* logging of activities or not [BDyess]    */
char   *logFile = NULL;		/* logfile to be used for logging [BDyess]  */
FILE   *logfilehandle;
int     showPhaser = 0;		/* settings for showphaser stuff [BDyess]   */
int     vary_hull = 0;		/* setting for varying hull indicator
				   [BDyess] */
int     warpStreaks = 1;	/* flag for warp star streaking [BDyess]    */
int     fastQuit = 0;		/* flag for fast quit [BDyess]              */
int     pigSelf = 1;		/* pigcall response from self [BDyess]      */
int     continuousMouse = 0;	/* continuous mouse flag [BDyess]	    */
int	extendedMouse = 0;	/* more buttons for mouse */
int     clickDelay = 5;		/* # of updates to delay before repeating   */
 /* turns on and off continuousMouse for each button [BDyess] */
int     buttonRepeatMask = 0;
 /* mouse event [BDyess]			    */
int     allowContinuousMouse = 1;	/* allow continuous mouse to work
					   flag, so  */
 /* each server can turn it off [BDyess]     */
int     autoQuit = 60;		/* time to wait before auto-quit [BDyess]   */

int     msgBeep = 1;		/* ATM - msg beep */
int     scanmapped = 0;		/* ATM - scanners */

/* no longer integers.  See defaults.c for default values. [BDyess] */
char    *showlocal;
char    *showgalactic;
int	showLocalLen, showGalacticLen;	/* length of current showlocal and */
					/* showgalactic [BDyess] */
int     sendmotdbitmaps = 1;

char   *title = NULL;
char   *shipnos = "0123456789abcdefghijklmnopqrstuvwxyz";
int     sock = -1;
int     xtrekPort = -1;
int     queuePos = -1;
int     pickOk = -1;
int     lastRank = -1;
int     promoted = 0;
int     loginAccept = -1;
unsigned localflags = 0;
int     tournMask = 15;
int     nextSocket;		/* socket to use when we get ghostbusted... */
int     updatePlayer[MAX_PLAYER];	/* Needs updating on player * list */
char   *serverName = NULL;
int     loggedIn = 0;
int     reinitPlanets = 0;
int     redrawPlayer[MAX_PLAYER];	/* Needs redrawing on galactic map */
int     lastUpdate[MAX_PLAYER];	/* Last update of this player */
int     timerDelay = 200000;	/* micro secs between updates */
int     reportKills = 1;	/* report kill messages? */

int     scanplayer;		/* who to scan */
int     showTractor = 1;	/* show visible tractor beams */
int     commMode = 0;		/* UDP: 0=TCP only, 1=UDP updates */
int     commModeReq = 0;	/* UDP: req for comm protocol change */
int     commStatus = 0;		/* UDP: used when switching protocols */
int     commSwitchTimeout = 0;	/* UDP: don't wait forever */
int     udpTotal = 1;		/* UDP: total #of packets received */
int     udpDropped = 0;		/* UDP: count of packets dropped */
int     udpRecentDropped = 0;	/* UDP: #of packets dropped recently */
int     udpSock = -1;		/* UDP: the socket */
int     udpDebug = 0;		/* UDP: debugging info on/off */
int     udpClientSend = 1;	/* UDP: send our packets using UDP? */
int     udpClientRecv = 1;	/* UDP: receive with simple UDP */
int     udpSequenceChk = 1;	/* UDP: check sequence numbers */
int     updateSpeed = 10;	/* updates per second */

/* MOTD data */
struct page *currpage = NULL;
struct page *motddata = NULL;

/* metaserver window stuff */
int     usemeta = 0;
char   *metaserverAddress;

char    blk_refitstring[80] = "s=scout, d=destroyer, c=cruiser, b=battleship, a=assault, o=starbase";
int     blk_gwidth;
float   blk_windgwidth;
int     showKitchenSink = 0;
int     blk_showStars = 1;
int     blk_bozolist = -1;
/*
 * These are considered "borgish" features by some, so the server has to turn
 * them on.  All are default off, no way for player to turn them on.
 */
int     blk_friendlycloak = 0;	/* Show color of cloakers who are friendly. */

int     forceMono = 0;

extern double Sin[], *Cos;

W_Color borderColor, backColor, textColor, myColor, warningColor, shipCol[6],
        rColor, yColor, gColor, unColor, foreColor;

/*char  teamlet[] =
{'I', 'F', 'R', 'K', 'O'};
char  *teamshort[] =
{"IND", "FED", "ROM", "KLI", "ORI"};*/

char    pseudo[PSEUDOSIZE];
char    defpasswd[PSEUDOSIZE];
char    login[PSEUDOSIZE];

struct rank ranks[NUMRANKS] =
{
    {2.0, 1.0, 0.0, "Ensign"},
    {4.0, 2.0, 0.8, "Lieutenant"},
    {8.0, 3.0, 0.8, "Lt. Cmdr."},
    {8.0, 3.0, 0.8, "Commander"},
    {15.0, 4.0, 0.8, "Captain"},
    {20.0, 5.0, 0.8, "Flt Cptn."},
    {25.0, 6.0, 0.8, "Commodore"},
    {40.0, 8.0, 0.8, "Rear Adml."},
    {40.0, 8.0, 0.8, "Admiral"}
};


int     nranks2 = 18;
struct rank2 *ranks2;


int     nroyals = 5;
struct royalty *royal = 0;

W_Window messagew, w, mapw, statwin, baseWin, infow = 0, iconWin, tstatw,
        war, warnw, helpWin, teamWin[4], qwin,	planetw, planetw2, playerw, 
	rankw, optionWin = 0, metaWin = 0, macroWin = 0, defWin, motdWin = 0,
	newstatwin = 0;

	/* messwa, messwt, messwi, messwk, */ 
	/* reviewWin, phaserwin, */ 

W_Window scanw, scanwin, udpWin;

W_Window spWin;

W_Window toolsWin = NULL;
int     shelltools = 1;

int     rotate = 0;
int     rotate_deg = 0;

int     messageon = 0;
int     warp = 0;

int     RSA_Client = 1;
int     blk_zoom = 0;		/* zoom in to 1/4 galaxy */
int	show_armies_on_local = 0;	/* show # of armies on local display */

/* zoom map based on alert status?  Timer is to let tab override for x updates. -JR*/
/* Now that I've done it, I find it pretty annoying ;-)  Here it is anyway. */
int     autoZoom=0, autoUnZoom=0, auto_zoom_timer=0, autoZoomOverride=15;

int     use_msgw = 0;		/* send last message to message window */

int     show_shield_dam = 1;	/* show shield damage by color */


int     tryShort = 1;		/* for .xtrekrc option */
int     recv_short = 0;
int     recv_mesg = 1;
int     recv_kmesg = 1;
int     recv_threshold = 0;
char    recv_threshold_s[8] = {'0', '\0'};
int     recv_warn = 1;
int godToAllOnKills = 1;

int     ping = 0;		/* to ping or not to ping */
long    packets_sent = 0;	/* # all packets sent to server */
long    packets_received = 0;	/* # all packets received */
W_Window pStats;

int     lowercaset = 0;		/* I hate shift-T for team.  put "lowercaset:
				   on" to allow 't' -JR */
int     why_dead = 0;		/* add reason for death to SP kill msgs. */
int     cloakerMaxWarp = 0;	/* server reports cloaker's speed as 15. */
int     F_dead_warp = 0;	/* dead players reported at warp 14 */
int     F_feature_packets = 0;	/* whether to use them or not */
int     F_multiline_enabled = 0;/* is the MMACRO flag enabled? */
int     F_UseNewMacro = 1;	/* Not sure this is actually checked... */
int	F_terrain = 1;		/* Enable terrain sending */
unsigned char F_terrain_major = 1;	/* Version 1.0 of terrain */
unsigned char F_terrain_minor = 0;
int	F_gz_motd = 0;		/* Can't handle gzipped MOTD yet */
unsigned char	F_gz_motd_major = 0;	/* call it v0.0 then */
unsigned char	F_gz_motd_minor = 0;
int     F_armies_shipcap;	/* server sends ship army cap. in ship_cap */

int     F_allow_beeplite = 1;
unsigned char    F_beeplite_flags = LITE_PLAYERS_MAP |
LITE_PLAYERS_LOCAL |
LITE_SELF |
LITE_PLANETS |
LITE_SOUNDS |
LITE_COLOR;

char   *distlite[NUM_DIST] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int     DefLite = 0;
int     UseLite = 0;

int     emph_planet_seq_n[MAX_PLANETS] = {0,};
int     emph_player_seq_n[MAX_PLAYER] = {0,};
int     beep_lite_cycle_time_player = 10;
int     beep_lite_cycle_time_planet = 10;
W_Color emph_planet_color[MAX_PLANETS];
W_Color emph_player_color[MAX_PLAYER];

/* When you enter game send request for full update SRS 3/15/94 */
int     askforUpdate = 0;

int     jubileePhasers = 0;	/* cycle phaser hits through all the colors.
				   Idea from COW-lite.  -JR */
int     enemyPhasers = 0;       /* Draw wide beams for enemy phasers.
				   (range = 1-10, 0=off) -JR */

int     scrollBeep = 1;

int     recordGame = 0;
char   *recordFile = 0;
int     maxRecord = 1000000;	/* default 1 meg max */

int     playback = 0;
char   *playFile = 0;
int     pb_update = 0, pb_advance = 0, paused = 1, pb_scan=0, pb_slow=0;

int redrawDelay = 0;

int localShipStats = 0;
char *statString;
int statHeight=20, localStatsX=200, localStatsY=260;

int showIND=0;

char CLIENTVERS[MAX_CLIENT_VERSION_STRING] = "3.3p0";

/* old stuff from gameconf.h */
struct teaminfo_s *teaminfo = NULL;
int number_of_teams = 0;
