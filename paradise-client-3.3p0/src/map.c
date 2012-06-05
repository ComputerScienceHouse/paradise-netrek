/* map.c, all the routines that use the map window [BDyess] */

#include "config.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "images.h"
#include "packets.h"
#include "gppackets.h"

#define DRAWGRID		4

PlanetImageNode *maphead = NULL;

/* data from planets.c [BDyess] */
extern struct point *asteroid_footprints;
extern int next_asteroid_footprint;

int lockx, locky;

/* creates a new PlanetImageNode (including the associated image) given the 
   planet [BDyess] */
PlanetImageNode *
createMapImageNode(struct planet *p, bitstruct bits)
{
  int     offset;
  W_Image *image = NULL;
  char    *s;
  W_Image **imagelist =(W_Image**)malloc(sizeof(W_Image*)*
                                         (strlen(showgalactic)+1));
  int imagelist_index = 0;

  PlanetImageNode *new = (PlanetImageNode*)malloc(sizeof(PlanetImageNode));

  new->left = new->right = NULL;
  new->bits = bits;
 /* printf("bits for %s are: %x\n",p->pl_name,bits);*/

  /*printf("creating new imagenode for planet %s.\n",p->pl_name);*/

  if (paradise) {
      switch (PL_TYPE(*p)) {
      case PLWHOLE:
	  image = getImage(I_MWORMHOLE);
	  break;
      case PLSTAR:
	  image = getImage(I_STARM);
	  break;
      case PLAST:
	  image = getImage(I_MASTEROID);
      }
  }
  if (image) {
      imagelist[imagelist_index++] = image;
  } else if(bits.b_scouted == 0) {  /* not scouted [BDyess] */
      imagelist[imagelist_index++] = getImage(I_MPLAN_NOINFO);
  } else {
    for(s = showgalactic; *s && *s != ','; s++) {
      switch (*s) {
        case 'A':	/* scout info Age [BDyess] */
	case 'a':
	    offset = bits.b_age;
	    image = getImage((*s=='A' ? I_A_M0:I_A_OVERLAY_M0)+offset);
	    break;
	case 'F':	/* facilities [BDyess] */
	case 'f':
	    offset = bits.b_facilities;
	    image = getImage((*s=='F' ? I_F_M0000:I_F_OVERLAY_M0000)+offset);
	    break;
	case 'R':    /* resources [BDyess] */
	case 'r':
	    offset = bits.b_resources;
	    image = getImage((*s=='R' ? I_R_M000 : I_R_OVERLAY_M000)+offset);
	    break;
	case 'S':   /* surface [BDyess] */
	case 's':
	    switch (bits.b_surface << PLATSHIFT) {
	      case PLPOISON:
		image = getImage(*s == 'S' ? I_S_MTOXC : I_S_OVERLAY_MTOXC);
		break;
	      case PLATYPE3:
		image = getImage(*s == 'S' ? I_S_MTNTD : I_S_OVERLAY_MTNTD);
		break;
	      case PLATYPE2:
		image = getImage(*s == 'S' ? I_S_MTHIN : I_S_OVERLAY_MTHIN);
		break;
	      case PLATYPE1:
		image = getImage(*s == 'S' ? I_S_MSTND : I_S_OVERLAY_MSTND);
		break;
	      default:
		printf("unknown surface type %d for %s.\n",
		       p->pl_flags & PLATMASK,p->pl_name);
	    }
	    break;
	case 'T':   /* team [BDyess] */
	case 't':
	    switch (mask_to_idx(bits.b_team)) {
	      case INDi:
	      default:
		image = getImage(*s == 'T' ? I_T_INDMPLANET : 
					     I_T_OVERLAY_INDMPLANET);
		break;
	      case FEDi:
		image = getImage(*s == 'T' ? I_T_FEDMPLANET : 
					     I_T_OVERLAY_FEDMPLANET);
		break;
	      case ROMi:
		image = getImage(*s == 'T' ? I_T_ROMMPLANET : 
					     I_T_OVERLAY_ROMMPLANET);
		break;
	      case KLIi:
		image = getImage(*s == 'T' ? I_T_KLIMPLANET : 
					     I_T_OVERLAY_KLIMPLANET);
		break;
	      case ORIi:
		image = getImage(*s == 'T' ? I_T_ORIMPLANET : 
					     I_T_OVERLAY_ORIMPLANET);
		break;
	    }
	    break;
      }
      imagelist[imagelist_index++] = image;
    }
  }
  imagelist[imagelist_index] = NULL;
  new->image = W_CreateCombinedImage(imagelist,planetColor(p));
  new->image->filename = p->pl_name;
  free(imagelist);
  return new;
}

/* looks in the given tree for a given composite image.  If found, returns
   the image.  If not, creates the image, adds it to the tree, and returns
   the fresh image. [BDyess] */
W_Image *
getPlanetImage(PlanetImageNode *lhead, struct planet *p, bitstruct bits,
          PlanetImageNode * (*createNode) P((struct planet *p, bitstruct bits)))
{
  while(memcmp(&lhead->bits,&bits,sizeof(bits)) != 0) {
    if(memcmp(&bits,&lhead->bits,sizeof(bits))) {		/* go left */
      if(lhead->left) lhead = lhead->left;
      else {
	lhead->left = createNode(p,bits);
	return lhead->left->image;
      }
    } else { 				/* go right */
      if(lhead->right) lhead = lhead->right;
      else {
	lhead->right = createNode(p,bits);
	return lhead->right->image;
      }
    }
  }
  return lhead->image;
}

/* creates a word of bits significant to drawing planets [BDyess] */
/* s is showgalactic/showlocal */
/* l is showGalacticLen/showLocalLen */
bitstruct
createPlanetBits(struct planet *p, char *s, int l)
{
    int F = 0, R = 0, S = 0, T = 0, A = 0;
    bitstruct bits;
    unsigned int tbits, overlay;
    char *r;
    int i;

    memset(&bits,0,sizeof(bits));

    if(!(p->pl_info & idx_to_mask(me->p_teami))) { /* is it scouted? [BDyess] */
      bits.b_scouted = 0;
      return bits;
    } else {
      bits.b_scouted = 1;
    }
    /* bits go on backwards, so they can be used later */
    for(r=s+l-1; r >= s; r--) {
      overlay = 0;		/* bit 4 OFF */
      switch (*r) {
        case 'a':		/* age [BDyess] */
	  overlay = 8;
	  /* FALLTHRU */
        case 'A':
	  if(A) break;
	  A = 1;
          {
#define NSCOUTAGES 5
	    int infoage = 4;
	    if (!(p->pl_info & idx_to_mask(me->p_teami)))
		tbits = NSCOUTAGES - 1;
            else if (p->pl_owner == idx_to_mask(me->p_teami))
		tbits = 0;
            else {
	      for (i = 0; i < NSCOUTAGES - 1 && 
	                  infoage < status2->clock - p->pl_timestamp;
		   i++, infoage = infoage * 5 / 3) {
		/* NULL */
	      }
	      tbits = i;
	    }
	  }
	  bits.b_age = tbits;
	  bits.b_layers <<= 4;
	  bits.b_layers += overlay + B_AGE;
	  break;
        case 'f':
          overlay = 8;		/* bit 4 ON */
	  /* FALLTHRU */
	case 'F':		/* facilities */
	  if(F) break;		/* in case some idiot uses FFFFFFFFF, the */
	  F = 1;                /* bits don't overflow */
	  tbits = 0;
	  tbits |= (p->pl_flags & PLFUEL    ) ?  1 : 0;
	  tbits |= (p->pl_flags & PLREPAIR  ) ?  2 : 0;
	  tbits |= (p->pl_armies > 4        ) ?  4 : 0;
	  tbits |= (paradise && p->pl_flags & PLSHIPYARD) ?  8 : 0;
	  bits.b_facilities = tbits;
	  bits.b_layers <<= 4;
	  bits.b_layers += overlay + B_FACILITIES;
	  break;
	case 'r':
          overlay = 8;		/* bit 4 ON */
	  /* FALLTHRU */
	case 'R':		/* resources */
	  if(R) break;
	  R = 1;
	  tbits = 0;
	  if(paradise) {
	    tbits |= (p->pl_flags & PLDILYTH )  ?  1 : 0;
	    tbits |= (p->pl_flags & PLMETAL  )  ?  2 : 0;
	    tbits |= (p->pl_flags & PLARABLE )  ?  4 : 0;
	  } else {
	    tbits |= (p->pl_flags & PLFUEL   )  ?  1 : 0;
	    tbits |= (p->pl_flags & PLREPAIR )  ?  2 : 0;
	    tbits |= (p->pl_flags & PLAGRI   )  ?  4 : 0;
	  }
	  bits.b_resources = tbits;
	  bits.b_layers <<= 4;
	  bits.b_layers += overlay + B_RESOURCES;
	  break;
	case 's':		/* surface */
          overlay = 8;		/* bit 4 ON */
	  /* FALLTHRU */
	case 'S':
	  if(S) break;
	  S = 1;
	  tbits = 0;
	  tbits |= (p->pl_flags & PLATMASK) >> PLATSHIFT;
	  bits.b_surface = tbits;
	  bits.b_layers <<= 4;
	  bits.b_layers += overlay + B_SURFACE;
	  break;
	case 't':		/* team */
          overlay = 8;		/* bit 4 ON */
	  /* FALLTHRU */
	case 'T':
	  if(T) break;
	  T = 1;
	  tbits = 0;
	  tbits |= p->pl_owner & 0xf;
	  bits.b_team = tbits;
	  bits.b_layers <<= 4;
	  bits.b_layers += overlay + B_TEAM;
	  break;
	default:
          /* shouldn't happen */
	  printf("Unrecognized showgalactic/showlocal specifier: %c\n",*r);
	  break;
      }
    }
    /* now include team.  Team has to be included to get the 
       colors right. [BDyess] */
    bits.b_team = p->pl_owner & 0xf;
    /* lastly, include planet type (planet, star, asteroid, etc.).  Type is
       determined from a combination of three bits (16, 23, & 24).  [BDyess] */
    bits.b_type = (p->pl_flags & (1<<16)) >> 14 |
            (p->pl_flags & (1<<23 | 1<<24)) >> 23;
    return bits;
}

void
drawMapPlanet(struct planet *p, int x, int y)
{
    W_Image *image = NULL;
    int     bigx = 0, bigy = 0;
    char    buf[4];
    int     len;
    bitstruct bits;


    bits = createPlanetBits(p,showgalactic,showGalacticLen);

    if(maphead) image = getPlanetImage(maphead,p,bits,createMapImageNode);
    else {
      maphead = createMapImageNode(p,bits);
      image = maphead->image;
    }
    bigx = image->width;
    bigy = image->height;

#ifdef WORMHOLES_DONT_HAVE_NAMES
    if(PL_TYPE(*p) == PLWHOLE) {
      /* no names for wormholes [BDyess] */
      *buf = 0;
    } else 
#endif
    {
      if (0 == strncmp(p->pl_name, "New ", 4)) {
	  strncpy(buf, p->pl_name + 4, 3);
      } else if (0 == strncmp(p->pl_name, "Planet ", 7)) {
	  strncpy(buf, p->pl_name + 7, 3);
      } else
	  strncpy(buf, p->pl_name, 3);
      buf[3] = 0;
    }
    len = strlen(buf);

    /* moving planets */
    if (pl_update[p->pl_no].plu_update == 1) {
	int odx = scaleMapX(pl_update[p->pl_no].plu_x);
	int ody = scaleMapY(pl_update[p->pl_no].plu_y);

	W_ClearArea(mapw, odx - (bigx / 2), ody - (bigy / 2),
		    bigx, bigy);

	W_WriteText(mapw, odx - (bigx / 2), ody + (bigy / 2),
		    backColor, buf, len, planetFont(p));

	pl_update[p->pl_no].plu_update = 0;
    }
    /* this is delayed so above 'moving planets' code will have a chance
       to work.  [BDyess] */
    if(PL_TYPE(*p) == PLPLANET) 
      W_DrawImageNoClip(mapw, x - image->width / 2,
			y - image->height / 2,
			(udcounter + p->pl_no) / planetChill,
			image,
			planetColor(p));
    else
      W_DrawImageNoClip(mapw, x - image->width / 2,
			y - image->height / 2,
			udcounter + p->pl_no,
			image,
			planetColor(p));
    if (UseLite && emph_planet_seq_n[p->pl_no] > 0 &&
	(F_beeplite_flags & LITE_PLANETS)) {
	int     seq_n;

	image = getImage(I_EMPH_PLANET_SEQ);
	seq_n = emph_planet_seq_n[p->pl_no] % image->frames;
	if ((emph_planet_seq_n[p->pl_no] -= 1) > 0) {
	  W_DrawImage(mapw, x - image->width / 2,
			    y - image->height / 2,
			    seq_n,
			    image,
			    emph_planet_color[p->pl_no]);
	} else
	    W_ClearArea(mapw, x - (image->width / 2),
			y - (image->height / 2),
			image->width, image->height);
	p->pl_flags |= PLREDRAW;	/* Leave redraw on until done
				       highlighting */
	pl_update[p->pl_no].plu_update = 1;
    }
    W_WriteText(mapw, x - (bigx / 2), y + (bigy / 2),
		planetColor(p), buf, len, planetFont(p));
    if (showIND && (p->pl_info & idx_to_mask(me->p_teami)) && 
       (p->pl_owner == NOBODY) && (PL_TYPE(*p) == PLPLANET)) {
	W_MakeLine (mapw, x + (bigx / 2 - 1), y + (bigy / 2 - 1),
		    x - (bigx / 2), y - (bigy / 2), W_White);
	W_MakeLine (mapw, x - (bigx / 2), y + (bigy / 2 - 1),
		    x + (bigx / 2 - 1), y - (bigy / 2), W_White);
    }
    if(me->p_flags & PFPLLOCK && me->p_planet == p->pl_no) {
      if(clearlmcount) {
        /* already a triangle somewhere */
        W_WriteTriangle(mapw, clearlmark[0], clearlmark[1], 4, 0, backColor);
      }
      W_WriteTriangle(mapw, x, y - bigy / 2 - 4, 4, 0, foreColor);
      clearlmark[0] = x;
      clearlmark[1] = y - bigy / 2 - 4;
      clearlmcount = 1;
    }
}

void
map(void)
{
    int     nplan;
    register int i;
    register struct player *j;
    register struct planet *l;
    register int dx, dy;
    static int osx = 0, osy = 0;	/* old square */
    static int last_offsetx, last_offsety;
    static int grid_fuse;	/* TSH */
    int     color, pl = 0;
    W_Image *image;

    /*
       last_lock is used to hold the begin/end point for lock line; [0] holds
       me->; [1] holds target lock; lockx[2] holds status of line so we can
       erase if line is there but lock if off
    */
    static int last_lockx[3] = {0, 0, 0}, last_locky[2];
    int     me_galx, me_galy;

    grid_fuse++;		/* we only draw the grids every DRAWGRID
				   interval */

    /* set number of planets for later */
    nplan = (paradise) ? nplanets : 40;

    if (blk_zoom) {
	gwidth = blk_gwidth / 2;
	offsetx = zoom_offset(me->p_x);
	offsety = zoom_offset(me->p_y);

	if (offsetx != last_offsetx || offsety != last_offsety)
	    redrawall = 1;

	last_offsetx = offsetx;
	last_offsety = offsety;
    } else {
	gwidth = blk_gwidth;
	offsetx = 0;
	offsety = 0;
    }


    if (redrawall) {
        recalcWindowConstants();
	W_ClearWindow(mapw);
    }

    /* draw stepped-on asteroids [BDyess] */
    if(received_terrain_info) {
      int j,k,minj,mink,conv;

      conv = blk_gwidth / 250;
      if(redrawall) {
	for( j = 0; j < 250; j++ ) {
	  for( k = 0; k < 250; k++ ) {
	    if(terrainInfo[j*250+k].types) {
	      W_DrawPoint( mapw, scaleMapX(j*conv + 400),
				 scaleMapY(k*conv + 400), W_White);
	    }
	  }
	}
      } else {
	for(i=0; i<next_asteroid_footprint; i++) {
	  minj = asteroid_footprints[i].x * 5 - 5;
	  mink = asteroid_footprints[i].y * 5 - 5;
	  if(minj > 244) minj = 244;
	  if(mink > 244) mink = 244;
	  if(minj < 0) minj = 0;
	  if(mink < 0) mink = 0;
	  /* draw the 15x15 square of stepped on asteroids [BDyess] */
	  for(j = 0; j < 15; j++) {
	    for(k = 0; k < 15; k++) {
	      if(terrainInfo[(j+minj)*250+k+mink].types) {
		W_DrawPoint( mapw, scaleMapX((j+minj)*conv + 400),
				   scaleMapY((k+mink)*conv + 400), W_White);
	      }
	    }
	  }
	}
      }
      next_asteroid_footprint = 0;
    }

    /* update the entire map fairly frequently if the plotter line is
       on [BDyess] */
    if (plotter && udcounter % 5 == 0) 
        redrawall = 1;

    if (reinitPlanets) {
	initPlanets();
	reinitPlanets = 0;
	/* planets moved so the lines need updating [BDyess] */
	if(hockey) hockeyInit();
    }

    galactic_hockey();

    me_galx = scaleMapX(me->p_x);
    me_galy = scaleMapY(me->p_y);

    /* draw grid on galactic */
    if ((redrawall || (grid_fuse % DRAWGRID) == 0) && (paradise) && (drawgrid)) {
	int     x, y, width, h, grid;
	char    numbuf[1];
	grid = scaleMap(GRIDSIZE);
	for (i = 1; i <= 6 / (blk_zoom ? 2 : 1); i++) {
	    /* looks nasty but we only have to do it 3 times if blk_zoom */

	    /* horizontal line */
	    x = 0;
	    y = i * grid;
	    width = mapside;
	    numbuf[0] = '0' + (char) i;
	    if (blk_zoom) {
		/* we might have to clip */
		dy = i * GRIDSIZE + offsety;
		if (dy >= 0 && dy <= blk_gwidth + 2 * GRIDSIZE) {
		    if (offsetx < 0) {
			x = grid;
			width = 2 * x;
		    } else if (offsetx + 3 * GRIDSIZE > blk_gwidth) {
			width = 2 * grid;
		    }
		    W_MakeTractLine(mapw, x, y, x + width, y, W_Grey);
		}
		if (sectorNums) {
		    numbuf[0] = '0' + (char) (i + offsety / 33333);
		    if ((numbuf[0] == '0') || (numbuf[0] == '7'))
			numbuf[0] = ' ';
		    if (i == 1)	/* so numbers dont overwrite in 1st box */
			W_WriteText(mapw, x + 2, y - grid + 11, W_Grey, numbuf, 1,
				    W_RegularFont);
		    else
			W_WriteText(mapw, x + 2, y - grid + 2, W_Grey, numbuf, 1,
				    W_RegularFont);
		}
	    } else {
		W_MakeTractLine(mapw, x, y, x + width, y, W_Grey);
		if (sectorNums) {
		    W_WriteText(mapw, x + 2, y - grid + 2, W_Grey, numbuf, 1, W_RegularFont);
		}
	    }
	    /* vertical line */
	    x = i * grid;
	    y = 0;
	    h = mapside;

	    if (blk_zoom) {
		/* we might have to clip */
		dx = i * GRIDSIZE + offsetx;
		if (dx >= 0 && dx <= blk_gwidth + 2 * GRIDSIZE) {
		    if (offsety < 0) {
			y = grid;
			h = 2 * y;
		    } else if (offsety + 3 * GRIDSIZE > blk_gwidth) {
			h = 2 * grid;
		    }
		    W_MakeTractLine(mapw, x, y, x, y + h, W_Grey);
		}
		if (sectorNums) {
		    numbuf[0] = '0' + (char) (i + offsetx / 33333);
		    if ((numbuf[0] == '0') || (numbuf[0] == '7'))
			numbuf[0] = ' ';
		    if (i == 1)
			W_WriteText(mapw, x - grid + 11, y + 2, W_Grey, numbuf, 1,
				    W_RegularFont);
		    else
			W_WriteText(mapw, x - grid + 2, y + 2, W_Grey, numbuf, 1,
				    W_RegularFont);
		}
	    } else {
		W_MakeTractLine(mapw, x, y, x, y + h, W_Grey);
		if (sectorNums) {
		    W_WriteText(mapw, x - grid + 2, y + 2, W_Grey, numbuf, 1,
				W_RegularFont);
		}
	    }
	}

	dx = ((me->p_x - offsetx) / GRIDSIZE) * GRIDSIZE;
	dy = ((me->p_y - offsety) / GRIDSIZE) * GRIDSIZE;
	if (!redrawall && ((osx != dx) || (osy != dy))) {

	    /* clear old sector */
	    x = scaleMap(osx);
	    y = scaleMap(osy);
	    width = h = grid;

	    W_DrawSectorHighlight(mapw, x, y, width, h, backColor);

	    osx = dx;
	    osy = dy;
	}
	/* draw our current sector */
	x = scaleMap(dx);
	width = h = grid;
	y = scaleMap(dy);

	W_DrawSectorHighlight(mapw, x, y, width, h, yColor);
    }
    /* Erase ships */
    for (i = 0, j = &players[i]; i < nplayers; i++, j++) {
	lastUpdate[i]++;
	/*
	   Erase the guy if: redrawPlayer[i] is set and the mapmode setting
	   allows it.
	*/
	if (!redrawPlayer[i] || (mapmode == GMAP_INFREQUENT && lastUpdate[i] < 5))
	    continue;
	lastUpdate[i] = 0;
	/* Clear his old image... */
	if (mclearzone[2][i]) {
	    /* XFIX */
	    if (!redrawall || plotter) {
		W_ClearArea(mapw, mclearzone[0][i], mclearzone[1][i],
			    mclearzone[2][i], mclearzone[3][i]);
		/* Redraw the hole just left next update */
		checkRedraw(mclearzone[4][i], mclearzone[5][i]);
	    }
	    mclearzone[2][i] = 0;
	}
    }
    /* Draw Planets */
    if(! (hockey && cleanHockeyGalactic)) 
    {
      for (i = 0, l = &planets[i]; i < nplan; i++, l++) {
	  if (!(l->pl_flags & PLREDRAW) && (!redrawall))
	      continue;
	  l->pl_flags &= ~PLREDRAW;	/* Turn redraw flag off! */

	  dx = scaleMapX(l->pl_x);
	  dy = scaleMapY(l->pl_y);

	  if (PtOutsideWin(dx, dy))
	      continue;

          drawMapPlanet(l,dx,dy);
       }
    }
    if(clearlmcount) {
      W_WriteTriangle(mapw, clearlmark[0], clearlmark[1], 4, 0, 
		      backColor);
      clearlmcount = 0;
    }
    if(me->p_flags & PFPLLOCK) {
      clearlmark[0] = scaleMapX(planets[me->p_planet].pl_x);
      clearlmark[1] = scaleMapY(planets[me->p_planet].pl_y)
                      - 10 /*image height*/ / 2 - 4;
      W_WriteTriangle(mapw, clearlmark[0], clearlmark[1], 4, 0, foreColor);
      clearlmcount = 1;
    }
    /* Draw ships */
    for (i = 0, j = &players[i]; i < nplayers; i++, j++) {
	/*
	   We draw the guy if redrawall, or we just erased him. Also, we
	   redraw if we haven't drawn for 30 frames. (in case he was erased
	   by other ships).
	*/
	if (lastUpdate[i] != 0 && (!redrawall) && lastUpdate[i] < 30)
	    continue;
	if (j->p_status != PALIVE)
	    continue;
	lastUpdate[i] = 0;
	dx = scaleMapX(j->p_x);
	dy = scaleMapY(j->p_y);

	if ((showLock & 1) && !(j->p_flags & PFCLOAK)) {
	    if ((me->p_flags & PFPLOCK) && (me->p_playerl == j->p_no)) {
		if (lockLine) {
		    if ((last_lockx[0] != me_galx) || (last_locky[0] != me_galx) ||
			(last_lockx[1] != dx) || (last_locky[1] != dy)) {
			W_MakeTractLine(mapw, last_lockx[0], last_locky[0],
				   last_lockx[1], last_locky[1], backColor);
			last_lockx[0] = me_galx;
			last_locky[0] = me_galy;
			last_lockx[1] = dx;
			last_locky[1] = dy;
			last_lockx[2] = 1;
			W_MakeTractLine(mapw, last_lockx[0], last_locky[0],
				     last_lockx[1], last_locky[1], W_Green);

		    }
		}
		/*W_WriteTriangle(mapw, dx, dy + 6, 4, 1, foreColor);*/
		pl = 1;
	    }
	}
	/* draws a line on the map indicating your current course [BDyess] */
	if (plotter || clearplotter) {
	    static int old_plotline[4] = {-1};
	    double dist, dist1, dist2;
	    if(old_plotline[0] != -1) {
	      W_MakeLine(mapw, 
	                 old_plotline[0],
	                 old_plotline[1],
	                 old_plotline[2],
	                 old_plotline[3],
			 backColor);
	    }
	    clearplotter = 0;
	    if(plotter) {
	      old_plotline[0] = me_galx;
	      old_plotline[1] = me_galy;
	      if(Cos[me->p_dir] > 0) {
		dist1 = (mapside - me_galx) / Cos[me->p_dir];
	      } else {
		dist1 = me_galx / -Cos[me->p_dir];
	      }
	      if(Sin[me->p_dir] > 0) {
		dist2 = (mapside - me_galy) / Sin[me->p_dir];
	      } else {
		dist2 = me_galy / -Sin[me->p_dir];
	      }
	      dist = dist1 > dist2 ? dist2 : dist1;
	      old_plotline[2] = me_galx + dist * Cos[me->p_dir];
	      old_plotline[3] = me_galy + dist * Sin[me->p_dir];
	      W_MakeLine(mapw, 
			 old_plotline[0],
			 old_plotline[1],
			 old_plotline[2],
			 old_plotline[3],
			 W_Green);
	    }
	}
	if (PtOutsideWin(dx, dy))
	    continue;

	if (blk_friendlycloak == 1) {
	    if (j->p_flags & PFCLOAK) {
		if (myPlayer(j))
		    color = myColor;
		else if (friendlyPlayer(j))
		    color = playerColor(j);
		else
		    color = unColor;
	    } else
		color = playerColor(j);

	    pl = 0;
	    if (j->p_flags & PFCLOAK)
		W_WriteText(mapw, dx - W_Textwidth * cloakcharslen / 2,
		    dy - W_Textheight / 2, color, cloakchars, cloakcharslen,
			    W_RegularFont);
	    else
		W_WriteText(mapw, dx - W_Textwidth,
			    dy - W_Textheight / 2, color, j->p_mapchars, 2,
			    shipFont(j));
	} else {
	    if (j->p_flags & PFCLOAK) {
		W_WriteText(mapw, dx - W_Textwidth * cloakcharslen / 2,
		  dy - W_Textheight / 2, unColor, cloakchars, cloakcharslen,
			    W_RegularFont);
	    } else {
		W_WriteText(mapw, dx - W_Textwidth,
		    dy - W_Textheight / 2, playerColor(j), j->p_mapchars, 2,
			    shipFont(j));
	    }
	}

	if (UseLite && emph_player_seq_n[i] > 0 &&
	    (F_beeplite_flags & LITE_PLAYERS_MAP)) {
	    image = getImage(I_EMPH_PLAYER_SEQ);
	    W_DrawImage(w, dx - image->width / 2,
			   dy - image->height / 2,
			   emph_player_seq_n[i],
			   image,
			   emph_player_color[i]);
	    emph_player_seq_n[i] -= 1;
	    mclearzone[0][i] = dx - (image->width / 2 - 1);
	    mclearzone[1][i] = dy - (image->height / 2 + 1);
	    mclearzone[2][i] = image->width;
	    mclearzone[3][i] = image->height;
	    mclearzone[4][i] = j->p_x;
	    mclearzone[5][i] = j->p_y;
	    /*
	       Force redraw for this guy no matter what. Even if stationary.
	    */
	    lastUpdate[i] = 30;
	    /* Leave redraw on until done highlighting */
	    redrawPlayer[i] = 1;
	} else
	{
	    if (j->p_flags & PFCLOAK) {
		mclearzone[0][i] = dx - W_Textwidth * cloakcharslen / 2;
		mclearzone[1][i] = dy - W_Textheight / 2;
		mclearzone[2][i] = W_Textwidth * cloakcharslen;
	    } else {
		mclearzone[0][i] = dx - W_Textwidth;
		mclearzone[1][i] = dy - W_Textheight / 2;
		mclearzone[2][i] = W_Textwidth * 2;
	    }
	    if (pl)
		mclearzone[3][i] = W_Textheight + 8;
	    else
		mclearzone[3][i] = W_Textheight;
	    /* Set these so we can checkRedraw() next time */
	    mclearzone[4][i] = j->p_x;
	    mclearzone[5][i] = j->p_y;
	    redrawPlayer[i] = 0;
	}
    }
    /* draw viewBox if wanted [BDyess] */
    if (viewBox && allowViewBox) {
	static int viewx = 0, viewy = 0;
        int viewdist = scaleMap(center*scale);

	dx = scaleMapX(me->p_x);
	dy = scaleMapY(me->p_y);
	if (viewx != dx || viewy != dy || redrawall) {
	    /* clear old dots - placed here for less flicker */
	    W_DrawPoint(mapw, viewx + viewdist, viewy + viewdist, backColor);
	    W_DrawPoint(mapw, viewx + viewdist, viewy - viewdist, backColor);
	    W_DrawPoint(mapw, viewx - viewdist, viewy + viewdist, backColor);
	    W_DrawPoint(mapw, viewx - viewdist, viewy - viewdist, backColor);
	    /* redraw any planets they overwrote */
	    viewx = unScaleMapX(viewx);	/* correct from view scale */
	    viewy = unScaleMapY(viewy);
	    checkRedraw(viewx + view, viewy + view);
	    checkRedraw(viewx + view, viewy - view);
	    checkRedraw(viewx - view, viewy + view);
	    checkRedraw(viewx - view, viewy - view);
	    /* draw the new points */
	    W_DrawPoint(mapw, dx + viewdist, dy + viewdist, W_White);
	    W_DrawPoint(mapw, dx + viewdist, dy - viewdist, W_White);
	    W_DrawPoint(mapw, dx - viewdist, dy + viewdist, W_White);
	    W_DrawPoint(mapw, dx - viewdist, dy - viewdist, W_White);
	    viewx = dx;		/* store the points for later */
	    viewy = dy;		/* clearing */
	}
    }
    if (showLock & 1) {
	/* for now draw this everytime */
	if (me->p_flags & PFPLLOCK) {
	    l = &planets[me->p_planet];
	    dx = scaleMapX(l->pl_x);
	    dy = scaleMapY(l->pl_y);

	    if (lockLine) {
		if ((last_lockx[0] != me_galx) || (last_locky[0] != me_galx) ||
		    (last_lockx[1] != dx) || (last_locky[1] != dy)) {
		    W_MakeTractLine(mapw, last_lockx[0], last_locky[0],
				    last_lockx[1], last_locky[1], backColor);
		    last_lockx[0] = me_galx;
		    last_locky[0] = me_galy;
		    last_lockx[1] = dx;
		    last_locky[1] = dy;
		    last_lockx[2] = 1;
		    W_MakeTractLine(mapw, last_lockx[0], last_locky[0],
				    last_lockx[1], last_locky[1], W_Green);
		}
	    }
	}
    }
    /* if lock line is drawn but no lock; erase lock line */
    if (lockLine && ((me->p_flags & (PFPLLOCK | PFPLOCK)) == 0) && last_lockx[2]) {
	W_MakeTractLine(mapw, last_lockx[0], last_locky[0], last_lockx[1],
			last_locky[1], backColor);
	last_lockx[2] = 0;
    }
    /* redraw warp beacon routes */

    for (i = 0; i < ngthingies; i += 2) {
	struct thingy *k = &thingies[i + nplayers * npthingies];
	if (k[0].t_shape == SHP_WARP_BEACON &&
	    k[1].t_shape == SHP_WARP_BEACON) {
	    W_MakeLine(mapw, scaleMapX(k[0].t_x),
		       scaleMapY(k[0].t_y),
		       scaleMapX(k[1].t_x),
		       scaleMapY(k[1].t_y),
		       W_Grey);
	}
    }
  redrawall = 0;
}
