/*
 * detonate.c
 */
#include "copyright.h"

#include "config.h"

#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/* Detonate torp */

void
detmine(void)
{
    if (paradise) {
	sendDetMineReq(-1);
    } else {
	register int i;

	for (i = 0; i < ntorps; i++) {
	    int     j = i + me->p_no * ntorps;
	    if (torps[j].t_status == TMOVE ||
		torps[j].t_status == TSTRAIGHT) {
		sendDetMineReq(j);
	    }
	}
    }
}
