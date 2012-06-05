/*
 * planets.c
 *
 * Kevin P. Smith  2/21/89
 *
 * This file contains the galaxy definition as well as some support for
 *  determining when parts of the galactic map need to be redrawn.
 */
#include "copyright2.h"

#include "config.h"
#include <stdlib.h>
#include <math.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/* define to get profiling info, to better set DEPTH and DETAIL [BDyess] */
#if 0
#define PROFILE1		/* maxdepth printed */
#define PROFILE2		/* every PLREDRAW setting printed (VERBOSE) */
#endif /*0*/

/* Note:  DETAIL * MUST * be a factor of GWIDTH. */
#define DETAIL 50		/* Size of redraw array */
#define RADIUS 2		/* number of adjacent cells to register in */
#define SIZE 4000
#define DEPTH 8
#ifdef PROFILE1
#define ADDPLANET(i,j,k) { \
    for(z=0;z<DEPTH;z++) { \
	if(redraws[i][j][z] == (char)-1) { \
	    redraws[i][j][z] = k; \
	    break; \
	} \
    }  z++; \
    if(z > maxdepth) maxdepth = z; \
}
#else				/* not PROFILEing */
#define ADDPLANET(i,j,k) { \
    for(z=0;z<DEPTH;z++) { \
	if(redraws[i][j][z] == (char)-1) { \
	    redraws[i][j][z]=k; \
	    break; \
	} \
    } \
}
#endif				/* PROFILE1 */

#define LEFT 	1		/* position flags for corner guessing */
#define RIGHT 	2
#define TOP 	4
#define BOTTOM	8

struct _star {
    int     s_x, s_y;
    int     s_color;
};

static void redrawStarSector P((int sectorx, int sectory));

/*
   planets may be on top of each other or very close, so more than one planet
   can exist in a given block.  The 3rd dimension is for a list of planets.
   [BDyess]

*/
char    redraws[DETAIL][DETAIL][DEPTH];
struct point *asteroid_footprints;
int max_asteroid_footprints = 100;
int next_asteroid_footprint = 0;
static int initialized = 0;

/*
   rewritten to repair UNBELIEVABLE inefficiencies in the original [BDyess]
*/
void
initPlanets(void)
{
    register int i, j, k, x, y, z, r, s;
    struct planet *pl;
#ifdef PROFILE1
    int     maxdepth = 0;
#endif				/* PROFILE1 */

    /* initialize asteroid footfall table [BDyess] */
    asteroid_footprints = (struct point*)
			  malloc(sizeof(struct point)*max_asteroid_footprints);

    /* initialize lookup array */
    for (i = 0; i < DETAIL; i++) {
	for (j = 0; j < DETAIL; j++) {
	    for (k = 0; k < DEPTH; k++) {
		redraws[i][j][k] = -1;
	    }
	}
    }

    /*
       check all the planets.  If part of the planet falls inside a box in
       the lookup array, add it to the refresh list for that box.  [BDyess]
    */
    for (k = 0, pl = planets; k < nplanets; k++, pl++) {
	x = pl->pl_x;
	y = pl->pl_y;
	i = x / SIZE - 3;
	j = y / SIZE - 2;
	/*
	   fill cells around the target cell with the planetnum like so: ****
	
	**
	
	x** ****
	
	**
	
	**	taller than wide because of text ****
	
	[BDyess]
	*/
	for (r = 0; r < 5; r++) {
	    i++;
	    if (i <= 0)
		continue;
	    if (i >= DETAIL)
		break;
	    for (s = 0; s < 7; s++) {
		j += s;
		if (j <= 0)
		    continue;
		if (j >= DETAIL)
		    break;
		ADDPLANET(i, j, k)
		    j -= s;
	    }
	}
    }
    initialized = 1;
#ifdef PROFILE1
    printf("max depth = %d\n", maxdepth);
#endif				/* PROFILE1 */
}

void
checkRedraw(int x, int y)
{
    int     j;
    char    i;

    if (!initialized || x < 0 || y < 0 || x >= blk_gwidth || y >= blk_gwidth)
	return;

    x /= SIZE;
    y /= SIZE;

    if(received_terrain_info) {
      /* add the footprint to the list if not already there [BDyess] */
      for(j = 0; j < next_asteroid_footprint; j++) {
	if(asteroid_footprints[j].x == x &&
	   asteroid_footprints[j].y == y) break;
      }
      if(j == next_asteroid_footprint) {
	next_asteroid_footprint++;
	asteroid_footprints[j].x = x;
	asteroid_footprints[j].y = y;
      }
      /* if out of space, create some more [BDyess] */
      if(next_asteroid_footprint == max_asteroid_footprints) {
	asteroid_footprints = (struct point*)realloc(asteroid_footprints,
					     max_asteroid_footprints *= 2);
      }
    }

    for (j = 0; j < DEPTH; j++) {
	i = redraws[x][y][j];
	if (i == (char) -1)
	    return;
	planets[i].pl_flags |= PLREDRAW;
#ifdef PROFILE2
	printf("setting PLREDRAW flag for %s\n", planets[i].pl_name);
#endif				/* PROFILE2 */
    }
}


/* NOTE: this has been rewritten -- see below */

#define NUMSTARS 1600

static int starsX[NUMSTARS];
static int starsY[NUMSTARS];

void
_initStars(void)
{
    register int i;

    for (i = 0; i < NUMSTARS; i++) {
	starsX[i] = (i % 40) * 5000 + random() % 5000;
	starsY[i] = (i / 40) * 5000 + random() % 5000;
    }
}

void
_drawStars(void)
{
    int     i;
    int     x, y;

    for (i = 0; i < NUMSTARS; i++) {
	x = starsX[i] - me->p_x;
	y = starsY[i] - me->p_y;
	if (ABS(x) < 10000 && ABS(y) < 10000) {
	    x = scaleLocal(x);
	    y = scaleLocal(y);
	    W_DrawPoint(w, x, y, W_White);
	    W_CacheClearArea(w, x, y, 1, 1);
	}
    }
}

/*
 *  This rewrite improves drawStars() by a factor of about 30 (according to
 *  gprof)
 */

/* blk_gwidth/(WINSIDE * SCALE) == 10 for blk_gwidth == 200000 */
static struct _star stars[10][10][16];

void
initStars(void)
{
    register int i, j, k;

    for (i = 0; i < 10; i++) {
	for (j = 0; j < 10; j++) {
	    for (k = 0; k < 16; k++) {
		stars[i][j][k].s_x = random() % 20000;
		stars[i][j][k].s_y = random() % 20000;
		stars[i][j][k].s_color = randcolor();
	    }
	}
    }
}

int
randcolor(void)
{
    switch (random() % 10) {
	case 0:return W_Yellow;
    case 1:
	return W_Red;
    case 2:
	return W_Green;
    case 3:
	return W_Cyan;
    default:
	return W_White;
    }
}

void
drawStars(void)
{
    /*
       note: cpp symbols in expressions (WINSIDE*SCALE) will be precalculated
       by any C optimizer
    */
    int     sectorx = me->p_x / (fullview), sectory = me->p_y / (fullview);
    int     sector_offx = me->p_x - sectorx * (fullview), sector_offy = me->p_y - sectory * (fullview);
    int     l = 0, r = 0, t = 0, b = 0;

    if (sector_offx < 0) {	/* goddamn rounding towards 0 */
	sectorx--;
	sector_offx += fullview;
    }
    if (sector_offy < 0) {	/* goddamn rounding towards 0 */
	sectory--;
	sector_offy += fullview;
    }
#define	MAXSECTOR	(blk_gwidth/(fullview))

    /* at worst we have to redraw 4 star sectors */

    /* draw the one we're in */
    /*
       check first to make sure it's valid.  This is mainly important for if
       it tries to redraw and we're already dead

       or if we exit - 3/24/2000 rpg
    */
    if (sectorx < 0 || sectory < 0 || 
        sectorx >= MAXSECTOR || sectory >= MAXSECTOR)
	return;

    l = sector_offx < view && sectorx > 0;
    r = sector_offx > view && sectorx + 1 < MAXSECTOR;
    t = sector_offy < view && sectory > 0;
    b = sector_offy > view && sectory + 1 < MAXSECTOR;

    if (t) {
	if (l)			/* redraw upper-left sector */
	    redrawStarSector(sectorx-1,sectory-1);

	/* redraw upper sector */
	redrawStarSector(sectorx,sectory-1);

	if (r)			/* redraw upper-right sector */
	    redrawStarSector(sectorx+1,sectory-1);
    }
    if (l)			/* redraw left sector */
	redrawStarSector(sectorx - 1,sectory);

    /* redraw center sector */
    redrawStarSector(sectorx,sectory);

    if (r)			/* redraw right sector */
	redrawStarSector(sectorx + 1,sectory);

    if (b) {
	if (l)			/* redraw bottom-left sector */
	    redrawStarSector(sectorx - 1,sectory + 1);

	/* redraw bottom sector */
	redrawStarSector(sectorx,sectory + 1);

	if (r)			/* redraw bottom-right sector */
	    redrawStarSector(sectorx + 1,sectory + 1);
    }
    W_FlushPointCaches(w);
}

static void
redrawStarSector(int sectorx, int sectory)
{
    register int i, dx, dy, dxx, dyy, xbase = sectorx * fullview, 
             ybase = sectory * fullview;
    register struct _star *s;
    static int warpflag = 0;	/* assume starting out not in warp */
    static int streaksOn = 0, lastspeed = 0, lastsubspeed = 0, updates = 0;
    static int streaklength = 1;
    struct _star *star_sector = stars[sectorx][sectory];

    if (warpStreaks) {
	if (warpflag != (me->p_flags & PFWARP)) {	/* change in warp state */
	    streaksOn = 1;
	    warpflag = (me->p_flags & PFWARP);
	}
	if (streaksOn) {
	    if (warpflag && (me->p_speed < lastspeed ||
	    (me->p_speed == lastspeed && me->p_subspeed <= lastsubspeed))) {
		/* finished accelerating */
		updates++;
		if (updates > 5) {
		    lastspeed = me->p_speed;
		    lastsubspeed = me->p_subspeed;
		    updates = 0;
		    streaksOn = 0;
		    redrawStarSector(sectorx,sectory);
		    return;
		}
	    } else if (streaklength == 1 || (!warpflag && ((me->p_speed > lastspeed) ||
	    (me->p_speed == lastspeed && me->p_subspeed >= lastsubspeed)))) {
		/* finished decelerating */
		updates++;
		if (updates > 5) {
		    lastspeed = me->p_speed;
		    lastsubspeed = me->p_subspeed;
		    updates = 0;
		    streaksOn = 0;
		    streaklength = 1;
		    redrawStarSector(sectorx,sectory);
		    return;
		}
	    } else
		updates = 0;
	    lastspeed = me->p_speed;
	    lastsubspeed = me->p_subspeed;
	    /* draw the streaks */
	    if (warpflag)
		streaklength += 3;
	    else
		streaklength--;
	    dxx = (int) (Cos[me->p_dir] * streaklength);
	    dyy = (int) (Sin[me->p_dir] * streaklength);
	    for (i = 0, s = star_sector; i < 16; i++, s++) {
		dx = (s->s_x + xbase) - me->p_x;
		dy = (s->s_y + ybase) - me->p_y;
		if (ABS(dx) > (view) || ABS(dy) > (view))
		    continue;

		dx = scaleLocal(dx);
		dy = scaleLocal(dy);
		W_CacheLine(w, dx, dy, dx - dxx, dy - dyy, s->s_color);

		clearline[0][clearlcount] = dx;
		clearline[1][clearlcount] = dy;
		clearline[2][clearlcount] = dx - dxx;
		clearline[3][clearlcount] = dy - dyy;
		clearlcount++;
	    }
	    return;
	}
    }
    for (i = 0, s = star_sector; i < 16; i++, s++) {
	dx = (s->s_x + xbase) - me->p_x;
	dy = (s->s_y + ybase) - me->p_y;
	if (ABS(dx) > (view) || ABS(dy) > (view))
	    continue;

	dx = scaleLocal(dx);
	dy = scaleLocal(dy);
	W_CachePoint(w, dx, dy, s->s_color);
	/*
	   this is a minor kludge: as long as there are less then 128 stars
	   in a sector these cached requests will not actually be written to
	   the X server until the next redraw cycle begins.
	*/
	W_CacheClearArea(w, dx, dy, 1, 1);
    }
}
