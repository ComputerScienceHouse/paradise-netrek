/*
 * Socket.c
 *
 * Kevin P. Smith 1/29/89
 * UDP stuff v1.0 by Andy McFadden  Feb-Apr 1992
 *
 * UDP protocol v1.0
 *
 * Routines to allow connection to the xtrek server.
 */
#include "copyright2.h"

/* to see the packets sent/received: [BDyess] */
#if 0
#define SHOW_SEND
#define SHOW_RECEIVED
#endif				/* 0 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif  
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif  
#ifdef HAVE_NETINET_IN_H 
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif 
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif
#include "str.h"
        
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "packets.h"
#include "gppackets.h"

#ifdef UNIX_SOUND
#include "sound.h"
#endif  

#define BIGINT 2000000000

#define UDP_PORTSWAP

int     send_total = 0;
int     receive_total = 0;

/* Prototypes */
static void resetForce P((void));
static void checkForce P((void));
static int doRead P((int asock));
static void handleTorp P((struct torp_spacket * packet));
static void handleTorpInfo P((struct torp_info_spacket * packet));
static void handleStatus P((struct status_spacket * packet));
static void handleSelf P((struct you_spacket * packet));
static void handlePlayer P((struct player_spacket * packet));
static void handleWarning P((struct warning_spacket * packet));
void sendServerPacket P((struct player_spacket * packet));
static void handlePlanet P((struct planet_spacket * packet));
static void handlePhaser P((struct phaser_spacket * packet));
void handleMessage P((struct mesg_spacket * packet));
static void handleQueue P((struct queue_spacket * packet));
static void handlePickok P((struct pickok_spacket * packet));
static void handleLogin P((struct login_spacket * packet));
static void handlePlasmaInfo P((struct plasma_info_spacket * packet));
static void handlePlasma P((struct plasma_spacket * packet));
static void handleFlags P((struct flags_spacket * packet));
static void handleKills P((struct kills_spacket * packet));
static void handlePStatus P((struct pstatus_spacket * packet));
static void handleMotd P((struct motd_spacket * packet));
static void handleMask P((struct mask_spacket * packet));
static void pickSocket P((int old));
static void handleBadVersion P((struct badversion_spacket * packet));
int gwrite P((int fd, char *buf, int bytes));
static void handleHostile P((struct hostile_spacket * packet));
static void handlePlyrLogin P((struct plyr_login_spacket * packet));
static void handleStats P((struct stats_spacket * packet));
static void handlePlyrInfo P((struct plyr_info_spacket * packet));
static void handlePlanetLoc P((struct planet_loc_spacket * packet));
static void handleReserved P((struct reserved_spacket * packet));

static void handleScan P((struct scan_spacket * packet));
static void handleSequence P((struct sequence_spacket * packet));
static void handleUdpReply P((struct udp_reply_spacket * packet));
static void informScan P((int p));
static int openUdpConn P((void));
#ifdef UDP_PORTSWAP
static int connUdpConn P((void));
#endif
static int recvUdpConn P((void));
static void printUdpInfo P((void));
/*static void dumpShip P((struct ship *shipp ));*/
/*static int swapl P((int in ));*/
static void handleShipCap P((struct ship_cap_spacket * packet));
static void handleMotdPic P((struct motd_pic_spacket * packet));
static void handleStats2 P((struct stats_spacket2 * packet));
static void handleStatus2 P((struct status_spacket2 * packet));
static void handlePlanet2 P((struct planet_spacket2 * packet));
static void handleTerrain2 P((struct terrain_packet2 * pkt));
static void handleTerrainInfo2 P((struct terrain_info_packet2 *pkt));
static void handleTempPack P((struct obvious_packet * packet));
static void handleThingy P((struct thingy_spacket * packet));
static void handleThingyInfo P((struct thingy_info_spacket * packet));
static void handleRSAKey P((struct rsa_key_spacket * packet));
void    handlePing();
static void handleExtension1 P((struct paradiseext1_spacket *));

static void handleEmpty();


void    handleShortReply(), handleVPlayer(), handleVTorp(),
        handleSelfShort(), handleSelfShip(), handleVPlanet(), handleSWarning();
void    handleVTorpInfo(), handleSMessage();

void	handleVPhaser(), handleVKills(), handle_s_Stats();

void    handleFeature();	/* feature.c */

struct packet_handler handlers[] = {
    {NULL},			/* record 0 */
    {handleMessage},		/* SP_MESSAGE */
    {handlePlyrInfo},		/* SP_PLAYER_INFO */
    {handleKills},		/* SP_KILLS */
    {handlePlayer},		/* SP_PLAYER */
    {handleTorpInfo},		/* SP_TORP_INFO */
    {handleTorp},		/* SP_TORP */
    {handlePhaser},		/* SP_PHASER */
    {handlePlasmaInfo},		/* SP_PLASMA_INFO */
    {handlePlasma},		/* SP_PLASMA */
    {handleWarning},		/* SP_WARNING */
    {handleMotd},		/* SP_MOTD */
    {handleSelf},		/* SP_YOU */
    {handleQueue},		/* SP_QUEUE */
    {handleStatus},		/* SP_STATUS */
    {handlePlanet},		/* SP_PLANET */
    {handlePickok},		/* SP_PICKOK */
    {handleLogin},		/* SP_LOGIN */
    {handleFlags},		/* SP_FLAGS */
    {handleMask},		/* SP_MASK */
    {handlePStatus},		/* SP_PSTATUS */
    {handleBadVersion},		/* SP_BADVERSION */
    {handleHostile},		/* SP_HOSTILE */
    {handleStats},		/* SP_STATS */
    {handlePlyrLogin},		/* SP_PL_LOGIN */
    {handleReserved},		/* SP_RESERVED */
    {handlePlanetLoc},		/* SP_PLANET_LOC */
    {handleScan},		/* SP_SCAN (ATM) */
    {handleUdpReply},		/* SP_UDP_STAT */
    {handleSequence},		/* SP_SEQUENCE */
    {handleSequence},		/* SP_SC_SEQUENCE */
    {handleRSAKey},		/* SP_RSA_KEY */
    {handleMotdPic},		/* SP_MOTD_PIC */
    {handleStats2},		/* SP_STATS2 */
    {handleStatus2},		/* SP_STATUS2 */
    {handlePlanet2},		/* SP_PLANET2 */
    {handleTempPack},		/* SP_TEMP_5 */
    {handleThingy},		/* SP_THINGY */
    {handleThingyInfo},		/* SP_THINGY_INFO */
    {handleShipCap},		/* SP_SHIP_CAP */

    {handleShortReply},		/* SP_S_REPLY */
    {handleSMessage},		/* SP_S_MESSAGE */
    {handleSWarning},		/* SP_S_WARNING */
    {handleSelfShort},		/* SP_S_YOU */
    {handleSelfShip},		/* SP_S_YOU_SS */
    {handleVPlayer},		/* SP_S_PLAYER */
    {handlePing},		/* SP_PING */
    {handleVTorp},		/* SP_S_TORP */
    {handleVTorpInfo},		/* SP_S_TORP_INFO */
    {handleVTorp},		/* SP_S_8_TORP */
    {handleVPlanet},		/* SP_S_PLANET */
    {handleGameparams},
    {handleExtension1},
    {handleTerrain2},		/* 53 */
    {handleTerrainInfo2},	/* 54 */
    {handleEmpty},		/* 55 */
    {handleEmpty},		/* SP_S_SEQUENCE */
    {handleVPhaser},		/* SP_S_PHASER */
    {handleVKills},		/* SP_S_KILLS */
    {handle_s_Stats},		/* SP_S_STATS */
    {handleFeature},		/* SP_FEATURE */
};

#define NUM_HANDLERS	(sizeof(handlers)/sizeof(*handlers))

#define NUM_PACKETS (sizeof(handlers) / sizeof(handlers[0]) - 1)

int     serverDead = 0;

/* prints the total number of bytes sent/received.  Called when exiting the
   client [BDyess] */
void
print_totals(void)
{
    time_t  timeSpent = time(NULL) - timeStart;
    
    if(timeStart == 0 || timeSpent == 0) {	/* never connected [BDyess] */
      printf("%8d bytes sent.\n%8d bytes received.\n",send_total,receive_total);
      return;
    }

    /*
       printf("Total bytes sent:     %d\nTotal bytes received: %d\n",
       send_total, receive_total);
    */
    timeSpent = timeSpent ? timeSpent : 1;
    /* ftp format [BDyess] */
    if (timeSpent < 600 /* 10 minutes */ ) {
	printf("%8d bytes sent     in %d seconds (%.3f Kbytes/s)\n",
	       send_total, (int)timeSpent,
	       (float) send_total / (1024.0 * timeSpent));
	printf("%8d bytes received in %d seconds (%.3f Kbytes/s)\n",
	       receive_total, (int)timeSpent,
	       (float) receive_total / (1024.0 * timeSpent));
    } else {			/* number too big for seconds, use minutes */
	printf("%8d bytes sent     in %.1f minutes (%.3f Kbytes/s)\n",
	       send_total, timeSpent / 60.0,
	       (float) send_total / (1024.0 * timeSpent));
	printf("%8d bytes received in %.1f minutes (%.3f Kbytes/s)\n",
	       receive_total, timeSpent / 60.0,
	       (float) receive_total / (1024.0 * timeSpent));
    }
}

int     udpLocalPort = 0;
static int udpServerPort = 0;
static u_long serveraddr = 0;
static u_short serverport = 0;
static long sequence = 0;
static int drop_flag = 0;
static int chan = -1;		/* tells sequence checker where packet is
				   from */
static short fSpeed, fDirection, fShield, fOrbit, fRepair, fBeamup, fBeamdown, fCloak,
        fBomb, fDockperm, fPhaser, fPlasma, fPlayLock, fPlanLock, fTractor,
        fRepress;

/* reset all the "force command" variables */
static void
resetForce(void)
{
    fSpeed = fDirection = fShield = fOrbit = fRepair = fBeamup = fBeamdown =
    fCloak = fBomb = fDockperm = fPhaser = fPlasma = fPlayLock = fPlanLock =
    fTractor = fRepress = -1;
}

/*
 * If something we want to happen hasn't yet, send it again.
 *
 * The low byte is the request, the high byte is a max count.  When the max
 * count reaches zero, the client stops trying.  Checking is done with a
 * macro for speed & clarity.
 */
#define FCHECK_FLAGS(flag, force, const) {                      \
        if (force > 0) {                                        \
            if (((me->p_flags & flag) && 1) ^ ((force & 0xff) && 1)) {  \
                speedReq.type = const;                          \
                speedReq.speed = (force & 0xff);                \
                sendServerPacket((struct player_spacket *)&speedReq);   \
                V_UDPDIAG(("Forced %d:%d\n", const, force & 0xff));     \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}
#define FCHECK_VAL(value, force, const) {                       \
        if (force > 0) {                                        \
            if ((value) != (force & 0xff)) {                    \
                speedReq.type = const;                          \
                speedReq.speed = (force & 0xff);                \
                sendServerPacket((struct player_spacket *)&speedReq);   \
                V_UDPDIAG(("Forced %d:%d\n", const, force & 0xff));     \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}
#define FCHECK_TRACT(flag, force, const) {                      \
        if (force > 0) {                                        \
            if (((me->p_flags & flag) && 1) ^ ((force & 0xff) && 1)) {  \
                tractorReq.type = const;                        \
                tractorReq.state = ((force & 0xff) >= 0x40);    \
                tractorReq.pnum = (force & 0xff) & (~0x40);     \
                sendServerPacket((struct player_spacket *)&tractorReq); \
                V_UDPDIAG(("Forced %d:%d/%d\n", const,          \
                        tractorReq.state, tractorReq.pnum));    \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}

static void
checkForce(void)
{
    struct speed_cpacket speedReq;
    struct tractor_cpacket tractorReq;

    /* upgrading kludge [BDyess] */
    if (!upgrading)
	FCHECK_VAL(me->p_speed, fSpeed, CP_SPEED);	/* almost always repeats */
    FCHECK_VAL(me->p_dir, fDirection, CP_DIRECTION);	/* (ditto) */
    FCHECK_FLAGS(PFSHIELD, fShield, CP_SHIELD);
    FCHECK_FLAGS(PFORBIT, fOrbit, CP_ORBIT);
    FCHECK_FLAGS(PFREPAIR, fRepair, CP_REPAIR);
    FCHECK_FLAGS(PFBEAMUP, fBeamup, CP_BEAM);
    FCHECK_FLAGS(PFBEAMDOWN, fBeamdown, CP_BEAM);
    FCHECK_FLAGS(PFCLOAK, fCloak, CP_CLOAK);
    FCHECK_FLAGS(PFBOMB, fBomb, CP_BOMB);
    FCHECK_FLAGS(PFDOCKOK, fDockperm, CP_DOCKPERM);
    FCHECK_VAL(phasers[me->p_no].ph_status, fPhaser, CP_PHASER);	/* bug: dir 0 */
    FCHECK_VAL(plasmatorps[me->p_no].pt_status, fPlasma, CP_PLASMA);	/* (ditto) */
    FCHECK_FLAGS(PFPLOCK, fPlayLock, CP_PLAYLOCK);
    FCHECK_FLAGS(PFPLLOCK, fPlanLock, CP_PLANLOCK);

    /* kludge to help prevent self-deflects */
    if(! (hockey && me->p_tractor == 'g'-'a'+10 /*puck*/)) {
      FCHECK_TRACT(PFTRACT, fTractor, CP_TRACTOR);
      FCHECK_TRACT(PFPRESS, fRepress, CP_REPRESS);
    }
}


int
idx_to_mask(int i)
{
    if (i == number_of_teams)
	return ALLTEAM;
    return 1 << i;
}

int
mask_to_idx(int m)
{
    switch(m) {
      case NOBODY:
        return INDi;
      case FEDm:
        return FEDi;
      case ROMm:
        return ROMi;
      case KLIm:
        return KLIi;
      case ORIm:
        return ORIi;
      default:
        return number_of_teams;
    }
}

void
connectToServer(int port)
{
    int     s;
    struct sockaddr_in addr;
    struct sockaddr_in naddr;
    int     len;
    fd_set  readfds;
    struct timeval timeout;
    struct hostent *hp;

    serverDead = 0;
    if (sock != -1) {
	shutdown(sock, 2);
	sock = -1;
    }
    sleep(3);			/* I think this is necessary for some unknown
				   reason */

    printf("Waiting for connection (port %d). \n", port);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	printf("I can't create a socket\n");
	EXIT(2);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(s, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
	sleep(10);
	if (bind(s, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
	    sleep(10);
	    if (bind(s, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
		printf("I can't bind to port!\n");

		EXIT(3);
	    }
	}
    }
    listen(s, 1);

    len = sizeof(naddr);

tryagain:
    timeout.tv_sec = 240;	/* four minutes */
    timeout.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(s, &readfds);
    if (select(32, &readfds, NULL, NULL, &timeout) == 0) {
	printf("Well, I think the server died!\n");
	EXIT(0);
    }
    sock = accept(s, (struct sockaddr *) & naddr, &len);

    if (sock == -1) {
	perror("accept");
	goto tryagain;
    }

    close(s);
    pickSocket(port);		/* new socket != port */


    /*
       This is strictly necessary; it tries to determine who the caller is,
       and set "serverName" and "serveraddr" appropriately.
    */
    len = sizeof(struct sockaddr_in);
    if (getpeername(sock, (struct sockaddr *) & addr, &len) < 0) {
	perror("unable to get peername");
	serverName = "nowhere";
    } else {
	hp = gethostbyaddr((char *) &addr.sin_addr.s_addr, sizeof(long), AF_INET);
	serveraddr = addr.sin_addr.s_addr;
	serverport = addr.sin_port;
	if (hp != NULL) {
	    serverName = (char *) malloc(strlen(hp->h_name) + 1);
	    strcpy(serverName, hp->h_name);
	} else {
	    serverName = (char *) malloc(strlen((char *) inet_ntoa(addr.sin_addr)) + 1);
	    strcpy(serverName, (char *) inet_ntoa(addr.sin_addr));
	}
    }
    printf("Connection from server %s (0x%lx)\n", serverName, serveraddr);
}

char *
callServer(int port, char *server)
{
    int     s;
    struct sockaddr_in addr;
    struct hostent *hp;
    serverDead = 0;

    printf("Calling %s on port %d.\n", server, port);
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	printf("I can't create a socket\n");
	EXIT(0);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if ((addr.sin_addr.s_addr = inet_addr(server)) == -1) {
	if ((hp = gethostbyname(server)) == NULL) {
	    /* netrek.org alias lookup [BDyess] */
	    char *buf = (char*)malloc(BUFSIZ);

	    strcpy(buf,server);
	    strcat(buf,".netrek.org");
	    if((hp = gethostbyname(buf)) == NULL) {
	      printf("Who is %s?\n", server);
	      EXIT(0);
	    }
	    server = buf;
	}
	addr.sin_addr.s_addr = *(long *) hp->h_addr;
    }
    serveraddr = addr.sin_addr.s_addr;

    if (connect(s, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
	printf("Server not listening!\n");
	EXIT(0);
    }
    printf("Got connection.\n");

    sock = s;
/* pickSocket is utterly useless with DNet, but the server needs the
   packet to tell it the client is ready to start. */

    startRecorder();
    pickSocket(port);		/* new socket != port */

    return server;
}

int
isServerDead(void)
{
    return (serverDead);
}

void
socketPause(int sec, int usec)
{
    struct timeval timeout;
    fd_set  readfds;

    if (playback)
	return;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    if (udpSock >= 0)		/* new */
	FD_SET(udpSock, &readfds);
    select(32, &readfds, 0, 0, &timeout);
}

int
readFromServer(void)
{
    struct timeval timeout;
    fd_set  readfds;
    int     retval = 0, rs;

    if (playback) {
	while (!pb_update)
	    doRead(sock);
	return 1;
    }

    if (serverDead)
	return (0);
    if (commMode == COMM_TCP)
	drop_flag = 0;		/* just in case */

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    if (udpSock >= 0)
	FD_SET(udpSock, &readfds);
    if ((rs = select(32, &readfds, 0, 0, &timeout)) != 0) {
	if (rs < 0) {
	    /* NEW */
	    perror("select");
	    return 0;
	}
	/* Read info from the xtrek server */
	if (FD_ISSET(sock, &readfds)) {
	    chan = sock;
	    retval += doRead(sock);
	}
	if (udpSock >= 0 && FD_ISSET(udpSock, &readfds)) {
	    /* WAS V_ *//* should be! */
	    V_UDPDIAG(("Activity on UDP socket\n"));
	    chan = udpSock;
	    if (commStatus == STAT_VERIFY_UDP) {
		warning("UDP connection established");
		sequence = 0;	/* reset sequence #s */
		resetForce();
		
		if (udpDebug)
		    printUdpInfo();
		UDPDIAG(("UDP connection established.\n"));
		
		commMode = COMM_UDP;
		commStatus = STAT_CONNECTED;
		commSwitchTimeout = 0;
		if (udpClientRecv != MODE_SIMPLE)
		    sendUdpReq(COMM_MODE + udpClientRecv);
		if (udpWin) {
		    udprefresh(UDP_CURRENT);
		    udprefresh(UDP_STATUS);
		}
	    }
	    retval += doRead(udpSock);
	}
    }

    /* if switching comm mode, decrement timeout counter */
    if (commSwitchTimeout > 0) {
	if (!(--commSwitchTimeout)) {
	    /*
	      timed out; could be initial request to non-UDP server (which won't
	      be answered), or the verify packet got lost en route to the
	      server.  Could also be a request for TCP which timed out (weird),
	      in which case we just reset anyway.
	      */
	    commModeReq = commMode = COMM_TCP;
	    commStatus = STAT_CONNECTED;
	    if (udpSock >= 0)
		closeUdpConn();
	    if (udpWin) {
		udprefresh(UDP_CURRENT);
		udprefresh(UDP_STATUS);
	    }
	    warning("Timed out waiting for UDP response from server");
	    UDPDIAG(("Timed out waiting for UDP response from server\n"));
	}
    }
    /* if we're in a UDP "force" mode, check to see if we need to do something */
    if (commMode == COMM_UDP && udpClientSend > 1)
	checkForce();
    
    return (retval != 0);		/* convert to 1/0 */
}


/* this used to be part of the routine above */
unsigned char buf[BUFSIZ * 2 + 16];

static int
doRead(int asock)
{
    unsigned char   *bufptr;
    int     size;
    int     count;
    int     temp;

    struct timeval timeout;
    fd_set  readfds;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    count = sock_read(asock, buf, 2 * BUFSIZ);
/* TMP */

    if (count <= 0) {
	if (asock == udpSock) {
	    if (errno == ECONNREFUSED) {
		struct sockaddr_in addr;

		UDPDIAG(("asock=%d, sock=%d, udpSock=%d, errno=%d\n",
			 asock, sock, udpSock, errno));
		UDPDIAG(("count=%d\n", count));
		UDPDIAG(("Hiccup(%d)!  Reconnecting\n", errno));
		addr.sin_addr.s_addr = serveraddr;
		addr.sin_port = htons(udpServerPort);
		addr.sin_family = AF_INET;
		if (connect(udpSock, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
		    perror("connect");
		    UDPDIAG(("Unable to reconnect\n"));
		    /* and fall through to disconnect */
		} else {
		    UDPDIAG(("Reconnect successful\n"));
		    return (0);
		}
	    }
	    UDPDIAG(("*** UDP disconnected (res=%d, err=%d)\n",
		     count, errno));
	    warning("UDP link severed");
	    printUdpInfo();
	    closeUdpConn();
	    commMode = commModeReq = COMM_TCP;
	    commStatus = STAT_CONNECTED;
	    if (udpWin) {
		udprefresh(UDP_STATUS);
		udprefresh(UDP_CURRENT);
	    }
	    return (0);
	}
	printf("1) Got read() of %d. Server dead\n", count);
	perror("");
	serverDead = 1;
	return (0);
    }
    bufptr = buf;
    while (bufptr < buf + count) {
computesize:
	if ((*bufptr == SP_S_MESSAGE && (buf + count - bufptr <= 4))
	    || (buf + count - bufptr < 4)) {	/* last part may only be
						   needed for DNet...I'm not
						   so sure any more.
						   certainly doesn't hurt to
						   have it.-JR */
	    /*
	       printf("buf+count-bufptr=%d, *bufptr=%d\n",buf + count -
	       bufptr,*bufptr);
	    */
	    size = 0;		/* problem with reads breaking before size
				   byte for SP_S_MESSAGE has been read. Only
				   a problem for messages because reads break
				   on 4 byte boundaries,  other size bytes
				   are always in the first 4. -JR */
	} else
	{
	    size = size_of_spacket((unsigned char *)bufptr);
	    if (size < 1) {
		fprintf(stderr, "Unknown packet %d.  Aborting.\n",
			*bufptr);
		return (0);
	    }
#ifdef SHOW_RECEIVED
	    printf("recieved packet type %d, size %d\n", *bufptr, size);
#endif				/* SHOW_RECEIVED */
	    light_receive();
	    receive_total += size;
	}
	while (size > count + (buf - bufptr) || size == 0) {
	    /*
	       We wait for up to ten seconds for rest of packet. If we don't
	       get it, we assume the server died.
	    */
	    /*
	       printf("er, possible packet fragment, waiting for the
	       rest...\n");
	    */
	    if (!playback)
	    {
		timeout.tv_sec = 20;
		timeout.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(asock, &readfds);
		/* readfds=1<<asock; */
		temp = select(32, &readfds, 0, 0, &timeout);
		if (temp == 0) {
		    printf("Packet fragment.  Server must be dead\n");
		    serverDead = 1;
		    return (0);
		}
	    }
	    if (size == 0)
		/* 84=largest short packet message - the 4 we have */
		temp = sock_read(asock, buf + count, 84);
	    else
		temp = sock_read(asock, buf + count, size - (count + (buf - bufptr)));
	    count += temp;
	    if (temp <= 0)
	    {
		printf("2) Got read() of %d.  Server is dead\n", temp);
		serverDead = 1;
		return (0);
	    }
	    if (size == 0)	/* for the SP_S_MESSAGE problem */
		goto computesize;
	}
	if (playback && (*bufptr == REC_UPDATE)) {
	    pb_update++;
	    me->p_tractor = bufptr[1];
	    if (me->p_flags & PFPLOCK)
		me->p_playerl = bufptr[2];
	    else
		me->p_planet = bufptr[2];
/*	    printf("Read REC_UPDATE pseudo-packet!\n");*/
	} else
	    if (*bufptr >= 1 &&
		*bufptr < NUM_HANDLERS &&
		handlers[(int) *bufptr].handler != NULL) {
	    if (asock != udpSock ||
		(!drop_flag || *bufptr == SP_SEQUENCE || *bufptr == SP_SC_SEQUENCE)) {
		if (asock == udpSock)
		    packets_received++;	/* ping stuff */
		if (recordGame)
		    recordPacket(bufptr, size);
		(*(handlers[(unsigned char)*bufptr].handler)) (bufptr);
		/* printf("handled packet %d\n", (unsigned char)*bufptr); */
	    } else
		UDPDIAG(("Ignored type %d\n", *bufptr));
	} else {
	    printf("Handler for packet %d not installed...\n", *bufptr);
	}

	bufptr += size;
	if (bufptr > buf + BUFSIZ) {
	    memcpy(buf, buf + BUFSIZ, BUFSIZ);
	    if (count == BUFSIZ * 2) {
		if (playback)
		    temp = 0;
		else
		{
		    FD_ZERO(&readfds);
		    FD_SET(asock, &readfds);
		    /* readfds = 1<<asock; */
		    temp = select(32, &readfds, 0, 0, &timeout);
		}
		if (temp != 0) {
		    temp = sock_read(asock, buf + BUFSIZ, BUFSIZ);
		    count = BUFSIZ + temp;
		    if (temp <= 0)
		    {
			printf("3) Got read() of %d.  Server is dead\n", temp);
			serverDead = 1;
			return (0);
		    }
		} else {
		    count = BUFSIZ;
		}
	    } else {
		count -= BUFSIZ;
	    }
	    bufptr -= BUFSIZ;
	}
    }
    return (1);
}

#ifdef DEBUG
#define SANITY_TORPNUM(idx) \
	if ( (unsigned)(idx) >= ntorps*nplayers) { \
	    fprintf(stderr, "torp index %d out of bounds\n", (idx)); \
	    return; \
	}

#define SANITY_PNUM(idx) \
	if ( (unsigned)(idx) >= nplayers) { \
	    fprintf(stderr, "player number %d out of bounds\n", (idx)); \
	    return; \
	}

#define SANITY_PHASNUM(idx) \
	if ( (unsigned)(idx) >= nplayers*nphasers) { \
	    fprintf(stderr, "phaser number %d out of bounds\n", (idx)); \
	    return; \
	}

#define SANITY_PLASNUM(idx) \
	if ( (unsigned)(idx) >= nplayers*nplasmas) { \
	    fprintf(stderr, "plasma number %d out of bounds\n", (idx)); \
	    return; \
	}

#define SANITY_THINGYNUM(idx) \
	if ( (unsigned)(idx) >= npthingies*nplayers + ngthingies) { \
	    fprintf(stderr, "thingy index %x out of bounds\n", (idx)); \
	    return; \
	}

#define SANITY_PLANNUM(idx) \
	if ( (unsigned)(idx) >= MAXPLANETS) { \
	    fprintf(stderr, "planet index %d out of bounds\n", (idx)); \
	    return; \
	}

#define SANITY_SHIPNUM(idx) \
	if ( (unsigned)(idx) >= nshiptypes) { \
	    fprintf(stderr, "ship type %d out of bounds\n", (idx)); \
	    return; \
	}
#else
#define SANITY_TORPNUM(idx)
#define SANITY_PNUM(idx)
#define SANITY_PHASNUM(idx)
#define SANITY_PLASNUM(idx)
#define SANITY_THINGYNUM(idx)
#define SANITY_PLANNUM(idx)
#define SANITY_SHIPNUM(idx)
#endif


static void
handleTorp(struct torp_spacket *packet)
{
    struct torp *thetorp;

    SANITY_TORPNUM(ntohs(packet->tnum));

    thetorp = &torps[ntohs(packet->tnum)];
    thetorp->t_x = ntohl(packet->x);
    thetorp->t_y = ntohl(packet->y);
    thetorp->t_dir = packet->dir;

    if (rotate) {
	rotate_gcenter(&thetorp->t_x, &thetorp->t_y);
	rotate_dir(&thetorp->t_dir, rotate_deg);
    }
}


static void
handleTorpInfo(struct torp_info_spacket *packet)
{
    struct torp *thetorp;

    SANITY_TORPNUM(ntohs(packet->tnum));

    thetorp = &torps[ntohs(packet->tnum)];

    if (packet->status == TEXPLODE && thetorp->t_status == TFREE) {
	/* FAT: redundant explosion; don't update p_ntorp */
	/*
	   printf("texplode ignored\n");
	*/
	return;
    }

    if (thetorp->t_status == TFREE && packet->status) {
	players[thetorp->t_owner].p_ntorp++;
	thetorp->frame = 0;
    }
    if (thetorp->t_status && packet->status == TFREE) {
	players[thetorp->t_owner].p_ntorp--;
    }
    if (packet->status != thetorp->t_status) {
	/* FAT: prevent explosion reset */
	thetorp->t_status = packet->status;
	if (thetorp->t_status == TEXPLODE) {
	    thetorp->t_fuse = BIGINT;
	} else {
	    thetorp->t_war = packet->war;
	}
    } else {
        thetorp->t_war = packet->war;
    }
    thetorp->t_team = idx_to_mask(players[thetorp->t_owner].p_teami);
}

static void
handleStatus(struct status_spacket *packet)
{
    status->tourn = packet->tourn;
    status->armsbomb = ntohl(packet->armsbomb);
    status->planets = ntohl(packet->planets);
    status->kills = ntohl(packet->kills);
    status->losses = ntohl(packet->losses);
    status->time = ntohl(packet->time);
    status->timeprod = ntohl(packet->timeprod);
}

static void
handleSelf(struct you_spacket *packet)
{
    SANITY_PNUM(packet->pnum);
    me = (ghoststart ? &players[ghost_pno] : &players[packet->pnum]);
    myship = (me->p_ship);
    mystats = &(me->p_stats);
    me->p_hostile = packet->hostile;
    me->p_swar = packet->swar;
    me->p_armies = packet->armies;
    me->p_flags = ntohl(packet->flags);
    me->p_damage = ntohl(packet->damage);
    me->p_shield = ntohl(packet->shield);
    me->p_fuel = ntohl(packet->fuel);
    me->p_etemp = ntohs(packet->etemp);
    me->p_wtemp = ntohs(packet->wtemp);
    me->p_whydead = ntohs(packet->whydead);
    me->p_whodead = ntohs(packet->whodead);
    status2->clock = (unsigned long) packet->pad2;
    status2->clock += ((unsigned long) packet->pad3) << 8;
    if (packet->tractor & 0x40)
	me->p_tractor = (short) packet->tractor & (~0x40);	/* ATM - visible trac
								   tors */

}

static void
handlePlayer(struct player_spacket *packet)
{
    register struct player *pl;
    unsigned char newdir;

    SANITY_PNUM(packet->pnum);


    pl = &players[packet->pnum];
    newdir = packet->dir;
    if (rotate)
	rotate_dir(&newdir, rotate_deg);
    pl->p_dir = newdir;
    pl->p_speed = packet->speed;
    pl->p_x = ntohl(packet->x);
    pl->p_y = ntohl(packet->y);
    if (pl == me) {
	extern int my_x, my_y;	/* from shortcomm.c */
	my_x = me->p_x;
	my_y = me->p_y;
    }
    redrawPlayer[packet->pnum] = 1;

    if (me == pl) {
	extern int my_x, my_y;	/* short packets need unrotated co-ords! */
	my_x = pl->p_x;
	my_y = pl->p_y;
    }
    if (rotate) {
	rotate_gcenter(&pl->p_x, &pl->p_y);
    }
}


static void
handleWarning(struct warning_spacket *packet)
{
    warning((char *) packet->mesg);
}

static void
handleThingy(struct thingy_spacket *packet)
{
    struct thingy *thetorp;

    SANITY_THINGYNUM(ntohs(packet->tnum));

    thetorp = &thingies[ntohs(packet->tnum)];
    thetorp->t_x = ntohl(packet->x);
    thetorp->t_y = ntohl(packet->y);
    /* printf("drone at %d, %d\n", thetorp->t_x, thetorp->t_y); */
    thetorp->t_dir = packet->dir;


    if (rotate) {
	rotate_gcenter(&thetorp->t_x, &thetorp->t_y);
	rotate_dir(&thetorp->t_dir, rotate_deg);
    }

    if (thetorp->t_shape == SHP_WARP_BEACON)
	redrawall = 1;		/* shoot, route has changed */

}

static void
handleThingyInfo(struct thingy_info_spacket *packet)
{
    struct thingy *thetorp;

    SANITY_THINGYNUM(ntohs(packet->tnum));

    thetorp = &thingies[ntohs(packet->tnum)];

    thetorp->t_owner = ntohs(packet->owner);

    if (thetorp->t_shape == SHP_WARP_BEACON)
	redrawall = 1;		/* redraw the lines, I guess */

    if (ntohs(packet->shape) == SHP_BOOM && thetorp->t_shape == SHP_BLANK) {
	/* FAT: redundant explosion; don't update p_ntorp */
	/*
	   printf("texplode ignored\n");
	*/
	return;
    }

    if (thetorp->t_shape == SHP_BLANK && ntohs(packet->shape) != SHP_BLANK) {
	players[thetorp->t_owner].p_ndrone++;	/* TSH */
    }
    if (thetorp->t_shape != SHP_BLANK && ntohs(packet->shape) == SHP_BLANK) {
	players[thetorp->t_owner].p_ndrone--;	/* TSH */
    }
    thetorp->t_war = packet->war;

    if (ntohs(packet->shape) != thetorp->t_shape) {
	/* FAT: prevent explosion reset */
	int shape = ntohs(packet->shape);

        if(shape == SHP_BOOM || shape == SHP_PBOOM) {
	  if(thetorp->t_shape == SHP_FIGHTER)
	    shape = SHP_FBOOM;
	  if(thetorp->t_shape == SHP_MISSILE)
	    shape = SHP_DBOOM;
	  thetorp->t_fuse = BIGINT;
	}
	thetorp->t_shape = shape;
    }
}

void
sendShortPacket(int type, int state)
{
    struct speed_cpacket speedReq;

    speedReq.type = type;
    speedReq.speed = state;
#ifdef UNIX_SOUND
    if (type == CP_SHIELD) play_sound (SND_SHIELD); /* Shields */
#endif
    sendServerPacket((struct player_spacket *) & speedReq);
    /* printf("Sending packet #%d\n",type); */

    /* if we're sending in UDP mode, be prepared to force it */
    if (commMode == COMM_UDP && udpClientSend >= 2) {
	switch (type) {
	case CP_SPEED:
	    fSpeed = state | 0x100;
	    break;
	case CP_DIRECTION:
	    fDirection = state | 0x100;
	    break;
	case CP_SHIELD:
	    fShield = state | 0xa00;
	    break;
	case CP_ORBIT:
	    fOrbit = state | 0xa00;
	    break;
	case CP_REPAIR:
	    fRepair = state | 0xa00;
	    break;
	case CP_CLOAK:
	    fCloak = state | 0xa00;
	    break;
	case CP_BOMB:
	    fBomb = state | 0xa00;
	    break;
	case CP_DOCKPERM:
	    fDockperm = state | 0xa00;
	    break;
	case CP_PLAYLOCK:
	    fPlayLock = state | 0xa00;
	    break;
	case CP_PLANLOCK:
	    fPlanLock = state | 0xa00;
	    break;
	case CP_BEAM:
	    if (state == 1)
		fBeamup = 1 | 0x500;
	    else
		fBeamdown = 2 | 0x500;
	    break;
	}

	/* force weapons too? */
	if (udpClientSend >= 3) {
	    switch (type) {
	    case CP_PHASER:
		fPhaser = state | 0x100;
		break;
	    case CP_PLASMA:
		fPlasma = state | 0x100;
		break;
	    }
	}
    }
}

/* Pick a random type for the packet */
void
sendServerPacket(struct player_spacket *packet)
{
    int     size;

    if (serverDead)
	return;
    size = size_of_cpacket(packet);
    if (size < 1) {
	printf("Attempt to send strange packet %d!\n", packet->type);
	return;
    }
#ifdef SHOW_SEND
    printf("sending packet type %d, size %d\n", packet->type,
	   size);
#endif				/* SHOW_SEND */
    light_send();
    if (commMode == COMM_UDP) {
	/* for now, just sent everything via TCP */
    }
    if (commMode == COMM_TCP || !udpClientSend) {
	/* special case for verify packet */
	if (packet->type == CP_UDP_REQ) {
	    if (((struct udp_req_cpacket *) packet)->request == COMM_VERIFY)
		goto send_udp;
	}
	/*
	   business as usual (or player has turned off UDP transmission)
	*/
	if (gwrite(sock, (char *) packet, size) != size) {
	    printf("gwrite failed.  Server must be dead\n");
	    serverDead = 1;
	}
    } else {
	/*
	   UDP stuff
	*/
	switch (packet->type) {
	case CP_SPEED:
	case CP_DIRECTION:
	case CP_PHASER:
	case CP_PLASMA:
	case CP_TORP:
	case CP_QUIT:
	case CP_PRACTR:
	case CP_REPAIR:
	case CP_ORBIT:
	case CP_BOMB:
	case CP_BEAM:
	case CP_DET_TORPS:
	case CP_DET_MYTORP:
	case CP_TRACTOR:
	case CP_REPRESS:
	case CP_COUP:
	case CP_DOCKPERM:
	case CP_SCAN:
	case CP_PING_RESPONSE:
	case CP_CLOAK:
	case CP_SHIELD:
	case CP_PLANLOCK:
	    /*
	       these are non-critical but don't expire, send with TCP
	       [BDyess]
	    */
	    /* case CP_REFIT: */
	    /* case CP_PLAYLOCK: */
	    /* non-critical stuff, use UDP */
    send_udp:
	    packets_sent++;	/* ping stuff */

	    V_UDPDIAG(("Sent %d on UDP port\n", packet->type));
	    if (gwrite(udpSock, (char *) packet, size) != size) {
		UDPDIAG(("gwrite on UDP failed.  Closing UDP connection\n"));
		warning("UDP link severed");
		/* serverDead=1; */
		commModeReq = commMode = COMM_TCP;
		commStatus = STAT_CONNECTED;
		commSwitchTimeout = 0;
		if (udpWin) {
		    udprefresh(UDP_STATUS);
		    udprefresh(UDP_CURRENT);
		}
		if (udpSock >= 0)
		    closeUdpConn();
	    }
	    break;

	default:
	    /* critical stuff, use TCP */
	    if (gwrite(sock, (char *) packet, size) != size) {
		printf("gwrite failed.  Server must be dead\n");
		serverDead = 1;
	    }
	}
    }
}

static void
handlePlanet(struct planet_spacket *packet)
{
    struct planet *plan;
    /* FAT: prevent excessive redraw */
    int     redrawflag = 0;
    int     hockey_update = 0;

    SANITY_PLANNUM(packet->pnum);
    nplanets = 60;

    plan = &planets[packet->pnum];
    if (plan->pl_owner != packet->owner) {
	redrawflag = 1;
	hockey_update = 1;
    }
    plan->pl_owner = packet->owner;

    if (plan->pl_owner < (1 << 0) || plan->pl_owner > (1 << (number_of_teams - 1)))
	plan->pl_owner = NOBODY;

    if (plan->pl_info != packet->info)
	redrawflag = 1;
    plan->pl_info = packet->info;
    /* Redraw the planet because it was updated by server */

    if (plan->pl_flags != (int) ntohs(packet->flags))
	redrawflag = 1;
    plan->pl_flags = (unsigned short) ntohs(packet->flags);

    if (plan->pl_armies != ntohl(packet->armies))
	redrawflag = 1;

    plan->pl_armies = ntohl(packet->armies);
    if (plan->pl_info == 0) {
	plan->pl_owner = NOBODY;
    }
    if (redrawflag) {
	plan->pl_flags |= PLREDRAW;
	pl_update[packet->pnum].plu_update = 1;	/* used to mean the planet
						   had moved, now set as a
						   sign we need to erase AND
						   redraw. -JR */
	pl_update[packet->pnum].plu_x = planets[packet->pnum].pl_x;
	pl_update[packet->pnum].plu_y = planets[packet->pnum].pl_y;
	if (infomapped && infotype == PLANETTYPE &&
	    ((struct planet *) infothing)->pl_no == packet->pnum)
	    infoupdate = 1;
        if(hockey_update && hockey) hockeyInit();
    }
}

static void
handlePhaser(struct phaser_spacket *packet)
{
    struct phaser *phas;

    SANITY_PHASNUM(packet->pnum);

    phas = &phasers[packet->pnum];
    phas->ph_status = packet->status;
    phas->ph_dir = packet->dir;
    phas->ph_x = ntohl(packet->x);
    phas->ph_y = ntohl(packet->y);
    phas->ph_target = ntohl(packet->target);
    phas->ph_fuse = 0;

    if (rotate) {
	rotate_gcenter(&phas->ph_x, &phas->ph_y);
	rotate_dir(&phas->ph_dir, rotate_deg);
    }
#ifdef UNIX_SOUND
    if ((me->p_no == packet->pnum) && (packet->status != PHFREE)) {
        play_sound(SND_PHASER); /* Phasers */
    }
#endif
}

void
handleMessage(struct mesg_spacket *packet)
{
    if ((int) packet->m_from >= nplayers)
	packet->m_from = 255;
    dmessage(packet->mesg, packet->m_flags, packet->m_from, packet->m_recpt);
}


static void
handleQueue(struct queue_spacket *packet)
{
    queuePos = ntohs(packet->pos);
    /* printf("Receiving queue position %d\n",queuePos); */
}

static void
handleEmpty(char *ptr)
{
    printf("Unknown packet type: %d\n", *ptr);
    return;
}

void
sendTeamReq(int team, int ship)
{
    struct outfit_cpacket outfitReq;

    outfitReq.type = CP_OUTFIT;
    outfitReq.team = team;
    outfitReq.ship = ship;
    sendServerPacket((struct player_spacket *) & outfitReq);
}

static void
handlePickok(struct pickok_spacket *packet)
{
    pickOk = packet->state;
    if (playback) {		/* added when the packet is recorded. */
	extern int lastTeamReq;
	lastTeamReq = packet->pad2;
    }
}

void
sendLoginReq(char *name, char *pass, char *loginname, int query)
{
    struct login_cpacket packet;

    strcpy(packet.name, name);
    strcpy(packet.password, pass);
    if (strlen(loginname) > 15)
	loginname[15] = 0;
    strcpy(packet.login, loginname);
    packet.type = CP_LOGIN;
    packet.query = query;
    packet.pad2 = 0x69;		/* added 1/19/93 KAO */
    packet.pad3 = 0x43;		/* added 1/19/93 KAO *//* was 0x42 3/2/93 */
    sendServerPacket((struct player_spacket *) & packet);
}

static void
handleLogin(struct login_spacket *packet)
{


    loginAccept = packet->accept;
    if ((packet->pad2 == 69) && (packet->pad3 == 42))
	paradise = 1;
    else {
	/*nshiptypes = 8;*/
	/*nplayers=20;*/
	/*nplanets=40;*/
    }
    if (packet->accept) {

	/* we no longer accept keymaps from the server */

	mystats->st_flags = ntohl(packet->flags);
	keeppeace = (me->p_stats.st_flags / ST_KEEPPEACE) & 1;
    }
}

void
sendTractorReq(int state, int pnum)
{
    struct tractor_cpacket tractorReq;

    tractorReq.type = CP_TRACTOR;
    tractorReq.state = state;
    tractorReq.pnum = pnum;
    sendServerPacket((struct player_spacket *) & tractorReq);

    if (state)
	fTractor = pnum | 0x40;
    else
	fTractor = 0;
}

void
sendRepressReq(int state, int pnum)
{
    struct repress_cpacket repressReq;

    repressReq.type = CP_REPRESS;
    repressReq.state = state;
    repressReq.pnum = pnum;
    sendServerPacket((struct player_spacket *) & repressReq);

    if (state)
	fRepress = pnum | 0x40;
    else
	fRepress = 0;
}

void
sendDetMineReq(int torp)
{
    struct det_mytorp_cpacket detReq;

    detReq.type = CP_DET_MYTORP;
    detReq.tnum = htons(torp);
    sendServerPacket((struct player_spacket *) & detReq);
}

static void
handlePlasmaInfo(struct plasma_info_spacket *packet)
{
    struct plasmatorp *thetorp;

    SANITY_PLASNUM(ntohs(packet->pnum));

    thetorp = &plasmatorps[ntohs(packet->pnum)];
    if (packet->status == PTEXPLODE && thetorp->pt_status == PTFREE) {
	/* FAT: redundant explosion; don't update p_nplasmatorp */
	return;
    }
    if (!thetorp->pt_status && packet->status) {
	players[thetorp->pt_owner].p_nplasmatorp++;
#ifdef UNIX_SOUND
    play_sound (SND_PLASMA); /* Plasma */
#endif
    }
    if (thetorp->pt_status && !packet->status) {
	players[thetorp->pt_owner].p_nplasmatorp--;
    }
    thetorp->pt_war = packet->war;
    if (thetorp->pt_status != packet->status) {
	/* FAT: prevent explosion timer from being reset */
	thetorp->pt_status = packet->status;
	if (thetorp->pt_status == PTEXPLODE) {
	    thetorp->pt_fuse = BIGINT;
	}
    }
}

static void
handlePlasma(struct plasma_spacket *packet)
{
    struct plasmatorp *thetorp;

    SANITY_PLASNUM(ntohs(packet->pnum));

    thetorp = &plasmatorps[ntohs(packet->pnum)];
    thetorp->pt_x = ntohl(packet->x);
    thetorp->pt_y = ntohl(packet->y);

    if (rotate) {
	rotate_gcenter(&thetorp->pt_x, &thetorp->pt_y);
    }
}

static void
handleFlags(struct flags_spacket *packet)
{
    SANITY_PNUM(packet->pnum);

    if (players[packet->pnum].p_flags != ntohl(packet->flags) ||
    players[packet->pnum].p_tractor != ((short) packet->tractor & (~0x40))) {
	/* FAT: prevent redundant player update */
	redrawPlayer[packet->pnum] = 1;
    } else
	return;

    players[packet->pnum].p_flags = ntohl(packet->flags);
    if (packet->tractor & 0x40)
	players[packet->pnum].p_tractor = (short) packet->tractor & (~0x40);	/* ATM - visible
										   tractors */
    else
	players[packet->pnum].p_tractor = -1;
}

static void
handleKills(struct kills_spacket *packet)
{

    SANITY_PNUM(packet->pnum);

    if (players[packet->pnum].p_kills != ntohl(packet->kills) / 100.0) {
	players[packet->pnum].p_kills = ntohl(packet->kills) / 100.0;
	/* FAT: prevent redundant player update */
	updatePlayer[packet->pnum] |= ALL_UPDATE;
	if (infomapped && infotype == PLAYERTYPE &&
	    ((struct player *) infothing)->p_no == packet->pnum)
	    infoupdate = 1;
#ifdef ARMY_SLIDER
	if (me == &players[packet->pnum]) {
	    calibrate_stats();
	    redrawStats();
	}
#endif				/* ARMY_SLIDER */
    }
}

static void
handlePStatus(struct pstatus_spacket *packet)
{
    SANITY_PNUM(packet->pnum);

    if (packet->status == PEXPLODE) {
	players[packet->pnum].p_explode = 0;
    }
    /*
       Ignore DEAD status. Instead, we treat it as PEXPLODE. This gives us
       time to animate all the frames necessary for the explosions at our own
       pace.
    */
    if (packet->status == PDEAD) {
	packet->status = PEXPLODE;
    }
    if (players[packet->pnum].p_status != packet->status) {
	players[packet->pnum].p_status = packet->status;
	redrawPlayer[packet->pnum] = 1;
	updatePlayer[packet->pnum] |= ALL_UPDATE;
	if (infomapped && infotype == PLAYERTYPE &&
	    ((struct player *) infothing)->p_no == packet->pnum)
	    infoupdate = 1;
    }
}

static void
handleMotd(struct motd_spacket *packet)
{
    newMotdLine((char *) packet->line);
}

void
sendMessage(char *mes, int group, int indiv)
{
    struct mesg_cpacket mesPacket;

    if (recv_short) {
	int     size;
	size = strlen(mes);
	size += 5;		/* 1 for '\0', 4 packetheader */
	if ((size % 4) != 0)
	    size += (4 - (size % 4));
	mesPacket.pad1 = (char) size;

	/*
	   OH SHIT!!!! sizes[CP_S_MESSAGE] = size;
	*/

	mesPacket.type = CP_S_MESSAGE;
    } else
	mesPacket.type = CP_MESSAGE;
    mesPacket.group = group;
    mesPacket.indiv = indiv;
    strcpy(mesPacket.mesg, mes);
    sendServerPacket((struct player_spacket *) & mesPacket);
}

static void
handleMask(struct mask_spacket *packet)
{
    tournMask = packet->mask;
}

void
sendOptionsPacket(void)
{
    struct options_cpacket optPacket;
    register int i;
    long    flags;

    optPacket.type = CP_OPTIONS;
    flags = (
	     ST_NOBITMAPS * (!sendmotdbitmaps) +
	     ST_KEEPPEACE * keeppeace +
	     0
	);
    optPacket.flags = htonl(flags);
    /* copy the keymap and make sure no ctrl chars are sent [BDyess] */
    for (i = 32; i < 128; i++) {
	optPacket.keymap[i - 32] =
	    (myship->s_keymap[i] & 128) ? i : myship->s_keymap[i];
    }
    sendServerPacket((struct player_spacket *) & optPacket);
}

static void
pickSocket(int old)
{
    int     newsocket;
    struct socket_cpacket sockPack;
    newsocket = (getpid() & 32767);
    if (ghoststart)
	nextSocket = old;
    while (newsocket < 2048 || newsocket == old) {
	newsocket = (newsocket + 10687) & 32767;
    }
    sockPack.type = CP_SOCKET;
    sockPack.socket = htonl(newsocket);
    sockPack.version = (char) SOCKVERSION;
    sockPack.udp_version = (char) UDPVERSION;
    sendServerPacket((struct player_spacket *) & sockPack);
    /* Did we get new socket # sent? */
    if (serverDead)
	return;
    nextSocket = newsocket;
}

static void
handleBadVersion(struct badversion_spacket *packet)
{
    switch (packet->why) {
    case 0:
	printf("Sorry, this is an invalid client version.\n");
	printf("You need a new version of the client code.\n");
	break;
    default:
	printf("Sorry, but you cannot play xtrek now.\n");
	printf("Try again later.\n");
	break;
    }
    EXIT(1);
}

int
gwrite(int fd, char *buffer, int bytes)
{
    long    orig = bytes;
    register long n;
    if (playback)		/* pretend all is well */
	return (bytes);
    while (bytes) {
	n = sock_write(fd, buffer, bytes);
	if (n < 0) {
	    if (fd == udpSock) {
		fprintf(stderr, "Tried to write %d, 0x%x, %d\n",
			fd, (unsigned int) buffer, bytes);
		perror("write");
		printUdpInfo();
	    }
	    return (-1);
	}
	bytes -= n;
	buffer += n;
    }
    return (orig);
}

int
sock_read(int s, char *data, int size)
{
    if (playback)
	return readRecorded(s, data, size);

    return read(s, data, size);
}

static void
handleHostile(struct hostile_spacket *packet)
{
    register struct player *pl;

    SANITY_PNUM(packet->pnum);

    pl = &players[packet->pnum];
    if (pl->p_swar != packet->war ||
	pl->p_hostile != packet->hostile) {
	/* FAT: prevent redundant player update & redraw */
	pl->p_swar = packet->war;
	pl->p_hostile = packet->hostile;
	/* updatePlayer[packet->pnum]=1; why? */
	redrawPlayer[packet->pnum] = 1;
    }
}

static void
handlePlyrLogin(struct plyr_login_spacket *packet)
{
    register struct player *pl;

    SANITY_PNUM(packet->pnum);

    updatePlayer[packet->pnum] |= ALL_UPDATE;
    pl = &players[packet->pnum];

    strcpy(pl->p_name, packet->name);
    strcpy(pl->p_monitor, packet->monitor);
    strcpy(pl->p_login, packet->login);
    pl->p_stats.st_rank = packet->rank;
    if (packet->pnum == me->p_no) {
	/* This is me.  Set some stats */
	if (lastRank == -1) {
	    if (loggedIn) {
		lastRank = packet->rank;
	    }
	} else {
	    if (lastRank != packet->rank) {
		lastRank = packet->rank;
		promoted = 1;
	    }
	}
    }
}

static void
handleStats(struct stats_spacket *packet)
{
    register struct player *pl;

    SANITY_PNUM(packet->pnum);

    updatePlayer[packet->pnum] |= LARGE_UPDATE;
    if (infomapped && infotype == PLAYERTYPE &&
	((struct player *) infothing)->p_no == packet->pnum)
	infoupdate = 1;
    pl = &players[packet->pnum];
    pl->p_stats.st_tkills = ntohl(packet->tkills);
    pl->p_stats.st_tlosses = ntohl(packet->tlosses);
    pl->p_stats.st_kills = ntohl(packet->kills);
    pl->p_stats.st_losses = ntohl(packet->losses);
    pl->p_stats.st_tticks = ntohl(packet->tticks);
    pl->p_stats.st_tplanets = ntohl(packet->tplanets);
    pl->p_stats.st_tarmsbomb = ntohl(packet->tarmies);
    pl->p_stats.st_sbkills = ntohl(packet->sbkills);
    pl->p_stats.st_sblosses = ntohl(packet->sblosses);
    pl->p_stats.st_armsbomb = ntohl(packet->armies);
    pl->p_stats.st_planets = ntohl(packet->planets);
    pl->p_stats.st_maxkills = ntohl(packet->maxkills) / 100.0;
    pl->p_stats.st_sbmaxkills = ntohl(packet->sbmaxkills) / 100.0;
}

static void
handlePlyrInfo(struct plyr_info_spacket *packet)
{
    register struct player *pl;
    static int lastship = -1;

    SANITY_PNUM(packet->pnum);

    updatePlayer[packet->pnum] |= ALL_UPDATE;
    if (infomapped && infotype == PLAYERTYPE &&
	((struct player *) infothing)->p_no == packet->pnum)
	infoupdate = 1;
    pl = &players[packet->pnum];
    pl->p_ship = getship(packet->shiptype);
    if (packet->pnum == me->p_no && currentship != packet->shiptype) {
	currentship = packet->shiptype;
	/* sendOptionsPacket(); */
    }
    pl->p_teami = mask_to_idx(packet->team);
    if(pl->p_teami > number_of_teams) pl->p_teami = number_of_teams;
    pl->p_mapchars[0] = teaminfo[pl->p_teami].letter;
    /* printf("team: %d, letter: %c\n",pl->p_teami,pl->p_mapchars[0]); */
    pl->p_mapchars[1] = shipnos[pl->p_no];
    if (me == pl && lastship != currentship) {
	lastship = currentship;
	redrawTstats();
	calibrate_stats();
	redrawStats();		/* tsh */
    }
    redrawPlayer[packet->pnum] = 1;
}

void
sendUpdatePacket(long speed)
{
    struct updates_cpacket packet;

    packet.type = CP_UPDATES;
    timerDelay = speed;
    packet.usecs = htonl(speed);
    sendServerPacket((struct player_spacket *) & packet);
}

static void
handlePlanetLoc(struct planet_loc_spacket *packet)
{
    struct planet *pl;

    SANITY_PLANNUM(packet->pnum);

    pl = &planets[packet->pnum];
    pl_update[packet->pnum].plu_x = pl->pl_x;
    pl_update[packet->pnum].plu_y = pl->pl_y;

    if (pl_update[packet->pnum].plu_update != -1) {
	pl_update[packet->pnum].plu_update = 1;
	/*
	   printf("update: %s, old (%d,%d) new (%d,%d)\n", pl->pl_name,
	   pl->pl_x, pl->pl_y, ntohl(packet->x),ntohl(packet->y));
	*/
    } else {
	pl_update[packet->pnum].plu_update = 0;
	pl_update[packet->pnum].plu_x = ntohl(packet->x);
	pl_update[packet->pnum].plu_y = ntohl(packet->y);
    }
    pl->pl_x = ntohl(packet->x);
    pl->pl_y = ntohl(packet->y);
    strcpy(pl->pl_name, packet->name);
    pl->pl_namelen = strlen(packet->name);
    pl->pl_flags |= PLREDRAW;
    if (infomapped && infotype == PLANETTYPE &&
	((struct planet *) infothing)->pl_no == packet->pnum)
	infoupdate = 1;
    reinitPlanets = 1;
    if (pl->pl_x > blk_gwidth) {
	blk_gwidth = 200000;
	blk_windgwidth = ((float) MAPSIDE) / blk_gwidth;
    }
    if (rotate) {
	rotate_gcenter(&pl->pl_x, &pl->pl_y);
    }
}

static void
handleReserved(struct reserved_spacket *packet)
{
#ifdef AUTHORIZE
    struct reserved_cpacket response;

    response.type = CP_RESERVED;
    if (RSA_Client) {		/* can use -o option for old blessing */
	warning(RSA_VERSION);
	strncpy(response.resp, RSA_VERSION, RESERVED_SIZE);
	memcpy(response.data, packet->data, RESERVED_SIZE);
    } else {
	memcpy(response.data, packet->data, 16);
	memcpy(response.resp, packet->data, 16);
    }
    sendServerPacket((struct player_spacket *) & response);
#endif
}

static void
handleRSAKey(struct rsa_key_spacket *packet)
{
#ifdef AUTHORIZE
    struct rsa_key_cpacket response;
    struct sockaddr_in saddr;
    unsigned char *data;
    int     len;

    /* query the socket to determine the remote host (ATM) */
    len = sizeof(saddr);
    if (getpeername(sock, (struct sockaddr *) & saddr, &len) < 0) {
	perror("getpeername(sock)");
	exit(1);
    }

    data = packet->data;
    memcpy(data, &saddr.sin_addr.s_addr, sizeof(saddr.sin_addr.s_addr));
    data += sizeof(saddr.sin_addr.s_addr);
    memcpy(data, &saddr.sin_port, sizeof(saddr.sin_port));

	rsa_black_box(response.resp, packet->data,
		      response.public, response.global);
    response.type = CP_RSA_KEY;

    sendServerPacket((struct player_spacket *) & response);
#endif				/* AUTHORIZE */
}

static void
handleScan(struct scan_spacket *packet)
{
    struct player *pp;

    SANITY_PNUM(packet->pnum);

    if (packet->success) {
	pp = &players[packet->pnum];
	pp->p_fuel = ntohl(packet->p_fuel);
	pp->p_armies = ntohl(packet->p_armies);
	pp->p_shield = ntohl(packet->p_shield);
	pp->p_damage = ntohl(packet->p_damage);
	pp->p_etemp = ntohl(packet->p_etemp);
	pp->p_wtemp = ntohl(packet->p_wtemp);
    }
}

/*
 * UDP stuff
 */
void
sendUdpReq(int req)
{
    struct udp_req_cpacket packet;

    packet.type = CP_UDP_REQ;
    packet.request = req;

    if (req >= COMM_MODE) {
	packet.request = COMM_MODE;
	packet.connmode = req - COMM_MODE;
	sendServerPacket((struct player_spacket *) & packet);
	return;
    }
    if (req == COMM_UPDATE) {
	if (recv_short) {	/* not necessary */
/* Let the client do the work, and not the network :-) */
	    register int i;
	    for (i = 0; i < nplayers * ntorps; i++)
		torps[i].t_status = TFREE;

	    for (i = 0; i < nplayers * nplasmas; i++)
		plasmatorps[i].pt_status = PTFREE;

	    for (i = 0; i < nplayers; i++) {
		players[i].p_ntorp = 0;
		players[i].p_nplasmatorp = 0;
		phasers[i].ph_status = PHFREE;
	    }
	}
	sendServerPacket((struct player_spacket *) & packet);
	warning("Sent request for full update");
	return;
    }
    if (req == commModeReq) {
	warning("Request is in progress, do not disturb");
	return;
    }
    if (req == COMM_UDP) {
	/* open UDP port */
	if (openUdpConn() >= 0) {
	    UDPDIAG(("Bound to local port %d on fd %d\n", udpLocalPort, udpSock));
	} else {
	    UDPDIAG(("Bind to local port %d failed\n", udpLocalPort));
	    commModeReq = COMM_TCP;
	    commStatus = STAT_CONNECTED;
	    commSwitchTimeout = 0;
	    if (udpWin)
		udprefresh(UDP_STATUS);
	    warning("Unable to establish UDP connection");

	    return;
	}
    }
    /* send the request */
    packet.type = CP_UDP_REQ;
    packet.request = req;
    packet.port = htonl(udpLocalPort);
#ifdef UDP_PORTSWAP
    packet.connmode = CONNMODE_PORT;	/* have him send his port */
#else
    packet.connmode = CONNMODE_PACKET;	/* we get addr from packet */
#endif
    sendServerPacket((struct player_spacket *) & packet);

    /* update internal state stuff */
    commModeReq = req;
    if (req == COMM_TCP)
	commStatus = STAT_SWITCH_TCP;
    else
	commStatus = STAT_SWITCH_UDP;
    commSwitchTimeout = 25;	/* wait 25 updates (about five seconds) */

    UDPDIAG(("Sent request for %s mode\n", (req == COMM_TCP) ?
	     "TCP" : "UDP"));

#ifndef UDP_PORTSWAP
    if ((req == COMM_UDP) && recvUdpConn() < 0) {
	UDPDIAG(("Sending TCP reset message\n"));
	packet.request = COMM_TCP;
	packet.port = 0;
	commModeReq = COMM_TCP;
	sendServerPacket((struct player_spacket *) & packet);
	/* we will likely get a SWITCH_UDP_OK later; better ignore it */
	commModeReq = COMM_TCP;
	commStatus = STAT_CONNECTED;
	commSwitchTimeout = 0;
	closeUdpConn();
    }
#endif

    if (udpWin)
	udprefresh(UDP_STATUS);
}

static void
handleUdpReply(struct udp_reply_spacket *packet)
{
    struct udp_req_cpacket response;

    UDPDIAG(("Received UDP reply %d\n", packet->reply));
    commSwitchTimeout = 0;

    response.type = CP_UDP_REQ;

    switch (packet->reply) {
    case SWITCH_TCP_OK:
	if (commMode == COMM_TCP) {
	    UDPDIAG(("Got SWITCH_TCP_OK while in TCP mode; ignoring\n"));
	} else {
	    commMode = COMM_TCP;
	    commStatus = STAT_CONNECTED;
	    warning("Switched to TCP-only connection");
	    closeUdpConn();
	    UDPDIAG(("UDP port closed\n"));
	    if (udpWin) {
		udprefresh(UDP_STATUS);
		udprefresh(UDP_CURRENT);
	    }
	}
	break;
    case SWITCH_UDP_OK:
	if (commMode == COMM_UDP) {
	    UDPDIAG(("Got SWITCH_UDP_OK while in UDP mode; ignoring\n"));
	} else {
	    /* the server is forcing UDP down our throat? */
	    if (commModeReq != COMM_UDP) {
		UDPDIAG(("Got unsolicited SWITCH_UDP_OK; ignoring\n"));
	    } else {
#ifdef UDP_PORTSWAP
                udpServerPort = ntohl(packet->port);

                if (connUdpConn() < 0) {
                    UDPDIAG(("Unable to connect, resetting\n"));
                    warning("Connection attempt failed");
                    commModeReq = COMM_TCP;
                    commStatus = STAT_CONNECTED;
                    if (udpSock >= 0)
                        closeUdpConn();
                    if (udpWin) {
                        udprefresh(UDP_STATUS);
                        udprefresh(UDP_CURRENT);
                    }
                    response.request = COMM_TCP;
                    response.port = 0;
                    goto send;
                }

#endif

		UDPDIAG(("Connected to server's UDP port\n"));
		commStatus = STAT_VERIFY_UDP;
		if (udpWin)
		    udprefresh(UDP_STATUS);
		response.request = COMM_VERIFY;	/* send verify request on UDP */
		response.port = 0;
		commSwitchTimeout = 25;	/* wait 25 updates */
#ifdef UDP_PORTSWAP
	send:
#endif
		sendServerPacket((struct player_spacket *) & response);
	    }
	}
	break;
    case SWITCH_DENIED:
	if (ntohs(packet->port)) {
	    UDPDIAG(("Switch to UDP failed (different version)\n"));
	    warning("UDP protocol request failed (bad version)");
	} else {
	    UDPDIAG(("Switch to UDP denied\n"));
	    warning("UDP protocol request denied");
	}
	commModeReq = commMode;
	commStatus = STAT_CONNECTED;
	commSwitchTimeout = 0;
	if (udpWin)
	    udprefresh(UDP_STATUS);
	if (udpSock >= 0)
	    closeUdpConn();
	break;
    case SWITCH_VERIFY:
	UDPDIAG(("Received UDP verification\n"));
	break;
    default:
	fprintf(stderr, "netrek: Got funny reply (%d) in UDP_REPLY packet\n",
		packet->reply);
	break;
    }
}

#define MAX_PORT_RETRY  10
static int
openUdpConn(void)
{
    struct sockaddr_in addr;
    struct hostent *hp;
    int     attempts;

    if (playback)
	return 0;
    if (udpSock >= 0) {
	fprintf(stderr, "netrek: tried to open udpSock twice\n");
	return (0);		/* pretend we succeeded (this could be bad) */
    }
    if ((udpSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("netrek: unable to create DGRAM socket");
	return (-1);
    }

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    errno = 0;
    udpLocalPort = (getpid() & 32767) + (random() % 256);
    for (attempts = 0; attempts < MAX_PORT_RETRY; attempts++) {
	while (udpLocalPort < 2048) {
	    udpLocalPort = (udpLocalPort + 10687) & 32767;
	}
	addr.sin_port = htons(udpLocalPort);
	if (bind(udpSock, (struct sockaddr *) & addr, sizeof(addr)) >= 0)
	    break;
    }
    if (attempts == MAX_PORT_RETRY) {
	perror("netrek: bind");
	UDPDIAG(("Unable to find a local port to bind to\n"));
	close(udpSock);
	udpSock = -1;
	return (-1);
    }
    UDPDIAG(("Local port is %d\n", udpLocalPort));

    /* determine the address of the server */
    if (!serveraddr) {
	if ((addr.sin_addr.s_addr = inet_addr(serverName)) == -1) {
	    if ((hp = gethostbyname(serverName)) == NULL) {
		printf("Who is %s?\n", serverName);

		EXIT(0);
	    } else {
		addr.sin_addr.s_addr = *(long *) hp->h_addr;
	    }
	}
	serveraddr = addr.sin_addr.s_addr;
	UDPDIAG(("Found serveraddr == 0x%x\n", (unsigned int) serveraddr));
    }
    return (0);
}

#ifdef UDP_PORTSWAP
static int connUdpConn(void)
{
    struct sockaddr_in addr;
    int     len;

    if (playback)
        return 0;

    addr.sin_addr.s_addr = serveraddr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(udpServerPort);

    UDPDIAG(("Connecting to host 0x%x on port %d\n", serveraddr, udpServerPort))
;
    if (connect(udpSock, &addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error %d: ");
        perror("netrek: unable to connect UDP socket");
        printUdpInfo();         /* debug */
        return (-1);
    }

    return (0);
}
#endif


static int
recvUdpConn(void)
{
    fd_set  readfds;
    struct timeval to;
    struct sockaddr_in from;
    int     fromlen, res;

    if (playback)
	return 0;
    memset(&from, 0, sizeof(from));	/* don't get garbage if really broken */

    /* we patiently wait until the server sends a packet to us */
    /* (note that we silently eat the first one) */
    UDPDIAG(("Issuing recvfrom() call\n"));
    printUdpInfo();
    fromlen = sizeof(from);
    FD_ZERO(&readfds);
    FD_SET(udpSock, &readfds);
    to.tv_sec = 6;		/* wait 3 seconds, then abort */
    to.tv_usec = 0;
    if ((res = select(32, &readfds, 0, 0, &to)) <= 0) {
	if (!res) {
	    UDPDIAG(("timed out waiting for response"));
	    warning("UDP connection request timed out");
	    return (-1);
	} else {
	    perror("select() before recvfrom()");
	    return (-1);
	}
    }
    if (recvfrom(udpSock, buf, BUFSIZ, 0, (struct sockaddr *) & from, &fromlen) < 0) {
	perror("recvfrom");
	UDPDIAG(("recvfrom failed, aborting UDP attempt"));
	return (-1);
    }
    if (from.sin_addr.s_addr != serveraddr) {
	/* safe? */
	serveraddr = from.sin_addr.s_addr;
	UDPDIAG(("Warning: from 0x%x, but server is 0x%x\n",
	   (unsigned int) from.sin_addr.s_addr, (unsigned int) serveraddr));
    }
    if (from.sin_family != AF_INET) {
	UDPDIAG(("Warning: not AF_INET (%d)\n", from.sin_family));
    }
    udpServerPort = ntohs(from.sin_port);
    UDPDIAG(("recvfrom() succeeded; will use server port %d\n", udpServerPort));

    if (connect(udpSock, (struct sockaddr *) & from, sizeof(from)) < 0) {
	perror("netrek: unable to connect UDP socket after recvfrom()");
	close(udpSock);
	udpSock = -1;
	return (-1);
    }
    return (0);
}

int
closeUdpConn(void)
{
    V_UDPDIAG(("Closing UDP socket\n"));
    if (playback)
	return 0;

    if (udpSock < 0) {
	fprintf(stderr, "netrek: tried to close a closed UDP socket\n");
	return (-1);
    }
    shutdown(udpSock, 2);
    close(udpSock);
    udpSock = -1;
    return 0;
}

static void
printUdpInfo(void)
{
    struct sockaddr_in addr;
    int     len;

    len = sizeof(addr);
    if (getsockname(udpSock, (struct sockaddr *) & addr, &len) < 0) {
/*      perror("printUdpInfo: getsockname");*/
	return;
    }
    UDPDIAG(("LOCAL: addr=0x%x, family=%d, port=%d\n",
	     (unsigned int) addr.sin_addr.s_addr,
	     addr.sin_family, ntohs(addr.sin_port)));

    if (getpeername(udpSock, (struct sockaddr *) & addr, &len) < 0) {
/*      perror("printUdpInfo: getpeername");*/
	return;
    }
    UDPDIAG(("PEER : addr=0x%x, family=%d, port=%d\n",
	     (unsigned int) addr.sin_addr.s_addr,
	     addr.sin_family, ntohs(addr.sin_port)));
}

static void
handleSequence(struct sequence_spacket *packet)
{
    static int recent_count = 0, recent_dropped = 0;
    long    newseq;

    drop_flag = 0;
    if (chan != udpSock)
	return;			/* don't pay attention to TCP sequence #s */
    udpTotal++;
    recent_count++;

    /* update percent display every 256 updates (~50 seconds usually) */
    if (!(udpTotal & 0xff))
	if (udpWin)
	    udprefresh(UDP_DROPPED);

    newseq = (long) ntohs(packet->sequence);
/*    printf("read %d - ", newseq);*/

    if (((unsigned short) sequence) > 65000 &&
	((unsigned short) newseq) < 1000) {
	/* we rolled, set newseq = 65536+sequence and accept it */
	sequence = ((sequence + 65536) & 0xffff0000) | newseq;
    } else {
	/* adjust newseq and do compare */
	newseq |= (sequence & 0xffff0000);

	if (!udpSequenceChk) {	/* put this here so that turning seq check */
	    sequence = newseq;	/* on and off doesn't make us think we lost */
	    return;		/* a whole bunch of packets. */
	}
	if (newseq > sequence) {
	    /* accept */
	    if (newseq != sequence + 1) {
		udpDropped += (newseq - sequence) - 1;
		udpTotal += (newseq - sequence) - 1;	/* want TOTAL packets */
		recent_dropped += (newseq - sequence) - 1;
		recent_count += (newseq - sequence) - 1;
		if (udpWin)
		    udprefresh(UDP_DROPPED);
		UDPDIAG(("sequence=%ld, newseq=%ld, we lost some\n",
			 sequence, newseq));
	    }
	    sequence = newseq;
	} else {
	    /* reject */
	    if (packet->type == SP_SC_SEQUENCE) {
		V_UDPDIAG(("(ignoring repeat %ld)\n", newseq));
	    } else {
		UDPDIAG(("sequence=%ld, newseq=%ld, ignoring transmission\n",
			 sequence, newseq));
	    }
	    /*
	       the remaining packets will be dropped and we shouldn't count
	       the SP_SEQUENCE packet either
	    */
	    packets_received--;
	    drop_flag = 1;
	}
    }
/*    printf("newseq %d, sequence %d\n", newseq, sequence);*/
    if (recent_count > UDP_RECENT_INTR) {
	/* once a minute (at 5 upd/sec), report on how many were dropped */
	/* during the last UDP_RECENT_INTR updates                       */
	udpRecentDropped = recent_dropped;
	recent_count = recent_dropped = 0;
	if (udpWin)
	    udprefresh(UDP_DROPPED);
    }
}

/*
static void
dumpShip(struct ship *shipp)
{
  printf("ship stats:\n");
  printf("phaser range = %d\n", shipp->s_phaserrange);
  printf("max speed = %d\n", shipp->s_maxspeed);
  printf("max shield = %d\n", shipp->s_maxshield);
  printf("max damage = %d\n", shipp->s_maxdamage);
  printf("max egntemp = %d\n", shipp->s_maxegntemp);
  printf("max wpntemp = %d\n", shipp->s_maxwpntemp);
  printf("max armies = %d\n", shipp->s_maxarmies);
  printf("type = %d\n", shipp->s_type);
  printf("torp speed = %d\n", shipp->s_torpspeed);
  printf("letter = %c\n", shipp->s_letter);
  printf("desig = %2.2s\n", shipp->s_desig);
  printf("bitmap = %d\n", shipp->s_bitmap);
  if(F_armies_shipcap == 1)
  {
    printf("s_armies = 0x%x\n", shipp->s_armies);
    if(shipp->s_armies & 0x80)
      printf("SFNARMYNEEDKILL, %f armies per kill\n",
             (float)(shipp->s_armies & 0x7f) / 10.0);
  }
  printf("\n");
}
*/

static void
handleShipCap(struct ship_cap_spacket *packet)		/* SP_SHIP_CAP */
{
    struct shiplist *temp;

    /*
       What are we supposed to do?
    */

    SANITY_SHIPNUM(ntohs(packet->s_type));

    if (packet->operation) {	/* remove ship from list */
	temp = shiptypes;
	if (temp->ship->s_type == (int) ntohs(packet->s_type)) {
	    shiptypes = temp->next;
	    shiptypes->prev = NULL;
	}
	while (temp->next != NULL) {
	    if (temp->next->ship->s_type == (int) ntohs(packet->s_type)) {
		temp = temp->next;
		temp->prev->next = temp->next;
		if (temp->next)
		    temp->next->prev = temp->prev;
		free(temp->ship);
		free(temp);
		return;
	    } else {
		temp = temp->next;
	    }
	}
    }
    /*
       Since we're adding the ship, we need to find out if we already have
       that ship, and if so, replace it.
    */

    temp = shiptypes;
    while (temp != NULL) {
	if (temp->ship->s_type == (int) ntohs(packet->s_type)) {
	    temp->ship->s_type = ntohs(packet->s_type);
	    temp->ship->s_torpspeed = ntohs(packet->s_torpspeed);
	    temp->ship->s_phaserrange = ntohs(packet->s_phaserrange);
	    if (temp->ship->s_phaserrange < 200)	/* backward
							   compatibility */
		temp->ship->s_phaserrange *= PHASEDIST / 100;
	    temp->ship->s_maxspeed = ntohl(packet->s_maxspeed);
	    temp->ship->s_maxfuel = ntohl(packet->s_maxfuel);
	    temp->ship->s_maxshield = ntohl(packet->s_maxshield);
	    temp->ship->s_maxdamage = ntohl(packet->s_maxdamage);
	    temp->ship->s_maxwpntemp = ntohl(packet->s_maxwpntemp);
	    temp->ship->s_maxegntemp = ntohl(packet->s_maxegntemp);
	    temp->ship->s_maxarmies = ntohs(packet->s_maxarmies);
	    if(F_armies_shipcap == 1)
  	      temp->ship->s_armies = packet->s_armies;
	    temp->ship->s_letter = packet->s_letter;
	    temp->ship->s_desig[0] = packet->s_desig1;
	    temp->ship->s_desig[1] = packet->s_desig2;
	    temp->ship->s_bitmap = ntohs(packet->s_bitmap);
	    buildShipKeymap(temp->ship);
/*dumpShip(temp->ship);*/
	    return;
	}
	temp = temp->next;
    }

    /*
       Not there, so we need to make a new entry in the list for it.
    */
    temp = (struct shiplist *) malloc(sizeof(struct shiplist));
    temp->next = shiptypes;
    temp->prev = NULL;
    if (shiptypes)
	shiptypes->prev = temp;
    shiptypes = temp;
    temp->ship = (struct ship *) malloc(sizeof(struct ship));
    temp->ship->s_type = ntohs(packet->s_type);
    temp->ship->s_torpspeed = ntohs(packet->s_torpspeed);
    temp->ship->s_phaserrange = ntohs(packet->s_phaserrange);
    temp->ship->s_maxspeed = ntohl(packet->s_maxspeed);
    temp->ship->s_maxfuel = ntohl(packet->s_maxfuel);
    temp->ship->s_maxshield = ntohl(packet->s_maxshield);
    temp->ship->s_maxdamage = ntohl(packet->s_maxdamage);
    temp->ship->s_maxwpntemp = ntohl(packet->s_maxwpntemp);
    temp->ship->s_maxegntemp = ntohl(packet->s_maxegntemp);
    temp->ship->s_maxarmies = ntohs(packet->s_maxarmies);
    temp->ship->s_armies = packet->s_armies;
    temp->ship->s_letter = packet->s_letter;
    temp->ship->s_desig[0] = packet->s_desig1;
    temp->ship->s_desig[1] = packet->s_desig2;
    temp->ship->s_bitmap = ntohs(packet->s_bitmap);
    buildShipKeymap(temp->ship);
  /*dumpShip(temp->ship);*/
}

static void
handleMotdPic(struct motd_pic_spacket *packet)		/* SP_SHIP_CAP */
{
    int     x, y, page, width, height;

    x = ntohs(packet->x);
    y = ntohs(packet->y);
    width = ntohs(packet->width);
    height = ntohs(packet->height);
    page = ntohs(packet->page);

    newMotdPic(x, y, width, height, (char *) packet->bits, page);
}

static void
handleStats2(struct stats_spacket2 *packet)
{
    struct stats2 *p;		/* to hold packet's player's stats2 struct */

    SANITY_PNUM(packet->pnum);

    updatePlayer[packet->pnum] |= LARGE_UPDATE;
    if (infomapped && infotype == PLAYERTYPE &&
	((struct player *) infothing)->p_no == packet->pnum)
	infoupdate = 1;
    p = &(players[packet->pnum].p_stats2);	/* get player's stats2 struct */
    p->st_genocides = ntohl(packet->genocides);
    p->st_tmaxkills = (float) ntohl(packet->maxkills) / 100.0;
    p->st_di = (float) ntohl(packet->di) / 100.0;
    p->st_tkills = (int) ntohl(packet->kills);
    p->st_tlosses = (int) ntohl(packet->losses);
    p->st_tarmsbomb = (int) ntohl(packet->armsbomb);
    p->st_tresbomb = (int) ntohl(packet->resbomb);
    p->st_tdooshes = (int) ntohl(packet->dooshes);
    p->st_tplanets = (int) ntohl(packet->planets);
    p->st_tticks = (int) ntohl(packet->tticks);
    p->st_sbkills = (int) ntohl(packet->sbkills);
    p->st_sblosses = (int) ntohl(packet->sblosses);
    p->st_sbticks = (int) ntohl(packet->sbticks);
    p->st_sbmaxkills = (float) ntohl(packet->sbmaxkills) / 100.0;
    p->st_wbkills = (int) ntohl(packet->wbkills);
    p->st_wblosses = (int) ntohl(packet->wblosses);
    p->st_wbticks = (int) ntohl(packet->wbticks);
    p->st_wbmaxkills = (float) ntohl(packet->wbmaxkills) / 100.0;
    p->st_jsplanets = (int) ntohl(packet->jsplanets);
    p->st_jsticks = (int) ntohl(packet->jsticks);
    if (p->st_rank != (int) ntohl(packet->rank) ||
	p->st_royal != (int) ntohl(packet->royal)) {
	p->st_rank = (int) ntohl(packet->rank);
	p->st_royal = (int) ntohl(packet->royal);
	updatePlayer[packet->pnum] |= ALL_UPDATE;
    }
}

static void
handleStatus2(struct status_spacket2 *packet)
{
    updatePlayer[me->p_no] |= LARGE_UPDATE;
    if (infomapped && infotype == PLAYERTYPE &&
	((struct player *) infothing)->p_no == me->p_no)
	infoupdate = 1;
    status2->tourn = packet->tourn;
    status2->dooshes = ntohl(packet->dooshes);
    status2->armsbomb = ntohl(packet->armsbomb);
    status2->resbomb = ntohl(packet->resbomb);
    status2->planets = ntohl(packet->planets);
    status2->kills = ntohl(packet->kills);
    status2->losses = ntohl(packet->losses);
    status2->sbkills = ntohl(packet->sbkills);
    status2->sblosses = ntohl(packet->sblosses);
    status2->sbtime = ntohl(packet->sbtime);
    status2->wbkills = ntohl(packet->wbkills);
    status2->wblosses = ntohl(packet->wblosses);
    status2->wbtime = ntohl(packet->wbtime);
    status2->jsplanets = ntohl(packet->jsplanets);
    status2->jstime = ntohl(packet->jstime);
    status2->time = ntohl(packet->time);
    status2->timeprod = ntohl(packet->timeprod);
}

static void
handlePlanet2(struct planet_spacket2 *packet)
{
    static int first_planet_packet = 1;

    SANITY_PLANNUM(packet->pnum);
    if(first_planet_packet)
    {
      first_planet_packet = 0;
      nplanets = packet->pnum+1;
    }
    else
    {
      if((packet->pnum+1) > nplanets)
        nplanets = packet->pnum+1;
    }

    planets[packet->pnum].pl_owner = packet->owner;
    planets[packet->pnum].pl_info = packet->info;
    planets[packet->pnum].pl_flags = ntohl(packet->flags);
    if(PL_TYPE(planets[packet->pnum]) != PLPLANET) {
      planets[packet->pnum].pl_owner = ALLTEAM;
    }
    planets[packet->pnum].pl_timestamp = ntohl(packet->timestamp);
    planets[packet->pnum].pl_armies = ntohl(packet->armies);
    planets[packet->pnum].pl_flags |= PLREDRAW;
    pl_update[packet->pnum].plu_update = 1;
    pl_update[packet->pnum].plu_x = planets[packet->pnum].pl_x;
    pl_update[packet->pnum].plu_y = planets[packet->pnum].pl_y;
    if (infomapped && infotype == PLANETTYPE &&
	((struct planet *) infothing)->pl_no == packet->pnum)
	infoupdate = 1;
}

static void 
handleTerrainInfo2(struct terrain_info_packet2 *pkt)
{
#ifdef ZDIAG2
    fprintf( stderr, "Receiving terrain info packet\n" );
    fprintf( stderr, "Terrain dims: %d x %d\n", ntohs(pkt->xdim), ntohs(pkt->ydim) );
#endif
    received_terrain_info = TERRAIN_STARTED;
    terrain_x = ntohs(pkt->xdim);
    terrain_y = ntohs(pkt->ydim);
}; 

static void
handleTerrain2(struct terrain_packet2 *pkt)
{
    static int curseq = 0, totbytes = 0, done = 0;
    int i;
#if defined(ZDIAG) || defined(ZDIAG2)
    int status;
#endif /* ZDIAG || ZDIAG2 */
    unsigned long dlen;
#ifdef ZDIAG2
    static unsigned char sum = 0;
    static unsigned numnz = 0;
#endif
    static unsigned char *gzipTerrain = NULL, *orgTerrain = NULL;
    
#ifdef ZDIAG2
    fprintf( stderr, "Receiving Terrain packet.  This should be %d.\n", curseq+1 );
#endif

    if( (done == TERRAIN_DONE) && (received_terrain_info == TERRAIN_STARTED ) ){
      /* receiving new terrain info */
      free( gzipTerrain );
      free( orgTerrain );
      free( terrainInfo );
      gzipTerrain = orgTerrain = NULL;
      terrainInfo = NULL;
      curseq = done = totbytes = 0;
    }
      
    curseq++;
    if( (curseq != pkt->sequence) || !(received_terrain_info) ){
      /* Should fill in a list of all packets missed */
      /* or request header packet from server */
      fprintf( stderr, "Blech!  Received terrain packet before terrain_info\n" );
      return;
    }
#ifdef ZDIAG2
    fprintf( stderr, "Receiving packet %d out of %d\n", curseq, pkt->total_pkts );
#endif
    if( !gzipTerrain ){
      gzipTerrain = (unsigned char *)malloc( pkt->total_pkts << 7 );
#if defined(ZDIAG) || defined(ZDIAG2)
      fprintf( stderr, "Allocating %d bytes for gzipTerrain.\n", pkt->total_pkts << 7 );
#endif
		/* another yukko constant */
    }
    if( !orgTerrain ){
      orgTerrain = (unsigned char *)malloc( terrain_x*terrain_y );
      dlen = terrain_x * terrain_y;
#if defined(ZDIAG) || defined(ZDIAG2)
      fprintf( stderr, "Allocating %d bytes for orgTerrain.\n", dlen );
#endif
    }
    for( i = 0; i < pkt->length; i++ ){
#ifdef ZDIAG2
      if( !(i%10) ){
        fprintf( stderr, "Params: %d, %d\n", ((curseq-1)<<7)+i, i );
      }
#endif
      gzipTerrain[((curseq-1)<<7)+i] = pkt->terrain_type[i];
    }
    totbytes += pkt->length;
    if( curseq == pkt->total_pkts ){
#if defined(ZDIAG) || defined(ZDIAG2)
      status = uncompress( orgTerrain, &dlen, gzipTerrain, totbytes );
      if( status != Z_OK ){
        if( status == Z_BUF_ERROR ){
          fprintf( stderr, "Unable to uncompress -- Z_BUF_ERROR.\n" );
        }
        if( status == Z_MEM_ERROR ){
          fprintf( stderr, "Unable to uncompress -- Z_MEM_ERROR.\n" );
        }
        if( status = Z_DATA_ERROR ){
          fprintf( stderr, "Unable to uncompress -- Z_DATA_ERROR!\n" );
        }
      }
      else{
        fprintf( stderr, "Total zipped terrain received: %d bytes\n", totbytes );
      }
#else
      uncompress( orgTerrain, &dlen, gzipTerrain, totbytes );
#endif
      terrainInfo = (struct t_unit *)malloc( dlen * sizeof( struct t_unit ) );
      for( i = 0; i < dlen; i++ ){
        terrainInfo[i].types = orgTerrain[i];
#ifdef ZDIAG2
	sum |= orgTerrain[i];
        if( orgTerrain[i] != 0 ){
          numnz++;
        }
#endif
      }
      done = received_terrain_info = TERRAIN_DONE;
#ifdef ZDIAG2
      fprintf( stderr, "Sum = %d, numnz = %d\n", sum, numnz );
#endif
    }
}    

static void
handleTempPack(struct obvious_packet *packet)		/* SP_SHIP_CAP */
{
    struct obvious_packet reply;
    /* printf("New MOTD info available\n"); */
    erase_motd();
    reply.type = CP_ASK_MOTD;
    sendServerPacket((struct player_spacket *) & reply);
}

/* handlers for the extension1 packet */

int
compute_extension1_size(char *pkt)
{
    if (pkt[0] != SP_PARADISE_EXT1)
	return -1;

    switch (pkt[1]) {
    case SP_PE1_MISSING_BITMAP:
	return sizeof(struct pe1_missing_bitmap_spacket);
    case SP_PE1_NUM_MISSILES:
	return sizeof(struct pe1_num_missiles_spacket);
    default:
	return -1;
    }
}

static void
handleExtension1(struct paradiseext1_spacket *packet)
{
    switch (packet->subtype) {
    case SP_PE1_MISSING_BITMAP:
	{
	    struct pe1_missing_bitmap_spacket *pkt =
	    (struct pe1_missing_bitmap_spacket *) packet;

	    newMotdPic(ntohs(pkt->x),
		       ntohs(pkt->y),
		       ntohs(pkt->width),
		       ntohs(pkt->height),
		       0,
		       ntohs(pkt->page));
	}
	break;
    case SP_PE1_NUM_MISSILES:
	me->p_totmissiles = ntohs(((struct pe1_num_missiles_spacket *) packet)->num);
	/* printf("updated totmissiles to %d\n",me->p_totmissiles); */
	if (me->p_totmissiles < 0)
	    me->p_totmissiles = 0;	/* SB/WB have -1 */
	break;
    default:
	printf("unknown paradise extension packet 1 subtype = %d\n",
	       packet->subtype);
    }
}
