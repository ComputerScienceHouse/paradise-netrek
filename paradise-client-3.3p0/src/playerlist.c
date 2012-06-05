/*
 * playerlist.c
 * modified to sort by teams by Bill Dyess on 9/23/93
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
#include "packets.h"
#include "gppackets.h"

/* Prototypes */
static void dofulllist P((struct player * pptr, int vpos));
/* static char *get_players_rank_name P((struct player *j));*/
void getdesig P((struct player * j, char *desig));

struct teamstruct {
    short   teamnum;
    short   totalnum;
    short   outfitnum;
    short   row;
    short   outfitrow;
};

int     lastsortPlayers, lasthnk, lastspl, lastshowDead;

void
playerlist(void)
{
    int     i;
    char    buf[100];

    if (slot == NULL) {
	if ((slot = (short *) malloc(sizeof(short) * 33)) == NULL) {
	    perror("out of memory?!?\n");
	    exit(-1);
	}
    }
    if (*playerList != 0 && *playerList != ',') {	/* do the wide thing
							   [BDyess] */
	wideplayerlist();
	return;
    }
    for (i = 0; i < 33; i++)
	slot[i] = -2;
    lastsortPlayers = sortPlayers;
    lasthnk = hideNoKills;
    lastspl = showPreLogins;
    lastshowDead = showDead;

    W_ClearWindow(playerw);

    (void) strcpy(buf, "  Type Rank      Name            Kills        Type Rank      Name            Kills");
    W_WriteText(playerw, 0, 1, textColor, buf, strlen(buf), W_RegularFont);

    if (!paradise)
	(void) strcpy(buf, "  Type Rank      Name            Kills   Win  Loss  Ratio Offense Defense     DI");
    else
	(void) strcpy(buf, "  Type Rank      Name            Kills   Win  Loss  Ratio  Battle Strategy     DI");

    W_WriteText(playerw, 0, 20, textColor, buf, strlen(buf), W_RegularFont);

    for (i = 0; i < nplayers; i++)
	updatePlayer[i] |= ALL_UPDATE;

    playerlist2();
}


void
playerlist2(void)
{
    register int i, k, z;
    char    buf[100];
    short   extra = 0, outfitextra = 0;
    struct teamstruct team[4];
    struct teamstruct *quadrant[4], *tempquad;	/* topleft, topright,
						   bottomleft, bottomright */
    register struct player *j;
    static short sequentialSort = 0;
    char   *rname;
    char    desig[3];
    short   currentSlot;
    static int usingWide = 0;

    if (*playerList != 0 && *playerList != ',') {	/* do the wide thing
							   [BDyess] */
	if (slot == NULL) {
	    playerlist();
	    return;
	}
	wideplayerlist2();
	usingWide = 1;
	return;
    }
    /* keeps the playerlist from being drawn when not allowed [BDyess] */
    if (!allowPlayerlist)
	return;

    if (slot == NULL || lastsortPlayers != sortPlayers ||
	lasthnk != hideNoKills ||
	lastspl != showPreLogins ||
	lastshowDead != showDead ||
	usingWide) {
	usingWide = 0;
        if(resizePlayerList)
            W_ResizeText(playerw,83,W_WindowHeight(playerw));
	playerlist();
	return;
    }
    if (!W_IsMapped(playerw))
	return;
    if (updatePlayer[me->p_no] & LARGE_UPDATE)
	dofulllist(me, 21);
    if (blk_bozolist >= 0)
	if (updatePlayer[blk_bozolist] & LARGE_UPDATE)
	    dofulllist(&players[blk_bozolist], 22);
    /*
       if sortPlayers is on: the players team goes in the top left (top right
       if robSort: on), the next-largest team in the top right, the third
       largest in the bottom left, and everything else in the bottom right
    */
    if (sortPlayers) {
	memset(team, 0, sizeof(team));
	for (i = 0; i < 4; i++)
	    team[i].teamnum = i;
	/* figure out how many players on each team */
	for (i = 0, j = &players[i]; i < nplayers; i++, j++) {
	    if (j->p_status != PFREE && !(j->p_status == POUTFIT && j->p_teami < 0)) {
		if (j->p_teami < 0 || j->p_teami == number_of_teams) {
		    if (!showDead)
			continue;
		    extra++;
		    if (j->p_status == POUTFIT && sortOutfitting)
			outfitextra++;
		} else if (j->p_teami == number_of_teams) {
		    if (!showPreLogins)
			continue;
		    extra++;
		    if (j->p_status == POUTFIT && sortOutfitting)
			outfitextra++;
		} else {
		    team[j->p_teami].totalnum++;
		    if (j->p_status == POUTFIT && showDead && sortOutfitting)
			team[j->p_teami].outfitnum++;
		}
	    }
	    /*
	       printf("player %d is on team %d with status %d\n",
	       i,j->p_teami,j->p_status);
	    */
	}

	for (i = 0; i < 4; i++)
	    quadrant[i] = &team[i];
	if (me->p_teami >= 0 && me->p_teami < number_of_teams)
	    team[me->p_teami].totalnum += 10000;
	for (i = 3; i > 0; i--) {
	    for (k = 0; k < i; k++) {
		if (quadrant[k]->totalnum < quadrant[k + 1]->totalnum) {
		    tempquad = quadrant[k];
		    quadrant[k] = quadrant[k + 1];
		    quadrant[k + 1] = tempquad;
		}
	    }
	}
	if (me->p_teami >= 0 && me->p_teami < number_of_teams) {
	    team[me->p_teami].totalnum -= 10000;
	    if (robsort) {
		quadrant[0] = quadrant[1];
		quadrant[1] = &team[me->p_teami];
	    }
	}
	quadrant[3]->totalnum += extra;
	quadrant[3]->outfitnum += outfitextra;

	if (quadrant[0]->totalnum > 16 || quadrant[1]->totalnum > 16) {
	    /*
	       if one team is so huge as to not fit on just one side, then
	       just list all the players sequentially, but still team sorted.
	       For now, set a flag and clear any freshly empty space.
	    */
	    if (sequentialSort == 0) {
		sequentialSort = 1;
		playerlist();
		return;
	    }
	    quadrant[0]->row = 0;
	    quadrant[1]->row = quadrant[0]->totalnum;
	    quadrant[2]->row = quadrant[1]->row + quadrant[1]->totalnum;
	    quadrant[3]->row = quadrant[2]->row + quadrant[2]->totalnum;
	    /* wipe out the blank sections */
	    for (i = quadrant[3]->row + quadrant[3]->totalnum; i < 32; i++) {
		if (slot[i] != -1) {
		    /*
		       if we get here, it's not possible to have players
		       blank on the left side
		    */
		    W_ClearArea(playerw, 44, i - 16 + 1, 41, 1);
		    slot[i] = -1;
		}
	    }
	} else {
	    if (sequentialSort == 1) {
		sequentialSort = 0;
		playerlist();
		return;
	    }
	    quadrant[0]->row = quadrant[1]->row = 2;
	    if (quadrant[0]->totalnum + quadrant[2]->totalnum <= 16) {
		quadrant[2]->row = 18 - quadrant[2]->totalnum;
	    } else {
		quadrant[2]->row = 2 + quadrant[0]->totalnum;
		quadrant[3]->totalnum += quadrant[0]->totalnum + quadrant[2]->totalnum - 16;
	    }
	    if (quadrant[1]->totalnum + quadrant[3]->totalnum <= 16) {
		quadrant[3]->row = 18 - quadrant[3]->totalnum;
	    } else {
		quadrant[3]->row = 2 + quadrant[1]->totalnum;
		quadrant[2]->row -= quadrant[1]->totalnum + quadrant[3]->totalnum - 16;
	    }

	    for (i = 0; i < 4; i++)
		quadrant[i]->outfitrow = quadrant[i]->row + quadrant[i]->totalnum -
		    quadrant[i]->outfitnum;
	    /* wipe out the blank sections */
	    for (i = 2 + quadrant[0]->totalnum; i < quadrant[2]->row; i++) {
		if (slot[i - 2] != -1 || players[slot[i - 2]].p_status != POUTFIT) {
		    W_ClearArea(playerw, 0, i, 41, 1);
		    slot[i - 2] = -1;
		}
	    }
	    for (i = 2 + quadrant[1]->totalnum; i < quadrant[3]->row; i++) {
		if (slot[16 + i - 2] != -1) {
		    W_ClearArea(playerw, 44, i, 41, 1);
		    slot[16 + i - 2] = -1;
		}
	    }
	}
    }
    /* update the display for each player */
    for (i = 0, j = &players[i]; i < nplayers; i++, j++) {

	if (!updatePlayer[i] && !sortPlayers && slot[i] == j->p_no)
	    continue;

	getdesig(j, desig);
	if (!desig[0]) {
	    if (!sortPlayers)
		W_ClearArea(playerw, (i > 15) ? 44 : 0, (i > 15) ? i - 16 + 2 : i + 2, 41, 1);
	    continue;		/* don't display this guy, he's toast */
	}
	rname = get_players_rank_name(j);
	(void) sprintf(buf, "%c%c %2.2s  %-9.9s %-15.15s",
		       teaminfo[j->p_teami].letter,
		       shipnos[j->p_no],
		       desig,
		       rname,
		       j->p_name);
	/*
	   replace 0.00 kills with spaces if not alive and paradise or not
	   RSA
	*/
	/* this is optional with the hideNoKills option 	                     */
	if ((j->p_kills <= 0 &&
	     (paradise || RSA_Client <= 0) && hideNoKills)
	    || (j->p_status & ~PALIVE))
	    strcat(buf, "      ");
	else
	    sprintf(buf + strlen(buf), "%6.2f", j->p_kills);
	if (!sortPlayers) {
	    W_WriteText(playerw, (i > 15) ? 44 : 0,
			(i > 15) ? i - 16 + 2 : i + 2, playerColor(j), buf,
			strlen(buf), shipFont(j));
	    slot[i] = j->p_no;
	} else if (!sequentialSort) {
	    /* find out which quadrant he should be in */
	    for (z = 0; j->p_teami != quadrant[z]->teamnum && z < 3; z++);
	    if (j->p_status == POUTFIT && sortOutfitting)
		k = quadrant[z]->outfitrow++ - quadrant[z]->row;
	    else
		k = 0;
	    if (quadrant[z]->row + k > 18) {
		z += 1;
		if (z > 3)
		    z -= 2;
	    }
	    currentSlot = (z % 2) * 16 + quadrant[z]->row - 2 + k;
	    if (slot[currentSlot] != j->p_no || updatePlayer[j->p_no] & SMALL_UPDATE) {
		slot[currentSlot] = j->p_no;
		W_WriteText(playerw, 44 * (z % 2), k + quadrant[z]->row,
			    playerColor(j), buf, strlen(buf), shipFont(j));
	    }
	    if (!k)
		quadrant[z]->row++;
	} else {		/* do sequential sort placement */
	    /* find out which quadrant he should be in */
	    for (z = 0; j->p_teami != quadrant[z]->teamnum && z < 3; z++);
	    currentSlot = quadrant[z]->row++;
	    if (slot[currentSlot] != j->p_no || updatePlayer[j->p_no] & SMALL_UPDATE) {
		if (currentSlot >= 16) {
		    W_WriteText(playerw, 44, currentSlot - 16 + 2, playerColor(j),
				buf, strlen(buf), shipFont(j));
		} else {
		    W_WriteText(playerw, 0, currentSlot + 2, playerColor(j), buf,
				strlen(buf), shipFont(j));
		}
	    }
	}

    updatePlayer[j->p_no] = NO_UPDATE;
  }
}

void
getdesig(struct player *j, char *desig)
{
    switch (j->p_status) {
    case PALIVE:
	strncpy(desig, j->p_ship->s_desig, 2);
        desig[2] = 0;
	break;
    case PTQUEUE:
	strcpy(desig, "tq");
	break;
    case POUTFIT:
	if ((j->p_teami < 0) || !showDead)
	    desig[0] = 0;
	else
	    strcpy(desig, "--");
	break;
    case PEXPLODE:
    case PDEAD:
	if (showDead)
	    strcpy(desig, "**");
	else
	    desig[0] = 0;
	break;
    case POBSERVE:
	strcpy(desig, "ob");
	break;
    default:
	desig[0] = 0;
	break;
    }
    /*
       if (!*desig) j->p_teami = -1;
    */
    if (j->p_teami == number_of_teams && !showPreLogins)
	desig[0] = 0;
}

static void
dofulllist(struct player *pptr, int vpos)
{
    char    buf[100];
    char   *rname;
    char    desig[3];
    struct ratings r;

    if (!W_IsMapped(playerw))
	return;

    getdesig(pptr, desig);
    if (!desig[0]) {
	if (pptr->p_no == blk_bozolist)
	    blk_bozolist = -1;
	W_ClearArea(playerw, 0, vpos, 83, 1);
	return;
    }
    get_ratings(pptr, &r);

    rname = get_players_rank_name(pptr);
    if (!paradise)
	sprintf(buf, "%c%c %2.2s  %-9.9s %-16.16s%5.2f %5d %5d %6.2f   %5.2f   %5.2f %8.2f  ",
		teaminfo[pptr->p_teami].letter,
		shipnos[pptr->p_no],
		desig /* pptr->p_ship->s_desig */ ,
		rname,
		pptr->p_name,
		pptr->p_kills,
		r.r_kills,
		r.r_losses,
		r.r_ratio,
		r.r_offrat,
		r.r_defrat,
		r.r_di);
    else
	sprintf(buf, "%c%c %2.2s  %-9.9s %-16.16s%5.2f %5d %5d %6.2f   %5.2f   %5.2f %8.2f  ",
		teaminfo[pptr->p_teami].letter,
		shipnos[pptr->p_no],
		desig /* pptr->p_ship->s_desig */ ,
		rname,
		pptr->p_name,
		pptr->p_kills,
		r.r_kills,
		r.r_losses,
		r.r_ratio,
		r.r_batrat,
		r.r_stratrat,
		r.r_di);

    W_WriteText(playerw, 0, vpos, playerColor(pptr), buf, strlen(buf),
		shipFont(pptr));
}

char *
get_players_rank_name(struct player *j)
{
    char   *r;
    if (j->p_stats.st_rank < 0)
	j->p_stats.st_rank = 0;
    if (!paradise) {
	r = ranks[j->p_stats.st_rank].name;
    } else {
	if (j->p_stats2.st_royal == 0)
	    r = ranks2[j->p_stats2.st_rank].name;
	else
	    r = royal[j->p_stats2.st_royal].name;
    }
    return (r);
}

void
playerwEvent(W_Event *data)
{
    int     key;
    struct obtype *target;

    key = doKeymap(data);
    if (key == -1)
	return;
    switch (key) {
    case 'l':			/* lock on [BDyess] */
	target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
	if(!target || players[target->o_num].p_flags & PFCLOAK) {
	  warning("Sorry sir, I can't get a lock on that vessel.");
	  break;
	}
	sendPlaylockReq(target->o_num);
	me->p_playerl = target->o_num;
	break;
    case 'i':
    case 'I':
	if (!infomapped)
	    inform(data->Window, data->x, data->y, key);
	else
	    destroyInfo();
	break;
    case 'X':
	macroState = 1;
	warning("Macro mode");
	break;
    default:
	warning("Invalid key");
    }
}

void
selectblkbozo(W_Event *data)
{
    int     width, slotnum;

    if (*playerList
	&& *playerList != ',')
	return;
    width = W_WindowWidth(data->Window);
    if (data->key == W_RBUTTON || data->key == W_LBUTTON || data->key == W_MBUTTON)
	if (data->y > W_Textheight * 2 + 1 && data->y < W_Textheight * 19) {
	    W_TranslatePoints(data->Window, &data->x, &data->y);
	    slotnum = 0;
	    if (data->x > width / 2)
		slotnum += 16;
	    /* only look for players in the slotspace */
	    if (data->y <= (16 + 1)) {
		slotnum += data->y - 2;
		if (slot[slotnum] != me->p_no)
		    blk_bozolist = slot[slotnum];
		if (blk_bozolist >= 0) {
		    updatePlayer[blk_bozolist] |= LARGE_UPDATE;
		} else {
		    blk_bozolist = -1;
		    W_ClearArea(playerw, 0, 22, 83, 1);
		}
	    }
	}
}
