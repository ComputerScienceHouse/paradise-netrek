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

/* was control_mess.c, but changed to message.c for a smaller filename (BG) */

/*

   Ugh, this code is in the middle of a rewrite.

   It used to use a tokenizer with a global dictionary to split the
   input into words.  The tokenizer accepted abbreviations as long as
   these were unique.  However, adding a new word to the dictionary
   would often cause old abbreviations to be invalidated.

   I wrote a new parser that was called as each token needed to be
   extracted.  This used a dictionary that was local to each submenu.
   This localizes changes to the menu structure so that effects of
   adding new commands are minimized.

   Some of the file is converted to use this, but not all.  Eventually
   the entire module will use the context-sensitive tokenizer.

*/

#include <signal.h>
#include <ctype.h>
#include "config.h"
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif
#include "proto.h"
#include "ntserv.h"
#include "data.h"
#include "shmem.h"

enum token_names_e {
    HELPTOK = 128,
    CONTROLTOK,
    VERSIONTOK,
    QUEUETOK,
    LEAGUETOK,
    PARAMTOK,
    INFOTOK,
    OBSERVETOK,
    CLUECHECKTOK,

    NUKEGAMETOK,
    FREESLOTTOK,
    ROBOTTOK,
    SNAKETOK,
    TOURNTOK,
    NEWGALAXY,
    SHIPTIMERTOK,

    REFITTOK,
    PLAYERTOK,
    EJECTTOK,
    DIETOK,
    ARMIESTOK,
    PLASMATOK,
    MISSILETOK,
    PLANETTOK,
    RANKTOK,
    MOVETOK,

    PASSWDTOK,
    RATINGSTOK,

    TIMETOK,
    CAPTAINTOK,
    RESTARTTOK,
    STARTTOK,
    PASSTOK,
    TIMEOUTTOK,
    TEAMNAMETOK,
    AWAYTOK,
    HOMETOK,
    PAUSETOK,
    CONTINUETOK,
    MAXPLAYERTOK,

    TEAMTOK,

    INDTOK,			/* these need to be adjacent and in order */
    FEDTOK,			/* these need to be adjacent and in order */
    ROMTOK,			/* these need to be adjacent and in order */
    KLITOK,			/* these need to be adjacent and in order */
    ORITOK,			/* these need to be adjacent and in order */

    SHIPTOK,

    SCOUTTOK,			/* these need to be adjacent and in order */
    DESTROYERTOK,		/* these need to be adjacent and in order */
    CRUISERTOK,			/* these need to be adjacent and in order */
    BATTLESHIPTOK,		/* these need to be adjacent and in order */
    ASSAULTTOK,			/* these need to be adjacent and in order */
    STARBASETOK,		/* these need to be adjacent and in order */
    ATTTOK,			/* these need to be adjacent and in order */
    JUMPSHIPTOK,		/* these need to be adjacent and in order */
    FRIGATETOK,			/* these need to be adjacent and in order */
    WARBASETOK,			/* these need to be adjacent and in order */
    LIGHTCRUISERTOK,
    CARRIERTOK,
    UTILITYTOK,
    PATROLTOK,

    PLUSTOK,
    MINUSTOK,
    ROYALTOK,
    QUIETTOK,
    KILLSTOK,
    HOSETOK,
    SUPERTOK,
    ALLOWTOK,			/* control allow [teams] */

    ERRORTOK = 255
};

static int god_silent = 0;

/**********************************************************************/

/* New parsing method. */

struct control_cmd {
    char   *literal;		/* the command they should type */
    enum token_names_e tok;	/* what the parser should return */
    char   *doc;		/* documentation to print for a help command */
};

/* Scans the string cmd for the first whitespace delimited string.
   Tries to match this string in a case-insensitive manner against the
   list in legals.  Returns the appropriate token.  Alters the char*
   pointed to by after to be the beginning of the next
   whitespace-delimited string.

   *cmd:	unmodified
   *legals:	unmodified
   *after:	MODIFIED

   */

static enum token_names_e
next_token(char *cmd, struct control_cmd *legals, char **after)
{
    char    buf[80];		/* space for the token */
    char   *s;
    int     i;
    int     ambiguous = 0;
    enum token_names_e potentialtok = ERRORTOK;

    while (*cmd && isspace(*cmd))
	cmd++;

    if (!*cmd)
	return ERRORTOK;

    for (s = buf; *cmd && !isspace(*cmd); s++, cmd++)
	*s = *cmd;
    *s = 0;

    while (*cmd && isspace(*cmd))
	cmd++;

    if (after)
	*after = cmd;		/* so they can find the next token */

    for (i = 0; legals[i].literal; i++) {
	int     wordlen = strlen(buf);
	if (0 == strncasecmp(buf, legals[i].literal, wordlen)) {
	    if (strlen(legals[i].literal) == wordlen) {
		ambiguous = 0;
		potentialtok = legals[i].tok;
		break;		/* exact match */
	    }
	    if (potentialtok != ERRORTOK) {
		ambiguous = 1;	/* this isn't the only match */
		return ERRORTOK;
	    }
	    potentialtok = legals[i].tok;
	}
    }

    return potentialtok;
}

static int
match_token(char *cmd, char *token, char **after)
{
    struct control_cmd	legals[2];
    legals[0].literal = token;
    legals[0].tok = HELPTOK;	/* pick any token but ERRORTOK */
    legals[1].literal = 0;
    return HELPTOK==next_token(cmd, legals, after);
}

/* Get a player slot number.
   Returns -1 on failure, slot number on success.
   Slot number is guaranteed to be <MAXPLAYER. */

static int
get_slotnum(char *cmd, char **after)
{
    int	rval;
    while (*cmd && isspace(*cmd))
	cmd++;

    if (!*cmd)
	return -1;		/* no token */

    if (cmd[1] && !isspace(cmd[1]))
	return -1;		/* token too long */

    if (*cmd>='0' && *cmd<='9')
	rval = *cmd-'0';
    else if (*cmd>='a' && *cmd<='z')
	rval = *cmd-'a' + 10;
    else if (*cmd>='A' && *cmd<='Z')
	rval = *cmd-'A' + 10;
    else
	return -1;

    if (rval>=MAXPLAYER)
	return -1;		/* there aren't that many players */

    if (after) {
	/* scan to next token */
	cmd++;
	while (*cmd && isspace(*cmd))
	    cmd++;
	*after = cmd;
    }

    return rval;
}

/**********************************************************************/

static void 
respond(char *msg, int type)
{
    if (type == 1)
	warning(msg);
    else
	pmessage2(msg, me->p_no, MINDIV, MCONTROL, 255);
}

static void
bad_slotnum(char *msg)
{
    char	buf[256];
    sprintf(buf, "`%s' requires player slot number", msg);
    respond(buf, 1);
}


/* Get a single token.
   Returns 0 on failure, 1 on success.
   Token is returned in dst. */

static int
get_one_token(char *cmd, char *dst, int dstsize, char **after)
{
    while (*cmd && isspace(*cmd))
	cmd++;

    if (!*cmd)
	return 0;		/* no token */

    while (dstsize>1 && *cmd && !isspace(*cmd)) {
	*(dst++) = *(cmd++);
	dstsize--;
    }
    *dst = 0;

    if (after) {
	/* scan to next token */
	while (*cmd && isspace(*cmd))
	    cmd++;
	*after = cmd;
    }

    return 1;
}

/* Get an integer
   Integer is returned in dst.
   Returns 0 on failure without modifying dst.
   */

static int
get_int(char *cmd, int *dst, char **after)
{
    int	rval, offset;

    if (1!=sscanf(cmd, " %i%n", &rval, &offset))
	return 0;

    cmd += offset;
    if (*cmd && !isspace(*cmd))
	return 0;		/* token wasn't all digits */

    *dst = rval;

    if (after) {
	/* scan to next token */
	while (*cmd && isspace(*cmd))
	    cmd++;
	*after = cmd;
    }

    return 1;
}

/* Get a double
   Double is returned in dst.
   Returns 0 on failure without modifying dst.
   */

static int
get_double(char *cmd, double *dst, char **after)
{
    double	rval;
    int		offset;

    if (1!=sscanf(cmd, " %lg%n", &rval, &offset))
	return 0;

    cmd += offset;
    if (*cmd && !isspace(*cmd))
	return 0;		/* token wasn't all digits */

    *dst = rval;

    if (after) {
	/* scan to next token */
	while (*cmd && isspace(*cmd))
	    cmd++;
	*after = cmd;
    }

    return 1;
}

static int
get_teamid(char *cmd, int *team, char **after)
{
    int	i,j;

    while (*cmd && isspace(*cmd)) {
	cmd++;
    }
    if (cmd[3] && !isspace(cmd[3]))
	return 0;		/* too long */

    *team = NOBODY;

    for (i = -1; i < NUMTEAM; i++) {
	j = idx_to_mask(i);
	if (0==strncasecmp(cmd, teams[j].shortname, 3)) {
	  *team = i;
	  cmd += 3;
	  break;
	}
    }

    if (after) {
	/* scan to next token */
	while (*cmd && isspace(*cmd))
	    cmd++;
	*after = cmd;
    }

    return i<NUMTEAM;
}

static int
get_shipid(char *cmd, int *shipn, char **after)
{
    int	i;

    while (*cmd && isspace(*cmd)) {
	cmd++;
    }
    *shipn = -1;

    for (i = 0; i < NUM_TYPES; i++) {
	struct ship	*ship = &shipvals[i];
	int	len;
	len = strlen(ship->s_name);
	if (0==strncasecmp(cmd, ship->s_name, len) &&
	     (cmd[len]==0 || isspace(cmd[len]))) {
	    *shipn = i;
	    cmd += len;
	    break;
	} else if (tolower(cmd[0])==tolower(ship->s_desig1) &&
		   tolower(cmd[1])==tolower(ship->s_desig2) &&
		   (cmd[2]==0 || isspace(cmd[2]))) {
	    *shipn = i;
	    cmd += 2;
	    break;
	}
    }

    if (after) {
	/* scan to next token */
	while (*cmd && isspace(*cmd))
	    cmd++;
	*after = cmd;
    }

    return i<NUM_TYPES;
}

/* writes a comma-separated list of help strings into the message window */

static void 
respond_with_help_string(struct control_cmd *legals)
{
    int     i;
    char    buf[65];		/* leave space for the message prefix */

    strcpy(buf, "Available commands: ");
    for (i = 0; legals[i].literal; i++) {
	if (!(legals[i].doc && legals[i].doc[0]))
	    continue;
	if (strlen(buf) + 3 + strlen(legals[i].doc) > sizeof(buf)) {
	    respond(buf, 0);
	    strcpy(buf, "    ");
	    if (!buf[0]) {	/* one of the help strings was just too long */
		respond("ACK! programmer error: help string too long", 0);
		return;
	    }
	    i--;		/* retry */
	    continue;
	}
	strcat(buf, legals[i].doc);
	if (legals[i+1].literal)
	    strcat(buf, ", ");
    }
    if (buf[0])
	respond(buf, 0);
}

/*
 * Here we handle the controls on players.
 * If you add something, make sure you place it in the help.
 * Thanks, have a nice day.
 */

static int 
parse_control_player(char *cmd)
{
    char    buf[120];
    int     pnum;
    struct player *victim;
    int     godliness = me->p_stats.st_royal - GODLIKE + 1;
    char	*arg;
    static struct control_cmd available_cmds[] = {
	{"help", HELPTOK, 0},
	{"die", DIETOK, "die"},
	{"eject", EJECTTOK, "eject"},
	{"armies", ARMIESTOK, "armies [%d=5]"},
	{"plasma", PLASMATOK, "plasma [%d]"},
	{"missiles", MISSILETOK, "missiles [%d=max]"},
	{"team", TEAMTOK, "team <teamstr>"},
	{"ship", SHIPTOK, "ship <shiptype>"},
	{"rank", RANKTOK, "rank (+|-|%d)"},
	{"royal", ROYALTOK, "royal (+|-|%d)"},
	{"kills", KILLSTOK, "kills (+|-|%d)"},
	{"hose", HOSETOK, "hose"},
	{"move", MOVETOK, "move %d %d"},
	{0}
    };

    pnum = get_slotnum(cmd, &cmd);
    if (pnum<0) {
	bad_slotnum("control player");
	return 0;
    }
    victim = &players[pnum];

    if (victim->p_status == PFREE) {
	respond("Slot is not alive.", 1);
	return 1;
    }

/*
 * These would probably work better as pointers to functions instead of
 * a giant switch, but what the hell, I'm lazy.
 * Maybe I'll change it later.
 */

    switch (next_token(cmd, available_cmds, &arg)) {
    case DIETOK:
	victim->p_ship.s_type = STARBASE;
	victim->p_whydead = KPROVIDENCE;
	victim->p_explode = 10;
	victim->p_status = PEXPLODE;
	victim->p_whodead = 0;
	if (!god_silent) {
	    sprintf(buf, "%s (%2s) was utterly obliterated by %s (%2s).",
		    victim->p_name, twoletters(victim),
		    me->p_name, twoletters(me));
	    pmessage(buf, 0, MALL, MCONTROL);
	}
	return 1;

    case EJECTTOK:
	victim->p_ship.s_type = STARBASE;
	victim->p_whydead = KQUIT;
	victim->p_explode = 10;
	victim->p_status = PEXPLODE;
	victim->p_whodead = 0;
	if (!god_silent) {
	    sprintf(buf,
		    "%s (%2s) has been ejected from the game by %s (%2s).",
		    victim->p_name, twoletters(victim),
		    me->p_name, twoletters(me));
	    pmessage(buf, 0, MALL, MCONTROL);
	}
	return 1;

    case ARMIESTOK:
	{
	    int     armies = 5;
	    if (*arg && !get_int(arg, &armies, (char**)0)) {
		respond("optional arg to `control player <slotnum> armies` must be integer", 0);
		return 0;
	    }
	    victim->p_armies += armies;
	    if (!god_silent) {
		sprintf(buf, "%s (%2s) has been given %d armies by %s (%2s).",
			victim->p_name, twoletters(victim), armies,
			me->p_name, twoletters(me));
		pmessage(buf, 0, MALL, MCONTROL);
	    }
	    return 1;
	}

    case PLASMATOK:
	{
	    int     yes = 1;
	    if (*arg && !get_int(arg, &yes, (char**)0)) {
		respond("optional arg to `control player <slotnum> plasma` must be integer", 0);
		return 0;
	    }

	    if (yes)
		victim->p_ship.s_nflags |= SFNPLASMAARMED;
	    else
		victim->p_ship.s_nflags &= ~SFNPLASMAARMED;

	    if (!god_silent) {
		sprintf(buf, "%s (%2s) has been %s plasma torps by %s (%2s).",
			victim->p_name, twoletters(victim),
			yes ? "given" : "denied",
			me->p_name, twoletters(me));
		pmessage(buf, 0, MALL, MCONTROL);
	    }
	    return 1;
	}

    case MISSILETOK:
	{
	    int     yes = shipvals[victim->p_ship.s_type].s_missilestored;

	    if (*arg && !get_int(arg, &yes, (char**)0)) {
		respond("optional arg to `control player <slotnum> missile` must be integer", 0);
		return 0;
	    }

	    if (yes) {
		victim->p_ship.s_nflags |= SFNHASMISSILE;
		victim->p_ship.s_missilestored = yes;
	    }
	    else {
		victim->p_ship.s_nflags &= ~SFNHASMISSILE;
	    }

	    if (!god_silent) {
		sprintf(buf, "%s (%2s) has been %s %d missiles by %s (%2s).",
			victim->p_name, twoletters(victim),
			yes ? "given" : "denied",
			yes, me->p_name, twoletters(me));
		pmessage(buf, 0, MALL, MCONTROL);
	    }
	    return 1;
	}

    case TEAMTOK:
	{
	    int     team;

	    if (!get_teamid(arg, &team, (char**)0)) {
		respond("available teams: FED ORI ROM KLI IND", 0);
		return 0;
	    }
	    team = idx_to_mask(team);

	    victim->p_hostile |= victim->p_team;
	    victim->p_team = team;
	    victim->p_hostile &= ~team;
	    victim->p_swar &= ~team;
	    sprintf(buf, "%s (%2s) has been changed to a %s by %s (%2s).",
		    victim->p_name, twoletters(victim), teams[team].nickname,
		    me->p_name, twoletters(me));
	    if (!god_silent)
		pmessage(buf, 0, MALL, MCONTROL);
	    return 1;
	}

    case SHIPTOK:
	{
	    int     ship;
	    if (!get_shipid(arg, &ship, (char**)0)) {
		respond("available ships: SC DD CA AS BB SB AT JS FR WB CL CV SUPER", 0);
		return 0;
	    }
	    /* If others are docked, then kick them off */
	    if (allows_docking(victim->p_ship)) {
		int     i;
		for (i = 0; i < victim->p_ship.s_numports; i++) {
		    base_undock(victim, i);
		}
	    }
	    get_ship_for_player(victim, ship);
	    switch_special_weapon();
	    victim->p_flags &= ~PFENG;
	    sprintf(buf, "%s (%2s) has been changed to a %c%c by %s (%2s).",
		    victim->p_name, twoletters(victim),
		    victim->p_ship.s_desig1, victim->p_ship.s_desig2,
		    me->p_name, twoletters(me));
	    if (!god_silent)
		pmessage(buf, 0, MALL, MCONTROL);
	    return 1;
	}

    case RANKTOK:
	{
	    int     rank = victim->p_stats.st_rank;

	    if (match_token(arg, "+", (char**)0))
		rank++;
	    else if (match_token(arg, "-", (char**)0))
		rank--;
	    else if (!get_int(arg, &rank, (char**)0)) {
		respond("Try: control player %d rank [%d]+-", 0);
		return 0;
	    }

	    if (rank < 0)
		rank = 0;
	    if (rank >= NUMRANKS)
		rank = NUMRANKS - 1;

	    victim->p_stats.st_rank = rank;
	    sprintf(buf, "%s (%2s) has been given a rank of %s by %s (%2s).",
		    victim->p_name, twoletters(victim),
		    ranks[victim->p_stats.st_rank].name,
		    me->p_name, twoletters(me));
	    if (!god_silent)
		pmessage(buf, 0, MALL, MCONTROL);
	    return 1;
	}

    case ROYALTOK:
	{
	    int     rank = victim->p_stats.st_royal;

	    if (match_token(arg, "+", (char**)0))
		rank++;
	    else if (match_token(arg, "-", (char**)0))
		rank--;
	    else if (!get_int(arg, &rank, (char**)0)) {
		respond("Try: control player %d royal [%d]+-", 0);
		return 0;
	    }

	    if (rank < 0)
		rank = 0;
	    if (rank >= NUMROYALRANKS)
		rank = NUMROYALRANKS - 1;

	    if (rank>=GODLIKE && godliness < 2) {
	      respond("You aren't powerful enough to grant godlike royalty.",
		      1);
	      return 1;
	    }
	    victim->p_stats.st_royal = rank;
	    sprintf(buf, "%s (%2s) has been given a rank of %s by %s (%2s).",
		    victim->p_name, twoletters(victim),
		    royal[victim->p_stats.st_royal].name,
		    me->p_name, twoletters(me));
	    if (!god_silent)
		pmessage(buf, 0, MALL, MCONTROL);
	    return 1;
	}

    case KILLSTOK:
	{
	    double     kills = victim->p_kills;

	    if (match_token(arg, "+", (char**)0))
		kills += 1;
	    else if (match_token(arg, "-", (char**)0)) {
		kills -= 1;
		if (kills<0) kills=0;
	    } else if (!get_double(arg, &kills, (char**)0)) {
		respond("Try: control player %d kills [%f]+-", 0);
		return 0;
	    }

	    victim->p_kills = kills;
	    sprintf(buf, "%s (%2s) has been given %f kills by %s (%2s).",
		    victim->p_name, twoletters(victim),
		    kills, me->p_name, twoletters(me));
	    if (!god_silent)
		pmessage(buf, 0, MALL, MCONTROL);
	    return 1;
	}

    case HOSETOK:
	victim->p_shield = 0;
	victim->p_damage = victim->p_ship.s_maxdamage / 2;
	sprintf(buf, "%s (%2s) has been hosed by %s (%2s).",
		victim->p_name, twoletters(victim),
		me->p_name, twoletters(me));
	if (!god_silent)
	    pmessage(buf, 0, MALL, MCONTROL);
	return 1;

    case MOVETOK:
	{
	    int     x, y;
	    char	*s;
	    if (! (get_int(arg, &x, &s) && get_int(s, &y, (char**)0))) {
		respond("Try: control player %d move %d %d", 0);
		return 0;
	    }

	    if (x <= 0 || y <= 0 || x >= 200000 || y >= 200000) {
		respond("You want to move him where?", 0);
		return 0;
	    }
	    victim->p_x = x;
	    victim->p_y = y;
	    sprintf(buf, "%s (%2s) has been moved to %d %d by %s (%2s).",
		    victim->p_name, twoletters(victim),
		    x, y,
		    me->p_name, twoletters(me));
	    if (!god_silent)
		pmessage(buf, 0, MALL, MCONTROL);
	    return 1;
	}

    case HELPTOK:		/* fall through */
    default:
	respond_with_help_string(available_cmds);
	return 0;
    }
}

static int 
parse_control(char *str)
{
    char    buf[120];
    struct player *victim;
    int     i;
    int     godliness = me->p_stats.st_royal - GODLIKE + 1;

    static struct control_cmd available_cmds[] = {
	{"help", HELPTOK, 0},
	{"freeslot", FREESLOTTOK, "freeslot %p"},
	{"player", PLAYERTOK, "player ..."},
	{"robot", ROBOTTOK, "robot [args]"},
	{"snake", SNAKETOK, "snake [args]"},
	{"quiet", QUIETTOK, "quiet"},
	{"nukegame", NUKEGAMETOK, "nukegame"},
	{"restart", RESTARTTOK, "restart"},
	{"galaxy", NEWGALAXY, "galaxy"},
	{"shiptimer", SHIPTIMERTOK, "shiptimer (teamstr|shipstr)*"},
	{"allow", ALLOWTOK, "allow [teams]"},
	{0}
    };
    char   *nexttoken;

    if (godliness <= 0) {
	return 0;		/* "fail" silently.  Don't advertise divine
				   powers to peasants. */
    }


    switch (next_token(str, available_cmds, &nexttoken)) {
    case FREESLOTTOK:
	{
	    int slot = get_slotnum(nexttoken, (char**)0);
	    if (slot<0) {
		respond("\"control freeslot\" requires slot number.", 0);
		return 1;
	    }
	    victim = &players[slot];
	    if (victim->p_ntspid)
		kill(victim->p_ntspid, SIGHUP);

	    victim->p_status = PFREE;
	    victim->p_ntspid = 0;

	    if (!god_silent) {
		sprintf(buf, "Player slot %s has been freed by %s (%2s).",
			nexttoken, me->p_name, twoletters(me));
		pmessage(buf, 0, MALL, MCONTROL);
	    }
	}
	return 1;

    case ALLOWTOK:
	{
	    int     newlock = 0;
	    int	team;
	    char	*s;
	    if (0==*nexttoken) {
		newlock = ALLTEAM;
	    } else {
		for (s = nexttoken; get_teamid(s, &team, &s); ) {
		    newlock |= idx_to_mask(team);
		}
		if (*s) {
		    respond("Usage: control allow [fed] [rom] [kli] [ori]", 0);
		    return 1;
		}
	    }

	    status2->nontteamlock = newlock;
	    strcpy(buf, "Allowed teams now set to:");
	    if (status2->nontteamlock == ALLTEAM) {
		strcat(buf, " <all teams>");
	    }
	    else {
		if (status2->nontteamlock & FED)
		    strcat(buf, " fed");
		if (status2->nontteamlock & ROM)
		    strcat(buf, " rom");
		if (status2->nontteamlock & KLI)
		    strcat(buf, " kli");
		if (status2->nontteamlock & ORI)
		    strcat(buf, " ori");
	    }
	    respond(buf, 0);
	}
	return 1;
    case PLAYERTOK:
	return parse_control_player(nexttoken);

    case ROBOTTOK:
	{
	    int     pid;
	    char	*s;

	    pid = fork();
	    if (pid == 0) {
		char	*argv[40];
		argv[0] = build_path(ROBOT);

		s = nexttoken;
		for (i=1; 1; i++) {
		    int size=80;
		    argv[i] = malloc(size);
		    if (!get_one_token(s, argv[i], size, &s))
			break;
		    realloc(argv[i], strlen(argv[i])+1);
		}
		free(argv[i]);
		argv[i] = 0;

		execvp(argv[0], argv);
		fprintf(stderr, "Ack! Unable to exec %s\n", argv[0]);
		exit(1);
	    }
	    else if (pid < 0) {
		respond("Unable to fork robot", 0);
	    }
	    else {
		sprintf(buf, "Robot forked (pid %d) with arguments %s",
			pid, nexttoken);
		respond(buf, 1);
	    }
	}
	return 1;

    case SNAKETOK:
	{
	    int     pid;
	    char        *s;

	    pid = fork();
	    if (pid == 0) {
		char    *argv[40];
		argv[0] = build_path(SNAKE);

		s = nexttoken;
		for (i=1; 1; i++) {
		    int size=80;
		    argv[i] = malloc(size);
		    if (!get_one_token(s, argv[i], size, &s))
			break;
		    realloc(argv[i], strlen(argv[i])+1);
		}
		free(argv[i]);
		argv[i] = 0;

		execvp(argv[0], argv);
		fprintf(stderr, "Ack! Unable to exec %s\n", argv[0]);
                exit(1);
	    }
	    else if (pid < 0) {
		respond("Unable to fork snake", 0);
	    }
	    else {
		sprintf(buf, "Snake forked (pid %d) with arguments %s",
			pid, nexttoken);
		respond(buf, 1);
	    }
	}
	return 1;

    case QUIETTOK:
	if (godliness < 2) {
	    respond("No sneaking allowed", 0);
	    return 1;
	}
	sprintf(buf, "Switching to %s mode.", god_silent ? "loud" : "quiet");
	respond(buf, 0);
	god_silent = !god_silent;
	return 1;
    case NUKEGAMETOK:
	warning("Nuking game.  Have a nice day.");
	if (!god_silent) {
	    sprintf(buf, "The game has been nuked by %s (%2s).",
		    me->p_name, twoletters(me));
	    pmessage(buf, 0, MALL, MCONTROL);
	}
	kill(status->nukegame, 15);
	return 1;
    case RESTARTTOK:
	warning("Attempting daemon restart.");
	startdaemon(status2->league, 1);
	return 1;
    case NEWGALAXY:
	explode_everyone(KPROVIDENCE, 0);
	if (!god_silent) {
	    sprintf(buf, "The galaxy has been reset by %s (%2s).",
		    me->p_name, twoletters(me));
	    pmessage(buf, 0, MALL, MCONTROL);
	}
	status2->newgalaxy = 1;
	warning("Creating new galaxy");
	return 1;
    case SHIPTIMERTOK:
	{
	    int     teammask = 0;
	    int     shipmask = 0;
	    int     i, j;
	    char	*s=nexttoken;
	    while (1) {
		if (get_shipid(s, &j, &s))
		    shipmask |= 1<<j;
		else if (get_teamid(s, &j, &s))
		    teammask |= idx_to_mask(j);
		else if (*s) {
		    respond("Usage:", 0);
		    respond("control shiptimers (fed|rom|kli|ori)* (sc|dd|ca|bb|as|sb|at|js|fr|wb)*", 0);
		    respond("  resets the ship timers.", 0);
		    return 0;
		} else
		    break;
	    }
	    for (i = 0; i < NUMTEAM; i++) {
		int     teammask = idx_to_mask(i);
		if (teammask && !(teammask & (1 << i)))
		    continue;

		for (j = 0; j < NUM_TYPES; j++) {
		    if (shipmask && !(shipmask & (1 << j)))
			continue;

		    if (teams[teammask].s_turns[j]) {
			sprintf(buf, "%s %s reset", teams[teammask].name, shipvals[j].s_name);
			respond(buf, 0);
			teams[teammask].s_turns[j] = 0;
		    }
		}
	    }
	}
	return 1;
    case HELPTOK:		/* fall through */
    default:
	respond_with_help_string(available_cmds);
	return 0;
    }
}

/*
 *
 */

static int 
parse_info(char *cmd)
{
    char    buf[120];
    char	*nexttoken;

    static struct control_cmd available_cmds[] = {
	{"help", HELPTOK, 0},
	{"shiptimer", SHIPTIMERTOK, "shiptimer (teamstr|shipstr)*"},
	{0}
    };

    switch (next_token(cmd, available_cmds, &nexttoken)) {
    case SHIPTIMERTOK:
	{
	    int     race = 0;
	    int     i, j;
	    int	    anydead=0;
	    for (i = 0; i < NUMTEAM; i++) {
		int     teammask = idx_to_mask(i);
		if (race && !(race & (1 << i)))
		    continue;
		for (j = 0; j < NUM_TYPES; j++) {
		    if (teams[teammask].s_turns[j]) {
			sprintf(buf, "%s %s: %d minutes", teams[teammask].name,
			    shipvals[j].s_name, teams[teammask].s_turns[j]);
			anydead=1;
			respond(buf, 0);
		    }
		}
	    }
	    if (!anydead)
	      respond("All ships are available", 0); 
	}
	return 1;
    case HELPTOK:
    default:
	respond("Available subcommands: shiptimer", 0);
	return 1;
    }
}

static int 
parse_player(char *cmd)
{
    static int passver = 0;
    char    buf[80];
    char	*nexttoken;

    static struct control_cmd available_cmds[] = {
	{"help", HELPTOK, 0},
	{"password", PASSWDTOK, "password %s"},
	{"passwd", PASSWDTOK, 0},
	{"ratings", RATINGSTOK, "ratings"},
	{"rank", RANKTOK, "rank"},
	{0}
    };

    if (0==*cmd) {
	if (passver)
	    respond("Password change cancelled.", 0);

	passver = 0;
	return 1;
    }

    switch (next_token(cmd, available_cmds, &nexttoken)) {
    case PASSWDTOK:
	{
	    static char  newpass[16];

	    if (me->p_pos < 0) { /* guest login */
		respond("You don't have a password!", 0);
	    } else if (*nexttoken==0) {
		respond("\"player password\" requires new password as argument.", 0);
		respond("  example: \"player password lh4ern\"", 0);
	    } else if (!passver) {
                memset(newpass, 0, 16);
                strncpy(newpass, crypt(nexttoken, me->p_name), 15);
		respond("Repeat \"player password\" command to verify new password.", 0);
		respond(" or send \"player\" (no arguments) to cancel.", 0);
		passver = 1;
	    } else {
		char   *paths;
		int	fd;

                char    tmpbuf[16];
                memset(tmpbuf, 0, 16);
                strncpy(tmpbuf, crypt(nexttoken, me->p_name), 15);
                if (!strcmp(newpass, tmpbuf)) {
		    /* perhaps it'd be better to put this part in */
		    /* a different place */
		    paths = build_path(PLAYERFILE);
		    fd = open(paths, O_WRONLY, 0644);
		    if (fd >= 0) {
			lseek(fd, 16 + me->p_pos * sizeof(struct statentry), 0);
			write(fd, newpass, 16);
			close(fd);
			respond("Password changed.", 0);
		    }
		    else {
			respond("open() of playerfile failed, password not changed.", 0);
		    }
		}
		else
		    respond("Passwords did not match, password unchanged.", 0);
		passver = 0;
	    }
	}
	return 1;
    case RATINGSTOK:		/* print your ratings */
	{
	    struct rating r;
	    compute_ratings(me, &r);

	    sprintf(buf, "Bomb:%5.2f   Plnts:%5.2f   Rsrcs:%5.2f    Dshs:%5.2f   Offns:%5.2f", r.bombrat, r.planetrat, r.resrat, r.dooshrat, r.offrat);
	    respond(buf, 0);

	    sprintf(buf, "  JS:%5.2f      SB:%5.2f      WB:%5.2f   Ratio:%5.2f", r.jsrat, r.sbrat, r.wbrat, r.ratio);
	    respond(buf, 0);

	    sprintf(buf, "Overall Ratings:  Battle:%5.2f   Strat:%5.2f   Spec. Ship:%5.2f", r.battle, r.strategy, r.special);
	    respond(buf, 0);
	}
	return 1;

    case RANKTOK:		/* print the requirements for the next rank */
	{
	    int rank;
	    rank = me->p_stats.st_rank;
	    strcpy(buf, "Your current rank is ");
	    strcat(buf, ranks[rank].name);
	    respond(buf, 0);
	    if (rank == NUMRANKS - 1)
		return 1;

	    sprintf(buf, "To make the next rank (%s) you need:",
		    ranks[rank + 1].name);
	    respond(buf, 0);

	    sprintf(buf, "    Genocides: %d   DI: %.2f   Battle: %.2f",
		    ranks[rank + 1].genocides, ranks[rank + 1].di,
		    ranks[rank + 1].battle);
	    respond(buf, 0);

	    sprintf(buf, "    Strategy: %.2f   Spec. Ships: %.2f",
		    ranks[rank + 1].strategy, ranks[rank + 1].specship);
	    respond(buf, 0);
	}
	return 1;

    case HELPTOK:
    default:
	respond_with_help_string(available_cmds);
	return 1;
    }
}

/*
 *
 */

static void 
umpire_speak(char *msg)
{
    pmessage(msg, -1, MALL, UMPIRE);
}

static void 
talk_about_team(struct league_team *team, char *type)
{
    char    buf[120];
    struct player *captain;

    if (team->captain >= 0)
	captain = &players[team->captain];
    else
	captain = 0;

    sprintf(buf, "The %s team is named `%s'.", type, team->name);
    respond(buf, 0);

    if (captain)
	sprintf(buf, "  %s (%s) is their captain.", captain->p_name,
		twoletters(captain));
    else
	strcpy(buf, "  They have not chosen a captain yet.");
    respond(buf, 0);

    if (team->index >= 0) {
	sprintf(buf, "  They have chosen the %s",
		teams[idx_to_mask(team->index)].name);
    }
    else {
	strcpy(buf, "  They have not chosen an empire yet.");
    }
    respond(buf, 0);
}

static int 
team_really_ready(struct league_team *team)
{
    if (team->index < 0) {
	respond("You haven't chosen an empire", 1);
	return 0;
    }
    if (team->name[0] == 0) {
	respond("You haven't chosen a name", 1);
	return 0;
    }
    return 1;
}

static void 
trydefect(struct player *victim, enum HomeAway dest, char *destname, 
          enum HomeAway from, char *fromname, struct player *actor)
{
    char    buf[120];
    struct league_team *fromteam =
    (from == AWAY) ? &status2->away : &status2->home;

    if (victim->p_status==PFREE) {
      respond("Uh, he's not in the game.",1);
      return;
    }

    if (victim->p_homeaway == dest) {
	sprintf(buf, "%s already belong to the %s team",
		actor == victim ? "You" : "They", destname);
	respond(buf, 1);
	return;
    }
    if (actor->p_homeaway != from) {
	sprintf(buf, "You don't belong to the %s team.  You can't kick him off.",
		fromname);
	respond(buf, 1);
	return;
    }
    if (fromteam->captain == actor->p_no) {
	if (victim == actor) {
	    if (status2->league > 1 || status2->home.ready || status2->away.ready) {
		respond("You can't defect in the middle of the game.  You're the captain!", 1);
		return;
	    }
	    sprintf(buf, "%s (%s), the captain of the %s team, has defected!",
		    victim->p_name, twoletters(victim), fromname);
	    umpire_speak(buf);
	}
	else {
	    sprintf(buf, "%s (%s) has kicked %s (%s) off his team.",
		    actor->p_name, twoletters(actor),
		    victim->p_name, twoletters(victim));
	    umpire_speak(buf);
	}
    }
    else {
	if (victim == actor) {
	    if (status2->league > 1 || status2->home.ready || status2->away.ready) {
		respond("Only the captain can kick you off now.", 1);
		return;
	    }
	    sprintf(buf, "%s (%s) has defected to the %s team!",
		    victim->p_name, twoletters(victim),
		    destname);
	    umpire_speak(buf);
	}
	else {
	    respond("Only the captain can kick other people off the team.", 1);
	    return;
	}
    }
    victim->p_homeaway = dest;
    victim->p_status = PEXPLODE;
    victim->p_whydead = KPROVIDENCE;
    victim->p_explode = 1;
}

static int 
parse_league(char *subcommand)
{
    struct league_team *myteam, *otherteam;
    char   *teamtype;
    char    buf[120];
    int     i;
    int     iscaptain;
    char   *nexttoken;
    static char captain_only[] = "That command is reserved for team captains.";
    static struct control_cmd available_cmds[] = {
	{"help", HELPTOK, 0},
	{"captain", CAPTAINTOK, "captain [%d]"},
	{"time", TIMETOK, "time [%d %d]"},
	{"pass", PASSTOK, "pass"},
	{"start", STARTTOK, "start [%d]"},
	{"restart", RESTARTTOK, "restart [%d]"},
#if 0
	{"timeout", TIMEOUTTOK, "timeout [%d]"},
#endif
	{"teamname", TEAMNAMETOK, "teamname %s"},
	{"information", INFOTOK, "information"},
	{"away", AWAYTOK, "away [%d]"},
	{"home", HOMETOK, "home [%d]"},
	{"newgalaxy", NEWGALAXY, "newgalaxy [%d]"},
	{"pause", PAUSETOK, "pause"},
	{"continue", CONTINUETOK, "continue"},
	{"maxplayer", MAXPLAYERTOK, "maxplayer [%d]"},
	{"freeslot", FREESLOTTOK, "freeslot %d"},
	{0}
    };

    switch (me->p_homeaway) {
    case HOME:
	myteam = &status2->home;
	otherteam = &status2->away;
	teamtype = "home";
	break;
    case AWAY:
	myteam = &status2->away;
	otherteam = &status2->home;
	teamtype = "away";
	break;
    default:
	respond("WHOA! internal error.  You aren't on a team!", 0);
	respond("I'm afraid I'm going to have to kill you", 0);
	me->p_status = PEXPLODE;
	me->p_explode = 1;
	return 0;
    }

    iscaptain = (myteam->captain == me->p_no);

    /********************/

    if (get_teamid(subcommand, &i, (char**)0)) {
	if (!iscaptain) {
	    respond(captain_only, 1);
	    return 1;
	}
	if (status2->league != 1) {
	    respond("The game has started.  You can't change your mind now.", 1);
	    return 1;
	}
	if ((myteam->ready || otherteam->ready) && myteam->index >= 0) {
	    respond("One of the teams is ready.  You can't change your mind now.", 1);
	    return 1;
	}
	if (otherteam->index >= 0) {
	    if (i == otherteam->index) {
		respond("The other team has already chosen that empire", 1);
		return 1;
	    }
	}
	else {
	    if (me->p_homeaway == HOME) {
		if (!status2->awaypassed) {
		    respond("Away team gets first choice of empire.", 1);
		    return 1;
		}
	    }
	    else /* away */ if (myteam->index >= 0 && 0 == status2->awaypassed) {
		respond("Give the other team a chance to choose a side, will ya?", 1);
		return 1;
	    }
	    else if (status2->awaypassed == 1) {
		respond("You passed the choice of empire.  You have to wait for their choice.", 1);
		return 1;
	    }
	}

	if (i == myteam->index) {
	    respond("That already IS your empire.", 1);
	    return 1;
	}
	if (i < 0) {
	    respond("You can't change your mind without a reset.  Ask for one", 1);
	    return 1;
	}
	else
	    sprintf(buf, "The %s team has chosen the %s for their empire.",
		    teamtype, teams[idx_to_mask(i)].name);
	umpire_speak(buf);

	myteam->index = i;

	return 1;
    } else switch (next_token(subcommand, available_cmds, &nexttoken)) {
    default:
    case HELPTOK:		/********************/
	if (iscaptain) {
	    respond_with_help_string(available_cmds);
	}
	else {
	    respond("Available commands: captain [ %d ], time, information, maxplayer.", 0);
	}
	return 1;

    case CAPTAINTOK:		/********************/
	{
	    int	j;
	    i = !get_int(nexttoken, &j, (char**)0) || j;
	}
	if (i) {
	    if (myteam->captain < 0 ||
		players[myteam->captain].p_status != PALIVE ||
		players[myteam->captain].p_team != me->p_team) {
		if (myteam->captain >= 0) {
		    /*
		       safety valve in case the person is ghostbusted or on
		       another team
		    */
		    sprintf(buf, "%s has been overthrown as captain of the %s team",
			    players[myteam->captain].p_name, teamtype);
		    umpire_speak(buf);
		}
		respond("OK.  *POOF* you're a captain!", 1);
		sprintf(buf, "%s (%s) is now fearless leader of the %s team!",
			me->p_name, twoletters(me), teamtype);
		umpire_speak(buf);
		myteam->captain = me->p_no;
	    }
	    else if (iscaptain) {
		respond("Listen, silly.  You already are captain.  No point in rubbing it in", 1);
	    }
	    else {
		/* if myteam->captain were <0, we wouldn't get here */
		struct player *capn = &players[myteam->captain];
		sprintf(buf, "Sorry, %s (%s) is already captain of your team.",
			capn->p_name, twoletters(capn));
		respond(buf, 1);
	    }
	}
	else {
	    if (iscaptain) {
		respond("Wimp.  We didn't want you for captain anyway.", 1);
		sprintf(buf, "%s (%s) has chickened out.",
			me->p_name, twoletters(me));
		umpire_speak(buf);
		sprintf(buf, "Who now will lead the %s team?", teamtype);
		umpire_speak(buf);
		myteam->captain = -1;
	    }
	    else {
		respond("You can't quit being a captain.  You weren't one in the first place.", 1);
	    }
	}
	return 1;

    case TIMETOK:		/********************/
	if (0==*nexttoken) {
	    switch (status2->league) {
	    case 2:
		sprintf(buf, "%d seconds left in pre-tourney warm-up.",
			status2->leagueticksleft / SECONDS(1));
		break;
	    case 3:
		sprintf(buf, "%d minutes left in regulation play.",
			status2->leagueticksleft / MINUTES(1));
		break;
	    case 4:
		sprintf(buf, "%d minutes left in overtime.",
			status2->leagueticksleft / MINUTES(1));
		break;
	    default:
		sprintf(buf, "game is configured for %d minutes (%d overtime).",
			configvals->regulation_minutes, configvals->overtime_minutes);
		break;
	    }
	    respond(buf, 0);
	    return 1;
	}
	else if (!iscaptain) {
	    respond(captain_only, 1);
	    return 1;
	}
	else if (status2->league != 1) {
	    respond("You can only adjust the time parameters during the configuration phase", 1);
	    return 1;
	}
	else if (otherteam->ready) {
	    respond("The other team is ready to start.  You can't change the game params NOW.", 1);
	    return 1;
	}
	else if (!get_int(nexttoken,&myteam->desired.regulation, &nexttoken)
		 || !get_int(nexttoken, &myteam->desired.overtime, (char**)0)){
	    respond("Usage: time [ %d %d ]", 1);
	    return 1;
	}

	if (status2->home.desired.regulation == status2->away.desired.regulation
	    && status2->home.desired.overtime == status2->away.desired.overtime) {
	    configvals->regulation_minutes = status2->home.desired.regulation;
	    configvals->overtime_minutes = status2->home.desired.overtime;
	    sprintf(buf, "The captains have agreed to a %d minute game (%d overtime).", configvals->regulation_minutes, configvals->overtime_minutes);
	    umpire_speak(buf);
	}
	else {
	    sprintf(buf, "The %s team wishes a game of %d minutes (%d overtime)",
	    teamtype, myteam->desired.regulation, myteam->desired.overtime);
	    umpire_speak(buf);
	}

	return 1;

    case PASSTOK:		/********************/
	if (status2->league != 1) {
	    respond("The time for that is long past.", 1);
	    return 1;
	}
	if (!iscaptain) {
	    respond(captain_only, 1);
	    return 1;
	}
	if (me->p_homeaway == AWAY && status2->awaypassed) {
	    respond("You already passed the choice of empire.", 1);
	    return 1;
	}
	else if (me->p_homeaway == HOME && status2->awaypassed == 0) {
	    respond("You can't possibly pass the choice of empire.  You don't HAVE it!", 1);
	    return 1;
	}
	else if (status2->awaypassed > 1) {
	    respond("You both passed already, so get on with it.  (indecisive wishy-washy cretins)", 1);
	    return 1;
	}
	status2->awaypassed++;

	sprintf(buf, "The %s team has passed the choice of empire", teamtype);
	umpire_speak(buf);

	if (status2->awaypassed > 1) {
	    umpire_speak("Computer choosing randomly for both teams");
	    if (status2->home.index < 0) {
		status2->home.index = lrand48() % ((status2->away.index < 0) ? 4 : 3);
		if (status2->away.index >= 0 &&
		    status2->home.index >= status2->away.index)
		    status2->home.index++;
	    }
	    if (status2->away.index < 0) {
		status2->away.index = lrand48() % 3;
		if (status2->away.index >= status2->home.index)
		    status2->away.index++;
	    }
	}
	return 1;

    case STARTTOK:		/********************/
	if (status2->league != 1) {
	    respond("The game has already started.", 1);
	    return 1;
	}
	if (!iscaptain) {
	    respond(captain_only, 1);
	    return 1;
	}
	if (get_int(nexttoken, &myteam->ready, (char**)0) && !myteam->ready) {
	    sprintf(buf, "The %s team is not ready.", teamtype);
	    umpire_speak(buf);
	    return 0;
	}
	myteam->ready = 1;
	if (!team_really_ready(myteam)) {
	    respond("Your team is not really ready.  You need a name and an empire.", 0);
	    myteam->ready = 0;
	    return 0;
	}

	if (otherteam->ready && !team_really_ready(otherteam)) {
	    otherteam->ready = 0;
	    sprintf(buf, "The %s team was ready but the other wasn't.", teamtype);
	}
	else {
	    sprintf(buf, "The %s team is ready to start now.", teamtype);
	}
	umpire_speak(buf);


	if (otherteam->ready) {
	    /* shit! we're good to go! */

	    umpire_speak("Both sides are ready.  Let the carnage begin!");
	    umpire_speak("Everybody dies.  T-mode starts in 1 minute.");
	    status2->league = 2;
	    status2->leagueticksleft = 60 * TICKSPERSEC;

	    /* version */

	    explode_everyone(KTOURNSTART, 0);
	}
	return 1;

    case RESTARTTOK:		/********************/
	if (status2->league != 1) {
	    respond("The game has started.  You can't change your mind now.", 1);
	    return 1;
	}

	if (!iscaptain) {
	    respond(captain_only, 1);
	    return 1;
	}

	myteam->desired.restart = !get_int(nexttoken, &i, (char**)0) || i;

	sprintf(buf, myteam->desired.restart ?
		"%s (%s) would like to restart team selection." :
		"%s (%s) is satisfied with the teams.",
		me->p_name, twoletters(me));
	umpire_speak(buf);

	if (status2->home.desired.restart && status2->away.desired.restart) {
	    umpire_speak("Both captains have agreed to restart team selection.");

	    status2->awaypassed = 0;
	    status2->home.index = status2->away.index = -1;

	    status2->home.ready = status2->away.ready = 0;

	    status2->home.desired.restart = status2->away.desired.restart = 0;
	}

	return 1;

    case TIMEOUTTOK:
	respond("NYI", 0);
	return 1;

    case TEAMNAMETOK:		/********************/
	if (status2->league != 1) {
	    respond("The game has started.  You can't change your mind now.", 1);
	    return 1;
	}
	if (!iscaptain) {
	    respond(captain_only, 1);
	    return 1;
	}
	if (0==*nexttoken) {
	    respond("What do you want to call your team?\n", 1);
	    return 1;
	}
	strncpy(myteam->name, nexttoken, sizeof(myteam->name));
	myteam->name[sizeof(myteam->name) - 1] = 0;

	sprintf(buf, "Henceforth let the %s team be known as `%s'!",
		teamtype, myteam->name);
	umpire_speak(buf);

	return 1;

    case INFOTOK:		/********************/
	sprintf(buf, "The game will last for %d minutes (%d overtime)",
		configvals->regulation_minutes,
		configvals->overtime_minutes);
	respond(buf, 0);
	sprintf(buf, "Teams are limited to %d players on the field at once",
		configvals->playersperteam);
	respond(buf, 0);
	sprintf(buf, "You are on the %s team.", teamtype);
	respond(buf, 0);
	talk_about_team(&status2->home, "home");
	talk_about_team(&status2->away, "away");

	if (status2->awaypassed > 1)
	    umpire_speak("Both teams passed empire choice.  Computer assigned.");
	else if (status2->awaypassed)
	    umpire_speak("Away has passed empire choice to the Home team");
	else
	    umpire_speak("Away chooses empire first");

	return 1;


    case AWAYTOK:		/********************/
	{
	    struct player *victim;
	    if (!*nexttoken)
		victim = me;
	    else {
		int	idx = get_slotnum(nexttoken, (char**)0);
		if (idx<0) {
		    respond("`league away' requires a valid slot number", 0);
		    return 1;
		} else {
		    victim = &players[idx];
		}
	    }
	    trydefect(victim, AWAY, "away", HOME, "home", me);
	}
	return 1;

    case HOMETOK:		/********************/
	{
	    struct player *victim;
	    if (!*nexttoken)
		victim = me;
	    else {
		int	idx = get_slotnum(nexttoken, (char**)0);
		if (idx<0) {
		    respond("`league away' requires a valid slot number", 0);
		    return 1;
		} else {
		    victim = &players[idx];
		}
	    }
	    trydefect(victim, HOME, "home", AWAY, "away", me);
	}

	return 1;
    case NEWGALAXY:		/********************/

	if (status2->league != 1) {
	    respond("The game has started.  You can't change your mind now.", 1);
	    return 1;
	}

	if (myteam->ready || otherteam->ready) {
	    respond("You can't reset the galaxy now.  We're almost ready!", 1);
	    return 1;
	}
	if (!iscaptain) {
	    respond(captain_only, 1);
	    return 1;
	}

	{
	    int	j;
	    myteam->desired.galaxyreset =
		!get_int(nexttoken, &j, (char**)0) || j;
	}

	if (myteam->desired.galaxyreset) {
	    sprintf(buf, "%s (%s) is dissatisfied with the galaxy",
		    me->p_name, twoletters(me));
	}
	else {
	    sprintf(buf, "%s (%s) thinks the galaxy is just fine, thank you.",
		    me->p_name, twoletters(me));
	}
	umpire_speak(buf);

	if (status2->home.desired.galaxyreset &&
	    status2->away.desired.galaxyreset) {
	    umpire_speak("Both captains have agreed that the galaxy sucks.");
	    status2->newgalaxy = 1;
	    warning("Creating new galaxy");

	    status2->home.desired.galaxyreset = status2->away.desired.galaxyreset = 0;

	    status2->awaypassed = 0;
	    status2->home.index = status2->away.index = -1;
	    status2->home.ready = status2->away.ready = 0;
	    status2->home.desired.restart = status2->away.desired.restart = 0;
	}

	return 1;

    case PAUSETOK:		/********************/
	if (!iscaptain) {
	    respond(captain_only, 1);
	    return 1;
	}

	myteam->desirepause = 1;

	if (status2->home.desirepause && status2->away.desirepause) {
	    /* well, it's unanimous! */
	    status2->paused = SECONDS(10);
	    umpire_speak("The game has been paused");
	}
	else {
	    sprintf(buf, "The %s team wishes to PAUSE the game.", teamtype);
	    umpire_speak(buf);
	}

	status2->pausemsgfuse = 0;

	return 1;

    case CONTINUETOK:		/********************/
	if (!iscaptain) {
	    respond(captain_only, 1);
	    return 0;
	}

	myteam->desirepause = 0;

	sprintf(buf, "The %s team wishes to CONTINUE.", teamtype);
	umpire_speak(buf);

	status2->pausemsgfuse = 0;

	return 1;

    case MAXPLAYERTOK:
	{
	    int mp;
	    if (!get_int(nexttoken, &mp, (char**)0)) {
		sprintf(buf, "The game is currently configured for a maximum of %d players on each", configvals->playersperteam);
		respond(buf, 0);
		respond("team.", 0);
		if (configvals->playersperteam != myteam->desired.maxplayers) {
		    sprintf(buf, "You, however, want it to be %d.",
			    myteam->desired.maxplayers);
		    respond(buf, 0);
		}
	    }
	    else {
		if (!iscaptain) {
		    respond(captain_only, 1);
		    return 1;
		}

		if (mp < 1) {
		    respond("That's a stupid value for players-per-team.", 1);
		    return 1;
		}

		myteam->desired.maxplayers = mp;
		if (status2->away.desired.maxplayers
		    == status2->home.desired.maxplayers) {
		    configvals->playersperteam = mp;
		    sprintf(buf, "Captains have agreed to limit players per team to %d", mp);
		    umpire_speak(buf);
		}
		else {
		    sprintf(buf, "The %s team would like to limit the number of ",
			    teamtype);
		    umpire_speak(buf);
		    sprintf(buf, "players per team to %d", mp);
		    umpire_speak(buf);
		}
	    }
	}
	return 1;
    case FREESLOTTOK:
	{
	    int slotnum;
	    struct player *victim;
	    slotnum = get_slotnum(nexttoken, (char**)0);
	    if (slotnum<0) {
		respond("freeslot requires slot number", 1);
		return 0;
	    }

	    victim = &players[slotnum];
	    if (victim->p_ntspid)
		kill(victim->p_ntspid, SIGHUP);

	    victim->p_status = PFREE;
	    victim->p_ntspid = 0;

	    sprintf(buf, "Player slot %s has been freed by %s (%2s).",
		    nexttoken, me->p_name, twoletters(me));
	    umpire_speak(buf);
	}
	return 1;
    }
}
/*
 *
 */



/*
 * Parse command messages.
 */

int 
parse_command_mess(char *str, unsigned char who)
{
    char    buf[120];
    char   *nexttoken;
    int     godlike = me->p_stats.st_royal - GODLIKE + 1;

    static struct control_cmd available_cmds[] = {
	{"help", HELPTOK, 0},
	{"control", CONTROLTOK, "control ..."},
	{"league", LEAGUETOK, "league ..."},
	{"version", VERSIONTOK, "version"},
	{"player", PLAYERTOK, "player ..."},
	{"queue", QUEUETOK, "queue"}, {"tq", QUEUETOK, 0},
	{"information", INFOTOK, "information ..."},
	{"observe", OBSERVETOK, "observe"},
	{"parameters", PARAMTOK, "parameters"}, {"params", PARAMTOK, 0},
	{"cluecheck", CLUECHECKTOK, "cluecheck [%s]"},
	{0}
    };

    if (godlike < 0)
	godlike = 0;

    switch (next_token(str, available_cmds, &nexttoken)) {
    case HELPTOK:
	sprintf(buf, "Available commands: %s%s%s",
		godlike ? "control ..., " : "",
		status2->league ? "league ..., " :
		"",
		"version, player ..., queue,");
	respond(buf, 0);
	respond("   information ..., observe [%d], parameters, cluecheck [%s], help", 0);
	return 1;
    case CONTROLTOK:
	if (status2->league) {
		respond("God controls are disabled during league play.", 1);
	} else {
		return parse_control(nexttoken);
	}

    case VERSIONTOK:
	sprintf(buf, "NetrekII (Paradise), %s", PARAVERS);
	respond(buf, 0);
	return 1;

    case PLAYERTOK:
	return parse_player(nexttoken);

    case QUEUETOK:
	if (me->p_status == PTQUEUE) {
	    detourneyqueue();
	}
	else if (status->tourn)
	    respond("Dude, what are you waiting for!?  It's T-mode.  GO FIGHT!", 1);
	else if (!(me->p_flags & PFGREEN))
	    respond("Can not enter the tourney queue while at alert", 1);
	else if (me->p_damage != 0 || me->p_shield < me->p_ship.s_maxshield)
	    respond("Can not enter the tourney queue while damaged", 1);
	else if (me->p_armies > 0)
	    respond("Can not take armies into the tourney queue", 1);
	else {
	    /*
	       well, stick them on the queue.  They will be awoken when T
	       mode arrives
	    */

	    evaporate(me);

	    /* need code to blab about this */
	    me->p_status = PTQUEUE;
	    sprintf(buf, "%s has entered the tournament queue to wait for T-mode", me->p_name);
	    pmessage(buf, -1, MALL, "GOD->ALL");
	}
	return 1;

    case LEAGUETOK:
	if (status2->league) {
	    return parse_league(nexttoken);
	}
	else {
	    respond("League commands are disabled during non-league play.", 1);
	    return 1;
	}

    case INFOTOK:
	return parse_info(nexttoken);

    case OBSERVETOK:
	{
	    int     i;
	    i = !get_int(nexttoken, &i, (char**)0) || i;

	    if (i) {
		if (me->p_observer && me->p_status == POBSERVE) {
		    respond("You are already an observer.", 1);
		}
		else {
		    if (!(me->p_flags & PFGREEN))
			respond("Can not become an observer while at alert", 1);
		    else if (me->p_damage != 0 || me->p_shield < me->p_ship.s_maxshield)
			respond("Can not become an observer while damaged", 1);
		    else if (me->p_armies > 0)
			respond("Can not become an observer while carrying armies", 1);
		    else {
			evaporate(me);
			me->p_status = POBSERVE;
		    }
		    me->p_observer = 1;
		}
	    }
	    else {
		if (me->p_observer && me->p_status == POBSERVE) {
		    me->p_observer = 0;
		    if (!(status2->league && status->tourn))
			evaporate(me);
		}
	    }
	}
	return 1;

    case PARAMTOK:
	warning("Transmitting new game parameters");
	updateGameparams();
	return 1;
    case CLUECHECKTOK:
#if defined(CLUECHECK1) || defined(CLUECHECK2)
	return accept_cluecheck(nexttoken);
#else
        return 0;
#endif
    default:
	return 0;
    }
}
