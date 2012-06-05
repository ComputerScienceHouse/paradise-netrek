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

#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "config.h"
#include "proto.h"

#include "shmem.h"

#define SHMEM_C
#include "data.h"
#undef SHMEM_C

/*---------------------------SHARED MEMORY STRUCTURE----------------------*/

#include "shmemP.h"

/*-------------------------------------------------------------------------*/

struct player *players;
struct torp *torps;
struct missile *missiles;
struct thingy *thingies;
struct plasmatorp *plasmatorps;
struct status *status;
struct status2 *status2;
struct planet *planets;
struct t_unit *terrain_grid;
struct phaser *phasers;
int    *stars;
struct mctl *mctl;
struct message *messages;
struct team *teams;

struct ship *shipvals;
struct configuration *configvals;
int    *shipsallowed;		/* points inside configvals */
int    *weaponsallowed;		/* ditto */
int    *sun_effect;
int    *ast_effect;
int    *neb_effect;
int    *wh_effect;
int    *num_nebula;
int    *nebula_density;
int    *nebula_subclouds;
int    *num_asteroid;
float  *asteroid_thickness;
int    *asteroid_density;
int    *asteroid_radius;
float  *asteroid_thick_variance;
int    *asteroid_dens_variance;
int    *asteroid_rad_variance;
int    *improved_tracking;
int    *tstart;
int    *newgalaxy;
int    *nontteamlock;
char	*cluephrase_storage;

static int PKEY = 128;

/*------------------------------STARTDAEMON---------------------------------*/
/*  This function starts the daemon.  It get the path to where the daemon
resides, then tries to start it up.  */

void 
startdaemon(int leagueflag, int restart)
{
    int     i;			/* temp var */
    char   *hell;		/* to hold path to daemon */

    if (restart) {
	fprintf(stderr, "RE-");
	kill(status->nukegame, SIGKILL);	/* daemon is killed without
						   giving it a chance to
						   blast the shmem */
	sleep(2);
    }
    fprintf(stderr, "Starting daemon...\n");	/* record that deaemon is */
    i = fork();			/* starting */
    if (i == 0) {		/* if successful */
	char   *nargv[10];
	int     argc = 0;
	hell = build_path(DAEMON);
	nargv[argc++] = "daemon";
	if (leagueflag)
	    nargv[argc++] = "-l";
	if (restart)
	    nargv[argc++] = "-a";
	nargv[argc] = 0;
	execv(hell, nargv);

	perror(hell);		/* did it work */
	fprintf(stderr, "Couldn't start daemon!!!\n");
	_exit(1);		/* let's get out of here */
    }
}



static int shmid = -1;		/* ID number of shared mem */


/*--------------------------------OPENMEM----------------------------------*/
/* if code is 0, openmem will exit(1) upon finding the memory gone.
   if code is 1, openmem will fork a netrek daemon and retry.
   if code is 2, openmem will blast any already existing shared memory.
   */

void 
openmem(int code, int leagueflag)
{
    extern int errno;		/* to get error number */
    key_t   shmemKey = PKEY;	/* shared memory's key */
    struct memory *sharedMemory;/* to point to shared memory */
    char   *k;

    k = getenv("TREKSHMKEY");	/* grab shared mem id environment variable */
    if (k) {			/* if it's set... */
	int     i;
	if (sscanf(k, "%d", &i) > 0 && i > 0)	/* ...grab its value... */
	    shmemKey = i;	/* ...and use it */
    }

    errno = 0;			/* clear error number */
    shmid = shmget(shmemKey, 0, 0);	/* get id of shared mem */
    if (code == 2) {
	if (shmid >= 0) {
	    /* If we're in daemon mode and we opened the memory */
	    /* "There can be only one". */
	    fprintf(stderr, "Killing existing segment\n");
	    shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0);
	    /* create a new one */
	}
	shmid = shmget(shmemKey, sizeof(struct memory), IPC_CREAT | 0666);
    }
    if (shmid < 0) {		/* if shared mem does not exist */
	switch (code) {
	case 0:
	    if (errno != ENOENT) {	/* catastrophic, sell all your stock */
		perror("shmget");	/* and leave the country type of */
		exit(1);	/* error--suicide */
	    }
	    fprintf(stderr, "Daemon not running (err:%d)\n", errno);
	    exit(1);
	case 1:
	    if (errno != ENOENT) {	/* catastrophic, sell all your stock */
		perror("shmget");	/* and leave the country type of */
		exit(1);	/* error--suicide */
	    }
	    startdaemon(leagueflag, 0);	/* try to start the daemon */
	    sleep(2);		/* wait for a sec for deamon to start */
	    shmid = shmget(shmemKey, 0, 0);	/* try again to get shared
						   mem */
	    if (shmid < 0) {	/* failure again then get out of here */
		fprintf(stderr, "Daemon not running (err:%d)\n", errno);
		exit(1);
	    }
	    break;
	case 2:
	    perror("daemon: can't open shared memory");
	    exit(1);
	}
    }
    if (code == 2) {
	struct shmid_ds smbuf;

	shmctl(shmid, IPC_STAT, &smbuf);	/* Hose Ed's robots */
	smbuf.shm_perm.uid = geteuid();
	smbuf.shm_perm.mode = 0666;
	shmctl(shmid, IPC_SET, &smbuf);
    }
    sharedMemory = (struct memory *) shmat(shmid, (char *) 0, 0);
    if (sharedMemory == (struct memory *)(-1)) {
	perror("shared memory");
	exit(1);
    }
    if (code == 2) {
	/* set the magic number */
	sharedMemory->shmem_size = sizeof(struct memory);
    }
    else {
	if (sharedMemory->shmem_size != sizeof(struct memory)) {
	    fprintf(stderr, "shared memory segment magic number mismatch!\
 (%d != %ld)\n  aborting to prevent corruption of game data\n",
		    sharedMemory->shmem_size, (long) sizeof(struct memory));
	    exit(1);
	}
    }
    players = sharedMemory->players;	/* set pointer to fields */
    torps = sharedMemory->torps;/* in shared memory structure */
    missiles = sharedMemory->missiles;	/* for easy access */
    thingies = sharedMemory->thingies;
    plasmatorps = sharedMemory->plasmatorps;
    status = &sharedMemory->status;
    status2 = &sharedMemory->status2;
    planets = sharedMemory->planets;
    terrain_grid = sharedMemory->terrain_grid;
    phasers = sharedMemory->phasers;
    stars = sharedMemory->stars;
    mctl = &sharedMemory->mctl;
    messages = sharedMemory->messages;
    teams = sharedMemory->teams;
    shipvals = sharedMemory->shipvals;
    configvals = &sharedMemory->configvals;
    galaxyValid = (char *) &sharedMemory->galaxyValid;
/*    configvals->gwidth = 200000; *//* pick up from galaxy generator later */
/*    configvals->numplanets = 60; *//* this too */

    shipsallowed = configvals->shipsallowed;
    weaponsallowed = configvals->weaponsallowed;
    sun_effect = configvals->sun_effect;
    ast_effect = configvals->ast_effect;
    neb_effect = configvals->neb_effect;
    wh_effect = configvals->wh_effect;
    num_nebula = &(configvals->num_nebula);
    nebula_density = &(configvals->nebula_density);
    nebula_subclouds = &(configvals->nebula_subclouds);
    num_asteroid = &(configvals->num_asteroid);
    asteroid_thickness = &(configvals->asteroid_thickness);
    asteroid_density = &(configvals->asteroid_density);
    asteroid_radius = &(configvals->asteroid_radius);
    asteroid_thick_variance = &(configvals->asteroid_thick_variance);
    asteroid_dens_variance = &(configvals->asteroid_dens_variance);
    asteroid_rad_variance = &(configvals->asteroid_rad_variance);
    improved_tracking = configvals->improved_tracking;

    cluephrase_storage = sharedMemory->cluephrase_storage;

    stars[1] = -1;
}

void 
blast_shmem(void)
{
    shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0);
}
