/*
 * dashboard.c - graphic tstatw - 6/2/93
 *
 * copyright 1993 Lars Bernhardsson (lab@mtek.chalmers.se)
 * Free to use as long as this notice is left here.
 *
 * Color by Nick Trown.
 * Paradise shoehorning by Bill Dyess.
 * Rainbow dashboard by Bill Dyess
 */

#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "images.h"

#ifdef UNIX_SOUND
#include "sound.h"
#endif

#define DB_NOFILL 0
#define DB_LINE 1
#define DB_FILL 2

#define DB_3DIGITS 0
#define DB_5DIGITS 1

/*
#define BAR_LENGTH 56
#define BAR_LENGTH_THIRD 18
*/
#define TSTATW_BASEX 32

#define SPACING 4

int     BAR_LENGTH = 56;
int     BAR_LENGTH_THIRD = 18;

int     column[4];

/* code to draw and erase packet lights 2/5/94 [BDyess] */

#define SENDX 		7
#define SENDY		1
#define RECEIVEX	3
#define RECEIVEY	1

int     send_lit = 0, receive_lit = 0;

void
light_send(void)
{
    if (packetLights == 0 || send_lit)
	return;
    send_lit = 1;
    W_DrawPoint(tstatw, SENDX, SENDY, W_Green);
    W_DrawPoint(tstatw, SENDX + 1, SENDY, W_Green);
    W_DrawPoint(tstatw, SENDX, SENDY + 1, W_Green);
    W_DrawPoint(tstatw, SENDX + 1, SENDY + 1, W_Green);
}

void
light_receive(void)
{
    if (packetLights == 0 || receive_lit)
	return;
    receive_lit = 2;
    W_DrawPoint(tstatw, RECEIVEX, RECEIVEY, W_Yellow);
    W_DrawPoint(tstatw, RECEIVEX + 1, RECEIVEY, W_Yellow);
    W_DrawPoint(tstatw, RECEIVEX, RECEIVEY + 1, W_Yellow);
    W_DrawPoint(tstatw, RECEIVEX + 1, RECEIVEY + 1, W_Yellow);
}

void
light_erase(void)
{
    if (receive_lit == 1) {
	W_DrawPoint(tstatw, RECEIVEX, RECEIVEY, backColor);
	W_DrawPoint(tstatw, RECEIVEX + 1, RECEIVEY, backColor);
	W_DrawPoint(tstatw, RECEIVEX, RECEIVEY + 1, backColor);
	W_DrawPoint(tstatw, RECEIVEX + 1, RECEIVEY + 1, backColor);
    }
    if (receive_lit)
	receive_lit--;
    if (send_lit == 1) {
	W_DrawPoint(tstatw, SENDX, SENDY, backColor);
	W_DrawPoint(tstatw, SENDX + 1, SENDY, backColor);
	W_DrawPoint(tstatw, SENDX, SENDY + 1, backColor);
	W_DrawPoint(tstatw, SENDX + 1, SENDY + 1, backColor);
    }
    if (send_lit)
	send_lit--;
}

void
db_box(int x, int y, int width, int height, int f, int color)
{
    int     border = W_White;

    if (color == W_Red)
	border = color;

    if (width == 0 || height == 0)
	return;

    switch (f) {
    case DB_FILL:
	W_FillArea(tstatw, x, y, width + 1, height + 1, color);
	break;
    case DB_LINE:
	W_MakeLine(tstatw, x + width, y, x + width, y + height, border);
	W_MakeLine(tstatw, x + width, y + 4, x + BAR_LENGTH, y + 4, border);
	break;
    case DB_NOFILL:
	W_MakeLine(tstatw, x, y, x + width, y, border);
	W_MakeLine(tstatw, x + width, y, x + width, y + height, border);
	W_MakeLine(tstatw, x + width, y + height, x, y + height, border);
	W_MakeLine(tstatw, x, y + height, x, y, border);
	break;
    }
}

void
db_bar(char *lab, int x, int y, 
       int value, int tmpmax, int max, int digits, int color)
{
    unsigned int wt, wv, tc, tw;
    char    valstr[32];

    switch (digits) {
    case DB_3DIGITS:
	tc = 11;
	tw = W_Textwidth * tc;
	sprintf(valstr, "%2.2s[%3d/%3d]", lab, value, tmpmax);
	W_ClearArea(tstatw, x, y, tw + BAR_LENGTH, W_Textheight);
	break;
    case DB_5DIGITS:
    default:
	tc = 15;
	tw = W_Textwidth * tc;
	sprintf(valstr, "%2.2s[%5d/%5d]", lab, value, tmpmax);
	W_ClearArea(tstatw, x, y, tw + BAR_LENGTH, W_Textheight);
	break;
    }

    if (max) {
	wt = (BAR_LENGTH * tmpmax) / max;
	wv = (BAR_LENGTH * value) / max;
    } else {
	wt = 0;
	wv = 0;
    }
    if (wt > BAR_LENGTH)
	wt = BAR_LENGTH;
    if (wv > BAR_LENGTH)
	wv = BAR_LENGTH;

    W_WriteText(tstatw, x, y, textColor, valstr, tc, W_RegularFont);

    db_box(x + tw, y, BAR_LENGTH, W_Textheight - 1, DB_NOFILL, color);
    if (wt >= wv && wt > 0)
	db_box(x + tw, y, wt, W_Textheight - 1, DB_LINE, color);

    if (wv > 0)
	db_box(x + tw, y, wv, W_Textheight - 1, DB_FILL, color);
}

void
db_color_bar(char *lab, int x, int y, 
             int barvalue, int numvalue, int tmpmax, int max, int digits)
{
    unsigned int wt, wv, tw, tc;
    char    valstr[32];
    int     color = W_White;

    switch (digits) {
    case DB_3DIGITS:
	tc = 11;
	tw = W_Textwidth * tc;
	sprintf(valstr, "%2.2s[%3d/%3d]", lab, numvalue, tmpmax);
	W_ClearArea(tstatw, x, y, tw + BAR_LENGTH, W_Textheight);
	break;
    case DB_5DIGITS:
    default:
	tc = 15;
	tw = W_Textwidth * tc;
	sprintf(valstr, "%2.2s[%5d/%5d]", lab, numvalue, tmpmax);
	W_ClearArea(tstatw, x, y, tw + BAR_LENGTH, W_Textheight);
	break;
    }

    if (max) {
	wt = (int) ((float) BAR_LENGTH * ((float) tmpmax / (float) max));
	wv = (int) ((float) BAR_LENGTH * ((float) barvalue / (float) max));
    } else {
	wt = 0;
	wv = 0;
    }
    if (wt > BAR_LENGTH)
	wt = BAR_LENGTH;
    if (wv > BAR_LENGTH)
	wv = BAR_LENGTH;

    W_WriteText(tstatw, x, y, color, valstr, tc, W_RegularFont);

    db_box(x + tw, y, BAR_LENGTH, W_Textheight - 1, DB_NOFILL, color);
    if (wt >= wv && wt > 0)
	db_box(x + tw, y, wt, W_Textheight - 1, DB_LINE, color);

    /* draw rainbow bars */
    if(xpm)
      W_DrawImageBar(tstatw, x + tw, y, wv, getImage(I_RAINBOW));
    else 
    {
      if (wv > 0)
	  db_box(x + tw, y, wv > BAR_LENGTH_THIRD ? BAR_LENGTH_THIRD : wv, 
	         W_Textheight - 1, DB_FILL, W_Green);
      if (wv > BAR_LENGTH_THIRD)
	  db_box(x + tw + BAR_LENGTH_THIRD, y, 
	         wv > 2 * BAR_LENGTH_THIRD ? BAR_LENGTH_THIRD
		  : wv - BAR_LENGTH_THIRD, W_Textheight - 1, 
		 DB_FILL, W_Yellow);
      if (wv > 2 * BAR_LENGTH_THIRD)
	  db_box(x + tw + 2 * BAR_LENGTH_THIRD, y, 
	         (wv > BAR_LENGTH ? BAR_LENGTH : wv) -
		   2 * BAR_LENGTH_THIRD, W_Textheight - 1, DB_FILL, W_Red);
    }
}

/* handles the dashboard timer [BDyess] 10/29/93 */
void
db_timer(int fr, int xloc, int yloc)
{
    static time_t oldtime = -1;
    static int lastTimerType = -1;
    time_t  now = 0;
    static char lasttimer[TIMESTRLEN], *timer;
    int     left, right, x, pos;

    if(playback) {
	pb_framectr(xloc, yloc);
	return;
    }
    if (timerType != lastTimerType || fr) {
	char *s = NULL;

	fr = 1;
	lastTimerType = timerType;
	switch (timerType) {
	case T_NONE:
	    W_ClearArea(tstatw, xloc, yloc, (TIMESTRLEN+3) * W_Textwidth,
	                W_Textheight);
	    memset(lasttimer, ' ', TIMESTRLEN);
	    oldtime = now;
	    break;
	case T_DAY:
	    s = "NOW";
	    break;
	case T_SERVER:
	    s = "SRV";
	    break;
	case T_SHIP:
	    s = "SHP";
	    break;
	case T_USER:
	    s = "TMR";
	    break;
	}
	if(s) {
	  W_WriteText(tstatw, xloc, yloc, textColor, s, 3, W_RegularFont);
	}
    }
    if (!timerType)
	return;
    now = time(NULL);
    if (now != oldtime || fr) {
	/*
	   get the timer string and start comparing it with the old one. Only
	   print the differences
	*/
	timer = timeString(now - timeBank[timerType]);
	x = xloc + 4 * W_Textwidth;
	left = 0;
	right = -1;
	pos = 0;

	/*
	   run through the string to find any differences.  Print any
	   continuous differences with one W_WriteText call.
	*/
	if (fr) {
	    W_WriteText(tstatw, x, yloc, textColor, timer,
			TIMESTRLEN, W_RegularFont);
	} else {
	    while (pos < TIMESTRLEN) {
		if (timer[pos] == lasttimer[pos]) {
		    if (left <= right)
			W_WriteText(tstatw, x + left * W_Textwidth, yloc, textColor,
			     timer + left, right - left + 1, W_RegularFont);
		    left = pos + 1;
		    right = pos;
		} else
		    right++;
		pos++;
	    }
	    if (left <= right)
		W_WriteText(tstatw, x + left * W_Textwidth, yloc, textColor,
			    timer + left, right - left + 1, W_RegularFont);
	    }
	oldtime = now;
	strcpy(lasttimer, timer);
    }
    return;
}

void
db_flags(int fr)
{
    static float old_kills = -1.0;
    static int old_torp = -1;
    static int old_drone = 0;
    static int old_totmissiles = 0;
    static unsigned int old_flags = ~(unsigned int) 0;
    static int old_tourn = -1;
    static int old_spd = -1;
    unsigned char current_tourn;
    char    buf[26];
    int    i;

#ifdef UNIX_SOUND
    if ((me->p_etemp > me->p_ship->s_maxegntemp) && (me->p_flags & PFENG))
          maybe_play_sound (SND_THERMAL); /* Engines Over Thermal Limit, play ONLY once */
    else  sound_completed  (SND_THERMAL); /* Done with Etmp, Replay Sound if Etmp Again */
#endif

    if (fr || me->p_flags != old_flags) {
	buf[0] = (me->p_flags & PFSHIELD ? 'S' : ' ');

	if (me->p_flags & PFGREEN)
	    buf[1] = 'G';
	else if (me->p_flags & PFYELLOW)
	    buf[1] = 'Y';
	else
	    buf[1] = 'R';
	buf[2] = (me->p_flags & (PFPLLOCK | PFPLOCK) ? 'L' : ' ');
	buf[3] = (me->p_flags & PFREPAIR ? 'R' : ' ');
	buf[4] = (me->p_flags & PFBOMB ? 'B' : ' ');
	buf[5] = (me->p_flags & PFORBIT ? 'O' : ' ');
	buf[6] = (me->p_flags & (PFDOCK | PFDOCKOK)) ? 'D' : ' ';
	buf[7] = (me->p_flags & PFCLOAK ? 'C' : ' ');
	buf[8] = (me->p_flags & PFWEP ? 'W' : ' ');
	buf[9] = (me->p_flags & PFENG ? 'E' : ' ');
	if (me->p_flags & PFPRESS)
	    buf[10] = 'P';
	else if (me->p_flags & PFTRACT)
	    buf[10] = 'T';
	else
	    buf[10] = ' ';
	if (me->p_flags & PFBEAMUP)
	    buf[11] = 'u';
	else if (me->p_flags & PFBEAMDOWN)
	    buf[11] = 'd';
	else
	    buf[11] = ' ';

	/* Flags turn red with etemped/wtemped [BDyess] */
	if (me->p_flags & (PFWEP | PFENG))
	    W_WriteText(tstatw, 2, 3, W_Red, "Flags", 5, W_RegularFont);
	else
	    W_WriteText(tstatw, 2, 3, textColor, "Flags", 5, W_RegularFont);
	/* blue 'Warp' text [BDyess] */
	if (me->p_flags & PFWARP)
	    W_WriteText(tstatw, 2 + 6 * W_Textwidth, 3, W_Cyan, "    Warp", 8,
			W_BoldFont);
	/* red 'Afterbrn' text [BDyess] */
	else if (me->p_flags & PFAFTER)
	    W_WriteText(tstatw, 2 + 6 * W_Textwidth, 3, W_Red, "Afterbrn", 8,
			W_RegularFont);
	/* yellow 'WarpPrep' or 'WrpPause' text [BDyess] */
	else if (me->p_flags & PFWARPPREP)
	    if (me->p_flags & PFWPSUSPENDED)
		W_WriteText(tstatw, 2 + 6 * W_Textwidth, 3, W_Yellow, "WrpPause",
			    8, W_RegularFont);
	    else
		W_WriteText(tstatw, 2 + 6 * W_Textwidth, 3, W_Yellow, "WarpPrep",
			    8, W_RegularFont);
	/* green 'Impulse' text [BDyess] */
	else if (me->p_speed > 0)
	    W_WriteText(tstatw, 2 + 6 * W_Textwidth, 3, W_Green, " Impulse", 8,
			W_RegularFont);
	/* white 'Stopped' text [BDyess] */
	else
	    W_WriteText(tstatw, 2 + 6 * W_Textwidth, 3, textColor, " Stopped", 8,
			W_RegularFont);
	W_WriteText(tstatw, 2, 3 + (W_Textheight + SPACING), textColor, buf, 12, W_RegularFont);
	old_flags = me->p_flags;
	old_spd = me->p_speed;
    } else if ( (me->p_speed == 0 && old_spd) ||
	        (me->p_speed && old_spd == 0) ) {
	if (me->p_speed > 0)
	    W_WriteText(tstatw, 2 + 6 * W_Textwidth, 3, W_Yellow, " Impulse", 8,
			W_RegularFont);
	else
	    W_WriteText(tstatw, 2 + 6 * W_Textwidth, 3, textColor, " Stopped", 8,
			W_RegularFont);
	old_spd = me->p_speed;
    }
    current_tourn = paradise ? status2->tourn : status->tourn;

    if (fr || current_tourn != old_tourn) {
	if (current_tourn)
	    W_WriteText(tstatw, 74, 3 + W_Textheight + SPACING, textColor, "T", 1, W_BoldFont);
	else
	    W_WriteText(tstatw, 74, 3 + W_Textheight + SPACING, textColor, " ", 1, W_BoldFont);

	old_tourn = current_tourn;
    }
    if (fr || me->p_kills != old_kills) {
	if (me->p_kills > 0.0) {
/*	  W_WriteText (tstatw, column[3], 3+W_Textheight + SPACING, textColor, "Kills:", 6, W_RegularFont);*/
	    sprintf(buf, "Kills: %5.2f", me->p_kills);
	    W_WriteText(tstatw, column[3], 3 + W_Textheight + SPACING, textColor, buf, strlen(buf), W_RegularFont);
	} else {
	    W_ClearArea(tstatw, column[3], 3 + W_Textheight + SPACING, 12 * W_Textwidth, W_Textheight);
	}
	old_kills = me->p_kills;
    }
    if (fr || me->p_ntorp != old_torp) {
	if (me->p_ntorp > 0) {
/*	  W_WriteText (tstatw, column[3]+17*W_Textwidth, 3+W_Textheight + SPACING, textColor, "Torps:",
		       6, W_RegularFont);
*/
	    sprintf(buf, "Torps: %d", me->p_ntorp);
	    W_WriteText(tstatw, column[3] + 17 * W_Textwidth, 
	    	        3 + W_Textheight + SPACING, textColor, buf, 
			strlen(buf), W_RegularFont);
	} else {
	    W_ClearArea(tstatw, column[3] + 17 * W_Textwidth, 3 + W_Textheight + SPACING, 8 * W_Textwidth, 10);
	}
	old_torp = me->p_ntorp;
    }
    /* code to show the number of drones out */
    strcpy(buf, "Missiles ");
    if (fr || me->p_totmissiles != old_totmissiles || me->p_ndrone != old_drone)
    {
	if (me->p_totmissiles > 0)
	    sprintf(buf + strlen(buf), "Left: %d ", me->p_totmissiles);
	old_totmissiles = me->p_totmissiles;
	if (me->p_ndrone > 0)
	    sprintf(buf + strlen(buf), "Out: %d ", me->p_ndrone);
	old_drone = me->p_ndrone;
	if (!me->p_totmissiles && !me->p_ndrone) {	/* clear missile text */
	    W_ClearArea(tstatw, column[3], 3 + 2 * (W_Textheight + SPACING), 24 * W_Textwidth, W_Textheight);
	} else {
	    for (i = strlen(buf); i < 24; i++)
		buf[i] = ' ';
	    buf[24] = 0;
	    W_WriteText(tstatw, column[3], 3 + 2 * (W_Textheight + SPACING), textColor, buf, 24, W_RegularFont);
        }
    }
}

void
db_redraw_krp(int fr)
{
    static int old_spd = -1, old_cur_spd = -1;
    static int old_shl = -1, old_dam = -1;
    static int old_arm = -1, old_cur_arm = -1;
    static int old_wpn = -1, old_egn = -1;
    static int old_ful = -1;
    register int cur_max;
    register int value;
    int     color;
    register int mid;

    if (fr)
	W_ClearWindow(tstatw);

    db_flags(fr);

    db_timer(fr, 1, 3 + 2 * (W_Textheight + SPACING));

    light_erase();

    if (paradise)
	cur_max = me->p_ship->s_maxspeed - (int) ((float) me->p_ship->s_maxspeed
		  * (float) me->p_damage / (float) me->p_ship->s_maxdamage);
    else
	cur_max = (me->p_ship->s_maxspeed + 1) - ((me->p_ship->s_maxspeed + 1) * me->p_damage)
	    / me->p_ship->s_maxdamage;
    if (cur_max > me->p_ship->s_maxspeed)
	cur_max = me->p_ship->s_maxspeed;
    if (cur_max < 0)
	cur_max = 0;

    if (fr || me->p_speed != old_spd || cur_max != old_cur_spd) {
	if (me->p_speed >= me->p_ship->s_maxspeed - 2)
	    color = W_Red;
	else
	    color = W_Green;
	db_bar("Sp", column[1], 3,
	   me->p_speed, cur_max, me->p_ship->s_maxspeed, DB_3DIGITS, color);
	old_spd = me->p_speed;
	old_cur_spd = cur_max;
    }
    if (fr || me->p_shield != old_shl) {
	value = (100 * me->p_shield) / me->p_ship->s_maxshield;
	if (value <= 16)
	    color = W_Red;
	else if (value <= 66)
	    color = W_Yellow;
	else
	    color = W_Green;
	db_bar("Sh", column[1], 3 + W_Textheight + SPACING,
	     me->p_shield, me->p_ship->s_maxshield, me->p_ship->s_maxshield,
	       DB_3DIGITS, color);
	old_shl = me->p_shield;
    }
    if (fr || me->p_damage != old_dam) {
	value = (100 * (me->p_ship->s_maxdamage - me->p_damage)) / me->p_ship->s_maxdamage;
	if (value <= 16)
	    color = W_Red;
	else if (value <= 66)
	    color = W_Yellow;
	else
	    color = W_Green;
	db_bar("Hu", column[1], 3 + 2 * (W_Textheight + SPACING),
	       (me->p_ship->s_maxdamage - me->p_damage),
	       me->p_ship->s_maxdamage, me->p_ship->s_maxdamage,
	       DB_3DIGITS, color);
	old_dam = me->p_damage;
    }

    if(F_armies_shipcap != 1)
    {
      if (me->p_ship->s_type == ASSAULT)
	  cur_max = (((me->p_kills * 3) > me->p_ship->s_maxarmies) ?
		     me->p_ship->s_maxarmies : (int) (me->p_kills * 3));
      else if (me->p_ship->s_type == STARBASE)
	  cur_max = me->p_ship->s_maxarmies;
      else
	  cur_max = (((me->p_kills * 2) > me->p_ship->s_maxarmies) ?
		     me->p_ship->s_maxarmies : (int) (me->p_kills * 2));
    }
    else
    {
      if (me->p_ship->s_armies & 0x80)
        cur_max = me->p_kills * (me->p_ship->s_armies & 0x7f) / 10;
      else
        cur_max = me->p_ship->s_maxarmies;

      if(cur_max > me->p_ship->s_maxarmies)
        cur_max = me->p_ship->s_maxarmies;
    }

    if (fr || me->p_armies != old_arm || cur_max != old_cur_arm) {
	value = me->p_armies;
	mid = me->p_ship->s_maxarmies / 3;
	if (value <= mid)
	    color = W_Green;
	else if (value > mid * 2)
	    color = W_Red;
	else
	    color = W_Yellow;
	db_bar("Ar", column[2], 3,
	 me->p_armies, cur_max, me->p_ship->s_maxarmies, DB_3DIGITS, color);
	old_arm = me->p_armies;
	old_cur_arm = cur_max;
    }
    if (fr || me->p_wtemp != old_wpn) {
	value = (100 * me->p_wtemp) / me->p_ship->s_maxwpntemp;
	if (value <= 16)
	    color = W_Green;
	else if (value <= 66)
	    color = W_Yellow;
	else
	    color = W_Red;
	db_bar("Wt", column[2], 3 + W_Textheight + SPACING,
	       me->p_wtemp / 10, me->p_ship->s_maxwpntemp / 10, me->p_ship->s_maxwpntemp / 10, DB_3DIGITS
	       ,color);
	old_wpn = me->p_wtemp;
    }
    if (fr || me->p_etemp != old_egn) {
	value = (100 * me->p_etemp) / me->p_ship->s_maxegntemp;
	if (value <= 16)
	    color = W_Green;
	else if (value <= 66)
	    color = W_Yellow;
	else
	    color = W_Red;
	db_bar("Et", column[2], 3 + 2 * (W_Textheight + SPACING),
	       me->p_etemp / 10, me->p_ship->s_maxegntemp / 10, me->p_ship->s_maxegntemp / 10, DB_3DIGITS
	       ,color);
	old_egn = me->p_etemp;
    }
    if (fr || me->p_fuel != old_ful) {
	value = ((100 * me->p_fuel) / me->p_ship->s_maxfuel);
	if (value <= 16)
	    color = W_Red;
	else if (value <= 66)
	    color = W_Yellow;
	else
	    color = W_Green;
	db_bar("Fu", column[3], 3,
	       me->p_fuel, me->p_ship->s_maxfuel, me->p_ship->s_maxfuel, DB_5DIGITS, color);
	old_ful = me->p_fuel;
    }
}


void
db_redraw_BRM(int fr)
{
    static int old_spd = -1, old_cur_spd = -1;
    static int old_shl = -1, old_dam = -1;
    static int old_arm = -1, old_cur_arm = -1;
    static int old_wpn = -1, old_egn = -1;
    static int old_ful = -1;
    register int cur_max;
    register int value;
    int     color;
    register int mid;
    register int tmp;

    if (fr)
	W_ClearWindow(tstatw);

    db_flags(fr);

    db_timer(fr, 1, 3 + 2 * (W_Textheight + SPACING));
    light_erase();

    cur_max = (me->p_ship->s_maxspeed + 1) - ((me->p_ship->s_maxspeed + 1) * me->p_damage)
	/ me->p_ship->s_maxdamage;
    if (cur_max > me->p_ship->s_maxspeed)
	cur_max = me->p_ship->s_maxspeed;
    if (cur_max < 0)
	cur_max = 0;

    if (fr || me->p_speed != old_spd || cur_max != old_cur_spd) {
	if (Dashboard == 3) {
	    db_color_bar("Sp", column[1], 3, me->p_speed, me->p_speed, cur_max,
			 me->p_ship->s_maxspeed, DB_3DIGITS);
	} else {
	    if (me->p_speed >= me->p_ship->s_maxspeed - 2)
		color = W_Yellow;
	    else
		color = W_White;
	    db_bar("Sp", column[1], 3,
	    me->p_speed, cur_max, me->p_ship->s_maxspeed, DB_3DIGITS, color);
	}
	old_spd = me->p_speed;
	old_cur_spd = cur_max;
    }
    if (fr || me->p_shield != old_shl) {
	if (Dashboard == 3) {
	    tmp = me->p_ship->s_maxshield - me->p_shield;
	    db_color_bar("Sh", column[1], 3 + W_Textheight + SPACING, tmp, tmp,
			 me->p_ship->s_maxshield, me->p_ship->s_maxshield,
			 DB_3DIGITS);
	} else {
	    value = (100 * me->p_shield) / me->p_ship->s_maxshield;
	    /*
	       mid = (distress[index].max_shld - distress[index].min_shld) /
	       2;
	    */
	    mid = 33;
	    if (value <= mid)
		color = W_Red;
	    /* else if (value < distress[index].max_shld) */
	    else if (value < mid * 2)
		color = W_Yellow;
	    else
		color = W_White;
	    db_bar("Sh", column[1], 3 + W_Textheight + SPACING, me->p_ship->s_maxshield - me->p_shield,
		   me->p_ship->s_maxshield, me->p_ship->s_maxshield, DB_3DIGITS, color);
	}
	old_shl = me->p_shield;
    }
    if (fr || me->p_damage != old_dam) {
	if (Dashboard == 3) {
	    db_color_bar("Hu", column[1], 3 + 2 * (W_Textheight + SPACING),
			 me->p_damage, me->p_damage, me->p_ship->s_maxdamage,
			 me->p_ship->s_maxdamage, DB_3DIGITS);
	} else {
	    value = (100 * me->p_damage) / me->p_ship->s_maxdamage;
	    /* mid = (distress[index].max_dam - distress[index].min_dam) / 2; */
	    mid = 33;
	    /* if (value <= distress[index].min_dam) */
	    if (value <= mid)
		color = W_White;
	    else if (value > mid * 2)
		color = W_Red;
	    else
		color = W_Yellow;
	    db_bar("Hu", column[1], 3 + 2 * (W_Textheight + SPACING),
		   me->p_damage, me->p_ship->s_maxdamage, me->p_ship->s_maxdamage, DB_3DIGITS, color);
	}
	old_dam = me->p_damage;
    }
    if(F_armies_shipcap != 1)
    {
      if (me->p_ship->s_type == ASSAULT)
	  cur_max = (((me->p_kills * 3) > me->p_ship->s_maxarmies) ?
		     me->p_ship->s_maxarmies : (int) (me->p_kills * 3));
      else if (me->p_ship->s_type == STARBASE)
	  cur_max = me->p_ship->s_maxarmies;
      else
	  cur_max = (((me->p_kills * 2) > me->p_ship->s_maxarmies) ?
		     me->p_ship->s_maxarmies : (int) (me->p_kills * 2));
    }
    else
    {
      if (me->p_ship->s_armies & 0x80)
        cur_max = me->p_kills * (me->p_ship->s_armies & 0x7f) / 10;
      else
        cur_max = me->p_ship->s_maxarmies;

      if(cur_max > me->p_ship->s_maxarmies)
        cur_max = me->p_ship->s_maxarmies;
    }

    /* doing rainbow colors for armies makes little since, so I don't */
    if (fr || me->p_armies != old_arm || cur_max != old_cur_arm) {
	if (Dashboard == 3) {
	    db_bar("Ar", column[2], 3,
		   me->p_armies, cur_max, me->p_ship->s_maxarmies, DB_3DIGITS, W_White);
	} else {
	    value = me->p_armies;
	    /*
	       mid = (distress[index].max_arms - distress[index].min_arms) /
	       2;
	    */
	    mid = me->p_ship->s_maxarmies / 3;
	    /* if (value <= distress[index].min_arms) */
	    if (value <= mid)
		color = W_White;
	    else if (value > mid * 2)
		color = W_Red;
	    else
		color = W_Yellow;
	    db_bar("Ar", column[2], 3,
		   me->p_armies, cur_max, me->p_ship->s_maxarmies, DB_3DIGITS, color);
	}
	old_arm = me->p_armies;
	old_cur_arm = cur_max;
    }
    if (fr || me->p_wtemp != old_wpn) {
	if (Dashboard == 3) {
	    tmp = me->p_wtemp / 10;
	    db_color_bar("Wt", column[2], 3 + W_Textheight + SPACING, tmp, tmp,
			 me->p_ship->s_maxwpntemp / 10,
			 me->p_ship->s_maxwpntemp / 10, DB_3DIGITS);
	} else {
	    value = (100 * me->p_wtemp) / me->p_ship->s_maxwpntemp;
	    /*
	       mid = (distress[index].max_wtmp - distress[index].min_wtmp) /
	       2;
	    */
	    mid = 67;
	    if (value > mid)
		color = W_Red;
	    /* else if (value <= distress[index].min_wtmp) */
	    else if (value <= mid / 2)
		color = W_White;
	    else
		color = W_Yellow;
	    db_bar("Wt", column[2], 3 + W_Textheight + SPACING, me->p_wtemp / 10, me->p_ship->s_maxwpntemp / 10,
		   me->p_ship->s_maxwpntemp / 10, DB_3DIGITS, color);
	}
	old_wpn = me->p_wtemp;
    }
    if (fr || me->p_etemp != old_egn) {
	if (Dashboard == 3) {
	    tmp = me->p_etemp / 10;
	    db_color_bar("Et", column[2], 3 + 2 * (W_Textheight + SPACING), tmp, tmp,
			 me->p_ship->s_maxegntemp / 10,
			 me->p_ship->s_maxegntemp / 10, DB_3DIGITS);
	} else {
	    value = (100 * me->p_etemp) / me->p_ship->s_maxegntemp;
	    /*
	       mid = (distress[index].max_etmp - distress[index].min_etmp) /
	       2;
	    */
	    mid = 67;
	    if (value <= mid / 2)
		color = W_White;
	    /* else if (value < mid / 2 + mid) */
	    else if (value < mid)
		color = W_Yellow;
	    else
		color = W_Red;
	    db_bar("Et", column[2], 3 + 2 * (W_Textheight + SPACING), me->p_etemp / 10, me->p_ship->s_maxegntemp / 10,
		   me->p_ship->s_maxegntemp / 10, DB_3DIGITS, color);
	}
	old_egn = me->p_etemp;
    }
    if (fr || me->p_fuel != old_ful) {
	if (Dashboard == 3) {
	    db_color_bar("Fu", column[3], 3, me->p_ship->s_maxfuel - me->p_fuel,
		   me->p_fuel, me->p_ship->s_maxfuel, me->p_ship->s_maxfuel,
			 DB_5DIGITS);
	} else {
	    value = ((100 * me->p_fuel) / me->p_ship->s_maxfuel);
	    /*
	       mid = (distress[index].max_fuel - distress[index].min_fuel) /
	       2;
	    */
	    mid = 33;
	    if (value <= mid)
		color = W_Red;
	    /* else if (value > distress[index].max_fuel) */
	    else if (value > mid * 2)
		color = W_White;
	    else
		color = W_Yellow;
	    db_bar("Fu", column[3], 3,
		   me->p_fuel, me->p_ship->s_maxfuel, me->p_ship->s_maxfuel, DB_5DIGITS, color);
	}
	old_ful = me->p_fuel;
    }
}

void
db_redraw(int fr)
{
    static int oldDashboard = -1;

    if (oldDashboard < 0) {	/* 1st time only? */
	BAR_LENGTH = ((W_WindowWidth(tstatw) - 90 - 4 * W_Textwidth) / 3) - W_Textwidth * 12;
	BAR_LENGTH_THIRD = BAR_LENGTH / 3;
	column[0] = 2;		/* not used */
	column[1] = 90;
	column[2] = 90 + 11 * W_Textwidth + BAR_LENGTH + 6;
	column[3] = column[2] + 11 * W_Textwidth + BAR_LENGTH + 6;
    }
    if (Dashboard != oldDashboard) {
	oldDashboard = Dashboard;
	fr = 1;
    }
    if (Dashboard == 2)
	db_redraw_krp(fr);
    else
	db_redraw_BRM(fr);
}

void
stline(int flag)
{
    static char buf1[80];
    static char buf2[80];
    static char whichbuf = 0;
    static int lastDashboard;

    register char *buf, *oldbuf;
    register char *s = NULL;
    register int i, j;
    int     k = 0;

/* if you don't do something like this, then switching in the options menu
   is 'entertaining'  */
    if (Dashboard != lastDashboard) {
	lastDashboard = Dashboard;
	redrawTstats();
	return;
    }
/* use the new dashboard if we can */
    if (Dashboard) {
	db_redraw(flag);
	return;
    }
    /* Do da clock */
    db_timer(flag, W_WindowWidth(tstatw) - (12 * W_Textwidth + 5), 27);
    light_erase();


    /* Instead of one sprintf, we do all this by hand for optimization */

    if (flag)
	whichbuf = 0;		/* We must completely refresh */

    if (whichbuf != 2) {
	buf = buf1;
	oldbuf = buf2;
    } else {
	buf = buf2;
	oldbuf = buf1;
    }
    buf[0] = (me->p_flags & PFSHIELD ? 'S' : ' ');
    if (me->p_flags & PFGREEN)
	buf[1] = 'G';
    else if (me->p_flags & PFYELLOW)
	buf[1] = 'Y';
    else if (me->p_flags & PFRED)
	buf[1] = 'R';
    buf[2] = (me->p_flags & (PFPLLOCK | PFPLOCK) ? 'L' : ' ');
    buf[3] = (me->p_flags & PFREPAIR ? 'R' : ' ');
    buf[4] = (me->p_flags & PFBOMB ? 'B' : ' ');
    buf[5] = (me->p_flags & PFORBIT ? 'O' : ' ');
    buf[6] = (me->p_flags & PFDOCKOK ? 'D' : ' ');
/*      buf[6] = (me->p_flags & PFDOCK ? 'D' : ' ');*/
    buf[7] = (me->p_flags & PFCLOAK ? 'C' : ' ');
    buf[8] = (me->p_flags & PFWEP ? 'W' : ' ');
    buf[9] = (me->p_flags & PFENG ? 'E' : ' ');
    if (me->p_flags & PFPRESS)
	buf[10] = 'P';
    else if (me->p_flags & PFTRACT)
	buf[10] = 'T';
    else
	buf[10] = ' ';
    if (me->p_flags & PFBEAMUP)
	buf[11] = 'u';
    else if (me->p_flags & PFBEAMDOWN)
	buf[11] = 'd';
    else
	buf[11] = ' ';
    if (!paradise)
	buf[12] = (status->tourn) ? 't' : ' ';
    else
	buf[12] = (status2->tourn) ? 't' : ' ';

    buf[13] = ' ';

/* w/i indicator is a kludge - it just guesses based on the speed of
   the ship */
    sprintf(buf + 14, "%2d%c   %3d %3d  %1d  %6.2f %3d %6d  %4d %4d  ",
	    me->p_speed,	/* (me->p_speed > me->p_ship->s_maxspeed+2) */
	    me->p_flags & PFWARP ? 'w' : me->p_flags & PFAFTER ? 'a' : 'i',
	    me->p_damage, me->p_shield, me->p_ntorp, me->p_kills,
	    me->p_armies, me->p_fuel, me->p_wtemp / 10, me->p_etemp / 10);

    if (whichbuf == 0) {
	/* Draw status line */
	W_WriteText(tstatw, TSTATW_BASEX, 16, textColor, buf, 66, W_RegularFont);
	whichbuf = 1;
    } else {			/* Hacks to make it print only what is
				   necessary */
	whichbuf = 3 - whichbuf;
	j = -1;
	for (i = 0; i < 66; i++) {
	    if (*(buf++) != *(oldbuf++)) {
		/* Different string */
		if (j == -1) {
		    k = i;
		    s = buf - 1;
		}
		j = 0;
	    } else {
		/* Same string */
		if (j == -1)
		    continue;
		j++;
		if (j == 20) {	/* Random number */
		    W_WriteText(tstatw, TSTATW_BASEX + W_Textwidth * k, 16, textColor,
				s, i - k - 19, W_RegularFont);
		    j = -1;
		}
	    }
	}
	if (j != -1) {
	    W_WriteText(tstatw, TSTATW_BASEX + W_Textwidth * k, 16, textColor, s, i - k - j,
			W_RegularFont);
	}
    }
}

void
redrawTstats(void)
{
    char    buf[100];
    int	buffered = W_IsBuffered(tstatw);
    
    if(buffered) {
      W_ClearBuffer(tstatw);
    } else {
      W_ClearWindow(tstatw);
    }

    /* use new dashboard if possible */
    if (Dashboard) {
	db_redraw(1);
	if(buffered)
	  W_DisplayBuffer(tstatw);
	return;
    }
    stline(1);			/* This is for refresh.  We redraw player
				   stats too */
    strcpy(buf, "Flags        Speed  Dam Shd Trp  Kills Ams   Fuel  Wtmp Etmp  Special"	/* "Flags        Warp
											   Dam Shd Torps  Kills
											   Armies   Fuel  Wtemp
	       Etemp" */ );
    W_WriteText(tstatw, TSTATW_BASEX, 5, textColor, buf, strlen(buf), 
    		W_RegularFont);
    sprintf(buf,
	    "Maximum:      %2d    %3d %3d  8         %3d %6d  %4d %4d ",
	    me->p_ship->s_maxspeed, me->p_ship->s_maxdamage,
	    me->p_ship->s_maxshield, me->p_ship->s_maxarmies,
	    me->p_ship->s_maxfuel, me->p_ship->s_maxwpntemp / 10,
	    me->p_ship->s_maxegntemp / 10);
    W_WriteText(tstatw, TSTATW_BASEX, 27, textColor, buf, strlen(buf), 
    		W_RegularFont);

    if(buffered)
      W_DisplayBuffer(tstatw);
}
