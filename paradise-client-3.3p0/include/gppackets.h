#ifndef GPPACKETS_H
#define GPPACKETS_H

/* the definitions of {INT,CARD}{8,16,32} are in packets.h */
#include "packets.h"

struct gameparam_spacket {
    INT8    type;
    INT8    subtype;		/* this packet is not real */
    /* generic game parameter packet */
    INT16   pad;
};

struct gp_sizes_spacket {
    INT8    type;
    INT8    subtype;		/* =0 */

    CARD8   nplayers;
    CARD8   nteams;		/* max of 8 */

    CARD8   nshiptypes;
    CARD8   nranks;		/* number of ranks */
    CARD8   nroyal;		/* number of royalties */
    CARD8   nphasers;

    CARD8   ntorps;
    CARD8   nplasmas;
    CARD8   nthingies;		/* per-player */
    CARD8   gthingies;		/* auxiliary thingies */

    CARD32  gwidth;		/* galaxy width */
    /* 16 bytes */
    CARD32  flags;		/* some game parameter flags */
#define	GP_WRAPVERT	(1<<0)
#define	GP_WRAPHORIZ	(1<<1)
#define GP_WRAPALL	(GP_WRAPVERT|GP_WRAPHORIZ)

    /*
       The following bytes are room for growth. The size of this packet is
       unimportant because it only gets sent once.  hopefully we've got
       plenty of room.
    */
    INT32   ext1;		/* maybe more flags? */
    INT32   ext2;
    INT32   ext3;
    /* 32 bytes */

    INT32   ext4;
    INT32   ext5;
    INT32   ext6;
    INT32   ext7;

    INT32   ext8;
    INT32   ext9;
    INT32   ext10;
    INT32   ext11;		/* 16 ints, 64 bytes */
};

struct gp_team_spacket {
    INT8    type;
    INT8    subtype;		/* =1 */

    CARD8   index;		/* team index */
    char    letter;		/* team letter 'F' */

    char    shortname[3];	/* non-null-terminated 3-letter abbrev 'FED' */
    CARD8   pad;
    /* 8 bytes */
    char    teamname[56];	/* padding to 64 byte packet */
};

struct gp_teamlogo_spacket {
    /*
       This packet contains several adjacent rows of a team's logo bitmap
       Data is in raw XBM format (scanline-padded to 8 bits). Maximum bitmap
       size is 99x99, which takes 1287 (99x13) bytes.
    */
    INT8    type;
    INT8    subtype;		/* =2 */

    INT8    logowidth;		/* <= 99 */
    INT8    logoheight;		/* <= 99 */

    INT8    y;			/* y coord of the start of this packets info */
    INT8    thisheight;		/* the height of this packet's info */
    INT8    teamindex;		/* which team's logo this is */

    char    data[768 - 7];	/* pad packet to 768 bytes. */
};

struct gp_shipshape_spacket {
    INT8    type;
    INT8    subtype;		/* =3 */

    CARD8   shipno;
    INT8    race;		/* -1 is independent */
    CARD8   nviews;		/* typically 16 */

    CARD8   width, height;
    CARD8   pad1;
};

struct gp_shipbitmap_spacket {
    INT8    type;
    INT8    subtype;		/* =4 */

    CARD8   shipno;
    INT8    race;		/* -1 is independent */
    CARD8   thisview;		/* 0..nviews-1 */

    CARD8   bitmapdata[999];
};

struct gp_rank_spacket {
    INT8    type;
    INT8    subtype;		/* =5 */

    INT8    rankn;		/* rank number */

    char    name[-3 + 64 - 20];	/* name of the rank */

    INT32   genocides;
    INT32   milliDI;		/* DI*1000 */
    INT32   millibattle;	/* battle*1000 */
    INT32   millistrat;		/* strategy*1000 */
    INT32   millispec;		/* special ships*1000 */
};

struct gp_royal_spacket {
    INT8    type;
    INT8    subtype;		/* =6 */

    CARD8   rankn;		/* rank number */

    char    name[-3 + 64];	/* name of the rank */
};

struct gp_teamplanet_spacket {
    INT8    type;
    INT8    subtype;		/* =7 */

    INT8    teamn;		/* 0..7 */
    CARD8   pad1;

    INT32   ext1;		/* extensions? */

    /*
       Bitmaps of the team logo and masks.  The bitmap of the planet will be
       constructed with (mask ? logo : planet), applied bitwise. This
       calculation is equivalent to (logo&mask)|(planet&~mask)
    */

    /* bitmap 30x30, X bitmap format (scanline padded to 8 bits) */
    CARD8   tactical[120];
    CARD8   tacticalM[120];

    /* bitmap 16x16, X bitmap format (scanline padded to 8 bits) */
    CARD8   galactic[32];
    CARD8   galacticM[32];
};

#endif
