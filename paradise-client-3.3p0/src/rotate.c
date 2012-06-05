/*
 * rotate.c
 *
 */
#include "copyright2.h"

#include "config.h"
#include <math.h>

#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

void
rotate_dir(unsigned char *d, int r)
{
    (*d) += r;
}

/* general rotation */
/*
    int    *x, *y;		 values used and returned 
    int     d;			 degree (pi == 128) 
    int     cx, cy;		 around center point 
*/
void
rotate_coord(int *x, int *y, int d, int cx, int cy)
{
    register
    int     ox, oy;

    ox = *x;
    oy = *y;

    switch (d) {

    case 0:
	return;

    case 64:
    case -192:
	*x = cy - oy + cx;
	*y = ox - cx + cy;
	break;

    case 128:
    case -128:
	*x = cx - ox + cx;
	*y = cy - oy + cy;
	break;

    case 192:
    case -64:
	*x = oy - cy + cx;
	*y = cx - ox + cy;
	break;

    default:{
	    /* do it the hard way */
	    /*
	       there is another way to do this, by using the cos(d) and
	       sin(d) and the stock rotation matrix from linear algebra.
	    */
	    double  dir;
	    double  r, dx, dy;
	    double  rd = (double) d * 3.1415927 / 128.;

	    if (*x != cx || *y != cy) {
		dx = (double) (*x - cx);
		dy = (double) (cy - *y);
		dir = atan2(dx, dy) - 3.1415927 / 2.;
		r = hypot(dx, dy);
		dir += rd;
		*x = (int) (r * cos(dir) + cx);
		*y = (int) (r * sin(dir) + cy);
	    }
	}
    }
}

void
rotate_gcenter(int *x, int *y)
{
    rotate_coord(x, y, rotate_deg, blk_gwidth / 2, blk_gwidth / 2);
}

void
rotate_all(void)
{
    register int i;
    register struct planet *l;
    register struct player *j;

    int     nplan = (paradise) ? nplanets : 40;
    int     old_rotate_deg = rotate_deg;

    redrawall = 1;
    reinitPlanets = 1;

    rotate_deg = -old_rotate_deg + rotate * 64;

    for (i = 0, l = planets; i < nplan; i++, l++) {
	rotate_gcenter(&l->pl_x, &l->pl_y);
    }

    for (i=0, j = players; i < nplayers; i++, j++) {
	rotate_gcenter(&j->p_x, &j->p_y);
	rotate_dir(&j->p_dir, rotate_deg);
    }

    rotate_deg = rotate * 64;
}
