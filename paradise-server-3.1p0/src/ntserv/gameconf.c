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
#include "gppackets.h"

/* some game params packet stuff */
/* status experimental */

static void
updateGPrank(void)
{
    int     i;
    struct gp_rank_spacket pkt;

    pkt.type = SP_GPARAM;
    pkt.subtype = 5;

    for (i = 0; i < NUMRANKS; i++) {
	pkt.rankn = i;
	pkt.genocides = htonl(ranks[i].genocides);
#define COMPUTE_STATS(a, b) (unsigned long)((long double)(a) * (long double)(b))
	pkt.milliDI = htonl(COMPUTE_STATS( 1000, ranks[i].di));
	pkt.millibattle = htonl(COMPUTE_STATS( 1000, ranks[i].battle));
	pkt.millistrat = htonl(COMPUTE_STATS(1000, ranks[i].strategy));
	pkt.millispec = htonl(COMPUTE_STATS(1000, ranks[i].specship));
#undef COMPUTE_STATS
	strcpy((char *) pkt.name, (char *) ranks[i].name);

	sendClientPacket((struct player_spacket *) & pkt);
    }
}

static void
updateGProyal(void)
{
    int     i;
    struct gp_royal_spacket pkt;

    pkt.type = SP_GPARAM;
    pkt.subtype = 6;

    for (i = 0; i < NUMROYALRANKS; i++) {
	pkt.rankn = i;
	strcpy((char *) pkt.name, royal[i].name);
	sendClientPacket((struct player_spacket *) & pkt);
    }
}

static void
updateGPteams(void)
{
    struct gp_team_spacket pkt;
    int     i;

    for (i = 0; i < NUMTEAM; i++) {
	pkt.type = SP_GPARAM;
	pkt.subtype = 1;

	pkt.index = i;
	pkt.letter = teams[idx_to_mask(i)].letter;
	strncpy((char *) pkt.shortname, teams[idx_to_mask(i)].shortname,
		sizeof(pkt.shortname));
	strncpy((char *) pkt.teamname, teams[idx_to_mask(i)].name, sizeof(pkt.teamname));

	sendClientPacket((struct player_spacket *) & pkt);
    }
}

static void
send_one_teamlogo(int teamidx, unsigned char *data, int width, int height)
{
    struct gp_teamlogo_spacket pkt;
    int     pwidth;

    if (width > 99 || height > 99) {
	printf("logo too big: %dx%d\n", width, height);
	return;
    }

    pkt.type = SP_GPARAM;
    pkt.subtype = 2;

    pkt.logowidth = width;
    pkt.logoheight = height;
    pkt.teamindex = teamidx;

    pwidth = (width - 1) / 8 + 1;
    pkt.thisheight = sizeof(pkt.data) / pwidth;

    for (pkt.y = 0; pkt.y < height; pkt.y += pkt.thisheight) {

	if (pkt.y + pkt.thisheight > height)
	    pkt.thisheight = height - pkt.y;

	memcpy(pkt.data, data + pkt.y * pwidth, pkt.thisheight * pwidth);
	sendClientPacket((struct player_spacket *) & pkt);
    }
}

static void
updateGPteamlogos(void)
{
    char    buf[40];
    unsigned char   *data;
    int     width, height;
    int     i;

    for (i = 0; i < NUMTEAM; i++) {
	sprintf(buf, "artwork/%d/logo", i);
	if (status2->league == 1) {
	    if (i == 0)
		sprintf(buf, "artwork/%s/logo", status2->home.name);
	    else if (i == 1)
		sprintf(buf, "artwork/%s/logo", status2->away.name);
	}
	else if (status2->league) {
	    if (i == status2->home.index)
		sprintf(buf, "artwork/%s/logo", status2->home.name);
	    else if (i == status2->away.index)
		sprintf(buf, "artwork/%s/logo", status2->away.name);
	}
	{
	    FILE   *fp;
	    fp = fopen(build_path(buf), "r");
	    if (!fp)
		continue;	/* no image to transmit */
	    ParseXbmFile(fp, &width, &height, &data);
	    fclose(fp);
	}
	if (!data) {
	    continue;
	}
	send_one_teamlogo(i, data, width, height);
	free(data);
    }
}

static void
updateGPshipshapes(void)
{
#if 0
    {
	struct gp_shipshape_spacket pkt;

	pkt.type = SP_GPARAM;
	pkt.subtype = 3;

	pkt.shipno = ATT;
	pkt.race = -1;
	pkt.nviews = 1;
	pkt.width = borgcube_width;
	pkt.height = borgcube_height;

	sendClientPacket((struct player_spacket *) & pkt);
    }

    {
	struct gp_shipbitmap_spacket pkt;

	pkt.type = SP_GPARAM;
	pkt.subtype = 4;

	pkt.shipno = ATT;
	pkt.race = -1;
	pkt.thisview = 0;

	memcpy(pkt.bitmapdata, borgcube_bits, sizeof(borgcube_bits));

	sendClientPacket((struct player_spacket *) & pkt);
    }
#endif
}

static void
updateGPplanetbitmaps(void)
{
    struct gp_teamplanet_spacket pkt;
    char    buf[40];
    unsigned char   *data;
    int     width, height;
    int     i;

    for (i = 0; i < NUMTEAM; i++) {

	pkt.type = SP_GPARAM;
	pkt.subtype = 7;

	pkt.teamn = i;

	sprintf(buf, "artwork/%d/tplanet", i);
	if (status2->league == 1) {
	    if (i == 0)
		sprintf(buf, "artwork/%s/tplanet", status2->home.name);
	    else if (i == 1)
		sprintf(buf, "artwork/%s/tplanet", status2->away.name);
	}
	else if (status2->league) {
	    if (i == status2->home.index)
		sprintf(buf, "artwork/%s/tplanet", status2->home.name);
	    else if (i == status2->away.index)
		sprintf(buf, "artwork/%s/tplanet", status2->away.name);
	}
	{
	    FILE   *fp;
	    fp = fopen(build_path(buf), "r");
	    if (!fp)
		continue;	/* no image to transmit */
	    ParseXbmFile(fp, &width, &height, &data);
	    fclose(fp);
	}
	if (!data) {
	    continue;
	}
	memcpy(pkt.tactical, data, 120);
	memcpy(pkt.tacticalM, data, 120);
	free(data);

	sprintf(buf, "artwork/%d/gplanet", i);
	if (status2->league == 1) {
	    if (i == 0)
		sprintf(buf, "artwork/%s/gplanet", status2->home.name);
	    else if (i == 1)
		sprintf(buf, "artwork/%s/gplanet", status2->away.name);
	}
	else if (status2->league) {
	    if (i == status2->home.index)
		sprintf(buf, "artwork/%s/gplanet", status2->home.name);
	    else if (i == status2->away.index)
		sprintf(buf, "artwork/%s/gplanet", status2->away.name);
	}
	{
	    FILE   *fp;
	    fp = fopen(build_path(buf), "r");
	    if (!fp)
		continue;	/* no image to transmit */
	    ParseXbmFile(fp, &width, &height, &data);
	    fclose(fp);
	}
	if (!data) {
	    continue;
	}
	memcpy(pkt.galactic, data, 32);
	memcpy(pkt.galacticM, data, 32);
	free(data);

	sendClientPacket((struct player_spacket *) & pkt);
    }

}

void
updateGameparams(void)
{
    struct gp_sizes_spacket pkt;

    memset((char *) &pkt, 0, sizeof(pkt));

    pkt.type = SP_GPARAM;
    pkt.subtype = 0;

    pkt.nplayers = MAXPLAYER;
    pkt.nteams = 4;
    pkt.nshiptypes = NUM_TYPES;
    pkt.nranks = NUMRANKS;
    pkt.nroyal = NUMROYALRANKS;
    pkt.nphasers = 1;
    pkt.ntorps = MAXTORP;
    pkt.nplasmas = MAXPLASMA;
    pkt.nthingies = NPTHINGIES;
    pkt.gthingies = NGTHINGIES;
    pkt.gwidth = GWIDTH;
    pkt.flags = 0;
    pkt.nplanets = configvals->numplanets;
    sendClientPacket((struct player_spacket *) & pkt);

    updateGPteams();
    if (me == 0 || !(me->p_stats.st_flags & ST_NOBITMAPS)) {
	updateGPteamlogos();
	updateGPshipshapes();
	updateGPplanetbitmaps();
    }
    updateGPrank();
    updateGProyal();
}

/* end game params stuff */
