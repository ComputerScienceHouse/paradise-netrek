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

#include <sys/stat.h>
#include "config.h"
#include "proto.h"
#include "daemonII.h"
#include "data.h"
#include "shmem.h"
#include "structdesc.h"


/*-----------------------------MODULE VARS--------------------------------*/

static struct stat oldstat;	/* to hold info about file so we */
 /* can check whether it has changed */

/* these might not match new designations provided in the sysdef.  Oh well */
static char *shiptypes[NUM_TYPES] = {
    "SC", "DD", "CA", "BB", "AS", "SB", "AT",
    "JS", "FR", "WB", "CL", "CV", "UT", "PT"
};


static char *weapontypes[WP_MAX] = {"PLASMA", "TRACTOR", "MISSILE", "FIGHTER"};

static char *systemtypes[SHIPS_SYSTEMS] = {
    "PLASMA", "TRACTOR", "MISSILE", "FIGHTER", "PHOTON", "PHASER", "SHIELD",
    "REPAIR", "CLOAK", "SCANNER", "SENSOR", "WARP", "IMPULSE", "DOCK",
    "NAVIGATION", "COMMUNICATION",
};

/*--------------------------------------------------------------------------*/



extern struct field_desc ship_fields[];



/*-----------------------------INTERNAL FUNCTIONS-------------------------*/

static int 
touch(char *file)
{
#ifdef HAVE_UTIME_NULL
    return(utime(file, NULL));
#else
    time_t  now[2];

    now[0] = now[1] = time(0);

    return(utime(file, (void *) now));
#endif
}

static void 
load_clue_phrases(void)
{
    char	*path;
    FILE	*fp;
    int		count;
    int		i;
    char	*s = cluephrase_storage; /* damn, I'm tired of typing it */

    path = build_path(CLUEPHRASEFILE);

    fp = fopen(path, "r");
    if (!fp)
	return;

    count =fread(s, 1, CLUEPHRASE_SIZE-2, fp);
    fclose(fp);

    if (count<0) {
      /* ACK, read fail */
      s[0] = 0;
    } else {
      s[count] = 0;		/* two zeros terminates the clue phrase list */
      s[count+1] = 0;
      for (i=0; i<count; i++)
	if (s[i]=='\n')
	  s[i] = 0;
    }
}

/*------------------------------READSTRINGS-------------------------------*/
/*  This function reads in a list of strings.  An array of strings contains
the name of the possible strings in the list.  If the string is read in from
the file and matches one of the strings in the array, then the corresponding
element in the parameter 'array' is set to 1.  All keys must be of the
same length.  */

/* args:
    char   *type;		 Used to specify the type for err messages
    char   *string;		 the string to parse.
    char  **keys;		 the array of key strings
    int    *array;		 the array tofill with 1's
    int     max;		 the size of the array */
static void 
readstrings(char *type, char *string, char **keys, int *array, int max)
{
    int     i;			/* looping var */

    while (*string != '\0') {	/* go until end of string */
	while ((*string == '\n') || (*string == ' ') || (*string == ',')
	       || (*string == '\t'))
	    string++;		/* go to next char if white space */
	if (*string == '\0')	/* if end of string found */
	    break;
	for (i = 0; i < max; i++) {	/* search through keys */
	    if (strncmp(keys[i], string, strlen(keys[i])) == 0) {
		string += strlen(keys[i]);	/* go to next key */
		array[i] = 1;	/* set array element to 1 */
		break;		/* found it, break out of for loop */
	    }
	}
	if (i == max) {		/* if unknown key then */
	    fprintf(stderr, "%s type %s unknown!\n", type, string);
	    string++;		/* print error */
	}
    }
}


/* modifies flagp to return result */
static void 
read_longflags(long *flagp, char *str, void *names)
{
    char    buf[80];
    struct  longflags *lfd = (struct longflags *)names;
    struct  longflags_desc *fld = lfd->lfd;

    *flagp = 0;

    while (*str) {
	int     i;
	i = 0;
	while (*str != 0 && *str != ',')
	    buf[i++] = *(str++);
	if (*str == ',')
	    str++;
	buf[i] = 0;

	for (i = 0; fld[i].name; i++)
	    if (0 == strcasecmp(buf, fld[i].name))
		break;

	if (!fld[i].name) {
	    fprintf(stderr, "unknown flag %s\n", buf);
	    continue;
	}
	*flagp |= fld[i].bitvalue;
    }
}


/*--------------------------------SHIPDEFS---------------------------------*/
/*  This function gets all of the field values for a ship.  Each line of
input for the ship fields has a single ship field on it.  The name of the
ship field is followed by the value to place in that field.  The function
stops when it encounters a line with "end" on it.   It places the values
into the shipvals so that the next time getship is called, the new values
will be used.  */

static void 
shipdefs(int s, FILE *f)
{
    struct ship *currship = shipvals + s;
    char    buf[256];		/* to get a string from file */
    char    field_name[64];	/* to get name of field */
    char    value[256];		/* to get name */
    char    sdesig[64];		/* to get ship letters */
    int     len;		/* to hold length of read in string */
    int     i;			/* looping var */
    int     offset;		/* to offset into ship structure */

    if ((s < 0) || (s >= NUM_TYPES)) {	/* invalid ship number? */
	fprintf(stderr, "invalid ship number in .sysdef file\n");
	return;
    }
    while (1) {			/* loop until break */
	if (0 == fgets(buf, sizeof(buf), f))	/* get a string of input */
	    break;		/* if end of file then break */
	if (buf[0] == '!' || buf[0] == '#')
	    continue;		/* skip lines that begin with ! or # */
	len = strlen(buf);
	if (buf[len - 1] == '\n')	/* blast trailing newline */
	    buf[--len] = 0;
	if (strncmp(buf, "end", 3) == 0) {	/* if end of ship then break */
	    return;
	}
	if (shipvals == 0)
	    continue;

	/* get field name and value */
	sscanf(buf, "%s %s %s", sdesig, field_name, value);

	for (i = 0; ship_fields[i].name; i++) {	/* loop through field names */
	    if (strcmp(field_name, &(ship_fields[i].name[0])) == 0 ||	/* found field? */
		(strncmp(field_name, "s_", 2) == 0 &&
		 strcmp(field_name + 2, &(ship_fields[i].name[0])) == 0))
		break;
	}
	if (ship_fields[i].name == 0) {	/* if we did not find name */
	    fprintf(stderr, "invalid field name in ship description `%s'\n",
		    field_name);
	    continue;		/* print error, get next */
	}
	offset = ship_fields[i].offset;	/* get offset into struct */
	switch (ship_fields[i].type) {	/* parse the right type */
	case FT_CHAR:
	    sscanf(value, "%c", offset + (char *) currship);
	    break;
	case FT_SHORT:
	    sscanf(value, "%hi", (short*)(offset + (char *) currship));
	    break;
	case FT_INT:
	    sscanf(value, "%i", (int*)(offset + (char *) currship));
	    break;
	case FT_LONG:
	    sscanf(value, "%li", (long*)(offset + (char *) currship));
	    break;
	case FT_FLOAT:
	    sscanf(value, "%f", (float*)(offset + (char *) currship));
	    break;
	case FT_STRING:
	    sscanf(value, "%s", offset + (char *) currship);
	    break;
	case FT_LONGFLAGS:
	    read_longflags((long *) (offset + (char *) currship), value,
			   ship_fields[i].aux);
	    break;
	default:
	    fprintf(stderr, "Internal error, unknown field type %d\n",
		    ship_fields[i].type);
	}
    }
}

/*--------------------------------------------------------------------------*/
static void 
initteamvals(void)
{
    strcpy(teams[NOBODY].nickname, "Independent");
    strcpy(teams[NOBODY].name, "Independents");
    teams[NOBODY].letter = 'I';
    strcpy(teams[NOBODY].shortname, "IND");

    strcpy(teams[FED].nickname, "Fed");
    strcpy(teams[FED].name, "Federation");
    teams[FED].letter = 'F';
    strcpy(teams[FED].shortname, "FED");

    strcpy(teams[ROM].nickname, "Romulan");
    strcpy(teams[ROM].name, "Romulans");
    teams[ROM].letter = 'R';
    strcpy(teams[ROM].shortname, "ROM");

    strcpy(teams[KLI].nickname, "Klingon");
    strcpy(teams[KLI].name, "Klingons");
    teams[KLI].letter = 'K';
    strcpy(teams[KLI].shortname, "KLI");

    strcpy(teams[ORI].nickname, "Orion");
    strcpy(teams[ORI].name, "Orions");
    teams[ORI].letter = 'O';
    strcpy(teams[ORI].shortname, "ORI");
}

/*--------------------------------------------------------------------------*/



#define OFFSET_OF(field)	( (char*)(&((struct configuration*)0)->field) -\
 			  (char*)0)

static struct field_desc config_fields[] = {
    {"TOURN", FT_INT, OFFSET_OF(tournplayers)},
    {"TESTERS", FT_INT, OFFSET_OF(ntesters)},

    {"CONFIRM", FT_BYTE, OFFSET_OF(binconfirm)},
    {"MAXLOAD", FT_FLOAT, OFFSET_OF(maxload)},
    {"UDP", FT_BYTE, OFFSET_OF(udpAllowed)},
    {"MINUPDDELAY", FT_INT, OFFSET_OF(min_upd_delay)},
    {"MINOBSUPDDELAY", FT_INT, OFFSET_OF(min_observer_upd_delay)},

    {"PLKILLS", FT_FLOAT, OFFSET_OF(plkills)},
    {"MSKILLS", FT_FLOAT, OFFSET_OF(mskills)},
    {"EROSION", FT_FLOAT, OFFSET_OF(erosion)},
    {"PENETRATION", FT_FLOAT, OFFSET_OF(penetration)},
    {"NEWTURN", FT_INT, OFFSET_OF(newturn)},
    {"HIDDEN", FT_INT, OFFSET_OF(hiddenenemy)},
    {"PLANUPDSPD", FT_FLOAT, OFFSET_OF(planupdspd)},
#ifdef LOADABLE_PLGEN
    {"GALAXYGENERATOR", FT_STRING, OFFSET_OF(galaxygenerator)},
#else
    {"GALAXYGENERATOR", FT_INT, OFFSET_OF(galaxygenerator)},
#endif
    {"NUMWORMPAIRS", FT_INT, OFFSET_OF(num_wormpairs)},
    {"NUMNEBULA", FT_INT, OFFSET_OF(num_nebula)},
    {"NEBULADENSITY", FT_INT, OFFSET_OF(nebula_density)},
    {"NEBULASUBCLOUDS", FT_INT, OFFSET_OF(nebula_subclouds)},
    {"NUMASTEROID", FT_INT, OFFSET_OF(num_asteroid)},
    {"ASTEROIDTHICKNESS", FT_FLOAT, OFFSET_OF(asteroid_thickness)},
    {"ASTEROIDDENSITY", FT_INT, OFFSET_OF(asteroid_density)},
    {"ASTEROIDRADIUS", FT_INT, OFFSET_OF(asteroid_radius)},
    {"ASTEROIDTHICKVAR", FT_FLOAT, OFFSET_OF(asteroid_thick_variance)},
    {"ASTEROIDDENSVAR", FT_INT, OFFSET_OF(asteroid_dens_variance)},
    {"ASTEROIDRADVAR", FT_INT, OFFSET_OF(asteroid_rad_variance)},
    {"POPSCHEME", FT_BYTE, OFFSET_OF(popscheme)},
    {"POPCHOICE", FT_BYTE, OFFSET_OF(popchoice)},
    {"POPSPEED%", FT_INT, OFFSET_OF(popspeed)},
    {"RESOURCEBOMBING", FT_BYTE, OFFSET_OF(resource_bombing)},
    {"REVOLTS", FT_BYTE, OFFSET_OF(revolts)},
    {"BRONCOSHIPVALS", FT_BYTE, OFFSET_OF(bronco_shipvals)},
    {"EVACUATION", FT_BYTE, OFFSET_OF(evacuation)},
    {"JUSTIFY_GALAXY", FT_BYTE, OFFSET_OF(justify_galaxy)},
    {"AFTERBURNERS", FT_BYTE, OFFSET_OF(afterburners)},
    {"WARPDRIVE", FT_BYTE, OFFSET_OF(warpdrive)},
    {"FUELEXPLOSIONS", FT_BYTE, OFFSET_OF(fuel_explosions)},
    {"NEWCLOAK", FT_BYTE, OFFSET_OF(newcloak)},
    {"BRONCORANKS", FT_BYTE, OFFSET_OF(bronco_ranks)},
    {"NEWARMYGROWTH", FT_BYTE, OFFSET_OF(new_army_growth)},
    {"WARPDECEL", FT_BYTE, OFFSET_OF(warpdecel)},
    {"AFFECTSHIPTIMERSOUTSIDET", FT_BYTE, OFFSET_OF(affect_shiptimers_outside_T)},
    {"DURABLESCOUTING", FT_BYTE, OFFSET_OF(durablescouting)},
    {"FACILITYGROWTH", FT_BYTE, OFFSET_OF(facilitygrowth)},
    {"RPRDURINGWARPPREP", FT_BYTE, OFFSET_OF(repair_during_warp_prep)},
    {"RPRDURINGWARP", FT_BYTE, OFFSET_OF(repair_during_warp)},
    {"FIREDURINGWARPPREP", FT_BYTE, OFFSET_OF(fireduringwarpprep)},
    {"FIREDURINGWARP", FT_BYTE, OFFSET_OF(fireduringwarp)},
    {"FIREWHILEDOCKED", FT_BYTE, OFFSET_OF(firewhiledocked)},
    {"WARPPREPSTYLE", FT_BYTE, OFFSET_OF(warpprepstyle)},
    {"BASERANKSTYLE", FT_BYTE, OFFSET_OF(baserankstyle)},
    {"CLOAKDURINGWARPPREP", FT_BYTE, OFFSET_OF(cloakduringwarpprep)},
    {"CLOAKWHILEWARPING", FT_BYTE, OFFSET_OF(cloakwhilewarping)},
    {"TRACTABORTWARP", FT_BYTE, OFFSET_OF(tractabortwarp)},
    {"ORBITDIRPROB", FT_FLOAT, OFFSET_OF(orbitdirprob)},
    {"NEWORBITS", FT_BYTE, OFFSET_OF(neworbits)},
    {"PLANETSINPLAY", FT_INT, OFFSET_OF(planetsinplay)},
    {"PLANETLIMITTYPE", FT_INT, OFFSET_OF(planetlimittype)},
    {"BEAMLASTARMIES", FT_BYTE, OFFSET_OF(beamlastarmies)},
    {"TIMEOUTS", FT_INT, OFFSET_OF(timeouts)},
    {"REGULATIONMINUTES", FT_INT, OFFSET_OF(regulation_minutes)},
    {"OVERTIMEMINUTES", FT_INT, OFFSET_OF(overtime_minutes)},
    {"PING_PERIOD", FT_INT, OFFSET_OF(ping_period)},
    {"PING_ILOSS_INTERVAL", FT_INT, OFFSET_OF(ping_iloss_interval)},
    {"PING_GHOSTBUST", FT_INT, OFFSET_OF(ping_allow_ghostbust)},
    {"PING_GHOSTBUST_INTERVAL", FT_INT, OFFSET_OF(ping_ghostbust_interval)},
    {"CLUECHECK", FT_BYTE, OFFSET_OF(cluecheck) },
    {"CLUEDELAY", FT_INT, OFFSET_OF(cluedelay) },
    {"CLUETIME", FT_INT, OFFSET_OF(cluetime) },
    {"CLUESOURCE", FT_INT, OFFSET_OF(cluesource) },
    {"VARIABLE_WARP", FT_INT, OFFSET_OF(variable_warp)},
    {"WARPPREP_SUSPENDABLE", FT_INT, OFFSET_OF(warpprep_suspendable)},
    {"NOPREGAMEBEAMUP", FT_INT, OFFSET_OF(nopregamebeamup)},
    {"GAMESTARTNUKE", FT_INT, OFFSET_OF(gamestartnuke)},
    {"NOTTIMEOUT", FT_INT, OFFSET_OF(nottimeout)},
    {"WARPZONE", FT_INT, OFFSET_OF(warpzone)},
    {"HELPFULPLANETS", FT_INT, OFFSET_OF(helpfulplanets)},
    {"WBBOMBINGCREDIT", FT_BYTE, OFFSET_OF(wb_bombing_credit)},
    {"JSPLANETCREDIT", FT_BYTE, OFFSET_OF(js_assist_credit)},
    {"BUTTTORP_PENALTY", FT_BYTE, OFFSET_OF(butttorp_penalty)},
    {"SLOWBOMB", FT_BYTE, OFFSET_OF(slow_bomb)},
    {"ROBOTSTATS", FT_BYTE, OFFSET_OF(robot_stats)},
    {"LOSING_ADVANTAGE", FT_FLOAT, OFFSET_OF(losing_advantage)},
    {"VICTORY_PLANETS", FT_INT, OFFSET_OF(victory_planets)},
    {"REVOLT_WITH_FACILITIES", FT_BYTE, OFFSET_OF(revolt_with_facilities)},
    {"KILL_CARRIED_ARMIES", FT_FLOAT, OFFSET_OF(kill_carried_armies)},
    {"SHIPYARD_BUILT_BY_SB_ONLY", FT_BYTE, OFFSET_OF(shipyard_built_by_sb_only)},
    {"CAN_BOMB_OWN_SHIPYARD", FT_BYTE, OFFSET_OF(can_bomb_own_shipyard)},
    {"SURRSTART", FT_INT, OFFSET_OF(surrstart)},
    {"SURREND", FT_INT, OFFSET_OF(surrend)},
    {"SURRLENGTH", FT_INT, OFFSET_OF(surrlength)},
    {"ARMY_DEFEND_FACILITIES", FT_FLOAT, OFFSET_OF(army_defend_facilities)},
    {"ARMY_DEFEND_BARE", FT_FLOAT, OFFSET_OF(army_defend_bare)},
    {0}
};

#undef OFFSET_OF



/*----------------------------VISIBLE FUNCTIONS----------------------------*/

/*------------------------------READSYSDEFAULTS----------------------------*/
/*  This function reads in the system defaults from a file.  A number of
defaults are set and if the file contains keywords with different settings
then they are changed.  */

void 
readsysdefaults(void)
{
    int     i;			/* looping var */
    FILE   *f;			/* to open sysdefaults file */
    char    buf[200];		/* to get a line of text */
    char   *s;			/* to point to fields in text */
    char   *paths;		/* to hold path name of dot dir */

    load_clue_phrases();

    /*
       put default values in the configuration values for readsysdefaults()
       to override
    */
    configvals->tournplayers = 5;
    configvals->ntesters =
      status2->league ? 2 :
	12;

    configvals->binconfirm = 0;
    configvals->maxload = 100.0;
    configvals->udpAllowed = 1;
    configvals->min_upd_delay = 200000;	/* 5 updates/sec */
    configvals->min_observer_upd_delay = 333000;	/* 3 updates/sec */

    configvals->plkills = 2;
    configvals->mskills = 2;  /* was 2.5, changed 5-Nov-94 by PLC */
    configvals->erosion = 0.0;
    configvals->penetration = 0.0;
    configvals->newturn = 0;
    configvals->hiddenenemy = 1;
    configvals->planupdspd = 0;
    configvals->justify_galaxy = 1; /* changed 5-Nov-94 by PLC */

#ifdef LOADABLE_PLGEN
    strcpy(configvals->galaxygenerator, "3");
#else
    configvals->galaxygenerator = 3;	/* Heath's galaxy generator */
                                        /* changed 5-Nov-94 by PLC */
#endif

    configvals->num_wormpairs = 0;
    configvals->resource_bombing = 1;
    configvals->revolts = 1;
    configvals->afterburners = 1;
    configvals->warpdrive = 1;
    configvals->fuel_explosions = 1;
    configvals->bronco_shipvals = 0;
    configvals->evacuation = 1;	/* evacuation is allowed in paradise */
    configvals->newcloak = 1;
    configvals->new_army_growth = 1;	/* WAY faster than bronco in many
					   cases */
    configvals->warpdecel = 0;
    configvals->affect_shiptimers_outside_T = 0;

    configvals->durablescouting = 0;
    configvals->facilitygrowth = 1;
    configvals->orbitdirprob = 1; /* changed 5-Nov-94 by PLC */
    configvals->neworbits = 1;
    configvals->planetsinplay = 17; /* changed 5-Nov-94 by PLC */
    configvals->planetlimittype = 0;
    configvals->popscheme = 1;
    configvals->popchoice = 1;
    configvals->popspeed = 14; /* was 9, changed 5-Nov-94 by PLC */

    configvals->repair_during_warp_prep = 1;
    configvals->repair_during_warp = 1;
    configvals->fireduringwarpprep = 0;
    configvals->fireduringwarp = 0;
    configvals->firewhiledocked = 0;
    configvals->tractabortwarp = 0; /* changed 5-Nov-94 by PLC */

    configvals->warpprepstyle = WPS_TABORT;
    configvals->baserankstyle = 0;
    configvals->cloakduringwarpprep = 0;
    configvals->cloakwhilewarping = 1;

    configvals->num_nebula = 0;
    configvals->nebula_density = 240; /* temporily used as size */
    configvals->nebula_subclouds = 0; 
    configvals->num_asteroid = 0;
    configvals->asteroid_thickness = 1.0;  /* small to medium sized */
    configvals->asteroid_radius = 12;    /* the distance from the "owning" star */
    configvals->asteroid_density = 60; /* density is % chance an eligible tgrid
                                          locale will have asteroids */
    configvals->asteroid_thick_variance = 3.0;  
    configvals->asteroid_rad_variance = 8; 
    configvals->asteroid_dens_variance = 40;

    configvals->beamlastarmies = 0;

    for (i = 0; i < SHIPS_SYSTEMS; i++)
	configvals->sun_effect[i] = (i == SS_PHOTON ||
				     i == SS_PLASMA ||
				     i == SS_MISSILE ||
				     i == SS_FIGHTER);
    for (i = 0; i < SHIPS_SYSTEMS; i++)
	configvals->ast_effect[i] = (i == SS_PHOTON ||
				     i == SS_PLASMA ||
				     i == SS_MISSILE ||
				     i == SS_FIGHTER ||
				     i == SS_IMPULSE);
    for (i = 0; i < SHIPS_SYSTEMS; i++)
	configvals->neb_effect[i] = 0;
    for (i = 0; i < SHIPS_SYSTEMS; i++)
	configvals->wh_effect[i] = (i == SS_PHOTON ||
				    i == SS_PLASMA ||
				    i == SS_MISSILE ||
				    i == SS_FIGHTER ||
				    i == SS_WARP ||
				    i == SS_IMPULSE);

    for (i = 0; i < SHIPS_SYSTEMS; i++)
	configvals->improved_tracking[i] =
	  (
#if 0
	   /* I recommend improved tracking for these - RF */
	   i == SS_MISSILE ||
	   i == SS_FIGHTER ||	/* except this doesn't work just yet */
	   /* fighter torps REALLY need this - MDM */
	   i == SS_PHOTON  ||
#endif
	   0);

    for (i = 0; i < NUM_TYPES; i++)
	configvals->shipsallowed[i] = (i == SCOUT ||
				       i == DESTROYER ||
				       i == CRUISER ||
				       i == BATTLESHIP ||
				       i == ASSAULT ||
				       i == STARBASE ||
				       i == JUMPSHIP ||
				       i == FRIGATE ||
				       i == WARBASE);
    for (i = 0; i < WP_MAX; i++)
	configvals->weaponsallowed[i] = (i == WP_PLASMA ||
					 i == WP_TRACTOR ||
					 i == WP_MISSILE ||
					 0);

    configvals->timeouts = 0;	/* NYI */
    configvals->regulation_minutes = 60;
    configvals->overtime_minutes = 0;	/* NYI */
    configvals->playersperteam = 8;

    configvals->nopregamebeamup = 0;
    configvals->gamestartnuke = 0;
    configvals->nottimeout = 0;

    configvals->ping_period = 2;/* every 2 seconds */
    configvals->ping_iloss_interval = 10;
    configvals->ping_allow_ghostbust = 0;
    configvals->ping_ghostbust_interval = 10;

    configvals->cluecheck = 0;	/* don't check clue */
    configvals->cluetime = 5*60;	/* 5 minutes */
    configvals->cluedelay = 2*60*60; /* 2 hours */
    configvals->cluesource = CC_PHRASE_LIST_FILE;
    configvals->variable_warp = 1;	/* warp speed is variable [BDyess] */
                                        /* changed 5-Nov-94 by PLC */
    configvals->warpprep_suspendable = 1;	/* warp prep is suspendable
						   [BDyess] */
                                        /* changed 5-Nov-94 by PLC */
    configvals->warpzone = 0;		/* warp zone default off [BDyess] */

    configvals->plgrow.fuel = 100;
    configvals->plgrow.agri = 350;
    configvals->plgrow.repair = 150;
    configvals->plgrow.shipyard = 400;

    configvals->helpfulplanets = 0;

    configvals->wb_bombing_credit = 1;
    configvals->js_assist_credit = 1;
    configvals->butttorp_penalty = 0;
    configvals->slow_bomb = 1;
    configvals->robot_stats = 1;

    configvals->losing_advantage = 0.0;

    configvals->victory_planets = 0;		/* original Paradise */
    configvals->revolt_with_facilities = 1;

    configvals->kill_carried_armies = 0.0;	/* original Paradise */
    configvals->shipyard_built_by_sb_only = 0;	/* original Paradise */
    configvals->can_bomb_own_shipyard = 0;	/* original Paradise */

    configvals->surrstart = 4;
    configvals->surrend = 7;
    configvals->surrlength = 25;

    configvals->army_defend_facilities = 0.0;	/* original Paradise */
    configvals->army_defend_bare = 0.0;		/* original Paradise */

    getshipdefaults();
    initteamvals();

    /* set server defaults */
    testtime = -1;		/* not in testing mode */

    if (status2->league)
	return;			/* don't read .sysdef during league game */
    paths = build_path(SYSDEF_FILE);	/* cat on sysdef filename */
    f = fopen(paths, "r");	/* attempt to open file */
    if (f == NULL) {		/* if failure to open file */
	fprintf(stderr, "No system defaults file!\n");
	return;			/* error message then out of here */
    }
    stat(paths, &oldstat);	/* record info about file */

    while (fgets(buf, 199, f) != NULL) {	/* read strings until end of
						   file */
	if (buf[0] == '!' || buf[0] == '#' )
	    continue;		/* skip lines that begin with ! */
	s = strchr(buf, '=');	/* find the equals sign in string */
	if (s == NULL)		/* if no equals sign then */
	    continue;		/* go to next string */
	*s = '\0';		/* break buf into rhs and lhs */
	s++;
	if (strcmp(buf, "SHIPS") == 0) {	/* if ship enabling then */
	    for (i = 0; i < NUM_TYPES; i++)	/* go through ships */
		shipsallowed[i] = 0;	/* turn off all ships */
	    readstrings("SHIPS", s, shiptypes, shipsallowed, NUM_TYPES);
	}
	else if (strcmp(buf, "WEAPONS") == 0) {	/* if weapon enabling */
	    for (i = 0; i < WP_MAX; i++)	/* set all weapons as off */
		weaponsallowed[i] = 0;	/* then read in weapons */
	    readstrings("WEAPONS", s, weapontypes, weaponsallowed, WP_MAX);
	}
	else if (strcmp(buf, "SUN_EFFECT") == 0) {
	    for (i = 0; i < SHIPS_SYSTEMS; i++)
		sun_effect[i] = 0;
	    readstrings("SUN_EFFECT", s, systemtypes, sun_effect, SHIPS_SYSTEMS);
	}
	else if (strcmp(buf, "ASTEROID_EFFECT") == 0) {
	    for (i = 0; i < SHIPS_SYSTEMS; i++)
		ast_effect[i] = 0;
	    readstrings("ASTEROID_EFFECT", s, systemtypes, ast_effect, SHIPS_SYSTEMS);
	}
	else if (strcmp(buf, "NEBULA_EFFECT") == 0) {
	    for (i = 0; i < SHIPS_SYSTEMS; i++)
		neb_effect[i] = 0;
	    readstrings("NEBULA_EFFECT", s, systemtypes, neb_effect, SHIPS_SYSTEMS);	 	}
	else if (strcmp(buf, "WORMHOLE_EFFECT") == 0) {
	    for (i = 0; i < SHIPS_SYSTEMS; i++)
		wh_effect[i] = 0;
	    readstrings("WORMHOLE_EFFECT", s, systemtypes, wh_effect, SHIPS_SYSTEMS);	 	}
	else if (strcmp(buf, "IMPROVED_TRACKING") == 0) {
	    for (i = 0; i < SHIPS_SYSTEMS; i++)
		improved_tracking[i] = 0;
	    readstrings("IMPROVED_TRACKING", s, systemtypes, improved_tracking, SHIPS_SYSTEMS);
	}
	else if (strcmp(buf, "SHIP") == 0) {	/* if ship being entered */
	    shipdefs(atoi(s), f);
	}
	else if (strcmp(buf, "RELOAD_SHIPDEFAULTS") == 0) {
	    getshipdefaults();
	}
	else {
	    for (i = 0; config_fields[i].name; i++) {
		if (strcmp(buf, config_fields[i].name) == 0)
		    break;
	    }
	    if (!config_fields[i].name) {
		fprintf(stderr, "System default %s unknown\n", buf);
	    }
	    else {
		int     offset = config_fields[i].offset;
		char   *curr = (char *) configvals;
		switch (config_fields[i].type) {	/* parse the right type */
		case FT_BYTE:
		    *(offset + curr) = atoi(s);
		    break;
		case FT_SHORT:
		    sscanf(s, "%hi", (short*)(offset + curr));
		    break;
		case FT_INT:
		    sscanf(s, "%i", (int*)(offset + curr));
		    break;
		case FT_LONG:
		    sscanf(s, "%li", (long*)(offset + curr));
		    break;
		case FT_FLOAT:
		    sscanf(s, "%f", (float*)(offset + curr));
		    break;
		case FT_STRING:
		    sscanf(s, "%s", offset + curr);
		    break;
		case FT_LONGFLAGS:
		    read_longflags((long *) (offset + curr), s,
				   config_fields[i].aux);
		    break;
		default:
		    fprintf(stderr, "Internal error, unknown config field type %d\n",
			    config_fields[i].type);
		}
	    }
	}
    }

    if (configvals->tournplayers < 1)	/* get number of players needed */
	configvals->tournplayers = 5;	/* cannot set tournplayers to 0 */

    if (configvals->erosion > 1)
	configvals->erosion = 1;
    if (configvals->penetration > 1)
	configvals->penetration = 1;
    if (configvals->penetration < 0)
	configvals->penetration = 0;

    if (configvals->ping_period <= 0)
	configvals->ping_period = 1;
    if (configvals->ping_iloss_interval <= 0)
	configvals->ping_iloss_interval = 1;
    if (configvals->ping_ghostbust_interval <= 0)
	configvals->ping_ghostbust_interval = 1;

    fclose(f);			/* close sysdefaults file */
}




/*--------------------------UPDATE_SYS_DEFAULTS---------------------------*/
/* This function will update all of the defaults if the default file
 * changed.  It is called often, and assuming the OS caches the file
 * inode info well, this isn't a problem.
  This function returns a 1 if new sysdefaults were loaded.  Otherwise a
zero is returned.  */

int 
update_sys_defaults(void)
{
    struct stat newstat;
    static char   *paths = NULL;		/* to hold full pathname */

    if (status2->league)
	return 0;		/* don't read .sysdef during league game */

    if (!paths)		/* just do the build_path once */
	paths = strdup(build_path(SYSDEF_FILE));

    if (stat(paths, &newstat) == 0) {	/* get info about sysdef file */
	if (newstat.st_ino != oldstat.st_ino ||	/* if the file has */
	    newstat.st_mtime != oldstat.st_mtime) {	/* changed then */
	    readsysdefaults();	/* load new defaults and */

	    /*
	       touch the motd file so the clients will re-load the sysdef
	       screen
	    */
	    touch(build_path(MOTD));

	    return (1);		/* return that they were loaded */
	}
    }
    return (0);			/* return 0 for no new stats */
}

/*--------------------------------------------------------------------------*/





/*--------END OF FILE-------*/
