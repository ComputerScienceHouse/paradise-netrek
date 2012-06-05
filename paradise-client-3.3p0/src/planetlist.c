/*
 * planetlist.c
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

/* Prototypes */
static void planet_list_paradise P((void));
static void planet_list_normal P((void));
static void print_planet P((W_Window wind, int line, struct planet * j));

/*
** Open a window which contains all the planets and their current
** statistics.  Players will not know about planets that their team
** has not orbited.
*/

void
planetlist(void)
{
    /*
       W_ClearWindow(planetw);
    */
    if (!paradise) {		/* if not a paradise server then */
	planet_list_normal();
    } else {			/* else must be a paradise server */
	planet_list_paradise();
    }
}


/*This function provides the planet list for a normal server. */

static void
planet_list_normal(void)
{
    int     k = 0;		/* for row number */
    int     i;			/* looping var */
    struct planet *j;		/* to point to a planet */
    char    buf[100];		/* to sprintf into */
    char    buf1[40];
    W_Window wind;

    wind = planetw;

    for (i = 0, j = &planets[i]; i < nplanets; i++, j++) {
        /* ignore invisible planets */
        if (j->pl_x < 0 || j->pl_y < 0)
	  continue;

	if (i == 0 || i == nplanets / 2) {
	    if (i != 0) {
		wind = planetw2;
	    }
	    sprintf(buf, "Planet name           Own Armies      Resources          Info");
	    W_WriteText(wind, 2, 1, textColor, buf, strlen(buf), 
	    		W_RegularFont);
	    k = 2;
	}
	sprintf(buf1, "%-16s ", j->pl_name);
	if (j->pl_info & idx_to_mask(me->p_teami)) {
	    sprintf(buf, "%3s %3d       %s%s%s      %s    ",
		    teaminfo[mask_to_idx(j->pl_owner)].shortname,
		    j->pl_armies,
		    (j->pl_flags & PLREPAIR ? "REPR " : "     "),
		    (j->pl_flags & PLFUEL ? "FUEL " : "     "),
		    (j->pl_flags & PLAGRI ? "AGRI " : "     "),
		    team_bit_string(j->pl_info));
	    W_WriteText(wind, 2, k, planetColor(j), buf1, strlen(buf1),
			planetFont(j));
	    W_WriteText(wind, 24, k++, planetColor(j), buf, strlen(buf),
			planetFont(j));
	}
	/* end of have info */
	else {			/* else no info on planet */
	    W_WriteText(wind, 2, k++, planetColor(j), buf1, strlen(buf1),
			planetFont(j));
	}
    }				/* end of for loop */

}



/*This function provides the planet list for a paradise server version 2.0 */

static void
planet_list_paradise(void)
{
    typedef struct planet *plptr;

    int     k = 0;		/* for row number */
    int     i, team_pnum;	/* looping var */
    plptr   j;			/* to point to a planet */
    char    buf[100];		/* to sprintf into */
    W_Window wind;
    extern int number_of_teams;
    plptr **team_p;
    int    *team_pcount;

    wind = planetw;

    /* this malloc stuff will handle any number of teams/races */

    /* team's planet counters */
    team_pcount = (int *) malloc((number_of_teams + 2) * sizeof(int));
    for (i = 0; i < number_of_teams + 2; i++)
	team_pcount[i] = 0;

    if (mapSort) {
	/* make some memory */
	team_p = (plptr **) malloc((number_of_teams + 2) * sizeof(struct planet *));
	for (i = 0; i < (number_of_teams + 2); i++)
	    team_p[i] = (plptr *) malloc(nplanets * sizeof(struct planet *));

	/* loop thru and put proper team planeter point on each planet */
	for (i = 0, j = &planets[i]; i < nplanets; i++, j++) {
	    k = mask_to_idx(j->pl_owner) + 1;	/* which team gets planet */
	    team_p[k][team_pcount[k]] = j;
	    team_pcount[k]++;
	}

	/* go thru each teams planet list and display */
	for (i = 0, k = 0; i < (number_of_teams + 2); i++) {
	    for (team_pnum = 0; team_pnum < team_pcount[i]; team_pnum++, k++) {

		j = team_p[i][team_pnum];
		/* (nplanets+13)/2 is the height of window; from newwin.c */
		if (k == 0 || k >= ((nplanets + 13) / 2)) {
		    if (k != 0)
			wind = planetw2;
		    sprintf(buf, "Planet name      sctr own armies RESOURCES  SURFC  ATMOS    VISIT    TIME");
		    W_WriteText(wind, 2, 1, textColor, buf, strlen(buf),
		    		W_RegularFont);
		    k = 2;
		}
		print_planet(wind, k, j);
	    }			/* end of 2nd for */
	    if (team_pcount[i] > 0)
		k++;
	}			/* end of 1st for */

	for (i = 0; i < (number_of_teams + 2); i++)
	    free(team_p[i]);
	free(team_p);
    } else {			/* do the original alpa only sort planet list */

	for (i = 0, j = &planets[i]; i < nplanets; i++, j++, k++) {
	    if (i == 0 || i == nplanets / 2) {

		sprintf(buf, "Planet name      sctr own armies RESOURCES  SURFC  ATMOS    VISIT    TIME");

		if (i != 0) {
		    wind = planetw2;
		}
		W_WriteText(wind, 2, 1, textColor, buf, strlen(buf), 
		  	    W_RegularFont);
		k = 2;
	    }
	    team_pcount[mask_to_idx(j->pl_owner) + 1]++;
	    print_planet(wind, k, j);
	}
    }

    k++;
    for (i = 0; i < (number_of_teams + 1); i++) {
	W_Color cur_color;

	cur_color = shipCol[i];
	sprintf(buf, "%s: ", teaminfo[i-1].shortname);
	W_WriteText(wind, i * 7 + 2, k, cur_color, buf, strlen(buf), 
		    W_RegularFont);
	sprintf(buf, " %-2d", team_pcount[i]);
	W_WriteText(wind, i * 7 + 2, k + 1, cur_color, buf, strlen(buf), 
	            W_RegularFont);
    }

    free(team_pcount);

}				/* end of planet_list_paradise */


/****************************** print_planet() ************************/
static void
print_planet(W_Window wind, int line, struct planet *j)
{
    char    buf[100];		/* to sprintf into */

    sprintf(buf, "%-16s %d-%d", j->pl_name, (j->pl_x / GRIDSIZE) + 1,
	    (j->pl_y / GRIDSIZE) + 1);
    W_WriteText(wind, 2, line, textColor, buf, strlen(buf),
		W_RegularFont);

    if (j->pl_info & idx_to_mask(me->p_teami)) {
	if (PL_TYPE(*j) == PLSTAR) {	/* if planet actually a star */
	    W_WriteText(wind, 24, line, textColor, "---S T A R---", 13,
		       W_RegularFont);
	} else if (PL_TYPE(*j) == PLWHOLE) { /* if wormhole... */
	   W_WriteText(wind, 24, line, textColor, "---W O R M H O L E---", 21,
			W_RegularFont);
	} else {		/* else planet not a star */
	    char   *s = NULL;

	    switch (j->pl_flags & PLATMASK) {
	    case PLPOISON:
		s = "TOXC";
		break;
	    case PLATYPE3:
		s = "TNTD";
		break;
	    case PLATYPE2:
		s = "THIN";
		break;
	    case PLATYPE1:
		s = "STND";
		break;
	    };
	    sprintf(buf, "%3s %3d       %c%c%c%c     %c%c%c    %4s",
		    teaminfo[mask_to_idx(j->pl_owner)].shortname,
		    j->pl_armies,
		    (j->pl_flags & PLREPAIR ? 'R' : ' '),
		    (j->pl_flags & PLFUEL ? 'F' : ' '),
		    (j->pl_flags & PLAGRI ? 'A' : ' '),
		    (j->pl_flags & PLSHIPYARD ? 'S' : ' '),
		    (j->pl_flags & PLDILYTH ? 'D' : ' '),
		    (j->pl_flags & PLMETAL ? 'M' : ' '),
		    (j->pl_flags & PLARABLE ? 'A' : ' '),
		    s);

	    W_WriteText(wind, 24, line, planetColor(j), buf, strlen(buf),
			planetFont(j));

	    sprintf(buf, "%4s   %3ld",
		    team_bit_string(j->pl_info),
		    ((idx_to_mask(me->p_teami) == j->pl_owner) ? 0 : (status2->clock - j->pl_timestamp)));

	    W_WriteText(wind, 64, line, planetColor(j), buf, strlen(buf),
			planetFont(j));
	}
    } else {
	sprintf(buf, "--- No info; Scout me ---");
	W_WriteText(wind, 24, line, textColor, buf, strlen(buf),
		    W_RegularFont);
    }
}				/* end of print_planet */
