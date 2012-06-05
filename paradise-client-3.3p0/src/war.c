/*
 * war.c
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
#include "packets.h"
#include "gppackets.h"

static int newhostile;

/* Set up the war window and map it */
static char *feds = "FED - ";
static char *roms = "ROM - ";
static char *klis = "KLI - ";
static char *oris = "ORI - ";
static char *gos = "  Re-program";
static char *exs = "  Exit - no change";
static char *peaces = "Peace";
static char *hostiles = "Hostile";
static char *wars = "War";

/* Prototypes */
static void fillwin P((int menunum, char *string, int hostile, int warbits, int team));
static void warrefresh P((void));

void
warwindow(void)
{
    W_MapWindow(war);
    newhostile = me->p_hostile;
    warrefresh();
}

static void
warrefresh(void)
{
    fillwin(0, feds, newhostile, me->p_swar, FEDm);
    fillwin(1, roms, newhostile, me->p_swar, ROMm);
    fillwin(2, klis, newhostile, me->p_swar, KLIm);
    fillwin(3, oris, newhostile, me->p_swar, ORIm);
    W_WriteText(war, 0, 4, textColor, gos, strlen(gos), 0);
    W_WriteText(war, 0, 5, textColor, exs, strlen(exs), 0);
}

static void
fillwin(int menunum, char *string, int hostile, int warbits, int teammask)
{
    char    buf[80];

    if (teammask & warbits) {
	(void) sprintf(buf, "  %s%s", string, wars);
	W_WriteText(war, 0, menunum, rColor, buf, strlen(buf), 0);
    } else if (teammask & hostile) {
	(void) sprintf(buf, "  %s%s", string, hostiles);
	W_WriteText(war, 0, menunum, yColor, buf, strlen(buf), 0);
    } else {
	(void) sprintf(buf, "  %s%s", string, peaces);
	W_WriteText(war, 0, menunum, gColor, buf, strlen(buf), 0);
    }
}

void
waraction(W_Event *data)
{
    int     enemymask;

    if (data->y == 4) {
	W_UnmapWindow(war);
	sendWarReq(newhostile);
	return;
    }
    if (data->y == 5) {
	W_UnmapWindow(war);
	return;
    }
    enemymask = 1 << data->y;

    if (me->p_swar & enemymask) {
	warning("You are already at war!");
	W_Beep();
    } else {
	if (idx_to_mask(me->p_teami) == enemymask) {
	    warning("It would never work ... your crew would have you in the brig in no time.");
	} else {
	    newhostile ^= enemymask;
	}
    }
    warrefresh();
}
