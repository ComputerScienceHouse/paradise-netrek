/*
 * udpopt.c - present UDP control window
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#define UDPBORDER	2
#define UDPLEN		35

/* Set up the UDP control window */
void
udpwindow(void)
{
    register int i;

    for (i = 0; i < UDP_NUMOPTS; i++)
	udprefresh(i);

    /* Map window */
    W_MapWindow(udpWin);
}

/*
 * Refresh item i
 */
void
udprefresh(int i)
{
    char    buf[BUFSIZ];

    switch (i) {
    case UDP_CURRENT:
	sprintf(buf, "UDP channel is %s", (commMode == COMM_TCP) ?
		"CLOSED" : "OPEN");
	break;
    case UDP_STATUS:
	strcpy(buf, "> Status: ");
	switch (commStatus) {
	case STAT_CONNECTED:
	    strcat(buf, "Connected");
	    break;
	case STAT_SWITCH_UDP:
	    strcat(buf, "Requesting switch to UDP");
	    break;
	case STAT_SWITCH_TCP:
	    strcat(buf, "Requesting switch to TCP");
	    break;
	case STAT_VERIFY_UDP:
	    strcat(buf, "Verifying UDP connection");
	    break;
	default:
	    fprintf(stderr, "netrek: UDP error: bad commStatus (%d)\n",
		    commStatus);
	}
	break;
    case UDP_DROPPED:
	sprintf(buf, "> UDP trans dropped: %d (%d%% | %d%%)", udpDropped,
		udpDropped * 100 / udpTotal,	/* (udpTotal always > 0) */
		udpRecentDropped * 100 / UDP_RECENT_INTR);
	break;
    case UDP_SEQUENCE:
	sprintf(buf, "Sequence checking is %s", (udpSequenceChk) ?
		"ON" : "OFF");
	break;
    case UDP_DEBUG:
	sprintf(buf, "Debugging info is ");
	switch (udpDebug) {
	case 0:
	    strcat(buf, "OFF");
	    break;
	case 1:
	    strcat(buf, "ON (connect msgs only)");
	    break;
	case 2:
	    strcat(buf, "ON (verbose output)");
	    break;
	}
	break;
    case UDP_SEND:
	sprintf(buf, "Sending with ");
	switch (udpClientSend) {
	case 0:
	    strcat(buf, "TCP only");
	    break;
	case 1:
	    strcat(buf, "simple UDP");
	    break;
	case 2:
	    strcat(buf, "enforced UDP (state only)");
	    break;
	case 3:
	    strcat(buf, "enforced UDP (state & weap)");
	    break;
	}
	break;
    case UDP_RECV:
	sprintf(buf, "Receiving with ");
	switch (udpClientRecv) {
	case MODE_TCP:
	    strcat(buf, "TCP only");
	    break;
	case MODE_SIMPLE:
	    strcat(buf, "simple UDP");
	    break;
	case MODE_FAT:
	    strcat(buf, "fat UDP");
	    break;
	}
	break;
    case UDP_FORCE_RESET:
	sprintf(buf, "Force reset to TCP");
	break;
    case UDP_UPDATE_ALL:
	sprintf(buf, "Request full update (=)");
	break;
    case UDP_DONE:
	strcpy(buf, "Done");
	break;
    default:
	fprintf(stderr, "netrek: UDP error: bad udprefresh(%d) call\n", i);
    }

    W_WriteText(udpWin, 0, i, textColor, buf, strlen(buf), 0);
}

void
udpaction(W_Event *data)
{
    register int i;

    switch (data->y) {
    case UDP_CURRENT:
	if (commMode == COMM_TCP)
	    sendUdpReq(COMM_UDP);
	else
	    sendUdpReq(COMM_TCP);
	break;

    case UDP_STATUS:
    case UDP_DROPPED:
	W_Beep();
	break;
    case UDP_SEQUENCE:
	udpSequenceChk = !udpSequenceChk;
	udprefresh(UDP_SEQUENCE);
	break;
    case UDP_SEND:
	udpClientSend++;
	if (udpClientSend > 3)
	    udpClientSend = 0;
	udprefresh(UDP_SEND);
	break;
    case UDP_RECV:
	udpClientRecv++;
	if (udpClientRecv >= MODE_DOUBLE)
	    udpClientRecv = 0;
	udprefresh(UDP_RECV);
	sendUdpReq(COMM_MODE + udpClientRecv);
	break;
    case UDP_DEBUG:
	udpDebug++;
	if (udpDebug > 2)
	    udpDebug = 0;
	udprefresh(UDP_DEBUG);
	break;
    case UDP_FORCE_RESET:
	/* clobber UDP */
	UDPDIAG(("*** FORCE RESET REQUESTED\n"));
	sendUdpReq(COMM_TCP);
	commMode = commModeReq = COMM_TCP;
	commStatus = STAT_CONNECTED;
	commSwitchTimeout = 0;
	udpClientSend = udpClientRecv = udpSequenceChk = udpTotal = 1;
	udpDebug = udpDropped = udpRecentDropped = 0;
	if (udpSock >= 0)
	    closeUdpConn();
	for (i = 0; i < UDP_NUMOPTS; i++)
	    udprefresh(i);
	break;
    case UDP_UPDATE_ALL:
	sendUdpReq(COMM_UPDATE);
	break;
    case UDP_DONE:
	udpdone();
	break;
    }
}

void
udpdone(void)
{
    /* Unmap window */
    W_UnmapWindow(udpWin);
}
