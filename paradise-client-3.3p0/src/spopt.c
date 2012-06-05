/*
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

/*
 * Refresh item i
 */

void
sprefresh(int i)
{
    char    buf[BUFSIZ];

    switch (i) {

    case SPK_VFIELD:
	sprintf(buf, "%seceive variable and short packets",
		recv_short ? "R" : "Don't r");
	break;
    case SPK_MFIELD:
	sprintf(buf, "%seceive messages", recv_mesg ? "R" : "Don't r");
	break;
    case SPK_KFIELD:
	sprintf(buf, "%seceive kill messages", recv_kmesg ? "R" : "Don't r");
	break;
    case SPK_WFIELD:
	sprintf(buf, "%seceive warning messages", recv_warn ? "R" : "Don't r");
	break;
    case SPK_TFIELD:
	sprintf(buf, "Receive threshold: %s_", recv_threshold_s);
	break;
    case SPK_WHYFIELD:
	sprintf(buf, "%sdd why dead messages", why_dead ? "A" : "Don't a");
	break;
    case SPK_DONE:
	sprintf(buf, "Done");
	break;
    }

    W_WriteText(spWin, 0, i, textColor, buf, strlen(buf), 0);
}

void
spwindow(void)
{
    register int i;

    for (i = 0; i < SPK_NUMFIELDS; i++)
	sprefresh(i);

    /* Map window */
    W_MapWindow(spWin);
}

void
spdone(void)
{
    /* Unmap window */
    W_UnmapWindow(spWin);
}

void
spaction(W_Event *data)
{
    int     v;
    register int i;
    register char *cp;

    /* unmap window on space or ESC [BDyess] */
    if(data->type == W_EV_KEY && (data->key == ' ' || data->key == 27)) {
      W_UnmapWindow(spWin);
      return;
    }

    switch (data->y) {

    case SPK_VFIELD:
	if (data->type == W_EV_BUTTON) {
	    if (recv_short)
		sendShortReq(SPK_VOFF);
	    else
		sendShortReq(SPK_VON);
	}
	break;

    case SPK_MFIELD:
	if (data->type == W_EV_BUTTON) {
	    if (recv_mesg)
		sendShortReq(SPK_MOFF);
	    else
		sendShortReq(SPK_MON);
	}
	break;

    case SPK_KFIELD:
	if (data->type == W_EV_BUTTON) {
	    if (recv_kmesg)
		sendShortReq(SPK_M_NOKILLS);
	    else
		sendShortReq(SPK_M_KILLS);
	}
	break;

    case SPK_WFIELD:
	if (data->type == W_EV_BUTTON) {
	    if (recv_warn)
		sendShortReq(SPK_M_NOWARN);
	    else
		sendShortReq(SPK_M_WARN);
	}
	break;

    case SPK_TFIELD:
	if (data->type == W_EV_KEY) {
	    switch (data->key) {
	    case '\b':
	    case '\177':
		cp = recv_threshold_s;
		i = strlen(cp);
		if (i > 0) {
		    cp += i - 1;
		    *cp = '\0';
		}
		break;
	    case '\025':
	    case '\030':
		recv_threshold_s[0] = '\0';
		break;

	    default:
		if (data->key >= '0' && data->key <= '9') {
		    cp = recv_threshold_s;
		    i = strlen(cp);
		    if (i < 4) {
			cp += i;
			cp[1] = '\0';
			cp[0] = data->key;
		    }
		}
		break;
	    }
	    sprefresh(SPK_TFIELD);
	}
	break;

    case SPK_WHYFIELD:
	if (F_feature_packets && data->type == W_EV_BUTTON) {
	    if (why_dead)
		sendFeature("WHY_DEAD", 'S', 0, 0, 0);
	    else
		sendFeature("WHY_DEAD", 'S', 1, 0, 0);
	}
	break;
    case SPK_DONE:

	if (data->type == W_EV_BUTTON) {
	    if (sscanf(recv_threshold_s, "%d", &v) != 1)
		strcpy(recv_threshold_s, "0");
	    else if (recv_threshold != v) {
		recv_threshold = v;
		sendThreshold(recv_threshold);
	    }
	    spdone();
	}
	break;

    }
}

