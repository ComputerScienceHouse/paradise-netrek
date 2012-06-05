/*
 * option.c
 */
#include "copyright.h"

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#ifdef DEBUG
#define IFDEBUG(foo)    foo
#else
#define IFDEBUG(foo)
#endif

static int notdone;		/* not done flag */
static int oldzoom = -2;

/* kludgy way to make options that do some action when clicked: */
static int clearPhaserStats = 1;
static int reloadShipBitmaps = 1;

static int old_rotate, old_rotate_deg;

static int old_ping;

/* static int updateSpeed= 5;*/
static int lastUpdateSpeed = -1;

static char newkeys[14];
static char newckeys[14];
static char newbuttons[14];
static char newcbuttons[14];

static int remapAllShips = 1;

char *keymapmes[] = {
    "Key/Buttonmap only affect THIS ship type",
    "Key/Buttonmap affect ALL ship types",
    ""
};

char   *timermes[] =
{"Timer shows nothing (off)",
    "Timer shows time of day",
    "Timer shows time on server",
    "Timer shows time in ship",
    "Timer shows user-set time",
""};

char   *localmes[] =
{"Show owner on local planets",
    "Show facilities on local planets",
    "Show nothing on local planets",
    "Show surface properties on local planets",
    "Show MOO facilities on local planets",
""};

char   *galacticmes[] =
{"Show owner on galactic map",
    "Show facilities on galactic map",
    "Show nothing on galactic map",
    "Show surface properties on galactic map",
    "Show scout info age on galactic map",
    "Show MOO facilities on galactic map",
""};

char   *rotatemess[] =
{"Don't rotate galaxy",
    "Rotate galaxy 90 degrees",
    "Rotate galaxy 180 degrees",
    "Rotate galaxy 270 degrees",
    ""
};

char   *mapupdates[] =
{"Don't update galactic map",
    "Update galactic map frequently",
    "Update galactic map rarely",
""};

static char *lockoptions[] =
{"Don't show lock icon",
    "show lock icon on galactic map only",
    "Show lock icon on tactical map only",
"Show lock icon on both map windows", ""};

static char *phaseroptions[] =
{"Don't show phaser messages",
    "Phaser messages in kill window",
    "Phaser messages in phaser window",
    "Phaser messages in phaser window only",
    "Phaser messages in total review only",
""};

static char *dashboardoptions[] =
{"Old Dashboard",
    "New Dashboard",
    "Color Dashboard",
    "Rainbow Dashboard",
""};

static char *autoZoomOpts[] =
{"Don't auto-zoom map",
     "Auto-zoom map on Red OR Yellow Alert",
     "Auto-zoom map on Red Alert",
""};

static char *autoUnZoomOpts[] =
{"Don't auto-unzoom map",
     "Auto-unzoom map on Green alert",
     "Auto-unzoom map on Green OR Yellow Alert",
""};

static char *autoSetWarOpts[] =
{"Don't auto set war declarations",
        "Set war with non-zero player teams",
        "Set war with largest enemy team",
""};

/* useful for options that are an int with a range */
struct int_range {
    int     min_value;		/* value is >= this */
    int     max_value;		/* value is <= this */
    int     increment;		/* a click raises/lowers this amount */
};


/*
 * Only one of op_option, op_targetwin, and op_string should be defined. If
 * op_string is defined, op_size should be too and op_text is used without a
 * "Don't" prefix. if op_range is defined, there should be a %d in op_text
 * for it, op_size will be non-useful, and the 'Don't ' prefix won't appear
 */
struct option {
    char   *op_text;		/* text to display when on */
    int    *op_option;		/* variable to test/modify (optional) */
    W_Window *op_targetwin;	/* target window to map/unmap (optional) */
    char   *op_string;		/* string to modify (optional) */
    int     op_size;		/* size of *op_string (optional) */
    char  **op_array;		/* array of strings to switch between */
    struct int_range *op_range;	/* struct definint an integer range option */

    int     op_num;		/* used internally */
};

/* for the paged options menus */
struct option_menu {
    int     page_num;		/* page number of this menu */
    struct option_menu *Next;
    struct option *menu;	/* pointers to arrary of options */
    int     numopt;		/* number of options in this menu page */
    int     updated;		/* 1 if options can be changed externally */
};

/* pointer to first entry in the options menu list */
static
struct option_menu *FirstMenu = NULL;
static
struct option_menu *CurrentMenu = NULL;	/* menu currently looked at */
int     MenuPage = 1;		/* current menu page */
int     MaxOptions = 0;		/* maximum number of options in all menu
				   pages */
struct int_range MenuPages =
{1, 1, 1};

/* range of updates for keep-info-window-up option */
struct int_range keepInfo_range =
{0, 100, 1};

/* updates: use of the int range thing... */
struct int_range updates_range =
{1, 10, 1};

struct int_range redraw_delay_range =
{0, 10, 1};

/* range of menus. Will be updated when menu list is assembled */
struct int_range Menus_Range =
{1, 1, 1};

struct int_range clickDelay_range =
{0, 20, 1};

struct int_range beeplite_planet_range =
{0, 50, 1};
struct int_range beeplite_player_range =
{0, 50, 1};

struct int_range puck_arrow_size_range =
{0, 5, 1};

struct int_range zoom_override_range =
{0, 99, 1};

struct int_range statHeight_range =
{4,100, 4};

struct int_range planetChill_range =
{1,10,1};

/* menus */

struct option Features_Menu[] =
{
    {"Defaults Menu", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {"Page %d (click here to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {0, &mapmode, 0, 0, 0, mapupdates, NULL},
#if FIXME
    {0, &showgalactic, 0, 0, 0, galacticmes, NULL},
    {0, &showlocal, 0, 0, 0, localmes, NULL},
#endif /*FIXME*/
    {"show IND planets", &showIND, 0, 0, 0, NULL, NULL},
    {"send MOTD bitmaps", &sendmotdbitmaps, 0, 0, 0, NULL, NULL},
    {"reload ship bitmaps", &reloadShipBitmaps, 0, 0, 0, NULL, NULL},
    {"stay peaceful when reborn", &keeppeace, 0, 0, 0, NULL, NULL},
    {0, &remapAllShips, 0, 0, 0, keymapmes, NULL},
    {"new keymap entries: %s_", 0, 0, newkeys, 13, NULL, NULL},
    {"new ckeymap entries: %s_", 0, 0, newckeys, 13, NULL, NULL},
    {"new buttonmap entries: %s_", 0, 0, newbuttons, 13, NULL, NULL},
    {"new cbuttonmap entries: %s_", 0, 0, newcbuttons, 13, NULL, NULL},
    {"report kill messages", &reportKills, 0, 0, 0, NULL, NULL},
    {"keep info %d upds (0=forever)", &keepInfo, 0, 0, 0, 0, &keepInfo_range},
    {"%d updates per second", &updateSpeed, 0, 0, 0, 0, &updates_range},
    {"%d/10 sec screen refresh delay", &redrawDelay, 0, 0, 0, 0, &redraw_delay_range}, 
    {"collect ping stats", &ping, 0, 0, 0, NULL, NULL},
    {"avoid message kludge", &niftyNewMessages, 0, 0, 0, NULL, NULL},
    {"use continuous mouse", &continuousMouse, 0, 0, 0, NULL, NULL},
    {"%d updates repeat delay", &clickDelay, 0, 0, 0, 0, &clickDelay_range},
#ifdef UNIX_SOUND
    {"play sound effects", &playSounds, 0, 0, 0, NULL, NULL},
#endif
    {"done", &notdone, 0, 0, 0, NULL, NULL},
    {NULL, 0, 0, 0, 0, NULL, NULL, /**/ -1}
};

struct option Window_Menu[] =
{
    {"Window Menu", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {"Page %d (click here to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {"show \"all\" message window", 0, &messWin[WALL].window, 0, 0, NULL, NULL},
    {"show \"team\" message window", 0, &messWin[WTEAM].window, 0, 0, NULL, NULL},
    {"show \"your\" message window", 0, &messWin[WINDIV].window, 0, 0, NULL, NULL},
    {"show \"kill\" message window", 0, &messWin[WKILL].window, 0, 0, NULL, NULL},
    {"show \"phaser\" message window", 0, &messWin[WPHASER].window, 0, 0, NULL, NULL},
    {"show \"joined\" message window", 0, &messWin[WREVIEW].window, 0, 0, NULL, NULL},
    {"show ship statistics window", 0, &statwin, 0, 0, NULL, NULL},
    {"show new ship statistics window", 0, &newstatwin, 0, 0, NULL, NULL},
    {"show network statistics window", 0, &pStats, 0, 0, NULL, NULL},
    {"show help window", 0, &helpWin, 0, 0, NULL, NULL},
    {"show xtrekrc defaults window", 0, &defWin, 0, 0, NULL, NULL},
    {"show shell tools window", 0, &toolsWin, 0, 0, NULL, NULL},
    {0, &showPhaser, 0, 0, 0, phaseroptions, NULL},
    {"", &showLock, 0, 0, 0, lockoptions, NULL},
    {"show lock line", &lockLine, 0, 0, 0, NULL, NULL},
    {"sort planets by team", &mapSort, 0, 0, 0, NULL, NULL},
    {"enable message warp", &warp, 0, 0, 0, NULL, NULL},
    {"use info icon", &infoIcon, 0, 0, 0, NULL, NULL},
    {"done", &notdone, 0, 0, 0, NULL, NULL},
    {NULL, 0, 0, 0, 0, NULL, NULL, /**/ -1}
};

struct option Display_Menu[] =
{
    {"Features Display Menu", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {"Page %d (click here to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {0, &rotate, 0, 0, 0, rotatemess, NULL},
    {"draw background stars", &blk_showStars, 0, 0, 0, NULL},
    {"show warp/star streaks", &warpStreaks, 0, 0, 0, NULL, NULL},
    {"show tractor/pressor", &showTractorPressor, 0, 0, 0, NULL, NULL},
    {"show all tractors/pressors", &showAllTractorPressor, 0, 0, 0, NULL, NULL},
    {"show shields", &showShields, 0, 0, 0, NULL, NULL},
    {"show shield damage", &show_shield_dam, 0, 0, 0, NULL, NULL},
    {"show hull damage indicators", &vary_hull, 0, 0, 0, NULL, NULL},

    {"show tactical planet names", &namemode, 0, 0, 0, NULL, NULL},

    {"zoom galactic map", &blk_zoom, 0, 0, 0, NULL, NULL},
    {0,&autoZoom, 0, 0, 0, autoZoomOpts, NULL},
    {0,&autoUnZoom, 0, 0, 0, autoUnZoomOpts, NULL},
    {"Manual zoom overrides auto-zoom %d updates",&autoZoomOverride, 0, 0, 0, NULL, 
	 &zoom_override_range},
    {"draw galactic map grid", &drawgrid, 0, 0, 0, NULL},
    {"show sector numbers", &sectorNums, 0, 0, 0, NULL, NULL},
    {"show view box", &viewBox, 0, 0, 0, NULL, NULL},
    {0, &autoSetWar, 0, 0, 0, autoSetWarOpts, NULL},
    {"use RCD highlighting", &UseLite, 0, 0, 0, NULL, NULL},
    {"highlight/use default RCDs", &DefLite, 0, 0, 0, NULL, NULL},
    {"No. of updates to highlight player: %d", &beep_lite_cycle_time_player,
    0, 0, 0, NULL, &beeplite_player_range},
    {"No. of updates to highlight planet: %d", &beep_lite_cycle_time_planet,
    0, 0, 0, NULL, &beeplite_planet_range},
    {"show number of armies on local", &show_armies_on_local, 
    0, 0, 0, NULL, NULL},
    {"Planet rotation chill factor: %d", &planetChill, 0, 0, 0, NULL, 
    &planetChill_range},
    {"draw rounded asteroids", &rounded_asteroids, 0, 0, 0, NULL, NULL},
    {"done", &notdone, 0, 0, 0, NULL, NULL},
    {NULL, 0, 0, 0, 0, NULL, NULL, /**/ -1}
};

struct option Playerdash_Menu[] =
{
    {"Playerlist/Dashboard Menu", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {"Page %d (click here to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {"sort playerlist", &sortPlayers, 0, 0, 0, NULL, NULL},
    {"show dead players in playerlist", &showDead, 0, 0, 0, NULL, NULL},
    {"show players as they log in", &showPreLogins, 0, 0, 0, NULL, NULL},
    {"hide 0.00 kills in playerlist", &hideNoKills, 0, 0, 0, NULL, NULL},
    {"sort outfitting to bottom", &sortOutfitting, 0, 0, 0, NULL, NULL},
    {0, &Dashboard, 0, 0, 0, dashboardoptions, NULL},
    {0, &timerType, 0, 0, 0, timermes, NULL},
    {"show ship stats on local window",&localShipStats, 0, 0, 0, NULL, NULL},
    {"Local ship stats height: %d",&statHeight, 0, 0, 0, NULL, &statHeight_range},
    {"show packet lights", &packetLights, 0, 0, 0, NULL, NULL},
    {"keep phaser statistics", &phaserStats,0,0,0,NULL, NULL},
    {"clear phaser statistics", &clearPhaserStats, 0, 0, 0, NULL, NULL},
    {"done", &notdone, 0, 0, 0, NULL, NULL},
    {NULL, 0, 0, 0, 0, NULL, NULL, /**/ -1}
};

struct option Hockey_Menu[] =
{
    {"Hockey Menu", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {"Page %d (click here to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
    {"show tactical hockey lines", &tacticalHockeyLines, 0, 0, 0, NULL, NULL},
    {"show galactic hockey lines", &galacticHockeyLines, 0, 0, 0, NULL, NULL},
    {"show planets on the galactic", &cleanHockeyGalactic, 0, 0, 0, NULL, NULL},
    {"color hockey lines by team", &teamColorHockeyLines, 0, 0, 0, NULL, NULL},
    {"use Puck bitmap", &puckBitmap, 0, 0, 0, NULL, NULL},
    {"use Puck direction arrow", &puckArrow, 0, 0, 0, NULL, NULL},
    {"Puck Arrow Size: %d", &puckArrowSize, 0, 0,0,NULL,&puck_arrow_size_range},
    {"done", &notdone, 0, 0, 0, NULL, NULL},
    {NULL, 0, 0, 0, 0, NULL, NULL, /**/ -1}
};

#define NUMOPTIONS(menu) ((sizeof((menu))/sizeof((menu)[0]))-1)


/* option menu sizes and such */
#define OPTIONBORDER	BORDER
#define OPTIONLEN	40

static void RefreshOptions P((void));
static void SetMenuPage P((int pagenum));
static int InitOptionMenus P((void));
static void AddOptMenu P((struct option NewMenu[], int updated));
static int NumOptions P((struct option OpMenu[]));
/*static void UpdateOptions P((void));*/
static void optionrefresh P((register struct option * op));


/* Set up the option menus and window. */
void
optionwindow(void)
{
    /* Init not done flag */
    notdone = 1;

    old_ping = ping;
    old_rotate = rotate;
    old_rotate_deg = rotate_deg;

    *newkeys = '\0';
    *newckeys = '\0';
    *newbuttons = '\0';
    *newcbuttons = '\0';
    if (FirstMenu == NULL) {
	if (!paradise) {	/* remove the extra planet settings */
	    localmes[3] = "";
	    galacticmes[3] = "";
	}
	MaxOptions = InitOptionMenus();
	if (MaxOptions < 0) {
	    fprintf(stderr, "InitOptionMenus() error %d!\n", MaxOptions);
	    notdone = 0;
	    return;
	}
    }
    /* Create window big enough to hold option windows */
    if (optionWin == NULL) {

	optionWin = W_MakeMenu("option", winside + 10, 10, OPTIONLEN,
			       MaxOptions, baseWin, OPTIONBORDER);
	CurrentMenu = FirstMenu;

	RefreshOptions();
    }
    W_ResizeMenu(optionWin, OPTIONLEN, CurrentMenu->numopt);
    /* Map window */
    W_MapWindow(optionWin);
}

/* refresh all current options */
static void
RefreshOptions(void)
{
    int     i;
    struct option_menu *option;

    if (notdone == 0 || (option = CurrentMenu) == NULL)
	return;

    for (i = 0; i < option->numopt; i++) {
	optionrefresh(&(option->menu[i]));
    }
}

/* Redraw the specified option entry */
void
optionredrawtarget(W_Window win)
{
    register struct option *op;

    for (op = CurrentMenu->menu; op->op_text; op++) {
	if (op->op_targetwin && win == *op->op_targetwin) {
	    optionrefresh(op);
	    break;
	}
    }
}

/* Redraw the specified option option */
void
optionredrawoption(int *ip)
{
    register struct option *op;

    for (op = CurrentMenu->menu; op->op_num >= 0; op++) {
	if (ip == op->op_option) {
	    optionrefresh(op);
	    break;
	}
    }
}

/* Refresh the option window given by the option struct */
static void
optionrefresh(struct option *op)
{
    register int on;
    char    buf[BUFSIZ];

    if (op == NULL || notdone == 0)
	return;

    if (op->op_string) {
	(void) sprintf(buf, op->op_text, op->op_string);
    } else if (op->op_array) {	/* Array of strings */
	strcpy(buf, op->op_array[*op->op_option]);
    } else if (op->op_range) {
	(void) sprintf(buf, op->op_text, *(op->op_option));
    } else {
	/* Either a boolean or a window */
	if (op->op_option)
	    on = *op->op_option;/* use int for status */
	else if (op->op_targetwin)
	    on = W_IsMapped(*op->op_targetwin);	/* use window for status */
	else
	    on = 1;		/* shouldn't happen */

	if (!on)
	    strcpy(buf, "Don't ");
	else
	    buf[0] = '\0';
	strcat(buf, op->op_text);
    }

    if (islower(buf[0]))
	buf[0] = toupper(buf[0]);

    if (op->op_num == 0) {	/* title */
	W_WriteText(optionWin, 0, op->op_num, W_Yellow, buf,strlen(buf),0);
    } else if (op->op_num == 1) {	/* "click" entry */
	W_WriteText(optionWin, 0, op->op_num, W_Green, buf, strlen(buf),0);
    } else
	W_WriteText(optionWin, 0, op->op_num, textColor,buf,strlen(buf),0);
}

/* deal with events sent to the option window */
int
optionaction(W_Event *data)
{
    register struct option *op;
    int     i;
    register char *cp;

    if (data->y >= CurrentMenu->numopt) {
	W_Beep();
	return (0);
    }
    if (notdone == 0)
	return (0);

    op = &(CurrentMenu->menu[data->y]);

    /* Update string; don't claim keystrokes for non-string options */
    /* deal with options with string input first */
    if (op->op_string == 0) {
	if (data->type == W_EV_KEY)
	    return (0);
    } else {
	if (data->type == W_EV_BUTTON)
	    return (0);
	switch (data->key) {

	case '\b':		/* delete character */
	case '\177':
	    cp = op->op_string;
	    i = strlen(cp);
	    if (i > 0) {
		cp += i - 1;
		*cp = '\0';
	    }
	    break;

	case '\027':		/* word erase */
	    cp = op->op_string;
	    i = strlen(cp);
	    /* back up over blanks */
	    while (--i >= 0 && isspace(cp[i]));
	    i++;
	    /* back up over non-blanks */
	    while (--i >= 0 && !isspace(cp[i]));
	    i++;
	    cp[i] = '\0';
	    break;

	case '\025':		/* kill line */
	case '\030':
	    op->op_string[0] = '\0';
	    break;

	default:		/* add character to the list */
	    if (data->key < 32 || data->key > 127)
		break;
	    cp = op->op_string;
	    i = strlen(cp);
	    if (i < (op->op_size - 1) && !iscntrl(data->key)) {
		cp += i;
		cp[1] = '\0';
		cp[0] = data->key;
	    } else
		W_Beep();
	    break;
	}
    }

    /* Toggle int, if it exists */
    if (op->op_array) {
	if (data->key == W_LBUTTON) {
	    (*op->op_option)++;
	    if (*(op->op_array)[*op->op_option] == '\0') {
		*op->op_option = 0;
	    }
	} else if (data->key == W_MBUTTON) {
	    /* set option number to zero on the middle key to ease shutoff */
	    *op->op_option = 0;
	} else if (data->key == W_RBUTTON) {
	    /* if right button, decrease option  */
	    (*op->op_option)--;
	    /* if decreased too far, set to top option */
	    if (*(op->op_option) < 0) {
		*op->op_option = 0;
		while (*(op->op_array)[*op->op_option] != '\0') {
		    (*op->op_option)++;
		}
		(*op->op_option)--;
	    }
	}
#ifdef FIXME
	if(op->op_option == &showgalactic)
	    redrawall=1;
#endif /*FIXME*/
    } else if (op->op_range) {
	if (data->key == W_RBUTTON) {
	    (*op->op_option) -= op->op_range->increment;
	} else if (data->key == W_MBUTTON) {
	    (*op->op_option) = op->op_range->min_value;
	} else if (data->key == W_LBUTTON) {
	    (*op->op_option) += op->op_range->increment;
	}
	/* wrap value around within option range */
	if (*(op->op_option) > op->op_range->max_value)
	    *(op->op_option) = op->op_range->min_value;
	if (*(op->op_option) < op->op_range->min_value)
	    *(op->op_option) = op->op_range->max_value;
    } else if (op->op_option) {
	*op->op_option = !*op->op_option;
	if(op->op_option == &blk_zoom) {
	    if(!paradise) {
		blk_zoom=0;
	    } else {
		redrawall=1;
	    }
	} else if((op->op_option == &blk_showStars) && !paradise)
	    blk_showStars = 0;
	else if((op->op_option == &drawgrid) ||
		(op->op_option == &sectorNums))
	    redrawall=1;
	else if(op->op_option == &clearPhaserStats) {
	    phasFired=phasHits=totalDmg=0;
	    clearPhaserStats=1;
	    warning("Phaser statistics reset!");
	} else if(op->op_option == &reloadShipBitmaps) {
	    reloadShipBitmaps = 1;
	    warning("Kick a hacker to make it work");
	}
    }
    /* Map/unmap window, if it exists */
    if (op->op_targetwin) {
	if (W_IsMapped(*op->op_targetwin)) {
	    if (*op->op_targetwin == udpWin)
		udpdone();
	    else if (*op->op_targetwin == spWin)
		spdone();
	    else
		W_UnmapWindow(*op->op_targetwin);
	} else {
	    if (*op->op_targetwin == udpWin)
		udpwindow();
	    else if (*op->op_targetwin == spWin)
		spwindow();
	    else
		W_MapWindow(*op->op_targetwin);
	    if (*op->op_targetwin == pStats)
		redrawPStats();
	    if (*op->op_targetwin == defWin)
		showdef();
	}
    }
    /* deal with possible menu change */
    if (MenuPage != CurrentMenu->page_num) {
	SetMenuPage(MenuPage);
	RefreshOptions();
    }
    if (!notdone)		/* if done, that is */
	optiondone();
    else
	optionrefresh(op);


    return (1);
}

/*
 * find the menu in the menus linked list that matches the one in the *
 * argument
 */
static void
SetMenuPage(int pagenum)
{
    int     i = 1;
    if (FirstMenu != NULL)
	for (CurrentMenu = FirstMenu; CurrentMenu->Next != NULL &&
	     CurrentMenu->page_num != pagenum; i++, CurrentMenu = CurrentMenu->Next);
    W_ResizeMenu(optionWin, OPTIONLEN, CurrentMenu->numopt);
}

void
optiondone(void)
{
    int shpn;
    struct ship *shp;

    /* Unmap window */
    W_UnmapWindow(optionWin);

    if(remapAllShips) {
	for(shpn=0;shpn<nshiptypes;shpn++) {
	    shp=getship(shpn);
	    keymapAdd(newkeys, (char*)shp->s_keymap);
	    ckeymapAdd(newckeys, (char*)shp->s_keymap);
	    buttonmapAdd(newbuttons, (char*)shp->s_buttonmap);
	    cbuttonmapAdd(newcbuttons, (char*)shp->s_buttonmap);
	}
    } else { /* only affect current ship */
	/* update keymap and buttonmap [Bdyess] */
	keymapAdd(newkeys, (char*)myship->s_keymap);
	ckeymapAdd(newckeys, (char*)myship->s_keymap);
	buttonmapAdd(newbuttons, (char*)myship->s_buttonmap);
	cbuttonmapAdd(newcbuttons, (char*)myship->s_buttonmap);
    }

    /* optionrefresh(&(option[KEYMAP])); Not sure why this is really needed */

    sendOptionsPacket();	/* update server as to the client's options */

    if (updateSpeed != lastUpdateSpeed) {
	sendUpdatePacket(1000000 / updateSpeed);
	lastUpdateSpeed = updateSpeed;
    }
    if (oldzoom != blk_zoom) {
	redrawall = 1;
	oldzoom = blk_zoom;
    }
    if (ping != old_ping) {
	old_ping = ping;
	if (ping)
	    startPing();
	else
	    stopPing();
    }
    if (DefLite)
	litedefaults();

    if (rotate != old_rotate) {
	rotate_all();
	old_rotate = rotate;
	old_rotate_deg = rotate_deg;

    }
}

/* set up menus linked list */
static int
InitOptionMenus(void)
{
    int     i = 1;
    int     maxopts = 0;

    IFDEBUG(printf("Adding OptionMenus\n");
    )
    /* AddOptMenu( &OptionsMenu, 0); */
	AddOptMenu(Features_Menu, 0);
    AddOptMenu(Window_Menu, 0);
    AddOptMenu(Display_Menu, 0);
    AddOptMenu(Playerdash_Menu, 0);
    AddOptMenu(Hockey_Menu, 0);
    /* AddOptMenu(SillyFeatures_Menu, 0); */
    /* AddOptMenu(Network_Menu, 0); */

    for (i = 1, CurrentMenu = FirstMenu; CurrentMenu != NULL;
	 i++, CurrentMenu = CurrentMenu->Next) {
	CurrentMenu->page_num = i;	/* repage the menus.. */
	if (CurrentMenu->numopt > maxopts)
	    maxopts = CurrentMenu->numopt;
    }
    CurrentMenu = FirstMenu;
    Menus_Range.max_value = i - 1;
    IFDEBUG(printf("OptionMenus Added! Maxopt = %d \n", i);
    )
	return maxopts;
}

static void
AddOptMenu(struct option *NewMenu, int updated)
{
    struct option_menu *menuptr;
    struct option_menu *newmenu;
    int     i = 0;

    IFDEBUG(printf("AddOptMenu\n");
    )
	menuptr = FirstMenu;

    newmenu = (struct option_menu *) malloc(sizeof(struct option_menu));
    if (newmenu == NULL) {
	perror("Malloc Error adding a menu");
	return;
    }
    /* add to list */
    if (FirstMenu == NULL) {
	FirstMenu = newmenu;
    } else {
	for (i = 0, menuptr = FirstMenu; menuptr->Next != NULL; menuptr = menuptr->Next)
	    i++;
	menuptr->Next = newmenu;
    }
    newmenu->page_num = i;
    newmenu->Next = NULL;
    newmenu->numopt = NumOptions(NewMenu);
    newmenu->menu = NewMenu;
    newmenu->updated = updated;
    IFDEBUG(printf("Menu Added! \n", i);
    )
}

static int
NumOptions(struct option *OpMenu)
{
    int     i = 0;
    struct option *ptr;

    ptr = &OpMenu[0];
    for (i = 0; ptr->op_num != -1 && ptr->op_option != &notdone; i++) {
	IFDEBUG(printf("Option #%d..\n", i);
	)
	    IFDEBUG(if (ptr->op_text != NULL) printf("OP_Text:%s\n", ptr->op_text);
	)
	    ptr = &OpMenu[i];
	ptr->op_num = i;
    }

    IFDEBUG(printf("NumOptions in this menu: %d\n", i);
    )
	return i;
}
