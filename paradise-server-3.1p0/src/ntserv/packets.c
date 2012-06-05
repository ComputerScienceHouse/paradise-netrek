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

#include "config.h"
#include "proto.h"
#include "ntserv.h"
#include "gppackets.h"
#include "packets.h"
#include "wtext.h"

static size_t client_packet_sizes[] = {
  0, 
  sizeof(struct mesg_cpacket),
  sizeof(struct speed_cpacket),
  sizeof(struct dir_cpacket),
  sizeof(struct phaser_cpacket),
  sizeof(struct plasma_cpacket),
  sizeof(struct torp_cpacket),
  sizeof(struct quit_cpacket),
  sizeof(struct login_cpacket),
  sizeof(struct outfit_cpacket),
  /* 10 v */
  sizeof(struct war_cpacket),
  sizeof(struct practr_cpacket),
  sizeof(struct shield_cpacket),
  sizeof(struct repair_cpacket),
  sizeof(struct orbit_cpacket),
  sizeof(struct planlock_cpacket),
  sizeof(struct playlock_cpacket),
  sizeof(struct bomb_cpacket),
  sizeof(struct beam_cpacket),
  sizeof(struct cloak_cpacket),
  /* 20 v */
  sizeof(struct det_torps_cpacket),
  sizeof(struct det_mytorp_cpacket),
  sizeof(struct copilot_cpacket),
  sizeof(struct refit_cpacket),
  sizeof(struct tractor_cpacket),
  sizeof(struct repress_cpacket),
  sizeof(struct coup_cpacket),
  sizeof(struct socket_cpacket),
  sizeof(struct options_cpacket),
  sizeof(struct bye_cpacket),
  /* 30 v */
  sizeof(struct dockperm_cpacket),
  sizeof(struct updates_cpacket),
  sizeof(struct resetstats_cpacket),
  sizeof(struct reserved_cpacket),
  sizeof(struct scan_cpacket),
  sizeof(struct udp_req_cpacket),
  sizeof(struct sequence_cpacket),
  sizeof(struct rsa_key_cpacket),
  sizeof(struct obvious_packet),
  0,
  /* 40 v */
  0,
  0,
  sizeof(struct ping_cpacket),
  sizeof(struct shortreq_cpacket),
  sizeof(struct threshold_cpacket),
  0,				/* CP_S_MESSAGE */
  0,				/* CP_S_RESERVED */
  0,				/* CP_S_DUMMY */
  0,
  0,
  /* 50 v */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  /* 60 v */
  sizeof( struct feature_cpacket )	/* CP_FEATURE */
};

#define num_cpacket_sizes	sizeof(client_packet_sizes)/sizeof(*client_packet_sizes)

int
size_of_cpacket(void *pkt)
{
    CARD8	type;
    CARD8	subtype;

    type = ((CARD8*)pkt)[0];
    subtype = ((CARD8*)pkt)[1];

    if (type < num_cpacket_sizes && client_packet_sizes[type]>0)
      return client_packet_sizes[type];

    switch (type) {
    case CP_S_MESSAGE:
	return ((unsigned char*)pkt)[3];
    case CP_S_RESERVED:
    case CP_S_DUMMY:
	/* hmm? good question */
	return 0;

    default:
	return 0;
   }
}


static int server_packet_sizes[] = {
    0,				/* record 0 */
    sizeof(struct mesg_spacket),/* SP_MESSAGE */
    sizeof(struct plyr_info_spacket),	/* SP_PLAYER_INFO */
    sizeof(struct kills_spacket),	/* SP_KILLS */
    sizeof(struct player_spacket),	/* SP_PLAYER */
    sizeof(struct torp_info_spacket),	/* SP_TORP_INFO */
    sizeof(struct torp_spacket),/* SP_TORP */
    sizeof(struct phaser_spacket),	/* SP_PHASER */
    sizeof(struct plasma_info_spacket),	/* SP_PLASMA_INFO */
    sizeof(struct plasma_spacket),	/* SP_PLASMA */
    /* 10 v */
    sizeof(struct warning_spacket),	/* SP_WARNING */
    sizeof(struct motd_spacket),/* SP_MOTD */
    sizeof(struct you_spacket),	/* SP_YOU */
    sizeof(struct queue_spacket),	/* SP_QUEUE */
    sizeof(struct status_spacket),	/* SP_STATUS */
    sizeof(struct planet_spacket),	/* SP_PLANET */
    sizeof(struct pickok_spacket),	/* SP_PICKOK */
    sizeof(struct login_spacket),	/* SP_LOGIN */
    sizeof(struct flags_spacket),	/* SP_FLAGS */
    sizeof(struct mask_spacket),/* SP_MASK */
    /* 20 v */
    sizeof(struct pstatus_spacket),	/* SP_PSTATUS */
    sizeof(struct badversion_spacket),	/* SP_BADVERSION */
    sizeof(struct hostile_spacket),	/* SP_HOSTILE */
    sizeof(struct stats_spacket),	/* SP_STATS */
    sizeof(struct plyr_login_spacket),	/* SP_PL_LOGIN */
    sizeof(struct reserved_spacket),	/* SP_RESERVED */
    sizeof(struct planet_loc_spacket),	/* SP_PLANET_LOC */
    sizeof(struct scan_spacket),/* SP_SCAN (ATM) */
    sizeof(struct udp_reply_spacket),	/* SP_UDP_REPLY */
    sizeof(struct sequence_spacket),	/* SP_SEQUENCE */
    /* 30 v */
    sizeof(struct sc_sequence_spacket),	/* SP_SC_SEQUENCE */
    sizeof(struct rsa_key_spacket),	/* SP_RSA_KEY */
    sizeof(struct motd_pic_spacket),	/* SP_MOTD_PIC */
    sizeof(struct stats_spacket2),	/* SP_STATS2 */
    sizeof(struct status_spacket2),	/* SP_STATUS2 */
    sizeof(struct planet_spacket2),	/* SP_PLANET2 */
    sizeof(struct obvious_packet),	/* SP_NEW_MOTD */
    sizeof(struct thingy_spacket),	/* SP_THINGY */
    sizeof(struct thingy_info_spacket),	/* SP_THINGY_INFO */
    sizeof(struct ship_cap_spacket),	/* SP_SHIP_CAP */
    /* 40 v */
    sizeof(struct shortreply_spacket),	/* SP_S_REPLY */
    -1,					/* SP_S_MESSAGE */
    -1,					/* SP_S_WARNING */
    sizeof(struct youshort_spacket),	/* SP_S_YOU */
    sizeof(struct youss_spacket),	/* SP_S_YOU_SS */
    -1,					/* SP_S_PLAYER */
    sizeof(struct ping_spacket),	/* SP_PING */
    -1,					/* SP_S_TORP */
    -1,					/* SP_S_TORP_INFO */
    20,					/* SP_S_8_TORP */
    /* 50 v */
    -1,					/* SP_S_PLANET */
    -1,					/* SP_GPARAM */
    -1,					/* SP_PARADISE_EXT1 */
    sizeof(struct terrain_packet2),     /* SP_TERRAIN2 */
    sizeof(struct terrain_info_packet2), /* SP_TERRAIN_INFO2 */
    -1,
    -1,
    -1,
    -1,
    -1,
    /* 60 v */
    sizeof(struct feature_spacket), 	/* SP_FEATURE */
    -1				/* The fools!  I'm pretty sure there is an */
                                /* off-by-one in the test in the last fnc */
                                /* but I'm not sure and this WILL fix it. */
};

#define num_spacket_sizes (sizeof(server_packet_sizes) / sizeof(server_packet_sizes[0]) - 1)

static unsigned char numofbits[256] =
{0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4,
 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

static int vtsize[9] =
{4, 8, 8, 12, 12, 16, 20, 20, 24};	/* How big is the torppacket */

static int vtisize[9] =
{4, 7, 9, 11, 13, 16, 18, 20, 22};	/* 4 byte Header + torpdata */


static int
padto4(int sz)
{
    return (sz%4) ? (sz/4+1)*4 : sz;
}

int 
size_of_spacket(unsigned char *pkt)
{
  switch(pkt[0]) {
    case SP_GPARAM:
	switch (pkt[1]) {
	case 0:
	    return sizeof(struct gp_sizes_spacket);
	case 1:
	    return sizeof(struct gp_team_spacket);
	case 2:
	    return sizeof(struct gp_teamlogo_spacket);
	case 3:
	    return sizeof(struct gp_shipshape_spacket);
	case 4:
	    return sizeof(struct gp_shipbitmap_spacket);
	case 5:
	    return sizeof(struct gp_rank_spacket);
	case 6:
	    return sizeof(struct gp_royal_spacket);
	case 7:
	    return sizeof(struct gp_teamplanet_spacket);
	default:
	    return 0;
	}
    case SP_S_MESSAGE:
	return padto4(pkt[4]); /* IMPORTANT  Changed */
    case SP_S_WARNING:
	if (pkt[1] == STEXTE_STRING ||
	    pkt[1] == SHORT_WARNING) {
	    return padto4(pkt[3]);
	} else
	    return 4;	/* Normal Packet */
    case SP_S_PLAYER:
	if (pkt[1] & 128) {	/* Small +extended Header */
	  return padto4(((pkt[1] & 0x3f) * 4) + 4);
	} else if (pkt[1] & 64) {			/* Small Header */
	    return padto4(((pkt[1] & 0x3f) * 4) + 4);
	} else {		/* Big Header */
	    return padto4((pkt[1] * 4 + 12));
	}
    case SP_S_TORP:
	return padto4(vtsize[numofbits[pkt[1]]]);
    case SP_S_TORP_INFO:
	return padto4((vtisize[numofbits[pkt[1]]] + numofbits[pkt[3]]));
    case SP_S_PLANET:
	return padto4((pkt[1] * VPLANET_SIZE) + 2);
    case SP_PARADISE_EXT1:
	switch (pkt[1]) {
	case SP_PE1_MISSING_BITMAP:
	    return sizeof(struct pe1_missing_bitmap_spacket);
	case SP_PE1_NUM_MISSILES:
	    return sizeof(struct pe1_num_missiles_spacket);
	default:
	    return 0;
	}
    default:
	return (*pkt< num_spacket_sizes && server_packet_sizes[*pkt] >= 0)
	    ? server_packet_sizes[*pkt] : 0;
    }
}
