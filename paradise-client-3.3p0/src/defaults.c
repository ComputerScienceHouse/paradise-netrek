/* defaults.c
 *
 * Kevin P. Smith  6/11/89
 * Bill Dyess      9/29/93  - moved all default settings to resetDefaults
 *                           from main
 * Bill Dyess	   11/8/93  - created expandFilename function
 * Bill Dyess      11/20/93 - added #include capability
 */

#include "copyright2.h"

#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#define MAXLINE 1024

/* list of files to search for [BDyess] */
static char *filelist[] = {".paradise/paradiserc", 
                           ".paradiserc",
			   ".netrekrc",
			   ".xtrekrc",
			   NULL};

/* Prototypes */
static FILE *findDefaults P((char *home, char **deffile, char *base));

/* expands ~ (home dir) and environment vars in filenames.  This is primarily
   for filenames specified in the .xtrekrc file, where the shell never gets
   a chance to expand them.  [BDyess] */
char *
expandFilename(char *filename)
{
    char    buf[MAXLINE], *src = filename, *dest = buf, tmpbuf[MAXLINE],
           *tmppntr, *envpntr;
    struct passwd *pw;

    if (!src)
	return filename;
    while (*src) {
	if (*src == '~') {
	    src++;
	    if (*src != '/' && *src) {	/* ~username */
		tmppntr = tmpbuf;
		while (isalpha(*src))
		    *tmppntr++ = *src++;
		*tmppntr = 0;
		pw = getpwnam(tmpbuf);
		if (!pw) {
		    fprintf(stderr, "Error: can't find username %s, inserting ~ literally.\n", tmpbuf);
		    *dest++ = '~';
		    strcpy(dest, tmpbuf);
		} else
		    strcpy(dest, pw->pw_dir);
	    } else {		/* just ~, insert $HOME */
		envpntr = getenv("HOME");
		if (envpntr != NULL)
		    strcpy(dest, envpntr);
		else
		    printf("HOME environment variable missing, ignoring ~.\n");
	    }
	    while (*dest)
		dest++;
	} else if (*src == '$') {
	    /* copy the shortest env var that matches */
	    /* if in {}'s, the work is done for us */
	    src++;
	    if (*src == '{') {
		src++;
		tmppntr = tmpbuf;
		while (*src != '}' && *src)
		    *tmppntr++ = *src;
		*tmppntr = 0;
		envpntr = getenv(tmpbuf);
		if (envpntr)
		    strcpy(dest, getenv(tmpbuf));
		while (*dest)
		    dest++;
	    } else if (*src) {	/* we have to work */
		tmppntr = tmpbuf;
		*tmppntr++ = *src++;
		*tmppntr = 0;
		while (!getenv(tmpbuf) && *src) {
		    *tmppntr++ = *src++;
		    *tmppntr = 0;
		}
		if (*src)
		    strcpy(dest, getenv(tmpbuf));
		while (*dest)
		    dest++;
	    }
	} else if (*src == '\\') {	/* escaped char */
	    src++;
	    if (*src)
		*dest++ = *src++;
	} else {		/* just a regular char */
	    *dest++ = *src++;
	}
    }
    *dest = 0;
    dest = (char *) realloc(filename, strlen(buf) + 1);
    strcpy(dest, buf);
    return dest;
}

void
freeDefaults(void)
{
    struct stringlist *tmp;
    while (defaults) {
	tmp = defaults;
	defaults = defaults->next;
	free(tmp->string);
	free(tmp->value);
	free(tmp);
    }
}

char *
initDefaults(char *deffile)
{
    FILE   *fp = NULL;
    char    buf[MAXLINE];
    char   *includeFile;
    char   *home;
    char   *v, *r;
    struct stringlist *new;
    int     ok, i;
    int     skip = 0;

    /* if(defaults) freeDefaults(); */
    if (!deffile) {
	deffile = (char *) malloc(256);
	home = getenv("HOME");
        for(i = 0;filelist[i];i++) {
	  /* home == NULL means search the current directory [BDyess] */
	  fp = findDefaults(NULL, &deffile, filelist[i]);
	  if(fp) break;
	  fp = findDefaults(home, &deffile, filelist[i]);
	  if(fp) break;
	}
    } else {
	fp = fopen(deffile, "r");
    }
    if (!fp)
	return deffile;
    printf("Reading defaults from %s.\n", deffile);
    while (fgets(buf, MAXLINE - 1, fp)) {
	if (skip) {
	    skip = strncasecmp(buf, "paradise-include", 16);
	    continue;
	} else {
	    skip = !strncasecmp(buf, "paradise-exclude", 16);
	    if (skip)
		continue;
	}
	/* if (*buf=='#' || *buf==';') continue; */
	v = buf;
	if (*buf == '#') {
	    if (strncasecmp(buf + 1, "include", 7) == 0) {	/* #include statement */
		v = buf + 8;
		ok = 0;
		while (*v == ' ' || *v == '<' || *v == '"') {
		    if (*v != ' ')
			ok = 1;
		    v++;
		}
		if (!ok)
		    continue;	/* must have " or < before filename */
		includeFile = strdup(v);
		r = includeFile + strlen(includeFile) - 1;
		*r = 0;		/* remove trailing \n */
		r--;
		ok = 0;
		while ((*r == '"' || *r == '>' || *r == ' ') && r > includeFile) {
		    if (*r != ' ')
			ok = 1;
		    *r = 0;
		    r--;
		}
		if (!ok || r <= includeFile)
		    continue;	/* if no ending '>' or '"' */
		/* or no filename, forget it */
		includeFile = expandFilename(includeFile);
		initDefaults(includeFile);	/* recursively add the file  */
		free(includeFile);
	    }
	    continue;
	}
	if (*buf != 0)
	    buf[strlen(buf) - 1] = 0;
	while (*v != ':' && *v != 0) {
	    v++;
	}
	if (*v == 0)
	    continue;
	*v = 0;
	v++;
	while (*v == ' ' || *v == '\t') {
	    v++;
	}
	if (*v != 0) {
	    new = (struct stringlist *) malloc(sizeof(struct stringlist));
	    new->next = defaults;
	    if(defaults)
		defaults->prev=new;
	    new->prev = NULL;
	    new->string = strdup(buf);
	    new->value = strdup(v);
	    new->searched = 0;
	    defaults = new;
	}
    }
    fclose(fp);
    return deffile;
}

/* changed to search the entire defaults list once instead of as many as 
   three times.  This is faster unless you have a "default.nickname:" entry
   for almost every default on every server... -JR */

static char *
getdefault(char *str, char *defstr)
{
    struct stringlist *slNick=0, *slFlavor=0, *slNorm=0, *sl;
    char    tempNick[80], tempFlavor[80];

    tempNick[0] = tempFlavor[0] = 0;
    if (!str)
	return NULL;
    if (!strlen(str))
	return NULL;

    if (defNickName) 
	sprintf(tempNick, "%s.%s", str, defNickName);

    if (defFlavor) 
	sprintf(tempFlavor, "%s.%s", str, defFlavor);

    for(sl=defaults;sl;sl=sl->next) {
	if(defNickName && !strcasecmp(sl->string, tempNick)) {
	    if(!dump_defaults) return sl->value;
	    if(!slNick)
		slNick=sl;
	} else if(defFlavor && !strcasecmp(sl->string, tempFlavor)) {
	    if(!slFlavor)
		slFlavor = sl;
	}

	if (!strcasecmp(sl->string, str)) {
	    if(!slNorm)
		slNorm = sl;
	}
    }
    /* if dump_defaults is true, print out all the strings being searched
       for in a way similar to the .paradiserc format [BDyess] */
    if(dump_defaults) {
      char buf[BUFSIZ],*end;
      
      strcpy(buf,str);
      end = buf + strlen(buf);
      *end = ':'; 
      end[1] = 0;
      if(!slFlavor) slFlavor = slNorm;
      if(!slNick) slNick = slFlavor;
      printf("%-40s%s\n",buf,slNorm ? slNorm->value : defstr);
      *end++ = '.';
      if(*tempFlavor) {
	sprintf(end,"%s:",defFlavor);
	printf("%-40s%s\n",buf,slFlavor ? slFlavor->value : defstr);
      }
      if(*tempNick) {
	sprintf(end,"%s:",defNickName);
	printf("%-40s%s\n",buf,slNick ? slNick->value : defstr);
      }
    }
    if(slNick)
	return slNick->value;
    if(slFlavor)
	return slFlavor->value;
    if(slNorm)
	return slNorm->value;

    return (NULL);
}

int
booleanDefault(char *def, int preferred)
{
    char   *str;

    str = getdefault(def,preferred ? "on" : "off");
    if (str == NULL)
	return (preferred);
    if (!strcasecmp(str, "on") || !strcasecmp(str, "true") || 
        !strcasecmp(str, "1")) {
	return (1);
    } else {
	return (0);
    }
}

int
intDefault(char *def, int preferred)
{
    char   *str;
    char   numstring[20];

    if(dump_defaults) {
      sprintf(numstring,"%d",preferred);
    }
    str = getdefault(def,numstring);
    if (!str)
	return preferred;
    return atoi(str);
}

/* gets the default for the given def and returns it if it exists.
   Otherwise returns a fresh copy of the preferred string */
char *
stringDefault(char *def, char *preferred)
{
    char   *str;

    str = getdefault(def,preferred ? preferred : "(null)");
    if (!str)
	return preferred ? strdup(preferred) : NULL;
    return strdup(str);
}

/* no default file given on command line.
   See if serverName is defined.  If it exists we look for
   HOME/.xtrekrc-<serverName> and .xtrekrc-<serverName>
   Otherwise we try DEFAULT_SERVER. */
/* modified to accept .paradiserc (or any base filename) 12/21/93 [BDyess] */
/* extended to check for for unadorned base and to return a fp [BDyess] */

static FILE *
findDefaults(char *home, char **deffile, char *base)
{
    FILE   *fp;

    /* check base-serverName */
    if (serverName) {
	if (home)
	    sprintf(*deffile, "%s/%s-%s", home, base, serverName);
	else
	    sprintf(*deffile, "%s-%s", base, serverName);
	fp = fopen(*deffile, "r");
	if (fp)
	    return fp;
    }
    /* check base-DEFAULT_SERVER */
    if (home)
	sprintf(*deffile, "%s/%s-%s", home, base, DEFAULT_SERVER);
    else
	sprintf(*deffile, "%s-%s", base, DEFAULT_SERVER);
    fp = fopen(*deffile, "r");
    if (fp)
	return fp;

    /* check just base */
    if (home)
	sprintf(*deffile, "%s/%s", home, base);
    else
	strcpy(*deffile, base);
    fp = fopen(*deffile, "r");
    return fp;
}

/* default ship code, courtesy of Robert Blackburn <blackburn@Virginia.EDU>.
   used by main(), modified to accept 2 letter initials 11/15/91 TC */
/* modified to accept pt/ut's 10/10/93 [BDyess] */

int
defaultShip(int preferred)
{
    char   *type;

    type = stringDefault("defaultship",NULL);
    if(type == NULL)
        return preferred;
    if ((strcasecmp(type, "scout") == 0) || 
        (strcasecmp(type, "SC") == 0))
	return SCOUT;
    else if ((strcasecmp(type, "destroyer") == 0) || 
             (strcasecmp(type, "DD") == 0))
	return DESTROYER;
    else if ((strcasecmp(type, "cruiser") == 0) || 
             (strcasecmp(type, "CA") == 0))
	return CRUISER;
    else if ((strcasecmp(type, "battleship") == 0) || 
             (strcasecmp(type, "BB") == 0))
	return BATTLESHIP;
    else if ((strcasecmp(type, "assault") == 0) || 
             (strcasecmp(type, "AS") == 0))
	return ASSAULT;
    else if ((strcasecmp(type, "starbase") == 0) || 
             (strcasecmp(type, "SB") == 0))
	return STARBASE;
    else if ((strcasecmp(type, "jumpship") == 0) || 
             (strcasecmp(type, "JS") == 0))
	return JUMPSHIP;
    else if ((strcasecmp(type, "warbase") == 0) || 
             (strcasecmp(type, "WB") == 0))
	return WARBASE;
    else if ((strcasecmp(type, "flagship") == 0) || 
             (strcasecmp(type, "FL") == 0))
	return FLAGSHIP;
    else if ((strcasecmp(type, "lightcruiser") == 0) || 
             (strcasecmp(type, "CL") == 0))
	return LIGHTCRUISER;
    else if ((strcasecmp(type, "carrier") == 0) || 
             (strcasecmp(type, "CV") == 0))
	return CARRIER;
    else if ((strcasecmp(type, "patrol") == 0) || 
             (strcasecmp(type, "PT") == 0))
	return PATROL;
    else if ((strcasecmp(type, "utility") == 0) || 
             (strcasecmp(type, "UT") == 0))
	return UTILITY;
    else
	return preferred;
}

void
initLogFile(void)
{
    if (logFile && logmess) {
	logfilehandle = fopen(logFile, "a");
	if (!logfilehandle) {
	    fprintf(stderr, "Can't open %s: ", logFile);
	    perror("");
	    logFile = NULL;
	} else {
	    printf("Opening %s for logging\n", logFile);
	    fprintf(logfilehandle, "\n-----------------------------------------------------------------\nStarted logging for server %s at%s\n-----------------------------------------------------------------\n",
		    serverName, timeString(time(NULL)));
	}
    }
}

void
resetDefaults(void)
{
    char   *buf;
#ifdef AUTHORIZE
    if (RSA_Client >= 0)
	RSA_Client = booleanDefault("useRSA", RSA_Client);
    else
	RSA_Client = (RSA_Client == -2);
#endif


    showShields = booleanDefault("showshields", showShields);
    showStats = booleanDefault("showstats", showStats);
    keeppeace = booleanDefault("keeppeace", keeppeace);
    reportKills = booleanDefault("reportkills", reportKills);
    blk_showStars = booleanDefault("showstars", blk_showStars);
    showMySpeed = booleanDefault("showMySpeed", showMySpeed);
    showTractorPressor = booleanDefault("showTractorPressor",
					showTractorPressor);
    /* show all tractor/pressor beams, not just your own [BDyess] */
    showAllTractorPressor = booleanDefault("showAllTractorPressor",
					   showAllTractorPressor);
    showLock = intDefault("showLock", showLock);
    drawgrid = booleanDefault("showgrid", drawgrid);
    namemode = booleanDefault("namemode", namemode);

    Dashboard = booleanDefault("newDashboard", Dashboard);
    cup_half_full = booleanDefault("newDashboard2", cup_half_full);
    if (cup_half_full)
	Dashboard = 2;
    Dashboard = intDefault("Dashboard", Dashboard);

    unixSoundPath = stringDefault("soundPath", "?");

    cloakchars = stringDefault("cloakchars", "??");
    cloakcharslen = strlen(cloakchars);

    showPhaser = intDefault("showPhaser", showPhaser);
    logmess = booleanDefault("logging", logmess);
    logFile = stringDefault("logfile",NULL);
    if(logFile)
	logFile=expandFilename(logFile);

    initLogFile();
    vary_hull = booleanDefault("warnhull", vary_hull);

    shelltools = booleanDefault("shellTools", shelltools);
    warpStreaks = booleanDefault("warpStreaks", warpStreaks);
    use_msgw = booleanDefault("useMsgw", use_msgw);
    logPhaserMissed = booleanDefault("logPhaserMissed", logPhaserMissed);
    phaserStats = booleanDefault("phaserStats", phaserStats);
    jubileePhasers = booleanDefault("jubileePhasers", jubileePhasers);
    enemyPhasers = intDefault("enemyPhasers", enemyPhasers);
    show_shield_dam = booleanDefault("showShieldDam", show_shield_dam);
    updateSpeed = intDefault("udpupdates", updateSpeed);
    updateSpeed = intDefault("updatespersecond", updateSpeed);
    updateSpeed = intDefault("updatespersec", updateSpeed);
    extraBorder = booleanDefault("extraAlertBorder", extraBorder);
    if (booleanDefault("galacticfrequent", 0))
	mapmode = GMAP_FREQUENT;
    continuousMouse = booleanDefault("continuousMouse", continuousMouse);
    if (continuousMouse)
	buttonRepeatMask = 1 << W_LBUTTON | 1 << W_RBUTTON | 1 << W_MBUTTON;
    clickDelay = intDefault("clickDelay", clickDelay);
    if (booleanDefault("continuousMouse.L", 0))
	buttonRepeatMask |= 1 << W_LBUTTON;
    if (booleanDefault("continuousMouse.M", 0))
	buttonRepeatMask |= 1 << W_MBUTTON;
    if (booleanDefault("continuousMouse.R", 0))
	buttonRepeatMask |= 1 << W_RBUTTON;
    if (buttonRepeatMask)
	continuousMouse = 1;
    autoQuit = intDefault("autoQuit", autoQuit);
    if (autoQuit > 199) {
	autoQuit = 199;
	printf("autoQuit reduced to 199\n");
    }
    pigSelf = booleanDefault("pigSelf", pigSelf);
    /* info icon shows info in place of the icon bitmap [BDyess] */
    infoIcon = booleanDefault("infoIcon", infoIcon);
    showGalacticSequence = stringDefault("showGalacticSequence", "012345");
    for (buf = showGalacticSequence; *buf; buf++) {
	if (*buf - '0' > 5) {
	    fprintf(stderr, "Error in showGalacticSequence: %d too high, ignoring\n", *buf);
	    free(showGalacticSequence);
	    showGalacticSequence = strdup("012345");
	    break;
	}
    }
    showLocalSequence = stringDefault("showLocalSequence", "01234");
    for (buf = showLocalSequence; *buf; buf++) {
	if (*buf - '0' > 4) {
	    fprintf(stderr, "Error in showLocalSequence: %d too high, ignoring\n", *buf);
	    free(showLocalSequence);
	    showLocalSequence = strdup("01234");
	    break;
	}
    }

    udpDebug = booleanDefault("udpDebug", udpDebug);
    udpClientSend = intDefault("udpClientSend", udpClientSend);
    /* note: requires send */
    udpClientRecv = intDefault("udpClientReceive", udpClientRecv);
    tryUdp = booleanDefault("tryUdp", tryUdp);
    udpSequenceChk = booleanDefault("udpSequenceCheck", udpSequenceChk);

    tryShort = booleanDefault("tryShort", tryShort);	/* auto-try S_P [BDyess] */

    /* playerlist settings */
    robsort = booleanDefault("robsort", robsort);
    sortPlayers = booleanDefault("sortPlayers", sortPlayers);
    hideNoKills = booleanDefault("hidenokills", hideNoKills);
    showDead = booleanDefault("showDead", showDead);
    showPreLogins = booleanDefault("showPreLogins", showPreLogins);
    sortOutfitting = booleanDefault("sortOutfitting", sortOutfitting);
    timerType = intDefault("timertype", timerType);
    /*
       default: old playerlist (ie, no format string), number shiptype rank
       name kills wins losses ratio offense defense di, number shiptype name
       kills login ratio totalrating di [BDyess]
    */
    playerListStart = stringDefault("playerList",
				    ",nTRNKWLr O D d,nTR N  K lrSd");
    playerList = playerListStart;
    resizePlayerList = booleanDefault("resizePlayerList",resizePlayerList);

    packetLights = booleanDefault("packetLights", packetLights);

    viewBox = booleanDefault("viewBox", viewBox);
    sectorNums = booleanDefault("sectorNums", sectorNums);
    lockLine = booleanDefault("lockLine", lockLine);
    mapSort = booleanDefault("mapSort", mapSort);
    autoSetWar = intDefault("autoSetWar", autoSetWar);

    colorFriends = booleanDefault("colorFriends", colorFriends);
    colorEnemies = booleanDefault("colorEnemies", colorEnemies);

    /* metaFork = booleanDefault("metaFork",metaFork); */

    keepInfo = intDefault("keepInfo", keepInfo);

    /* if there are alternatives to message warp, use it anyway? -JR */
    warp = booleanDefault("messageWarp", warp);

    askforUpdate = booleanDefault("askforUpdate", askforUpdate);
    lowercaset = booleanDefault("lowercaset", lowercaset);
    scrollBeep = booleanDefault("scrollBeep", scrollBeep);
    godToAllOnKills = booleanDefault("godToAllOnKills",godToAllOnKills);

    autoZoom=intDefault("autoZoom", autoZoom);
    autoUnZoom = intDefault("autoUnZoom",autoUnZoom);
    autoZoomOverride = intDefault("autoZoomOverride",autoZoomOverride);

    {
	DefLite = booleanDefault("DefLite", DefLite);
	UseLite = booleanDefault("UseLite", UseLite);

	if (DefLite)
	    litedefaults();

	beep_lite_cycle_time_planet = intDefault("planetCycleTime",
					       beep_lite_cycle_time_planet);
	beep_lite_cycle_time_player = intDefault("playerCycleTime",
					       beep_lite_cycle_time_player);
    }

    localShipStats=booleanDefault("localShipStats",localShipStats);
    statString = stringDefault("statString","DSEWF");
    statHeight = intDefault("statHeight",statHeight);
    if(statHeight<4) statHeight=4;
    if(statHeight>100) statHeight = 100;
    localStatsX = intDefault("localStatsX", localStatsX);
    localStatsY = intDefault("localStatsY", localStatsY);

    showIND = booleanDefault("showIND",showIND);

    /* hockey specific defs [BDyess] */
    galacticHockeyLines = booleanDefault("galacticHockeyLines",
    				galacticHockeyLines);
    tacticalHockeyLines = booleanDefault("tacticalHockeyLines",
    				tacticalHockeyLines);
    cleanHockeyGalactic = booleanDefault("cleanHockeyGalactic",
    				cleanHockeyGalactic);
    teamColorHockeyLines = booleanDefault("teamColorHockeyLines",
    				teamColorHockeyLines);
    puckBitmap = booleanDefault("puckBitmap",puckBitmap);
    puckArrow = booleanDefault("puckArrow",puckArrow);
    puckArrowSize = intDefault("puckArrowSize",puckArrowSize);

    /* galactic map plotting line [BDyess] */
    plotter = booleanDefault("plotter",plotter);

    redrawDelay = intDefault("redrawDelay",redrawDelay);

    /* no longer integers [BDyess] */
    {
      char *ok = "RFSTArfsta,", *s;
      int error = 0;
      char *galacticdefault = "F,T,A";
      char *localdefault = "Sf,Sft,Rft";

      showgalactic = stringDefault("showgalactic", NULL);
      if(showgalactic == NULL) {
        showgalactic = galacticdefault;
      } else {
        for(s = showgalactic; *s; s++) {
	  if(index(ok, *s) == NULL) {
	    printf("Invalid showGalactic setting: %s\n",showgalactic);
	    error = 1;
	    showgalactic = galacticdefault;
	    break;
	  }
	}
      }
      showGalacticSequence = showgalactic;
      for(s = showGalacticSequence;*s && *s != ',';s++) /*NULL*/;
      showGalacticLen = s - showGalacticSequence;
      
      showlocal = stringDefault("showLocal", NULL);
      if(showlocal == NULL) {
        showlocal = localdefault;
      } else {
        for(s = showlocal; *s; s++) {
	  if(index(ok, *s) == NULL) {
	    printf("Invalid showLocal setting: %s\n",showlocal);
	    error = 1;
	    showlocal = localdefault;
	    break;
	  }
	}
      }
      showLocalSequence = showlocal;
      for(s = showLocalSequence;*s && *s != ',';s++) /*NULL*/;
      showLocalLen = s - showLocalSequence;

      if(error) printf("\
For showLocal and showGalactic Use a string with the following letters:\n\
   F (Facilities), R (Resources), S (Surface), T (Team)\n\
such that the first bitmap you want drawn is the first letter, the second\n\
bitmap is the second letter (and will be drawn on top of the first), etc.\n\
Capital letters mean to use the planet image, lower case letters mean to use\n\
an overlay image.  Multiple sets can be used by placing a comma between them.\n\
Switch between sets with 'V' and 'B'.  [BDyess]\n\
\n\
Compiled in defaults:\n\
showLocal:	%s\n\
showGalactic:	%s\n\
\n",localdefault,galacticdefault);
    }

    planetChill = intDefault("planetChill",planetChill);
    if((buf = stringDefault("preload",NULL))) {
      char filename[BUFSIZ],*s;

      printf("Preloading:");
      while(*buf) {
	s = filename;
	while(*buf && *buf != ',') *s++ = *buf++;
	if(*buf == ',') buf++;
	*s = 0;
	if(0 == strcmp(filename,"ALL")) {
	  printf(" ALL!");
	  fflush(stdout);
	  loadAllImages();
	  break;
	}
	printf(" %s",filename);
	fflush(stdout);
        loadImageByFilename(filename);
      }
      printf("\n");
    }
    hudwarning = booleanDefault("HUDWarning",hudwarning);
    showNewStats = booleanDefault("showNewStats", showNewStats);
    scrollSaveLines = intDefault("scrollSaveLines", scrollSaveLines);
    useExternalImages = booleanDefault("useExternalImages", useExternalImages);

    initkeymap(-1);
    initMacros();

    /*
       sendOptionsPacket();
    *//* There is nothing on the server side that should be controlled by
       xtrekrc variables. */
}
