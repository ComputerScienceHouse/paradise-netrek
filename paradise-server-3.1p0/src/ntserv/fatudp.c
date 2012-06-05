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

/* define to get LOTS of fat UDP debugging messages */
/* #define FATDIAG */

FAT_NODE fat_kills[MAXPLAYER];
FAT_NODE fat_torp_info[MAXPLAYER * MAXTORP];
FAT_NODE fat_thingy_info[TOTALTHINGIES];
FAT_NODE fat_phaser[MAXPLAYER];
FAT_NODE fat_plasma_info[MAXPLAYER * MAXPLASMA];
FAT_NODE fat_you;
FAT_NODE fat_status2;
FAT_NODE fat_planet2[MAXPLANETS];
FAT_NODE fat_flags[MAXPLAYER];
FAT_NODE fat_hostile[MAXPLAYER];

/* define the lists */
#define MAX_FAT_LIST	5	/* tweakable; should be > 1 */
typedef struct {
    FAT_NODE *head;
    FAT_NODE *tail;
}       FAT_LIST;
static FAT_LIST fatlist[MAX_FAT_LIST], tmplist[MAX_FAT_LIST];
/* tweakable parameters; compare with UDPBUFSIZE */
/* NOTE: FAT_THRESH + MAX_FAT_DATA must be < UDPBUFSIZE                */
/*       MAX_FAT_DATA must be larger than biggest semi-critical packet */
#define MAX_FAT_DATA	100	/* add at most this many bytes */
#define MAX_NONFAT	10	/* if we have this much left, stop */


void
reset_fat_list(void)
{
    int	i;
    for (i = 0; i < MAX_FAT_LIST; i++)
	fatlist[i].head = fatlist[i].tail = (FAT_NODE *) NULL;
}
/*
 * ---------------------------------------------------------------------------
 *	Fat City
 * ---------------------------------------------------------------------------
 */

/*
 * Remove a FAT_NODE from a queue.  If it's at the head or the tail of a
 * list, then we need to figure out which list it's in and update the head
 * or tail pointer.  It's easier to go searching than to maintain a queue
 * number in every FAT_NODE.
 *
 * This routine looks too complex... there must be a simpler way to do this.
 */
static void 
dequeue(FAT_NODE *fatp)
{
    int     i;

#ifdef V_FATDIAG
    for (i = 0; i < MAX_FAT_LIST; i++) {
	printf("fatlist[i].head = 0x%.8lx tail = 0x%.8lx\n",
	       fatlist[i].head, fatlist[i].tail);
	if ((fatlist[i].head == NULL && fatlist[i].tail != NULL) ||
	    (fatlist[i].head != NULL && fatlist[i].tail == NULL)) {
	    printf("before!\n");
	    kill(getpid(), 15);
	}
    }
#endif


    if (fatp->next == NULL) {
	/* it's at the head or not in a queue */
	for (i = 0; i < MAX_FAT_LIST; i++) {
	    if (fatlist[i].head == fatp) {
		fatlist[i].head = fatp->prev;	/* move head back */
		if (fatlist[i].head != NULL)
		    (fatlist[i].head)->next = NULL;	/* amputate */
		break;
	    }
	}
    }
    else {
	/* it's not at the head */
	if (fatp->prev != NULL)
	    fatp->prev->next = fatp->next;
    }

    if (fatp->prev == NULL) {
	/* it's at the tail or not in a queue */
	for (i = 0; i < MAX_FAT_LIST; i++) {
	    if (fatlist[i].tail == fatp) {
		fatlist[i].tail = fatp->next;	/* move head fwd */
		if (fatlist[i].tail != NULL)
		    (fatlist[i].tail)->prev = NULL;	/* amputate */
		break;
	    }
	}
    }
    else {
	/* it's not at the tail */
	if (fatp->next != NULL)
	    fatp->next->prev = fatp->prev;
    }

#ifdef FATDIAG
    printf("Removed 0x%.8lx...", fatp);	/* FATDIAG */
    for (i = 0; i < MAX_FAT_LIST; i++) {
	if ((fatlist[i].head == NULL && fatlist[i].tail != NULL) ||
	    (fatlist[i].head != NULL && fatlist[i].tail == NULL)) {
	    printf("after: %d %.8lx %.8lx\n", i, fatlist[i].head, fatlist[i].tail);
	    kill(getpid(), 15);
	}
    }
#endif
    fatp->prev = NULL;
    fatp->next = NULL;
}

/*
 * Add a FAT_NODE to the tail of a temporary queue.  The merge() routine
 * merges the temporary queues with the fatlists once the transmission is
 * sent.
 */
static void 
enqueue(FAT_NODE *fatp, int list)
{
#ifdef FATDIAG
    printf("added to tmplist %d\n", list);	/* FATDIAG */
#endif

    if (tmplist[list].tail == NULL) {
	/* list was empty */
	tmplist[list].tail = tmplist[list].head = fatp;
    }
    else {
	/* list wasn't empty */
	fatp->next = tmplist[list].tail;
	fatp->next->prev = fatp;
	tmplist[list].tail = fatp;
    }
}

/*
 * This updates the "fat" tables; it's called from sendClientData().
 */
/* pick a random type for the packet */
void 
updateFat(struct player_spacket *packet)
{
    FAT_NODE *fatp;
    struct kills_spacket *kp;
    struct torp_info_spacket *tip;
    struct thingy_info_spacket *thip;
    struct phaser_spacket *php;
    struct plasma_info_spacket *pip;
    /* struct you_spacket *yp; */
    /* struct status_spacket2 *sp2; */
    /* struct planet_spacket *plp; */
    struct planet_spacket2 *plp2;
    struct flags_spacket *fp;
    struct hostile_spacket *hp;
    int     idx;

    /* step 1 : find the FAT_NODE for this packet */
    switch (packet->type) {
    case SP_KILLS:
	kp = (struct kills_spacket *) packet;
	idx = (int) kp->pnum;
	fatp = &fat_kills[idx];
	break;
    case SP_TORP_INFO:
	tip = (struct torp_info_spacket *) packet;
	idx = (int) ntohs(tip->tnum);
	fatp = &fat_torp_info[idx];
	break;
    case SP_THINGY_INFO:
	thip = (struct thingy_info_spacket *) packet;
	idx = (int) ntohs(thip->tnum);
	fatp = &fat_thingy_info[idx];
	break;
    case SP_PHASER:
	php = (struct phaser_spacket *) packet;
	idx = (int) php->pnum;
	fatp = &fat_phaser[idx];
	break;
    case SP_PLASMA_INFO:
	pip = (struct plasma_info_spacket *) packet;
	idx = (int) ntohs(pip->pnum);
	fatp = &fat_plasma_info[idx];
	break;
    case SP_YOU:
	/* yp = (struct you_spacket *) packet; */
	fatp = &fat_you;
	break;
    case SP_STATUS2:
	/* sp = (struct status_spacket *) packet; */
	fatp = &fat_status2;
	break;
    case SP_PLANET2:
	plp2 = (struct planet_spacket2 *) packet;
	idx = plp2->pnum;
	fatp = &fat_planet2[idx];
	break;
    case SP_FLAGS:
	fp = (struct flags_spacket *) packet;
	idx = (int) fp->pnum;
	fatp = &fat_flags[idx];
	break;
    case SP_HOSTILE:
	hp = (struct hostile_spacket *) packet;
	idx = (int) hp->pnum;
	fatp = &fat_hostile[idx];
	break;
    default:
	fprintf(stderr, "Fat error: bad semi-critical type (%d) in updateFat\n", (CARD8)packet->type);
	return;
    }

    if (fatp->packet != (PTR) packet) {
	fprintf(stderr, "Fat error: fatp->packet=0x%.8lx, packet=0x%.8lx\n",
		(unsigned long) fatp->packet, (unsigned long) packet);
	return;
    }
    /* step 2 : move this dude to temporary list 0 */
    dequeue(fatp);
    enqueue(fatp, 0);
}

/*
 * This fattens up the transmission, adding up to MAX_FAT_DATA bytes.  The
 * packets which get added will be moved to a higher queue, giving them less
 * priority for next time.  Note that they are added to a parallel temporary
 * list (otherwise they'd could be sent several times in the same transmission
 * as the algorithm steps through the queues), and merged later on.
 *
 * Packets are assigned from head to tail, on a first-fit basis.  If a
 * semi-critical packet is larger than MAX_FAT_DATA, this routine will never
 * send it, but it will skip around it.
 *
 * This routine is called from flushSockBuf, before the transmission is sent.
 *
 * A possible improvement is to have certain packets "expire", never to be
 * seen again.  This way we don't keep resending torp packets for players who
 * are long dead.  This raises the possibility that the dead player's torps
 * will never go away though.
 */
int
fatten(void)
{
    int     bytesleft;
    FAT_NODE *fatp, *nextfatp;
    int     list;

#ifdef FATDIAG
    printf("--- fattening\n");
#endif
    bytesleft = MAX_FAT_DATA;
    for (list = 0; list < MAX_FAT_LIST; list++) {
	fatp = fatlist[list].head;
	while (fatp != NULL) {
	    nextfatp = fatp->prev;	/* move toward tail */
#ifdef FATDIAG
	    if (nextfatp == fatp) {
		printf("Hey!  nextfatp == fatp!\n");
		kill(getpid(), 15);
	    }
#endif
	    if (fatp->pkt_size < bytesleft) {
		/* got one! */
		sendUDPbuffered(0, fatp->packet, fatp->pkt_size);
		bytesleft -= fatp->pkt_size;

		packets_sent++;	/* counts as a udp packet sent */

		/* move the packet to a higher queue (if there is one) */
		dequeue(fatp);
		if (list + 1 == MAX_FAT_LIST) {
		    /* enqueue(fatp, list); *//* keep packets on high queue? */
		}
		else {
		    enqueue(fatp, list + 1);
		}

		/* done yet? */
		if (bytesleft < MAX_NONFAT)
		    goto done;	/* don't waste time searching anymore */
	    }
	    fatp = nextfatp;
	}
    }

done:
    /* at this point, we either filled with fat or ran out of queued packets */
#ifdef FATDIAG
    printf("--- done\n");
#endif
    V_UDPDIAG(("- Added %d grams of fat\n", MAX_FAT_DATA - bytesleft));
    return (0);			/* some compilers need something after a goto */
}


/*
 * This gets called from flushSockBuf after the transmission is sent.  It
 * appends all the packets sitting in temporary queues to the corresponding
 * fat queues, where they will be eligible for fattening next transmission.
 */
void 
fatMerge(void)
{
    int     i;

    for (i = 0; i < MAX_FAT_LIST; i++) {
	if (tmplist[i].head == NULL)
	    continue;		/* temp list is empty, nothing to do */

	if (fatlist[i].head == NULL) {
	    /* fatlist is empty; just copy pointers */
	    fatlist[i].head = tmplist[i].head;
	    fatlist[i].tail = tmplist[i].tail;

	}
	else {
	    /* stuff in both */
	    (tmplist[i].head)->next = fatlist[i].tail;	/* fwd pointer */
	    (fatlist[i].tail)->prev = tmplist[i].head;	/* back pointer */
	    fatlist[i].tail = tmplist[i].tail;	/* move tail back */
	    tmplist[i].head = tmplist[i].tail = NULL;	/* clear the tmp list */
	}
	tmplist[i].head = tmplist[i].tail = NULL;
#ifdef FATDIAG
	printf("merged list %d: %.8lx %.8lx\n", i, fatlist[i].head,
	       fatlist[i].tail);
#endif
    }
}
