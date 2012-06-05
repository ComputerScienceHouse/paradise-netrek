/* code for hockey lines [BDyess] 9/14/94 */

#include "config.h"
#include <stdio.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

struct player *puck;

/* check to see if on a hockey server and do initialization [BDyess] */
void
hockeyInit(void)
{
  int i;
  struct planet *l, *last;
  int rightmost = 0;
  int leftmost = blk_gwidth;
  int nplan = paradise ? nplanets : 40; /* shouldn't be Paradise... */
  int middle = blk_gwidth / 2;
  int top = 0;
  int bottom = blk_gwidth;
  int line = 0;
  struct point boxpoints[4];
		    /* top-topleft, top-bottomright, bottom-topright,
		       bottom-bottomright, {x,y} for each [BDyess] */
  W_Color topcolor,bottomcolor;

  topcolor = bottomcolor = W_Grey;

  /* we're playing hockey if a player named "Puck" exists in the 'g'
     slot, is independant, is in a scout, and has the login name
     'Robot'. [BDyess] */
  /* ok, I just saw a server with Ij as puck.  Sigh.  Lets instead
     search for a player named "Puck" that is in a scout, independant,
     and has login name "Robot". [BDyess] */
  if(!hockey) {
    for(puck = &players[0]; puck < &players[nplayers]; puck++) {
      if(0 == strcmp(puck->p_name,"Puck") &&
	 0 == strcmp(puck->p_login,"Robot") &&
	 puck->p_teami < 0 &&
	 SCOUT == puck->p_ship->s_type) {

	printf("Hi Puck!\n");
	hockey = 1;
	break;
      }
    }
  }
  if(!hockey) return;
  if(puck->p_ship->s_type != PUCK) {
    puck->p_ship = getship(PUCK);
    if(! puckBitmap) {
      puck->p_ship->s_bitmap = SCOUT;
    }
    redrawPlayer[puck->p_no] = 1;
  }
  if(hockey && tacticalHockeyLines) {
    last = &planets[nplan];
    /* guess where the lines are supposed to go based on planet
       location.  Always draws straight lines, ie. if the border
       bows out, tough - the furthest out planet will have a vertical
       line through it. [BDyess] */

    /* first the left and rightmost lines [BDyess] */
    for(l = &planets[0];l < last;l++) {
      if(l->pl_x < leftmost) {
	leftmost = l->pl_x;
      }
      if(l->pl_x > rightmost) {
	rightmost = l->pl_x;
      }
    }
    hlines[line].vertical = 1;
    hlines[line].pos = leftmost;
    hlines[line].end1 = 0;
    hlines[line].end2 = blk_gwidth;
    hlines[line].color = W_Grey;
    line++;
    hlines[line].vertical = 1;
    hlines[line].pos = rightmost;
    hlines[line].end1 = 0;
    hlines[line].end2 = blk_gwidth;
    hlines[line].color = W_Grey;
    line++;
    /* now guess the middle planet.  Pick the planet closest to the
       middle of the screen. [BDyess] */
    hlines[line].pos = 0;
    for(l = &planets[0];l < last;l++) {
      if(ABS(l->pl_y - middle) < ABS(hlines[line].pos - middle))
	hlines[line].pos = l->pl_y;
    }
    middle = hlines[line].pos;
    hlines[line].end1 = leftmost;
    hlines[line].end2 = rightmost;
    hlines[line].vertical = 0;
    hlines[line].color = W_Red;
    line++;
    /* now find the upper and lower middle lines by picking the planets
       closest to the center yet still above/below the middle and inside
       the left and rightmost [BDyess] */
    for(l = &planets[0];l < last;l++) {
      if(NOBODY == l->pl_owner) continue;
      if(l->pl_y > top && l->pl_y < middle && 
	 l->pl_x < rightmost && l->pl_x > leftmost) {
	top = l->pl_y;
	topcolor = planetColor(l);
      }
      if(l->pl_y < bottom && l->pl_y > middle && 
	 l->pl_x < rightmost && l->pl_x > leftmost) {
	bottom = l->pl_y;
	bottomcolor = planetColor(l);
      }
    }
    hlines[line].pos = top;
    hlines[line].end1 = leftmost;
    hlines[line].end2 = rightmost;
    hlines[line].vertical = 0;
    hlines[line].color = teamColorHockeyLines ? topcolor : W_Cyan;
    line++;
    hlines[line].pos = bottom;
    hlines[line].end1 = leftmost;
    hlines[line].end2 = rightmost;
    hlines[line].vertical = 0;
    hlines[line].color = teamColorHockeyLines ? bottomcolor : W_Cyan;
    line++;
    /* last, try to find the goal box.  Search for the planets that
       are inside the left and right, above/below the upper/lower middle,
       and are not neutral.  Of those planets, take the top left and
       bottom right points [BDyess] */
    /* toplefts */
    boxpoints[0].x = boxpoints[0].y = boxpoints[2].x = boxpoints[2].y =
    		blk_gwidth;
    /* bottomrights */
    boxpoints[1].x = boxpoints[1].y = boxpoints[3].x = boxpoints[3].y = 0;
    for(l = &planets[0];l < last;l++) {
      /* don't want nobody's planets */
      if(l->pl_owner == NOBODY) continue;
      /* check for out-of-bounds */
      if((l->pl_y >= top && l->pl_y <= bottom) ||
	 l->pl_x >= rightmost || l->pl_x <= leftmost) continue; 
      /* top or bottom? */
      if(l->pl_y < middle) i = 0; /* top */
      else i = 2; 		  /* bottom */
      if(l->pl_x <= boxpoints[i].x &&
         l->pl_y <= boxpoints[i].y) {  /* new topleft */
	boxpoints[i].x = l->pl_x;
	boxpoints[i].y = l->pl_y;
      }
      if(l->pl_x >= boxpoints[i+1].x &&
         l->pl_y >= boxpoints[i+1].y) { /* new bottomright */
	boxpoints[i+1].x = l->pl_x;
	boxpoints[i+1].y = l->pl_y;
      }
    }
    if(! teamColorHockeyLines) {
      topcolor = bottomcolor = W_Grey;
    }
    hlines[line].vertical = 0;
    hlines[line].pos = boxpoints[0].y;
    hlines[line].end1 = boxpoints[0].x;
    hlines[line].end2 = boxpoints[1].x;
    hlines[line].color = topcolor;
    line++;
    hlines[line].vertical = 1;
    hlines[line].pos = boxpoints[0].x;
    hlines[line].end1 = boxpoints[0].y;
    hlines[line].end2 = boxpoints[1].y;
    hlines[line].color = topcolor;
    line++;
    hlines[line].vertical = 0;
    hlines[line].pos = boxpoints[1].y;
    hlines[line].end1 = boxpoints[0].x;
    hlines[line].end2 = boxpoints[1].x;
    hlines[line].color = W_Red;
    line++;
    hlines[line].vertical = 1;
    hlines[line].pos = boxpoints[1].x;
    hlines[line].end1 = boxpoints[0].y;
    hlines[line].end2 = boxpoints[1].y;
    hlines[line].color = topcolor;
    line++;

    hlines[line].vertical = 0;
    hlines[line].pos = boxpoints[2].y;
    hlines[line].end1 = boxpoints[2].x;
    hlines[line].end2 = boxpoints[3].x;
    hlines[line].color = W_Red;
    line++;
    hlines[line].vertical = 1;
    hlines[line].pos = boxpoints[2].x;
    hlines[line].end1 = boxpoints[2].y;
    hlines[line].end2 = boxpoints[3].y;
    hlines[line].color = bottomcolor;
    line++;
    hlines[line].vertical = 0;
    hlines[line].pos = boxpoints[3].y;
    hlines[line].end1 = boxpoints[2].x;
    hlines[line].end2 = boxpoints[3].x;
    hlines[line].color = bottomcolor;
    line++;
    hlines[line].vertical = 1;
    hlines[line].pos = boxpoints[3].x;
    hlines[line].end1 = boxpoints[2].y;
    hlines[line].end2 = boxpoints[3].y;
    hlines[line].color = bottomcolor;
    line++;
  }
}

/* draw the tactical hockey lines [BDyess] */
void
tactical_hockey(void)
{
  int i;
  struct hockeyLine *l = &hlines[0];
  int dx,dx1,dx2,dy,dy1,dy2;
  static int old_tacticalHockeyLines, old_galacticHockeyLines,
             old_cleanHockeyGalactic, old_teamColorHockeyLines,
	     old_puckBitmap;

  if(puck->p_ship->s_type != PUCK) {
    puck->p_ship = getship(PUCK);
    redrawPlayer[puck->p_no] = 1;
  }
  if(puckBitmap != old_puckBitmap) {
    if(puckBitmap) {
      puck->p_ship->s_bitmap = PUCK;
    } else {
      puck->p_ship->s_bitmap = SCOUT;
    }
    old_puckBitmap = puckBitmap;
  }
  if(tacticalHockeyLines != old_tacticalHockeyLines) {
    redrawall = 1;
    old_tacticalHockeyLines = tacticalHockeyLines;
  }
  if(galacticHockeyLines != old_galacticHockeyLines) {
    redrawall = 1;
    old_galacticHockeyLines = galacticHockeyLines;
  }
  if(cleanHockeyGalactic != old_cleanHockeyGalactic) {
    redrawall = 1;
    old_cleanHockeyGalactic = cleanHockeyGalactic;
  }
  if(teamColorHockeyLines != old_teamColorHockeyLines) {
    old_teamColorHockeyLines = teamColorHockeyLines;
    hockeyInit();
  }
  /* draw whatever hockey lines are visible [BDyess] */
  if(hockey && tacticalHockeyLines) {		/* if it should be drawn */
    for(i = 0, l = &hlines[0]; i < NUM_HOCKEY_LINES; i++, l++) {
      if(l->vertical) {
	dx = l->pos - me->p_x;
	dy1 = l->end1 - me->p_y;
	dy2 = l->end2 - me->p_y;
	/* is it in view? [BDyess] */
        if(ABS(dx) <= view && 
	   (ABS(dy1) <= view || ABS(dy2) <= view ||
	    (l->end1 <= me->p_y && l->end2 >= me->p_y))) {
	  dx = scaleLocal(dx);
	  dy1 = scaleLocal(dy1);
	  dy2 = scaleLocal(dy2);
	  W_CacheLine(w, dx, dy1, dx, dy2, l->color);
	  clearline[0][clearlcount] = dx;
	  clearline[1][clearlcount] = dy1;
	  clearline[2][clearlcount] = dx;
	  clearline[3][clearlcount] = dy2; 
	  clearlcount++;
	}
      } else {			/* horizontal */
	dy = l->pos - me->p_y;
	dx1 = l->end1 - me->p_x;
	dx2 = l->end2 - me->p_x;
	/* is it in view? [BDyess] */
        if(ABS(dy) <= view && 
	   (ABS(dx1) <= view || ABS(dx2) <= view ||
	    (l->end1 <= me->p_x && l->end2 >= me->p_x))) {
	  dy = scaleLocal(dy);
	  dx1 = scaleLocal(dx1);
	  dx2 = scaleLocal(dx2);
	  W_CacheLine(w, dx1, dy,  dx2, dy, l->color);
	  clearline[0][clearlcount] = dx1;
	  clearline[1][clearlcount] = dy;
	  clearline[2][clearlcount] = dx2;
	  clearline[3][clearlcount] = dy;
	  clearlcount++;
	}
      }
    }
  }
}

/* draw the tactical hockey lines [BDyess] */
void
galactic_hockey(void)
{
  int i;
  struct hockeyLine *l;
  int dx,dx1,dx2,dy,dy1,dy2;

  if(hockey && galacticHockeyLines) {	/* if it should be drawn */
    if(blk_zoom) {
      gwidth = blk_gwidth / 2;
      offsetx = zoom_offset(me->p_x);
      offsety = zoom_offset(me->p_y);
      /* keep last offset? */
    } else {
      gwidth = blk_gwidth;
      offsetx = offsety = 0;
    }
    for(i = 0, l = &hlines[0]; i < NUM_HOCKEY_LINES; i++, l++) {
      if(l->vertical) {
	dx = scaleMapX(l->pos);
	dy1 = scaleMapY(l->end1);
	dy2 = scaleMapY(l->end2);
	W_MakeLine(mapw, dx, dy1, dx, dy2, l->color);
      } else {
	dy = scaleMapY(l->pos);
	dx1 = scaleMapX(l->end1);
	dx2 = scaleMapX(l->end2);
        W_MakeLine(mapw, dx1, dy, dx2, dy, l->color);
      }
    }
  }
}

/* draws a direction triangle on top of puck [BDyess] */
void
drawPuckArrow(void)
{
  int dir = puck->p_dir, x = puck->p_x, y = puck->p_y;
  int x1,x2,x3,y1,y2,y3;	/* triangle verticies */
  int dir1, dir2, dir3;

  /* draw the mini point triangle */

  /* first, find the center of the new triangle. */
  x = scaleLocal(x - me->p_x); /* map galactic to local */
  y = scaleLocal(y - me->p_y);
  x = x + Cos[dir] * 5;
  y = y + Sin[dir] * 5; 

  /* now get the directions of the 3 corners */
  dir1 = dir;
  dir2 = dir + 85;
  if(dir2 > 255) dir2 -= 256;
  dir3 = dir + 170;
  if(dir3 > 255) dir3 -= 256;

  /* now get the three corners */
  x1 = x + Cos[dir1] * puckArrowSize;	 
  y1 = y + Sin[dir1] * puckArrowSize;
  x2 = x + Cos[dir2] * puckArrowSize;	 
  y2 = y + Sin[dir2] * puckArrowSize;
  x3 = x + Cos[dir3] * puckArrowSize;	 
  y3 = y + Sin[dir3] * puckArrowSize;
  W_WriteAnyTriangle(w, x1, y1, x2, y2, x3, y3, W_Red);
}
