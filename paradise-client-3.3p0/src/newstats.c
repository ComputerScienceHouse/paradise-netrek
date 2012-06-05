/*
 * newstats.c
 *
 * As best I can tell, this was written by Tundra Dan.
 */
#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include <limits.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "images.h"

#define NUM_ELS(a)		(sizeof (a) / sizeof (*(a)))
#define NUM_SLIDERS		NUM_ELS(sliders)

#ifdef ARMY_SLIDER
    int     last_armies;        /* XXX: for stats */
#else
    short   last_armies;
#endif                          /* ARMY_SLIDER */

typedef struct slider {
    int     min, max;
    int	    xloc;
    int	    yloc;
    int	    xsize;
    int	    ysize;

    int	    x1loc;
    int	    y1loc;
    int     red;
    int     yellow;
    int     diff;
    int    *var;
    int     lastVal;
}       SLIDER;

typedef struct record {
    int    *data;
    int     last_value;
}       RECORD;

static SLIDER sliders[] = {
    { 0, 100, 0, 0, 0, 0},
    { 0, 100, 0, 0, 0, 44},
    { 0, 10000, 9, 64, 4, 85},
    { 0, 1200, 0, 0, 0, 0},
    { 0, 1200, 41, 122, 3, 20},
    { 0, 10, 3, 64, 1, 85},
};

typedef struct race {
    int     w_x1;
    int     w_x2;
    int     w_y1;
    int     w_y2;
    int     w_y3;
    int     w_y4;
    int     w_a1;
    int	    w_a2;
    int     w_l1;
    int     w_a3;
    int	    w_a4;
    int     w_l2;

} RACE;

static RACE race_vars[] = {
    { 78, 41, 112, 142, 112, 122, 0, 0,  180, 180, 180, 180 },
    { 92, 27, 87,  112, 87,  92,  0, 0,  180, 250, 180, 110 },
    { 92, 26, 105, 134, 129, 134, 0, 10, 170, 180, 180, 180 },
    { 87, 30, 89,  134, 120, 130, 0, 80, 100, 240, 180, 120 },
};

float old_kills = -1.0;
int old_team = -2;
int old_torp = -1;
int old_drone = 0;
int old_totmissiles = 0;
int out_msl;
int tot_msl;
int out_ftr;
int tot_ftr;
int missile = 0;
int tick = -2;
int tick1 = 0;
W_Image *image;
/* state variables to keep from redrawing needlessly [BDyess] */
int old_tourn = -1, old_flags = 0;
int old_armies = -1;

unsigned char current_tourn;

char tstat[12];   /* All Text Data */

/*static int textWidth = 0;*/
static int initialized = 0;

/* Prototypes */
static void box P((int filled, int x, int y, int wid, int hei, W_Color color));
static void initStats P((void));
static int st_width = -1;
void calibrate_Newstats P((void));

void format_value(value, start, length)
int value, start, length;
{
   int i;
   int x;
   int flag = 0;
   int p = 1;

   for(i = 0; i < length; i++) {
      p = (10 * p);
   }
   for(i = 0; i < length; i++) {
      p = p / 10;  

      if (p == 0) {
         p = 1;
      }
      if (i == 0) {
         x = (value/ p);
      } else {
         x = ((value/ p) % 10);
      }

      if ((i == length - 1) && (x == 0) && (flag == 0)) {
         tstat[start + i] = '0';
         return;
      }
      if ((x == 0) && (flag == 0)) {
         tstat[start + i] = ' ';
      } else {
         flag = 1;
         tstat[start + i] = '0' + x;
      } 
   }
   return;
}

void stat_timer(fr, xloc, yloc) 
int fr, xloc, yloc; 
{
   static time_t oldtime = -1;
   static int lastTimerType = -1;
   time_t  now = 0;
   static char lasttimer[9], *timer;
   int     left, right, x, pos;

   if (timerType != lastTimerType || fr) {
      fr = 1;
      lastTimerType = timerType;
      switch (timerType) {
      case T_NONE:
         W_ClearArea(newstatwin,xloc,yloc, 12 * W_Textwidth, W_Textheight);
         strcpy(lasttimer, "        ");
         oldtime = now;
         break;
      case T_DAY:
         W_WriteText(newstatwin,xloc,yloc,textColor,"NOW",3,W_RegularFont);
         break;
      case T_SERVER:
         W_WriteText(newstatwin,xloc,yloc,textColor,"SRV",3,W_RegularFont);
         break;
      case T_SHIP:
         W_WriteText(newstatwin,xloc,yloc,textColor,"SHP",3,W_RegularFont);
         break;
      case T_USER:
         W_WriteText(newstatwin,xloc,yloc,textColor,"TMR",3,W_RegularFont);
         break;
      }
   }
   if (!timerType)
      return;
   now = time(NULL);
   if (now != oldtime || fr) {
      /*
       * get the timer string and start comparing it with the old one. Only
       * print the differences
       */
      timer = timeString(now - timeBank[timerType]);
      x = xloc + 4 * W_Textwidth;
      left = 0;
      right = -1;
      pos = 0;

      /*
       *  run through the string to find any differences.  Print any
       *  continuous differences with one W_WriteText call.
       */
      if (fr) {
         W_WriteText(newstatwin, x, yloc, textColor, timer, 8, W_RegularFont);
      } else {
         while (pos < 8) {
            if (timer[pos] == lasttimer[pos]) {
               if (left <= right)
                  W_WriteText(newstatwin, x + left * W_Textwidth, yloc, 
                  textColor, timer + left, right - left + 1, W_RegularFont);
               left = pos + 1;
               right = pos;
            } else
               right++;
            pos++;
        }
        if (left <= right)
           W_WriteText(newstatwin, x + left * W_Textwidth, yloc, textColor,
                       timer + left, right - left + 1, W_RegularFont);
      }
      oldtime = now;
      strcpy(lasttimer, timer);
   }
   return;
}


static void initStats()
{
    int     i;

    if ((me->p_teami) != old_team) {
       W_ClearWindow(newstatwin);
       old_team = me->p_teami;
       switch(old_team) {
          case 0:
             sliders[4].ysize = 23;
             sliders[4].xsize = 3;
             break;
          case 1:
             sliders[4].ysize = 28;
             sliders[4].xsize = 3;
             break;
          case 2:
             sliders[4].ysize = 24;
             sliders[4].xsize = 4;
             break;
          default:
             sliders[4].ysize = 27;
             sliders[4].xsize = 3;
             break;
       }
    }
    calibrate_Newstats();

    if (initialized)
	return;
    initialized = 1;

    sliders[0].var = &(me->p_shield);
    sliders[1].var = &(me->p_damage);
    sliders[2].var = &(me->p_fuel);
    sliders[3].var = &(me->p_wtemp);
    sliders[4].var = &(me->p_etemp);
    sliders[5].var = &(me->p_speed);
    for (i = 0; i < NUM_SLIDERS; i++) {
	sliders[i].diff = sliders[i].max - sliders[i].min;
	sliders[i].lastVal = 0;
    }
    st_width = W_WindowWidth(newstatwin);
}

void 
redrawNewStats()
{
    int     i;

    W_ClearWindow(newstatwin);
    initStats();

    out_msl = 0;
    tot_msl = 0;
    out_ftr = 0;
    tot_ftr = 0;
    image = getImage(I_NEWSTATS_STATUS_TEMPLATE);
    W_DrawImage(newstatwin, 0, 0, -1, image, W_White);

    for (i = 0; i < NUM_SLIDERS; i++) {
        box(1, sliders[i].xloc, sliders[i].yloc, sliders[i].xsize, 
        sliders[i].ysize, W_Black);    
	sliders[i].lastVal = 0;
    }
    /* change all the status values so that everything will be redrawn[BDyess]*/
    old_tourn = -1; 
    old_flags = me->p_flags ^ INT_MAX;
    old_armies = -1;
    updateNewStats();
}

void updateNewStats()
{
    int     i, value, lastVal, new_y;
    int     r, y, t;
    SLIDER *s;
    RACE *rs;
    int     changed_flags;	/* flags that have changed.  XOR of old and
                                   new flags.  [BDyess] */

    initStats();

    rs = &race_vars[me->p_teami];

    stat_timer(1, 26, 375);			/* writes the time */
    /* draws the converging dots at the top of the window */
    if (tick != -2) {
       box(1, 32 + tick, 2, 2, 2, W_Black); 
       box(1, 86 - tick, 2, 2, 2, W_Black); 
    }
    if (tick == 26)
       tick = -2;
    tick += 2; 
    box(1, 32 + tick, 2, 2, 2, W_Yellow); 
    box(1, 86 - tick, 2, 2, 2, W_Yellow); 

    /* only update T flag if changed [BDyess] */
    current_tourn = paradise ? status2->tourn : status->tourn;
    if(current_tourn != old_tourn) {
       old_tourn = current_tourn;
       if (current_tourn) {
	  image = getImage(I_NEWSTATS_FLAG_WAR);
	  W_DrawImage(newstatwin, 50, 207, -1, image, W_Red);
       } else {
          box(1, 50, 207, 50, 20, W_Black);
       }
    }
    /* clear parts of the screen? */
    changed_flags = me->p_flags ^ old_flags;
    old_flags = me->p_flags;
    if (changed_flags & PFBOMB) {
      if ((me->p_flags & PFBOMB)) {
	 image = getImage(I_NEWSTATS_FLAG_BOMB);
	 W_DrawImage(newstatwin, 40, 202, -1, image, W_Cyan);
	 image = getImage(I_NEWSTATS_FLAG_FUSE);
	 W_DrawImage(newstatwin, 40, 222, -1, image, W_Red);
      } else {  /* clear the old images [BDyess] */
         box(1, 40, 202, 10, 25, backColor);
      }
    }
    /* only update if needed [BDyess] */
    if (changed_flags & (PFBEAMUP|PFBEAMDOWN)) {
      if ((me->p_flags & PFBEAMUP)) {
	 image = getImage(I_NEWSTATS_FLAG_UP);
	 W_DrawImage(newstatwin, 5, 206, -1, image, W_Yellow);
	 image = getImage(I_NEWSTATS_FLAG_BEAMING);
	 W_DrawImage(newstatwin, 5, 212, -1, image, W_Cyan);
	 image = getImage(I_NEWSTATS_FLAG_DOWN);
	 W_DrawImage(newstatwin, 5, 222, -1, image, W_Cyan);
      } else if ((me->p_flags & PFBEAMDOWN)) {
	 image = getImage(I_NEWSTATS_FLAG_UP);
	 W_DrawImage(newstatwin, 5, 206, -1, image, W_Cyan);
	 image = getImage(I_NEWSTATS_FLAG_BEAMING);
	 W_DrawImage(newstatwin, 5, 212, -1, image, W_Cyan);
	 image = getImage(I_NEWSTATS_FLAG_DOWN);
	 W_DrawImage(newstatwin, 5, 222, -1, image, W_Yellow);
      } else {
         /* clear the old images [BDyess] */
	 box(1, 5, 206, 12, 22, backColor);
      }
    }
    if (changed_flags & PFAFTER) {
      image = getImage(I_NEWSTATS_FLAG_SPEED2);
      if (me->p_flags & PFAFTER) {
	 W_DrawImage(newstatwin, 22, 184, -1, image, W_Yellow);
      } else {
         box(1, 22, 184, image->width, image->height, backColor);
      }
    }
    /* draw army picture, but only if the state of carrying/not carrying has
       changed.  [BDyess] */
    if (old_armies != me->p_armies) {
      if(!(old_armies > 0 && me->p_armies > 0)) {
	image = getImage(I_NEWSTATS_FLAG_ARMY);
	if (me->p_armies > 0) {
	   W_DrawImage(newstatwin, 0, 184, -1, image, W_Yellow);
	} else {
	   box(1, 0, 184, image->width, image->height, backColor);
	}
      }
      old_armies = me->p_armies;
    }
    if (changed_flags & PFREPAIR) {
      image = getImage(I_NEWSTATS_FLAG_REPAIR);
      if ((me->p_flags & PFREPAIR)) {
	 W_DrawImage(newstatwin, 0, 207, -1, image, W_Cyan);
      } else {
         box(1, 0, 207, image->width, image->height, backColor);
      }
    }

    if (changed_flags & PFORBIT || changed_flags & (PFGREEN|PFYELLOW|PFRED)) {
      /* erase old [BDyess] */
      box(1, 16, 33, 24, 5, W_Black);
      box(1, 18, 27, 18, 18, W_Black);
      /* draw new [BDyess] */
      if (me->p_flags & PFGREEN) {
	 image = getImage(I_NEWSTATS_FLAG_ALERT);
	 W_DrawImage(newstatwin, 18, 27, -1, image, gColor);
      } else if (me->p_flags & PFYELLOW) {
	 image = getImage(I_NEWSTATS_FLAG_ALERT);
	 W_DrawImage(newstatwin, 18, 27, -1, image, yColor);
      } else if (me->p_flags & PFRED) {
	 image = getImage(I_NEWSTATS_FLAG_ALERT);
	 W_DrawImage(newstatwin, 18, 27, -1, image, rColor);
      }
      if (me->p_flags & PFORBIT) {
	 W_WriteArc(0, newstatwin, 28, 35, 24, 3, 175, 185, W_Grey);
	 image = getImage(I_NEWSTATS_FLAG_TORP);
	 W_DrawImage(newstatwin, 38, 34, -1, image, W_Cyan);
      }
    }

/****************/
/* STOPPED HERE */
/****************/

    if (me->p_nplasmatorp > 0) {
       image = getImage(I_NEWSTATS_FLAG_PLASMA);
       W_DrawImage(newstatwin, 58, 18, -1, image, W_White);
       W_WriteText(newstatwin, 70, 258, W_Red, "OUT", 3, W_RegularFont); 
    } else {
       W_WriteText(newstatwin, 70, 258, W_Black, "OUT", 3, W_RegularFont); 
       box(1, 58, 18, 8, 8, W_Black);
    }
    box(1, 9, 237, 33, 33, W_Black);
    if (me->p_flags & PFCLOAK) {
       box(1, 9, 237, 33, 33, W_Black);
       image = getImage(I_NEWSTATS_FLAG_CLOAK);
       W_DrawImage(newstatwin, 9, 238, -1, image, W_Grey);
    }
    if (me->p_flags & PFTRACT) {
       box(1, 9, 237, 33, 33, W_Black);
       image = getImage(I_NEWSTATS_FLAG_TRACTOR);
       W_DrawImage(newstatwin, 9, 238, -1, image, W_Green);
    }
    if (me->p_flags & PFPRESS) {
       box(1, 9, 237, 33, 33, W_Black);
       image = getImage(I_NEWSTATS_FLAG_PRESSOR);
       W_DrawImage(newstatwin, 9, 238, -1, image, W_Yellow);
    }
    if (me->p_flags & PFWARPPREP) {
       W_WriteArc(0, newstatwin, 24, 254, 28, 29, 0, 360, W_Grey);
       if (tick1 == 4)
          tick1 = 0;
       switch(tick1) {
          case 0: 
             box(1, 23, 238, 2, 2, W_Cyan);
             box(1, 11, 261, 2, 2, W_Cyan);
             box(1, 35, 261, 2, 2, W_Cyan);
             break; 
          case 1: 
             box(1, 30, 266, 2, 2, W_Cyan);
             box(1, 30, 240, 2, 2, W_Cyan);
             box(1,  9, 253, 2, 2, W_Cyan);
             break; 
          case 2: 
             box(1, 23, 268, 2, 2, W_Cyan);
             box(1, 11, 245, 2, 2, W_Cyan);
             box(1, 35, 245, 2, 2, W_Cyan);
             break; 
          case 3: 
             box(1, 17, 266, 2, 2, W_Cyan);
             box(1, 17, 240, 2, 2, W_Cyan);
             box(1, 38, 253, 2, 2, W_Cyan);
             break; 
       }
       tick1++;
    }

    format_value((int)me->p_kills, 0, 4);
    tstat[4] = '.';
    tstat[5] = '0' + ((int)(me->p_kills * 10) % 10);
    tstat[6] = '0' + ((int)(me->p_kills * 100) % 10);
    W_WriteText(newstatwin, 56, 339, W_Yellow, tstat, 7, W_RegularFont); 
 
    format_value(me->p_armies, 0, 5);
    W_WriteText(newstatwin, 60, 354, W_Yellow, tstat, 5, W_RegularFont); 

    if ((me->p_ndrone != old_drone) || (me->p_totmissiles != old_totmissiles)){ 
       if ((me->p_ndrone > 0) && (me->p_totmissiles == 0)) {  /* Fighter */
          missile = 1;
          tot_ftr = npthingies;
          out_ftr = me->p_ndrone;
       } else {  /* Missle */
          if (tot_ftr > 0) {
             missile = 1;
             tot_ftr = npthingies;
             tot_msl = 0;
             out_msl = 0;
             out_ftr = 0;
          } else {
             missile = 0;
             tot_msl = me->p_totmissiles;
             out_ftr = 0;
             out_msl = me->p_ndrone;
          }
       }
    }
    old_drone = me->p_ndrone;
    old_totmissiles = me->p_totmissiles;

    if (missile == 0) {
       format_value(out_msl, 0, 2);
       tstat[2] = '/';
       format_value(tot_msl, 3, 2);
       W_WriteText(newstatwin, 65, 248, W_White, tstat, 5, W_RegularFont);
       box(1, 50, 25, 4, 8, W_Black);
       box(1, 56, 24, 4, 8, W_Black);
       box(1, 63, 24, 4, 8, W_Black);
       box(1, 69, 25, 4, 8, W_Black);
       box(1, 44, 25, 4, 8, W_Black);
       box(1, 75, 25, 4, 8, W_Black);
       box(1, 38, 25, 4, 8, W_Black);
       box(1, 81, 25, 4, 8, W_Black);
       image = getImage(I_NEWSTATS_FLAG_MISSILE);
       if (me->p_ndrone > 0)
	  W_DrawImage(newstatwin, 56, 24, -1, image, W_White);
       if (me->p_ndrone > 1)
	  W_DrawImage(newstatwin, 63, 24, -1, image, W_White);
       if (me->p_ndrone > 2)
	  W_DrawImage(newstatwin, 50, 25, -1, image, W_White);
       if (me->p_ndrone > 3)
	  W_DrawImage(newstatwin, 69, 25, -1, image, W_White);
       if (me->p_ndrone > 4)
	  W_DrawImage(newstatwin, 44, 25, -1, image, W_White);
       if (me->p_ndrone > 5)
	  W_DrawImage(newstatwin, 75, 25, -1, image, W_White);
       if (me->p_ndrone > 6)
	  W_DrawImage(newstatwin, 38, 25, -1, image, W_White);
       if (me->p_ndrone > 7)
	  W_DrawImage(newstatwin, 81, 25, -1, image, W_White);
    } else {
       format_value(out_ftr, 0, 2);
       tstat[2] = '/';
       format_value(tot_ftr, 3, 2);
       W_WriteText(newstatwin, 65, 239, W_White, tstat, 5, W_RegularFont);
       box(1, 50, 25, 5, 8, W_Black);
       box(1, 56, 24, 5, 8, W_Black);
       box(1, 63, 24, 5, 8, W_Black);
       box(1, 69, 25, 5, 8, W_Black);
       box(1, 44, 25, 5, 8, W_Black);
       box(1, 75, 25, 5, 8, W_Black);
       box(1, 38, 25, 5, 8, W_Black);
       box(1, 81, 25, 5, 8, W_Black);
       box(1, 50, 25, 5, 8, W_Black);
       box(1, 56, 24, 5, 8, W_Black);
       box(1, 63, 24, 5, 8, W_Black);
       box(1, 69, 25, 5, 8, W_Black);
       box(1, 44, 25, 5, 8, W_Black);
       box(1, 75, 25, 5, 8, W_Black);
       box(1, 38, 25, 5, 8, W_Black);
       box(1, 81, 25, 5, 8, W_Black);
       image = getImage(I_NEWSTATS_FLAG_FIGHTER);
       if (me->p_ndrone > 0)
	  W_DrawImage(newstatwin, 56, 24, -1, image, W_White);
       if (me->p_ndrone > 1)
	  W_DrawImage(newstatwin, 63, 24, -1, image, W_White);
       if (me->p_ndrone > 2)
	  W_DrawImage(newstatwin, 50, 25, -1, image, W_White);
       if (me->p_ndrone > 3)
	  W_DrawImage(newstatwin, 69, 25, -1, image, W_White);
       if (me->p_ndrone > 4)
	  W_DrawImage(newstatwin, 44, 25, -1, image, W_White);
       if (me->p_ndrone > 5)
	  W_DrawImage(newstatwin, 75, 25, -1, image, W_White);
       if (me->p_ndrone > 6)
	  W_DrawImage(newstatwin, 38, 25, -1, image, W_White);
       if (me->p_ndrone > 7)
	  W_DrawImage(newstatwin, 81, 25, -1, image, W_White);
    }
    box(1, 57, 32, 4, 3, W_Black);
    box(1, 63, 32, 4, 3, W_Black);
    box(1, 51, 33, 4, 3, W_Black);
    box(1, 69, 33, 4, 3, W_Black);
    box(1, 45, 35, 4, 3, W_Black);
    box(1, 75, 35, 4, 3, W_Black);
    box(1, 39, 38, 4, 3, W_Black);
    box(1, 81, 38, 4, 3, W_Black);
    image = getImage(I_NEWSTATS_FLAG_TORP);
    if (me->p_ntorp > 0)  
       W_DrawImage(newstatwin, 57, 32, -1, image, W_White);
    if (me->p_ntorp > 1)  
       W_DrawImage(newstatwin, 63, 32, -1, image, W_White);
    if (me->p_ntorp > 2)  
       W_DrawImage(newstatwin, 51, 33, -1, image, W_White);
    if (me->p_ntorp > 3)  
       W_DrawImage(newstatwin, 69, 33, -1, image, W_White);
    if (me->p_ntorp > 4)  
       W_DrawImage(newstatwin, 45, 35, -1, image, W_White);
    if (me->p_ntorp > 5)  
       W_DrawImage(newstatwin, 75, 35, -1, image, W_White);
    if (me->p_ntorp > 6)  
       W_DrawImage(newstatwin, 39, 38, -1, image, W_White);
    if (me->p_ntorp > 7)  
       W_DrawImage(newstatwin, 81, 38, -1, image, W_White);
    
    switch(me->p_teami) {
       case 0: /* FED */
          s = &sliders[1];            /* Hull Damage Stuff */
          value = *(s->var);
          lastVal = sliders[1].lastVal;
          if (value > s->max)
	     value = s->max;
          t = value * sliders[1].ysize / s->diff;
          r = lastVal * sliders[1].ysize / s->diff;
          W_WriteArc(1, newstatwin, 60, 76, r + 9, r + 9, 0, 180, W_Black);
          W_WriteArc(1, newstatwin, 60, 76, t + 8, t + 8, 0, 180, W_Red);

          if (me->p_flags & PFENG) { /* Engine Temp */
             box(1, 54, 111, 12, 16, W_Red);
          } else {
             box(1, 54, 111, 12, 16, W_White);
          }
          if (me->p_flags & PFWEP) { /* Weapon Temp */
             box(1, 53, 81, 14, 9, W_Red);
             box(1, 56, 129, 8, 7, W_Red);
          } else {
             box(1, 53, 81, 14, 9, W_White);
             box(1, 56, 129, 8, 7, W_White);
          }
	  image = getImage(I_NEWSTATS_FLAG_FED);
	  W_DrawImage(newstatwin, 0, 0, -1, image, W_Yellow);
	  image = getImage(I_NEWSTATS_FLAG_SHIP_FED);
	  W_DrawImage(newstatwin, 26, 47, -1, image, W_White);
          sliders[4].xloc = 40; 
          sliders[4].yloc = 119;
          sliders[4].x1loc = 77; 
          sliders[4].y1loc = 119;
          break;
       case 1: /* ROM */
          s = &sliders[1];            /* Hull Damage Stuff */
          value = *(s->var);
          lastVal = sliders[1].lastVal;
          if (value > s->max)
	     value = s->max;
          t = value * sliders[1].ysize / s->diff;
          r = lastVal * sliders[1].ysize / s->diff;
          W_WriteArc(1, newstatwin, 60, 100, r + 7, r + 9, 180, 180, W_Black);
          W_WriteArc(1, newstatwin, 60, 100, t + 6, t + 8, 180, 180, W_Red);
          if (me->p_flags & PFENG) { /* Engine Temp */
             box(1, 55, 129, 10, 12, W_Red);
             box(1, 57, 129, 5, 16, W_Red);
          } else {
             box(1, 55, 129, 10, 12, W_White);
             box(1, 57, 129, 5, 16, W_White);
          }
          if (me->p_flags & PFWEP) { /* Weapon Temp */
             box(1, 54, 56, 12, 15, W_Red);
          } else {
             box(1, 54, 56, 12, 15, W_White);
          }
	  image = getImage(I_NEWSTATS_FLAG_ROM);
	  W_DrawImage(newstatwin, 0, 0, -1, image, W_Red);
	  image = getImage(I_NEWSTATS_FLAG_SHIP_ROM);
	  W_DrawImage(newstatwin, 26, 47, -1, image, W_White);
          sliders[4].xloc = 26;
          sliders[4].yloc = 89;
          sliders[4].x1loc = 91;
          sliders[4].y1loc = 89;
          sliders[4].ysize = 22;
          break;
       case 2: /* KLI */
          s = &sliders[1];            /* Hull Damage Stuff */
          value = *(s->var);
          lastVal = sliders[1].lastVal;
          if (value > s->max)
	     value = s->max;
          t = (int)(value * 0.6 * sliders[1].ysize / s->diff);
          r = (int)(lastVal * 0.6 * sliders[1].ysize / s->diff);
          W_WriteArc(1, newstatwin, 53, 105, r+5, r, 70, 200, W_Black);
          W_WriteArc(1, newstatwin, 53, 105, t+5, t, 70, 200, W_Red);
          W_WriteArc(1, newstatwin, 66, 105, r+5, r, 0, 110, W_Black);
          W_WriteArc(1, newstatwin, 66, 105, r+5, r, 270, 90, W_Black);
          W_WriteArc(1, newstatwin, 66, 105, t+5, t, 0, 110, W_Red);
          W_WriteArc(1, newstatwin, 66, 105, t+5, t, 270, 90, W_Red);
          if (me->p_flags & PFENG) { /* Engine Temp */
             box(1, 53, 107, 13, 13, W_Red);
          } else {
             box(1, 53, 107, 13, 13, W_Black);
             box(1, 56, 113, 6, 8, W_White);
          }
          if (me->p_flags & PFWEP) { /* Weapon Temp */
             box(1, 57, 42, 5, 13, W_Red);
          } else {
             box(1, 57, 42, 5, 13, W_White);
          }
	  image = getImage(I_NEWSTATS_FLAG_KLI);
	  W_DrawImage(newstatwin, 0, 0, -1, image, W_Green);
	  image = getImage(I_NEWSTATS_FLAG_SHIP_KLI);
	  W_DrawImage(newstatwin, 22, 40, -1, image, W_White);
          sliders[4].xloc = 25;
          sliders[4].yloc = 110;
          sliders[4].x1loc = 90;
          sliders[4].y1loc = 110;
          break;
       default: /* ORI, IND */
          s = &sliders[1];            /* Hull Damage Stuff */
          value = *(s->var);
          lastVal = sliders[1].lastVal;
          if (value > s->max)
	     value = s->max;
          t = (int)(value * 0.5 * sliders[1].ysize / s->diff);
          r = (int)(lastVal * 0.5 * sliders[1].ysize / s->diff);
          W_WriteArc(1, newstatwin, 60, 107, r + 5, r + 5, 0, 360, W_Black);
          W_WriteArc(1, newstatwin, 60, 107, r + 4, t + 4, 0, 360, W_Red);
          if (me->p_flags & PFENG) { /* Engine Temp */
             box(1, 36, 104, 8, 6, W_Red);
             box(1, 74, 104, 8, 6, W_Red);
          } else {
             box(1, 36, 104, 8, 6, W_White);
             box(1, 74, 104, 8, 6, W_White);
          }
          if (me->p_flags & PFWEP) { /* Weapon Temp */
             box(1, 57, 51, 3, 15, W_Red);
             box(1, 53, 63, 11, 7, W_Red);
          } else {
             box(1, 57, 51, 3, 15, W_White);
             box(1, 53, 63, 11, 7, W_White);
          }
	  image = getImage(I_NEWSTATS_FLAG_ORI);
	  W_DrawImage(newstatwin, 0, 0, -1, image, W_Cyan);
	  image = getImage(I_NEWSTATS_FLAG_SHIP_ORI);
	  W_DrawImage(newstatwin, 27, 47, -1, image, W_White);
          sliders[4].xloc = 29;
          sliders[4].yloc = 98;
          sliders[4].x1loc = 86;
          sliders[4].y1loc = 98;
          break;
    }

    if (me->p_flags & PFWARP) { 
       W_WriteArc(0, newstatwin, rs->w_x1, rs->w_y1, 11, 10, rs->w_a1, rs->w_l1,
           W_Cyan);
       W_WriteArc(0, newstatwin, rs->w_x2, rs->w_y1, 11, 10, rs->w_a2, rs->w_l1,
           W_Cyan);
       W_WriteArc(0, newstatwin, rs->w_x1, rs->w_y2, 11, 10, rs->w_a3, rs->w_l2,
           W_Cyan); 
       W_WriteArc(0, newstatwin, rs->w_x2, rs->w_y2, 11, 10, rs->w_a4, rs->w_l2,
           W_Cyan); 
       W_MakeLine(newstatwin, rs->w_x1 + 7, rs->w_y1, rs->w_x1 + 7, rs->w_y2, 
          W_Cyan);
       W_MakeLine(newstatwin, rs->w_x2 - 6, rs->w_y1, rs->w_x2 - 6, rs->w_y2, 
          W_Cyan);
       W_MakeLine(newstatwin, rs->w_x1 - 6, rs->w_y3, rs->w_x1 - 6, rs->w_y4, 
          W_Cyan);
       W_MakeLine(newstatwin, rs->w_x2 + 7, rs->w_y3, rs->w_x2 + 7, rs->w_y4, 
          W_Cyan);
    } else {
       W_WriteArc(0, newstatwin, rs->w_x1, rs->w_y1, 11, 10, rs->w_a1, rs->w_l1,
          W_Black);
       W_WriteArc(0, newstatwin, rs->w_x2, rs->w_y1, 11, 10, rs->w_a2, rs->w_l1,
          W_Black);
       W_WriteArc(0, newstatwin, rs->w_x1, rs->w_y2, 11, 10, rs->w_a3, rs->w_l2,
          W_Black);
       W_WriteArc(0, newstatwin, rs->w_x2, rs->w_y2, 11, 10, rs->w_a4, rs->w_l2,
          W_Black);
       W_MakeLine(newstatwin, rs->w_x1 + 7, rs->w_y1, rs->w_x1 + 7, rs->w_y2, 
          W_Black);
       W_MakeLine(newstatwin, rs->w_x2 - 6, rs->w_y1, rs->w_x2 - 6, rs->w_y2, 
          W_Black);
       W_MakeLine(newstatwin, rs->w_x1 - 6, rs->w_y3, rs->w_x1 - 6, rs->w_y4, 
          W_Black);
       W_MakeLine(newstatwin, rs->w_x2 + 7, rs->w_y3, rs->w_x2 + 7, rs->w_y4, 
          W_Black);
    }

    for (i = 0; i < NUM_SLIDERS; i++) {
       s = &sliders[i];
       value = *(s->var);
       lastVal = sliders[i].lastVal;
       if ((i == 0) || (i == 2))
	    value = s->max - value;
       if (value < s->min)
	    value = s->min;
       else if (value > s->max)
	    value = s->max;
       new_y = value * sliders[i].ysize / s->diff;
       y = (s->yellow * sliders[i].ysize / s->diff);
       r = (s->red * sliders[i].ysize / s->diff);
       t = (s->max * sliders[i].ysize / s->diff);
       if (i == 0) {
          if (me->p_flags & PFSHIELD) {
             if (value > (s->max - 5)) {
                if (lastVal <= (s->max - 5)) {
	      	   W_WriteArc(0,newstatwin,62, 97, 119, 119, 50, 90, W_Black);
                   W_WriteArc(0,newstatwin,62, 97, 119, 118, 50, 90, W_Black);
                   W_WriteArc(0,newstatwin,62, 97, 119, 119, 220, 85, W_Black); 
                   W_WriteArc(0,newstatwin,62, 97, 119, 118, 220, 85, W_Black); 
                }                
                W_WriteArc(0,newstatwin, 62, 97, 119, 119, 50, 90, W_Grey); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 118, 50, 90, W_Grey); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 119, 220, 85, W_Grey); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 118, 220, 85, W_Grey); 
             } else if (value > s->red) {
                if ((lastVal > (s->max - 5)) || (lastVal <= s->red)) {
	  	   W_WriteArc(0,newstatwin,62, 97, 119, 119, 50, 90, W_Black);
                   W_WriteArc(0,newstatwin,62, 97, 119, 118, 50, 90, W_Black);
                   W_WriteArc(0,newstatwin,62, 97, 119, 119, 220, 85, W_Black); 
                   W_WriteArc(0,newstatwin,62, 97, 119, 118, 220, 85, W_Black); 
                }                
                W_WriteArc(0,newstatwin, 62, 97, 119, 119, 50, 90, rColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 118, 50, 90, rColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 119, 220, 85, rColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 118, 220, 85, rColor); 
	     } else if (value > s->yellow) {
                if ((lastVal > s->red) || (lastVal <= s->yellow)) {
	   	   W_WriteArc(0,newstatwin,62, 97, 119, 119, 50, 90, W_Black);
                   W_WriteArc(0,newstatwin,62, 97, 119, 118, 50, 90, W_Black);
                   W_WriteArc(0,newstatwin,62, 97, 119, 119, 220, 85, W_Black); 
                   W_WriteArc(0,newstatwin,62, 97, 119, 118, 220, 85, W_Black); 
                }                
                W_WriteArc(0,newstatwin, 62, 97, 119, 119, 50, 90, yColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 118, 50, 90, yColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 119, 220, 85, yColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 118, 220, 85, yColor); 
             } else if (value >= 0) {
                if (lastVal > s->yellow) {
	   	   W_WriteArc(0,newstatwin,62, 97, 119, 119, 50, 90, W_Black);
                   W_WriteArc(0,newstatwin,62, 97, 119, 118, 50, 90, W_Black);
                   W_WriteArc(0,newstatwin,62, 97, 119, 119, 220, 85, W_Black); 
                   W_WriteArc(0,newstatwin,62, 97, 119, 118, 220, 85, W_Black); 
                }                
                W_WriteArc(0,newstatwin, 62, 97, 119, 119, 50, 90, gColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 118, 50, 90, gColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 119, 220, 85, gColor); 
                W_WriteArc(0,newstatwin, 62, 97, 119, 118, 220, 85, gColor); 
             }
          } else {
	     W_WriteArc(0,newstatwin, 62, 97, 119, 119, 50, 90, W_Black);
             W_WriteArc(0,newstatwin, 62, 97, 119, 118, 50, 90, W_Black);
             W_WriteArc(0,newstatwin, 62, 97, 119, 119, 220, 85, W_Black); 
             W_WriteArc(0,newstatwin, 62, 97, 119, 118, 220, 85, W_Black); 
          }
          format_value(value, 0, 3);
          tstat[3] = '/';
          format_value(s->max, 4, 3);
	  W_WriteText(newstatwin, 56, 280, W_Cyan, tstat, 7, W_RegularFont); 

       } else if (i == 1) {
          if (value != lastVal) {
             box(1, 67, 16, 25, 9, W_Black);
          }
          if (value > 0) {
	     image = getImage(I_NEWSTATS_FLAG_HULL);
	     W_DrawImage(newstatwin, 67, 16, -1, image, rColor);
          } else {
	     image = getImage(I_NEWSTATS_FLAG_HULL);
	     W_DrawImage(newstatwin, 67, 16, -1, image, W_Grey);
          }
          format_value(value, 0, 3);
          tstat[3] = '/';
          format_value(s->max, 4, 3);
          W_WriteText(newstatwin, 56, 290, W_Cyan, tstat, 7, W_RegularFont);
       } else if (i == 2) {
          if (value != lastVal) {
	     box(1, 31, 16, 26, 8, W_Black);
          }
	  image = getImage(I_NEWSTATS_FLAG_FUEL);
          if (value > s->red) {
	     W_DrawImage(newstatwin, 32, 16, -1, image, rColor);
	  } else if (value > s->yellow) {
	     W_DrawImage(newstatwin, 32, 16, -1, image, yColor);
          } else if (value > 0) {
	     W_DrawImage(newstatwin, 32, 16, -1, image, gColor);
          } else {
	     W_DrawImage(newstatwin, 32, 16, -1, image, W_Grey);
          }
          format_value(value, 0, 5);
          tstat[5] = '/';
          format_value(s->max, 6, 5);
	  W_WriteText(newstatwin, 32, 330, W_White, tstat, 11, W_RegularFont); 
       } else if (i == 3) {
          if (value != lastVal) {
	     box(1, 31, 7, 30, 8, W_Black);
          }
	  image = getImage(I_NEWSTATS_FLAG_WTEMP);
          if (value > s->red) {
	     W_DrawImage(newstatwin, 32, 7, -1, image, rColor);
	  } else if (value > s->yellow) {
	     W_DrawImage(newstatwin, 32, 7, -1, image, yColor);
          } else if (value > 0) {
	     W_DrawImage(newstatwin, 32, 7, -1, image, gColor);
          } else {
	     W_DrawImage(newstatwin, 32, 7, -1, image, W_Grey);
          }
          format_value(value/10, 0, 3);
          tstat[3] = '/';
          format_value(s->max/10, 4, 3);

	  W_WriteText(newstatwin, 56, 320, W_White, tstat, 7, W_RegularFont); 
       } else if (i == 4) {
          if (value != lastVal) {
	     box(1, 62, 7, 27, 8, W_Black);
          }
	  image = getImage(I_NEWSTATS_FLAG_ETEMP);
          if (value > s->red) {
	     W_DrawImage(newstatwin, 62, 7, -1, image, rColor);
	  } else if (value > s->yellow) {
	     W_DrawImage(newstatwin, 62, 7, -1, image, yColor);
          } else if (value > 0) {
	     W_DrawImage(newstatwin, 62, 7, -1, image, gColor);
          } else {
	     W_DrawImage(newstatwin, 62, 7, -1, image, W_Grey);
          }
          format_value(value/10, 0, 3);
          tstat[3] = '/';
          format_value(s->max/10, 4, 3);

	  W_WriteText(newstatwin, 56, 310, W_White, tstat, 7, W_RegularFont); 
          if (me->p_flags & PFAFTER) {
	     box(1, 28, 200, 4, 9, W_Black);
	     image = getImage(I_NEWSTATS_FLAG_SPEED1);
             if (value > s->red) {
	        W_DrawImage(newstatwin, 28, 200, -1, image, rColor);
	     } else if (value > s->yellow) {
	        W_DrawImage(newstatwin, 28, 200, -1, image, yColor);
             } else {
	        W_DrawImage(newstatwin, 28, 200, -1, image, gColor);
             }
          }
       } else if (i == 5) { 
          format_value(value, 0, 3);
          tstat[3] = '/';
          format_value(s->max, 4, 3);
	  W_WriteText(newstatwin, 56, 300, W_Cyan, tstat, 7, W_RegularFont); 
       }
       if ((i == 5) || (i == 2) || (i == 4)) {
          if (value > s->red) {
             box(1, sliders[i].xloc, sliders[i].yloc + (t - new_y),
                sliders[i].xsize, new_y - r, rColor);
             box(1, sliders[i].xloc, sliders[i].yloc + (t - r),
                sliders[i].xsize, y, yColor);
             box(1, sliders[i].xloc, sliders[i].yloc + (t - y),
                sliders[i].xsize, y, gColor);
          } else if (value > s->yellow) {
             box(1, sliders[i].xloc, sliders[i].yloc + (t - new_y),
                sliders[i].xsize, new_y - y, yColor);
             box(1, sliders[i].xloc, sliders[i].yloc + (t - y),
                sliders[i].xsize, y, gColor);
          } else {
             box(1, sliders[i].xloc, sliders[i].yloc + (t - new_y),
                sliders[i].xsize, new_y, gColor);
          }
          box(1, sliders[i].xloc, sliders[i].yloc, sliders[i].xsize,
             (t - new_y), backColor);
       }  
       if (i == 4) {
          if (value > s->red) {
             box(1, sliders[i].x1loc, sliders[i].y1loc + (t - new_y),
                sliders[i].xsize, new_y - r, rColor);
             box(1, sliders[i].x1loc, sliders[i].y1loc + (t - r),
                sliders[i].xsize, y, yColor);
             box(1, sliders[i].x1loc, sliders[i].y1loc + (t - y),
                sliders[i].xsize, y, gColor);
          } else if (value > s->yellow) {
             box(1, sliders[i].x1loc, sliders[i].y1loc + (t - new_y),
                sliders[i].xsize, new_y - y, yColor);
             box(1, sliders[i].x1loc, sliders[i].y1loc + (t - y),
                sliders[i].xsize, y, gColor);
          } else {
             box(1, sliders[i].x1loc, sliders[i].y1loc + (t - new_y),
                sliders[i].xsize, new_y, gColor);
          }
          box(1, sliders[i].x1loc, sliders[i].y1loc, sliders[i].xsize,
             (t - new_y), backColor);
       }  
       sliders[i].lastVal = value;
    }
}

static void 
box(int filled, int x, int y, int wid, int hei, W_Color color)
{
    if (wid == 0)
	return;

    if (filled) {
	/* XFIX */
	W_FillArea(newstatwin, x, y, wid + 1, hei + 1, color);
	return;
    }
    W_MakeLine(newstatwin, x, y, x + wid, y, color);
    W_MakeLine(newstatwin, x + wid, y, x + wid, y + hei, color);
    W_MakeLine(newstatwin, x + wid, y + hei, x, y + hei, color);
    W_MakeLine(newstatwin, x, y + hei, x, y, color);
}


void 
calibrate_Newstats(void)
{
    register int i;
    static struct ship *oldship = NULL;

    /* no need if still in the same ship [BDyess] */
    if(oldship == NULL) oldship = me->p_ship;
    else if(me->p_ship == oldship) return;
    oldship = me->p_ship;

    sliders[0].min = 0;
    sliders[0].max = me->p_ship->s_maxshield;
    sliders[0].yellow = .33 * ((double) sliders[0].max);
    sliders[0].red = .66 * ((double) sliders[0].max);

    sliders[1].min = 0;
    sliders[1].max = me->p_ship->s_maxdamage;
    sliders[1].yellow = .33 * ((double) sliders[1].max);
    sliders[1].red = .66 * ((double) sliders[1].max);

    sliders[2].min = 0;
    sliders[2].max = me->p_ship->s_maxfuel;
    sliders[2].yellow = .33 * ((double) sliders[2].max);
    sliders[2].red = .66 * ((double) sliders[2].max);

    sliders[3].min = 0;
    sliders[3].max = 1.0 * ((double) me->p_ship->s_maxwpntemp);
    sliders[3].yellow = .33 * ((double) sliders[3].max);
    sliders[3].red = .66 * ((double) sliders[3].max);

    sliders[4].min = 0;
    sliders[4].max = 1.0 * ((double) me->p_ship->s_maxegntemp);
    sliders[4].yellow = .33 * ((double) sliders[4].max);
    sliders[4].red = .66 * ((double) sliders[4].max);

    sliders[5].min = 0;
    sliders[5].max = 1.0 * ((double) me->p_ship->s_maxspeed);
    sliders[5].yellow = .33 * ((double) sliders[5].max);
    sliders[5].red = .66 * ((double) sliders[5].max);

    for (i = 0; i < NUM_SLIDERS; i++)
	sliders[i].diff = sliders[i].max - sliders[i].min;
}
