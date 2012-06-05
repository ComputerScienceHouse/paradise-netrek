/*
 * interface.c
 *
 * This file will include all the interfaces between the input routines
 *  and the daemon.  They should be useful for writing robots and the
 *  like
 */
#include "copyright.h"

#include "config.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "packets.h"
#include "gppackets.h"

void
set_speed(int speed)
{
    /* don't repeat useless commands [BDyess] */
    if (me->p_desspeed != speed || me->p_speed != speed)
	sendSpeedReq(speed);
    me->p_desspeed = speed;
}

void
set_course(unsigned int dir)
{
    /* don't repeat commands [BDyess] */
    if (me->p_desdir != dir || me->p_dir != dir)
	sendDirReq(dir);
    me->p_desdir = dir;
}

void
shield_up(void)
{
    if (!(me->p_flags & PFSHIELD)) {
	sendShieldReq(1);
    }
}

void
shield_down(void)
{
    if (me->p_flags & PFSHIELD) {
	sendShieldReq(0);
    }
}

void
shield_tog(void)
{
    if (me->p_flags & PFSHIELD) {
	sendShieldReq(0);
    } else {
	sendShieldReq(1);
    }
}

void
bomb_planet(void)
{
    if (!(me->p_flags & PFBOMB)) {
	sendBombReq(1);
    }
}

void
beam_up(void)
{
    if (!(me->p_flags & PFBEAMUP)) {
	sendBeamReq(1);		/* 1 means up... */
    }
}

void
beam_down(void)
{
    if (!(me->p_flags & PFBEAMDOWN)) {
	sendBeamReq(2);		/* 2 means down... */
    }
}

void
cloak(void)
{
    if (me->p_flags & PFCLOAK) {
	sendCloakReq(0);
    } else {
	sendCloakReq(1);
    }
}

int
mtime(void)
{
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *) 0);
    return (tv.tv_sec & 0x0ffff) * 1000 + tv.tv_usec / 1000;
}
