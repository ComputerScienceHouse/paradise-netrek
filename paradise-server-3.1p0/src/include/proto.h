/*------------------------------------------------------------------
  Copyright 1989		Kevin P. Smith
				Scott Silvey

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies.

  NETREK II -- Paradise

  Permission to use, copy, modify, and distribute this software and
  its documentation, or any derivative works thereof,  for any 
  NON-COMMERCIAL purpose and without fee is hereby granted, provided
  that this copyright notice appear in all copies.  No
  representations are made about the suitability of this software for
  any purpose.  This software is provided "as is" without express or
  implied warranty.

	Xtrek Copyright 1986			Chris Guthrie
	Netrek (Xtrek II) Copyright 1989	Kevin P. Smith
						Scott Silvey
	Paradise II (Netrek II) Copyright 1993	Larry Denys
						Kurt Olsen
						Brandon Gillespie
		                Copyright 2000  Bob Glamm

--------------------------------------------------------------------*/

#ifndef PROTO_H
#define PROTO_H

#include "config.h"
#include "defs.h"
#include "struct.h"

/* *************************************************************************
   from common/
   ************************************************************************* */

/* common/cutil.c */
/* r_signal() takes a signal number and a handler of type void f(), returns
   old handler.  Handler _has_ to have no args in the parens. */
/* The signal handlers should be the only prototypes without the P(). */
void (*r_signal 
         P( (int, void (*)()) ) 
     ) ();
#ifndef HAVE_STRDUP
char *strdup P((char *));
#endif

/* common/data.c */
void init_data P((char *));

/* common/detonate.c */
void detothers P((void));

/* common/enter.c */
int findrslot P((void));
int enter P((int, int, int, int, int));

/* common/getship.c */
void getship P((struct ship *, int));
void get_ship_for_player P((struct player *, int));

/* common/grid.c */
void move_planet P((int, int, int, int));
void move_player P((int, int, int, int));
void move_torp P((int, int, int, int));
void move_missile P((int, int, int, int));

/* common/imath.c  - this needs to go IMHO as everyone has FPUs these days ;)*/
int isqrt P((int));
int ihypot P((int, int));

/* common/interface.c */
void set_speed P((int, int));
void set_course P((unsigned char));
void shield_up P((void));
void shield_down P((void));
void bomb_planet P((void));
void beam_up P((void));
void beam_down P((void));
void repair P((void));
void repair_off P((void));
void cloak_on P((void));
void cloak_off P((void));
void lock_planet P((int));
void lock_player P((int));
void tractor_player P((int));
void pressor_player P((int));
void declare_war P((int));
void switch_special_weapon P((void));
void do_refit P((int));
int allowed_ship P((int, int, int, int));
int numPlanets P((int));

/* common/orbit.c */
void orbit P((void));
void newdock P((int));

/* common/parse-ranks.c */
void parse_ranks P((char *));

/* common/path.c */
char *build_path P((char *));

/* common/phaser.c */
void phaser P((unsigned char));

/* common/plutil.c */
int idx_to_mask P((int));
int mask_to_idx P((int));
int undock_player P((struct player *));
int base_undock P((struct player *, int));
void enforce_dock_position P((struct player *));
void dock_to P((struct player *, int, int));
void scout_planet P((int, int));
void evaporate P((struct player *));
void explode_everyone P((int, int));
int random_round P((double));
char *twoletters P((struct player *));

/* common/shmem.c */
void startdaemon P((int, int));
void openmem P((int, int));
void blast_shmem P((void));

/* common/sintab.c */
void init_trig P((void));

/* common/smessage.c */
void pmessage P((char *, int, int, char *));
void pmessage2 P((char *, int, int, char *, unsigned char));

/* common/torp.c */
void ntorp P((unsigned char, int));

/* common/util.c */
unsigned char getcourse P((int, int, int, int));
int angdist P((unsigned char, unsigned char));
int temporally_spaced P((struct timeval *, int));
int check_fire_warp P((void));
int check_fire_warpprep P((void));
int check_fire_docked P((void));

/* common/warning.c */
void warning P((char *));
void updateWarnings P((void));
void imm_warning P((char *));

#endif /* PROTO_H */
