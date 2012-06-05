/* This file implements all SHORT_PACKETS functions */
/*  HW 19.07.93 */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "packets.h"
#include "gppackets.h"

#ifdef UNIX_SOUND
#include "sound.h"
#endif

char *
ship_type(struct ship *shp)
{
    static char ring[4][3];
    static int ri;
    char   *rval;
    ring[ri][0] = shp->s_desig[0];
    ring[ri][1] = shp->s_desig[1];
    ring[ri][2] = 0;
    rval = ring[ri];
    ri++;
    if (ri >= sizeof(ring) / sizeof(*ring))
	ri = 0;
    return rval;
}
#define TEAM_LETTER(p) ( teaminfo[(p).p_teami].letter )
#define TEAM_SHORT(p) ( teaminfo[(p).p_teami].shortname )
#define TEAM_LETTERP(pl) ( teaminfo[mask_to_idx((pl).pl_owner)].letter )
#define TEAM_SHORTP(pl) ( teaminfo[mask_to_idx((pl).pl_owner)].shortname )
#define reviewWin	messWin[WREVIEW].window
#define MAXTORP	ntorps
#define MAXPLASMA	nplasmas

/* from here on all SHORT_PACKETS */
#include "wtext.h"		/* here are all warningdefines */

/*      Here are all warnings that are send with SP_S_WARNING */
/*              HW 93           */

/* DaemonMessages */
char   *daemon_texts[] =
{
/* Game_Paused() */
    "Game is paused.  CONTINUE to continue.",	/* 0 */
    "Game is no-longer paused!",/* 1 */
    "Game is paused. Captains CONTINUE to continue.",	/* 2 */
    "Game will continue in 10 seconds",	/* 3 */
/* send_about_to_start() */
    "Teams chosen.  Game will start in 1 minute.",	/* 4 */
    "----------- Game will start in 1 minute -------------",	/* 5 */
};

/* VARITEXTE = warnings with 1 or more arguments argument */
char   *vari_texts[] =
{
/* redraw.c */
    "Engineering:  Energizing transporters in %d seconds",	/* 0 */
    "Stand By ... Self Destruct in %d seconds",	/* 1 */
    "Helmsman:  Docking manuever completed Captain.  All moorings secured at port %d.",	/* 2 */
/* interface.c from INL server */
    "Not constructed yet. %d minutes required for completion",	/* 3 */

};



char   *w_texts[] =
{
/* socket.c             */
    "Tractor beams haven't been invented yet.",	/* 0 */
    "Weapons's Officer:  Cannot tractor while cloaked, sir!",	/* 1 */
    "Weapon's Officer:  Vessel is out of range of our tractor beam.",	/* 2 */

/* handleRepressReq */
/****************       coup.c  ***********************/
/*      coup()  */
    "You must have one kill to throw a coup",	/* 3 */
    "You must orbit your home planet to throw a coup",	/* 4 */
    "You already own a planet!!!",	/* 5 */
    "You must orbit your home planet to throw a coup",	/* 6 */
    "Too many armies on planet to throw a coup",	/* 7 */
    "Planet not yet ready for a coup",	/* 8 */
/*      getentry.c              */
/*      getentry()              */
    "I cannot allow that.  Pick another team",	/* 9 */
    "Please confirm change of teams.  Select the new team again.",	/* 10 */
    "That is an illegal ship type.  Try again.",	/* 11 */
    "That ship hasn't beed designed yet.  Try again.",	/* 12 */
    "Your new starbase is still under construction",	/* 13 */
    "Your team is not capable of defending such an expensive ship!",	/* 14 */
    "Your team's stuggling economy cannot support such an expenditure!",	/* 15 */
    "Your side already has a starbase!",	/* 16 */
/*      plasma.c        */
/* nplasmatorp(course, type)    */
    "Plasmas haven't been invented yet.",	/* 17 */
    "Weapon's Officer:  Captain, this ship is not armed with plasma torpedoes!",	/* 18 */
    "Plasma torpedo launch tube has exceeded the maximum safe temperature!",	/* 19 */
    "Our fire control system limits us to 1 live torpedo at a time captain!",	/* 20 */
    "Our fire control system limits us to 1 live torpedo at a time captain!",	/* 21 */
    "We don't have enough fuel to fire a plasma torpedo!",	/* 22 */
    "We cannot fire while our vessel is undergoing repairs.",	/* 23 */
    "We are unable to fire while in cloak, captain!",	/* 24 */
/********       torp.c  *********/
/*      ntorp(course, type)     */
    "Torpedo launch tubes have exceeded maximum safe temperature!",	/* 25 */
    "Our computers limit us to having 8 live torpedos at a time captain!",	/* 26 */
    "We don't have enough fuel to fire photon torpedos!",	/* 27 */
    "We cannot fire while our vessel is in repair mode.",	/* 28 */
    "We are unable to fire while in cloak, captain!",	/* 29 */
    "We only have forward mounted cannons.",	/* 30 */
/*      phasers.c       */
/*      phaser(course) */
    "Weapons Officer:  This ship is not armed with phasers, captain!",	/* 31 */
    "Phasers have not recharged",	/* 32 */
    "Not enough fuel for phaser",	/* 33 */
    "Can't fire while repairing",	/* 34 */
    "Weapons overheated",	/* 35 */
    "Cannot fire while cloaked",/* 36 */
    "Phaser missed!!!",		/* 37 */
    "You destroyed the plasma torpedo!",	/* 38 */
/*      interface.c     */
/* bomb_planet()        */
    "Must be orbiting to bomb",	/* 39 */
    "Can't bomb your own armies.  Have you been reading Catch-22 again?",	/* 40 */
    "Must declare war first (no Pearl Harbor syndrome allowed here).",	/* 41 */
    "Bomb out of T-mode?  Please verify your order to bomb.",	/* 42 */
    "Hoser!",			/* 43 */
/*      beam_up()       */
    "Must be orbiting or docked to beam up.",	/* 44 */
    "Those aren't our men.",	/* 45 */
    "Comm Officer: We're not authorized to beam foriegn troops on board!",	/* 46 */
/* beam_down() */
    "Must be orbiting or docked to beam down.",	/* 47 */
    "Comm Officer: Starbase refuses permission to beam our troops over.",	/* 48 */
/*      declare_war(mask)       */
    "Pausing ten seconds to re-program battle computers.",	/* 49 */
/* do_refit(type) */
    "You must orbit your HOME planet to apply for command reassignment!",	/* 50 */
    "You must orbit your home planet to apply for command reassignment!",	/* 51 */
    "Can only refit to starbase on your home planet.",	/* 52 */
    "You must dock YOUR starbase to apply for command reassignment!",	/* 53 */
    "Must orbit home planet or dock your starbase to apply for command reassignment!",	/* 54 */
    "Central Command refuses to accept a ship in this condition!",	/* 55 */
    "You must beam your armies down before moving to your new ship",	/* 56 */
    "That ship hasn't been designed yet.",	/* 57 */
    "Your side already has a starbase!",	/* 58 */
    "Your team is not capable of defending such an expensive ship",	/* 59 */
    "Your new starbase is still under construction",	/* 60 */
    "Your team's stuggling economy cannot support such an expenditure!",	/* 61 */
    "You are being transported to your new vessel .... ",	/* 62 */
/* redraw.c */
/* auto_features()  */
    "Engineering:  Energize. [ SFX: chimes ]",	/* 63 */
    "Wait, you forgot your toothbrush!",	/* 64 */
    "Nothing like turning in a used ship for a new one.",	/* 65 */
    "First officer:  Oh no, not you again... we're doomed!",	/* 66 */
    "First officer:  Uh, I'd better run diagnostics on the escape pods.",	/* 67 */
    "Shipyard controller:  This time, *please* be more careful, okay?",	/* 68 */
    "Weapons officer:  Not again!  This is absurd...",	/* 69 */
    "Weapons officer:  ... the whole ship's computer is down?",	/* 70 */
    "Weapons officer:  Just to twiddle a few bits of the ship's memory?",	/* 71 */
    "Weapons officer:  Bah! [ bangs fist on inoperative console ]",	/* 72 */
    "First Officer:  Easy, big guy... it's just one of those mysterious",	/* 73 */
    "First Officer:  laws of the universe, like 'tires on the ether'.",	/* 74 */
    "First Officer:  laws of the universe, like 'Klingon bitmaps are ugly'.",	/* 75 */
    "First Officer:  laws of the universe, like 'all admirals have scummed'.",	/* 76 */
    "First Officer:  laws of the universe, like 'Mucus Pig exists'.",	/* 77 */
    "First Officer:  laws of the universe, like 'guests advance 5x faster'.",	/* 78 */
/* orbit.c */
/*orbit() */
    "Helmsman: Captain, the maximum safe speed for docking or orbiting is warp 2!",	/* 79 */
    "Central Command regulations prohibits you from orbiting foreign planets",	/* 80 */
    "Helmsman:  Sensors read no valid targets in range to dock or orbit sir!",	/* 81 */
/* redraw.c */
    "No more room on board for armies",	/* 82 */
    "You notice everyone on the bridge is staring at you.",	/* 83 */
/* startdaemon.c */
/* practice_robo() */
    "Can't send in practice robot with other players in the game.",	/* 84 */
/*      socket.c */
/*      doRead(asock) */
    "Self Destruct has been canceled",	/* 85 */
/* handleMessageReq(packet) */
    "Be quiet",			/* 86 */
    "You are censured.  Message was not sent.",	/* 87 */
    "You are ignoring that player.  Message was not sent.",	/* 88 */
    "That player is censured.  Message was not sent.",	/* 89 */
/* handleQuitReq(packet) */
    "Self destruct initiated",	/* 90 */
/* handleScan(packet) */
    "Scanners haven't been invented yet",	/* 91 */
/* handleUdpReq(packet) */
    "WARNING: BROKEN mode is enabled",	/* 92 */
    "Server can't do that UDP mode",	/* 93 */
    "Server will send with TCP only",	/* 94 */
    "Server will send with simple UDP",	/* 95 */
    "Request for fat UDP DENIED (set to simple)",	/* 96 */
    "Request for double UDP DENIED (set to simple)",	/* 97 */
/* forceUpdate() */
    "Update request DENIED (chill out!)",	/* 98 */
/* INL redraw.c */
    "Player lock lost while player dead.",	/* 99 */
    "Can only lock on own team.",	/* 100 */
    "You can only warp to your own team's planets!",	/* 101 */
    "Planet lock lost on change of ownership.",	/* 102 */
    " Weapons officer: Finally! systems are back online!",	/* 103 */

};

#define NUMWTEXTS (sizeof w_texts / sizeof w_texts[0])
#define NUMVARITEXTS ( sizeof vari_texts / sizeof   vari_texts[0])
#define NUMDAEMONTEXTS ( sizeof daemon_texts / sizeof daemon_texts[0])


void    add_whydead P((char *, int));
void    new_flags P((unsigned int, int));

int shortversion = SHORTVERSION;

extern int vtisize[];
extern unsigned char numofbits[];
int     my_x, my_y;		/* for rotation we need to keep track of our
				   real coordinates */
/* SP_S_WARNING vari texte */
char   *s_texte[256];		/* Better with a malloc scheme */
char    no_memory[] =
{"Not enough memory for warning string!"};

int     spwinside = 500;	/* WINSIDE from Server */
long    spgwidth = 100000;


void
sendThreshold(int v)
{
    struct threshold_cpacket p;

    p.type = CP_S_THRS;
    p.thresh = v;
    sendServerPacket((struct player_spacket *) & p);
}

void
handleVTorp(unsigned char *sbuf)
{
    unsigned char *which, *data;
    unsigned char bitset;
    struct torp *thetorp;
    int     dx, dy;
    int     shiftvar;

    int     i;
    register int shift = 0;	/* How many torps are extracted (for shifting
				   ) */

/* now we must find the data ... :-) */
    if (sbuf[0] == SP_S_8_TORP) {	/* MAX packet */
	bitset = 0xff;
	which = &sbuf[1];
	data = &sbuf[2];
    } else {			/* Normal Packet */
	bitset = sbuf[1];
	which = &sbuf[2];
	data = &sbuf[3];
    }

    thetorp = &torps[((unsigned char) *which * 8)];
    for (shift = 0, i = 0; i < 8;
         i++, thetorp++, bitset >>= 1) {
	if (bitset & 01) {
	    dx = (*data >> shift);
	    data++;
	    shiftvar = (unsigned char) *data;	/* to silence gcc */
	    shiftvar <<= (8 - shift);
	    dx |= (shiftvar & 511);
	    shift++;
	    dy = (*data >> shift);
	    data++;
	    shiftvar = (unsigned char) *data;	/* to silence gcc */
	    shiftvar <<= (8 - shift);
	    dy |= (shiftvar & 511);
	    shift++;
	    if (shift == 8) {
		shift = 0;
		data++;
	    }
/* This is necessary because TFREE/TMOVE is now encoded in the bitset */
	    if (thetorp->t_status == TFREE) {
		thetorp->t_status = TMOVE;	/* guess */
		players[thetorp->t_owner].p_ntorp++;
#ifdef UNIX_SOUND
            play_sound (SND_FIRETORP); /* Fire Torp */
#endif
	    } else if (thetorp->t_owner == me->p_no && thetorp->t_status == TEXPLODE) {
		thetorp->t_status = TMOVE;	/* guess */
	    }
	    /* Check if torp is visible */
	    if (dx > spwinside || dy > spwinside) {
/*	        printf("received invisible torp.\n");*/
		thetorp->t_x = -100000;	/* Not visible */
		thetorp->t_y = -100000;
	    } else {		/* visible */
/*	    thetorp->t_x = me->p_x + ((dx - spwinside / 2) * SCALE);
	    thetorp->t_y = me->p_y + ((dy - spwinside / 2) * SCALE); */
		thetorp->t_x = my_x + ((dx - spwinside / 2) * SCALE);
		thetorp->t_y = my_y + ((dy - spwinside / 2) * SCALE);
		if (rotate) {
		    rotate_coord(&thetorp->t_x, &thetorp->t_y, rotate_deg,
				 spgwidth / 2, spgwidth / 2);
		}
	    }
	}
	/* if */
	else {			/* We got a TFREE */
	    if (thetorp->t_status && thetorp->t_status != TEXPLODE) {
		players[thetorp->t_owner].p_ntorp--;
		thetorp->t_status = TFREE;	/* That's no guess */
	    }
	}
    }				/* for */
}

void
handleSelfShort(struct youshort_spacket *packet)
{
    me = (ghoststart ? &players[ghost_pno] : &players[packet->pnum]);
    myship = me->p_ship;
    mystats = &(me->p_stats);
    me->p_hostile = packet->hostile;
    me->p_swar = packet->swar;
    me->p_armies = packet->armies;
    me->p_flags = ntohl(packet->flags);
    me->p_whydead = packet->whydead;
    me->p_whodead = packet->whodead;
}

void
handleSelfShip(struct youss_spacket *packet)
{
    if (!me)
	return;			/* wait.. */

    me->p_damage = ntohs(packet->damage);
    me->p_shield = ntohs(packet->shield);
    me->p_fuel = ntohs(packet->fuel);
    me->p_etemp = ntohs(packet->etemp);
    me->p_wtemp = ntohs(packet->wtemp);
}

void
handleVPlayer(unsigned char *sbuf)
{
    register int x, y, i, numofplayers, pl_no, save;
    register struct player *pl;
    unsigned char newdir;	/* needed for uncloak kludge with ROTATERACE */
    int     offset;

    numofplayers = (unsigned char) sbuf[1] & 0x3f;

    if (sbuf[1] & (unsigned char) 128) {	/* Short Header + Extended */
	sbuf += 4;
	offset = 32;		/* more than 32 players? */
    }
    else if (sbuf[1] & 64) {	/* Short Header  */

        if (shortversion == SHORTVERSION) {
            if (sbuf[2] == 2) {
                int *tmp = (int *) &sbuf[4];

                new_flags(ntohl(*tmp), sbuf[3]);
                tmp++;
                new_flags(ntohl(*tmp), 0);
                sbuf += 8;

            }
            else if (sbuf[2] == 1) {
                int *tmp = (int *) &sbuf[4];

                new_flags(ntohl(*tmp), sbuf[3]);
                sbuf += 4;

            }
        }
	sbuf += 4;
	offset = 0;
    } else {
	struct player_s_spacket *packet = (struct player_s_spacket *) sbuf;
	pl = &players[me->p_no];
	pl->p_dir = (unsigned char) packet->dir;
	pl->p_speed = packet->speed;
	if (F_dead_warp && (pl->p_speed == 14)) {
	    if (pl->p_status == PALIVE) {
		pl->p_status = PEXPLODE;
		pl->p_explode = 0;
	    }
	}
        else {
            if (shortversion == SHORTVERSION) {

                struct player_s2_spacket *pa2=(struct player_s2_spacket *)sbuf;

                pl->p_x = my_x = SCALE * ntohs(pa2->x);
                pl->p_y = my_y = SCALE * ntohs(pa2->y);
                new_flags(ntohl(pa2->flags), 0);

            } else {
	        pl->p_x = my_x = ntohl(packet->x);
	        pl->p_y = my_y = ntohl(packet->y);

            }
	    if (rotate) {
		rotate_coord(&pl->p_x, &pl->p_y,
			     rotate_deg, spgwidth / 2, spgwidth / 2);
		rotate_dir(&pl->p_dir, rotate_deg);
	    }
	}
	redrawPlayer[me->p_no] = 1;
	sbuf += 12;		/* Now the small packets */
	offset = 0;
    }
    for (i = 0; i < numofplayers; i++) {
	pl_no = ((unsigned char) *sbuf & 0x1f) + offset;
	if (pl_no >= nplayers)
	    continue;
	save = (unsigned char) *sbuf;
	sbuf++;
	pl = &players[pl_no];
	pl->p_speed = (unsigned char) *sbuf & 15;	/* SPEED */
	newdir = (unsigned char) *sbuf & 0xf0;	/* realDIR */

	if (rotate)
	    rotate_dir(&newdir, rotate_deg);
	if (cloakerMaxWarp) {
	    if (pl->p_speed == 15)
		pl->p_flags |= PFCLOAK;
	    else
		pl->p_flags &= ~(PFCLOAK);
	}
	pl->p_dir = newdir;	/* realDIR */

	sbuf++;

        x = (unsigned char) *sbuf++;
	y = (unsigned char) *sbuf++;/* The lower 8 Bits are saved */

	/* Now we must preprocess the coordinates */
	if ((unsigned char) save & 64)
            x |= 256;
	if ((unsigned char) save & 128)
            y |= 256;
	/*
	   MAY CHANGE!  dead_warp is still an experiment, some servers are
	   also sending illegal co-ords for dead players, so we'll just
	   ignore 'em for now if the guy's dead.
	*/
	if (F_dead_warp && pl->p_speed == 14) {
	    if (pl->p_status == PALIVE) {
		pl->p_status = PEXPLODE;
		pl->p_explode = 0;
	    }
	    redrawPlayer[pl->p_no] = 1;
	    continue;
	}
	/* Now test if it's galactic or local coord */
	if (save & 32) {	/* It's galactic */
	    {
		pl->p_x = x * spgwidth / spwinside;
		pl->p_y = y * spgwidth / spwinside;
	    }
	} else {		/* Local */
	    pl->p_x = my_x + ((x - spwinside / 2) * SCALE);
	    pl->p_y = my_y + ((y - spwinside / 2) * SCALE);
	}
	redrawPlayer[pl->p_no] = 1;
	if (rotate) {
	    rotate_coord(&pl->p_x, &pl->p_y,
			 rotate_deg, spgwidth / 2, spgwidth / 2);
	}
    }				/* for */
}

void
handleSMessage(struct mesg_s_spacket *packet)
{
    char    buf[100];
    char    addrbuf[9];

    if (packet->m_from >= nplayers)
	packet->m_from = 255;

    if (packet->m_from == 255)
	strcpy(addrbuf, "GOD->");
    else {
	sprintf(addrbuf, " %c%c->", TEAM_LETTER(players[packet->m_from]),
		shipnos[players[packet->m_from].p_no]);
    }

    switch (packet->m_flags & (MTEAM | MINDIV | MALL)) {
    case MALL:
	sprintf(addrbuf + 5, "ALL");
	break;
    case MTEAM:
	sprintf(addrbuf + 5, TEAM_SHORT(*me));
	break;
    case MINDIV:
	/* I know that it's me -> xxx but i copied it straight ... */
	sprintf(addrbuf + 5, "%c%c ", TEAM_LETTER(players[packet->m_recpt]),
		shipnos[packet->m_recpt]);
	break;
    default:
	sprintf(addrbuf + 5, "ALL");
	break;
    }
    sprintf(buf, "%-9s%s", addrbuf, &packet->mesg);
    dmessage(buf, packet->m_flags, packet->m_from, packet->m_recpt);
}

void
handleShortReply(struct shortreply_spacket *packet)
{
    switch (packet->repl) {
    case SPK_VOFF:
        if (shortversion == SHORTVERSION && recv_short == 0) {
            printf("Using Short Packet Version 1.\n");
            shortversion = OLDSHORTVERSION;
            sendShortReq(SPK_VON);
        }
        else {
	    recv_short = 0;
	    sprefresh(SPK_VFIELD);
            sendUdpReq(COMM_UPDATE);
        }
	break;
    case SPK_VON:
	recv_short = 1;
	sprefresh(SPK_VFIELD);

        printf("Receiving Short Packet Version %d\n", shortversion);

	spwinside = (CARD16) ntohs(packet->winside);
	/*
	   bug in server-side SP code, wrong-endian machines confuse a client
	   with normal byte order
	*/
	if (spwinside == 0xf401)
	    spwinside = 0x01f4;

	if (paradise) {
	    spgwidth = (INT32) ntohl(packet->gwidth);
            blk_gwidth= spgwidth;
            blk_windgwidth = (float)spwinside;
            redrawall = 1;	/* force redrawing of galactic map */

	    /*
	       bug in server-side SP code, wrong-endian machines confuse a
	       client with normal byte order
	    */
	    if (spgwidth == 0xa0860100)
		spgwidth = 0x000186a0;
	    if (spgwidth == 0x0100)
		spgwidth = 0x000186a0;
	}
	break;
    case SPK_MOFF:
	recv_mesg = 0;
	sprefresh(SPK_MFIELD);
	W_SetSensitive(reviewWin, 0);
	break;
    case SPK_MON:
	recv_mesg = 1;
	sprefresh(SPK_MFIELD);
	W_SetSensitive(reviewWin, 1);
	break;
    case SPK_M_KILLS:
	recv_kmesg = 1;
	sprefresh(SPK_KFIELD);
	break;
    case SPK_M_NOKILLS:
	recv_kmesg = 0;
	sprefresh(SPK_KFIELD);
	break;
    case SPK_M_WARN:
	recv_warn = 1;
	sprefresh(SPK_WFIELD);
	break;
    case SPK_M_NOWARN:
	recv_warn = 0;
	sprefresh(SPK_WFIELD);
	break;

    case SPK_THRESHOLD:
	break;
    default:
	fprintf(stderr, "%s: unknown response packet value short-req: %d\n",
		"netrek", packet->repl);
    }
}

void
handleVTorpInfo(unsigned char *sbuf)
{
    unsigned char *bitset, *which, *data, *infobitset, *infodata;
    struct torp *thetorp;
    int     dx, dy;
    int     shiftvar;
    char    status1, war1;
    register int i;
    register int shift = 0;	/* How many torps are extracted (for shifting
				   ) */

/* now we must find the data ... :-) */
    bitset = &sbuf[1];
    which = &sbuf[2];
    infobitset = &sbuf[3];
/* Where is the data ? */
    data = &sbuf[4];
    infodata = &sbuf[vtisize[numofbits[(unsigned char) sbuf[1]]]];

    thetorp = &torps[(unsigned char) (*which * 8)];

    for (shift = 0, i = 0; i < 8;
	 thetorp++, *bitset >>= 1, *infobitset >>= 1, i++) {
	if (*bitset & 01) {
	    dx = (*data >> shift);
	    data++;
	    shiftvar = (unsigned char) *data;	/* to silence gcc */
	    shiftvar <<= (8 - shift);
	    dx |= (shiftvar & 511);
	    shift++;
	    dy = (*data >> shift);
	    data++;
	    shiftvar = (unsigned char) *data;	/* to silence gcc */
	    shiftvar <<= (8 - shift);
	    dy |= (shiftvar & 511);
	    shift++;
	    if (shift == 8) {
		shift = 0;
		data++;
	    }
	    /* Check for torp with no TorpInfo */
	    if (!(*infobitset & 01)) {
		if (thetorp->t_status == TFREE) {
		    thetorp->t_status = TMOVE;	/* guess */
		    players[thetorp->t_owner].p_ntorp++;
		} else if (thetorp->t_owner == me->p_no &&
			   thetorp->t_status == TEXPLODE) {	/* If TFREE got lost */
		    thetorp->t_status = TMOVE;	/* guess */
		}
	    }
	    /* Check if torp is visible */
	    if (dx > spwinside || dy > spwinside) {
/*	        printf("received invisible torp.\n");*/
		thetorp->t_x = -100000;	/* Not visible */
		thetorp->t_y = -100000;
	    } else {		/* visible */
/*	    thetorp->t_x = me->p_x + ((dx - spwinside / 2) * SCALE);
	    thetorp->t_y = me->p_y + ((dy - spwinside / 2) * SCALE);
*/
		thetorp->t_x = my_x + ((dx - spwinside / 2) *
				       SCALE);
		thetorp->t_y = my_y + ((dy - spwinside / 2) *
				       SCALE);

		if (rotate) {
		    rotate_coord(&thetorp->t_x, &thetorp->t_y, rotate_deg,
				 spgwidth / 2, spgwidth / 2);
		}
	    }
	}
	/* if */
	else {			/* Got a TFREE ? */
	    if (!(*infobitset & 01)) {	/* No other TorpInfo for this Torp */
		if (thetorp->t_status && thetorp->t_status != TEXPLODE) {
		    players[thetorp->t_owner].p_ntorp--;
		    thetorp->t_status = TFREE;	/* That's no guess */
		}
	    }
	}
	/* Now the TorpInfo */
	if (*infobitset & 01) {
	    war1 = (unsigned char) *infodata & 15 /* 0x0f */ ;
	    status1 = ((unsigned char) *infodata & 0xf0) >> 4;
	    infodata++;
	    if (status1 == TEXPLODE && thetorp->t_status == TFREE) {
		/* FAT: redundant explosion; don't update p_ntorp */
		continue;
	    }
	    if (thetorp->t_status == TFREE && status1) {
		players[thetorp->t_owner].p_ntorp++;
#ifdef UNIX_SOUND
                play_sound (SND_FIRETORP); /* Fire Torp */
#endif
	    }
	    if (thetorp->t_status && status1 == TFREE) {
		players[thetorp->t_owner].p_ntorp--;
	    }
	    thetorp->t_war = war1;
	    if (status1 != thetorp->t_status) {
		/* FAT: prevent explosion reset */
		thetorp->t_status = status1;
		if (thetorp->t_status == TEXPLODE) {
		    thetorp->t_fuse = 10000;
		}
	    }
	}			/* if */
    }				/* for */
}

void
handleVPlanet(unsigned char *sbuf)
{
    register int i;
    register int numofplanets;	/* How many Planets are in the packet */
    struct planet *plan;
    struct planet_s_spacket *packet = (struct planet_s_spacket *) & sbuf[2];
    /* FAT: prevent excessive redraw */
    int     redraw = 0;

    numofplanets = (unsigned char) sbuf[1];

    if (numofplanets > nplanets + 1)
	return;

    for (i = 0; i < numofplanets; i++, packet++) {
	if (packet->pnum < 0 || packet->pnum >= nplanets)
	    continue;

	plan = &planets[packet->pnum];
	if (plan->pl_owner != packet->owner)
	    redraw = 1;
	plan->pl_owner = packet->owner;

	if (plan->pl_info != packet->info)
	    redraw = 1;

	plan->pl_info = packet->info;
	/* Redraw the planet because it was updated by server */

	if (plan->pl_flags != (int) ntohs(packet->flags))
	    redraw = 1;
	plan->pl_flags = (int) ntohs(packet->flags);

	if (plan->pl_armies != (unsigned char) packet->armies) {
	    /*
	       don't redraw when armies change unless it crosses the '4' *
	       army limit. Keeps people from watching for planet 'flicker' *
	       when players are beaming
	    */
	    int     planetarmies = (unsigned char) packet->armies;
	    if ((plan->pl_armies < 5 && planetarmies > 4) ||
		(plan->pl_armies > 4 && planetarmies < 5))
		redraw = 1;
	}

	plan->pl_armies = (unsigned char) packet->armies;
	if (plan->pl_info == 0) {
	    plan->pl_owner = NOBODY;
	}

	if (redraw) {
	    plan->pl_flags |= PLREDRAW;
	    pl_update[packet->pnum].plu_update = 1;
	    pl_update[packet->pnum].plu_x = plan->pl_x;
	    pl_update[packet->pnum].plu_y = plan->pl_y;

	}
    }				/* FOR */
}

void
sendShortReq(int state)
{
    struct shortreq_cpacket shortReq;

    shortReq.type = CP_S_REQ;
    shortReq.req = state;
    shortReq.version = shortversion;
    switch (state) {
    case SPK_VON:
	warning("Sending short packet request");
	break;
    case SPK_VOFF:
	warning("Sending old packet request");
	break;
    default:
	break;
    }
    if ((state == SPK_SALL || state == SPK_ALL) && recv_short) {
	/* Let the client do the work, and not the network :-) */

	register int i;
	for (i = 0; i < nplayers * MAXTORP; i++)
	    torps[i].t_status = TFREE;

	for (i = 0; i < nplayers * MAXPLASMA; i++)
	    plasmatorps[i].pt_status = PTFREE;

	for (i = 0; i < nplayers; i++) {
	    players[i].p_ntorp = 0;
	    players[i].p_nplasmatorp = 0;
	    phasers[i].ph_status = PHFREE;
	}
	if (state == SPK_SALL)
	    warning("Sent request for small update (weapons+planets+kills)");
	else if (state == SPK_ALL)
	    warning("Sent request for medium update (all except stats)");
	else
	    warning("Sent some unknown request...");
    }
    sendServerPacket((struct player_spacket *) & shortReq);
}

char   *whydeadmess[] =
{"", "[quit]", "[photon]", "[phaser]", "[planet]", "[explosion]",
    "", "", "[ghostbust]", "[genocide]", "", "[plasma]", "[detted photon]", "[chain explosion]",
"[TEAM]", "", "[team det]", "[team explosion]"};


void
handleSWarning(struct warning_s_spacket *packet)
{
    char    buf[80];
    register struct player *target;
    register int damage;
    static int arg3, arg4;	/* Here are the arguments for warnings with
				   more than 2 arguments */
    static int karg3, karg4, karg5 = 0;
    char    killmess[20];

    switch (packet->whichmessage) {
    case TEXTE:		/* damage used as tmp var */
	damage = (unsigned char) packet->argument;
	damage |= (unsigned char) packet->argument2 << 8;
	if (damage >= 0 && damage < NUMWTEXTS)
	    warning(w_texts[damage]);
	break;
    case PHASER_HIT_TEXT:
	target = &players[(unsigned char) packet->argument & 0x3f];
	damage = (unsigned char) packet->argument2;
	if ((unsigned char) packet->argument & 64)
	    damage |= 256;
	if ((unsigned char) packet->argument & 128)
	    damage |= 512;
	(void) sprintf(buf, "Phaser burst hit %s for %d points", target->p_name, damage);
	warning(buf);
	break;
    case BOMB_INEFFECTIVE:
	sprintf(buf, "Weapons Officer: Bombing is ineffective.  Only %d armies are defending.",
		(int) packet->argument);	/* nifty info feature 2/14/92
						   TMC */
	warning(buf);
	break;
    case BOMB_TEXT:
	sprintf(buf, "Weapons Officer: Bombarding %s...  Sensors read %d armies left.",
		planets[(unsigned char) packet->argument].pl_name,
		(unsigned char) packet->argument2);
	warning(buf);
	break;
    case BEAMUP_TEXT:
	sprintf(buf, "%s: Too few armies to beam up",
		planets[(unsigned char) packet->argument].pl_name);
	warning(buf);
	break;
    case BEAMUP2_TEXT:
	sprintf(buf, "Beaming up. (%d/%d)", (unsigned char) packet->argument, (unsigned char) packet->argument2);
	warning(buf);
	break;
    case BEAMUPSTARBASE_TEXT:
	sprintf(buf, "Starbase %s: Too few armies to beam up",
		players[packet->argument].p_name);
	warning(buf);
	break;
    case BEAMDOWNSTARBASE_TEXT:
	sprintf(buf, "No more armies to beam down to Starbase %s.",
		players[packet->argument].p_name);
	warning(buf);

	break;
    case BEAMDOWNPLANET_TEXT:
	sprintf(buf, "No more armies to beam down to %s.",
		planets[(unsigned char) packet->argument].pl_name);
	warning(buf);
	break;
    case SBREPORT:
	sprintf(buf, "Transporter Room:  Starbase %s reports all troop bunkers are full!",
		players[packet->argument].p_name);
	warning(buf);
	break;
    case ONEARG_TEXT:
	if (packet->argument >= 0 && packet->argument < NUMVARITEXTS) {
	    sprintf(buf, vari_texts[packet->argument], (unsigned char) packet->argument2);
	    warning(buf);
	}
	break;
    case BEAM_D_PLANET_TEXT:
	sprintf(buf, "Beaming down.  (%d/%d) %s has %d armies left",
		arg3,
		arg4,
		planets[(unsigned char) packet->argument].pl_name,
		packet->argument2);
	warning(buf);
	break;
    case ARGUMENTS:
	arg3 = (unsigned char) packet->argument;
	arg4 = (unsigned char) packet->argument2;
	break;
    case BEAM_U_TEXT:
	sprintf(buf, "Transfering ground units.  (%d/%d) Starbase %s has %d armies left",
		(unsigned char) arg3, (unsigned char) arg4, players[packet->argument].p_name, (unsigned char) packet->argument2);
	warning(buf);
	break;
    case LOCKPLANET_TEXT:
	sprintf(buf, "Locking onto %s", planets[(unsigned char) packet->argument].pl_name);
	warning(buf);
	break;
    case SBRANK_TEXT:
	sprintf(buf, "You need a rank of %s or higher to command a starbase!", ranks[packet->argument].name);
	warning(buf);
	break;
    case SBDOCKREFUSE_TEXT:
	sprintf(buf, "Starbase %s refusing us docking permission captain.",
		players[packet->argument].p_name);
	warning(buf);
	break;
    case SBDOCKDENIED_TEXT:
	sprintf(buf, "Starbase %s: Permission to dock denied, all ports currently occupied.", players[packet->argument].p_name);
	warning(buf);
	break;
    case SBLOCKSTRANGER:
	sprintf(buf, "Locking onto %s (%c%c)",
		players[packet->argument].p_name,
		TEAM_LETTER(players[packet->argument]),
		shipnos[players[packet->argument].p_no]);
	warning(buf);
	break;
    case SBLOCKMYTEAM:
	sprintf(buf, "Locking onto %s (%c%c) (docking is %s)",
		players[packet->argument].p_name,
		TEAM_LETTER(players[packet->argument]),
		shipnos[players[packet->argument].p_no],
		(players[packet->argument].p_flags & PFDOCKOK) ? "enabled" : "disabled");
	warning(buf);
	break;
    case DMKILL:
	{
	    struct mesg_spacket msg;
	    int     killer, victim, armies;
	    float   kills;
	    victim = (unsigned char) packet->argument & 0x3f;
	    killer = (unsigned char) packet->argument2 & 0x3f;
	    /* that's only a temp */
	    damage = (unsigned char) karg3;
	    damage |= (karg4 & 127) << 8;
	    kills = damage / 100.0;
	    if (kills == 0.0)
		strcpy(killmess, "NO CREDIT");
	    else
		sprintf(killmess, "%0.2f", kills);
	    armies = (((unsigned char) packet->argument >> 6) | ((unsigned char) packet->argument2 & 192) >> 4);
	    if (karg4 & 128)
		armies |= 16;
	    if (armies == 0) {
		(void) sprintf(msg.mesg, "%s%s (%c%c) [%c%c] was kill %s for %s (%c%c) [%c%c]",
			       (godToAllOnKills ? "GOD->ALL " : ""),
			       players[victim].p_name,
			       TEAM_LETTER(players[victim]),
			       shipnos[victim],
			       players[victim].p_ship->s_desig[0],
			       players[victim].p_ship->s_desig[1],
			       killmess,
			       players[killer].p_name,
			       TEAM_LETTER(players[killer]),
			       shipnos[killer],
			       players[killer].p_ship->s_desig[0],
			       players[killer].p_ship->s_desig[1]);
		msg.m_flags = MALL | MVALID | MKILL;
	    } else {
		(void) sprintf(msg.mesg, "%s%s (%c%c+%d armies) [%c%c]: kill %s for %s (%c%c) [%c%c]",
			       (godToAllOnKills ? "GOD->ALL " : ""),
			       players[victim].p_name,
			       TEAM_LETTER(players[victim]),
			       shipnos[victim],
			       armies,
			       players[victim].p_ship->s_desig[0],
			       players[victim].p_ship->s_desig[1],
			       killmess,
			       players[killer].p_name,
			       TEAM_LETTER(players[killer]),
			       shipnos[killer],
			       players[killer].p_ship->s_desig[0],
			       players[killer].p_ship->s_desig[1]);
		msg.m_flags = MALL | MVALID | MKILLA;
	    }
	    if (why_dead) {
		add_whydead(msg.mesg, karg5);
		karg5 = 0;
	    }
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_recpt = 0;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case KILLARGS:
	karg3 = (unsigned char) packet->argument;
	karg4 = (unsigned char) packet->argument2;
	break;
    case KILLARGS2:
	karg5 = (unsigned char) packet->argument;
	break;
    case DMKILLP:
	{
	    struct mesg_spacket msg;
	    (void) sprintf(msg.mesg, "%s%s (%c%c) [%c%c] killed by %s (%c)",
			   (godToAllOnKills ? "GOD->ALL " : ""),
			   players[packet->argument].p_name,
			   TEAM_LETTER(players[packet->argument]),
			   shipnos[packet->argument],
			   players[packet->argument].p_ship->s_desig[0],
			   players[packet->argument].p_ship->s_desig[1],
			 planets[(unsigned char) packet->argument2].pl_name,
		  TEAM_LETTERP(planets[(unsigned char) packet->argument2]));
	    if (why_dead) {
		add_whydead(msg.mesg, karg5);
		karg5 = 0;
	    }
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_flags = MALL | MVALID | MKILLP;
	    msg.m_recpt = 0;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case DMBOMB:
	{
	    struct mesg_spacket msg;
	    char    buf1[80];
	    (void) sprintf(buf, "%-3s->%-3s", planets[(unsigned char) packet->argument2].pl_name, TEAM_SHORTP(planets[(unsigned char) packet->argument2]));
	    (void) sprintf(buf1, "We are being attacked by %s %c%c who is %d%% damaged.",
			   players[packet->argument].p_name,
			   TEAM_LETTER(players[packet->argument]),
			   shipnos[packet->argument],
			   arg3);
	    (void) sprintf(msg.mesg, "%s %s", buf, buf1);
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_flags = MTEAM | MBOMB | MVALID;
	    msg.m_recpt = planets[(unsigned char) packet->argument2].pl_owner;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case DMDEST:
	{
	    struct mesg_spacket msg;
	    char    buf1[80];
	    (void) sprintf(buf, "%s destroyed by %s (%c%c)",
			   planets[(unsigned char) packet->argument].pl_name,
			   players[packet->argument2].p_name,
			   TEAM_LETTER(players[packet->argument2]),
			   shipnos[(unsigned char) packet->argument2]);
	    (void) sprintf(buf1, "%-3s->%-3s",
			   planets[(unsigned char) packet->argument].pl_name, TEAM_SHORTP(planets[(unsigned char) packet->argument]));
	    (void) sprintf(msg.mesg, "%s %s", buf1, buf);
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_flags = MTEAM | MDEST | MVALID;
	    msg.m_recpt = planets[(unsigned char) packet->argument].pl_owner;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case DMTAKE:
	{
	    struct mesg_spacket msg;
	    char    buf1[80];
	    (void) sprintf(buf, "%s taken over by %s (%c%c)",
			   planets[(unsigned char) packet->argument].pl_name,
			   players[packet->argument2].p_name,
			   TEAM_LETTER(players[packet->argument2]),
			   shipnos[packet->argument2]);
	    (void) sprintf(buf1, "%-3s->%-3s",
			   planets[(unsigned char) packet->argument].pl_name, TEAM_SHORT(players[packet->argument2]));
	    (void) sprintf(msg.mesg, "%s %s", buf1, buf);
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_flags = MTEAM | MTAKE | MVALID;
	    msg.m_recpt = idx_to_mask(players[packet->argument2].p_teami);
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case DGHOSTKILL:
	{
	    struct mesg_spacket msg;
	    ushort  dam;
	    dam = (unsigned char) karg3;
	    dam |= (unsigned char) (karg4 & 0xff) << 8;
	    (void) sprintf(msg.mesg, "GOD->ALL %s (%c%c) was kill %0.2f for the GhostBusters",
			   players[(unsigned char) packet->argument].p_name, TEAM_LETTER(players[(unsigned char) packet->argument]),
			   shipnos[(unsigned char) packet->argument],
			   (float) dam / 100.0);
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_flags = MALL | MVALID;
	    msg.m_recpt = 0;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
/* INL Daemon Mesages */
    case INLDMKILLP:
	{
	    struct mesg_spacket msg;
	    *buf = 0;
	    if (arg3) {		/* Armies */
		sprintf(buf, "+%d", arg3);
	    }
	    (void) sprintf(msg.mesg, "%s%s[%s] (%c%c%s) killed by %s (%c)",
			   (godToAllOnKills ? "GOD->ALL " : ""),
			   players[(unsigned char) packet->argument].p_name,
			   ship_type(players[(unsigned char) packet->argument].p_ship),
			   TEAM_LETTER(players[(unsigned char) packet->argument]),
			   shipnos[(unsigned char) packet->argument],
			   buf,
			 planets[(unsigned char) packet->argument2].pl_name,
		  TEAM_LETTERP(planets[(unsigned char) packet->argument2]));
	    if (why_dead) {
		add_whydead(msg.mesg, karg5);
		karg5 = 0;
	    }
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_flags = MALL | MVALID | MKILLP;
	    msg.m_recpt = 0;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case INLDMKILL:
	{
	    struct mesg_spacket msg;
	    int     killer, victim, armies;
	    float   kills;
	    victim = (unsigned char) packet->argument & 0x3f;
	    killer = (unsigned char) packet->argument2 & 0x3f;
	    /* that's only a temp */
	    damage = (unsigned char) karg3;
	    damage |= (karg4 & 127) << 8;
	    kills = damage / 100.0;
	    armies = (((unsigned char) packet->argument >> 6) | ((unsigned char) packet->argument2 & 192) >> 4);
	    if (karg4 & 128)
		armies |= 16;
	    if (armies == 0) {
		(void) sprintf(msg.mesg, "%s%s[%s] (%c%c) was kill %0.2f for %s[%s] (%c%c)",
			       (godToAllOnKills ? "GOD->ALL " : ""),
			       players[victim].p_name,
			       ship_type(players[victim].p_ship),
			       TEAM_LETTER(players[victim]),
			       shipnos[victim],
			       kills,
			       players[killer].p_name,
			       ship_type(players[killer].p_ship),
			       TEAM_LETTER(players[killer]),
			       shipnos[killer]);
		msg.m_flags = MALL | MVALID | MKILL;
	    } else {
		(void) sprintf(msg.mesg, "%s%s[%s] (%c%c+%d armies) was kill %0.2f for %s[%s] (%c%c)",
			       (godToAllOnKills ? "GOD->ALL " : ""),
			       players[victim].p_name,
			       ship_type(players[victim].p_ship),
			       TEAM_LETTER(players[victim]),
			       shipnos[victim],
			       armies,
			       kills,
			       players[killer].p_name,
			       ship_type(players[killer].p_ship),
			       TEAM_LETTER(players[killer]),
			       shipnos[killer]);
		msg.m_flags = MALL | MVALID | MKILLA;
	    }
	    if (why_dead) {
		add_whydead(msg.mesg, karg5);
		karg5 = 0;
	    }
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_recpt = 0;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case INLDRESUME:
	{
	    struct mesg_spacket msg;
	    sprintf(msg.mesg, " Game will resume in %d seconds", (unsigned char) packet->argument);
	    msg.m_flags = MALL | MVALID;
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_recpt = 0;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case INLDTEXTE:
	if (packet->argument >= 0 && (unsigned char) packet->argument < NUMDAEMONTEXTS) {
	    struct mesg_spacket msg;
	    strcpy(msg.mesg, daemon_texts[(unsigned char) packet->argument]);
	    msg.m_flags = MALL | MVALID;
	    msg.type = SP_MESSAGE;
	    msg.mesg[79] = '\0';
	    msg.m_recpt = 0;
	    msg.m_from = 255;
	    handleMessage(&msg);
	}
	break;
    case STEXTE:
	warning(s_texte[(unsigned char) packet->argument]);
	break;
    case SHORT_WARNING:
	{
	    struct warning_spacket *warn = (struct warning_spacket *) packet;
	    warning(warn->mesg);
	}
	break;
    case STEXTE_STRING:
	{
	    struct warning_spacket *warn = (struct warning_spacket *) packet;
	    warning(warn->mesg);
	    s_texte[(unsigned char) warn->pad2] = (char *) 
	    				     malloc(warn->pad3 - 4);
	    if (s_texte[(unsigned char) warn->pad2] == NULL) {
		s_texte[(unsigned char) warn->pad2] = no_memory;
		warning("Could not add warning! (No memory!)");
	    } else
		strcpy(s_texte[(unsigned char) warn->pad2], warn->mesg);
	}
	break;
    default:
	warning("Unknown Short Warning!");
	break;
    }
}

#define MY_SIZEOF(a) (sizeof(a) / sizeof(*(a)))

void
add_whydead(char *s, int m)		/* 7/22/93 LAB */
{
    char    b[256];

    if (m < MY_SIZEOF(whydeadmess)) {
	sprintf(b, "%-50s %s", s, whydeadmess[m]);
	b[79] = '\0';
	strcpy(s, b);
    }
}

void handleVKills(unsigned char *sbuf)
{
  int i, numofkills, pnum;
  unsigned short pkills;
  unsigned char *data = &sbuf[2];

  numofkills = (unsigned char) sbuf[1];

  for (i = 0; i < numofkills; i++)
    {
      pkills = (unsigned short) *data++;
      pkills |= (unsigned short) ((*data & 0x03) << 8);
      pnum = (unsigned char) *data++ >> 2;

      if (pnum < 0 || pnum >= nplayers)
        {
          fprintf(stderr, "handleKills: bad index %d\n", pnum);
          return;
        }

      if (players[pnum].p_kills != ((float) pkills / 100.0))
        {
          players[pnum].p_kills = pkills / 100.0;
          /* FAT: prevent redundant player update */
          updatePlayer[pnum] |= ALL_UPDATE;

#ifdef ARMY_SLIDER
          if (me == &players[(int) pnum])
            {
              calibrate_stats();
              redrawStats();
            }
#endif /* ARMY_SLIDER */
        }

    }                                            /* for */

}                                                /* handleVKills */

void handleVPhaser(unsigned char *sbuf)
{
  struct phaser *phas;
  struct player *j;
  struct phaser_s_spacket *packet = (struct phaser_s_spacket *) &sbuf[0];

  /* not nice but.. */
  register int pnum, status, target, x, y, dir;

  status = packet->status & 0x0f;
  pnum = packet->pnum & 0x3f;

  switch (status)
    {
    case PHFREE:
      break;
    case PHHIT:
      target = (unsigned char) packet->target & 0x3f;
      break;
    case PHMISS:
      dir = (unsigned char) packet->target;
      break;
    case PHHIT2:
      x = SCALE * (ntohs(packet->x));
      y = SCALE * (ntohs(packet->y));
      target = packet->target & 0x3f;
      break;
    default:
      x = SCALE * (ntohs(packet->x));
      y = SCALE * (ntohs(packet->y));
      target = packet->target & 0x3f;
      dir = (unsigned char) packet->dir;
      break;
    }
  phas = &phasers[pnum];
  phas->ph_status = status;
  phas->ph_dir = dir;
  phas->ph_x = x;
  phas->ph_y = y;
  phas->ph_target = target;
  phas->ph_fuse = 0;

  if (rotate)
    {
      rotate_coord(&phas->ph_x, &phas->ph_y, rotate_deg, GWIDTH / 2, GWIDTH / 2)
;
      rotate_dir(&phas->ph_dir, rotate_deg);
    }
}

void handle_s_Stats(struct stats_s_spacket *packet)
{
  struct player *pl;

  if (packet->pnum < 0 || packet->pnum >= nplayers)
    {
      fprintf(stderr, "handleStats: bad index %d\n", packet->pnum);
      return;
    }

  pl = &players[packet->pnum];

  pl->p_stats.st_tkills = ntohs(packet->tkills);
  pl->p_stats.st_tlosses = ntohs(packet->tlosses);
  pl->p_stats.st_kills = ntohs(packet->kills);
  pl->p_stats.st_losses = ntohs(packet->losses);
  pl->p_stats.st_tticks = ntohl(packet->tticks);
  pl->p_stats.st_tplanets = ntohs(packet->tplanets);
  pl->p_stats.st_tarmsbomb = ntohl(packet->tarmies);
  pl->p_stats.st_sbkills = ntohs(packet->sbkills);
  pl->p_stats.st_sblosses = ntohs(packet->sblosses);
  pl->p_stats.st_armsbomb = ntohs(packet->armies);
  pl->p_stats.st_planets = ntohs(packet->planets);
  if ((pl->p_ship->s_type == STARBASE))
    {
      pl->p_stats.st_sbticks = ntohl(packet->maxkills);
    }
  else
    {
      pl->p_stats.st_maxkills = ntohl(packet->maxkills) / 100.0;
    }
  pl->p_stats.st_sbmaxkills = ntohl(packet->sbmaxkills) / 100.0;

  updatePlayer[packet->pnum] |= LARGE_UPDATE;
}

void    new_flags(unsigned int data, int which)
{
  int pnum, status;
  unsigned int new, tmp;
  unsigned int oldflags;
  struct player *j;

  tmp = data;
  for (pnum = which * 16; pnum < (which + 1) * 16 && pnum < nplayers; pnum++)
    {
      new = tmp & 0x03;
      tmp >>= 2;
      j = &players[pnum];

      oldflags = j->p_flags;
      switch (new)
        {
        case 0:                          /* PDEAD/PEXPLODE */
          status = PEXPLODE;
          j->p_flags &= ~PFCLOAK;
          break;
        case 1:                          /* PALIVE & PFCLOAK */
          status = PALIVE;
          j->p_flags |= PFCLOAK;
          break;
        case 2:                          /* PALIVE & PFSHIELD */
          status = PALIVE;
          j->p_flags |= PFSHIELD;
          j->p_flags &= ~PFCLOAK;
          break;
        case 3:                          /* PALIVE & NO shields */
          status = PALIVE;
          j->p_flags &= ~(PFSHIELD | PFCLOAK);
          break;
        default:
          break;
        }

      if (oldflags != j->p_flags)
        redrawPlayer[pnum] = 1;

      if (j->p_status == status)
        continue;

      if (status == PEXPLODE)
        {
          if (j->p_status == PALIVE)
            {
              j->p_explode = 0;
              j->p_status = status;
            }
          else                                   /* Do nothing */
            continue;
        }
      else
        {                                        /* really PALIVE ? */
          if (j == me)
            {
              /* Wait for POUTFIT */
              if (j->p_status == POUTFIT || j->p_status == PFREE)
                {
                  if (j->p_status != PFREE)
                    j->p_kills = 0.;

                  j->p_status = PALIVE;
                }
            }
          else
            {
              if (j->p_status != PFREE)
                j->p_kills = 0.;

              j->p_status = status;
            }
        }
      redrawPlayer[pnum] = 1;
      updatePlayer[pnum] |= ALL_UPDATE;
    }                                            /* for */
}
