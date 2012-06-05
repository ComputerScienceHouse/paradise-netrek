/* 
 * include/robot_newbie.h
 *
 * This is the header file for robot/robot_newbie.c
 *
 */


#ifndef ROBOT_NEWBIE_H
#define ROBOT_NEWBIE_H

#include "robot_functions.h"
#include "defs.h"


/* This is RobotServ's heartbeat - used by checkmess() */
static int ticks = 0;

/* Here are some names */
char* mastername = "RobotServ";
static char* team_s[4] = { "Federation", "Romulan", "Klingon", "Orion" };

#define NUMADJ 12
static char* adj_s[NUMADJ] = {
   "VICIOUS", "RUTHLESS", "IRONFISTED", "RELENTLESS",
   "MERCILESS", "UNFLINCHING", "FEARLESS", "BLOODTHIRSTY",
   "FURIOUS", "DESPERATE", "FRENZIED", "RABID"
};

#define NUMNAMES 20
static char* names[NUMNAMES] = {
   "Annihilator", "Banisher", "Blaster",
   "Demolisher", "Destroyer", "Eliminator",
   "Eradicator", "Exiler", "Obliterator",
   "Razer", "Demoralizer", "Smasher",
   "Shredder", "Vanquisher", "Wrecker",
   "Ravager", "Despoiler", "Abolisher",
   "Emasculator", "Decimator"
};


/* System dependend setups */
#define MIN_NUM_PLAYERS	(MAXPLAYER - 1) /* How many players to maintain. */
#define OROBOT          ROBODIR("/robot")
#define PORT		2592
#define RCMD            ROBOT
#define REMOTEHOST      "localhost"
#define TREKSERVER      "localhost"


/* Functions */
#define HOWOFTEN 1                       /*Robot moves every HOWOFTEN cycles*/
#define PERSEC (1000000/UPDATE/HOWOFTEN) /* # of robo calls per second*/
#define ROBOCHECK (10*PERSEC)            /* start or stop a robot */
#define SENDINFO  (120*PERSEC)           /* send info to all */

/*
 * Strange #defines from Vanilla needed to make RobotServ work -- they should
 * be replaced with the Paradise equivalent when there is time
 */
#define MZERO(b1,l)     bzero(b1,l)
#define WAIT3(x,y,z)    waitpid(-1, x, y )

#endif  /* #ifndef ROBOT_NEWBIE_H */


/* end include/robot_newbie.h */
