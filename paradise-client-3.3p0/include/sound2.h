/*
 * sound.h - Platform Independant Sound Support - July 1996
 * Sujal M. Patel (smpatel@umiacs.umd.edu)
 *
 *
 * Copyright (c) 1994-1996, Sujal M. Patel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      $Id: sound2.h,v 1.1 2000/01/31 21:50:37 glamm Exp $
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
