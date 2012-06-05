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

#include <netdb.h>
#include "config.h"
#include "proto.h"
#include "ntserv.h"
#include "data.h"
#include "shmem.h"

#define PING_DEBUG				0	/* debugging */

/* special fields in stats record for rountrip delay and packet loss */
#define INL_STATS


static unsigned char ping_id;	/* wraparound expected */
static int ping_lag;		/* ping roundtrip delay */

static int tloss_sc,		/* total packet loss s-c */
        tloss_cs,		/* total packet loss c-s */
        iloss_sc,		/* inc. packet loss s-c */
        iloss_cs;		/* inc. packet loss c-s */

/*
 * This structure allows us to send several pings before any response
 * is received without losing information -- as would be the case for
 * roundtrip times equal to or larger then the ping interval times.
 * HASHSIZE * ping interval must be greater then the largest
 * expected roundtrip time.
 */

#define HASHSIZE		32
#define PITH(i)			(((int)i) & (HASHSIZE-1))

static
struct {
    long    time;
    long    packets_sent_at_ping;
}       ping_sent[HASHSIZE];


static void
calc_loss(int i, struct ping_cpacket *packet)
{
    /* tloss vars */
    register int cp_recv,		/* client packets recv */
                 cp_sent;		/* client packets sent */
    int     s_to_c_dropped,	/* server to client */
            c_to_s_dropped;	/* client to server */
    static int old_s_to_c_dropped,	/* previous update */
            old_c_to_s_dropped;	/* "" */
    /* iloss vars */
    int     p_sent, p_recv;

    static
    int     timer;

    static int inc_packets_sent,/* packets sent start-point */
            inc_packets_received,	/* packets recvd start-pt  */
            inc_s_to_c_dropped,	/* dropped s-to-c start-pt */
            inc_c_to_s_dropped;	/* dropped c-to-s start-pt */

    if (!timer)
	timer = configvals->ping_iloss_interval;

    cp_recv = ntohl(packet->cp_recv);
    cp_sent = ntohl(packet->cp_sent);

    /* at ping time, total packets dropped from server to client */
    s_to_c_dropped = ping_sent[i].packets_sent_at_ping - cp_recv;

    if (s_to_c_dropped < old_s_to_c_dropped) {
	/*
	   The network may duplicate or send out-of-order packets. Both are
	   detected and thrown out by the client if sequence checking is on.
	   If not there's not much we can do -- there's no way to distinguish
	   a duplicated packet from a series of out of order packets.  While
	   the latter case cancels itself out eventually in terms of packet
	   loss, the former hides real packet loss by adding extra packets.
	   We'll have to kludge it by adding the extra packets the client
	   thinks it got to packets_sent
	*/
	packets_sent += old_s_to_c_dropped - s_to_c_dropped;
	/* and adjust s_to_c_dropped so we don't get a negative packet loss */
	s_to_c_dropped = old_s_to_c_dropped;
    }

    /* total loss server-to-client since start of connection */
    tloss_sc = 100 -
	(100 * (ping_sent[i].packets_sent_at_ping - s_to_c_dropped)) /
	ping_sent[i].packets_sent_at_ping;

    /*
       at ping time, total packets dropped from client to server NOTE: not
       packets_received_at_ping since the client may have sent any amount of
       packets between the time we sent the ping and the time the client
       received it.
    */
    c_to_s_dropped = cp_sent - packets_received;

#if PING_DEBUG >= 2
    printf("cp_sent: %d, packets_received: %d\n",
	   cp_sent, packets_received);
#endif

    if (c_to_s_dropped < old_c_to_s_dropped) {
	/*
	   The network may duplicate or send out-of-order packets. Since no
	   sequence checking is done by the server, there's not much we can
	   do -- there's no way to distinguish a duplicated packet from a
	   series of out of order packets.  While the latter case cancels
	   itself out eventually in terms of packet loss, the former hides
	   real packet loss by adding extra packets. We'll have to kludge it
	   by subtracting the extra packets we think we got from the client
	   from packets_received.
	*/
	packets_received -= old_c_to_s_dropped - c_to_s_dropped;
	/* and adjust c_to_s_dropped so we don't get a negative packet loss */
	c_to_s_dropped = old_c_to_s_dropped;
    }

    /* total loss client-to-server since start of connection */
    tloss_cs = 100 -
	(100 * (packets_received - c_to_s_dropped)) / (packets_received ? packets_received : 1);

    old_s_to_c_dropped = s_to_c_dropped;
    old_c_to_s_dropped = c_to_s_dropped;

    /* Incremental packet loss */

    /* packets sent since last ping response */
    p_sent = ping_sent[i].packets_sent_at_ping - inc_packets_sent;

    /* packets received since last ping response */
    p_recv = packets_received - inc_packets_received;

    if (!p_sent || !p_recv) {
	/* just in case */
	return;
    }

    /* percent loss server-to-client since PACKET_LOSS_INTERVAL */
    iloss_sc = 100 -
	(100 * (p_sent - (s_to_c_dropped - inc_s_to_c_dropped))) / p_sent;
    /*
       we're not going to do any of the adjustments we did in tloss
       calculations since this starts fresh every PACKET_LOSS_INTERVAL
    */
    if (iloss_sc < 0)
	iloss_sc = 0;

    /* total percent loss client-to-server since PACKET_LOSS_INTERVAL */
    iloss_cs = 100 -
	(100 * (p_recv - (c_to_s_dropped - inc_c_to_s_dropped))) / p_recv;
    /*
       we're not going to do any of the adjustments we did in tloss
       calculations since this starts fresh every PACKET_LOSS_INTERVAL
    */
    if (iloss_cs < 0)
	iloss_cs = 0;

    /*
       we update these variables every PACKET_LOSS_INTERVAL seconds to start
       a fresh increment
    */
    if ((timer % configvals->ping_iloss_interval) == 0) {
	inc_s_to_c_dropped = s_to_c_dropped;
	inc_c_to_s_dropped = c_to_s_dropped;

	inc_packets_sent = ping_sent[i].packets_sent_at_ping;
	inc_packets_received = packets_received;
    }

    timer++;
}

#ifdef INL_STATS

/*
 * Lag stats
 * struct player .p_avrt -	average round trip time ms
 * struct player .p_stdv - 	standard deviation in rt time
 * struct player .p_pkls -	input/output packet loss
 */

static int sum, n, s2;
static int M, var;

static void
update_lag_stats(void)
{
    n++;
    sum += ping_lag;
    s2 += (ping_lag * ping_lag);
    if (n == 1)
	return;

    M = sum / n;
    var = (s2 - M * sum) / (n - 1);

    /* average round trip time */
    me->p_avrt = M;
    /* standard deviation */
    if (var > 0)
	me->p_stdv = (int) isqrt(var);
}

static void
update_loss_stats(void)
{
    /*
       packet loss (as average of server-to-client, client-to-server loss),
       give tloss_sc extra weight (or should we?)
    */
    me->p_pkls = (2 * tloss_sc + tloss_cs) / 3;
}
#endif				/* INL_STATS */

/* utilities */

/* ms time from start */
static int
mstime(void)
{
    static struct timeval tv_base = {0, 0};
    struct timeval tv;

    if (!tv_base.tv_sec) {
	gettimeofday(&tv_base, NULL);
	return 0;
    }
    gettimeofday(&tv, NULL);
    return (tv.tv_sec - tv_base.tv_sec) * 1000 +
	(tv.tv_usec - tv_base.tv_usec) / 1000;
}

#ifdef PING_DEBUG
/* debugging */
static int
msetime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec - 732737182) * 1000 + tv.tv_usec / 1000;
}
#endif

static int
uchar_diff(int x, int y)
{
    register int res;

    res = x - y;

    if (res > 128)
	return res - 256;
    else if (res < -128)
	return res + 256;
    else
	return res;
}

/*
 * response from client
 */

void
pingResponse(struct ping_cpacket *packet)
{
    register int i;
    static int last_num;

    if (!ping || packet->pingme != 1)
	return;			/* oops, garbage */

    ping_ghostbust = 0;		/* don't ghostbust, client is alive */

    /* throw out out-of-order packets */
    i = uchar_diff((int) packet->number, last_num);
    if (i < 0) {
#if PING_DEBUG >= 1
	fprintf(stderr, "out-of-order response ignored: %d (last: %d)\n",
		packet->number, last_num);
	fflush(stderr);
#endif
	return;
    }
    last_num = packet->number;
    i = PITH(last_num);

    /* calculate roundtrip */
    ping_lag = mstime() - ping_sent[i].time;

#ifdef INL_STATS
    /* fill in lag stats fields */
    update_lag_stats();
#endif

    /* watch out for div by 0 */
    if (!packets_received || !ping_sent[i].packets_sent_at_ping)
	return;

    /* calculate total packet loss */
    calc_loss(i, packet);

#ifdef INL_STATS
    update_loss_stats();
#endif
}

/*
 * request from server
 */

void
sendClientPing(void)
{
    struct ping_spacket packet;

    ping_ghostbust++;
    ping_id++;			/* ok to wrap */

    packet.type = SP_PING;
    packet.number = (unsigned char) ping_id;
    packet.lag = htons((unsigned short) ping_lag);
    packet.tloss_sc = tloss_sc;
    packet.tloss_cs = tloss_cs;
    packet.iloss_sc = iloss_sc;
    packet.iloss_cs = iloss_cs;

    ping_sent[PITH(ping_id)].time = mstime();
    #ifdef PING_DEBUG
       printf("ping sent at %d\n", msetime());
    #endif

    sendClientPacket(&packet);

    ping_sent[PITH(ping_id)].packets_sent_at_ping = packets_sent;
}
