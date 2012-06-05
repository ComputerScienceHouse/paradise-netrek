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

#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include "config.h"
#include "proto.h"
#include "ntserv.h"
#include "data.h"
#include "shmem.h"

#define BUFSIZE 16738
static char buf[BUFSIZE];	/* Socket buffer */
static char *bufptr = buf;
#define UDPBUFSIZE 960		/* (tweakable; should be under 1300) */
static char udpbuf[UDPBUFSIZE];	/* UDP socket buffer */
static char *udpbufptr = udpbuf;
#ifdef DOUBLE_UDP
static char scbuf[UDPBUFSIZE];	/* semi-critical UDP socket buffer */
static char *scbufptr = scbuf;	/* (only used for double UDP) */
#endif
static long sequence;		/* the holy sequence number */

#define FAT_THRESH	500	/* if more than this, don't add fat */

extern int udpMode;

extern int clientDead;

int
buffersEmpty(void)
{
    return bufptr==buf &&
	(commMode!=COMM_UDP || udpbufptr==buf);
}

void
resetUDPbuffer(void)
{
    if (udpbufptr != udpbuf) {
	udpbufptr = udpbuf;	/* clear out any old data */
	sequence--;		/* we just killed a sequence packet */
    }
}

void resetUDPsequence(void)
{
    sequence = 1;
}

/*
 * If we're in UDP mode, add a sequence number to the transmission buffer.
 * Returns the #of bytes inserted.
 *
 * This will add a sequence # to transmissions on either channel.  However,
 * the current implementation doesn't put sequences on TCP transmissions
 * because mixed TCP packets and UDP packets rarely arrive in the order
 * in which they were sent.
 */
static int 
addSequence(char *outbuf)
{
    struct sequence_spacket *ssp;

    if (commMode != COMM_UDP || udpMode == MODE_TCP)
	return (0);

    packets_sent++;

    ssp = (struct sequence_spacket *) outbuf;
    ssp->type = SP_SEQUENCE;
    ssp->sequence = htons((unsigned short) sequence);
    sequence++;

    return (sizeof(struct sequence_spacket));
}

/* Flush the socket buffer */
void
flushSockBuf(void)
{
    int     cc;

    if (clientDead)
	return;
    if (bufptr != buf) {
	if ((cc = gwrite(sock, buf, bufptr - buf)) != bufptr - buf) {
	    fprintf(stderr, "std flush gwrite failed (%d, error %d)\n",
		    cc, errno);
	    clientDead = 1;
	}
	bufptr = buf /* + addSequence(buf) */ ;
    }
    /*
       This is where we try to add fat.  There's no point in checking at the
       other places which call gwrite(), because they only call it when the
       buffer is already full.
    */
    if (udpSock >= 0
	&& udpMode == MODE_FAT
	&& (udpbufptr - udpbuf) < FAT_THRESH)
	fatten();

    if (udpSock >= 0 && udpbufptr != udpbuf) {
#ifdef BROKEN
	/* debugging only!! */
	if (sequence % 5 == 0) {
	    /* act as if we did the gwrite(), but don't */
	    udpbufptr = udpbuf + addSequence(udpbuf);
	    goto foo;
	}
#endif
	if ((cc = gwrite(udpSock, udpbuf, udpbufptr - udpbuf)) != udpbufptr - udpbuf) {
	    fprintf(stderr, "UDP flush gwrite failed (%d, error %d)\n",
		    cc, errno);
/*	    clientDead=1;*/
	    UDPDIAG(("*** UDP disconnected for %s\n", me->p_name));
	    printUdpInfo();
	    closeUdpConn();
	    commMode = COMM_TCP;
	}
#ifdef DOUBLE_UDP
	sendSC();
#endif
	udpbufptr = udpbuf + addSequence(udpbuf);
    }
#ifdef BROKEN
foo:
#endif
    if (udpMode == MODE_FAT)
	fatMerge();
}

/* Transmission of some packets can be delayed indefinitely */

struct deferred_packet {
    void	*data;
    int	size;
    struct deferred_packet	*next;
};

static struct deferred_packet *df_head, *df_tail;

static int
haveDeferredPackets(void)
{
    return df_head != 0;
}

void
build_select_masks(fd_set *readfds, fd_set *writefds)
{
    if (readfds) {
	FD_ZERO(readfds);
	FD_SET(sock, readfds);
	if (udpSock>=0)
	    FD_SET(udpSock, readfds);
    }
    if (writefds) {
	FD_ZERO(writefds);
	if (haveDeferredPackets())
	    FD_SET(sock, writefds);
    }
}

int
socketPause(void)
{
    struct timeval timeout;
    fd_set     readfds, writefds;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    build_select_masks(&readfds, &writefds);

    return select(32, (fd_set *) & readfds, &writefds, 0, &timeout);
}

int
socketWait(void)
{
    fd_set  readfds, writefds;

    build_select_masks(&readfds, &writefds);

    return select(32, &readfds, &writefds, 0, (struct timeval *) 0);
}

int
gwrite(int fd, char *wbuf, int bytes)
{
    int     orig = bytes;
    int     n;
    char    tempbuf[80];
    struct timeval to;


    while (bytes) {
	n = write(fd, wbuf, bytes);
	if (n < 0) {
	    if (errno == ENOBUFS) {
		/*
		   The man pages don't mention this as a possibility. Yet, it
		   happens.  I guess I just wait for 1/10 sec, and continue?
		*/
		/*
		   I would use usleep() to do this, but this system ain't got
		   it...
		*/
		/* note: changed from 100 ms to 20 ms. (HAK) */
		to.tv_sec = 0;
		to.tv_usec = 20000;
		select(0, NULL, NULL, NULL, &to);
		continue;
	    }
	    if (errno == EINTR)	/* interrupted by signal, restart */
		continue;

	    if (fd == udpSock) {
		/* do we want Hiccup code here? */
		UDPDIAG(("Tried to write %d, 0x%lx, %d (error %d)\n",
			 fd, (unsigned long)wbuf, bytes, errno));
		printUdpInfo();
		logmessage("UDP gwrite failed:");
	    }
	    sprintf(tempbuf, "Died in gwrite, n=%d, errno=%d <%s@%s>",
		    n, errno, me->p_login, me->p_full_hostname);
	    logmessage(tempbuf);
	    return (-1);
	}
	bytes -= n;
	wbuf += n;
    }
    return (orig);
}


/* issc - is semi-critical */
void
sendUDPbuffered(int issc, void *packet, int size)
{
    if (udpbufptr - udpbuf+size >= UDPBUFSIZE) {
	int	cc;
	if ((cc = gwrite(udpSock, udpbuf, udpbufptr - udpbuf)) !=
	    udpbufptr - udpbuf) {
	    fprintf(stderr, "UDP gwrite failed (%d, error %d)\n",
		    cc, errno);
	    /*		    clientDead=1;*/
	    UDPDIAG(("*** UDP disconnected for %s\n", me->p_name));
	    printUdpInfo();
	    closeUdpConn();
	    commMode = COMM_TCP;
	}
#ifdef DOUBLE_UDP
	sendSC();	/* send semi-critical info, if needed */
#endif
	udpbufptr = udpbuf + addSequence(udpbuf);
    }
    memcpy(udpbufptr, packet, size);
    udpbufptr += size;

#ifdef DOUBLE_UDP
    if (issc && udpMode == MODE_DOUBLE) {
	memcpy(scbufptr, packet, size);
	scbufptr += size;
	V_UDPDIAG((" adding SC\n"));
    }
#endif
    if (issc && udpMode == MODE_FAT) {
	updateFat(packet);
    }
}


void 
sendTCPbuffered(void *packet, int size)
{
    int	cc;
    /* these are critical packets; send them via TCP */
    if (bufptr - buf + size >= BUFSIZE) {
	if ((cc = gwrite(sock, buf, bufptr - buf)) != bufptr - buf) {
	    fprintf(stderr, "TCP gwrite failed (%d, error %d)\n",
		    cc, errno);
	    clientDead = 1;
	}
	bufptr = buf		/* + addSequence(buf) */ ;
    }
    memcpy(bufptr, packet, size);
    bufptr += size;
}

/* Put a packet on the deferred queue. */

void
sendTCPdeferred(void *packet, int size)
{
    /* I'm having problems with UDP connection packet */
    sendTCPbuffered(packet, size);
}

/* When the socket is ready for write, toss a packet through the pipe
   Hopefully it won't block...
 */

void
flushDeferred(void)
{
    int	rval;

    if (df_head==0)
	return;

    /* could block, oh well */
    rval = gwrite(sock, df_head->data, df_head->size);

    if (rval != df_head->size) {
	fprintf(stderr, "TCP gwrite (deferred) failed (%d, error %d)\n",
		    rval, errno);
	clientDead = 1;
    }

    {
	struct deferred_packet	*temp=df_head;
	df_head = temp->next;
	free(temp->data);
	free(temp);
    }
    if (!df_head)
	df_tail = 0;		/* queue is empty */
}

/* sends all the deferred packets through the TCP buffer */
void
undeferDeferred(void)
{
    while (df_head) {
	sendTCPbuffered(df_head->data, df_head->size);

	{
	    struct deferred_packet	*temp=df_head;
	    df_head = temp->next;
	    free(temp->data);
	    free(temp);
	}
    }
    df_tail = 0;
}
