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

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zlib.h>
#include "config.h"
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include "proto.h"
#include "ntserv.h"
#include "data.h"
#include "shmem.h"

/* comment this out to disable it */
#undef DOUBLE_UDP

/* for local stress testing; drops 20% of packets */
#undef BROKEN

/* combine with BROKEN; drops 90% of packets */
#undef HOSED

static void forceUpdate P((void));
static int  isCensured P((char *));
static int  parseIgnore P((struct mesg_cpacket *));
static int  parseQuery P((struct mesg_spacket *));
static int  bouncePingStats P((struct mesg_spacket *));
static void bounce P((char *, int));


static void    handleTorpReq(), handlePhasReq(), handleSpeedReq();
static void    handleDirReq(), handleShieldReq(), handleRepairReq();
static void    handleOrbitReq();
static void    handlePractrReq(), handleBombReq(), handleBeamReq();
static void    handleCloakReq();
static void    handleDetTReq(), handleCopilotReq();
static void    handleOutfit(), handleLoginReq();
static void    handlePlasmaReq(), handleWarReq(), handlePlanlockReq();
static void    handlePlaylockReq(), handleDetMReq();
static void    handleTractorReq(), handleRepressReq();
static void    handleCoupReq(), handleRefitReq(), handleMessageReq();
static void    handleQuitReq(), handleOptionsPacket();
static void    handleSocketReq(), handleByeReq();
static void    handleDockingReq(), handleReset();
static void    handleUpdatesReq(), handleReserved();
static void    handleScan();
static void    handleUdpReq(), handleSequence();
static void    handleAskMOTD();
static void    handleRSAKey();
static void    handlePingResponse();
static void	handleShortReq(), handleThresh(), handleSMessageReq();

static int remoteaddr = -1;	/* inet address in net format */

extern int ignored[MAXPLAYER];

static struct packet_handler handlers[] = {
    {0},			/* record 0 */
    {handleMessageReq},		/* CP_MESSAGE */
    {handleSpeedReq},		/* CP_SPEED */
    {handleDirReq},		/* CP_DIRECTION */
    {handlePhasReq},		/* CP_PHASER */
    {handlePlasmaReq},		/* CP_PLASMA */
    {handleTorpReq},		/* CP_TORP */
    {handleQuitReq},		/* CP_QUIT */
    {handleLoginReq},		/* CP_LOGIN */
    {handleOutfit},		/* CP_OUTFIT */
    {handleWarReq},		/* CP_WAR */
    {handlePractrReq},		/* CP_PRACTR */
    {handleShieldReq},		/* CP_SHIELD */
    {handleRepairReq},		/* CP_REPAIR */
    {handleOrbitReq},		/* CP_ORBIT */
    {handlePlanlockReq},	/* CP_PLANLOCK */
    {handlePlaylockReq},	/* CP_PLAYLOCK */
    {handleBombReq},		/* CP_BOMB */
    {handleBeamReq},		/* CP_BEAM */
    {handleCloakReq},		/* CP_CLOAK */
    {handleDetTReq},		/* CP_DET_TORPS */
    {handleDetMReq},		/* CP_DET_MYTORP */
    {handleCopilotReq},		/* CP_COPLIOT */
    {handleRefitReq},		/* CP_REFIT */
    {handleTractorReq},		/* CP_TRACTOR */
    {handleRepressReq},		/* CP_REPRESS */
    {handleCoupReq},		/* CP_COUP */
    {handleSocketReq},		/* CP_SOCKET */
    {handleOptionsPacket},	/* CP_OPTIONS */
    {handleByeReq},		/* CP_BYE */
    {handleDockingReq},		/* CP_DOCKPERM */
    {handleUpdatesReq},		/* CP_UPDATES */
    {handleReset},		/* CP_RESETSTATS */
    {handleReserved},		/* CP_RESERVED */
    {handleScan},		/* CP_SCAN (ATM) */
    {handleUdpReq},		/* CP_UDP_REQ */
    {handleSequence},		/* CP_SEQUENCE */
    {handleRSAKey},		/* CP_RSA_KEY */
    {handleAskMOTD},		/* CP_ASK_MOTD */
    {0},
    {0},
    {0},
    {handlePingResponse},	/* CP_PING_RESPONSE */
    {handleShortReq},		/* CP_S_REQ */
    {handleThresh},		/* CP_S_THRS */
    {handleSMessageReq},	/* CP_S_MESSAGE */
    {0},			/* CP_S_RESERVED */
    {0},			/* CP_S_DUMMY */
    {0},			/* 48 */
    {0},			/* 49 */
    {0},			/* 50 */
    {0},			/* 51 */
    {0},			/* 52 */
    {0},			/* 53 */
    {0},			/* 54 */
    {0},			/* 55 */
    {0},			/* 56 */
    {0},			/* 57 */
    {0},			/* 58 */
    {0},			/* 59 */
    {handleFeature}, 		/* CP_FEATURE */
    {0}
};
#define NUM_PACKETS (sizeof(handlers) / sizeof(*handlers) - 1)

static int     packetsReceived[256] = {0};
static int     packetsSent[256] = {0};

int     clientDead = 0;	/* used in sockio.c as well */

static int udpLocalPort = 0;
static int udpClientPort = 0;

int udpMode = MODE_SIMPLE;	/* what kind of UDP trans we want */

/* needed for fast lookup of semi-critical fat nodes */
extern FAT_NODE fat_kills[MAXPLAYER];
extern FAT_NODE fat_torp_info[MAXPLAYER * MAXTORP];
extern FAT_NODE fat_thingy_info[TOTALTHINGIES];
extern FAT_NODE fat_phaser[MAXPLAYER];
extern FAT_NODE fat_plasma_info[MAXPLAYER * MAXPLASMA];
extern FAT_NODE fat_you;
extern FAT_NODE fat_status2;
extern FAT_NODE fat_planet2[MAXPLANETS];
extern FAT_NODE fat_flags[MAXPLAYER];
extern FAT_NODE fat_hostile[MAXPLAYER];

static struct plyr_info_spacket clientPlayersInfo[MAXPLAYER];
static struct plyr_login_spacket clientLogin[MAXPLAYER];
static struct hostile_spacket clientHostile[MAXPLAYER];
static struct stats_spacket clientStats[MAXPLAYER];
static struct player_spacket clientPlayers[MAXPLAYER];
static struct kills_spacket clientKills[MAXPLAYER];
static struct flags_spacket clientFlags[MAXPLAYER];
static struct pstatus_spacket clientPStatus[MAXPLAYER];
static int     msgCurrent;
static struct torp_info_spacket clientTorpsInfo[MAXPLAYER * MAXTORP];
static struct torp_spacket clientTorps[MAXPLAYER * MAXTORP];
static struct thingy_info_spacket clientThingysInfo[TOTALTHINGIES];
static struct thingy_spacket clientThingys[TOTALTHINGIES];
static int     clientThingyStatus[TOTALTHINGIES];
static struct phaser_spacket clientPhasers[MAXPLAYER];
static struct you_spacket clientSelf;
static struct pe1_num_missiles_spacket clientMissiles;
static struct planet_loc_spacket clientPlanetLocs[MAXPLANETS];
static struct plasma_info_spacket clientPlasmasInfo[MAXPLAYER * MAXPLASMA];
static struct plasma_spacket clientPlasmas[MAXPLAYER * MAXPLASMA];
static int     mustUpdate[MAXPLAYER];
static struct status_spacket2 clientStatus2;	/* new stats packets */
static struct stats_spacket2 clientStats2[MAXPLAYER];
static struct planet_spacket2 clientPlanets2[MAXPLANETS];

static int		send_threshold	= 0;	/* infinity */
static int		send_short	= 0;
static int		send_mesg 	= 1;
static int		send_kmesg	= 1;
static int		send_warn	= 1;
static int		spk_update_sall = 0; /* Small Update: Only weapons, Kills and Planets */
static int		spk_update_all = 0; /* Full Update minus SP_STATS */

#define         SPK_VOFF         0               /* variable packets off */
#define         SPK_VON          1               /* variable packets on */
#define         SPK_MOFF         2               /* message packets off */
#define         SPK_MON          3               /* message packets on */
#define 	SPK_M_KILLS	 4
#define 	SPK_M_NOKILLS	 5
#define		SPK_THRESHOLD	 6
#define		SPK_M_WARN	 7
#define		SPK_M_NOWARN	 8
#define	SPK_SALL 9		/* only planets,kills and weapons */
#define 	SPK_ALL 10	/* Full Update - SP_STATS */

int
connectToClient(char *machine, int port)
{
    int     ns;
    struct sockaddr_in addr;
    struct hostent *hp;
    /* int len,cc; */
    /* char buf[BUFSIZ]; */
    /* int i; */

    if (sock != -1) {
	shutdown(sock, 2);
	sock = -1;
    }
    /* printf("Connecting to %s through %d\n", machine, port); */

    if ((ns = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	printf("I cannot create a socket\n");
	exit(2);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (remoteaddr != -1) {
	addr.sin_addr.s_addr = remoteaddr;
    }
    else if ((addr.sin_addr.s_addr = inet_addr(machine)) == -1) {
	if ((hp = gethostbyname(machine)) == NULL) {
	    printf("I cannot get host name\n");
	    close(ns);
	    exit(1);
	}
	else {
	    memcpy((char*)&addr.sin_addr, hp->h_addr, hp->h_length);
	}
    }
    remoteaddr = addr.sin_addr.s_addr;

    if (connect(ns, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
	/* printf("I cannot connect through port %d\n", port); */
	close(ns);
	return (0);
    }
    sock = ns;
    initClientData();
    testtime = -1;
    return (1);
}

/* Check the socket to read it's inet addr for possible future use.
 */
void
checkSocket(void)
{
    struct sockaddr_in sin;
    int     length;

    length = sizeof(sin);
    if (getpeername(sock, (struct sockaddr *) & sin, &length) < 0) {
	/* A bad thing. */
	return;
    }
    remoteaddr = sin.sin_addr.s_addr;
}

void
initClientData(void)
/* invalidates all data, so it is all sent to the client */
{
    int     i;

    clientDead = 0;
    clientMissiles.num = 0;
    for (i = 0; i < MAXPLAYER; i++) {
	clientHostile[i].hostile = -1;
	clientStats[i].losses = -1;
	clientLogin[i].rank = -1;
	clientPlayersInfo[i].shiptype = -1;
	clientPStatus[i].status = -1;
	clientPlayers[i].x = htonl(-1);
	clientPhasers[i].status = -1;
	clientKills[i].kills = htonl(-1);
	clientFlags[i].flags = htonl(-1);
	mustUpdate[i] = 0;

	fat_hostile[i].packet = (PTR) & clientHostile[i];
	fat_hostile[i].pkt_size = sizeof(struct hostile_spacket);
	fat_hostile[i].prev = fat_hostile[i].next = (FAT_NODE *) NULL;
	fat_phaser[i].packet = (PTR) & clientPhasers[i];
	fat_phaser[i].pkt_size = sizeof(struct phaser_spacket);
	fat_phaser[i].prev = fat_phaser[i].next = (FAT_NODE *) NULL;
	fat_kills[i].packet = (PTR) & clientKills[i];
	fat_kills[i].pkt_size = sizeof(struct kills_spacket);
	fat_kills[i].prev = fat_kills[i].next = (FAT_NODE *) NULL;
	fat_flags[i].packet = (PTR) & clientFlags[i];
	fat_flags[i].pkt_size = sizeof(struct flags_spacket);
	fat_flags[i].prev = fat_flags[i].next = (FAT_NODE *) NULL;
    }
    for (i = 0; i < MAXPLAYER * MAXTORP; i++) {
	clientTorpsInfo[i].status = -1;
	clientTorps[i].x = -1;

	fat_torp_info[i].packet = (PTR) & clientTorpsInfo[i];
	fat_torp_info[i].pkt_size = sizeof(struct torp_info_spacket);
	fat_torp_info[i].prev = fat_torp_info[i].next = (FAT_NODE *) NULL;
    }
    for (i = 0; i < TOTALTHINGIES; i++) {
	clientThingysInfo[i].shape = htons(-1);
	clientThingysInfo[i].owner = htons(i / NPTHINGIES);
	clientThingys[i].x = htonl(-1);

	fat_thingy_info[i].packet = (PTR) & clientThingysInfo[i];
	fat_thingy_info[i].pkt_size = sizeof(struct torp_info_spacket);
	fat_thingy_info[i].prev = fat_thingy_info[i].next = (FAT_NODE *) NULL;
    }
    for (i = 0; i < MAXPLAYER * MAXPLASMA; i++) {
	clientPlasmasInfo[i].status = -1;
	clientPlasmas[i].x = -1;

	fat_plasma_info[i].packet = (PTR) & clientPlasmasInfo[i];
	fat_plasma_info[i].pkt_size = sizeof(struct plasma_info_spacket);
	fat_plasma_info[i].prev = fat_plasma_info[i].next = (FAT_NODE *) NULL;
    }
    for (i = 0; i < MAXPLANETS; i++) {
	clientPlanets2[i].armies = htonl(-1);
	clientPlanetLocs[i].x = htonl(-1);

	fat_planet2[i].packet = (PTR) & clientPlanets2[i];
	fat_planet2[i].pkt_size = sizeof(struct planet_spacket2);
	fat_planet2[i].prev = fat_planet2[i].next = (FAT_NODE *) 0;
    }
    msgCurrent = (mctl->mc_current + 1) % MAXMESSAGE;
    clientSelf.pnum = -1;

    fat_you.packet = (PTR) & clientSelf;
    fat_you.pkt_size = sizeof(struct you_spacket);
    fat_you.prev = fat_you.next = (FAT_NODE *) NULL;
    fat_status2.packet = (PTR) & clientStatus2;
    fat_status2.pkt_size = sizeof(struct status_spacket2);
    fat_status2.prev = fat_status2.next = (FAT_NODE *) 0;

    reset_fat_list();
}

int
isClientDead(void)
{
    return (clientDead);
}

void
sendQueuePacket(short int pos)
{
    struct queue_spacket qPacket;

    qPacket.type = SP_QUEUE;
    qPacket.pos = htons(pos);
    sendClientPacket((struct player_spacket *) & qPacket);
    flushSockBuf();
}

static void
sendClientSizedPacket(struct player_spacket *packet, int size)
 /* Pick a random type for the packet */
{
    int     orig_type;
    int     issc;
    static int oldStatus = POUTFIT;

#ifdef T_DIAG2
    if( (packet->type == SP_TERRAIN2) || (packet->type == SP_TERRAIN_INFO2) ){
      pmessage( "Sending TERRAIN packet\n", 0, MALL, MSERVA );
    }
#endif

    orig_type = packet->type;
    packet->type &= (char) 0x3f;

    /* if we're not given the size, calculate it */
    if (size<0) {
	if (size_of_spacket(packet) == 0) {
	    printf("Attempt to send strange packet %d\n", packet->type);
	    return;
	}
	size = size_of_spacket(packet);
    } else {
	/* pad to 32-bits */
	size = ((size-1)/4 + 1) * 4;
    }

#ifdef SHOW_PACKETS
    {
	FILE   *logfile;
	char   *paths;
	paths = build_path("logs/metaserver.log");
	logfile=fopen(paths, "a");
	if (logfile) {
	    fprintf(logfile, "Sending packet type %d (size %d)\n", (int)packet->type, size);
	    fclose(logfile);
        }

    }
#endif

    packetsSent[packet->type]++;


    if (commMode == COMM_TCP
	|| (commMode == COMM_UDP && udpMode == MODE_TCP)) {
	switch(orig_type) {
	case SP_MOTD:
	case SP_MOTD_PIC:
	    /* these can afford to be delayed */
	    sendTCPdeferred((void*)packet, size);
	    break;

	default:
	    /* business as usual, TCP */
	    sendTCPbuffered((void*)packet, size);
	    break;
	}
    }
    else {
	/*
	   do UDP stuff unless it's a "critical" packet (note that both kinds
	   get a sequence number appended) (FIX)
	*/
	issc = 0;
	switch (orig_type) {
	case SP_KILLS:
	case SP_TORP_INFO:
	case SP_THINGY_INFO:
	case SP_PHASER:
	case SP_PLASMA_INFO:
	case SP_YOU | 0x40:	/* ??? what is this? */
	case SP_STATUS:
	case SP_STATUS2:
	case SP_PLANET:
	case SP_PLANET2:
	case SP_FLAGS:
	case SP_HOSTILE:
	    /*
	       these are semi-critical; flag as semi-critical and fall
	       through
	    */
	    issc = 1;

	case SP_PLAYER:
	case SP_TORP:
	case SP_S_TORP:
	case SP_S_8_TORP:
	case SP_THINGY:
	case SP_YOU:
	case SP_PLASMA:
	case SP_STATS:
	case SP_STATS2:
/*	case SP_SCAN:*/
	case SP_PING:
	case SP_UDP_REPLY:	/* only reply when COMM_UDP is SWITCH_VERIFY */
	    /* these are non-critical updates; send them via UDP */
	    V_UDPDIAG(("Sending type %d\n", packet->type));
	    packets_sent++;
	    sendUDPbuffered(issc, (void*)packet, size);
	    break;

	case SP_MOTD:
	case SP_MOTD_PIC:
	    sendTCPdeferred((void*)packet, size);
	    break;

	default:
	    sendTCPbuffered((void*)packet, size);
	    break;
	}
    }

    if (me != NULL)
	oldStatus = me->p_status;
}

void
sendClientPacket(struct player_spacket *packet)
{
    sendClientSizedPacket(packet, -1);
}

static void
updateTorpInfos(void)
{
    register struct torp *torp;
    register int i;
    register struct torp_info_spacket *tpi;

    for (i = 0, torp = torps, tpi = clientTorpsInfo;
	 i < MAXPLAYER * MAXTORP;
	 i++, torp++, tpi++) {
	if (torp->t_owner == me->p_no) {
	    if (torp->t_war != tpi->war ||
		torp->t_status != tpi->status) {
		tpi->type = SP_TORP_INFO;
		tpi->war = torp->t_war;
		tpi->status = torp->t_status;
		tpi->tnum = htons(i);
		sendClientPacket((struct player_spacket *) tpi);
	    }
	}
	else {			/* Someone else's torp... */
	    if (torp->t_y > me->p_y + SCALE * WINSIDE / 2 ||
		torp->t_x > me->p_x + SCALE * WINSIDE / 2 ||
		torp->t_x < me->p_x - SCALE * WINSIDE / 2 ||
		torp->t_y < me->p_y - SCALE * WINSIDE / 2 ||
		torp->t_status == TFREE) {
		if (torp->t_status == TFREE && tpi->status == TEXPLODE) {
		    tpi->status = TFREE;
		    continue;
		}
		if (tpi->status != TFREE) {
		    tpi->status = TFREE;
		    tpi->tnum = htons(i);
		    tpi->type = SP_TORP_INFO;
		    sendClientPacket((struct player_spacket *) tpi);
		}
	    }
	    else {		/* in view */
		enum torp_status_e tstatus = torp->t_status;

		if (status2->paused
		    && players[torp->t_owner].p_team != me->p_team)
		    tstatus = TFREE;	/* enemy torps are invisible during
					   game pause */

		if (tstatus != tpi->status ||
		    (torp->t_war & me->p_team) != tpi->war) {
		    /* Let the client fade away the explosion on its own */
		    tpi->war = torp->t_war & me->p_team;
		    tpi->type = SP_TORP_INFO;
		    tpi->tnum = htons(i);
		    tpi->status = tstatus;
		    sendClientPacket((struct player_spacket *) tpi);
		}
	    }
	}
    }
}

static void
updateTorps(void)
{
    register struct torp *torp;
    register int i;
    register struct torp_spacket *tp;

    for (i = 0, torp = torps, tp = clientTorps;
	 i < MAXPLAYER * MAXTORP;
	 i++, torp++, tp++) {
	if (torp->t_owner == me->p_no) {

	    if (tp->x != htonl(torp->t_x) ||
		tp->y != htonl(torp->t_y)) {
		tp->type = SP_TORP;
		tp->x = htonl(torp->t_x);
		tp->y = htonl(torp->t_y);
		tp->dir = torp->t_dir;
		tp->tnum = htons(i);
		sendClientPacket((struct player_spacket *) tp);
	    }
	}
	else {			/* Someone else's torp... */
	    if (torp->t_y > me->p_y + SCALE * WINSIDE / 2 ||
		torp->t_x > me->p_x + SCALE * WINSIDE / 2 ||
		torp->t_x < me->p_x - SCALE * WINSIDE / 2 ||
		torp->t_y < me->p_y - SCALE * WINSIDE / 2 ||
		torp->t_status == TFREE) {
	      /* do nothing */
	    }
	    else {		/* in view */
		enum torp_status_e tstatus = torp->t_status;

		if (status2->paused
		    && players[torp->t_owner].p_team != me->p_team)
		    tstatus = TFREE;	/* enemy torps are invisible during
					   game pause */

		if (tstatus==TFREE)
		    continue;	/* no need to transmit position */

		if (tp->x != htonl(torp->t_x) ||
		    tp->y != htonl(torp->t_y)) {
		    tp->x = htonl(torp->t_x);
		    tp->y = htonl(torp->t_y);
		    tp->dir = torp->t_dir;
		    tp->tnum = htons(i);
		    tp->type = SP_TORP;
		    sendClientPacket((struct player_spacket *) tp);
		}
	    }
	}
    }
}

#define NIBBLE()	*(*data)++ = (torp->t_war & 0xf) | (torp->t_status << 4)

static int
encode_torp_status(struct torp *torp, int pnum, char **data, 
                   struct torp_info_spacket *tpi, 
		   struct torp_spacket *tp, 
		   int *mustsend)
{
    if (pnum!=me->p_no) {
	int	dx,dy;
	int	i = SCALE*WINSIDE/2;
	dx = me->p_x-torp->t_x;
	dy = me->p_y-torp->t_y;
	if (dx<-i  || dx > i || dy < -i || dy > i || torp->t_status==TFREE) {
	    if (torp->t_status==TFREE && tpi->status==TEXPLODE)
		tpi->status=TFREE;
	    else if (tpi->status != TFREE) {
		tpi->status=TFREE; 
		*mustsend=1;
	    }
	    return 0;
	}
    }

    if (torp->t_war != tpi->war) {
	tpi->war = torp->t_war;
	tpi->status = torp->t_status;
	NIBBLE();
	return 1;
    } else if (torp->t_status != tpi->status) {
	switch (torp->t_status) {
	case TFREE:
	    {
		int	rval=0;
		if (tpi->status==TEXPLODE) {
		    NIBBLE();
		    rval = 1;
		} else 
		    *mustsend=1;
		tpi->status = torp->t_status;
		tp->x = htonl(torp->t_x);
		tp->y = htonl(torp->t_y);
		return rval;
	    }
	    break;
	case TMOVE:
	case TSTRAIGHT:
	    tpi->status = torp->t_status;
	    break;
	default:
	    NIBBLE();
	    tpi->status = torp->t_status;
	    return 1;
	    break;
	}
    }
    return 0;
}

#define	TORP_INVISIBLE(tstatus) ( \
				 (tstatus)== TFREE || \
				 (tstatus) == TOFF || \
				 (tstatus) == TLAND)

static int
encode_torp_position(struct torp *torp, int pnum, char **data, int *shift, 
                     struct torp_spacket *cache)
{
    int	x,y;

    if (htonl(torp->t_x) == cache->x &&
	htonl(torp->t_y) == cache->y)
	return 0;

    cache->x = htonl(torp->t_x);
    cache->y = htonl(torp->t_y);

    if (TORP_INVISIBLE(torp->t_status)
	|| ( status2->paused &&
	    players[pnum].p_team != me->p_team)
	)
	return 0;

    x = torp->t_x/SCALE - me->p_x/SCALE + WINSIDE/2;
    y = torp->t_y/SCALE - me->p_y/SCALE + WINSIDE/2;

    if (x<0 || x >=WINSIDE ||
	y<0 || y >=WINSIDE) {
	if (pnum != me->p_no)
	    return 0;
	x = y = 501;
    }

    **data |= x<<*shift;
    *(++(*data)) = (0x1ff&x)>> (8-*shift);
    (*shift)++;
    **data |= y<<*shift;
    *(++(*data)) = (0x1ff&y)>> (8-*shift);
    (*shift)++;
    if (*shift==8) {
	*shift=0;
	*(++(*data))=0;
    }
    return 1;
}

static void
short_updateTorps(void)
{
    register int i;

    for (i = 0; i <= (MAXPLAYER*MAXTORP-1)/8; i++) {
	struct torp *torp = &torps[i*8];
	int	j;
	char	packet[2	/* packet type and player number */
		       +1	/* torp mask */
		       +1	/* torp info mask */
		       +18	/* 2*8 9-bit numbers */
                       +8       /* 8 torp info bytes */
		       ];
	char	*data = packet+4;
	char	info[8];
	char	*ip=info;
	int	shift=0;
	int	torppos_mask = 0;
	int	torpinfo_mask = 0;
	int	mustsend;

	/* encode screen x and y coords */
	data[0]=0;
#define TIDX	(j+i*8)
#define PNUM	((int)((j+i*8)/MAXTORP))
	for (j=0; j<8 && TIDX<MAXPLAYER*MAXTORP; j++) {
	    torpinfo_mask |= encode_torp_status
		(&torp[j], PNUM, &ip, &clientTorpsInfo[TIDX], &clientTorps[TIDX], &mustsend) << j;
	    torppos_mask |= encode_torp_position
		(&torp[j], PNUM, &data, &shift, &clientTorps[TIDX]) << j;
	}

/*	if (!torppos_mask)
	    continue; */

	if (torpinfo_mask) {
	    if (shift)
		data++;
	    for (j=0; j<8 && &info[j] < ip; j++)
		data[j] = info[j];
	    packet[0] = SP_S_TORP_INFO;
	    packet[1] = torppos_mask;
	    packet[2] = i;
	    packet[3] = torpinfo_mask;
	    sendClientSizedPacket(packet, (data-packet + j));
	} else if (torppos_mask==0xff) {
	    /* what a disgusting hack */
	    packet[2] = SP_S_8_TORP;
	    packet[3] = i;
	    sendClientSizedPacket(packet+2, 20);
	} else if (mustsend || torppos_mask != 0) {
	    packet[1] = SP_S_TORP;
	    packet[2] = torppos_mask&0xff;
	    packet[3] = i;
	    sendClientSizedPacket(packet+1, (data-(packet+1) + (shift!=0)));
	}
    }
#undef PNUM
#undef TIDX
}

static void
updateMissiles(void)
{
    register struct missile *missile;
    register int i;
    register struct thingy_info_spacket *dpi;
    register struct thingy_spacket *dp;

    for (i = 0, missile = missiles, dpi = clientThingysInfo, dp = clientThingys;
	 i < MAXPLAYER * NPTHINGIES;
	 i++, missile++, dpi++, dp++) {
	enum torp_status_e msstatus = missile->ms_status;

	if (status2->paused &&
	    players[missile->ms_owner].p_team != me->p_team)
	    msstatus = TFREE;	/* enemy torps are invisible during game
				   pause */

	switch (msstatus) {
	case TFREE:
	case TLAND:
	case TOFF:
	    dpi->shape = htons(SHP_BLANK);
	    break;
	case TMOVE:
	case TRETURN:
	case TSTRAIGHT:
	    dpi->shape = htons((missile->ms_type == FIGHTERTHINGY)
			       ? SHP_FIGHTER
			       : SHP_MISSILE);
	    break;
	case TEXPLODE:
	case TDET:
	    dpi->shape = htons(SHP_PBOOM);
	    break;
	}

	dpi->type = SP_THINGY_INFO;
	dpi->tnum = htons(i);

	dp->type = SP_THINGY;
	dp->tnum = htons(i);
	dp->dir = missile->ms_dir;

	if (missile->ms_owner == me->p_no) {
	    if (missile->ms_war != dpi->war ||
		msstatus != clientThingyStatus[i]) {
		dpi->war = missile->ms_war;
		clientThingyStatus[i] = msstatus;
		sendClientPacket((struct player_spacket *) dpi);
	    }
	    if (dp->x != htonl(missile->ms_x) ||
		dp->y != htonl(missile->ms_y)) {
		dp->x = htonl(missile->ms_x);
		dp->y = htonl(missile->ms_y);
		/* printf("missile at %d,%d\n", dp->x, dp->y); */
		sendClientPacket((struct player_spacket *) dp);
	    }
	}
	else {			/* Someone else's missile... */
	    if (msstatus == TFREE ||
		missile->ms_y > me->p_y + SCALE * WINSIDE / 2 ||
		missile->ms_x > me->p_x + SCALE * WINSIDE / 2 ||
		missile->ms_x < me->p_x - SCALE * WINSIDE / 2 ||
		missile->ms_y < me->p_y - SCALE * WINSIDE / 2) {
		if (msstatus == TFREE && clientThingyStatus[i] == TEXPLODE) {
		    clientThingyStatus[i] = TFREE;
		    continue;
		}
		if (clientThingyStatus[i] != TFREE) {
		    clientThingyStatus[i] = TFREE;
		    dpi->shape = htons(SHP_BLANK);
		    sendClientPacket((struct player_spacket *) dpi);
		}
	    }
	    else {		/* in view */
		if (dp->x != htonl(missile->ms_x) ||
		    dp->y != htonl(missile->ms_y)) {
		    dp->x = htonl(missile->ms_x);
		    dp->y = htonl(missile->ms_y);
		    sendClientPacket((struct player_spacket *) dp);
		}
		if (msstatus != clientThingyStatus[i] ||
		    (missile->ms_war & me->p_team) != dpi->war) {
		    /* Let the client fade away the explosion on its own */
		    dpi->war = missile->ms_war & me->p_team;
		    clientThingyStatus[i] = msstatus;
		    sendClientPacket((struct player_spacket *) dpi);
		}
	    }
	}
    }
}

static void 
fill_thingy_info_packet(struct thingy *thing, 
                        struct thingy_info_spacket *packet)
{
    switch (thing->type) {
    case TT_NONE:
	packet->war = 0;
	packet->shape = htons(SHP_BLANK);
	packet->owner = 0;
	break;
    case TT_WARP_BEACON:
	packet->war = 0;
	if (thing->u.wbeacon.owner == me->p_team) {
	    packet->shape = htons(SHP_WARP_BEACON);
	    packet->owner = htons(thing->u.wbeacon.owner);
	}
	else {
	    packet->shape = htons(SHP_BLANK);
	    packet->owner = 0;
	}
	break;
    default:
	printf("Unknown thingy type: %d\n", (int) thing->type);
	break;
    }
}

static void 
fill_thingy_packet(struct thingy *thing, 
                   struct thingy_spacket *packet)
{
    switch (thing->type) {
    case TT_NONE:
	packet->dir = 0;
	packet->x = packet->y = htonl(0);
	break;
    case TT_WARP_BEACON:
	packet->dir = 0;
	if (thing->u.wbeacon.owner == me->p_team) {
	    packet->x = htonl(thing->u.wbeacon.x);
	    packet->y = htonl(thing->u.wbeacon.y);
	}
	else {
	    packet->x = packet->y = htonl(0);
	}
	break;
    default:
	printf("Unknown thingy type: %d\n", (int) thing->type);
	break;
    }
}

static void 
updateThingies(void)
{
    struct thingy *thing;
    struct thingy_info_spacket *tip;
    struct thingy_spacket *tp;
    int     i;

    for (i = 0; i < NGTHINGIES; i++) {
	struct thingy_info_spacket ti1;
	struct thingy_spacket t2;

	thing = &thingies[i];
	tip = &clientThingysInfo[i + MAXPLAYER * NPTHINGIES];
	tp = &clientThingys[i + MAXPLAYER * NPTHINGIES];

	ti1.type = SP_THINGY_INFO;
	ti1.tnum = htons(i + MAXPLAYER * NPTHINGIES);
	fill_thingy_info_packet(thing, &ti1);

	if (0 != memcmp((char*)tip, (char*)&ti1, sizeof(ti1))) {
	    memcpy(tip, &ti1, sizeof(ti1));
	    sendClientPacket((struct player_spacket *) tip);
	}

	if (tip->shape != htons(SHP_BLANK)) {
	    t2.type = SP_THINGY;
	    t2.tnum = htons(i + MAXPLAYER * NPTHINGIES);
	    fill_thingy_packet(thing, &t2);

	    if (0 != memcmp(tp, &t2, sizeof(t2))) {
		memcpy(tp, &t2, sizeof(t2));
		sendClientPacket((struct player_spacket *) tp);
	    }
	}
    }

}

static void
updatePlasmas(void)
{
    register struct plasmatorp *torp;
    register int i;
    register struct plasma_info_spacket *tpi;
    register struct plasma_spacket *tp;

    for (i = 0, torp = plasmatorps, tpi = clientPlasmasInfo, tp = clientPlasmas;
	 i < MAXPLAYER * MAXPLASMA;
	 i++, torp++, tpi++, tp++) {
	if (torp->pt_owner == me->p_no) {
	    if (torp->pt_war != tpi->war ||
		torp->pt_status != tpi->status) {
		tpi->type = SP_PLASMA_INFO;
		tpi->war = torp->pt_war;
		tpi->status = torp->pt_status;
		tpi->pnum = htons(i);
		sendClientPacket((struct player_spacket *) tpi);
	    }
	    if (tp->x != htonl(torp->pt_x) ||
		tp->y != htonl(torp->pt_y)) {
		tp->type = SP_PLASMA;
		tp->x = htonl(torp->pt_x);
		tp->y = htonl(torp->pt_y);
		tp->pnum = htons(i);
		sendClientPacket((struct player_spacket *) tp);
	    }
	}
	else {			/* Someone else's torp... */
	    enum torp_status_e ptstatus = torp->pt_status;

	    if (status2->paused &&
		players[torp->pt_owner].p_team != me->p_team)
		ptstatus = TFREE;	/* enemy torps are invisible during
					   game pause */
	    if (torp->pt_y > me->p_y + SCALE * WINSIDE / 2 ||
		torp->pt_x > me->p_x + SCALE * WINSIDE / 2 ||
		torp->pt_x < me->p_x - SCALE * WINSIDE / 2 ||
		torp->pt_y < me->p_y - SCALE * WINSIDE / 2 ||
		ptstatus == PTFREE) {
		if (ptstatus == PTFREE && tpi->status == PTEXPLODE) {
		    tpi->status = PTFREE;
		    continue;
		}
		if (tpi->status != PTFREE) {
		    tpi->status = PTFREE;
		    tpi->pnum = htons(i);
		    tpi->type = SP_PLASMA_INFO;
		    sendClientPacket((struct player_spacket *) tpi);
		}
	    }
	    else {		/* in view */
		/* Send torp (we assume it moved) */
		tp->x = htonl(torp->pt_x);
		tp->y = htonl(torp->pt_y);
		tp->pnum = htons(i);
		tp->type = SP_PLASMA;
		sendClientPacket((struct player_spacket *) tp);
		if (ptstatus != tpi->status ||
		    (torp->pt_war & me->p_team) != tpi->war) {
		    tpi->war = torp->pt_war & me->p_team;
		    tpi->type = SP_PLASMA_INFO;
		    tpi->pnum = htons(i);
		    tpi->status = ptstatus;
		    sendClientPacket((struct player_spacket *) tpi);
		}
	    }
	}
    }
}

static void
updatePhasers(void)
{
    register int i;
    register struct phaser_spacket *ph;
    register struct phaser *phase;
    register struct player *pl;

    for (i = 0, ph = clientPhasers, phase = phasers, pl = players;
	 i < MAXPLAYER; i++, ph++, phase++, pl++) {
	if (pl->p_y > me->p_y + SCALE * WINSIDE / 2 ||
	    pl->p_x > me->p_x + SCALE * WINSIDE / 2 ||
	    pl->p_x < me->p_x - SCALE * WINSIDE / 2 ||
	    pl->p_y < me->p_y - SCALE * WINSIDE / 2) {
	    if (ph->status != PHFREE) {
		ph->pnum = i;
		ph->type = SP_PHASER;
		ph->status = PHFREE;
		sendClientPacket((struct player_spacket *) ph);
	    }
	}
	else {
	    if (phase->ph_status == PHHIT) {
		mustUpdate[phase->ph_target] = 1;
	    }
	    if (ph->status != phase->ph_status ||
		ph->dir != phase->ph_dir ||
		ph->target != htonl(phase->ph_target)) {
		ph->pnum = i;
		ph->type = SP_PHASER;
		ph->status = phase->ph_status;
		ph->dir = phase->ph_dir;
		ph->x = htonl(phase->ph_x);
		ph->y = htonl(phase->ph_y);
		ph->target = htonl(phase->ph_target);
		sendClientPacket((struct player_spacket *) ph);
	    }
	}
    }
}


#define PLFLAGMASK (PLRESMASK|PLATMASK|PLSURMASK|PLPARADISE|PLTYPEMASK)

void
updatePlanets(void)
{
    register int i;
    register struct planet *plan;
    register struct planet_loc_spacket *pll;
    register struct planet_spacket2 *pl;
    int     dx, dy;
    int     d2x, d2y;
    char   *name;

    for (i = 0, pl = clientPlanets2, plan = planets, pll = clientPlanetLocs;
	 i < NUMPLANETS;
	 i++, pl++, plan++, pll++) {
	/*
	   Send him info about him not having info if he doesn't but thinks
	   he does. Also send him info on every fifth cycle if the planet
	   needs to be redrawn.
	*/
	if (((plan->pl_hinfo & me->p_team) == 0) && (pl->info & me->p_team)) {
	    pl->type = SP_PLANET2;
	    pl->pnum = i;
	    pl->info = 0;
	    pl->flags = PLPARADISE;
	    sendClientPacket((struct player_spacket *) pl);
	}
	else {
	    struct teaminfo temp, *ptr;
	    if (configvals->durablescouting) {
		temp.owner = plan->pl_owner;
		temp.armies = plan->pl_armies;
		temp.flags = plan->pl_flags;
		temp.timestamp = status->clock;
		ptr = &temp;
	    }
	    else
		ptr = &plan->pl_tinfo[me->p_team];

	    if (pl->info != plan->pl_hinfo ||
		pl->armies != htonl(ptr->armies) ||
		pl->owner != ptr->owner ||
		pl->flags != htonl(ptr->flags & PLFLAGMASK)
		|| ((pl->timestamp != htonl(ptr->timestamp))
		    && (me->p_team != plan->pl_owner))) {
		pl->type = SP_PLANET2;
		pl->pnum = (char) i;
		pl->info = (char) plan->pl_hinfo;
		pl->flags = htonl(ptr->flags & PLFLAGMASK);
		pl->armies = htonl(ptr->armies);
		pl->owner = (char) ptr->owner;
		pl->timestamp = htonl(ptr->timestamp);
		sendClientPacket((struct player_spacket *) pl);
	    }
	}
	/* Assume that the planet only needs to be updated once... */

	/* Odd, changes in pl_y not supported.  5/31/92 TC */

	dx = ntohl(pll->x) - plan->pl_x;
	if (dx < 0)
	    dx = -dx;
	dy = ntohl(pll->y) - plan->pl_y;
	if (dy < 0)
	    dy = -dy;

	d2x = plan->pl_x - me->p_x;
	d2y = plan->pl_y - me->p_y;
	if (d2x < 0)
	    d2x = -d2x;
	if (d2y < 0)
	    d2y = -d2y;

	if (1 || plan->pl_hinfo & me->p_team)
	    name = plan->pl_name;
	else
	    name = "   ";
	/*
	   if ((pll->x != htonl(plan->pl_x)) || (pll->y !=
	   htonl(plan->pl_y))) {
	*/
	if (strcmp((char *) pll->name, name) ||
	    ((dx > 400 || dy > 400) ||
	     ((dx >= 20 || dy >= 20) && (d2x < 10000 && d2y < 10000)))) {
	    pll->x = htonl(plan->pl_x);
	    pll->y = htonl(plan->pl_y);
	    pll->pnum = i;
	    if (plan->pl_system == 0)
		pll->pad2 = 255;
	    else
		pll->pad2 = (char) stars[plan->pl_system];
	    strcpy((char *) pll->name, name);
	    pll->type = SP_PLANET_LOC;
	    sendClientPacket((struct player_spacket *) pll);
	}
    }
}

static void
updateMessages(void)
{
    int     i;
    struct message *cur;
    struct mesg_spacket msg;

    for (i = msgCurrent; i != (mctl->mc_current + 1) % MAXMESSAGE; i = (i + 1) % MAXMESSAGE) {
	if (i == MAXMESSAGE)
	    i = 0;
	cur = &messages[i];

	if (cur->m_flags & MVALID &&
	    (cur->m_flags & MALL ||
	     (cur->m_flags & MTEAM && cur->m_recpt == me->p_team) ||
	     (cur->m_flags & MINDIV && cur->m_recpt == me->p_no) ||
	     (cur->m_flags & MGOD && cur->m_from == me->p_no))) {
	    msg.type = SP_MESSAGE;
	    strncpy(msg.mesg, cur->m_data, 80);
	    msg.mesg[79] = '\0';
	    msg.m_flags = cur->m_flags;
	    msg.m_recpt = cur->m_recpt;
	    msg.m_from = cur->m_from;

	    if ((cur->m_from < 0)
		|| (cur->m_from > MAXPLAYER)
		|| (cur->m_flags & MGOD && cur->m_from == me->p_no)
		|| (cur->m_flags & MALL && !(ignored[cur->m_from] & MALL))
		|| (cur->m_flags & MTEAM && !(ignored[cur->m_from] & MTEAM)))
		sendClientPacket((struct player_spacket *) & msg);
	    else if (cur->m_flags & MINDIV) {

		/* session stats now parsed here.  parseQuery == true */
		/* means eat message 4/17/92 TC */

		if (!parseQuery(&msg))
		{
		    if (ignored[cur->m_from] & MINDIV)
			bounce("That player is currently ignoring you.",
			       cur->m_from);
		    else
			sendClientPacket((struct player_spacket *) & msg);
		}
	    }
	}
	msgCurrent = (msgCurrent + 1) % MAXMESSAGE;
    }
}

/* Asteroid/Nebulae socket code (5/16/95 rpg) */

#define DIM (MAX_GWIDTH/TGRID_GRANULARITY)

void
updateTerrain(void){
    int i;
    int j, maxfor;
#if defined(T_DIAG) || defined(T_DIAG2)
    int status;
#endif
    int npkts;
    struct terrain_packet2 tpkt;
    struct terrain_info_packet2 tinfo_pkt;
    unsigned long olen = DIM*DIM, dlen = DIM*DIM/1000+DIM*DIM+13;
#if defined(T_DIAG) || defined(T_DIAG2)
    char buf[80];
#endif
    unsigned char origBuf[DIM*DIM];
    unsigned char gzipBuf[DIM*DIM/1000+
		          DIM*DIM+13];
    /* Don't ask me.  The compression libs need (original size + 0.1% +
       12 bytes). */
    /* Note - this will have to be RADICALLY changed if alt1/alt2 are sent 
       as well. */

  /* check to see if client can handle the terrain data first */
  if( F_terrain ){
    if( galaxyValid[me->p_no] )	{	/* if we've already submitted a galaxy to client.. */
      return;				/* ... then don't do anything */
    }
     
#if defined(T_DIAG) || defined(T_DIAG2)
    { 
       char buf[80];
       sprintf( buf, "pno: %d gIsense: %d", me->p_no, galaxyValid[me->p_no] );
       pmessage( buf, 0, MALL, MSERVA );
    }
#endif
    galaxyValid[me->p_no] = 1;
    /* Send initial packet. */
    tinfo_pkt.type = SP_TERRAIN_INFO2;
    tinfo_pkt.xdim = htons(DIM);
    tinfo_pkt.ydim = htons(DIM);
    sendClientPacket( (struct player_spacket *) &tinfo_pkt );
    for( i = 0; i < DIM*DIM; i++ ){
      origBuf[i] = terrain_grid[i].types;	/* pack types field into array */
    }
#if defined(T_DIAG) || defined(T_DIAG2)
    status = compress( gzipBuf, &dlen, origBuf, olen );
    if( status != Z_OK ){
      pmessage( "TERRAIN: Cannot gzip terrain grid.", 0, MALL, MSERVA );
      return;
    }
    else{
      sprintf( buf, "TERRAIN: Original length %d, compressed length %d", DIM*DIM, dlen );
      pmessage( buf, 0, MALL, MSERVA );
    }
#else
    compress(gzipBuf, &dlen, (Byte *)origBuf, olen );
#endif
    npkts = (dlen>>LOG2NTERRAIN);
    if( dlen&TERRAIN_MASK ){
      npkts++;		/* require a partial packet */
    }
    for( i = 1; i <= npkts; i++ ){
      tpkt.type = SP_TERRAIN2;
      tpkt.sequence = (unsigned char)i;
      tpkt.total_pkts = (unsigned char)npkts;
      if( i < npkts ){
        maxfor = tpkt.length = NTERRAIN;
      }
      else{
        maxfor = tpkt.length = (dlen&TERRAIN_MASK);
      }
      for( j = 0; j < maxfor; j++ ){
        tpkt.terrain_type[j] = gzipBuf[((i-1)<<LOG2NTERRAIN) + j];
      }
      /* ok, packet is filled in, send it */
#ifdef T_DIAG2
      sprintf( buf, "Sending terrain packet %d of %d", tpkt.sequence, tpkt.total_pkts );
      pmessage( buf, 0, MALL, MSERVA );
#endif
      sendClientPacket( (struct player_spacket *) &tpkt );
    }
  }
}

void
updateMOTD(void)
{
    static int spinner = 0;

    if (--spinner < 0) {
	static struct stat oldstat;
	static int firsttime = 1;

	char   *path;
	struct stat newstat;
	struct obvious_packet pkt;

	spinner = 10;

	if (!firsttime) {
	    path = build_path(MOTD);
	    stat(path, &newstat);
	    if (newstat.st_ino == oldstat.st_ino &&
		newstat.st_mtime == oldstat.st_mtime)
		return;
	    oldstat.st_ino = newstat.st_ino;
	    oldstat.st_mtime = newstat.st_mtime;

	    pkt.type = SP_NEW_MOTD;
	    sendClientPacket((struct player_spacket *) & pkt);
	}
	else {
	    sendMotd();		/* can't build_path before this */
	    path = build_path(MOTD);
	    stat(path, &oldstat);
	    /*printf("%s: %d, %d\n", path, oldstat.st_ino, oldstat.st_mtime);*/
	    firsttime = 0;
	}
    }
}


void 
updateClient(void)
{
    int     i;

    for (i = 0; i < MAXPLAYER; i++) {
	mustUpdate[i] = 0;
    }
    updateShips();
    if (send_short)	{
      short_updateTorps();
    }else
	{
	    updateTorps();
	    updateTorpInfos();
	}

    if (weaponsallowed[WP_MISSILE] ||
	weaponsallowed[WP_FIGHTER])
	updateMissiles();
    updateThingies();
    sendMissileNum(me->p_ship.s_missilestored);
    updatePlasmas();
    updateStatus();
    updateSelf();
    updatePhasers();
    updatePlanets();
    updateTerrain();  /* for galaxy reset */
    updateMessages();
    updateWarnings();		/* warning.c */

    /*
       these checks are here to make sure we don't ping from the updateClient
       call in death().  The reason is that savestats() can take > 100ms,
       invalidating the next ping lag calc
    */
    /* Also, don't ping while in verification stage */

    if (ping
	&& (repCount % efticks(5 * configvals->ping_period) == 0)
	&& me->p_status != PDEAD
	&& me->p_status != POUTFIT
    /* && me->p_status != PTQUEUE */
#ifdef AUTHORIZE
	&& testtime <= 0
#endif
	)
	sendClientPing();	/* ping.c */

    if (buffersEmpty()) {
	/* We sent nothing!  We better send something to wake him */
	sendClientPacket((struct player_spacket *) & clientSelf);
    }
    flushSockBuf();
    repCount++;
}

void
briefUpdateClient(void)
{
    updateMessages();
    updateMOTD();
    updateWarnings();

    flushSockBuf();
    repCount++;
}

void
updateStatus(void)
{
    if (repCount % efticks(75) == 0 &&
	((ntohl(clientStatus2.timeprod) != status->timeprod) ||
	 (clientStatus2.tourn != status->tourn))) {
	clientStatus2.type = SP_STATUS2;
	clientStatus2.tourn = status->tourn;
	clientStatus2.dooshes = htonl(status->dooshes);
	clientStatus2.armsbomb = htonl(status->armsbomb);
	clientStatus2.resbomb = htonl(status->resbomb);
	clientStatus2.planets = htonl(status->planets);
	clientStatus2.kills = htonl(status->kills);
	clientStatus2.losses = htonl(status->losses);
	clientStatus2.sbkills = htonl(status->sbkills);
	clientStatus2.sblosses = htonl(status->sblosses);
	clientStatus2.sbtime = htonl(status->sbtime);
	clientStatus2.wbkills = htonl(status->wbkills);
	clientStatus2.wblosses = htonl(status->wblosses);
	clientStatus2.wbtime = htonl(status->wbtime);
	clientStatus2.jsplanets = htonl(status->jsplanets);
	clientStatus2.jstime = htonl(status->jstime);
	clientStatus2.time = htonl(status->time);
	clientStatus2.timeprod = htonl(status->timeprod);
	sendClientPacket((struct player_spacket *) & clientStatus2);
    }
}

static struct player *
maybe_watching(struct player *p)
{
    struct player *tg = &players[me->p_playerl];
    return (p == me
	    && me->p_status == POBSERVE
	    && (me->p_flags & PFPLOCK)
	    && (me->p_teamspy & tg->p_team)
	    && tg->p_spyable) ?
	tg : p;
}

static struct planet *
maybe_watching_planet(void)
{
    return (me->p_status == POBSERVE && (me->p_flags & PFPLLOCK)) ?
    &planets[me->p_planet] : 0;
}

void
updateSelf(void)
{
    struct player *watched = maybe_watching(me);
    int     armies = maybe_watching_planet()
    ? maybe_watching_planet()->pl_armies
    : watched->p_armies;
    int     damage;

    damage = watched->p_damage
	+ shipvals[watched->p_ship.s_type].s_maxdamage
	- watched->p_ship.s_maxdamage;

    if (ntohl(clientSelf.fuel) != watched->p_fuel ||
	ntohl(clientSelf.shield) != watched->p_shield ||
	ntohl(clientSelf.damage) != damage ||
	ntohs(clientSelf.etemp) != watched->p_etemp ||
	ntohs(clientSelf.wtemp) != watched->p_wtemp ||
	ntohl(clientSelf.flags) != watched->p_flags ||
	clientSelf.armies != armies ||
	clientSelf.swar != watched->p_swar ||
	ntohs(clientSelf.whydead) != watched->p_whydead ||
	ntohs(clientSelf.whodead) != watched->p_whodead ||
	clientSelf.pnum != me->p_no) {

	/* we want to send it, but how? */
	clientSelf.type = SP_YOU;

	if (commMode == COMM_UDP) {
	    if (ntohl(clientSelf.flags) != watched->p_flags ||
		clientSelf.armies != armies ||
		clientSelf.swar != watched->p_swar) {
		clientSelf.type = SP_YOU | 0x40;	/* mark as semi-critical */
	    }
	    if (ntohs(clientSelf.whydead) != watched->p_whydead ||
		ntohs(clientSelf.whodead) != watched->p_whodead ||
		clientSelf.pnum != me->p_no) {
		clientSelf.type = SP_YOU | 0x80;	/* mark as critical */
	    }
	}
	clientSelf.pnum = me->p_no;
	clientSelf.flags = htonl(watched->p_flags);
	clientSelf.swar = watched->p_swar;
	clientSelf.hostile = watched->p_hostile;
	clientSelf.armies = armies;
	clientSelf.shield = htonl(watched->p_shield);
	clientSelf.fuel = htonl(watched->p_fuel);
	clientSelf.etemp = htons(watched->p_etemp);
	clientSelf.wtemp = htons(watched->p_wtemp);
	clientSelf.whydead = htons(watched->p_whydead);
	clientSelf.whodead = htons(watched->p_whodead);
	clientSelf.damage = htonl(damage);
	/* ATM - visible tractor */
	clientSelf.tractor = (char) watched->p_tractor | 0x40;

	clientSelf.pad2 = (unsigned char) (status->clock & 0xFF);
	clientSelf.pad3 = (unsigned char) ((status->clock & 0xFF00) >> 8);
	sendClientPacket((struct player_spacket *) & clientSelf);
    }
}

extern int ignored[];

void
updateShips(void)
{
    register int i;
    register struct player *pl;
    register struct plyr_info_spacket *cpli;
    register struct player_spacket *cpl;
    register struct kills_spacket *kills;
    register struct flags_spacket *flags;
    register struct pstatus_spacket *pstatus;
    register struct plyr_login_spacket *login;
    register struct hostile_spacket *hostile;
    register struct stats_spacket2 *stats;
    int     update;
    int     x, y;

#define FLAGMASK (PFSHIELD|PFBOMB|PFORBIT|PFCLOAK|PFROBOT|PFPRACTR|PFDOCK|PFTRACT|PFPRESS|PFDOCKOK)
#define INVISOMASK (PFCLOAK|PFROBOT|PFPRACTR|PFDOCKOK)

    /* Please excuse the ugliness of this loop declaration */
    for (i = 0, pl = players, cpli = clientPlayersInfo, cpl = clientPlayers, kills = clientKills, flags = clientFlags, pstatus = clientPStatus, login = clientLogin, hostile = clientHostile, stats = clientStats2;
	 i < MAXPLAYER;
	 i++, pl++, cpli++, cpl++, kills++, flags++, pstatus++, login++, hostile++, stats++) {
	update = 0;
	if (strcmp(pl->p_name, (char *) login->name) != 0 ||
	    pl->p_stats.st_rank != login->rank ||
	    strcmp(pl->p_monitor, (char *) login->monitor) != 0) {
	    strncpy((char *) login->name, pl->p_name, 15);
	    strncpy((char *) login->monitor, pl->p_monitor, 15);
	    strncpy((char *) login->login, pl->p_login, 15);
	    login->name[15] = 0;
	    login->monitor[15] = 0;
	    login->login[15] = 0;
	    login->type = SP_PL_LOGIN;
	    login->pnum = i;
	    login->rank = pl->p_stats.st_rank;
	    sendClientPacket((struct player_spacket *) login);
	}
	if ((pl != me && (pl->p_swar & me->p_team) != hostile->war) ||
	    (pl != me && (pl->p_hostile & me->p_team) != hostile->hostile) ||
	    (pl == me && pl->p_swar != hostile->war) ||
	    (pl == me && pl->p_hostile != hostile->hostile)) {
	    hostile->type = SP_HOSTILE;
	    if (pl == me) {
		hostile->war = pl->p_swar;
		hostile->hostile = pl->p_hostile;
	    }
	    else {
		hostile->war = (pl->p_swar & me->p_team);
		hostile->hostile = (pl->p_hostile & me->p_team);
	    }
	    hostile->pnum = i;
	    sendClientPacket((struct player_spacket *) hostile);
	}
	/*
	   Send stat packets once per five updates. But, only send one.  We
	   will cycle through them all eventually.
	*/
	/*
	   Also, update if status chages (i.e., entered game, died, tq'ed,
	   etc)
	*/

	/* well this is a big doh 
	   htonl(pl->p_stats.st_di) is an invalid expression - it's not
	   an int.  I bet this part of it doesn't work very well ;) */

	if (pl->p_status != pstatus->status ||
	    (repCount % (MAXPLAYER * efticks(5)) == i * efticks(5) &&
	     (stats->di != htonl((int)(pl->p_stats.st_di)) ||
	      stats->kills != htonl(pl->p_stats.st_tkills) ||
	      stats->losses != htonl(pl->p_stats.st_tlosses) ||
	      stats->armsbomb != htonl(pl->p_stats.st_tarmsbomb) ||
	      stats->resbomb != htonl(pl->p_stats.st_tresbomb) ||
	      stats->dooshes != htonl(pl->p_stats.st_tdooshes) ||
	      stats->planets != htonl(pl->p_stats.st_tplanets) ||
	      stats->sbkills != htonl(pl->p_stats.st_sbkills) ||
	      stats->sblosses != htonl(pl->p_stats.st_sblosses) ||
	      stats->wbkills != htonl(pl->p_stats.st_wbkills) ||
	      stats->wblosses != htonl(pl->p_stats.st_wblosses) ||
	      stats->jsplanets != htonl(pl->p_stats.st_jsplanets) ||
	      stats->rank != htonl(pl->p_stats.st_rank) ||
	      stats->royal != htonl(pl->p_stats.st_royal)))) {

	    stats->genocides = htonl(pl->p_stats.st_genocides);
	    stats->maxkills = htonl((int) (pl->p_stats.st_tmaxkills * 100));
	    stats->di = htonl((int) (pl->p_stats.st_di * 100));
	    stats->kills = htonl(pl->p_stats.st_tkills);
	    stats->losses = htonl(pl->p_stats.st_tlosses);
	    stats->armsbomb = htonl(pl->p_stats.st_tarmsbomb);
	    stats->resbomb = htonl(pl->p_stats.st_tresbomb);
	    stats->dooshes = htonl(pl->p_stats.st_tdooshes);
	    stats->planets = htonl(pl->p_stats.st_tplanets);
	    stats->tticks = htonl(pl->p_stats.st_tticks);
	    stats->sbkills = htonl(pl->p_stats.st_sbkills);
	    stats->sblosses = htonl(pl->p_stats.st_sblosses);
	    stats->sbticks = htonl(pl->p_stats.st_sbticks);
	    stats->sbmaxkills = htonl((int) (pl->p_stats.st_sbmaxkills * 100));
	    stats->wbkills = htonl(pl->p_stats.st_wbkills);
	    stats->wblosses = htonl(pl->p_stats.st_wblosses);
	    stats->wbticks = htonl(pl->p_stats.st_wbticks);
	    stats->wbmaxkills = htonl((int) (pl->p_stats.st_wbmaxkills * 100));
	    stats->jsplanets = htonl(pl->p_stats.st_jsplanets);
	    stats->jsticks = htonl(pl->p_stats.st_jsticks);
	    stats->rank = htonl(pl->p_stats.st_rank);
	    stats->royal = htonl(pl->p_stats.st_royal);
	    stats->type = SP_STATS2;
	    stats->pnum = i;
/*      if (blk_metaserver == 0)*/
	    sendClientPacket((struct player_spacket *) stats);
	}
	if (maybe_watching(pl)->p_ship.s_type != cpli->shiptype ||
	    pl->p_team != cpli->team) {
	    cpli->type = SP_PLAYER_INFO;
	    cpli->pnum = i;
	    cpli->shiptype = maybe_watching(pl)->p_ship.s_type;
	    cpli->team = pl->p_team;
	    sendClientPacket((struct player_spacket *) cpli);
/*            if (!blk_flag)
              cpli->shiptype=pl->p_ship.s_type;*/
	}
	if (kills->kills != htonl((int) (maybe_watching(pl)->p_kills * 100))) {
	    kills->type = SP_KILLS;
	    kills->pnum = i;
	    kills->kills = htonl((int) (maybe_watching(pl)->p_kills * 100));
	    sendClientPacket((struct player_spacket *) kills);
	}
	{
	    int     plstat = pl->p_status;

	    if (status2->paused && pl->p_team != me->p_team)
		plstat = PFREE;	/* enemies are invisible when the game is
				   paused */

	    if (pstatus->status != plstat) {
		/*
		   We update the location of people whose status has changed.
		   (like if they just re-entered...)
		*/
		update = 1;
		pstatus->type = SP_PSTATUS;
		pstatus->pnum = i;
		pstatus->status = plstat;
		if (pl->p_status == PFREE) {
		    /*
		       I think this will turn off ignores for players that
		       leave. 7/24/91 TC
		    */
		    ignored[i] = 0;
		}
		sendClientPacket((struct player_spacket *) pstatus);
	    }
	}

	/* Used to send flags here, see below 8/7/91 TC */

	if (!configvals->hiddenenemy || pl->p_team == me->p_team ||
	    (maybe_watching(pl)->p_flags & PFSEEN) || (status->tourn == 0) ||
	    (maybe_watching(pl)->p_flags & PFROBOT)) {	/* a bot never has its PFSEEN bit set */

	    x = maybe_watching(pl)->p_x;
	    y = maybe_watching(pl)->p_y;
	    if (me != pl && flags->flags !=
		htonl(FLAGMASK & maybe_watching(pl)->p_flags)) {
		flags->type = SP_FLAGS;
		flags->pnum = i;
		flags->flags = htonl(FLAGMASK & maybe_watching(pl)->p_flags);
		flags->tractor = (char) maybe_watching(pl)->p_tractor | 0x40;	/* ATM - visible tractor */
		sendClientPacket((struct player_spacket *) flags);
	    }
	}
	else {
	    /* A hack to make him inviso */
	    x = -100000;
	    y = -100000;

	    /* reduce flag info if he's inviso 8/7/91 TC */

	    if (me != pl && flags->flags != htonl(INVISOMASK & pl->p_flags)) {
		flags->type = SP_FLAGS;
		flags->pnum = i;
		flags->flags = htonl(INVISOMASK & pl->p_flags);
		sendClientPacket((struct player_spacket *) flags);
	    }
	}
	if (x != ntohl(cpl->x) || y != ntohl(cpl->y) ||
	    maybe_watching(pl)->p_dir != cpl->dir ||
	    maybe_watching(pl)->p_speed != cpl->speed) {
	    /*
	       We update the player if: 1) haven't updated him for 9
	       intervals. 2) he is on the screen 3) he was on the screen
	       recently.
	    */
	    if (!update && repCount % efticks(9) != 0 &&
		(ntohl(cpl->x) < me->p_x - SCALE * WINSIDE / 2 ||
		 ntohl(cpl->x) > me->p_x + SCALE * WINSIDE / 2 ||
		 ntohl(cpl->y) > me->p_y + SCALE * WINSIDE / 2 ||
		 ntohl(cpl->y) < me->p_y - SCALE * WINSIDE / 2) &&
		(y > me->p_y + SCALE * WINSIDE / 2 ||
		 x > me->p_x + SCALE * WINSIDE / 2 ||
		 x < me->p_x - SCALE * WINSIDE / 2 ||
		 y < me->p_y - SCALE * WINSIDE / 2))
		continue;
	    /*
	       If the guy is cloaked, give information only occasionally, and
	       make it slightly inaccurate. Also, we don't give a direction.
	       The client has no reason to know.
	    */
	    if ((pl->p_flags & PFCLOAK) &&
		(pl->p_cloakphase == (CLOAK_PHASES - 1)) &&
		(maybe_watching(me) != pl) && !mustUpdate[i]) {
		if (repCount % efticks(9) != 0)
		    continue;
		cpl->type = SP_PLAYER;
		cpl->pnum = i;
		cpl->x = htonl(x + (lrand48() % 2000) - 1000);
		cpl->y = htonl(y + (lrand48() % 2000) - 1000);
		sendClientPacket(cpl);
		continue;
	    }
	    cpl->type = SP_PLAYER;
	    cpl->pnum = i;
	    cpl->x = htonl(x);
	    cpl->y = htonl(y);
	    cpl->speed = maybe_watching(pl)->p_speed;
	    cpl->dir = maybe_watching(pl)->p_dir;
	    sendClientPacket(cpl);
	}
    }
}

static int 
input_allowed(int packettype)
{
    switch (packettype) {
    case CP_MESSAGE:
    case CP_SOCKET:
    case CP_OPTIONS:
    case CP_BYE:
    case CP_UPDATES:
    case CP_RESETSTATS:
    case CP_RESERVED:
    case CP_RSA_KEY:
    case CP_ASK_MOTD:
    case CP_PING_RESPONSE:
    case CP_UDP_REQ:
    case CP_FEATURE:
    case CP_S_MESSAGE:
	return 1;
    default:
	if (!me)
		return 0;

	if (me->p_status == PTQUEUE)
	    return (packettype == CP_OUTFIT
		);
	else if (me->p_status == POBSERVE)
	    return (packettype == CP_QUIT
		    || packettype == CP_PLANLOCK
		    || packettype == CP_PLAYLOCK
		    || packettype == CP_SCAN
		    || packettype == CP_SEQUENCE	/* whatever this is */
		);

	if (inputMask >= 0 && inputMask != packettype)
	    return 0;
	if (me == NULL)
	    return 1;
	if (!(me->p_flags & (PFWAR | PFREFITTING)))
	    return 1;

	return 0;
    }
}

/* flushSockBuf, socketPause, socketWait
   were here */

static int rsock;		/* ping stuff */

/* ripped out of above routine */
static int
doRead(int asock)
{
    struct timeval timeout;
/*    int readfds;*/
    fd_set  readfds;
    char    buf[BUFSIZ * 2];
    char   *bufptr;
    int     size;
    int     count;
    int     temp;

    rsock = asock;		/* need the socket in the ping handler
				   routine */

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
/*    readfds = 1<<asock;*/
    FD_ZERO(&readfds);
    FD_SET(asock, &readfds);
    /* Read info from the xtrek server */
    count = read(asock, buf, BUFSIZ * 2);
    if (count <= 0) {
#if DEBUG
	/* (this happens when the client hits 'Q') */
	fprintf(stderr, "1) read() failed in doRead (%d, error %d)\n",
		count, errno);
	fprintf(stderr, "asock=%d, sock=%d\n", asock, sock);
#endif
	if (asock == udpSock) {
	    if (errno == ECONNREFUSED) {
		struct sockaddr_in addr;

		UDPDIAG(("Hiccup(%d)!  Reconnecting\n", errno));
		addr.sin_addr.s_addr = remoteaddr;
		addr.sin_port = htons(udpClientPort);
		addr.sin_family = AF_INET;
		if (connect(udpSock, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
		    perror("hiccup connect");
		    UDPDIAG(("Unable to reconnect\n"));
		    /* and fall through to disconnect */
		}
		else {
		    UDPDIAG(("Reconnect successful\n"));
		    return (0);
		}
	    }
	    UDPDIAG(("*** UDP disconnected (res=%d, err=%d)\n",
		     count, errno));
	    printUdpInfo();
	    closeUdpConn();
	    commMode = COMM_TCP;
	    return (0);
	}
	clientDead = 1;
	return (0);
    }
    bufptr = buf;
    while (bufptr < buf + count) {
	if (*bufptr < 1 ||
	    (unsigned char) *bufptr > NUM_PACKETS ||
	    size_of_cpacket((void*)bufptr) == 0) {
	    printf("Unknown packet type: %d, aborting...\n", *bufptr);
	    return (0);
	}
	size = size_of_cpacket(bufptr);
	while (size > count + (buf - bufptr)) {
	    /*
	       We wait for up to twenty seconds for rest of packet. If we
	       don't get it, we assume the client died.
	    */
	    timeout.tv_sec = 20;
	    timeout.tv_usec = 0;
	    /* readfds=1<<asock; */
	    FD_ZERO(&readfds);
	    FD_SET(asock, &readfds);
	    if (select(32, &readfds, 0, 0, &timeout) == 0) {
		logmessage("Died while waiting for packet...");
		fprintf(stderr, "1a) read() failed (%d, error %d)\n",
			count, errno);
		clientDead = 1;
		return (0);
	    }
	    temp = read(asock, buf + count, size - (count + (buf - bufptr)));
	    if (temp <= 0) {
		if (errno != EINTR) {
		    sprintf(buf, "Died in second read(), return=%d", temp);
		    logmessage(buf);
		    fprintf(stderr, "2) read() failed (%d, error %d)\n",
			    count, errno);
		    clientDead = 1;
		    return (0);
		}
	    }
	    else
		count += temp;
	}
	/*
	   Check to see if the handler is there and the request is legal. The
	   code is a little ugly, but it isn't too bad to worry about yet.
	*/
	packetsReceived[(unsigned char)*bufptr]++;

	if (asock == udpSock)
	    packets_received++;

	if (handlers[(unsigned char)*bufptr].handler != NULL) {
	    if (input_allowed(*bufptr)) {
		if (me && me->p_flags & PFSELFDEST
		    && *bufptr != CP_PING_RESPONSE) {
		    me->p_flags &= ~PFSELFDEST;
		    warning("Self Destruct has been canceled");
		}
		(*(handlers[(unsigned char)*bufptr].handler)) (bufptr);
	    }
	    /* Otherwise we ignore the request */
	}
	else {
	    printf("Handler for packet %d not installed...\n", *bufptr);
	}
	bufptr += size;
	if (bufptr > buf + BUFSIZ) {
	    memcpy(buf, buf + BUFSIZ, BUFSIZ);
	    if (count == BUFSIZ * 2) {
		/* readfds = 1<<asock; */
		FD_ZERO(&readfds);
		FD_SET(asock, &readfds);
		if (select(32, &readfds, 0, 0, &timeout)) {
		    temp = read(asock, buf + BUFSIZ, BUFSIZ);
		    count = BUFSIZ + temp;
		    if (temp <= 0) {
			sprintf(buf, "Died in third read(), return=%d", temp);
			fprintf(stderr, "3) read() failed (%d, error %d)\n",
				count, errno);
			logmessage(buf);
			clientDead = 1;
			return (0);
		    }
		}
		else {
		    count = BUFSIZ;
		}
	    }
	    else {
		count -= BUFSIZ;
	    }
	    bufptr -= BUFSIZ;
	}
    }
    return (1);
}

/* Find out if client has any requests */
int 
readFromClient(void)
{
    struct timeval timeout;
    fd_set  readfds, writefds;
    int     retval = 0;

    if (clientDead)
	return (0);

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    build_select_masks(&readfds, &writefds);

    if (select(32, &readfds, &writefds, (fd_set*)0, &timeout) != 0) {
	/* Read info from the xtrek client */
	if (FD_ISSET(sock, &readfds)) {
	    retval += doRead(sock);
	}
	if (udpSock >= 0 && FD_ISSET(udpSock, &readfds)) {
	    V_UDPDIAG(("Activity on UDP socket\n"));
	    retval += doRead(udpSock);
	}
	if (retval==0 &&	/* no other traffic */
	    FD_ISSET(sock, &writefds)) {
	    flushDeferred();	/* we have an eye in the packet hurricane */
	}
    }
    return (retval != 0);	/* convert to 1/0 */
}

static void 
handleTorpReq(struct torp_cpacket *packet)
{
    ntorp((CARD8)packet->dir, (int)TMOVE);
}

static void 
handlePhasReq(struct phaser_cpacket *packet)
{
    phaser(packet->dir);
}

static void 
handleSpeedReq(struct speed_cpacket *packet)
{
    set_speed(packet->speed, 1);
}

static void 
handleDirReq(struct dir_cpacket *packet)
{
    me->p_flags &= ~(PFPLOCK | PFPLLOCK);
    set_course(packet->dir);
}

static void 
handleShieldReq(struct shield_cpacket *packet)
{
    if (packet->state) {
	shield_up();
    }
    else {
	shield_down();
    }
}

static void 
handleRepairReq(struct repair_cpacket *packet)
{
    if (packet->state) {
	repair();
    }
    else {
	me->p_flags &= ~(PFREPAIR);
    }
}

static void 
handleOrbitReq(struct orbit_cpacket *packet)
{
    if (packet->state) {
	orbit();
    }
    else {
	me->p_flags &= ~PFORBIT;
	if (me->p_flags & PFDOCK) {
	    if (players[me->p_docked].p_speed > 4) {
		warning("It's unsafe to disengage from bases while over warp 4.");
		return;
	    }
	    else
		undock_player(me);
	}
    }
}

/*-----------------------------PRACTICE_ROBOT-----------------------------*/
/*  Send in a practice robot.  You can only bring i a practice robot if
no other players are in the game.  */

static void
practice_robo(void)
{
    char   *paths;		/* to hold dot dir path */
    char   *arg1;
    register int i;		/* looping var */
    register struct player *j;	/* to point to players */
    static struct timeval space = {0, 0};

    if (!temporally_spaced(&space, 1000000))	/* damn auto-repeating... */
	return;

    for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	if (j->p_status != PALIVE)	/* if payer not alive, that's ok */
	    continue;
	if (j == me)		/* ignore myself */
	    continue;
	warning("Can't send in practice robot with other players in the game.");
	return;			/* another player discovered--out of here */
    }

    if (fork() == 0) {		/* do if fork successful */
	(void) r_signal(SIGALRM, SIG_DFL);
	(void) close(0);
	(void) close(1);
	(void) close(2);
	switch (me->p_team) {	/* decide which teaem robot is on */
	case FED:
	    arg1 = "-Tf";	/* fed option */
	    break;
	case ROM:
	    arg1 = "-Tr";	/* rom option */
	    break;
	case KLI:
	    arg1 = "-Tk";	/* klingon option */
	    break;
	case ORI:
	    arg1 = "-To";	/* orion option */
	    break;
	default:
	    arg1 = "-Ti";	/* in case something screwy happens */
	    break;
	}

	paths = build_path(ROBOT);

	execl(paths, "robot", arg1, "-p", "-f", "-h", 0);
	_exit(1);		/* failure :(  died at birth */
    }
}

/*--------------------------------------------------------------------------*/

/* ARGSUSED */
static void 
handlePractrReq(struct practr_cpacket *packet)
{
    practice_robo();
}

static void 
handleBombReq(struct bomb_cpacket *packet)
{
    if (packet->state) {
	bomb_planet();
    }
    else {
	me->p_flags &= ~(PFBOMB);
    }
}

static void 
handleBeamReq(struct beam_cpacket *packet)
{
    if (packet->state == 1) {
	beam_up();
    }
    else if (packet->state) {
	beam_down();
    }
    else {
	me->p_flags &= ~(PFBEAMUP | PFBEAMDOWN);
    }
}

static void 
handleCloakReq(struct cloak_cpacket *packet)
{
    if (packet->state) {
	cloak_on();
    }
    else {
	cloak_off();
    }
}

/* ARGSUSED */
static void 
handleDetTReq(struct det_torps_cpacket *packet)
{
    detothers();
}

/* ARGSUSED */
static void 
handleCopilotReq(struct copilot_cpacket *packet)
{
/*
 * Unsupported...
    if (packet->state) {
	me->p_flags |= PFCOPILOT;
    } else {
	me->p_flags &= ~PFCOPILOT;
    }
 */
}

static void 
handleOutfit(struct outfit_cpacket *packet)
{
    shipPick = packet->ship;
    teamPick = packet->team;
}

void
sendPickokPacket(int state)
{
    struct pickok_spacket pickPack;

    pickPack.type = SP_PICKOK;
    pickPack.state = state;
    sendClientPacket((struct player_spacket *) & pickPack);
}

static void 
handleLoginReq(struct login_cpacket *packet)
{
    if (packet->pad2 == 0x69) {
	if (packet->pad3 == 0x42)
	    blk_flag = 1;	/* added 1/19/93 KAO */
	if (packet->pad3 == 0x43)
	    blk_flag = 2;
    }
    strncpy(namePick, (char *)packet->name, 16);
    namePick[15] = 0;
    strncpy(passPick, (char *)packet->password, 16);
    passPick[15] = 0;
    /* Is this a name query or a login? */
    if (packet->query) {
	passPick[15] = 1;
    }
    strncpy(login, (char *) packet->login, 16);
    login[15] = 0;
}

void
sendClientLogin(struct stats *stats)
{
    struct login_spacket logPacket;
    logPacket.pad2 = 69;
    if (configvals->galaxygenerator == 4)
	logPacket.pad3 = 88;
    else
	logPacket.pad3 = 42;
    logPacket.type = SP_LOGIN;
    if (stats == NULL) {
	logPacket.accept = 0;
    }
    else {
	logPacket.accept = 1;
	logPacket.flags = htonl(stats->st_flags);
    }
    sendClientPacket((struct player_spacket *) & logPacket);
}

static void 
handlePlasmaReq(struct plasma_cpacket *packet)
{
    if (me->p_specweap & SFNHASMISSILE) {
	fire_missile_dir(packet->dir);
    }
    else if (me->p_specweap & SFNPLASMAARMED) {
	nplasmatorp(packet->dir, PTMOVE);
    }
    else {
	warning("This ship is armed with no special weapons");
    }
}

static void 
handleWarReq(struct war_cpacket *packet)
{
    declare_war(packet->newmask);
}

static void 
handlePlanlockReq(struct planlock_cpacket *packet)
{
    lock_planet(packet->pnum);
}

static void 
handlePlaylockReq(struct playlock_cpacket *packet)
{
    lock_player(packet->pnum);
}

static void 
handleDetMReq(struct det_mytorp_cpacket *packet)
{
    struct torp *atorp;
    short   t;

    /* you can det individual torps */
    t = ntohs(packet->tnum);
    if (t < 0) {
	struct missile *dr;
	int     i, any;

	any = 0;

	for (i = 0; i < MAXTORP; i++) {
	    atorp = &torps[me->p_no * MAXTORP + i];
	    if (atorp->t_status == TMOVE || atorp->t_status == TSTRAIGHT) {
		atorp->t_status = TOFF;
		any = 1;
	    }
	}

	if (any)
	    return;

	for (i = 0; i < NPTHINGIES; i++) {
	    dr = &missiles[me->p_no * NPTHINGIES + i];
	    if (dr->ms_status == TMOVE || dr->ms_status == TSTRAIGHT) {
		switch (dr->ms_type) {
		case MISSILETHINGY:
		    dr->ms_status = TOFF;
		    break;
		case FIGHTERTHINGY:
		    dr->ms_status = TRETURN;
		    break;
		}
		any = 1;
	    }
	}

	/* any ? */
    }
    else {

	if (t < 0 || t >= MAXPLAYER * MAXTORP)
	    return;
	atorp = &torps[t];

	if (atorp->t_owner != me->p_no)
	    return;
	if (atorp->t_status == TMOVE || atorp->t_status == TSTRAIGHT) {
	    atorp->t_status = TOFF;
	}
    }
}

static void 
handleTractorReq(struct tractor_cpacket *packet)
{
    int     target;
    struct player *player;

    if (weaponsallowed[WP_TRACTOR] == 0) {
	warning("Tractor beams haven't been invented yet.");
	return;
    }
    target = packet->pnum;
    if (packet->state == 0) {
	me->p_flags &= ~(PFTRACT | PFPRESS);
	return;
    }
    if (me->p_flags & PFCLOAK) {
	warning("Weapons's Officer:  Cannot tractor while cloaked, sir!");
	return;
    }
    if (target < 0 || target >= MAXPLAYER || target == me->p_no)
	return;
    player = &players[target];
    if (player->p_flags & PFCLOAK)
	return;
    if (me->p_flags & PFDOCK && players[me->p_docked].p_speed > 4) {
	warning("It's unsafe to tractor while docked and moving at a warp greater then 4.");
	return;
    }
    if (ihypot(me->p_x - player->p_x, me->p_y - player->p_y) <
	(TRACTDIST) * me->p_ship.s_tractrng) {
	undock_player(me);
	me->p_flags &= ~PFORBIT;
	me->p_tractor = target;
	me->p_flags |= PFTRACT;

    }
    else {
	warning("Weapon's Officer:  Vessel is out of range of our tractor beam.");
    }
}

static void 
handleRepressReq(struct repress_cpacket *packet)
{
    int     target;
    struct player *player;

    if (weaponsallowed[WP_TRACTOR] == 0) {
	warning("Pressor beams haven't been invented yet.");
	return;
    }
    target = packet->pnum;
    if (packet->state == 0) {
	me->p_flags &= ~(PFTRACT | PFPRESS);
	return;
    }
    if (me->p_flags & PFCLOAK) {
	warning("Weapons's Officer:  Cannot pressor while cloaked, sir!");
	return;
    }
    if (target < 0 || target >= MAXPLAYER || target == me->p_no)
	return;
    player = &players[target];
    if (player->p_flags & PFCLOAK)
	return;
    if (me->p_flags & PFDOCK && players[me->p_docked].p_speed > 4) {
	warning("It's unsafe to pressor while docked and moving at a warp greater then 4.");
	return;
    }
    if (ihypot(me->p_x - player->p_x, me->p_y - player->p_y) <
	(TRACTDIST) * me->p_ship.s_tractrng) {
	undock_player(me);
	me->p_flags &= ~PFORBIT;
	me->p_tractor = target;
	me->p_flags |= (PFTRACT | PFPRESS);
    }
    else {
	warning("Weapon's Officer:  Vessel is out of range of our pressor beam.");
    }
}

void
sendMotdLine(char *line)
{
    struct motd_spacket motdPacket;

    motdPacket.type = SP_MOTD;
    strncpy(motdPacket.line, line, 80);
    motdPacket.line[79] = '\0';
    sendClientPacket((struct player_spacket *) & motdPacket);
}

/* ARGSUSED */
static void 
handleCoupReq(struct coup_cpacket *packet)
{
    switch_special_weapon();
}

static void 
handleRefitReq(struct refit_cpacket *packet)
{
    do_refit(packet->ship);
}

static void 
handleMessageReq(struct mesg_cpacket *packet)
{
    char    addrbuf[9];
    static long lasttime = 0 /* , time() */ ;
    static int balance = 0;	/* make sure he doesn't get carried away */
    long    thistime;

    /*
       Some random code to make sure the player doesn't get carried away
       about the number of messages he sends.  After all, he could try to jam
       peoples communications if we let him.
    */

    thistime = time(NULL);
    if (lasttime != 0) {
	balance = balance - (thistime - lasttime);
	if (balance < 0)
	    balance = 0;
    }
    lasttime = thistime;
    if (balance >= 15) {
	warning("Be quiet");
	balance += 3;
	if (balance > time(NULL) + 60) {
	    balance = time(NULL) + 60;
	}
	return;
    }
    balance += 3;
    /*packet->mesg[69] = '\0';*/
    sprintf(addrbuf, " %s->", twoletters(me));
    if (parseIgnore(packet))
	return;			/* moved this up 4/6/92 TC */
    if (packet->group&MGOD) {
	strcpy(addrbuf + 5, "GOD");
    } else if (packet->group&MALL) {
	sprintf(addrbuf + 5, "ALL");
	if (isCensured(me->p_login)) {
	    warning("You are censured.  Message was not sent.");
	    return;
	}
    } else if (packet->group&MTEAM) {
	if (packet->indiv != FED && packet->indiv != ROM &&
	    packet->indiv != KLI && packet->indiv != ORI)
	    return;
	if (isCensured(me->p_login) && (packet->indiv != me->p_team)) {
	    warning("You are censured.  Message was not sent.");
	    return;
	}
	sprintf(addrbuf + 5, teams[packet->indiv].shortname);
    } else if (packet->group&MINDIV) {
	if (packet->indiv < 0 || packet->indiv >= MAXPLAYER)
	    return;
	if (players[packet->indiv].p_status == PFREE)
	    return;
	if (isCensured(me->p_login) && (players[packet->indiv].p_team != me->p_team)) {
	    warning("You are censured.  Message was not sent.");
	    return;
	}
	if (ignored[packet->indiv] & MINDIV) {
	    warning("You are ignoring that player.  Message was not sent.");
	    return;
	}
	if ((me->p_team != players[packet->indiv].p_team) &&
	    (isCensured(players[packet->indiv].p_login))) {
	    warning("That player is censured.  Message was not sent.");
	    return;
	}
	sprintf(addrbuf + 5, "%s ", twoletters(&players[packet->indiv]));
    } else {
	return;
    }
    /* don't eat the parsed messages */
    pmessage2(packet->mesg, packet->indiv, packet->group, addrbuf, me->p_no);
    if (me->p_no == packet->indiv && packet->group == MINDIV) {
	char	tmpbuf[sizeof(packet->mesg)+1];
	strncpy(tmpbuf, (char *) packet->mesg, sizeof(packet->mesg));
	tmpbuf[sizeof(packet->mesg)] = 0;
	parse_command_mess(tmpbuf, me->p_no);
    }

/*
 * Blech !!!
 *
 * Don't do this:
 *
 * if (me->p_no == packet->indiv && packet->mesg[0] == '_')
 *   me = &players[0];
 */
}

/* ARGSUSED */
static void 
handleQuitReq(struct quit_cpacket *packet)
{
    if (me->p_status == POBSERVE) {
	me->p_status = PTQUEUE;
	return;
    }
    me->p_flags |= PFSELFDEST;

    switch (me->p_ship.s_type) {
    case STARBASE:
    case WARBASE:
	selfdest = 60;
	break;
    default:
	selfdest = 10;
    }

    selfdest = me->p_updates + selfdest * 10;

    warning("Self destruct initiated");
}

void
sendMaskPacket(int mask)
{
    struct mask_spacket maskPacket;

    maskPacket.type = SP_MASK;
    maskPacket.mask = mask;
    sendClientPacket((struct player_spacket *) & maskPacket);
}

static void 
handleOptionsPacket(struct options_cpacket *packet)
{
    mystats->st_flags = ntohl(packet->flags) |
	(mystats->st_flags & ST_CYBORG);	/* hacked fix 8/24/91 TC */
    keeppeace = (mystats->st_flags / ST_KEEPPEACE) & 1;
}

static void 
handleSocketReq(struct socket_cpacket *packet)
{
    nextSocket = ntohl(packet->socket);
    userVersion = packet->version;
    userUdpVersion = packet->udp_version;
}

/* ARGSUSED */
static void 
handleByeReq(struct bye_cpacket *packet)
{
    noressurect = 1;
}

int
checkVersion(void)
{
    struct badversion_spacket packet;

    if (userVersion != SOCKVERSION) {
	packet.type = SP_BADVERSION;
	packet.why = 0;
	sendClientPacket((struct player_spacket *) & packet);
	flushSockBuf();
	return (0);
    }
    return (1);
}

void
logEntry(void)
{
    FILE   *logfile;
    int     curtime;
    char   *paths;

    paths = build_path(LOGFILENAME);
    logfile = fopen(paths, "a");
    if (!logfile)
	return;
    curtime = time(NULL);

    fprintf(logfile, "Joining: %s, (%c) <%s@%s> %s", me->p_name,
	    shipnos[me->p_no],
	    me->p_login,	/* debug 2/21/92 TMC */
	    me->p_full_hostname,
	    ctime((time_t *) & curtime));

    fclose(logfile);
}

/* gwrite was here */

static void 
handleDockingReq(struct dockperm_cpacket *packet)
{
    int     i;

    if (allows_docking(me->p_ship)) {
	if (me->p_speed > 4 && me->p_docked) {
	    warning("It's unsafe to disengage other ships while over warp 4.");
	    return;
	}
	else {
	    for (i = 0; i < me->p_ship.s_numports; i++)
		base_undock(me, i);
	    me->p_docked = 0;

	    if (packet->state)
		me->p_flags |= PFDOCKOK;
	    else
		me->p_flags &= ~PFDOCKOK;
	}
    }
}

static void 
handleReset(struct resetstats_cpacket *packet)
{
    extern int startTkills, startTlosses, startTarms, startTplanets, startTticks;

    if (packet->verify != 'Y')
	return;

    /* Gee, they seem to want to reset their stats!  Here goes... */
    mystats->st_genocides = 0;
    mystats->st_tmaxkills = 0.0;
    mystats->st_di = 0.0;
    mystats->st_tkills = 0;
    mystats->st_tlosses = 0;
    mystats->st_tarmsbomb = 0;
    mystats->st_tresbomb = 0;
    mystats->st_tdooshes = 0;
    mystats->st_tplanets = 0;
    mystats->st_tticks = 1;
    mystats->st_sbkills = 0;
    mystats->st_sblosses = 0;
    mystats->st_sbmaxkills = 0.0;
    mystats->st_sbticks = 1;
    mystats->st_wbkills = 0;
    mystats->st_wblosses = 0;
    mystats->st_wbmaxkills = 0.0;
    mystats->st_wbticks = 1;
    mystats->st_jsplanets = 0;
    mystats->st_jsticks = 1;
    mystats->st_rank = 0;
    mystats->st_royal = 0;

    startTkills = mystats->st_tkills;
    startTlosses = mystats->st_tlosses;
    startTarms = mystats->st_tarmsbomb;
    startTplanets = mystats->st_tplanets;
    startTticks = mystats->st_tticks;
}

static void 
handleUpdatesReq(struct updates_cpacket *packet)
{
    struct itimerval udt;
    extern int interrupting;	/* main.c */
    int     min_delay = me->p_observer
    ? configvals->min_observer_upd_delay
    : configvals->min_upd_delay;

    timerDelay = ntohl(packet->usecs);
    if (timerDelay < min_delay)
	timerDelay = min_delay;
    if (timerDelay >= 1000000)
	timerDelay = 999999;

    if (interrupting) {		/* only setitimer if the ntserv is configured
				   to handle it.  It's NOT configured to
				   handle it in the outfit loop... */
	udt.it_interval.tv_sec = 0;
	udt.it_interval.tv_usec = timerDelay;
	udt.it_value.tv_sec = 0;
	udt.it_value.tv_usec = timerDelay;
	setitimer(ITIMER_REAL, &udt, 0);
    }

}

void
logmessage(char *string)
{
    FILE   *fp;
    char   *paths;

    paths = build_path(LOGFILENAME);

    fp = fopen(paths, "a");
    if (fp) {
	fprintf(fp, "%s\n", string);
	fclose(fp);
    }
}

static void 
handleRSAKey(struct rsa_key_cpacket *packet)
{
#ifdef AUTHORIZE
    struct rsa_key_spacket mysp;
    char    serverName[64];

    if (testtime == 1)
	return;
    if (RSA_Client != 1)
	return;
    memcpy(mysp.data, testdata, KEY_SIZE);
    if (gethostname(serverName, 64))
	fprintf(stderr, "gethostname() error\n");
    if (decryptRSAPacket(&mysp, packet, serverName)) {
	fprintf(stderr, "User verified incorrectly.\n");
	testtime = 1;
	return;
    }
    testtime = 0;
#endif				/* AUTHORIZE */
}

static void 
handleReserved(struct reserved_cpacket *packet)
{
#ifdef AUTHORIZE
    struct reserved_cpacket mycp;
    struct reserved_spacket mysp;
    struct rsa_key_spacket rsp;
    char    serverName[64];	/* now get serverName from system 8/2/92 TC */

    if (testtime == 1)
	return;
    if (memcmp(packet->data, testdata, RESERVED_SIZE) != 0) {
	testtime = 1;
	return;
    }
    if (!strncmp(packet->resp, RSA_VERSION, 3)) {
	/* This is an RSA type client */
	RSA_Client = 2;
	warning(RSA_VERSION);
	if (!strncmp(packet->resp, RSA_VERSION, strlen("RSA v??"))) {
	    /* This is the right major version */
	    RSA_Client = 1;
	    makeRSAPacket(&rsp);
	    memcpy(testdata, rsp.data, KEY_SIZE);
	    sendClientPacket((struct player_spacket *) & rsp);
	    return;
	}
	testtime = 1;
	return;
    }
    memcpy(mysp.data, testdata, RESERVED_SIZE);
    if (gethostname(serverName, 64))
	fprintf(stderr, "gethostname() error\n");	/* 8/2/92 TC */
    encryptReservedPacket(&mysp, &mycp, serverName, me->p_no);
    if (memcmp(packet->resp, mycp.resp, RESERVED_SIZE) != 0) {
	fprintf(stderr, "User verified incorrectly.\n");
	testtime = 1;
	return;
    }
    /* Use .sysdef CONFIRM flag to allow old style clients. */
    if (configvals->binconfirm == 2)
	testtime = 0;
    else
	testtime = 1;

#endif				/* AUTHORIZE */
}

static void 
handleScan(struct scan_cpacket *packet)		/* ATM */
{
#if 0
    struct scan_spacket response;
    struct player *pp;

    memset(&response, 0, sizeof(struct scan_spacket));
    response.type = SP_SCAN;
    response.pnum = packet->pnum;
    if (!weaponsallowed[WP_SCANNER]) {
	warning("Scanners haven't been invented yet");
	response.success = 0;
    } else {
	response.success = scan(packet->pnum);

	if (response.success) {
	    / fill in all the goodies /
	    pp = &players[packet->pnum];
	    response.p_fuel = htonl(pp->p_fuel);
	    response.p_armies = htonl(pp->p_armies);
	    response.p_shield = htonl(pp->p_shield);
	    response.p_damage = htonl(pp->p_damage);
	    response.p_etemp = htonl(pp->p_etemp);
	    response.p_wtemp = htonl(pp->p_wtemp);
	}
    }
    sendClientPacket((struct player_spacket*)&response);
#endif
}


static void
handlePingResponse(struct ping_cpacket *packet)
{
    char    buf[80];
    /* client requests pings by sending pingme == 1 on TCP socket */

    if (rsock == sock) {
	if (!ping && packet->pingme == 1) {
	    ping = 1;
	    sprintf(buf, "Server sending ping packets at %d second intervals",
		    configvals->ping_period);
	    warning(buf);
	    return;
	}
	/* client says stop */
	else if (ping && !packet->pingme) {
	    ping = 0;
	    warning("Server no longer sending ping packets.");
	    return;
	}
    }
    pingResponse(packet);	/* ping.c */
}

static void
handleShortReq(struct shortreq_cpacket *packet)
{
    struct shortreply_spacket	resp;

    switch(packet->req) {
    case SPK_VOFF:
	send_short = 0;
	warning("Not sending variable and short packets.  Back to default.");
	if (udpSock >= 0 && udpMode == MODE_FAT) forceUpdate();
	break;

    case SPK_VON:
	if ( packet->version != (char)SHORTVERSION){
	    warning("Your SHORT Protocol Version is not right!");
	    packet->req = SPK_VOFF;
	    break;		
	}
	if (!send_short) warning("Sending variable and short packets. "); /* send only firsttime */
	send_short = 1;
	resp.winside = ntohs(WINSIDE);
	resp.gwidth  = ntohl(GWIDTH);
	break;

    case SPK_MOFF:
	send_mesg = 1;
	warning("Obsolete!");
	packet->req = SPK_MON;
	break;
      
    case SPK_MON:
	send_mesg = 1;
	warning("All messages sent.");
	break;

    case SPK_M_KILLS:
	send_kmesg = 1;
	warning("Kill messages sent");
	break;
      
    case SPK_M_NOKILLS:
	send_kmesg = 1;
	warning("Obsolete!");
	packet->req = SPK_M_KILLS;
	break;
      
    case SPK_M_WARN:
	send_warn = 1;
	warning("Warn messages sent");
	break;
      
    case SPK_M_NOWARN:
	send_warn = 1;
	warning("Obsolete!");
	packet->req = SPK_M_WARN;	
	break;

    case SPK_SALL:
	if(send_short){
	    spk_update_sall = 1;
	    spk_update_all = 0;
	    forceUpdate();
	} else
	    warning("Activate SHORT Packets first!");
	return;

    case SPK_ALL:
	if(send_short){
	    spk_update_sall = 0;
	    spk_update_all = 1;
	    forceUpdate();
	} else
	    warning("Activate SHORT Packets first!");
	return;
	 
    default:
	warning("Unknown short packet code");
	return;
    }
   
    resp.type = SP_S_REPLY;
    resp.repl = (char) packet->req;

    sendClientPacket((struct player_spacket *) &resp);
}

static void
handleThresh(struct threshold_cpacket *packet)
{
    send_threshold = packet->thresh;
    warning("Server is compiled without Thresholdtesting!");
}

static void
handleSMessageReq(struct mesg_s_cpacket *packet)
{
    /* If someone would delete the hardcoded things in handleMessageReq */
    /*	like     packet->mesg[69]='\0';   */
    /* we could give handleMessageReq the packet without copying */
    /* But i have no time  HW 04/6/93 */

    struct mesg_cpacket mesPacket;  
    mesPacket.type =  CP_MESSAGE;
    mesPacket.group = packet->group;
    mesPacket.indiv  = packet->indiv;
    strcpy((char *) mesPacket.mesg, (char *) packet->mesg);
    handleMessageReq(& mesPacket);
    /* I hope this was it */
}

/*
 * ---------------------------------------------------------------------------
 *	Strictly UDP from here on
 * ---------------------------------------------------------------------------
 */

int
connUdpConn(void)
{
    struct sockaddr_in addr;
    int     len;

    if (udpSock > 0) {
	fprintf(stderr, "ntserv: tried to open udpSock twice\n");
	return (0);		/* pretend we succeeded (this could be bad) */
    }
    resetUDPbuffer();
    if ((udpSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("ntserv: unable to create DGRAM socket");
	return (-1);
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = remoteaddr;	/* addr of our client */
    addr.sin_port = htons(udpClientPort);	/* client's port */

    if (connect(udpSock, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
	perror("ntserv: connect to client UDP port");
	UDPDIAG(("Unable to connect() to %s on port %d\n", me->p_name,
		 udpClientPort));
	close(udpSock);
	udpSock = -1;
	return (-1);
    }
    UDPDIAG(("connect to %s's port %d on 0x%x succeded\n",
	     me->p_name, udpClientPort, remoteaddr));

    /* determine what our port is */
    len = sizeof(addr);
    if (getsockname(udpSock, (struct sockaddr *) & addr, &len) < 0) {
	perror("netrek: unable to getsockname(UDP)");
	UDPDIAG(("Can't get our own socket; connection failed\n"));
	close(udpSock);
	udpSock = -1;
	return (-1);
    }
    udpLocalPort = (int) ntohs(addr.sin_port);

    if (configvals->udpAllowed > 2)	/* verbose debug mode? */
	printUdpInfo();

    return (0);
}

int
closeUdpConn(void)
{
    V_UDPDIAG(("Closing UDP socket\n"));
    if (udpSock < 0) {
	fprintf(stderr, "ntserv: tried to close a closed UDP socket\n");
	return (-1);
    }
    shutdown(udpSock, 2);	/* wham */
    close(udpSock);		/* bam */
    udpSock = -1;		/* (nah) */

    return (0);
}

static void
handleUdpReq(struct udp_req_cpacket *packet)
{
    struct udp_reply_spacket response;
    int     mode;

    response.type = SP_UDP_REPLY;

    if (packet->request == COMM_VERIFY) {
	/* this request should ONLY come through the UDP connection */
	if (commMode == COMM_UDP) {
	    UDPDIAG(("Got second verify from %s; resending server verify\n",
		     me->p_name));
	    response.reply = SWITCH_VERIFY;
	    goto send;
	}
	UDPDIAG(("Receieved UDP verify from %s\n", me->p_name));
	UDPDIAG(("--- UDP connection established to %s\n", me->p_name));
#ifdef BROKEN
	warning("WARNING: BROKEN mode is enabled");
#endif

	resetUDPsequence();	/* reset sequence numbers */
	commMode = COMM_UDP;	/* at last */
	udpMode = MODE_SIMPLE;	/* just send one at a time */

	/* note that we don't NEED to send a SWITCH_VERIFY packet; the client */
	/*
	   will change state when it receives ANY packet on the UDP
	   connection
	*/
	/* (this just makes sure that it gets one) */
	/* (update: recvfrom() currently tosses the first packet it gets...)  */
	response.reply = SWITCH_VERIFY;
	goto send;
/*	return;*/
    }
    if (packet->request == COMM_MODE) {
	/* wants to switch modes; mode is in "conmode" */
	mode = packet->connmode;
	if (mode < MODE_TCP || mode > MODE_DOUBLE) {
	    warning("Server can't do that UDP mode");
	    UDPDIAG(("Got bogus request for UDP mode %d from %s\n",
		     mode, me->p_name));
	}
	else {
	    /* I don't bother with a reply, though it can mess up the opt win */
	    switch (mode) {
	    case MODE_TCP:
		warning("Server will send with TCP only");
		break;
	    case MODE_SIMPLE:
		warning("Server will send with simple UDP");
		break;
	    case MODE_FAT:
		warning("Server will send with fat UDP; sent full update");
		V_UDPDIAG(("Sending full update to %s\n", me->p_name));
		forceUpdate();
		break;
#ifdef DOUBLE_UDP
	    case MODE_DOUBLE:
		warning("Server will send with double UDP");
		scbufptr = scbuf + sizeof(struct sc_sequence_spacket);
		break;
#else
	    case MODE_DOUBLE:
		warning("Request for double UDP DENIED (set to simple)");
		mode = MODE_SIMPLE;
		break;
#endif				/* DOUBLE_UDP */
	    }

	    udpMode = mode;
	    UDPDIAG(("Switching %s to UDP mode %d\n", me->p_name, mode));
	}
	return;
    }
    if (packet->request == COMM_UPDATE) {
	/* client wants a FULL update */
	V_UDPDIAG(("Sending full update to %s\n", me->p_name));
	forceUpdate();

	return;
    }
    UDPDIAG(("Received request for %s mode from %s\n",
	     (packet->request == COMM_TCP) ? "TCP" : "UDP", me->p_name));
    if (packet->request == commMode) {
	/* client asking to switch to current mode */
	if (commMode == COMM_UDP) {
	    /*
	       client must be confused... whatever the cause, he obviously
	       isn't connected to us, so we better drop out end and retry.
	    */
	    UDPDIAG(("Rcvd UDP req from %s while in UDP mode; dropping old\n",
		     me->p_name));
	    closeUdpConn();
	    commMode = COMM_TCP;
	    /* ...and fall thru to the UDP request handler */
	}
	else {
	    /*
	       Again, client is confused.  This time there's no damage
	       though. Just tell him that he succeeded.  Could also happen if
	       the client tried to connect to our UDP socket but failed, and
	       decided to back off.
	    */
	    UDPDIAG(("Rcvd TCP req from %s while in TCP mode\n", me->p_name));

	    response.reply = SWITCH_TCP_OK;
	    sendClientPacket((struct player_spacket *) & response);

	    if (udpSock >= 0) {
		closeUdpConn();
		UDPDIAG(("Closed UDP socket\n"));
	    }
	    return;
	}
    }
    /* okay, we have a request to change modes */
    if (packet->request == COMM_UDP) {
	udpClientPort = ntohl(packet->port);	/* where to connect to */
	if (!configvals->udpAllowed) {
	    UDPDIAG(("Rejected UDP request from %s\n", me->p_name));
	    response.reply = SWITCH_DENIED;
	    response.port = htons(0);
	    goto send;
	}
	else {
	    if (userUdpVersion != UDPVERSION) {
		char    buf[80];
		sprintf(buf, "Server UDP is v%.1f, client is v%.1f",
			(float) UDPVERSION / 10.0,
			(float) userUdpVersion / 10.0);
		warning(buf);
		UDPDIAG(("%s (rejected %s)\n", buf, me->p_name));
		response.reply = SWITCH_DENIED;
		response.port = htons(1);
		goto send;
	    }
	    if (udpSock >= 0) {
		/* we have a socket open, but the client doesn't seem aware */
		/* (probably because our UDP verify got lost down the line) */
		UDPDIAG(("Receieved second request from %s, reconnecting\n",
			 me->p_name));
		closeUdpConn();
	    }
	    /* (note no openUdpConn(); we go straight to connect) */
	    if (connUdpConn() < 0) {
		response.reply = SWITCH_DENIED;
		response.port = 0;
		goto send;
	    }
	    UDPDIAG(("Connected UDP socket (%d:%d) for %s\n", udpSock,
		     udpLocalPort, me->p_name));

	    /* we are now connected to the client, but he's merely bound */
	    /* don't switch to UDP mode yet; wait until client connects */
	    response.reply = SWITCH_UDP_OK;
	    response.port = htonl(udpLocalPort);

	    UDPDIAG(("packet->connmode = %d\n", packet->connmode));
	    if (packet->connmode == CONNMODE_PORT) {
		/* send him our port # so he can connect to us */
		goto send;
	    }
	    else {		/* send him a packet; he'll get port from
				   recvfrom() */
		int     t = sizeof(response);
		if (gwrite(udpSock, (char *) &response, sizeof(response)) != t) {
		    UDPDIAG(("Attempt to send UDP packet failed; using alt\n"));
		}
		goto send;
	    }

	}
    }
    else if (packet->request == COMM_TCP) {
	closeUdpConn();
	commMode = COMM_TCP;
	response.reply = SWITCH_TCP_OK;
	response.port = 0;
	UDPDIAG(("Closed UDP socket for %s\n", me->p_name));
	goto send;
    }
    else {
	fprintf(stderr, "ntserv: got weird UDP request (%d)\n",
		packet->request);
	return;
    }
send:
    sendClientPacket((struct player_spacket *) & response);
}


/* used for debugging */
void
printUdpInfo(void)
{
    struct sockaddr_in addr;
    int     len;

    len = sizeof(addr);
    if (getsockname(udpSock, (struct sockaddr *) & addr, &len) < 0) {
	perror("printUdpInfo: getsockname");
	return;
    }
    UDPDIAG(("LOCAL: addr=0x%lx, family=%d, port=%d\n",
	     (u_long) addr.sin_addr.s_addr,
	     addr.sin_family, ntohs(addr.sin_port)));

    if (getpeername(udpSock, (struct sockaddr *) & addr, &len) < 0) {
	perror("printUdpInfo: getpeername");
	return;
    }
    UDPDIAG(("PEER : addr=0x%lx, family=%d, port=%d\n",
	     (u_long) addr.sin_addr.s_addr,
	     addr.sin_family, ntohs(addr.sin_port)));
}

static void 
handleSequence(void)
{
    /* we don't currently deal with sequence numbers from clients */
}

static void 
handleAskMOTD(void)
{
    sendMotd();
}


#ifdef DOUBLE_UDP
/*
 * If we're in double-UDP mode, then we need to send a separate semi-critical
 * transmission over UDP.  We need to give it the same sequence number as the
 * previous transmission, but the sequence packet will have type
 * SP_CP_SEQUENCE instead of SP_SEQUENCE.
 */
void 
sendSC(void)
{
    struct sequence_spacket *ssp;
    struct sc_sequence_spacket *sc_sp;
    int     cc;

    if (commMode != COMM_UDP || udpMode != MODE_DOUBLE) {
	/* mode not active, keep buffer clear */
	scbufptr = scbuf;
	return;
    }
    if (scbufptr - scbuf <= sizeof(struct sc_sequence_spacket)) {
	/* nothing to send */
	return;
    }
    /* copy sequence #, send what we got, then reset buffer */
    sc_sp = (struct sc_sequence_spacket *) scbuf;
    ssp = (struct sequence_spacket *) udpbuf;
    sc_sp->type = SP_SC_SEQUENCE;
    sc_sp->sequence = ssp->sequence;
    if ((cc = gwrite(udpSock, scbuf, scbufptr - scbuf)) != scbufptr - scbuf) {
	fprintf(stderr, "UDP sc gwrite failed (%d, error %d)\n", cc, errno);
	UDPDIAG(("*** UDP diSConnected for %s\n", me->p_name));
	printUdpInfo();
	closeUdpConn();
	commMode = COMM_TCP;
	return;
    }
    scbufptr = scbuf + sizeof(struct sc_sequence_spacket);
}

#endif

/*
 * This is a truncated version of initClientData().  Note that it doesn't
 * explicitly reset all the fat UDP stuff; sendClientData will take care of
 * that by itself eventually.
 *
 * Only semi-critical data is sent, with a few exceptions for non-critical
 * data which would be nice to update (stats, kills, player posn, etc).
 * The critical stuff can't be lost, so there's no point in resending it.
 *
 * (Since the fat data begins in an unqueued state, forceUpdate() should be
 * called immediately after switching to fat mode.  This guarantees that every
 * packet will end up on a queue.  The only real reason for doing this is so
 * that switching to fat mode will clear up your display and keep it cleared;
 * otherwise you could have torps floating around forever because the packet
 * for them isn't on a queue.  Will it reduce the effectiveness of fat UDP?
 * No, because as soon as the player hits the "update all" key it's gonna
 * happen anyway...)
 */
static void 
forceUpdate(void)
{
    static time_t lastone = 0;
    time_t  now;
    int     i;

    now = time(0);
    if (now - lastone < UDP_UPDATE_WAIT) {
	warning("Update request DENIED (chill out!)");
	return;
    }
    lastone = now;

/*    clientDead=0;*/
    for (i = 0; i < MAXPLAYER; i++) {
	clientHostile[i].hostile = -1;
	clientStats[i].losses = -1;	/* (non-critical, but nice) */
/*	clientLogin[i].rank= -1;		(critical) */
/*	clientPlayersInfo[i].shiptype= -1;	(critical) */
/*	clientPStatus[i].status= -1;		(critical) */
	clientPlayers[i].x = htonl(-1);	/* (non-critical, but nice) */
	clientPhasers[i].status = -1;
	clientKills[i].kills = htonl(-1);	/* (non-critical, but nice) */
	clientFlags[i].flags = htonl(-1);
	mustUpdate[i] = 0;
    }
    for (i = 0; i < MAXPLAYER * MAXTORP; i++) {
	clientTorpsInfo[i].status = -1;
/*	clientTorps[i].x= -1;			(non-critical) */
    }
    for (i = 0; i < MAXPLAYER * MAXPLASMA; i++) {
	clientPlasmasInfo[i].status = -1;
/*	clientPlasmas[i].x= -1;			(non-critical) */
    }
    for (i = 0; i < TOTALTHINGIES; i++) {
	
	clientThingysInfo[i].shape = htons(-1);
/*	clientThingys[i].x= -1;			(non-critical) */
    }
    for (i = 0; i < MAXPLANETS; i++) {
	clientPlanets2[i].armies = htonl(-2);
/*	clientPlanetLocs[i].x= htonl(-1);	(critical) */
    }
/*    msgCurrent=(mctl->mc_current+1) % MAXMESSAGE;*/
    clientSelf.pnum = -1;
}

/* note - this should really be replaced with a conf.whatever file that
   lists the logins of people that can't shut up */
static int 
isCensured(char *s)		/* return true if cannot message opponents */
{
    return (
#if 0
	    (strncmp(s, "am4m", 4) == 0) ||	/* 7/21/91 TC */
	    (strncmp(s, "dm3e", 4) == 0) ||	/* 7/21/91 TC */
	    (strncmp(s, "gusciora", 8) == 0) ||	/* 7/25/91 TC */
	    (strncmp(s, "flan", 4) == 0) ||	/* 4/2/91 TC */
	    (strncmp(s, "kc3b", 4) == 0) ||	/* 4/4/91 TC */
	    (strncmp(s, "windom", 6) == 0) ||	/* 7/20/92 TC */
#endif
	    0
	);
}

/* return true if you should eat message */

static int 
parseIgnore(struct mesg_cpacket *packet)
{
    char   *s;
    int     who;
    int     what;
    char    buf[80];
    int     noneflag;

/*    if (packet->indiv != me->p_no) return 0;*/

    s = (char *) packet->mesg;

    who = packet->indiv;
    if ((*s != ':') && (strncmp(s, "     ", 5) != 0))
	return 0;
    if ((who == me->p_no) || (*s == ' ')) {	/* check for borg call 4/6/92
						   TC */
	if (configvals->binconfirm)
	    warning("No cyborgs allowed in the game at this time.");
	else {
	    char    buf[80];
	    char    buf2[5];
	    int     i;
	    int     cybflag = 0;

	    strcpy(buf, "Possible cyborgs: ");
	    for (i = 0; i < MAXPLAYER; i++)
		if ((players[i].p_status != PFREE) &&
		    (players[i].p_stats.st_flags & ST_CYBORG)) {
		    sprintf(buf2, "%s ", twoletters(&players[i]));
		    strcat(buf, buf2);
		    cybflag = 1;
		}
	    if (!cybflag)
		strcat(buf, "None");
	    warning(buf);
	}
	if (*s != ' ')		/* if not a borg call, eat msg 4/6/92 TC */
	    return 1;
	else
	    return 0;		/* otherwise, send it 4/6/92 TC */
    }
    if (packet->group != MINDIV)
	return 0;		/* below is for indiv only */

    do {
	what = 0;
	switch (*(++s)) {
	case 'a':
	case 'A':
	    what = MALL;
	    break;
	case 't':
	case 'T':
	    what = MTEAM;
	    break;
	case 'i':
	case 'I':
	    what = MINDIV;
	    break;
	case '\0':
	    what = 0;
	    break;
	default:
	    what = 0;
	    break;
	}
	ignored[who] ^= what;
    } while (what != 0);

    strcpy(buf, "Ignore status for this player: ");
    noneflag = 1;
    if (ignored[who] & MALL) {
	strcat(buf, "All ");
	noneflag = 0;
    }
    if (ignored[who] & MTEAM) {
	strcat(buf, "Team ");
	noneflag = 0;
    }
    if (ignored[who] & MINDIV) {
	strcat(buf, "Indiv ");
	noneflag = 0;
    }
    if (noneflag)
	strcat(buf, "None");
    warning(buf);
    return 1;
}

/* give session stats if you send yourself a '?' 2/27/92 TC */
/* or '!' for ping stats (HAK) */
/* merged RSA query '#' here, too (HAK) */
/* return true if you should eat message */

static int 
parseQuery(struct mesg_spacket *packet)
{
    char    buf[80];
    float   sessionBombing, sessionPlanets, sessionOffense, sessionDefense;
    int     deltaArmies, deltaPlanets, deltaKills, deltaLosses, deltaTicks;

    extern int startTkills, startTlosses, startTarms, startTplanets, startTticks;

    /* 0-8 for address, 9 is space */

    if (packet->mesg[11] != '\0')	/* one character only */
	return 0;

    switch (packet->mesg[10]) {
    case '!':
	return bouncePingStats(packet);
    case '#':
	sprintf(buf, "Client: %s", RSA_client_type);
	bounce(buf, packet->m_from);
	return 1;
    case '?':
	deltaPlanets = me->p_stats.st_tplanets - startTplanets;
	deltaArmies = me->p_stats.st_tarmsbomb - startTarms;
	deltaKills = me->p_stats.st_tkills - startTkills;
	deltaLosses = me->p_stats.st_tlosses - startTlosses;
	deltaTicks = me->p_stats.st_tticks - startTticks;

	if (deltaTicks == 0)
	    return 1;		/* can happen if no tmode */

	sessionPlanets = (float) deltaPlanets *status->timeprod /
	        ((float) deltaTicks * status->planets);

	sessionBombing = (float) deltaArmies *status->timeprod /
	        ((float) deltaTicks * status->armsbomb);

	sessionOffense = (float) deltaKills *status->timeprod /
	        ((float) deltaTicks * status->kills);

	sessionDefense = (float) deltaTicks *status->losses /
	        (deltaLosses != 0 ?
		         ((float) deltaLosses * status->timeprod) :
		         (status->timeprod));

	sprintf(buf, "%2s stats: %d planets and %d armies. %d wins/%d losses. %5.2f hours.",
		twoletters(me),
		deltaPlanets,
		deltaArmies,
		deltaKills,
		deltaLosses,
		(float) deltaTicks / 36000.0);
	bounce(buf, packet->m_from);
	sprintf(buf, "Ratings: Pla: %5.2f  Bom: %5.2f  Off: %5.2f  Def: %5.2f  Ratio: %4.2f",
		sessionPlanets,
		sessionBombing,
		sessionOffense,
		sessionDefense,
		(float) deltaKills /
		(float) ((deltaLosses == 0) ? 1 : deltaLosses));
	bounce(buf, packet->m_from);
	return 1;
    default:
	return 0;
    }
    /* NOTREACHED */
}

static int 
bouncePingStats(struct mesg_spacket *packet)
{
    char    buf[80];

    if (me->p_avrt == -1) {
	/* client doesn't support it or server not pinging */
	sprintf(buf, "No ping stats available for %s",
		twoletters(me));
    }
    else {
	sprintf(buf, "%s ping stats: Average: %d ms, Stdv: %d ms, Loss: %d%%",
		twoletters(me),
		me->p_avrt,
		me->p_stdv,
		me->p_pkls);
    }
    bounce(buf, packet->m_from);

    return 1;
}

/* new code, sends bouncemsg to bounceto from GOD 4/17/92 TC */
static void 
bounce(char *bouncemsg, int bounceto)
{
    char    buf[10];

    sprintf(buf, "GOD->%s", twoletters(&players[bounceto]));
    pmessage(bouncemsg, bounceto, MINDIV, buf);
}


/*
 *
 */

void 
sendShipCap(void)
{
    struct ship_cap_spacket temppack;
    struct ship ship;
    int     i;

    if (!blk_flag)
	return;
    for (i = 0; i < NUM_TYPES; i++) {
	getship(&ship, i);
	temppack.type = SP_SHIP_CAP;
	temppack.operation = 0;
	temppack.s_type = htons(ship.s_type);
	temppack.s_torpspeed = htons(ship.s_torp.speed);
	temppack.s_phaserrange = htons(ship.s_phaser.speed);
	temppack.s_maxspeed = htonl(ship.s_imp.maxspeed);
	temppack.s_maxfuel = htonl(ship.s_maxfuel);
	temppack.s_maxshield = htonl(ship.s_maxshield);
	temppack.s_maxdamage = htonl(ship.s_maxdamage);
	temppack.s_maxwpntemp = htonl(ship.s_maxwpntemp);
	temppack.s_maxegntemp = htonl(ship.s_maxegntemp);
	temppack.s_width = htons(ship.s_width);
	temppack.s_height = htons(ship.s_height);
	temppack.s_maxarmies = htons(ship.s_maxarmies);
	temppack.s_armies = ((int)(10 * ship.s_armyperkill)) & 0x7f;
	if(ship.s_nflags & SFNARMYNEEDKILL)
	  temppack.s_armies |= 0x80;
	temppack.s_letter = ship.s_letter;
	temppack.s_desig1 = ship.s_desig1;
	temppack.s_desig2 = ship.s_desig2;
	if (blk_flag == 1)
	    temppack.s_bitmap = htons(ship.s_alttype);
	else
	    temppack.s_bitmap = htons(ship.s_bitmap);
	sendClientPacket((struct player_spacket *) & temppack);
    }
}

void 
sendMotdPic(int x, int y, char *bits, int page, int width, int height)
{
    struct motd_pic_spacket temppack;
    short   sx, sy, sp, sw, sh;
    int     size;

    size = (width / 8 + (width % 8 != 0)) * height;
    sx = x;
    sy = y;
    sp = page;
    sw = width;
    sh = height;
    temppack.type = SP_MOTD_PIC;
    temppack.x = htons(sx);
    temppack.y = htons(sy);
    temppack.width = htons(sw);
    temppack.height = htons(sh);
    temppack.page = htons(sp);
    memcpy(temppack.bits, bits, size);

    sendClientPacket((struct player_spacket *) & temppack);
}


void 
sendMotdNopic(int x, int y, int page, int width, int height)
{
    struct pe1_missing_bitmap_spacket temppack;

    temppack.type = SP_PARADISE_EXT1;
    temppack.subtype = SP_PE1_MISSING_BITMAP;
    temppack.page = htons((short) page);
    temppack.x = htons((short) x);
    temppack.y = htons((short) y);
    temppack.width = htons((short) width);
    temppack.height = htons((short) height);

    sendClientPacket((struct player_spacket *) & temppack);
}

/* tells the client how many missiles carried [BDyess] */
void
sendMissileNum(int num)
{

    /* remove the 1 || to enable missile updates [BDyess] */
    if (clientMissiles.num == htons(num))
	return;

    clientMissiles.type = SP_PARADISE_EXT1;
    clientMissiles.subtype = SP_PE1_NUM_MISSILES;
    clientMissiles.num = htons(num);

    sendClientPacket((struct player_spacket *) & clientMissiles);
}

/* this code was copied from

    portname.c, part of
    faucet and hose: network pipe utilities
    Copyright (C) 1992 Robert Forsman

    He has granted the Paradise project permission to use this code
    for non-profit purposes.

*/

static int
convert_hostname(char *name, struct in_addr *addr)
{
    struct hostent   *hp;
    int               len;

    hp = gethostbyname(name);
    if (hp != NULL)
        memcpy(addr, hp->h_addr, hp->h_length);
    else {
        int             count;
        unsigned int    a1,a2,a3,a4;

        count = sscanf(name,"%i.%i.%i.%i%n", &a1, &a2, &a3, &a4, &len);

        if (4!=count || 0!=name[len] )
            return 0;

        addr->s_addr = (((((a1 << 8) | a2) << 8) | a3) << 8) | a4;
    }
    return 1;
}

/*
  figure out if our client is exempt from RSA authentication.

  The host name resolution above doesn't handle gateways, which can
  have more than one internet address :/
*/

int
site_rsa_exempt(void) 
{
    FILE    *fp;
    char    buf[256];

    if (remoteaddr == -1) {
        printf("remote address is not yet available?!\n");
        return 0;        /* weird */
    }
    
    /* hopefully we've got the remote address at this point */

    fp = fopen(build_path(RSA_EXEMPTION_FILE), "r");

    if (!fp)
        return 0;        /* nobody is exempt */

    while (fgets(buf, sizeof(buf), fp)) {
        char              hostname[256];
        char             *playername;
        int               len;
        int               i;
        struct in_addr    addr;

        len = strlen(buf);

        if (buf[len-1]=='\n')
            buf[len-1] = 0;

        for (i=0; buf[i] && !isspace(buf[i]); i++)
            hostname[i] = buf[i];

        hostname[i] = 0;    /* hostname is copied to buffer */

        while (buf[i] && isspace(buf[i]))
            i++;

        playername = buf+i;    /* player name is stuff after hostname */

        if (!(*playername == 0 || strcmp(playername, me->p_name) == 0))
            continue;        /* name didn't match */

        /* shit, I gotta parse this crap myself.  I'll steal this code
           from hose - RF */
        if (!convert_hostname(hostname, &addr)) {
            printf("address in %s unparseable `%s'\n",
               RSA_EXEMPTION_FILE, hostname);
            continue;
        }

        if (addr.s_addr == remoteaddr)
            return 1;
    } /* while (line in rsa-exempt file) */

    fclose(fp);

    return 0;
}
