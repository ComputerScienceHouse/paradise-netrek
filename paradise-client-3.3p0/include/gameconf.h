/*
   this header declares declares variables for much information that
   used to be constant.
   */

#ifndef GAMECONF_H
#define GAMECONF_H

#include "Wlib.h"
#include "struct.h"
#include "proto.h"

struct teaminfo_s {
    char    name[32];		/* this is not meant to limit the length of
				   team names */
    W_Image *shield_logo;	/* logo that appears in the team choice
				   window */
    char    letter;		/* 1-letter abbreviation */
    char    shortname[4];	/* 3-letter abbreviation */
};

struct gameparam_spacket;

void handleGameparams P((struct gameparam_spacket *));
void load_default_teamlogos P((void));

#endif
