/*
 * robot/robot_functions.c
 *
 * This is a set of functions used by robots.
 */


#include "robot_functions.h"


/* This function logs the robot in */
void do_robot_login(void) {
   struct statentry player;
   struct stat buf;
   int     plfd, position, entries;
   char    *path;

   if (configvals->robot_stats) {
      path = build_path(PLAYERFILE);
      plfd = open(path, O_RDONLY, 0);
      if (plfd < 0) {
         fprintf(stderr, "%s: I cannot open the player file! (errno: %d)\n",
            argv0, errno);
         me->p_pos = -1;
         return;
      }

      position = 0;

      while (read(plfd, (char *) &player, sizeof(struct statentry)) ==
         sizeof(struct statentry)) {
         if (!strcmp(pseudo, player.name)) {
            close(plfd);
            me->p_pos = position;
            memcpy(&(me->p_stats), &player.stats, sizeof(struct stats));
            return;
         }

         position++;
      }  /* end while */

      /* Not in there, so create it */
      strcpy(player.name, pseudo);

      /* an invalid password to prevent logins */
      strcpy(player.password, "*");

      memset(&player.stats, 0, sizeof(struct stats));
      player.stats.st_tticks = 1;
      player.stats.st_flags = ST_INITIAL;

      plfd = open(path, O_RDWR | O_CREAT, 0644);
      if (plfd < 0) {
         fprintf(stderr, "%s: I cannot open the player file! (errno: %d)\n", 
            argv0, errno);
         me->p_pos = -1;
         return;
      }
      else {
         fstat(plfd, &buf);
         entries = buf.st_size / sizeof(struct statentry);
         lseek(plfd, entries * sizeof(struct statentry), 0);
         write(plfd, (char *) &player, sizeof(struct statentry));
         close(plfd);
         me->p_pos = entries;
         memcpy(&(me->p_stats), &player.stats, sizeof(struct stats));
      }
   }
}  /* end do_robot_login() */


/* This function saves the robot's stats */
void save_robot(void) {
   int     fd;
   char   *path;

   if(configvals->robot_stats) {
      if (me->p_pos < 0) {
         return;
      }

      path = build_path(PLAYERFILE); 
      fd = open(path, O_WRONLY, 0644);

      if (fd >= 0) {
         me->p_stats.st_lastlogin = time(NULL);
         lseek(fd, 32 + me->p_pos * sizeof(struct statentry), 0);
	 write(fd, (char *) &me->p_stats, sizeof(struct stats));
         close(fd);
      }
   }
}  /* end save_robot() */


/* Calculates class-specific stuff */
void config(void) {
   phrange = PHASEDIST * me->p_ship.s_phaser.damage / 100;
   trrange = TRACTDIST * me->p_ship.s_tractrng;

   switch (myship->s_type) {
      case SCOUT:
         dogslow = 5;
         dogfast = 7;
         runslow = 8;
         runfast = 9;
         closeslow = 5;
         closefast = 7;
         break;
      case DESTROYER:
         dogslow = 4;
         dogfast = 6;
         runslow = 6;
         runfast = 8;
         closeslow = 4;
         closefast = 6;
         break;
      case CRUISER:
         dogslow = 4;
         dogfast = 6;
         runslow = 6;
         runfast = 7;
         closeslow = 4;
         closefast = 6;
         break;
      case BATTLESHIP:
         dogslow = 3;
         dogfast = 5;
         runslow = 5;
         runfast = 6;
         closeslow = 3;
         closefast = 4;
         break;
      case FRIGATE:
         dogslow = 3;
         dogfast = 5;
         runslow = 5;
         runfast = 6;
         closeslow = 3;
         closefast = 4;
         break;
      case ASSAULT:
         dogslow = 3;
         dogfast = 5;
         runslow = 6;
         runfast = 7;
         closeslow = 3;
         closefast = 4;
         break;
      case JUMPSHIP:
         dogslow = 2;
         dogfast = 3;
         runslow = 2;
         runfast = 3;
         closeslow = 2;
         closefast = 3;
         break;
      case STARBASE:
         dogslow = 2;
         dogfast = 2;
         runslow = 2;
         runfast = 2;
         closeslow = 2;
         closefast = 2;
         break;
      case WARBASE:
         dogslow = 2;
         dogfast = 2;
         runslow = 2;
         runfast = 2;
         closeslow = 2;
         closefast = 2;
         break;
      case LIGHTCRUISER:
         dogslow = 5;
         dogfast = 6;
         runslow = 6;
         runfast = 7;
         closeslow = 5;
         closefast = 7;
         break;
      case CARRIER:
         dogslow = 3;
         dogfast = 4;
         runslow = 5;
         runfast = 6;
         closeslow = 4;
         closefast = 6;
         break;
      case UTILITY:
         dogslow = 3;
         dogfast = 4;
         runslow = 5;
         runfast = 6;
         closeslow = 4;
         closefast = 5;
         break;
      case PATROL:
         dogslow = 7;
         dogfast = 8;
         runslow = 9;
         runfast = 10;
         closeslow = 8;
         closefast = 9;
         break;
   }

   if (debug) {
      printf("My phaser range: %d.\n", phrange);
      printf("My tractor range: %d.\n", trrange);
   }

   if (!nofuel) {
      myship->s_phaser.cost = 0;
      myship->s_torp.cost = 0;
      myship->s_cloakcost = 0;
   }

   if (target >= 0) {
      myship->s_imp.maxspeed = 20;
      myship->s_imp.cost = 1;
      myship->s_egncoolrate = 100;
   }
}  /* end configRobot() */


/* 
 * This function means that the robot has nothing better to do.
 * If there are hostile players in the game, it will try to get
 * as close to them as it can, while staying in its own space.
 * Otherwise, it will head to the center of its own space.
 * CRD feature: robots now hover near their start planet - MAK,  2-Jun-93
 */
void go_home(struct Enemy *ebuf) {
   int     x, y;
   double  dx, dy;
   struct  player *j;

   if (ebuf == 0) {
      /* No enemies */
      if (debug) {
         fprintf(stderr, "%d) No enemies\n", me->p_no);
      }
      if (target >= 0) {
         /* First priority, current target (if any) */
         j = &players[target];
         x = j->p_x;
         y = j->p_y;
      }
      else if (startplanet == -1) {
         /* No start planet, so go to center of galaxy */
         x = (GWIDTH / 2);
         y = (GWIDTH / 2);
      }
      else {
         /* Return to start planet */
         x = planets[startplanet].pl_x + (lrand48() % 2000) - 1000;
         y = planets[startplanet].pl_y + (lrand48() % 2000) - 1000;
      }
   }
   else {
      /* Let's get near him */
      j = &players[ebuf->e_info];
      x = j->p_x;
      y = j->p_y;

      if (startplanet != -1) {
         /* Get between enemy and planet */
         int     px, py;
         double  theta;

         px = planets[startplanet].pl_x;
         py = planets[startplanet].pl_y;
         theta = atan2((double) (y - py), (double) (x - px));
         x = px + GUARDDIST * cos(theta);
         y = py + GUARDDIST * sin(theta);
      }
   }

   if (debug) {
      fprintf(stderr, "%d) moving towards (%d/%d)\n", me->p_no, x, y);
   }

   /*
    * Note that I've decided that robots should never stop moving.
    * It makes them too easy to kill.
    */

   me->p_desdir = getcourse(x, y, me->p_x, me->p_y);

   if (angdist(me->p_desdir, me->p_dir) > 64) {
      set_speed(dogslow, 1);
   }
   else if (me->p_etemp > 900) {    /* 90% of 1000 */
      set_speed(runslow, 1);
   }
   else {
      dx = x - me->p_x;
      dy = y - me->p_y;
      set_speed((ihypot((int)dx, (int)dy) / 5000) + 3, 1);
   }
   cloak_off();
}  /* end go_home() */


/* 
 * This function is pretty self-explanitory.  The 'bot scans through all
 * of the plasma torps in the game, and phasers any hostile ones in its
 * phaser range.  It returns TRUE if it phasored a plasma, FALSE otherwise.
 */
int phaser_plasmas(void) {
   register struct plasmatorp *pt;
   register int i;
   int     myphrange = phrange;
   int     firedphaser;

   for (i = 0, pt = &plasmatorps[0]; i < MAXPLASMA * MAXPLAYER; i++, pt++) {
      if (pt->pt_status != PTMOVE)
         continue;
      if (i == me->p_no)
         continue;
      if (!(pt->pt_war & me->p_team) && !(me->p_hostile & pt->pt_team))
         continue;
      if (abs(pt->pt_x - me->p_x) > myphrange)
         continue;
      if (abs(pt->pt_y - me->p_y) > myphrange)
         continue;
      if (ihypot(pt->pt_x - me->p_x, pt->pt_y - me->p_y) > myphrange)
         continue;

      /* Phaser the plasma */
      repair_off();
      cloak_off();
      phaser(getcourse(pt->pt_x, pt->pt_y, me->p_x, me->p_y));
      firedphaser = 1;
      break;
   }

   if (firedphaser) {
      /* Phasered a plasma */
      return 1;
   }
   else {
      /* Didn't phaser */
      return 0;
   }
}  /* end phaser_plasmas() */


/* Not quite sure what this does */
int projectDamage(int eNum, int *dirP) {
   double  tdx, tdy, mdx, mdy;
   register int i, j, mx, my, tx, ty, dx, dy;
   register int numHits = 0;
   register struct torp *t;

   *dirP = 0;

   for (i = 0, t = &torps[eNum * MAXTORP]; i < MAXTORP; i++, t++) {
      if (t->t_status == TFREE) {
	    continue;
      }

      tx = t->t_x;
      ty = t->t_y;
      mx = me->p_x;
      my = me->p_y;
      tdx = (double) t->t_speed * Cos[t->t_dir] * WARP1;
      tdy = (double) t->t_speed * Sin[t->t_dir] * WARP1;
      mdx = (double) me->p_speed * Cos[me->p_dir] * WARP1;
      mdy = (double) me->p_speed * Sin[me->p_dir] * WARP1;

      for (j = t->t_fuse; j > 0; j--) {
         tx += tdx;
         ty += tdy;
         mx += mdx;
         my += mdy;
         dx = tx - mx;
         dy = ty - my;

         if (ABS(dx) < EXPDIST && ABS(dy) < EXPDIST) {
            numHits++;
            *dirP += t->t_dir;
            break;
         }
      }   /* end for */
   }   /* end for */

   if (numHits > 0) {
      *dirP /= numHits;
   }

   return (numHits);
}  /* end projectDamage() */


/* Figures out if someone is tractoring or pressing the 'bot */
int isTractoringMe(struct Enemy *enemy_buf) {
   return ((enemy_buf->e_hisflags & PFTRACT) &&
      !(enemy_buf->e_hisflags & PFPRESS) &&
      (enemy_buf->e_tractor == me->p_no));
}  /* end isTractoringMe() */


/* Finds the nearest enemy and returns him */
struct Enemy* get_nearest(void) {
   static struct Enemy ebuf;
   register struct player *j;
   register int i;
   int     pcount = 0;
   int     intruder = 0;
   int     tdist;
   int     px, py;
   int     old_shield;
   int     old_damage;
   double  dx, dy;
   double  vxa, vya, l;   /* Used for trap shooting */
   double  vxt, vyt;
   double  vxs, vys;
   double  dp;
   double  he_x, he_y, area;

   /* Find an enemy */
   ebuf.e_info = me->p_no;
   ebuf.e_dist = GWIDTH + 1;
   pcount = 0;   /* number of human players in game */

   if (target >= 0) {
      j = &players[target];

      /* Make sure we're at war */
      if (!((me->p_swar | me->p_hostile) & j->p_team)) {
         declare_war(players[target].p_team);
      }

      /* We have an enemy - get his range */
      dx = j->p_x - me->p_x;
      dy = j->p_y - me->p_y;
      tdist = ihypot((int)dx, (int)dy);

      /* ignore target if outfitting */
      if (j->p_status != POUTFIT) {
         ebuf.e_info = target;
         ebuf.e_dist = tdist;
         ebuf.e_flags &= ~(E_INTRUDER);
      }

      /* Loop to find hostile ships within tactical range */
      for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
         if ((j->p_status != PALIVE) || (j == me) ||
            ((j->p_flags & PFROBOT) && (!hostile))) {
            continue;
         }
         else {
	    /* Other players in the game */
            pcount++;
         }
         if (((j->p_swar | j->p_hostile) & me->p_team)
            || ((me->p_swar | me->p_hostile) & j->p_team)) {
            /* We have an enemy - Get his range */
            dx = j->p_x - me->p_x;
            dy = j->p_y - me->p_y;
            tdist = ihypot((int)dx, (int)dy);

            /* if target's teammate is too close, mark as nearest */
            if ((tdist < ebuf.e_dist) && (tdist < 15000)) {
               ebuf.e_info = i;
               ebuf.e_dist = tdist;
               ebuf.e_flags &= ~(E_INTRUDER);
            }
	 }
      }  /* end for */
   }
   else {
      /* no target */
      /* avoid dead slots, me, other robots (which aren't hostile) */
      for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
         if ((j->p_status != PALIVE) || (j == me) ||
            ((j->p_flags & PFROBOT) && (!hostile))) {
            continue;
         }
         else {
	    /* Other players in the game */
            pcount++;
         }

         if (((j->p_swar | j->p_hostile) & me->p_team)
            || ((me->p_swar | me->p_hostile) & j->p_team)) {
            /* We have an enemy - Get his range */
            dx = j->p_x - me->p_x;
            dy = j->p_y - me->p_y;
            tdist = ihypot((int)dx, (int)dy);

            /* Check to see if ship is near our planet. */
            if (startplanet != -1) {
               px = planets[startplanet].pl_x;
               py = planets[startplanet].pl_y;

               intruder = (ihypot(j->p_x - px, j->p_y - py) < GUARDDIST);
            }
            if (tdist < ebuf.e_dist) {
               ebuf.e_info = i;
               ebuf.e_dist = tdist;

               if (intruder) {
                  ebuf.e_flags |= E_INTRUDER;
               }
               else {
                  ebuf.e_flags &= ~(E_INTRUDER);
               }
            }
         }
      }  /* end for */
   }  /* end else */

   if (pcount == 0) {
      /* no players in game */
      return (NOENEMY);
   }
   else if (ebuf.e_info == me->p_no) {
      /* no hostile players in the game */
      return (0);
   }
   else {
      /* Get torpedo course to nearest enemy */
      j = &players[ebuf.e_info];
      ebuf.e_flags &= ~(E_TSHOT);

      vxa = (j->p_x - me->p_x);
      vya = (j->p_y - me->p_y);
      l = ihypot((int)vxa, (int)vya);  /* Normalize va */

      if (l != 0) {
         vxa /= l;
         vya /= l;
      }

      vxs = (Cos[j->p_dir] * j->p_speed) * WARP1;
      vys = (Sin[j->p_dir] * j->p_speed) * WARP1;
      dp = vxs * vxa + vys * vya;  /* Dot product of (va vs) */
      dx = vxs - dp * vxa;
      dy = vys - dp * vya;
      l = hypot(dx, dy);  /* Determine how much speed is required */
      dp = (float) (me->p_ship.s_torp.speed * WARP1);
      l = (dp * dp - l * l);

      if (l > 0) {
         /* Only shoot if within distance */
         he_x = j->p_x + Cos[j->p_dir] * j->p_speed * 50 * WARP1;
         he_y = j->p_y + Sin[j->p_dir] * j->p_speed * 50 * WARP1;
         area = 50 * me->p_ship.s_torp.speed * WARP1;

         if (ihypot(he_x - me->p_x, he_y - me->p_y) < area) {
            l = sqrt(l);
            vxt = l * vxa + dx;
            vyt = l * vya + dy;
            ebuf.e_flags |= E_TSHOT;
            ebuf.e_tcourse = getcourse((int) vxt + me->p_x, 
               (int) vyt + me->p_y, me->p_x, me->p_y);
         }
      }

      /* Get phaser shot status */
      if (ebuf.e_dist < 0.8 * phrange) {
         ebuf.e_flags |= E_PSHOT;
      }
      else {
         ebuf.e_flags &= ~(E_PSHOT);
      }

      /* Get tractor/pressor status */
      if (ebuf.e_dist < trrange) {
         ebuf.e_flags |= E_TRACT;
      }
      else {
         ebuf.e_flags &= ~(E_TRACT);
      }

      /* get his phaser range */
      ebuf.e_phrange = PHASEDIST * j->p_ship.s_phaser.damage / 100;

      /* get course info */
      ebuf.e_course = getcourse(j->p_x, j->p_y, me->p_x, me->p_y);
      ebuf.e_edir = j->p_dir;
      ebuf.e_hisflags = j->p_flags;
      ebuf.e_tractor = j->p_tractor;

      if (debug) {
         fprintf(stderr, "Set course to enemy is %d (%d)\n",
            (int)ebuf.e_course, (int) ebuf.e_course * 360 / 256);
      }

      /* check to polymorph - don't polymorph to ATT */
      if ((polymorphic) && (j->p_ship.s_type != me->p_ship.s_type) &&
         (j->p_ship.s_type != ATT)) {
         old_shield = me->p_ship.s_maxshield;
         old_damage = me->p_ship.s_maxdamage;
         getship(&(me->p_ship), j->p_ship.s_type);
         config();

         if (me->p_speed > me->p_ship.s_imp.maxspeed) {
            me->p_speed = me->p_ship.s_imp.maxspeed;
         }

         me->p_shield = (me->p_shield * (float) me->p_ship.s_maxshield)
            / (float) old_shield;
         me->p_damage = (me->p_damage * (float) me->p_ship.s_maxdamage)
            / (float) old_damage;
      }

   return (&ebuf);
   }
}  /* end get_nearest() */


/* Returns the nearest planet */
struct planet * get_nearest_planet(void) {
   register struct planet *l;
   register struct planet *nearest;
   register int i;
   int     dist = GWIDTH;	/* Manhattan distance to nearest planet */
   int     ldist;

   nearest = &planets[0];

   for (i = 0, l = &planets[i]; i < NUMPLANETS; i++, l++) {
      if ((ldist = (abs(me->p_x - l->pl_x) + abs(me->p_y - l->pl_y))) < dist) {
         dist = ldist;
         nearest = l;
      }
   }  /* end for */

   return nearest;
}  /* end get_nearest_planet() */


/* This routine sets up the robot@nowhere name. */
void set_robot_name(struct player *myself) {
   strncpy(myself->p_login, "Robot", sizeof (myself->p_login));
   myself->p_login[sizeof(myself->p_login) - 1] = '\0';
   strncpy(myself->p_monitor, "Nowhere", sizeof(myself->p_monitor));
   myself->p_monitor[sizeof(myself->p_monitor) - 1] = '\0';
}  /* end set_robot_name() */


/* 
 * This function will seek out a nearby repair planet if nearby, otherwise
 * it will just repair.
 */
int do_repair(void) {
   /* Repair if necessary (we are safe) */
   register struct planet *l;
   int     dx, dy;
   int     dist;

   l = get_nearest_planet();
   dx = abs(me->p_x - l->pl_x);
   dy = abs(me->p_y - l->pl_y);

   if (me->p_damage > 0) {
      if ((me->p_swar | me->p_hostile) & l->pl_owner) {
         if (l->pl_armies > 0) {
            if ((dx < PFIREDIST) && (dy < PFIREDIST)) {
               if (debug) {
                  fprintf(stderr, "%d) on top of hostile planet (%s)\n", 
                     me->p_no, l->pl_name);
               }

	       /* can't repair on top of hostile planets */
               return (0);
            }

            if (ihypot((int)dx, (int)dy) < PFIREDIST) {
               if (debug) {
                  fprintf(stderr, "%d) on top of hostile planet (%s)\n", 
                     me->p_no, l->pl_name);
               }

	       /* can't repair on top of hostile planets */
               return (0);
            }
         }

         me->p_desspeed = 0;
      }
      else {
         /* if friendly */
         if ((l->pl_flags & PLREPAIR) &&
            !(me->p_flags & PFORBIT)) {	
            /* oh, repair! */
            dist = ihypot((int)dx, (int)dy);

            if (debug) {
               fprintf(stderr, "%d) locking on to planet %d\n", me->p_no, 
                  l->pl_no);
            }

            cloak_off();
            shield_down();
            me->p_desdir = getcourse(l->pl_x, l->pl_y, me->p_x, me->p_y);
            lock_planet(l->pl_no);
            me->p_desspeed = 4;

            if (dist - (ORBDIST / 2) < (11500 * me->p_speed * me->p_speed) /
               me->p_ship.s_imp.dec) {
               if (me->p_desspeed > 2) {
                  set_speed(2, 1);
               }
            }

            if ((dist < ENTORBDIST) && (me->p_speed <= 2)) {
               /* orbit planet */
               me->p_flags &= ~PFPLLOCK;
               orbit();
            }

            return (1);
         }
         else {
            /* not repair, so ignore it */
            me->p_desspeed = 0;
         }
      }

      shield_down();

      if (me->p_speed == 0) {
         repair();
      }

      if (debug) {
         fprintf(stderr, "%d) repairing damage at %d\n", me->p_no, 
            me->p_damage);
      }

      return (1);
   }
   else {
      return (0);
   }
}  /* end do_repair() */


/* Sends a message to everyone */
void messAll(char *buf) {
   static char addrbuf[20];

   sprintf(addrbuf, " %s->ALL", twoletters(me));
   pmessage2(buf, 0, MALL, addrbuf, me->p_no);
}  /* end messAll() */


/* This function destroys the robot gracefully */
void exitRobot(void) {
   static char buf[80];

   r_signal(SIGALRM, SIG_IGN);

   if (me != NULL && me->p_team != ALLTEAM) {
      if (target >= 0) {
         strcpy(buf, "I'll be back.");
         messAll(buf);
      }
      else {
         sprintf(buf, "%s %s (%s) leaving the game (%.16s@%.16s)",
            ranks[me->p_stats.st_rank].name,
            me->p_name,
            twoletters(me),
            me->p_login,
            me->p_monitor);
         messAll(buf);
      }
   }

   if (configvals->robot_stats) {
      save_robot();
   }

   if (debug) {
      fprintf(stderr, "%s is exiting.  Have a nice day.\n", twoletters(me));
   }

   /* Set flags and move robot before getting rid of it. */
   me->p_status = PFREE;
   move_player(me->p_no, -1,-1, 1);

   /* 
    * Something about Terminators hangs up the slot when a human
    * tries to log in on that slot, so...
    */
   strcpy(buf, me->p_name);
   memset(me, 0, sizeof(struct player));
   strcpy(me->p_name, buf);

   /*
    * all right, so zeroing out p_stats.st_tticks has undesireable side
    * effects when the client tries to compute ratings...
    */
   me->p_stats.st_tticks = 1;
   exit(0);
}  /* end exitRobot() */


/* end robot/robot_functions.c */
