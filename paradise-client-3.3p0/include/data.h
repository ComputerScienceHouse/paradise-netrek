/*
 * data.h
 */

#ifndef DATA_H
#define DATA_H

#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include "conftime.h"

#include "defs.h"
#include "struct.h"
#include "gameconf.h"

extern int useExternalImages;	/* uses external images in preference to
                                   compiled in images [BDyess] */

extern int rounded_asteroids;	/* draws rounded asteroid belt [BDyess] */

extern int scrollSaveLines;	/* default number of scrollback lines [BDyess]*/

extern int plotter;		/* galactic map plotting line [BDyess] */
extern int clearplotter;	/* flag for plotter state changes [BDyess] */

/* variables needed now that MAPSIDE, WINSIDE, and SCALE are variable [BDyess] */
extern int winside;		/* width of the local window [BDyess] */
extern int mapside;		/* width of the map window [BDyess] */
extern int scale;		/* translate server->local coord. [BDyess] */
extern int mapscale;		/* translate server->map coord. [BDyess] */
extern int center;		/* center of local window [BDyess] */
extern int fullview;		/* width of local window in server units[BDyess]*/
extern int view;		/* half fullview - how far the player can see */
				/* in each direction [BDyess] */

extern int showNewStats;	/* new stats window [BDyess] */

extern char warningbuf[];	/* holder of the current warning [BDyess] */
extern int  warncount;
extern int  warntimer;
extern char hwarningbuf[];	/* holder of the current high warning [BDyess]*/
extern int  hwarncount;		/* length of hwarningbuf [BDyess] */
extern int  hwarntimer;		/* timer for clearing of hwarning [BDyess] */
extern int  hudwarning;		/* HUD warning on or off [BDyess] */

extern int paradise;		/* is the server a paradise server? */
extern int hockey;		/* is the server a hockey server [BDyess] */

extern int gwidth;		/* galaxy width, adjusted for zoom [BDyess] */
extern int offsetx, offsety;	/* offsets when zooming [BDyess] */
extern Tractor *tracthead,      /* data for remembering where tractor lines */
               *tractcurrent, 	/* were drawn [BDyess] */
	       *tractrunner;

extern int planetChill;		/* udcounter is divided by this to find out
                                   the current frame - higher numbers mean
				   slower planet rotation [BDyess] */
extern int dump_defaults;	/* if on, all .paradiserc defaults are dumped
				   [BDyess] */
extern int xpm;			/* use xpm's or not [BDyess] */
extern char *xpmPath;		/* path prefix used to find pixmaps [BDyess] */
extern int cookie;		/* cookie mode [BDyess] */
extern int verbose_image_loading; /* image loading logging [BDyess] */
extern int useOR;		/* turn Rob's color allocator on so GXor can
                                   be used to draw images [BDyess] */
extern int colorFriends;	/* use .colored for friendly ships */
extern int colorEnemies;	/* use .colored for enemy ships */

extern char *imagedir;		/* dir containing image files [BDyess] */
extern char *imagedirend;	/* end of original imagedir [BDyess] */

extern int nplayers;
extern int nshiptypes;
extern int ntorps;
extern int npthingies;
extern int ngthingies;
extern int nplasmas;
extern int nphasers;
extern int nplanets;

/* hockey stuff [BDyess] */
extern int galacticHockeyLines;  /* draw lines on the galactic? [BDyess] */
extern int tacticalHockeyLines;  /* draw lines on the tactical? [BDyess] */
extern int cleanHockeyGalactic;  /* don't draw planets on the galactic when
                                    playing hockey [BDyess] */
extern int teamColorHockeyLines; /* color hockey lines by team [BDyess] */
extern int puckBitmap;		 /* use the puck bitmap [BDyess] */
extern int puckArrow;		 /* draw a direction arrow on puck [BDyess] */
extern int puckArrowSize;	 /* size of puckArrow [BDyess] */
extern struct	hockeyLine hlines[NUM_HOCKEY_LINES];

extern struct player *puck;	 /* pointer to puck [BDyess] */

extern int metaFork;		/* allow spawning off of clients from meta-
				   server window [BDyess] */
extern int viewBox;		/* flag for viewBox [BDyess] */
extern int allowViewBox;	/* allow flag for viewBox [BDyess] */
extern int allowShowArmiesOnLocal;	/* allow flag for showing # of planet
				 * armies on local display */

extern int sectorNums;		/* flag for numbering sectors in galactic -TH */
extern int lockLine;		/* flag for line from me to lock in galac -TH */

extern int sectorNums;		/* flag for numbering sectors in galactic -TH */
extern int lockLine;		/* flag for line from me to lock in galac -TH */
extern int mapSort;		/* use new sorting in galactic -TH */
extern int autoSetWar;          /* automatically set war dec's -TH */

extern char *playerList;	/* string of fields for wide playerlist */
extern char *playerListStart;	/* comma seperated set of strings for plist */
extern int resizePlayerList;

extern int packetLights;	/* flag for packetLights [BDyess] */

/* for showgalactic and showlocal rotation sequence [BDyess] */
extern char *showGalacticSequence, *showLocalSequence;

/* Lynx wants the playerlist blanked upon entry.  Ok, whatever [BDyess] */
extern int allowPlayerlist;

/* message window array [BDyess] */
extern struct messageWin messWin[];

/* global counters for number of queued messages [BDyess] */
extern int me_messages, all_messages, team_messages;

/* added 1/94 -JR */
extern int niftyNewMessages;

/* needed for rc_distress [BDyess] */
extern int F_gen_distress;

extern struct dmacro_list *distmacro;
extern struct dmacro_list dist_defaults[];
extern struct dmacro_list dist_prefered[];
extern int sizedist;

extern int F_allow_beeplite;
extern unsigned char F_beeplite_flags;

extern char *distlite[];
extern int UseLite;
extern int DefLite;
extern int emph_planet_seq_n[];
extern int emph_player_seq_n[];
extern W_Color emph_planet_color[MAX_PLANETS];
extern W_Color emph_player_color[MAX_PLAYER];
#define emph_planet_seq_frames 5
#define emph_planet_seq_width 24
#define emph_planet_seq_height 24
#define emph_player_seq_frames 3
#define emph_player_seq_width 24
#define emph_player_seq_height 24
#define emph_player_seql_frames 3
#define emph_player_seql_width 30
#define emph_player_seql_height 30
extern int beep_lite_cycle_time_player;
extern int beep_lite_cycle_time_planet;

/* time client connected to server [BDyess] */
extern time_t timeStart;

/* timer data */
extern int timerType;
extern time_t timeBank[];

extern struct macro *macrotable[256];
extern int macroState;
extern char lastMessage[100];

/* ghoststart data */
extern int ghoststart;
extern int ghost_pno;

/* defaults list */
extern struct stringlist *defaults;

/* upgrade kludge flag [BDyess] */
extern int upgrading;

/* continuousmouse kludge [BDyess] */
extern int exitInputLoop;

/* clearzone data */
extern int czsize;
extern struct _clearzone *clearzone;
extern int clearcount;
extern int clearline[4][32 + 2 * 32 + NUM_HOCKEY_LINES];
extern int clearlmark[2];
extern int clearlmcount;
extern int clearlcount;
extern int mclearzone[6][32];	/* for map window */

extern struct player *players;
extern struct player *me;
extern struct torp *torps;
extern struct thingy *thingies;
extern struct plasmatorp *plasmatorps;
extern struct status *status;
extern struct status2 *status2;
extern struct ship *myship;
extern struct shiplist *shiptypes;
extern struct stats *mystats;
extern struct planet *planets;
extern struct t_unit *terrainInfo;
extern int received_terrain_info;
extern int terrain_x;
extern int terrain_y;
extern struct phaser *phasers;
extern struct team *teams;
extern struct planet pdata[];

extern int logPhaserMissed;	/* log or not 'phaser missed' messages
				   [BDyess] */
extern int phaserStats;		/* Phaser statistics on or off -JR */
extern int phasFired, phasHits, totalDmg; /* the stats */

extern int infoIcon;		/* information icon flag [BDyess] */
extern int iconified;		/* iconified or not flag [BDyess] */
extern char *defaultsFile;
extern short *slot;
extern char *defNickName;
extern char *defFlavor;
extern int oldalert;
/*extern int remap[];*/
extern int udcounter;
extern char *title;
extern char *cloakchars;	/* characters used for cloakers, defaults to
				   ?? */
extern int cloakcharslen;
extern struct plupdate pl_update[];
extern int currentship;
extern int messpend;
extern int lastcount;
extern int mdisplayed;
extern int redrawall;
extern int nopilot;
extern int watch;
extern int selfdest;
extern int lastm;
extern int delay;
extern int rdelay;
extern int mapmode;
extern int namemode;
extern int showShields;
extern int showStats;
extern int msgBeep;		/* ATM - msg beep */
extern int infomapped;
extern void *infothing;		/* infow contents [BDyess] */
extern int infoupdate;		/* update flag for infow [BDyess] */
extern int infotype;		/* type of info thing [BDyess] */
extern int keepInfo;		/* .xtrekrc setting */
extern int infowin_up;		/* how long should it remain up? */
extern int scanmapped;		/* ATM - scanner stuff */
extern int mustexit;
extern int messtime;
extern int keeppeace;
extern int drawgrid;

/* playerlist settings */
extern int sortPlayers;
extern int hideNoKills;
extern int showDead;
extern int showPreLogins;
extern int sortOutfitting;
extern int robsort;

/* dashboard settings */
extern int Dashboard;
extern int newDashboard;
extern int cup_half_full;

extern int logmess;
extern char *logFile;
extern FILE *logfilehandle;
extern int showPhaser;
extern int vary_hull;
extern int warpStreaks;
extern int fastQuit;
extern int pigSelf;
extern int continuousMouse;
extern int extendedMouse;
extern int allowContinuousMouse;
extern int clickDelay;
 /* turns on and off continuousMouse for each button [BDyess] */
extern int buttonRepeatMask;
extern int autoQuit;

extern int messageon;

extern char blk_refitstring[80];
extern int blk_gwidth;
extern float blk_windgwidth;
/*extern int blk_altbits;*/
extern int showKitchenSink;
extern int blk_showStars;
extern int blk_bozolist;
extern int blk_friendlycloak;

extern int forceMono;

extern char *showlocal, *showgalactic;
extern int showLocalLen, showGalacticLen; /* length of current showlocal and */
					  /* showgalactic [BDyess] */
extern int sendmotdbitmaps;
extern char *shipnos;
extern int sock;
extern int xtrekPort;
extern int queuePos;
extern int pickOk;
extern int lastRank;
extern int promoted;
extern int loginAccept;
extern unsigned localflags;
extern int tournMask;
extern int nextSocket;
extern int updatePlayer[];
extern char *serverName;
extern int loggedIn;
extern int reinitPlanets;
extern int redrawPlayer[];
extern int lastUpdate[];
extern int timerDelay;
extern int reportKills;
extern char *unixSoundPath;
extern int playSounds;

extern int scanplayer;
extern int showTractor;
extern int commMode;		/* UDP */
extern int commModeReq;		/* UDP */
extern int commStatus;		/* UDP */
extern int commSwitchTimeout;	/* UDP */
extern int udpTotal;		/* UDP */
extern int udpDropped;		/* UDP */
extern int udpRecentDropped;	/* UDP */
extern int udpSock;		/* UDP */
extern int udpDebug;		/* UDP */
extern int udpClientSend;	/* UDP */
extern int udpClientRecv;	/* UDP */
extern int udpSequenceChk;	/* UDP */
extern int updateSpeed;

/* metaserver window stuff */
extern int usemeta;
extern char *metaserverAddress;

/* MOTD data */
extern struct page *currpage;
extern struct page *motddata;

extern int showMySpeed;
extern int showTractorPressor;
extern int showAllTractorPressor;
extern int allowShowAllTractorPressor;
extern int showLock;
extern int autoKey;
extern int extraBorder;
/* udp options */
extern int tryUdp;


extern double *Sin, *Cos;

#define NEW_SHIP_BM

/**************************/
/* stellar object bitmaps */
/*
  b	tactical owner
  mb	galactic owner
  b2	tactical resources
  mb2	galactic resources
  */

extern W_Color borderColor, backColor, textColor, myColor, warningColor, shipCol[6],
        rColor, yColor, gColor, unColor, foreColor;

/*extern char teamletdata[];
extern char *teamshortdata[];*/
extern char pseudo[PSEUDOSIZE];
extern char defpasswd[PSEUDOSIZE];
extern char login[PSEUDOSIZE];

extern struct rank ranks[NUMRANKS];
extern struct rank2 *ranks2;
extern struct royalty *royal;
extern int nranks2;
extern int nroyals;

extern W_Window messagew, w, mapw, statwin, baseWin, infow, iconWin, tstatw, 
	war, warnw, helpWin, teamWin[4], qwin, planetw, planetw2, rankw, 
	playerw, optionWin, metaWin, macroWin, defWin, motdWin, newstatwin;

	/* messwa, messwt, messwi, messwk, */ 
	/* reviewWin, phaserwin, */ 
extern W_Window scanw, scanwin, udpWin;

extern W_Window spWin;

extern W_Window toolsWin;
extern int shelltools;

extern int rotate;
extern int rotate_deg;

extern int messageon;
extern int warp;

extern int RSA_Client;
extern int blk_zoom;
extern int show_armies_on_local;
extern int autoZoom, autoUnZoom, auto_zoom_timer, autoZoomOverride;

extern int use_msgw;

extern int show_shield_dam;

extern int tryShort;
extern int recv_short;
extern int recv_mesg;
extern int recv_kmesg;
extern int recv_threshold;
extern char recv_threshold_s[];
extern int recv_warn;
extern int shortversion;

extern int godToAllOnKills;

/* ping client stuff, ick */
extern int ping;		/* to ping or not to ping */
extern long packets_sent;	/* # all packets sent to server */
extern long packets_received;	/* # all packets received */
extern W_Window pStats;

extern int lowercaset;

extern char *agriWord;

extern int F_feature_packets;
extern int why_dead;
extern int cloakerMaxWarp;
extern int F_dead_warp;
extern int F_multiline_enabled;
extern int F_UseNewMacro;
extern int F_terrain;
extern unsigned char F_terrain_major;
extern unsigned char F_terrain_minor;
extern int F_gz_motd;
extern unsigned char F_gz_motd_major;
extern unsigned char F_gz_motd_minor;
extern int F_armies_shipcap;

extern int askforUpdate;

extern int jubileePhasers;
extern int enemyPhasers;

extern int scrollBeep;

extern int recordGame;
extern char *recordFile;
extern int maxRecord;

extern int playback;
extern char *playFile;
extern int pb_update, paused, pb_advance, pb_scan, pb_slow;

extern int redrawDelay;

extern int localShipStats;
extern char *statString;
extern int statHeight, localStatsX, localStatsY;

extern int showIND;

extern struct teaminfo_s *teaminfo;
extern int number_of_teams;

extern char CLIENTVERS[];

#endif
