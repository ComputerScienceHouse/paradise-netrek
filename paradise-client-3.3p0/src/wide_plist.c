/*
 * playerlist.c
 *
 *   Fairly substantial re-write to do variable player lists: Sept 93 DRG
 *   Paradise shoehorning:  2/13/94  [BDyess]
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

static char header[BUFSIZ];
int     header_len=0;
char   *old_playerList=NULL;
char  **slottext=NULL;		/* array of strings shown in each slot
				   [BDyess] */
W_Color *slotcolors=NULL;		/* array of colors, one per slot [BDyess] */
int     slottext_size=0;		/* keep track of the size in case nplayers
				   changes */

/*===========================================================================*/

int
playerlistnum(void)
{
    int     num = 0;
    char   *ptr;

    ptr = playerList;
    header[0] = '\0';
    while (ptr[0] != '\0' && ptr[0] != ',') {
	switch (ptr[0]) {
	case 'n':		/* Ship Number */
	    strcat(header, " No");
	    num += 3;
	    break;
	case 'T':		/* Ship Type */
	    strcat(header, " Ty");
	    num += 3;
	    break;
	case 'R':		/* Rank */
	    strcat(header, " Rank      ");
	    num += 11;
	    break;
	case 'N':		/* Name */
	    strcat(header, " Name            ");
	    num += 17;
	    break;
	case 'K':		/* Kills */
	    strcat(header, " Kills");
	    num += 6;
	    break;
	case 'l':		/* Login Name */
	    strcat(header, " Login           ");
	    num += 17;
	    break;
	case 'O':		/* Offense */
	    strcat(header, " Offse");
	    num += 6;
	    break;
	case 'W':		/* Wins */
	    strcat(header, "  Wins");
	    num += 6;
	    break;
	case 'D':		/* Defense */
	    strcat(header, " Defse");
	    num += 6;
	    break;
	case 'L':		/* Losses */
	    strcat(header, "  Loss");
	    num += 6;
	    break;
	case 'S':		/* Total Rating (stats) */
	    strcat(header, " Stats");
	    num += 6;
	    break;
	case 'r':		/* Ratio */
	    strcat(header, " Ratio");
	    num += 6;
	    break;
	case 'd':		/* Damage Inflicted (DI) */
	    strcat(header, "      DI");
	    num += 8;
	    break;
	case ' ':		/* White Space */
	    strcat(header, " ");
	    num += 1;
	    break;
	case 'h':		/* Hockey stats */
	    strcat(header, " Goals  Assists  Points");
	    num += 23;
	    break;
	case 'B':		/* Bombing */
	    strcat(header, " Bmbng");
	    num += 6;
	    break;
	case 'b':		/* Armies Bombed */
	    strcat(header, " Bmbed");
	    num += 6;
	    break;
	case 'P':		/* Planets */
	    strcat(header, " Plnts");
	    num += 6;
	    break;
	case 'p':		/* Planets Taken */
	    strcat(header, " Plnts");
	    num += 6;
	    break;
	case 'G':		/* Doosh rating ('oGGing') [BDyess] */
	    strcat(header, " Dshng");
	    num += 6;
	    break;
	case 'g':		/* number of dooshes [BDyess] */
	    strcat(header, " Dshed");
	    num += 6;
	    break;
	case 'F':		/* Resource rating [BDyess] */
	    /* 'F' is from Farming...I'm out of good letters */
	    strcat(header, " Resrc");
	    num += 6;
	    break;
	case 'f':		/* number of Resources bombed [BDyess] */
	    strcat(header, " Rsrcs");
	    num += 6;
	    break;
	case 'Z':		/* SB rating [BDyess] */
	    /* 'Z' is the last letter - a SB is the team anchor. :) */
	    strcat(header, " SBrat");
	    num += 6;
	    break;
	case 'z':		/* WB rating (small SB) [BDyess] */
	    strcat(header, " WBrat");
	    num += 6;
	    break;
	case 'J':		/* JS rating - good letter [BDyess] */
	    strcat(header, " JSrat");
	    num += 6;
	    break;
	case 'j':		/* JS planets [BDyess] */
	    strcat(header, " JSpls");
	    num += 6;
	    break;
	case 'C':		/* SpeCial ships rating [BDyess] */
	    strcat(header, " Specl");
	    num += 6;
	    break;
	case 'E':		/* genocides (Endings) [BDyess] */
	    strcat(header, " Genos");
	    num += 6;
	    break;
	case 'M':		/* Display, Host Machine */
	    strcat(header, " Host Machine    ");
	    num += 17;
	    break;
	case 'H':		/* Hours Played */
	    strcat(header, "  Hours");
	    num += 7;
	    break;
	case 'k':		/* Max Kills */
	    strcat(header, " Max K");
	    num += 6;
	    break;
	case 'V':		/* Kills per hour */
	    strcat(header, "   KPH");
	    num += 6;
	    break;
	case 'v':		/* Deaths per hour */
	    strcat(header, "   DPH");
	    num += 6;
	    break;
	case 'w':		/* War staus */
	    if(ptr[1] == '-')
	    {
	      strcat(header, " War");
	      num += 4;
	      ptr++;
	    }
	    else
	    {
	      strcat(header, " War Stat");
	      num += 9;
	    }
	    break;
	case 's':		/* Speed */
	    strcat(header, " Sp");
	    num += 3;
	    break;
	default:
	    fprintf(stderr, "%c is not an option for the playerlist\n", ptr[0]);
	    break;
	}
	ptr++;
    }

    old_playerList = playerList;
    header_len = num;
    return (num);
}

/*===========================================================================*/

void
wideplayerlist(void)
{
    int     i;
    int     old_len=header_len;
    if (old_playerList != playerList) {
	playerlistnum();
        if(resizePlayerList)
            W_ResizeText(playerw,header_len,W_WindowHeight(playerw));
	/* init slottext [BDyess] */
    }
    for (i = 0; i < nplayers; i++)
	slot[i] = -1;

    W_ClearWindow(playerw);
    
    if (slottext && ((old_len != header_len) || (slottext_size != nplayers))) {
	/* free the old one */
	for (i = 0; i < slottext_size; i++) {
	    free(slottext[i]);
	}
	free(slottext);
	free(slotcolors);
	slottext=0;
    }
    if(!slottext) {
	slottext_size = nplayers;
	slottext = (char **) malloc(sizeof(char *) * nplayers);
	slotcolors = (W_Color *) malloc(sizeof(W_Color) * nplayers);
	for (i = 0; i < nplayers; i++) {
	    /* malloc extra room in case a line runs off the end */
	    slottext[i] = (char *) malloc(sizeof(char) * header_len + 30);
	    slottext[i][0] = 0;
	    slotcolors[i] = -1;
	}
    }
    W_WriteText(playerw, 0, 1, textColor, header, header_len, W_RegularFont);

    for (i = 0; i < nplayers; i++) {
	updatePlayer[i] = 1;
    }

    wideplayerlist2();
}

/*===========================================================================*/

/* little routine to print just the chars that are different between orig and
   new.  [BDyess] */
void
writeDiffText(W_Window window, int x, int y, W_Color color, 
              char *orig, char *new, W_Font font)
{
    int     i;
    char   *start;

    for (start = new, i = 0; new[i] && orig[i]; i++) {
	if (orig[i] != new[i])
	    continue;
	if (start == new + i) {
	    start++;
	    continue;
	} else {
	    W_WriteText(window, x + start - new, y, color, start, new + i - start, font);
	    start = new + i + 1;
	}
    }
    if (start != new + i) {
	/* finish up any remaining digits */
	W_WriteText(window, x + start - new, y, color, start, new + i - start, font);
    }
    if (new[i]) {
	/* write any text that extends past the old one */
	W_WriteText(window, x + i, y, color, new + i, strlen(new + i), font);
    } else if (orig[i]) {
	/* print spaces to clear to EOL */
	char   *freeme;
	unsigned int len;

	len = strlen(orig + i);
	freeme = (char *) malloc(len);
	memset(freeme, ' ', len);
	W_WriteText(window, x + i, y, color, freeme, (int)len, font);
    }
}

/*===========================================================================*/

void
plist_line(struct player *j, int pos)
{
    char    buf[BUFSIZ];
    char   *ptr;
    char    tmp[30];
    int     my_ticks;
    struct ratings r;
    W_Color color;

    get_ratings(j, &r);

    if (pos < 2) {
	printf("bad line position in playerlist\n");
	pos = 2;
    }
    if (paradise)
	my_ticks = j->p_stats2.st_tticks;
    else
	my_ticks = j->p_stats.st_tticks;

    ptr = playerList;
    buf[0] = '\0';
    while (ptr[0] != '\0' && ptr[0] != ',') {
	tmp[0] = '\0';
	switch (ptr[0]) {
	case 'n':		/* Ship Number */
		tmp[0] = ' ';
	    if (j->p_status != PALIVE) {
		tmp[1] = ' ';
	    } else {
		tmp[1] = teaminfo[j->p_teami].letter;
	    }
		tmp[2] = shipnos[j->p_no];
		tmp[3] = '\0';
	    strcat(buf, tmp);
	    break;
	case 'T':		/* Ship Type */
		tmp[0] = ' ';
	    switch (j->p_status) {
	    case PALIVE:
		tmp[1] = j->p_ship->s_desig[0];
		tmp[2] = j->p_ship->s_desig[1];
		break;
	    case PTQUEUE:
		tmp[1] = 't';
		tmp[2] = 'q';
		break;
	    case POUTFIT:
		tmp[1] = '-';
		tmp[2] = '-';
		break;
	    case PEXPLODE:
	    case PDEAD:
		tmp[1] = '*';
		tmp[2] = '*';
		break;
	    case POBSERVE:
		tmp[1] = 'o';
		tmp[2] = 'b';
		break;
	    default:
		tmp[1] = ' ';
		tmp[2] = ' ';
	    }
	    tmp[3] = '\0';
	    strcat(buf, tmp);
	    break;
	case 'R':		/* Rank */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%-10.10s", get_players_rank_name(j));
	    strcat(buf, tmp);
	    break;
	case 'N':		/* Name */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%-16.16s", j->p_name);
	    strcat(buf, tmp);
	    break;
	case 'K':		/* Kills */
	    tmp[0] = ' ';
	    if ((j->p_kills <= 0 &&
		 (paradise || RSA_Client <= 0) && hideNoKills)
		|| (j->p_status & ~PALIVE))
		strcpy(tmp + 1, "     ");
	    else
		sprintf(tmp + 1, "%5.2f", j->p_kills);
	    strcat(buf, tmp);
	    break;
	case 'l':		/* Login Name */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%-16.16s", j->p_login);
	    strcat(buf, tmp);
	    break;
	case 'O':		/* Offense */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_offrat);
	    strcat(buf, tmp);
	    break;
	case 'W':		/* Wins */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5d", r.r_kills);
	    strcat(buf, tmp);
	    break;
	case 'D':		/* Defense */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_defrat);
	    strcat(buf, tmp);
	    break;
	case 'L':		/* Losses */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5d", r.r_losses);
	    strcat(buf, tmp);
	    break;
	case 'S':		/* Total Rating (stats) */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_ratings);
	    strcat(buf, tmp);
	    break;
	case 'r':		/* Ratio */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_ratio);
	    strcat(buf, tmp);
	    break;
	case 'd':		/* Damage Inflicted (DI) */
	    tmp[0] = ' ';
	    /*
	       ftoa (Ratings * (j->p_stats.st_tticks / 36000.0), tmp+1, 0, 4,
	       2);
	    */
	    sprintf(tmp + 1, "%7.2f", r.r_di);
	    strcat(buf, tmp);
	    break;
	case ' ':		/* White Space */
	    strcat(buf, " ");
	    break;
	case 'h':		/* Hockey stats */
	    sprintf(tmp, " %5d %8d %7d", r.r_planets, r.r_armies,
	            r.r_planets + r.r_armies);
	    strcat(buf, tmp);
	    break;
	case 'B':		/* Bombing */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_bombrat);
	    strcat(buf, tmp);
	    break;
	case 'b':		/* Armies Bombed */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5d", r.r_armies);
	    strcat(buf, tmp);
	    break;
	case 'P':		/* Planets */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_planetrat);
	    strcat(buf, tmp);
	    break;
	case 'p':		/* Planets Taken */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5d", r.r_planets);
	    strcat(buf, tmp);
	    break;
	case 'G':		/* Doosh rating ('oGGing') [BDyess] */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_dooshrat);
	    strcat(buf, tmp);
	    break;
	case 'g':		/* Dooshes [BDyess] */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5d", r.r_dooshes);
	    strcat(buf, tmp);
	    break;
	case 'F':		/* Resource rating [BDyess] */
	    /* 'F' is from Farming...I'm out of good letters */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_resrat);
	    strcat(buf, tmp);
	    break;
	case 'f':		/* number of Resources bombed [BDyess] */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5d", r.r_resources);
	    strcat(buf, tmp);
	    break;
	case 'Z':		/* SB rating [BDyess] */
	    /* 'Z' is the last letter - a SB is the team anchor. :) */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_sbrat);
	    strcat(buf, tmp);
	    break;
	case 'z':		/* WB rating (small SB) [BDyess] */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_wbrat);
	    strcat(buf, tmp);
	    break;
	case 'J':		/* JS rating - good letter [BDyess] */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_jsrat);
	    strcat(buf, tmp);
	    break;
	case 'j':		/* JS planets [BDyess] */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5d", r.r_jsplanets);
	    strcat(buf, tmp);
	    break;
	case 'C':		/* SpeCial ships rating [BDyess] */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_specrat);
	    strcat(buf, tmp);
	    break;
	case 'E':		/* genocides (Endings) [BDyess] */
	    *tmp = ' ';
	    sprintf(tmp + 1, "%5d", r.r_genocides);
	    strcat(buf, tmp);
	    break;
	case 'M':		/* Display, Host Machine */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%-16.16s", j->p_monitor);
	    strcat(buf, tmp);
	    break;
	case 'H':		/* Hours Played */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%6.2f", my_ticks / 36000.0);
	    strcat(buf, tmp);
	    break;
	case 'k':		/* Max Kills  */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.2f", r.r_maxkills);
	    strcat(buf, tmp);
	    break;
	case 'V':		/* Kills Per Hour  */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.1f", r.r_killsPerHour);
	    strcat(buf, tmp);
	    break;
	case 'v':		/* Deaths Per Hour  */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%5.1f", r.r_lossesPerHour);
	    strcat(buf, tmp);
	    break;
	case 'w':		/* War staus */
	    {
	        int sh;
		
		sh = (ptr[1] == '-');

	        if (j->p_swar & idx_to_mask(me->p_teami))
	    	    strcat(buf, (sh ? " W  " : " WAR     "));
	        else if (j->p_hostile & idx_to_mask(me->p_teami))
	  	    strcat(buf, (sh ? " H  " : " HOSTILE "));
	        else
		    strcat(buf, (sh ? " P  " : " PEACEFUL"));

                if(sh) ptr++;
	    }
	    break;
	case 's':		/* Speed */
	    tmp[0] = ' ';
	    sprintf(tmp + 1, "%2d", j->p_speed);
	    strcat(buf, tmp);
	    break;
	default:
	    break;
	}
	ptr++;
    }

    color = playerColor(j);
    if (slot[pos - 2] == j->p_no && slotcolors[pos - 2] == color) {
	/* write the line, skipping chars that haven't changed [BDyess] */
	writeDiffText(playerw, 0, pos, color, slottext[pos - 2], buf,
		      shipFont(j));
    } else {
	W_WriteText(playerw, 0, pos, color, buf, strlen(buf),
		    shipFont(j));
    }
    strcpy(slottext[pos - 2], buf);
    slotcolors[pos - 2] = color;
    slot[pos - 2] = j->p_no;
}

/*===========================================================================*/

void
Sorted_playerlist2(void)
{
    register int i, h, pos = 1;
    register struct player *j;
    int     numplayers;

  /* 20, not 16, is the max for non-paradise! Mostly the extra 4 are */
  /* robots, but might as well show them and be safe... -JR*/
  numplayers = (paradise) ? nplayers : 36;

    /*
       if (++num % 21 == 0) { boolflag = 1; num = 0; }
    */
    /* go through the teams in order */
    for (h = 0; h < number_of_teams; h++) {
	/* skip my team, I'll come back to it later */
	if (me->p_teami == h)
	    continue;

	/* go through all the players looking for those on team h */
	for (i = 0, j = &players[0]; i < numplayers; i++, j++) {
	    if (j->p_teami != h)
		continue;

	    if (j->p_status == PFREE)
		continue;

          if(j->p_status == POUTFIT && !showDead) /* already know team */
                                                  /* is valid.. */
              continue;

	    pos++;		/* put this AFTER checking for free slots to
				   get a */
                 /* nice compact list... */

	    if (!updatePlayer[i] && slot[pos - 2] == i)
		continue;

	    updatePlayer[i] = 0;
	    plist_line(j, pos);
	}
    }

    /* now go through and do my team.  Note: ind players haven't been done */
    if (me->p_teami >= 0 && me->p_teami < number_of_teams) {
    for (i = 0, j = &players[i]; i < numplayers; i++, j++) {
	if (j->p_teami != me->p_teami)
	    continue;

	    if (j->p_status == PFREE)
	    continue;

      if(j->p_status == POUTFIT && !showDead) /* already know team */
                                                  /* is valid.. */
          continue;

      pos++;
	    if (!updatePlayer[i] && slot[pos - 2] == i)
	    continue;

	updatePlayer[i] = 0;
	plist_line(j, pos);
    }
    }

    for (i = 0, j = &players[0]; i < numplayers; i++, j++) {
	if (j->p_teami >= 0 && j->p_teami < number_of_teams)
	    continue;

	if (j->p_status == PFREE)
	    continue;

	pos++;

	if (!updatePlayer[i] && slot[pos - 2] == i)
	    continue;

	updatePlayer[i] = 0;
	plist_line(j, pos);
    }
    /* now continue clearing lines until we get to an empty one */
    pos++;
    while ((pos - 2) < numplayers && slot[pos - 2] != -1 && slottext[pos - 2][0]) {
	W_ClearArea(playerw, 0, pos, header_len, 1);
	slot[pos - 2] = -1;
	slottext[pos - 2][0] = 0;
	pos++;
    }
}

/*===========================================================================*/

void
wideplayerlist2(void)
{
    register int i;
    register struct player *j;
    int     numplayers;

    /* 20, not 16, is the max for non-paradise! Mostly the extra 4 are */
    /* robots, but might as well show them and be safe... -JR */
    numplayers = (paradise) ? nplayers : 36;

    if (old_playerList != playerList) {
	wideplayerlist();	/* refresh if playerList changed */
	return;
    }
    if (!W_IsMapped(playerw))
	return;

    if (sortPlayers) {
	Sorted_playerlist2();
	return;
    }
    for (i = 0, j = &players[i]; i < numplayers; i++, j++) {
	if (!updatePlayer[i])
	    continue;

	updatePlayer[i] = 0;

	if (j->p_status == PFREE) {
	    W_ClearArea(playerw, 0, i + 2, header_len, 1);
	    continue;
	}
	plist_line(j, i + 2);
    }
}
