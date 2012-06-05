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
#include "structdesc.h"

#define OFFSET_OF(field)	( (char*)(&((struct ship*)0)->field) -\
 			  (char*)0)

#define DEF_LF(prefix,name) #name, prefix##name
/* char *name, long bitvalue, char *help */
static struct longflags_desc all_ship_flag_names[] =
{
    { DEF_LF(SFN, UNDOCKABLE),   "cannot dock with another ship" },
    { DEF_LF(SFN, CANORBIT),     "can orbit hostile worlds" },
    { DEF_LF(SFN, CANWARP),      "has warp engines" },
    { DEF_LF(SFN, CANFUEL),      "can transfer fuel to docked ships" },
    { DEF_LF(SFN, CANREPAIR),    "can speed repair of docked ships" },
    { DEF_LF(SFN, CANREFIT),     "can let docked ships refit" },
    { DEF_LF(SFN, ARMYNEEDKILL), "needs kills to carry armies" },
    { DEF_LF(SFN, HASPHASERS),   "is armed with phasers" },
    { DEF_LF(SFN, PLASMASTYLE),  "360 arc of fire for plasmas" },
    { DEF_LF(SFN, MASSPRODUCED), "is mass produced" },
    { DEF_LF(SFN, PLASMAARMED),  "is armed with plasmas by default" },
    { DEF_LF(SFN, HASMISSILE),   "is armed with missiles by default" },
    { DEF_LF(SFN, HASFIGHTERS),  "has a fighter bay" },
    { 0, 0, 0 }
};

static struct longflags all_ship_longflags =
{
  "SFN",
  all_ship_flag_names,
};

static struct longflags_desc ship_bombflag_names[] = 
{
  { DEF_LF(SBOMB_, NONE),       "cannot bomb anything" },
  { DEF_LF(SBOMB_, ARMIES),     "can bomb armies" },
  { DEF_LF(SBOMB_, FUEL),       "can bomb fueling facility" },
  { DEF_LF(SBOMB_, AGRI),       "can bomb AGRI facility" },
  { DEF_LF(SBOMB_, REPAIR),     "can bomb repair facility" },
  { DEF_LF(SBOMB_, SHIPYARD),   "can bomb shipyard" },
  { DEF_LF(SBOMB_, FACILITIES), "can bomb all facilities" },
  { DEF_LF(SBOMB_, ALL),        "can bomb everything" },
  { 0, 0, 0 }
};

static struct longflags ship_bombflag_longflags =
{
  "SBOMB_",
  ship_bombflag_names
};

struct field_desc ship_fields[] = {
    {"alttype", FT_SHORT, OFFSET_OF(s_alttype)},
    {"name", FT_STRING, OFFSET_OF(s_name[0])},

    {"turns", FT_INT, OFFSET_OF(s_turns)},

    {"imp.acc", FT_INT, OFFSET_OF(s_imp.acc)},
    {"imp.dec", FT_INT, OFFSET_OF(s_imp.dec)},
    {"imp.cost", FT_INT, OFFSET_OF(s_imp.cost)},
    {"imp.maxspeed", FT_INT, OFFSET_OF(s_imp.maxspeed)},
    {"imp.etemp", FT_INT, OFFSET_OF(s_imp.etemp)},

    {"after.acc", FT_INT, OFFSET_OF(s_after.acc)},
    {"after.dec", FT_INT, OFFSET_OF(s_after.dec)},
    {"after.cost", FT_INT, OFFSET_OF(s_after.cost)},
    {"after.maxspeed", FT_INT, OFFSET_OF(s_after.maxspeed)},
    {"after.etemp", FT_INT, OFFSET_OF(s_after.etemp)},

    {"warp.acc", FT_INT, OFFSET_OF(s_warp.acc)},
    {"warp.dec", FT_INT, OFFSET_OF(s_warp.dec)},
    {"warp.cost", FT_INT, OFFSET_OF(s_warp.cost)},
    {"warp.maxspeed", FT_INT, OFFSET_OF(s_warp.maxspeed)},
    {"warp.etemp", FT_INT, OFFSET_OF(s_warp.etemp)},

    {"warpinitcost", FT_INT, OFFSET_OF(s_warpinitcost)},
    {"warpinittime", FT_INT, OFFSET_OF(s_warpinittime)},
    {"warpprepspeed", FT_INT, OFFSET_OF(s_warpprepspeed)},

    {"mass", FT_SHORT, OFFSET_OF(s_mass)},

    {"tractstr", FT_SHORT, OFFSET_OF(s_tractstr)},
    {"tractrng", FT_FLOAT, OFFSET_OF(s_tractrng)},
    {"tractcost", FT_INT, OFFSET_OF(s_tractcost)},
    {"tractetemp", FT_INT, OFFSET_OF(s_tractetemp)},

    {"torp.damage", FT_SHORT, OFFSET_OF(s_torp.damage)},
    {"torp.speed", FT_SHORT, OFFSET_OF(s_torp.speed)},
    {"torp.cost", FT_SHORT, OFFSET_OF(s_torp.cost)},
    {"torp.fuse", FT_SHORT, OFFSET_OF(s_torp.fuse)},
    {"torp.wtemp", FT_SHORT, OFFSET_OF(s_torp.wtemp)},
    {"torp.wtemp_halfarc", FT_SHORT, OFFSET_OF(s_torp.wtemp_halfarc)},
    {"torp.wtemp_factor", FT_SHORT, OFFSET_OF(s_torp.wtemp_factor)},
    {"torpturns", FT_SHORT, OFFSET_OF(s_torp.aux)},	/* name anomaly */

    {"phaser.damage", FT_SHORT, OFFSET_OF(s_phaser.damage)},
    {"phaser.range", FT_SHORT, OFFSET_OF(s_phaser.speed)},	/* name anomaly */
    {"phaser.cost", FT_SHORT, OFFSET_OF(s_phaser.cost)},
    {"phaser.fuse", FT_SHORT, OFFSET_OF(s_phaser.fuse)},
    {"phaser.wtemp", FT_SHORT, OFFSET_OF(s_phaser.wtemp)},

    {"missile.damage", FT_SHORT, OFFSET_OF(s_missile.damage)},
    {"missile.speed", FT_SHORT, OFFSET_OF(s_missile.speed)},
    {"missile.cost", FT_SHORT, OFFSET_OF(s_missile.cost)},
    {"missile.fuse", FT_SHORT, OFFSET_OF(s_missile.fuse)},
    {"missile.wtemp", FT_SHORT, OFFSET_OF(s_missile.wtemp)},
    {"missile.count", FT_SHORT, OFFSET_OF(s_missile.count)},
    {"missileturns", FT_SHORT, OFFSET_OF(s_missile.aux)},	/* name anomaly */
    {"missilestored", FT_SHORT, OFFSET_OF(s_missilestored)},

    {"plasma.damage", FT_SHORT, OFFSET_OF(s_plasma.damage)},
    {"plasma.speed", FT_SHORT, OFFSET_OF(s_plasma.speed)},
    {"plasma.cost", FT_SHORT, OFFSET_OF(s_plasma.cost)},
    {"plasma.fuse", FT_SHORT, OFFSET_OF(s_plasma.fuse)},
    {"plasma.wtemp", FT_SHORT, OFFSET_OF(s_plasma.wtemp)},
    {"plasmaturns", FT_SHORT, OFFSET_OF(s_plasma.aux)},	/* name anomaly */

    {"maxwpntemp", FT_INT, OFFSET_OF(s_maxwpntemp)},
    {"wpncoolrate", FT_SHORT, OFFSET_OF(s_wpncoolrate)},

    {"maxegntemp", FT_INT, OFFSET_OF(s_maxegntemp)},
    {"egncoolrate", FT_SHORT, OFFSET_OF(s_egncoolrate)},

    {"maxfuel", FT_INT, OFFSET_OF(s_maxfuel)},
    {"recharge", FT_SHORT, OFFSET_OF(s_recharge)},
    {"mingivefuel", FT_INT, OFFSET_OF(s_mingivefuel)},
    {"takeonfuel", FT_INT, OFFSET_OF(s_takeonfuel)},

    {"expldam", FT_SHORT, OFFSET_OF(s_expldam)},
    {"fueldam", FT_SHORT, OFFSET_OF(s_fueldam)},

    {"armyperkill", FT_FLOAT, OFFSET_OF(s_armyperkill)},
    {"maxarmies", FT_SHORT, OFFSET_OF(s_maxarmies)},
    {"bomb", FT_INT, OFFSET_OF(s_bomb)},
    {"bombflags", FT_LONGFLAGS, OFFSET_OF(s_bombflags), (void *) &ship_bombflag_longflags},

    {"repair", FT_SHORT, OFFSET_OF(s_repair)},
    {"maxdamage", FT_INT, OFFSET_OF(s_maxdamage)},
    {"maxshield", FT_INT, OFFSET_OF(s_maxshield)},
    {"shieldcost", FT_INT, OFFSET_OF(s_shieldcost)},

    {"detcost", FT_SHORT, OFFSET_OF(s_detcost)},
    {"detdist", FT_INT, OFFSET_OF(s_detdist)},
    {"cloakcost", FT_SHORT, OFFSET_OF(s_cloakcost)},

    {"scanrange", FT_SHORT, OFFSET_OF(s_scanrange)},

    {"numports", FT_SHORT, OFFSET_OF(s_numports)},

    {"letter", FT_CHAR, OFFSET_OF(s_letter)},
    {"desig1", FT_CHAR, OFFSET_OF(s_desig1)},
    {"desig2", FT_CHAR, OFFSET_OF(s_desig2)},

    {"bitmap", FT_SHORT, OFFSET_OF(s_bitmap)},
    {"width", FT_SHORT, OFFSET_OF(s_width)},
    {"height", FT_SHORT, OFFSET_OF(s_height)},

    {"timer", FT_INT, OFFSET_OF(s_timer)},
    {"maxnum", FT_INT, OFFSET_OF(s_maxnum)},
    {"rank", FT_INT, OFFSET_OF(s_rank)},
    {"numdefn", FT_INT, OFFSET_OF(s_numdefn)},
    {"numplan", FT_INT, OFFSET_OF(s_numplan)},

    {"nflags", FT_LONGFLAGS, OFFSET_OF(s_nflags), (void *) &all_ship_longflags},

    {0},
};

#undef OFFSET_OF
