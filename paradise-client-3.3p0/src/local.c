/* local.c, all code that writes to the local window (w) [BDyess] */

#include "copyright.h"

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
#include "packets.h"
#include "gameconf.h"
#include "images.h"
#ifdef UNIX_SOUND
#include "sound.h"
#endif

PlanetImageNode *localhead = NULL;

bitstruct createPlanetBits P((struct planet *p, char *showlocal, 
                                 int showLocalLen));

/* data from planets.c [BDyess] */
extern struct point *asteroid_footprints;

/* creates a new PlanetImageNode (including the associated image) given the 
   planet [BDyess] */
PlanetImageNode *
createLocalImageNode(struct planet *p, bitstruct bits)
{
  int     offset;
  W_Image *image = NULL;
  char    *s;
  W_Image **imagelist =(W_Image**)malloc(sizeof(W_Image*)*
                                         (showLocalLen)+2);
  int imagelist_index = 0;

  PlanetImageNode *new = (PlanetImageNode*)malloc(sizeof(PlanetImageNode));

  new->left = new->right = NULL;
  new->bits = bits;

  /*printf("creating new imagenode for local planet %s.\n",p->pl_name);*/

  if (paradise) {
      switch (PL_TYPE(*p)) {
      case PLWHOLE:
	  image = getImage(I_WORMHOLE);
	  break;
      case PLSTAR:
	  image = getImage(I_STAR);
	  break;
      case PLAST:
	  image = getImage(I_ASTEROIDS_ALL);
      }
  }
  if (image) {
      imagelist[imagelist_index++] = image;
  } else if (bits.b_scouted == 0) {  /* not scouted [BDyess] */
      imagelist[imagelist_index++] = getImage(I_PLAN_NOINFO);
  } else {
    for(s = showlocal; *s && *s != ','; s++) {
      switch (*s) {
        case 'A':	/* scout age [BDyess] */
	case 'a':
	    offset = bits.b_age;
	    image = getImage((*s == 'A' ? I_A_0 : I_A_OVERLAY_0) +offset);
	    break;
	case 'F':	/* facilities [BDyess] */
	case 'f':
	    offset = bits.b_facilities;
	    image = getImage((*s == 'F' ? I_F_0000 : I_F_OVERLAY_0000) +offset);
	    break;
	case 'R':    /* resources [BDyess] */
	case 'r':
	    offset = bits.b_resources;
	    image = getImage((*s == 'R' ? I_R_000 : I_R_OVERLAY_000) + offset);
	    break;
	case 'S':   /* surface [BDyess] */
	case 's':
	    switch (bits.b_surface << PLATSHIFT) {
	      case PLPOISON:
		image = getImage(*s == 'S' ? I_S_TOXC : I_S_OVERLAY_TOXC);
		break;
	      case PLATYPE3:
		image = getImage(*s == 'S' ? I_S_TNTD : I_S_OVERLAY_TNTD);
		break;
	      case PLATYPE2:
		image = getImage(*s == 'S' ? I_S_THIN : I_S_OVERLAY_THIN);
		break;
	      default:
	        printf("unknown planet type %d, faking it.\n",
							p->pl_flags & PLATMASK);
		/* FALLTHRU */
	      case PLATYPE1:
		image = getImage(*s == 'S' ? I_S_STND : I_S_OVERLAY_STND);
		break;
	    }
	    break;
	case 'T':   /* team [BDyess] */
	case 't':
	    switch (mask_to_idx(bits.b_team)) {
	      case INDi:
	      default:
		image = getImage(*s == 'T' ? I_T_INDPLANET : 
					     I_T_OVERLAY_INDPLANET);
		break;
	      case FEDi:
		image = getImage(*s == 'T' ? I_T_FEDPLANET : 
					     I_T_OVERLAY_FEDPLANET);
		break;
	      case ROMi:
		image = getImage(*s == 'T' ? I_T_ROMPLANET : 
					     I_T_OVERLAY_ROMPLANET);
		break;
	      case KLIi:
		image = getImage(*s == 'T' ? I_T_KLIPLANET : 
					     I_T_OVERLAY_KLIPLANET);
		break;
	      case ORIi:
		image = getImage(*s == 'T' ? I_T_ORIPLANET : 
					     I_T_OVERLAY_ORIPLANET);
		break;
	    }
	    break;
      }
      imagelist[imagelist_index++] = image;
    }
  }
  imagelist[imagelist_index] = NULL;
  new->image = W_CreateCombinedImage(imagelist,planetColor(p));
#ifdef DEBUG
  {
    W_Image **x;

    printf("new local image node created for %s with bits %x using these images:\n",
           p->pl_name,
           new->bits);
    for(x = imagelist; *x; x++) {
      printf("  %s\n",(*x)->filename);
    }
  }
#endif /*DEBUG*/
  new->image->filename = p->pl_name;
  free(imagelist);
  return new;
}

static void
drawPlanet(struct planet *p, int x, int y)
{
    W_Image *image = NULL;
    int     bigx = 0, bigy = 0;
    struct _clearzone *cz;
    bitstruct bits;

    bits = createPlanetBits(p,showlocal,showLocalLen);

    if(localhead) image = getPlanetImage(localhead,p,bits,createLocalImageNode);
    else {
      localhead = createLocalImageNode(p,bits);
      image = localhead->image;
    }
    bigx = image->width;
    bigy = image->height;
    if(PL_TYPE(*p) == PLPLANET) 
      W_DrawImageNoClip(w, x - bigx / 2,
		     y - bigy / 2,
		     (udcounter + p->pl_no) / planetChill,
		     image,
		     planetColor(p));
    else
      W_DrawImageNoClip(w, x - bigx / 2,
		     y - bigy / 2,
		     udcounter + p->pl_no,
		     image,
		     planetColor(p));

    if (showIND && (p->pl_info & idx_to_mask(me->p_teami)) && 
       (p->pl_owner == NOBODY) && (PL_TYPE(*p) == PLPLANET)) {
	W_CacheLine (w, x - (bigx / 2), y - (bigy / 2),
		     x + (bigx / 2 - 1), y + (bigy / 2 - 1),
		     W_White);
	W_CacheLine (w, x + (bigx / 2 - 1), y - (bigy / 2),
		     x - (bigx / 2), y + (bigy / 2 - 1),
		     W_White);
    }
    if (namemode) {
     if (PL_TYPE(*p) != PLWHOLE) {
	W_MaskText(w, x - p->pl_namelen * W_Textwidth / 2, y + 
	          (bigy / 2),
		   planetColor(p),
		   p->pl_name, p->pl_namelen, planetFont(p));
	cz = new_czone();
	cz->x = x - p->pl_namelen * W_Textwidth / 2;
	cz->y = y + (bigy / 2);
	cz->width = W_Textwidth * p->pl_namelen;
	cz->height = W_Textheight;
      }
    }
    
    if (paradise && show_armies_on_local && allowShowArmiesOnLocal)
      {
      if (PL_TYPE(*p) == PLPLANET)
        {
          if (p->pl_info & idx_to_mask(me->p_teami))
            {
              char dspstr[8];
              sprintf(dspstr, "%d", p->pl_armies);
              cz = new_czone();
              cz->x = x + bigx/2;
              cz->y = y - bigy/2;
              cz->width = W_Textwidth * strlen(dspstr);
              cz->height = W_Textheight;
              W_MaskText(w, cz->x, cz->y,
                         planetColor(p), dspstr, strlen(dspstr),
                         planetFont(p));
            }
        }
      }

    /*--------draw tactical lock*/
    if ((showLock & 2) && (me->p_flags & PFPLLOCK) && 
        (me->p_planet == p->pl_no)) {
	W_WriteTriangle(w, x, y - (bigy) / 2 - 6, 5, 0, 
			foreColor);
	cz=new_czone();
	cz->x = x - 5;
	cz->y = y - (bigy) / 2 - 12;
	cz->width = 11;
	cz->height = 7;
    }
    cz = new_czone();
    cz->x = x - (bigx / 2);
    cz->y = y - (bigy / 2);
    cz->width = bigx;
    cz->height = bigy;
}

/* call this from local for each player, instead of having an extra loop! */
static void
redraw_photon_torps(struct player *j)
{
    int     i, h;
    struct torp *k;
    int     dx, dy;
    struct _clearzone *cz;
    W_Image *image;

    i = j->p_no;
    if (!j->p_ntorp)
	return;
    for (h = 0, k = &torps[ntorps * i + h]; h < ntorps; h++, k++) {
	if (!k->t_status)
	    continue;
	dx = k->t_x - me->p_x;
	dy = k->t_y - me->p_y;
	if (ABS(dx) > view || ABS(dy) > view) {
	    /* Call any torps off screen "free" (if owned by other) */
	    if (k->t_status == TEXPLODE && j != me) {
		k->t_status = TFREE;
		j->p_ntorp--;
	    }
	    continue;
	}
	dx = scaleLocal(dx);
	dy = scaleLocal(dy);
#ifdef UNIX_SOUND
        if ((k->t_status == TEXPLODE) && (k->t_fuse == 5)) play_sound (SND_TORPHIT);
#endif
	if (k->t_status == TEXPLODE) {
	    image = getImage(friendlyTorp(k) ? I_MTORPCLOUD : I_ETORPCLOUD);

	    k->t_fuse--;
	    if (k->t_fuse <= 0) {
		k->t_status = TFREE;
		j->p_ntorp--;
		continue;
	    }
	    if (k->t_fuse > image->frames) {
		k->t_fuse = image->frames;
	    }
	    W_DrawImage(w, dx - image->width / 2,
	                   dy - image->height / 2,
			   image->frames - k->t_fuse,
			   image,
			   torpColor(k));
	    cz = new_czone();
	    cz->x = dx - (image->width / 2);
	    cz->y = dy - (image->height / 2);
	    cz->width = image->width;
	    cz->height = image->height;
        } else if (!friendlyTorp(k)) {
	    image = getImage(I_ETORP);
	    W_DrawImage(w, dx - image->width / 2,
			   dy - image->height/ 2,
			   udcounter + k->t_no,
			   image,
			   torpColor(k));

	    cz = new_czone();
	    cz->x = dx - (image->width / 2);
	    cz->y = dy - (image->height / 2);
	    cz->width = image->width;
	    cz->height = image->height;
	} else {
	    image = getImage(I_MTORP);
	    W_DrawImage(w, dx - image->width / 2,
			   dy - image->height / 2,
			   udcounter + k->t_no,
			   image,
			   torpColor(k));

	    cz = new_czone();
	    cz->x = dx - (image->width / 2);
	    cz->y = dy - (image->height / 2);
	    cz->width = image->width;
	    cz->height = image->height;
	}
    }
}

static void
draw_one_thingy(struct thingy *k)
{
    int     dx, dy;
    struct _clearzone *cz;
    W_Image *image;
    int     frame = 0;

    if (k->t_shape == SHP_BLANK)
	return;
    /* printf("%d,%d - %d,%d\n", me->p_x, me->p_y, k->t_x, k->t_y); */
    dx = k->t_x - me->p_x;
    dy = k->t_y - me->p_y;
    if (ABS(dx) > view || ABS(dy) > view) {
	return;
    }
    dx = scaleLocal(dx);
    dy = scaleLocal(dy);
    switch (k->t_shape) {
    case SHP_BOOM:
	image = getImage(friendlyThingy(k) ? I_MTORPCLOUD : I_ETORPCLOUD);
	k->t_fuse--;
	if (k->t_fuse <= 0) {
	    k->t_shape = SHP_BLANK;
	    return;
	}
	if (k->t_fuse > image->frames) {
	    k->t_fuse = image->frames;
	}
	frame = image->frames - k->t_fuse;
	break;
    case SHP_MISSILE:
	image = getImage(friendlyThingy(k) ? I_MDRONE : I_EDRONE);
	frame = (int) (k->t_dir * image->frames + 128) / 256;
	break;
    case SHP_TORP:
        image = getImage(friendlyThingy(k) ? I_MTORP : I_ETORP);
	frame = udcounter + k->t_no;
	break;
    case SHP_PLASMA:
    case SHP_MINE:		/* use plasma until I get a nifty bitmap */
        image = getImage(friendlyThingy(k) ? I_MPLASMATORP : I_EPLASMATORP);
	frame = udcounter + k->t_no;
	break;
    case SHP_PBOOM:
	image = getImage(friendlyThingy(k) ? I_MPLASMACLOUD : I_EPLASMACLOUD);
	k->t_fuse--;
	if (k->t_fuse < 0) {
	    k->t_shape = SHP_BLANK;
	    return;
	}
	if (k->t_fuse > image->frames) {
	    k->t_fuse = image->frames;
	}
	frame = image->frames - k->t_fuse;
	break;
    case SHP_FBOOM:
	image = getImage(friendlyThingy(k) ? I_MFIGHTERCLOUD : I_EFIGHTERCLOUD);
	k->t_fuse--;
	if (k->t_fuse < 0) {
	    k->t_shape = SHP_BLANK;
	    return;
	}
	if (k->t_fuse > image->frames) {
	    k->t_fuse = image->frames;
	}
	frame = image->frames - k->t_fuse;
	break;
    case SHP_DBOOM:
	image = getImage(friendlyThingy(k) ? I_MDRONECLOUD : I_EDRONECLOUD);
	k->t_fuse--;
	if (k->t_fuse < 0) {
	    k->t_shape = SHP_BLANK;
	    return;
	}
	if (k->t_fuse > image->frames) {
	    k->t_fuse = image->frames;
	}
	frame = image->frames - k->t_fuse;
	break;
    case SHP_FIGHTER:
	image = getImage(friendlyThingy(k) ? I_MFIGHTER : I_EFIGHTER);
	frame = (int) (k->t_dir * image->frames + 128) / 256;
	break;
    case SHP_WARP_BEACON:
	image = getImage(I_WARPBEACON);
	frame = udcounter;
	if (k->t_fuse > 4) {
	    image = getImage(I_WARPFLASH);
	}
	if (++(k->t_fuse) > 6) {
	    k->t_fuse = 0;
	}
	break;
    default:
        fprintf(stderr,"Wierd...unknown thingy number (%d).\n", k->t_shape);
	return;
    }
    cz = new_czone();
    cz->x = dx - (image->width / 2);
    cz->y = dy - (image->height / 2);
    cz->width = image->width;
    cz->height = image->height;
    W_DrawImage(w, cz->x, cz->y, frame, image, torpColor(k));
}

static void
redraw_drones(struct player *j)
{
    int     i, h;
    int     count;

    i = j->p_no;

    if (!j->p_ndrone)
	return;
    count = 0;

    for (h = i * npthingies; h < npthingies * (i + 1); h++) {
	draw_one_thingy(&thingies[h]);
	if (thingies[h].t_shape != SHP_BLANK)
	    count++;
    }
    j->p_ndrone = count;
}

static void
redraw_other_drones(void)
{
    int     h;

    for (h = 0; h < ngthingies; h++)
	draw_one_thingy(&thingies[nplayers * npthingies + h]);
}

static void
redraw_plasma_torps(struct player *j)
{
    int     h, i;
    register struct plasmatorp *pt;
    struct _clearzone *cz;
    int frame;

    int     dx, dy;
    W_Image *image;

    i = j->p_no;

    if (!j->p_nplasmatorp)
	return;
    for (h = 0, pt = &plasmatorps[nplasmas * i + h]; h < nplasmas; h++, pt++) {
	if (!pt->pt_status)
	    continue;
	dx = pt->pt_x - me->p_x;
	dy = pt->pt_y - me->p_y;
	if (ABS(dx) > view || ABS(dy) > view)
	    continue;
	dx = scaleLocal(dx);
	dy = scaleLocal(dy);
	frame = udcounter + pt->pt_no;
#ifdef UNIX_SOUND
        if ((pt->pt_status == TEXPLODE) && (pt->pt_fuse == 5))
                play_sound (SND_TORPHIT); /* Torp Hit used for Plasma Hit */
#endif
	if (pt->pt_status == PTEXPLODE) {
	    image = getImage(friendlyPlasmaTorp(pt) ? I_MPLASMACLOUD :
	                                              I_EPLASMACLOUD);
	    pt->pt_fuse--;
	    if (pt->pt_fuse <= 0) {
		pt->pt_status = PTFREE;
		j->p_nplasmatorp--;
		continue;
	    }
	    if (pt->pt_fuse > image->frames) {
		pt->pt_fuse = image->frames;
	    }
	    frame = image->frames - pt->pt_fuse;
	}
	/* needmore: if(pt->pt_war & idx_to_mask(me->p_teami)) */
	else if (pt->pt_owner != me->p_no && ((pt->pt_war & idx_to_mask(me->p_teami)) ||
					      (idx_to_mask(players[pt->pt_owner].p_teami) & (me->p_hostile | me->p_swar)))) {
            image = getImage(I_EPLASMATORP);
	} else {
            image = getImage(I_MPLASMATORP);
	}
	W_DrawImage(w, dx - image->width / 2,
		       dy - image->height / 2,
		       frame,
		       image,
		       plasmatorpColor(pt));
	cz = new_czone();
	cz->x = dx - (image->width / 2);
	cz->y = dy - (image->height / 2);
	cz->width = image->width;
	cz->height = image->height;
    }
}

void tactical_hockey P((void));

void
redraw_asteroids(void)
{
  int x, y, minx, miny, maxx, maxy;
  struct _clearzone *cz;
  int lx, ly, frame;
  W_Image *topleft, *topright, *bottomleft, *bottomright, *image;
  static W_Image *topleftsquare = NULL, *toprightsquare, *bottomleftsquare,
      *bottomrightsquare, *topleftrounded, *toprightrounded,
      *bottomleftrounded, *bottomrightrounded;

  if (!paradise)
    return;

  if(received_terrain_info) {
    int conv = blk_gwidth / 250;

    if(rounded_asteroids) {
      if(topleftsquare == NULL) {
	topleftsquare = getImage(I_ASTEROIDS_TOPLEFTSQUARE),
	toprightsquare = getImage(I_ASTEROIDS_TOPRIGHTSQUARE),
	bottomleftsquare = getImage(I_ASTEROIDS_BOTTOMLEFTSQUARE),
	bottomrightsquare = getImage(I_ASTEROIDS_BOTTOMRIGHTSQUARE),
	topleftrounded = getImage(I_ASTEROIDS_TOPLEFTROUNDED),
	toprightrounded = getImage(I_ASTEROIDS_TOPRIGHTROUNDED),
	bottomleftrounded = getImage(I_ASTEROIDS_BOTTOMLEFTROUNDED),
	bottomrightrounded = getImage(I_ASTEROIDS_BOTTOMRIGHTROUNDED);
      }

      /* draw asteroids [BDyess] */
      minx = (me->p_x - view) / conv;
      miny = (me->p_y - view) / conv;
      maxx = (me->p_x + view) / conv + 1;
      maxy = (me->p_y + view) / conv + 1;
      /* only check the asteroids visible [BDyess] */
      for(x = minx;x <= maxx; x++) {
	for(y = miny;y <= maxy; y++) {
	  if(terrainInfo[x*250+y].types) {
	    /* draw rounded corners if there are no adjacent squares.
	       Otherwise draw square corners [BDyess] */
	    /* topleft */
	    if (terrainInfo[(x-1)*250+y].types ||
		terrainInfo[x*250+y-1].types) {
	      topleft = topleftsquare;
	    } else {
	      topleft = topleftrounded;
	    }
	    /* topright */
	    if (terrainInfo[(x+1)*250+y].types ||
		terrainInfo[x*250+y-1].types) {
	      topright = toprightsquare;
	    } else {
	      topright = toprightrounded;
	    }
	    /* bottomleft */
	    if (terrainInfo[(x-1)*250+y].types ||
		terrainInfo[x*250+y+1].types) {
	      bottomleft = bottomleftsquare;
	    } else {
	      bottomleft = bottomleftrounded;
	    }
	    /* bottomright */
	    if (terrainInfo[(x+1)*250+y].types ||
		terrainInfo[x*250+y+1].types) {
	      bottomright = bottomrightsquare;
	    } else {
	      bottomright = bottomrightrounded;
	    }
	    lx = scaleLocal(x*conv - me->p_x);
	    ly = scaleLocal(y*conv - me->p_y);
	    frame = udcounter + y*conv + x;
	    W_DrawImageNoClip(w, lx,                ly,                 frame, 
			      topleft, W_Grey);
	    W_DrawImageNoClip(w, lx+topleft->width, ly,                 frame, 
			      topright,W_Grey);
	    W_DrawImageNoClip(w, lx,                ly+topleft->height, frame,
			      bottomleft,W_Grey);
	    W_DrawImageNoClip(w, lx+topleft->width, ly+topleft->height, frame, 
			      bottomright, W_Grey);
	    cz = new_czone();
	    cz->x = lx;
	    cz->y = ly;
	    cz->width = topleft->width + topright->width;
	    cz->height = topleft->height + bottomleft->height;
	  }
	}
      }
    } else {
      /* draw asteroids [BDyess] */
      minx = (me->p_x - view) / conv;
      miny = (me->p_y - view) / conv;
      maxx = (me->p_x + view) / conv + 1;
      maxy = (me->p_y + view) / conv + 1;
      image = getImage(I_ASTEROIDS_ALL);
      /* only check the asteroids visible [BDyess] */
      for(x = minx;x <= maxx; x++) {
	for(y = miny;y <= maxy; y++) {
	  if(terrainInfo[x*250+y].types) {
	    lx = scaleLocal(x*conv - me->p_x + 400) - image->width/2;
	    ly = scaleLocal(y*conv - me->p_y + 400) - image->height/2;
	    frame = udcounter + y*conv + x;
	    W_DrawImageNoClip(w, lx, ly, frame, image, W_Grey);
	    cz = new_czone();
	    cz->x = lx;
	    cz->y = ly;
	    cz->width = image->width;
	    cz->height = image->height;
	  }
	}
      }
    }
  }
}


void
redraw_all_planets(void)
{
    int     i;
    int     dx, dy;
    struct planet *l;
    int     nplan;

    nplan = paradise ? nplanets : 40;

    if(hockey) {
      tactical_hockey();
    }
    for (i = 0, l = &planets[i]; i < nplan; i++, l++) {
	dx = l->pl_x - me->p_x;
	dy = l->pl_y - me->p_y;
	if (ABS(dx) > view || ABS(dy) > view)
	    continue;
	dx = scaleLocal(dx);
	dy = scaleLocal(dy);
	drawPlanet(l,dx,dy);
    }
}

void 
doShowMySpeed(int dx, int dy, W_Image *ship_bits, struct player *j)
{
    struct _clearzone *cz;
    char    idbuf[5];
    int     len;
    int     color;

    sprintf(idbuf, "%c,%d", *(shipnos + j->p_no), me->p_speed);
    len = strlen(idbuf);
    /*
       color the playernum,speed based on warp/afterburn/warpprep state
       [BDyess]

       Changed to team color because of xpm mode. [BDyess]
    */
    color = shipCol[1 + me->p_teami];
    dx += ship_bits->width / 2;
    dy -= ship_bits->height / 2;
    /* underline number,speed if warp is suspended [BDyess] */
    W_MaskText(w, dx, dy, color, idbuf, len, (me->p_flags & PFWPSUSPENDED) ?
	       W_UnderlineFont : shipFont(j));

    cz = new_czone();
    cz->x = dx;
    cz->y = dy;
    cz->width = len * W_Textwidth;
    cz->height = W_Textheight;
}

/* show just about anything next to ship on local display
   DSFAPWE - for each letter in statString, show a line for:
   Damage, Shields, Fuel, Armies, sPeed, Wtemp, Etemp 

   lower case = reverse length
 */
static void
doLocalShipstats(void)
{
    char *sptr;
    int len, x=localStatsX;
    struct _clearzone *cz;
    W_Color color;

#define BARLENGTH(part,total) \
  len = me->p_ship->total > 0 ? \
        (statHeight*me->part) / me->p_ship->total : \
	0; \
	break;

    for(sptr=statString;*sptr;sptr++) {
	switch(toupper(*sptr)) {
	case 'W':
	    BARLENGTH(p_wtemp,s_maxwpntemp)
	case 'E':
	    BARLENGTH(p_etemp,s_maxegntemp)
	case 'P':
	    BARLENGTH(p_speed,s_maxspeed)
	case 'D':
	    BARLENGTH(p_damage,s_maxdamage)
	case 'S':
	    BARLENGTH(p_shield,s_maxshield)
	case 'F':
	    BARLENGTH(p_fuel,s_maxfuel)
	case 'A':
	    BARLENGTH(p_armies,s_maxarmies)
	default:
	    continue;
	}
	if(islower(*sptr))
	    len=statHeight - len;
	if(len>statHeight)
	    len=statHeight;

	if(len>2*(statHeight/3) || toupper(*sptr) == 'A')
	    color=W_Green;
	else if(len<(statHeight/3))
	    color=W_Red;
	else
	    color=W_Yellow;
	if(len>0)
	    W_CacheLine(w,x+W_Textwidth/2,localStatsY-len,x+W_Textwidth/2,localStatsY,color);
	W_MaskText(w,x,localStatsY+1,W_White,sptr,1,W_RegularFont);
	x+=W_Textwidth;
    }
    if(x>localStatsX) {
	W_CacheLine(w,localStatsX,localStatsY-statHeight,x,localStatsY-statHeight,W_White);
	W_CacheLine(w,localStatsX,localStatsY,x,localStatsY,W_White);
	cz=new_czone();
	cz->x=localStatsX;
	cz->y=localStatsY-statHeight+1;
	cz->width=x-localStatsX;
	cz->height=statHeight-2;
    }
}

void
local(void)
{
    int i;
    struct player *j;
    struct phaser *php;
    struct _clearzone *cz;
    W_Image *image, *cloakimage;

    int     dx, dy;
    char    idbuf[10];

    if (showKitchenSink) {
	image = getImage(I_KITCHEN_SINK);
	cz = new_czone();
	cz->width = image->width;
	cz->height = image->height;
	cz->x = 500 - cz->width;
	cz->y = 0;
	W_DrawImage(w, cz->x, 
	               cz->y, 
	      	       udcounter,
		       image,
		       W_Grey);
    }
    /*
       Kludge to try to fix missing ID chars on tactical (short range)
       display.
    */
    idbuf[0] = '0';
    idbuf[1] = '\0';

    redraw_asteroids();

    /* Draw Planets */
    redraw_all_planets();

    /* Draw ships */
    /* first pass, display those items that should always be on the bottom
       (right now, only explosions) [BDyess] */
    for (i = 0, j = &players[i]; i < nplayers; i++, j++) {
	int     k;

	if(j->p_status != PEXPLODE) continue;
	dx = j->p_x - me->p_x;
	dy = j->p_y - me->p_y;
	if (ABS(dx) > view || ABS(dy) > view) {
	    j->p_explode++;
	    continue;
	}
	dx = scaleLocal(dx);
	dy = scaleLocal(dy);
	k = j->p_explode;
#ifdef UNIX_SOUND
        if (k==0)
                {
                    if (j->p_ship->s_type == STARBASE)
                    play_sound(SND_EXP_SB);         /* Starbase Explode */
                    else play_sound(SND_EXPLOSION); /* Ship Explosion */
                }
#endif

	image = NULL;
	switch(j->p_ship->s_type) {
	  case STARBASE:
	  case WARBASE:
	  case JUMPSHIP:
	  case ATT:
	    image = getImage(I_SBEXPLOSION);
	    break;
	  default:
	    image = getImage(I_EXPLOSION);
	    break;
	}
	if(k < image->frames) {
	  W_DrawImage(w, dx - image->width / 2,
			 dy - image->height / 2,
			 k,
			 image,
			 playerColor(j));
	  cz = new_czone();
	  cz->x = dx - (image->width / 2);
	  cz->y = dy - (image->height / 2);
	  cz->width = image->width;
	  cz->height = image->height;
	  j->p_explode++;
	}
    }
    cloakimage = getImage(I_CLOAK);
    for (i = 0, j = &players[i]; i < nplayers; i++, j++) {
	int     tx, ty;
	if (me->p_x < 0)
	    continue;

	/* more efficient than using a separate loop for this */
	redraw_photon_torps(j);
	redraw_drones(j);
	redraw_plasma_torps(j);

	if (j->p_status != PALIVE)
	    continue;

#ifdef UNIX_SOUND
        if (myPlayer(j) && (j->p_flags & PFCLOAK) &&
           (j->p_cloakphase == 0)) play_sound (SND_CLOAK);
        if (myPlayer(j) && (!(j->p_flags & PFCLOAK)) &&
           (j->p_cloakphase == 6)) play_sound (SND_CLOAK);
#endif

	if (j->p_flags & PFCLOAK) {
	    if (j->p_cloakphase < (cloakimage->frames - 1)) {
		j->p_cloakphase++;
	    }
	} else {
	    if (j->p_cloakphase) {
		j->p_cloakphase--;
	    }
	}
	dx = j->p_x - me->p_x;
	dy = j->p_y - me->p_y;
	if (ABS(dx) > view || ABS(dy) > view) {
	    if(j->p_status == PEXPLODE)
		j->p_explode++;
	    continue;
	}
	dx = scaleLocal(dx);
	dy = scaleLocal(dy);
	if (j->p_status == PALIVE) {
            W_Image *shipimage = NULL;

            if(hockey)
	    {
	      if(!strcmp(j->p_name, "Puck"))
	        shipimage = getImage(I_IND_PU);
	      if(!strcmp(j->p_name, "Announcer"))
	        shipimage = getImage(I_IND_SB);
	    }
	    if(!shipimage)
	      shipimage = getShipImage(j);
	    /*shipimage = getShipImage(j->p_teami + 1, j->p_ship->s_bitmap);*/
	    if (j->p_flags & PFCLOAK && 
	       (j->p_cloakphase == (cloakimage->frames - 1))) {
		if (myPlayer(j)) {
		    W_DrawImage(w, dx - cloakimage->width / 2,
		                   dy - cloakimage->height / 2,
				   j->p_cloakphase,
				   cloakimage,
				   playerColor(j));
		    cz = new_czone();
		    cz->x = dx - (cloakimage->width / 2 + 1);
		    cz->y = dy - (cloakimage->height / 2 + 1);
		    cz->width = cloakimage->width + 2;
		    cz->height = cloakimage->height + 2;
		    doShields(dx, dy, shipimage, j);
		    doHull(dx, dy, shipimage, j);
		    if (showMySpeed)
			doShowMySpeed(dx, dy, shipimage, j);
		}
		continue;
	    }
	    cz = new_czone();
	    if (emph_player_seq_n[j->p_no] > 0 &&
		((F_beeplite_flags & LITE_PLAYERS_LOCAL) ||
		 ((j == me) && (F_beeplite_flags & LITE_SELF)))) {
		image = getImage(I_EMPH_PLAYER_SEQL);
		cz->x = dx - (image->width / 2);
		cz->y = dy - (image->height / 2);
		cz->width = image->width;
		cz->height = image->height;
		W_DrawImage(w, dx - image->width / 2,
			       dy - image->height / 2,
			       emph_player_seq_n[j->p_no],
			       image,
			       emph_player_color[j->p_no]);
	    } else
	    {
		cz->x = dx - (shipimage->width / 2);
		cz->y = dy - (shipimage->height / 2);
		cz->width = shipimage->width;
		cz->height = shipimage->height;
	    }
	    if (j->p_cloakphase > 0) {
	        if(xpm) {
		  W_OverlayImage(w, 
			 dx - shipimage->width / 2,
			 dy - shipimage->height / 2,
			 (rosette((int)j->p_dir,(int)shipimage->frames)),
			 shipimage,
			 j->p_cloakphase,
			 cloakimage,
			 playerColor(j));
		} else {
		  W_DrawImage(w, 
			 dx - shipimage->width / 2,
			 dy - shipimage->height / 2,
			 (rosette((int)j->p_dir,(int)shipimage->frames)),
			 shipimage,
			 playerColor(j));
		  W_DrawImage(w, 
			 dx - shipimage->width / 2,
			 dy - shipimage->height / 2,
			 j->p_cloakphase,
			 cloakimage,
			 playerColor(j));
		}
		doShields(dx, dy, shipimage, j);
		if (j == me) {
		    doHull(dx, dy, shipimage, j);
		    if (showMySpeed)
			doShowMySpeed(dx, dy, shipimage, j);
		}
		continue;
	    }
	    W_DrawImage(w, dx - shipimage->width / 2,
			   dy - shipimage->height / 2,
			   (rosette((int) j->p_dir, (int) shipimage->frames)),
			   shipimage,
			   playerColor(j));
	    if (showLock & 2) {
		if ((me->p_flags & PFPLOCK) && (me->p_playerl == j->p_no)) {
		    W_WriteTriangle(w, dx, dy + (shipimage->width / 2),
				    4, 1, foreColor);
		    cz = new_czone();
		    cz->x = dx - 4;
		    cz->y = dy + (shipimage->height / 2);
		    cz->width = 9;
		    cz->height = 5;
		}
	    }
	    doShields(dx, dy, shipimage, j);
	    doHull(dx, dy, shipimage, j);
	    if (showMySpeed && j == me)
		doShowMySpeed(dx, dy, shipimage, j);
	    else {
		int     color = playerColor(j);
		idbuf[0] = *(shipnos + j->p_no);

		W_MaskText(w, dx + (shipimage->width / 2),
			   dy - (shipimage->height / 2), color,
			   idbuf, 1, shipFont(j));

		cz = new_czone();
		cz->x = dx + (shipimage->width / 2);
		cz->y = dy - (shipimage->height / 2);
		cz->width = W_Textwidth;
		cz->height = W_Textheight;
	    }
	    if (j == puck && puckArrow) {
	      drawPuckArrow();
	    }
	}
	/* Now draw his phaser (if it exists) */
	php = &phasers[j->p_no];
	if (php->ph_status != PHFREE) {
	    if (php->ph_status == PHMISS) {
 		int dir = php->ph_dir;
  		/* Here I will have to compute end coordinate */
 		tx = j->p_x + j->p_ship->s_phaserrange * Cos[dir];
 		ty = j->p_y + j->p_ship->s_phaserrange * Sin[dir];
  		tx = scaleLocal(tx - me->p_x);
  		ty = scaleLocal(ty - me->p_y);
	    } else if (php->ph_status == PHHIT2) {
		tx = scaleLocal(php->ph_x - me->p_x);
		ty = scaleLocal(php->ph_y - me->p_y);
	    } else {		/* Start point is dx, dy */
		tx = scaleLocal(players[php->ph_target].p_x - me->p_x);
		ty = scaleLocal(players[php->ph_target].p_y - me->p_y);
	    }

	    php->ph_fuse++;

	    {
		if(friendlyPlayer(j) || j==me) /* check for j==me is for the screwy
						  dogfight server, where you are hostile
						  to your own team... */
		{
		    W_Color ph_col;
		    if (j==me && jubileePhasers) {

			if (php->ph_status == PHMISS)
			    ph_col = W_White;
			else {
			    switch (php->ph_fuse % 4) {
			    case 0:
				ph_col = W_Red;
				break;
			    case 1:
				ph_col = W_Yellow;
				break;
			    case 2:
				ph_col = W_Green;
				break;
			    case 3:
				ph_col = W_Cyan;
				break;
			    }
			}
		    } else {
			ph_col = (php->ph_fuse % 2 && php->ph_status != PHMISS) ?
			    foreColor :
			    shipCol[1 + j->p_teami];
		    }
		    W_CacheLine(w, dx, dy, tx, ty, ph_col);
		    clearline[0][clearlcount] = dx;
		    clearline[1][clearlcount] = dy;
		    clearline[2][clearlcount] = tx;
		    clearline[3][clearlcount] = ty;
		    clearlcount++;
		} else  if ((enemyPhasers > 0) && (enemyPhasers <= 10) && 
			    (php->ph_status == PHHIT)) {
		    register int    lx, ly, wx, wy, dir=php->ph_dir;
#define NORMALIZE(d) (((d) + 256) % 256)
		    wx = tx + enemyPhasers * Cos[NORMALIZE (dir + 64)];
		    wy = ty + enemyPhasers * Sin[NORMALIZE (dir + 64)];
		    lx = tx + enemyPhasers * Cos[NORMALIZE (dir - 64)];
		    ly = ty + enemyPhasers * Sin[NORMALIZE (dir - 64)];

		    W_CacheLine (w, wx, wy, dx, dy, shipCol[1 + j->p_teami]);
		    W_CacheLine (w, lx, ly, dx, dy, shipCol[1 + j->p_teami]);
		    php->ph_fuse++;
		    
		    clearline[0][clearlcount] = wx;
		    clearline[1][clearlcount] = wy;
		    clearline[2][clearlcount] = dx;
		    clearline[3][clearlcount] = dy;
		    clearlcount++;

		    clearline[0][clearlcount] = lx;
		    clearline[1][clearlcount] = ly;
		    clearline[2][clearlcount] = dx;
		    clearline[3][clearlcount] = dy;
		    clearlcount++;
		} else { /* not friendly &&  not using wide phasers, or a miss */
		    W_Color ph_col = (php->ph_fuse % 2 && php->ph_status != PHMISS) ?
			             foreColor :
				     shipCol[1 + j->p_teami];
		    W_CacheLine(w, dx, dy, tx, ty, ph_col);
		    clearline[0][clearlcount] = dx;
		    clearline[1][clearlcount] = dy;
		    clearline[2][clearlcount] = tx;
		    clearline[3][clearlcount] = ty;
		    clearlcount++;
		}
	    }			/* PHGHOST */
	}
    }
    /* draw torps and such after all the players [BDyess] */
    if (me->p_x >= 0) {
      for (i = 0, j = &players[i]; i < nplayers; i++, j++) {
	  redraw_photon_torps(j);
	  redraw_drones(j);
	  redraw_plasma_torps(j);
      }
    }

    /* ATM - show tractor/pressor beams (modified by James Collins) */
    /* showTractorPressor is a variable set by xtrekrc. */
    /* modified to show all T/P's 1/28/94 [BDyess] */
    /* fixed display bug 1/29/94 [BDyess] */
    /* Allow showAllTractorPressor in both Vanilla & Paradise
       (Trient Piepho, 3/13/2000 */
    dx = dy = center;
    if (showTractorPressor) {
	double  theta;
	unsigned char dir;
	int     lx[2], ly[2], px, py, target_width;
	struct player *victim = &players[me->p_tractor];
	int     last;
	if (showAllTractorPressor && allowShowAllTractorPressor) {
	    /* check everybody */
	    last = nplayers;
	    j = &players[0];
	    i = 0;
	} else {
	    /* only check self */
	    last = me->p_no + 1;
	    j = me;
	    i = me->p_no;
	}
	for (; i < last; i++, j++) {
	    if (!(j->p_flags & (PFTRACT | PFPRESS) && isAlive(j)))
		continue;
	    victim = &players[j->p_tractor];
	    if (victim->p_flags & PFCLOAK)
		break;		/* can't see tractors on cloaked opponents */
	    if (j == me) {
		dx = dy = center;
	    } else {
		dx = scaleLocal(j->p_x - me->p_x);
		dy = scaleLocal(j->p_y - me->p_y);
	    }
	    if (PtOutsideWin(dx, dy))
		continue;	/* he's off the screen */
	    px = scaleLocal(victim->p_x - me->p_x);
	    py = scaleLocal(victim->p_y - me->p_y);
	    if (px == dx && py == dy)
		break;
	    theta = atan2((double) (px - dx), (double) (dy - py)) + M_PI / 2.0;
	    dir = (unsigned char) (theta / M_PI * 128.0);
	    target_width = getShipImage(victim)->width;
	    /*target_width = getShipImage(victim->p_teami + 1,
				        victim->p_ship->s_bitmap)->width;*/
	    if (!(victim->p_flags & PFSHIELD))
		target_width /= 2;
	    lx[0] = px + (Cos[dir] * (target_width / 2));
	    ly[0] = py + (Sin[dir] * (target_width / 2));
	    lx[1] = px - (Cos[dir] * (target_width / 2));
	    ly[1] = py - (Sin[dir] * (target_width / 2));
	    if (j->p_flags & PFPRESS) {
		W_MakeTractLine(w, dx, dy, lx[0], ly[0], W_Yellow);
		W_MakeTractLine(w, dx, dy, lx[1], ly[1], W_Yellow);
	    } else {
		W_MakeTractLine(w, dx, dy, lx[0], ly[0], W_Green);
		W_MakeTractLine(w, dx, dy, lx[1], ly[1], W_Green);
	    }
	    /*
	       keeping track of tractors seperately from other lines allows
	       clearing them diffently.  Clearing them the same as other
	       solid lines caused occasional bits to be left on the screen.
	       This is the fix.  [BDyess]
	    */
	    if (tractcurrent == NULL) {	/* just starting */
		tractcurrent = tracthead = (Tractor *) malloc(sizeof(Tractor));
		tracthead->next = NULL;
	    }
	    tractcurrent->sx = dx;
	    tractcurrent->sy = dy;
	    tractcurrent->d1x = lx[0];
	    tractcurrent->d1y = ly[0];
	    tractcurrent->d2x = lx[1];
	    tractcurrent->d2y = ly[1];
	    /* get ready for the next run through */
	    if (tractcurrent->next) {	/* already malloc'd before */
		tractcurrent = tractcurrent->next;
	    } else {		/* new maximum, create a new struct */
		tractcurrent->next = (Tractor *) malloc(sizeof(Tractor));
		tractcurrent = tractcurrent->next;
		tractcurrent->next = NULL;
	    }
	}
    }
    redraw_other_drones();

    /* Draw Edges */
    /* sides of the galaxy [BDyess] */
    if (me->p_x < view) {
	int     sy, ey;
	dx = scaleLocal(-me->p_x);
	sy = scaleLocal(-me->p_y);
	ey = scaleLocal(blk_gwidth - me->p_y);
	if (sy < 0)
	    sy = 0;
	if (ey > winside - 1)
	    ey = winside - 1;
	/* XFIX */
	W_CacheLine(w, dx, sy, dx, ey, warningColor);
	/*
	   W_MakeLine(w, dx, sy, dx, ey, warningColor);
	*/
	clearline[0][clearlcount] = dx;
	clearline[1][clearlcount] = sy;
	clearline[2][clearlcount] = dx;
	clearline[3][clearlcount] = ey;
	clearlcount++;
    }
    if ((blk_gwidth - me->p_x) < view) {
	int     sy, ey;
	dx = scaleLocal(blk_gwidth-me->p_x);
	sy = scaleLocal(- me->p_y);
	ey = scaleLocal(blk_gwidth - me->p_y);
	if (sy < 0)
	    sy = 0;
	if (ey > winside - 1)
	    ey = winside - 1;
	/* XFIX */
	W_CacheLine(w, dx, sy, dx, ey, warningColor);
	/*
	   W_MakeLine(w, dx, sy, dx, ey, warningColor);
	*/
	clearline[0][clearlcount] = dx;
	clearline[1][clearlcount] = sy;
	clearline[2][clearlcount] = dx;
	clearline[3][clearlcount] = ey;
	clearlcount++;
    }
    if (me->p_y < view) {
	int     sx, ex;
	dy = scaleLocal(-me->p_y);
	sx = scaleLocal(-me->p_x);
	ex = scaleLocal(blk_gwidth - me->p_x);
	if (sx < 0)
	    sx = 0;
	if (ex > winside - 1)
	    ex = winside - 1;
	/* XFIX */
	W_CacheLine(w, sx, dy, ex, dy, warningColor);
	/*
	   W_MakeLine(w, sx, dy, ex, dy, warningColor);
	*/
	clearline[0][clearlcount] = sx;
	clearline[1][clearlcount] = dy;
	clearline[2][clearlcount] = ex;
	clearline[3][clearlcount] = dy;
	clearlcount++;
    }
    if ((blk_gwidth - me->p_y) < view) {
	int     sx, ex;
	dy = scaleLocal(blk_gwidth - me->p_y);
	sx = scaleLocal( - me->p_x);
	ex = scaleLocal(blk_gwidth - me->p_x);
	if (sx < 0)
	    sx = 0;
	if (ex > winside - 1)
	    ex = winside - 1;
	/* XFIX */
	W_CacheLine(w, sx, dy, ex, dy, warningColor);
	/*
	   W_MakeLine(w, sx, dy, ex, dy, warningColor);
	*/
	clearline[0][clearlcount] = sx;
	clearline[1][clearlcount] = dy;
	clearline[2][clearlcount] = ex;
	clearline[3][clearlcount] = dy;
	clearlcount++;
    }
    /* Change border color to signify alert status */

#ifdef UNIX_SOUND
    if (oldalert != (me->p_flags & (PFGREEN | PFYELLOW | PFRED)))
    {
      if (me->p_flags & PFRED)
            maybe_play_sound (SND_REDALERT);   /* Red Alert, play ONLY once */
      else  sound_completed  (SND_REDALERT);   /* Done with Red Alert */
    }
#endif

    if (oldalert != (me->p_flags & (PFGREEN | PFYELLOW | PFRED))) {
	if(paradise && auto_zoom_timer<udcounter && (autoZoom || autoUnZoom)) {
	    int old_zoom=blk_zoom;
	    switch(autoZoom) {
	    case 1:
		if(me->p_flags & (PFYELLOW | PFRED))
		    blk_zoom = 1;
		break;
	    case 2:
		if(me->p_flags & (PFRED))
		    blk_zoom = 1;
		break;
	    }
	    switch(autoUnZoom) {
	    case 1:
		if(me->p_flags & (PFGREEN))
		    blk_zoom = 0;
		break;
	    case 2:
		if(me->p_flags & (PFYELLOW | PFGREEN))
		    blk_zoom = 0;
		break;
	    }
	    if(old_zoom != blk_zoom) {
		redrawall=1;
	    }
	}
	oldalert = (me->p_flags & (PFGREEN | PFYELLOW | PFRED));
	if (infoIcon && iconified)
	    drawIcon();
	switch (oldalert) {
	case PFGREEN:
	    if (extraBorder)
		W_ChangeBorder(w, gColor);
	    W_ChangeBorder(baseWin, gColor);
	    W_ChangeBorder(iconWin, gColor);
	    break;
	case PFYELLOW:
	    if (extraBorder)
		W_ChangeBorder(w, yColor);
	    W_ChangeBorder(baseWin, yColor);
	    W_ChangeBorder(iconWin, yColor);
	    break;
	case PFRED:
	    if (extraBorder)
		W_ChangeBorder(w, rColor);
	    W_ChangeBorder(baseWin, rColor);
	    W_ChangeBorder(iconWin, rColor);
	    break;
	}
    }
    /* draw stars */

    if (blk_showStars)
	drawStars();

    if(localShipStats)
	doLocalShipstats();
    if(hudwarning) {
      if(warncount)
	W_MaskText(w, center - (warncount / 2) * W_Textwidth, 
	           winside - W_Textheight - HUD_Y, 
	           W_Green, warningbuf, warncount, W_RegularFont);
      if(hwarncount)
	W_MaskText(w, center - (hwarncount / 2) * W_Textwidth, HUD_Y, 
	           W_Yellow, hwarningbuf, hwarncount, W_BoldFont);
    }
}
