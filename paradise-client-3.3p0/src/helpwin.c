/*
 * helpwin.c
 * copyright 1991 ERic mehlhaff
 * Free to use, hack, etc. Just keep these credits here.
 * Use of this code may be dangerous to your health and/or system.
 * Its use is at your own risk.
 * I assume no responsibility for damages, real, potential, or imagined,
 * resulting  from the use of it.
 *
 * Hacked into paradise by Bill Dyess
 */

#include "config.h"
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/* this is a set of routines that makes up a multi column help window,
   ** and shows just what the keymaps current keymap representation of the
   ** keys are.
   **
   **  fillhelp() handles the filling in if the strings for the help window
   **  update_Help_to_Keymap() checks the keymap and sets it up in hte
   ** help window.
   **

   **
   ** Format for each entry is follows:
   ** first character is the hard-coded character representation for the key
   ** in the keymap.  Useful for when you re-key things. This  could confuse
   ** people who do'nt know the keymap format.
   **
   ** the next few spaces are either spaces or  keys that also do that
   ** function.  Note that if you have more than 3 keys set to do the same
   ** thing, they will not be displayed.
   ** So, you could, I suppose map everything to 'Q' and it would not
   ** show, but that's a pretty bizarre situation.
   **

   **
   ** Bugs & Comments:
   ** You have to be sure that helpWin is defined to be big enough to handle
   ** all the messages.  That's pretty much a trial&error by-hand thing
   ** at this point
   **
   ** BZZZT!  Not anymore, done automatically [BDyess]
 */

/*  fills in the help window to note all keys also mapped to the
   **  listed functions
 */
void    update_Help_to_Keymap();

char   *help_message[] =
{
    " 0     Set speed 0",
    " 1     Set speed 1",
    " 2     Set speed 2",
    " 3     Set speed 3",
    " 4     Set speed 4",
    " 5     Set speed 5",
    " 6     Set speed 6",
    " 7     Set speed 7",
    " 8     Set speed 8",
    " 9     Set speed 9",
    "^0     Set speed 10",
    "^1     Set speed 11",
    "^2     Set speed 12",
    "^3     Set speed 13",
    "^4     Set speed 14",
    "^5     Set speed 15",
    "^6     Set speed 16",
    "^7     Set speed 17",
    "^8     Set speed 18",
    "^9     Set speed 19",
    "^)     Set speed 20",
    "^!     Set speed 21",
    "^@     Set speed 22",
    "^#     Set speed 23",
    "^$     Set speed 24",
    "^%     Set speed 25",
    "^^     Set speed 26",
    "^&     Set speed 27",
    "^*     Set speed 28",
    "^(     Set speed 29",
    " %     speed = maximum",
    " #     speed = 1/2 maximum",
    " <     slow speed 1",
    " >     speed up 1",
    " `     Afterburners",
    " -     Engage warp",
    " k     Set course",
    " p     Fire phaser",
    " t     Fire photon torpedo",
    " f     Fire plasma torpedo",
    " C     Switch special weapon",
    " d     detonate enemy torps",
    " D     detonate own torps",
    " L     List players",
    " P     List planets",
    " S     Status graph toggle",
    " Z     New Status graph toggle",
    " ]     Put up shields",
    " [     Put down shields",
    " u     Shield toggle",
    " s     Shield toggle",
    " i     Info on player/planet",
    " I     Extended info on player",
    "^i     Info on a planet",
    " b     Bomb planet",
    " z     Beam up armies",
    " x     Beam down armies",
    " {     Cloak",
    " }     Uncloak",
    " T     Toggle tractor beam",
    " y     Toggle pressor beam",
    " _     Turn on tractor beam",
    " ^     Turn on pressor beam",
    " $     Turn off tractor/pressor beam",
    " R     Enter repair mode",
    " o     Orbit planet or dock to outpost",
    " e     Docking permission toggle",
    " r     Refit (change ship type)",
    " Q     Quit",
    " q     Fast Quit",
    " ?     Message window toggle",
    " c     Cloaking device toggle",
    " l     Lock on to player/planet",
    " ;     Lock on to planet",
    " h     Help window toggle",
    " w     War declarations window",
    " N     Planet names toggle",
    " V     Rotate local planet display",
    " B     Rotate galactic planet display",
    " *     Send in practice robot",
    " E     Send Distress signal",
    " F     Send armies carried report",
    " U     Show rankings window",
    " m     Message Window Zoom",
    " '     Message Zoom + start to Team",
    " /     Toggle sorted player list",
    " :     Toggle message logging",
    " !     activate kitchen sink",
    " +     Show UDP options window",
    " =     Update all",
    " ,     Ping stats window",
    " M     Show MOTD window",
    " ~     Toggle PacketWindow",
    " \\     Update small",
    " |     Update medium",

    "       (space) Unmap special windows",

    " @     Reset dashboard timer",
    "^t     Cycle timer",
    "^m     Toggle map zoom",
    " K     Cycle playerlist",

    " X     Enter Macro Mode",
    " X?    Show current Macros",

    " &     Reread .paradiserc",
    " )     Rotate galaxy clockwise",
    " (     Rotate galaxy counter-clockwise",
    "^r     Stop recording",
  " \"     Toggle shell tools window",
    " j     Toggle course plot line",
    0
};

int     helpmessages = (sizeof(help_message) / sizeof(char *));
/* this is the number of help messages there are */

#define MAXHELP 40
/* maximum length in characters of key explanation */


void
fillhelp(void)
{
    register int i = 0, row, column;
    char    helpmessage[MAXHELP];


    /* 4 column help window. THis may be changed depending on font size */
    for (column = 0; column < 4; column++) {
	for (row = 1; row < helpmessages / 4 + 2; row++) {
	    if (help_message[i] == 0)
		break;
	    else {
		strcpy(helpmessage, help_message[i]);
		update_Help_to_Keymap(helpmessage);
		W_WriteText(helpWin, MAXHELP * column, row - 1, textColor,
			    helpmessage, strlen(helpmessage), 
			    W_RegularFont);
		i++;
	    }
	}
	if (help_message[i] == 0)
	    break;
    }
}


/*  this takes the help messages and puts in the keymap, so the player can
 * see just what does  what!
 *
 * ordinary format:       "U     Show rankings window",
 * translatedd here to    "[ sE  Computer options window",
 */
void
update_Help_to_Keymap(char *helpmessage)
{
    int     i, num_mapped = 0, key;

    key = helpmessage[1];
    if (helpmessage[0] == '^') {
	/* control character */
	key += 128;
    }
    if ((int) strlen(helpmessage) < 6) {
	return;
    }
    for (i = 0; i < 256; i++) {
	if (myship->s_keymap[i] != key)
	    continue;
	if (i == key)
	    continue;		/* it's already there!  don't add it! */

	/* we've found a key mapped to key! */
	/* the key is i */
	num_mapped += 1 + (i > 127) ? 1 : 0;
	if (num_mapped > 3)
	    continue;		/* we've found too many! */

	/* put the key in the string */
	if (i > 127) {
	    helpmessage[1 + num_mapped] = '^';
	    helpmessage[2 + num_mapped] = i - 128;
	} else {
	    helpmessage[2 + num_mapped] = i;
	}
    }


    /* clear spaces if any area available */
/*  switch (num_mapped)
    {
    case 0:
      helpmessage[3] = ' ';
    case 1:
      helpmessage[4] = ' ';
    case 2:
      helpmessage[5] = ' ';
    case 3:
    default:
      helpmessage[6] = ' ';
    }
*/
    return;
}
