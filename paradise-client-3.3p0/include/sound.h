/*
 * sound.h - Platform Independant Sound Support - Dec. 1994
 *
 * Copyright 1994 Sujal M. Patel (smpatel@wam.umd.edu)
 * Conditions in "copyright.h"          
 */

#define SND_EXPLOSION 0
#define SND_CLOAK     1
#define SND_FIRETORP  2
#define SND_PHASER    3
#define SND_PLASMA    4
#define SND_SHIELD    5
#define SND_TORPHIT   6
#define SND_EXP_SB    7
#define SND_PARADISE  8
#define SND_THERMAL   9
#define SND_REDALERT  10

#ifdef __STDC__
void init_sound ();             /* Init Sound System                          */
void play_sound (int k);        /* Play a Sound                               */
void maybe_play_sound (int k);  /* Play sound if the last 'k' sound_completed */
void sound_completed (int k);   /* Complete a sound 'k'                       */
void kill_sound ();             /* Terminate a sound unpredictably :)         */
#endif
/*
 * sound.h - Platform Independant Sound Support - Dec. 1994
 *
 * Copyright 1994 Sujal M. Patel (smpatel@wam.umd.edu)
 * Conditions in "copyright.h"          
 */

#define SND_EXPLOSION 0
#define SND_CLOAK     1
#define SND_FIRETORP  2
#define SND_PHASER    3
#define SND_PLASMA    4
#define SND_SHIELD    5
#define SND_TORPHIT   6
#define SND_EXP_SB    7
#define SND_PARADISE  8
#define SND_THERMAL   9
#define SND_REDALERT  10

#ifdef __STDC__
void init_sound ();             /* Init Sound System                          */
void play_sound (int k);        /* Play a Sound                               */
void maybe_play_sound (int k);  /* Play sound if the last 'k' sound_completed */
void sound_completed (int k);   /* Complete a sound 'k'                       */
void kill_sound ();             /* Terminate a sound unpredictably :)         */
#endif
/*
 * sound.h - Platform Independant Sound Support - Dec. 1994
 *
 * Copyright 1994 Sujal M. Patel (smpatel@wam.umd.edu)
 * Conditions in "copyright.h"          
 */

#define SND_EXPLOSION 0
#define SND_CLOAK     1
#define SND_FIRETORP  2
#define SND_PHASER    3
#define SND_PLASMA    4
#define SND_SHIELD    5
#define SND_TORPHIT   6
#define SND_EXP_SB    7
#define SND_PARADISE  8
#define SND_THERMAL   9
#define SND_REDALERT  10

#ifdef __STDC__
void init_sound ();             /* Init Sound System                          */
void play_sound (int k);        /* Play a Sound                               */
void maybe_play_sound (int k);  /* Play sound if the last 'k' sound_completed */
void sound_completed (int k);   /* Complete a sound 'k'                       */
void kill_sound ();             /* Terminate a sound unpredictably :)         */
#endif
