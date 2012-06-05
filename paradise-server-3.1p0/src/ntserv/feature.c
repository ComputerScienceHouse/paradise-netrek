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

/*
 * Feature.c
 *
 * March, 1994.    Joe Rumsey, Tedd Hadley
 *
 * most of the functions needed to handle SP_FEATURE/CP_FEATURE
 * packets.  fill in the features list below for your client, and
 * add a call to reportFeatures just before the RSA response is sent.
 * handleFeature should just call checkFeature, which will search the
 * list and set the appropriate variable.  features unknown to the
 * server are set to the desired value for client features, and off
 * for server/client features.
 *
 * feature packets look like:
struct feature_cpacket {
   char                 type;
   char                 feature_type;
   char                 arg1,
                        arg2;
   int                  value;
   char                 name[80];
};

 *  type is CP_FEATURE, which is 60.  feature_spacket is identical.
 *
 *  Code lifted July 1995 by Bob Glamm - enabled into server code.
 *
 */

/* not the actual packets: this holds a list of features to report for */
/* this client. */
static struct feature {
    char   *name;
    int    *var;		/* holds allowed/enabled status */
    char    feature_type;	/* 'S' or 'C' for server/client */
    int     value;		/* desired status */
    char   *arg1, *arg2;	/* where to copy args, if non-null */
} features[] = {
    /*
       also sent seperately, put here for checking later. should be ok that
       it's sent twice.
    */
    {"FEATURE_PACKETS", &F_feature_packets, 'S', 1, 0, 0},

    {"VIEW_BOX", &F_allowViewBox, 'C', 1, 0, 0},
    {"SHOW_ALL_TRACTORS", &F_allowShowAllTractorPressor, 'S', 1, 0, 0},
#ifdef CONTINUOUS_MOUSE
    {"CONTINUOUS_MOUSE", &F_allowContinuousMouse, 'C', 1, 0, 0},
#endif
    {"NEWMACRO", &F_UseNewMacro, 'C', 1, 0, 0},
    /* {"SMARTMACRO",&F_UseSmartMacro, 'C', 1, 0, 0}, */
    {"MULTIMACROS", &F_multiline_enabled, 'S', 1, 0, 0},
    {"WHY_DEAD", &F_why_dead, 'S', 1, 0, 0},
    {"CLOAK_MAXWARP", &F_cloakerMaxWarp, 'S', 1, 0, 0},
/*{"DEAD_WARP", &F_dead_warp, 'S', 1, 0, 0},*/
    {"RC_DISTRESS", &F_gen_distress, 'S', 1, 0, 0},
#ifdef BEEPLITE
    {"BEEPLITE", &F_allow_beeplite, 'C', 1, (char*)&F_beeplite_flags, 0},
#endif
/* terrain features */
    {"TERRAIN", &F_terrain, 'S', 1, (char*)&F_terrain_major, (char*)&F_terrain_minor},
/* Gzipped MOTD */
    {"GZ_MOTD", &F_gz_motd, 'S', 1, (char*)&F_gz_motd_major, (char*)&F_gz_motd_minor},
/* armies in shipcap packet */
    {"ARMIES_IN_SHIPCAP", &F_armies_shipcap, 'S', 1, 0, 0},
    {0, 0, 0, 0, 0, 0}
};

void handleFeature(struct feature_cpacket *packet)
{
  int feature = 0;
/*
  int value = ntohl( packet->value );
*/

  while( features[feature].name ){
    if( !strcmp( features[feature].name, packet->name ) ){
      /* A match was found. */
      if( features[feature].var )
        *features[feature].var = packet->value;
      if( features[feature].arg1 )
        *features[feature].arg1 = packet->arg1;
      if( features[feature].arg2 )
        *features[feature].arg2 = packet->arg2;

      /*
       * tell the client that the server handles all of the above
       * server features.
       */
      if( features[feature].feature_type == 'S' )	{
        sendFeature( features[feature].name,
		     features[feature].feature_type,
                     features[feature].value,
                     (features[feature].arg1 ? *features[feature].arg1 : 0),
                     (features[feature].arg2 ? *features[feature].arg2 : 0) );
      }
    }
    feature++;
  }
}
    
void
sendFeature(char *name, int feature_type, int value, int arg1, int arg2)
{
    struct feature_cpacket packet;

    strncpy(packet.name, name, sizeof(packet.name));
    packet.type = SP_FEATURE;
    packet.name[sizeof(packet.name) - 1] = 0;
    packet.feature_type = feature_type;
    packet.value = htonl(value);
    packet.arg1 = arg1;
    packet.arg2 = arg2;
    sendClientPacket((struct player_spacket *) & packet);
}
