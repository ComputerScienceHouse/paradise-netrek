/*
 * varydamage.c
 */
#include "copyright.h"

#include "config.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "proto.h"
#include "images.h"

void
doShields(int dx, int dy, W_Image *ship_bits, struct player *j)
{
    if (showShields && (j->p_flags & PFSHIELD)) {
	/*-----------Colored shields by shield damage--------*/

	int     color = playerColor(j);
	if (show_shield_dam && j == me) {
	    float   ft;

	    ft = (float) j->p_shield / (float) j->p_ship->s_maxshield;
	    if (ft > 0.66)
		color = gColor;
	    else if (ft > 0.33)
		color = yColor;
	    else if (j->p_shield > 5)
		color = rColor;
	    else
		color = unColor;
	}
	W_DrawShield(w, dx, dy, ship_bits->width, color);
    }
}

void
doHull(int dx, int dy, W_Image *ship_bits, struct player *j)
{
    W_Image *image;
    
    if (j == me && vary_hull) {
	int     hull_left = (100 * (me->p_ship->s_maxdamage -
		     me->p_damage)) / me->p_ship->s_maxdamage, hull_num = 7;
	int     hull_color;

	if (hull_left <= 16) {
	    hull_num = 0;
	    hull_color = W_Red;
	} else if (hull_left <= 28) {
	    hull_num = 1;
	    hull_color = W_Red;
	} else if (hull_left <= 40) {
	    hull_num = 2;
	    hull_color = W_Red;
	} else if (hull_left <= 52) {
	    hull_num = 3;
	    hull_color = W_Yellow;
	} else if (hull_left <= 64) {
	    hull_num = 4;
	    hull_color = W_Yellow;
	} else if (hull_left <= 76) {
	    hull_num = 5;
	    hull_color = W_Yellow;
	} else if (hull_left <= 88) {
	    hull_num = 6;
	    hull_color = W_Green;
	} else
	    hull_color = W_Green /* playerColor (j) */ ;

        image = getImage(I_HULL);
	W_DrawImage(w, dx - image->width / 2 + 1,
	               dy - image->height /2 + 1,
		       hull_num,
		       image,
		       hull_color);
    }
}
