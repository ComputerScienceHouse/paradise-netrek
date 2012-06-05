/*
 * robot/robot_newbie.c
 *
 * This is a robot designed for newbie servers.  It will spawn a master 
 * robot, RobotServ, which will spawn newbie robots as needed, and kill
 * them off when needed to make room for real players.
 */


#include "robot_functions.h"
#include "robot_newbie.h"


/* 
 * yes, there is a warning() in ntserv/warning.c.  However, pulling
 * in that version of warning() results in pulling in almost ALL of
 * ntserv, which would hose things up pretty good.  Since the robot isn't
 * interested in warnings from the server, we define a null function
 * here instead.
 */
void warning(char *t) { }


/* Not sure what this does */
static int rprog(char *login) {
   if (strcmp(login, "robot!") == 0) {
      return 1;
   }

   return 0;
}  /* end rprog() */


/* this function destroys the robot - may be superceded by exitRobot() */
static void obliterate(int wflag, char kreason) {
   /* 
    * 0 = do nothing to war status, 1= make war with all, 
    * 2= make peace with all
    */
   struct player *j;
   int i, k;

   /* clear torps and plasmas out */
   MZERO(torps, sizeof(struct torp) * MAXPLAYER * (MAXTORP + MAXPLASMA));

   for (j = 0; j <= MAXPLAYER; j++) {
      if (j->p_status == PFREE) {
         continue;
      }

      j->p_status = PEXPLODE;
      j->p_whydead = kreason;

      if (j->p_ship.s_type == STARBASE) {
         j->p_explode = 2 * SBEXPVIEWS ;
      }
      else {
         j->p_explode = 10 ;
      }

      j->p_ntorp = 0;
      j->p_nplasmatorp = 0;

      if (wflag == 1) {
         j->p_hostile = (FED | ROM | ORI | KLI);  /* angry */
      }
      else if (wflag == 2) {
         j->p_hostile = 0;  /* otherwise make all peaceful */
      }
   }  /* end for */
}  /* end obliterate() */


/* This saves armies that are on robots that are being ejected */
static void save_armies(struct player *p) {
   int i, k;
   char buf[80];
   char addrbuf[20];

   k=10*(remap[p->p_team]-1);

   if (k>=0 && k<=30) {
      for (i=0; i<10; i++) {
         if (planets[i+k].pl_owner==p->p_team) {
            planets[i+k].pl_armies += p->p_armies;
            sprintf(addrbuf, "%s->ALL", mastername);
            sprintf(buf, "%s's %d armies placed on %s", p->p_name,
            p->p_armies, planets[k+i].pl_name);
            pmessage(buf, 0, MALL, addrbuf);
            break;
         }
      }  /* end for */
   }
}  /* end save_armies() */


/* This function stops a robot */
static void stop_this_bot(struct player *p) {
   char buf[80];
   char addrbuf[20];

   p->p_ship.s_type = STARBASE;
   p->p_whydead=KQUIT;
   p->p_explode=10;
   p->p_status=PEXPLODE;
   p->p_whodead=0;

   sprintf(addrbuf, "%s->ALL", mastername);
   sprintf(buf, "Robot %s (%2s) was ejected to make room for a human player.",
      p->p_name, twoletters(me));
   pmessage(buf, 0, MALL, addrbuf);

   if ((p->p_status != POBSERVE) && (p->p_armies>0)) {
      save_armies(p);
   }
}  /* end stop_this_bot() */


/* finds a robot to stop and calls stop_this_bot() */         
static void stop_a_robot(void) {                              
   int i;                                                     
   struct player *j;                                          

   /* Simplistic: nuke the first robot we see. */             
   for (i = 0, j = players; i < MAXPLAYER; i++, j++) {        
      if (j->p_status == PFREE) {                             
         continue;                                            
      }                                                       

      if (j->p_flags & PFROBOT) {                             
         continue;                                            
      }                                                       

      /* If he's at the MOTD we'll get him next time. */      
      if (j->p_status == PALIVE && rprog(j->p_login)) {       
         stop_this_bot(j);                                    
         return;                                              
      }                                                       
   }  /* end for */                                                        
}   /* end stop_a_robot() */                                  


/* This function cleans things up */
static void cleanup(int unused) {
   register struct player *j;
   register int i, retry;

   do {
      /* terminate all robots */
      for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
         if ((j->p_status == PALIVE) && rprog(j->p_login)) {
            stop_this_bot(j);
         }
      }  /* end for */

      usleep(2000000);
      retry=0;

      for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
         if ((j->p_status != PFREE) && rprog(j->p_login)) {
            retry++;
         }
      }  /* end for */
   } while (retry);   /* Some robots havn't terminated yet */

   for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
      if ((j->p_status != PALIVE) || (j == me)) {
         continue;
      }

      getship(&(j->p_ship), j->p_ship.s_type);
   }  /* end for */

   obliterate(1, KPROVIDENCE);
   status->gameup = 0;
   exitRobot();
}  /* end cleanup() */


/* Not sure what this does */
static char* namearg(void) {
   register i, k = 0;
   register struct player *j;
   char *name;
   int namef = 1;

   while (1) {
      name = names[random() % NUMNAMES];
      k++;
      namef = 0;

      for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
         if (j->p_status != PFREE 
            && strncmp(name, j->p_name, strlen(name) - 1) == 0) {
            namef = 1;
            break;
         }
      }  /* end for */

      if (!namef) {
         return name;
      }

      if (k == 50) {
         return "guest";
      }
   }  /* end while */
}  /* end namearg() */


/* Starts a robot */
static void start_a_robot(char *team) {
   char command[256];

/*   sprintf(command, "%s %s %s %s -h %s -p %d -n '%s' -X robot! -b -O -i",
      RCMD, REMOTEHOST, OROBOT, team, TREKSERVER, PORT, namearg() ); */
   sprintf(command, "%s -T%s", RCMD, team);

   if (fork() == 0) {
      sigset(SIGALRM, SIG_DFL);
      execl("/bin/sh", "sh", "-c", command, 0);
      perror("newbie'execl");
      _exit(1);
   }
}  /* end start_a_robot() */


/* Figure out how many players are in the game */
static int num_players(int *next_team) {
   int i;
   struct player *j;
   int team_count[MAXTEAM+1];
   int c = 0;

   team_count[ROM] = 0;
   team_count[FED] = 0;

   for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
      if (j->p_status != PFREE && j->p_status != POBSERVE &&
         !(j->p_flags & PFROBOT)) {
         team_count[j->p_team]++;
         c++;
      }
   }  /* end for */

   /* Assign which team gets the next robot. */
   if (team_count[ROM] > team_count[FED]) {
      *next_team = FED;
   }
   else {
      *next_team = ROM;
   }

   return c;
}  /* end num_players() */


/* This function sees if the game is populated by robots only */
static int is_robots_only(void) {
   register i;
   register struct player *j;

   for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
      if (j->p_status == PFREE) {
         continue;
      }
      
      if (j->p_flags & PFROBOT) {
         continue;
      }

      if (!rprog(j->p_login)) {
         /* Found a human. */
         return 0;
      }
   }  /* end for */

   /* Didn't find any humans. */
   return 1;
}  /* end is_robots_only() */


/* 
 * This function checks the RobotServ and then adjusts the numbers of robots
 * and outputs a message periodically.
 */
void checkmess(int unused) {
   static int no_humans = 0;
   struct status game_status;
   int PKEY = 128;
   int shmemKey = PKEY;
   int i;

   sigset(SIGALRM,checkmess);
   me->p_ghostbuster = 0;         /* keep ghostbuster away */

   if (me->p_status != PALIVE) {  /* So I'm not alive now... */
      fprintf(stderr, "ERROR: RobotServ died?!\n");
      cleanup(0);   /* RobotServ is dead for some unpredicted reason like xsg */
   }

   /* make sure shared memory is still valid */
   if (shmget(shmemKey, 0, 0) < 0) {
      exit(1);
      fprintf(stderr, "ERROR: Invalid shared memory.\n");
   }

   ticks++;

    /* End the current game if no humans for 60 seconds. */
   if ((ticks % ROBOCHECK) == 0) {
      if (no_humans >= 60) {
         cleanup(0);   /* Doesn't return. */
      }

      if (is_robots_only()) {
         no_humans += ROBOCHECK / PERSEC;
      }
      else {
         no_humans = 0;
      }
   }

   /* Stop or start a robot. */
   if ((ticks % ROBOCHECK) == 0) {
      int next_team;
      int np = num_players(&next_team);

      if (game_status.wait > 0 || np > MIN_NUM_PLAYERS) {
         stop_a_robot();
      }
      else if (np < MIN_NUM_PLAYERS ) {
         if (next_team == FED) {
            start_a_robot("-Tf");
         }
         else if (next_team == ROM) {
            start_a_robot("-Tr");
         }
         else if (next_team == KLI) {
            start_a_robot("-Tk");
         }
         else {  /* Orion */
            start_a_robot("-To");
         }
      }
   }

   if ((ticks % SENDINFO) == 0) {
      /* emit a message periodically */
      messAll("Welcome to Paradise!  This is a newbie server");
      messAll("Go to http://paradise.netrek.org for more info");
   }
}   /* end checkmess() */


/* Not sure what this does */
static void reaper(int sig) {
   int stat=0;
   static int pid;

   while ((pid = WAIT3(&stat, WNOHANG, 0)) > 0);

   sigset(SIGCHLD,reaper);
}  /* end reaper() */


/* This is where the magic happens - Praise Bob! */
int main(int argc, char *argv[]) {
   enum HomeAway homeaway = NEITHER;
   int oldmctl;
   int overload = 0;
   int team = 4;
   int pno;
   int class;
   int i;

   srandom(time(NULL));

   sigset(SIGCHLD, reaper);

   openmem(1, 0);

   sigset(SIGALRM, checkmess);

   if (!debug) {
      sigset(SIGINT, cleanup);
   }

   class = ATT;
   target = -1;   /* no targeted player */

   if ((pno = findrslot()) < 0) {
      fprintf(stderr, "Unable to get a slot");
      exit(0);
   }

   me = &players[pno];
   myship = &me->p_ship;
   mystats = &me->p_stats;
   lastm = mctl->mc_current;

   /* set the robot@nowhere fields */
   set_robot_name(me);

   /* Enter the game */
   enter(team, 0, pno, class, -1);

   me->p_pos = -1;           /* So robot stats don't get saved */
   me->p_flags |= PFROBOT;   /* Mark as a robot */
   me->p_x = GWIDTH/2;       /* displace to on overlooking position */
   me->p_y = GWIDTH/2;       /* maybe we should just make it fight? */
   me->p_hostile = 0;
   me->p_swar = 0;
   me->p_team = 0;           /* independent */

   oldmctl = mctl->mc_current;

   status->gameup = 1;

   /* allows robots to be forked by the daemon on some systems */
   {
      sigset_t unblock_everything;
      sigfillset(&unblock_everything);
      sigprocmask(SIG_UNBLOCK, &unblock_everything, NULL);
   }

   me->p_status = PALIVE;   /* Put robot in game */

   while (1) {
      sigpause(SIGALRM);
   }
}


/* end robot/robot_newbie.c */
