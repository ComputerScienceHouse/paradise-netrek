/*
 * sound.c - Platform Independant Sound Support - July 1996
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
 *      $Id: sound2.c,v 1.1 2000/01/06 21:26:45 ahn Exp $
 */

#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include "data.h"



static int soundfd;
static char audioOK = 1;
static char sound_flags[20]; /* Sound Flag for sound 1-19 */



void init_sound ()
{
  int i, pid, child,fd[2];
  char *argv[4];
  char filename[512];
#ifdef linux
  char *arch = "linux";
#else
#ifdef __FreeBSD__
  char *arch = "freebsd";
#else
#ifdef __NetBSD__
  char *arch = "netbsd";
#else
#ifdef sun
  char *arch = "sun";
#else
#ifdef foo
  char *arch = "foobar";
#else
  char *arch = "generic";
#endif
#endif
#endif
#endif
#endif

  signal(SIGCHLD, SIG_IGN);

  if(unixSoundPath[0] == '?')  {
      audioOK = 0;
      return;
  };

  /*  Create a pipe, set the write end to close when we exec the sound server,
      and set both (is the write end necessary?) ends to non-blocking   */
  pipe(fd);
  soundfd=fd[1];

  if( !(child=fork()) )  {
      close(fd[1]);
      dup2(fd[0],STDIN_FILENO);
      close(fd[0]);
      sprintf (filename, "paradise.sndsrv.%s", arch);
      argv[0] = filename;
      argv[1] = unixSoundPath;
      argv[2] = unixSoundDev;
      argv[3] = NULL;
      execvp(filename, argv);
      fprintf (stderr, "Couldn't Execute Sound Server (paradise.sndsrv.%s)!\n", arch);
      exit (0);
  };
  close(fd[0]);

  sleep(1);

  if (kill(child, 0))  {
      audioOK = 0;  
      close(soundfd);
  };

  for (i = 0; i < 19; i++) sound_flags[i] = 0;
} 

void play_sound (k)
int k;
{
  char c;

  c = k;
  if ((playSounds) && (audioOK)) write (soundfd, &c, sizeof (c));
}



void maybe_play_sound (k)
int k;
{
  char c;

  if (sound_flags[k] & 1) return;

  sound_flags[k] |= 1;

  c = (unsigned char)(k);
  if ((playSounds) && (audioOK)) write (soundfd, &c, sizeof (c));
}



void sound_completed (k)
int k;
{
  sound_flags[k] &= ~1;
}



void kill_sound ()
{ 
  char c;

  c = -1;               
  if ((playSounds) && (audioOK)) write (soundfd, &c, sizeof (c));
}
