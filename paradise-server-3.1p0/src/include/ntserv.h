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

#ifndef NTSERV_H
#define NTSERV_H

#include "config.h"
#include "defs.h"
#include "struct.h"
#include "packets.h"

/* *************************************************************************
   from ntserv/
   ************************************************************************* */

/* ntserv/cluecheck.c */
void init_motdbuf P((char *));
void countdown_clue P((void));
int accept_cluecheck P((char *));

/* ntserv/death.c */
void compute_ratings P((struct player *, struct rating *));
void death P((void));

/* ntserv/fatudp.c */
void reset_fat_list P((void));
void updateFat P((struct player_spacket *));
int fatten P((void));
void fatMerge P((void));

/* ntserv/feature.c */
void handleFeature P((struct feature_cpacket *));
void sendFeature P((char *, int, int, int, int));

/* ntserv/findslot.c */
int findslot P((int, enum HomeAway));

/* ntserv/gameconf.c */
void updateGameparams P((void));

/* ntserv/getentry.c */
void detourneyqueue P((void));
void getEntry P((int *, int *));
int realNumShips P((int)); /* duplicated in daemon code? */

/* ntserv/getname.c */
void getname P((void));
void savestats P((void));

/* ntserv/input.c */
void setflag P((void));
void input P((void));
int reconnect P((void));

/* ntserv/main.c */
void stop_interruptor P((void));
void start_interruptor P((void));
void exitGame P((void));
void sendMotd P((void));

/* ntserv/message.c */
int parse_command_mess P((char *, unsigned char ));

/* ntserv/missile.c */
void fire_missile_dir P((unsigned char));

/* ntserv/packets.c */
int size_of_cpacket P((void *));
int size_of_spacket P((unsigned char *));

/* ntserv/parsexbm.c */
int ParseXbmFile P((FILE *, int *, int *, unsigned char **));

/* ntserv/ping.c */
void pingResponse P((struct ping_cpacket *packet));
void sendClientPing P((void));

/* ntserv/plasma.c */
void nplasmatorp P((unsigned char, int));

/* ntserv/redraw.c */
void intrupt P((void));
void auto_features P((void));

/* ntserv/reserved.c */
void makeReservedPacket P((struct reserved_spacket *));
void encryptReservedPacket P((struct reserved_spacket *, struct reserved_cpacket *, char *, int));

/* ntserv/rsa-server.c */
void makeRSAPacket P((struct rsa_key_spacket *packet));
int  decryptRSAPacket P((struct rsa_key_spacket *, struct rsa_key_cpacket *, char *));

/* ntserv/socket.c */
int connectToClient P((char *, int));
void checkSocket P((void));
void initClientData P((void));
int isClientDead P((void));
void updateClient P((void));
void briefUpdateClient P((void));
void updateStatus P((void));
void updateSelf P((void));
void updateShips P((void));
void updatePlanets P((void));
void updateTerrain P((void));
void updateMOTD P((void));
void sendQueuePacket P((short int));
void sendClientPacket P((struct player_spacket *));
int readFromClient P((void));
void sendPickokPacket P((int));
void sendMotdLine P((char *));
void sendMaskPacket P((int));
int checkVersion P((void));
void logEntry P((void));
void logmessage P((char *));
int connUdpConn P((void));
int closeUdpConn P((void));
void printUdpInfo P((void));
#ifdef DOUBLE_UDP
void sendSC P((void));
#endif
void sendShipCap P((void));
void sendMotdPic P((int, int, char *, int, int, int));
void sendMotdNopic P((int, int, int, int, int));
void sendMissileNum P((int));
int site_rsa_exempt P((void));
void sendClientLogin P((struct stats *));

/* ntserv/sockio.c */
int buffersEmpty P((void));
void resetUDPbuffer P((void));
void resetUDPsequence P((void));
void flushSockBuf P((void));
void build_select_masks P((fd_set *, fd_set *));
int socketPause P((void));
int socketWait P((void));
int gwrite P((int, char *, int));
void sendUDPbuffered P((int, void *, int));
void sendTCPbuffered P((void *, int));
void sendTCPdeferred P((void *, int));
void flushDeferred P((void));
void undeferDeferred P((void));

/* ntserv/timecheck.c */
void load_time_access P((void));
int time_access P((void));

#endif
