/*
 * enter.c
 *
 * This version modified to work as the client in a socket based protocol.
 */
#include "copyright.h"

#include "config.h"

#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/* Enter the game */

/* Prototypes */

void
enter(void)
{
    redrawTstats();
    delay = 0;
}

/* Doesn't really openmem, but it will
 * set some stuff up...
 */
static struct status dummy1;
static struct status2 dummy2;
void
openmem(void)
{
    /* players, weapons, planets are handled in build_default_configuration */

    /* thingies allocation is handled in build_default_configuration */
    /* thingies = universe.drones; */


    status = &dummy1;
    status2 = &dummy2;

    /* mctl->mc_current=0; */
    status->time = 1;
    status->timeprod = 1;
    status->kills = 1;
    status->losses = 1;
    status->time = 1;
    status->planets = 1;
    status->armsbomb = 1;

    if (ghoststart) {
	me = &players[ghost_pno];
	myship = me->p_ship;
	mystats = &(me->p_stats);
    }
}
