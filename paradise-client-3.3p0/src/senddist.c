/*
 * distress.c
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/* #$!@$#% length of address field of messages */
#define ADDRLEN 10


/* this loads all sorts of useful data into a distress struct.
 */
struct distress *
loaddistress(enum dist_type i, W_Event *data)
{
    struct distress *dist;
    struct obtype *target;

    dist = (struct distress *) malloc(sizeof(struct distress));

    dist->sender = me->p_no;
    dist->dam = (100 * me->p_damage) / me->p_ship->s_maxdamage;
    dist->shld = (100 * me->p_shield) / me->p_ship->s_maxshield;
    dist->arms = me->p_armies;
    dist->fuelp = (100 * me->p_fuel) / me->p_ship->s_maxfuel;
    dist->wtmp = (100 * me->p_wtemp) / me->p_ship->s_maxwpntemp;
    dist->etmp = (100 * me->p_etemp) / me->p_ship->s_maxegntemp;
    /* so.. call me paranoid -jmn */
    dist->sts = (me->p_flags & 0xff) | 0x80;
    dist->wtmpflag = ((me->p_flags & PFWEP) > 0) ? 1 : 0;
    dist->etempflag = ((me->p_flags & PFENG) > 0) ? 1 : 0;
    dist->cloakflag = ((me->p_flags & PFCLOAK) > 0) ? 1 : 0;

    dist->distype = i;
    if (dist->distype > generic || dist->distype < take)
	dist->distype = generic;

    target = gettarget(0, me->p_x, me->p_y, TARG_PLANET);
    dist->close_pl = target->o_num;

    target = gettarget(data->Window, data->x, data->y, TARG_PLANET);
    dist->tclose_pl = target->o_num;

    target = gettarget(0, me->p_x, me->p_y, TARG_PLAYER | TARG_ENEMY);
    dist->close_en = target->o_num;

    target = gettarget(data->Window, data->x, data->y, TARG_PLAYER | TARG_ENEMY);
    dist->tclose_en = target->o_num;

    target = gettarget(0, me->p_x, me->p_y, TARG_PLAYER | TARG_FRIENDLY);
    dist->close_fr = target->o_num;

    target = gettarget(data->Window, data->x, data->y, TARG_PLAYER | TARG_FRIENDLY);
    dist->tclose_fr = target->o_num;

    target = gettarget(0, me->p_x, me->p_y, TARG_PLAYER);
    dist->close_j = target->o_num;

    target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
    dist->tclose_j = target->o_num;

    /* lets make sure these aren't something stupid */
    dist->cclist[0] = 0x80;
    dist->preappend[0] = '\0';
    dist->macroflag = 0;

    return (dist);
}

/* Coordinating function for _SENDING_ a RCD */
/* Send an rcd signal out to everyone. */

void
rcd(enum dist_type i, W_Event *data)
{
    char    ebuf[200];
    struct distress *dist;
    char    cry[MSG_LEN];
    char   *info;
    int     len;
    int     recip;
    int     group;


    group = MTEAM;
    recip = idx_to_mask(me->p_teami);

    dist = loaddistress(i, data);

    if (F_gen_distress) {
	/* send a generic distress message */
	Dist2Mesg(dist, ebuf);
	pmessage(ebuf, recip, group | MDISTR);
    } else {
	len = makedistress(dist, cry, distmacro[dist->distype].macro);

	if (len > 0) {
	    /* klude alert */
	    info = cry;
	    if (strncmp(getaddr2(MTEAM, mask_to_idx(recip)), cry, 8) == 0) {
		/*
		   this means we should _strip_ the leading bit because it's
		   redundant
		*/
		info = cry + 9;
	    }
	    pmessage(info, recip, group);
	}
    }

    free(dist);
}

/* the primary subroutine for newmacro, converts the strange and wonderful
   ** newmacro syntax into an actual message.
   ** This is about as inefficient as they come, but how often is the player
   ** going to send a macro??
   **  6/3/93 - jn
 */
int
pmacro(int mnum, char who, W_Event *data)
{
    char    addr;
    int     group, len, recip;
    char    cry[MSG_LEN];
    char   *pm;
    struct distress *dist;


    if (!F_UseNewMacro)
	return 0;

    /* get recipient and group */
    if ((who == 't') || (who == 'T'))
	addr = teaminfo[me->p_teami].letter;
    else
	addr = who;

    group = getgroup(addr, &recip);

    if (!group) {
	printf("Bad group!\n");
	return (0);
    }
    pm = macrotable[mnum]->string;

    dist = loaddistress(generic, data);

    len = makedistress(dist, cry, pm);

    if (len > 0)
	pmessage(cry, recip, group);

    free(dist);
    return 1;
}
