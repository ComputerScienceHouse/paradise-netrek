/*
 * include/robot_functions.h
 *
 * This is the header file for robot/robot_functions.c
 */

#ifndef ROBOT_FUNCTIONS_H
#define ROBOT_FUNCTIONS_H

/* System includes that we need */
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Netrek includes that we need */
#include "config.h"
#include "data.h"
#include "defs.h"
#include "ntserv.h"
#include "proto.h"
#include "shmem.h"
#include "weapons.h"

/* This is for go_home() */
#define GUARDDIST 8000

/* For get_nearest() */
#define E_INTRUDER 0x01
#define E_PSHOT    0x04
#define E_TRACT    0x08
#define E_TSHOT    0x02
#define NOENEMY    (struct Enemy *) -1


/* Robot flags */
int debug;
int hostile;
int nofuel;
int polymorphic;
/*
int berserk;
int fleet;
int level;
int practice;
int sticky;
*/

/* Robot variables */
int phrange;
int trrange;
int startplanet;
int target;

/* Robot speeds */
int dogslow;
int dogfast;
int runslow;
int runfast;
int closeslow;
int closefast;


/* The enemy */
struct Enemy  {
   int     e_info;
   int     e_dist;
   unsigned char e_course;     /* course to enemy */
   unsigned char e_edir;       /* enemy's current heading */
   unsigned char e_tcourse;    /* torpedo intercept course to enemy */
   unsigned int e_flags;
   int     e_tractor;          /* who he's tractoring/pressoring */
   int     e_phrange;          /* his phaser range */
   unsigned int e_hisflags;    /* his pflags. bug fix: 6/24/92 TC */
};

/* This function logs the robot in */
void do_robot_login(void);

/* This function saves the robot's stats */
void save_robot(void);

/* Calculates class-specific stuff */
void config(void);

/* 
 * This function means that the robot has nothing better to do.
 * If there are hostile players in the game, it will try to get
 * as close to them as it can, while staying in its own space.
 * Otherwise, it will head to the center of its own space.
 * CRD feature: robots now hover near their start planet - MAK,  2-Jun-93
 */
void go_home(struct Enemy*);

/* 
 * This function is pretty self-explanitory.  The 'bot scans through all
 * of the plasma torps in the game, and phasers any hostile ones in its
 * phaser range.  It returns TRUE if it phasored a plasma, FALSE otherwise.
 */
int phaser_plasmas(void);

/* Not quite sure what this does */
int projectDamage(int, int*);

/* Figures out if someone is tractoring or pressing the 'bot */
int isTractoringMe(struct Enemy*);

/* Finds the nearest enemy and returns him */
struct Enemy* get_nearest(void);

/* Returns the nearest planet */
struct planet * get_nearest_planet(void);

/* This routine sets up the robot@nowhere name. */
void set_robot_name(struct player*);

/* 
 * This function will seek out a nearby repair planet if nearby, otherwise
 * it will just repair.
 */
int do_repair(void);

/* Sends a message to everyone */
void messAll(char*);

/* This function destroys the robot gracefully */
void exitRobot(void);


#endif  /* #ifndef ROBOT_FUNCTIONS_H */

/* end include/robot_functions.h */
