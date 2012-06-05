/*
 * paradise.sndsrv.c - VoxWare(tm) Compatible Sound - Dec. 1994 
 *                     PC Speaker  Compatible Sound 
 *                     This server is Linux Specific.
 *
 * Copyright 1994-1995 Sujal M. Patel (smpatel@wam.umd.edu)
 * Conditions in "copyright.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "linux_soundcard.h" /* /usr/include/linux/soundcard.h */
#include "linux_pcsp.h"      /* /usr/include/linux/pcsp.h      */
#include <sys/time.h>
#include <signal.h>
#include <string.h>



char *FILENAME[] = {
                     "/explode.raw",
                     "/cloak.raw",
                     "/firetorp.raw",
                     "/phaser.raw",
                     "/plasma.raw",
                     "/shield.raw",
                     "/torphit.raw",
                     "/explode_big.raw",
                     "/paradise.raw",
                     "/thermal.raw",
                     "/redalert.raw"
                   };



/* Terminate: Signal Handler */
void quit ()
{
  exit (0);
}



void init (int argc, char **argv)
{
  int i;
  char s[1024];

  if (argc != 2)
  {
    printf ("This program is only executed by netrek.paradise\n");
    exit (1);
  }

  sleep (1); /* Wait for the Client to Setup the Named Pipe */

  for (i=0; i < (sizeof (FILENAME) / sizeof (char *)); i++)
  {
    s[0] = 0;
    strcat (s, argv[1]);
    if (s[(int)strlen(s) - 1] == '/') FILENAME[i]++;
    strcat (s, FILENAME[i]);
    FILENAME[i] = malloc ((int)strlen (s));
    strcpy (FILENAME[i],s);
  }

  signal(SIGTERM, quit);   /* Setup Terminate Signal Handler */
}



/*
   Setup DSP: Opens /dev/dsp or /dev/pcdsp
              Sets fragment size on VoxWare
              Sets speed to 8000hz
              Should set mono mode
              Error checking                
*/
int setup_dsp (int *fragsize, int *is_pcsp)
{
  int dsp, frag, value;

  dsp = open ("/dev/dsp", O_RDWR);

  if (dsp < 1) dsp = open ("/dev/pcdsp", O_RDWR);
  if (dsp < 1) dsp = open ("/dev/pcsp",  O_RDWR);

  if (dsp < 1)
  {
    fprintf (stderr, "paradise.sndsrv: Couldn't open DSP\n");
    return -1;
  }
 
  *is_pcsp = 0;
  *fragsize = 0;
  frag = 0x00020007;

  if (!ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &frag)) ioctl(dsp, SNDCTL_DSP_GETBLKSIZE, fragsize);

  if (!(*fragsize))
  { 
    /* Don't Assume just because you can't set the fragment, use proper IOCTL */
    fprintf (stderr, "paradise.sndsrv: Couldn't set Fragment Size.\nAssuming PC Speaker!\n");
    *fragsize = 128;
    *is_pcsp = 1;
  }
  
/* 
  *** Should Set to MONO mode, but I can't test this, so..... ***

  value = 0;
  if (dsp) printf ("%d\n", ioctl (dsp, SNDCTL_DSP_STEREO, &value));
*/   
 
  value = 8000;
  if (dsp)
  {
    if (ioctl (dsp, SNDCTL_DSP_SPEED, &value))
    {
      fprintf (stderr, "paradise.sndsrv: Couldn't set DSP rate!\n");
    }
  }

  return dsp;
}



/*
   This just keeps the pipe from breaking...
   Eventually I'll look at the paradise signal handlers and
   just trap this.
*/
int do_nothing (int in)
{
  char buffer[64];

  while (1)
  { 
    read (in, buffer, sizeof (buffer)); /* Make sure this pipe doesn't break */
    sleep (5);                          /* Clean this Crap up!!              */
  } 
}



void do_everything (int dsp, int in, int fragsize, 
                    int is_pcsp, int ppid)
{
  char k;
  int i, j, fd;
  int terminate = -1;             /* Which Sound to Terminate                              */
  int playing[16];                /* Sound numbers that we are playing                     */
  int playnum = 0;                /* Number of sounds currently being played               */
  unsigned char buffer[16][256];  /* Buffer for each sound, MUST be at least Fragsize long */
  unsigned char final[256];       /* Final Mixing Buffer                                   */

  for (;;)
  {
    /* Try to open a new sound if we get an integer on the 'in' pipe */
    if ((read (in, &k, sizeof (k)) > 0) && (playnum < 16))
    {
      /* Negative means terminate the FIRST sound in the buffer */
      if (k < 0) terminate = 0;
      else
      {
        fd = open (FILENAME[(int)k], O_RDONLY);
        if (fd > 0) playing[playnum++] = fd;
#ifdef DEBUG
        else fprintf (stderr, "paradise.sndsrv: The sound %s could not be opened\n", FILENAME[k]);
#endif
      }
    }
#ifdef DEBUG
    else if (playnum >= 16) fprintf (stderr, "paradise.sndsrv: Too many sounds to play!\n");
#endif

    /* Kill the parent with signal zero to see if it's still alive */
    if (kill (ppid, 0)) { kill (getpid(), SIGTERM); } /* Is this an Expensive operation?? */

    /* Of all the active sounds, read the next FRAGSIZE bytes */
    for (i=0; i < playnum; i++)
    {
      j = read (playing[i], buffer[i], fragsize);
      if ((j == 0) || (i == terminate))
      {
        /* Close Sounds that are finished */
        close (playing[i]);
        playnum--;
        for (j = i; j < playnum; j++)
          memcpy (&playing[j], &playing[j+1], sizeof (int)); /* Use a better data structure? */
        terminate = -1;
      }
      /* Silence the rest of the array (to fill up to FRAGSIZE) */
      else for (;j < fragsize; j++) buffer[i][j] = 128;
    }

    if (playnum)
    {
      /* Mix each sound into the final buffer */
      for (i=0; i < fragsize; i++)
      {
        final [i] = 128;
        for (j=0; j < playnum; j++)
        {
           final[i] += (buffer[j][i] - 128);
        }
      }
      /*
         The sound server is in a tight loop, EXCEPT for this
         write which blocks.  Any optimizations in the above
         code would really be helpful.  Right now the server
         takes up to 7% cpu on a 486DX/50.
      */
      write (dsp, final, fragsize);
    }
    else
    {
      /* 
         We have no sounds to play
         Just fill the buffer with silence and maybe play it 
      */
      for (j = 0; j < fragsize; j++) final [j] = 128; /* Avoid Pop on Audio Devices  */
      if (!is_pcsp) write (dsp, final, fragsize);     /* Avoid Static on PC Speaker  */
      else          usleep (8000);                    /* Sleep since we didn't write */
    }
  }
}



void main (argc, argv)
int argc;
char **argv;
{
  int dsp, in, is_pcsp, fragsize, ppid;
  char filename[512];

  init (argc, argv);
  dsp = setup_dsp (&fragsize, &is_pcsp);
  ppid = getppid();

  sprintf (filename, "/tmp/paradise.sound.fifo.pid.%d", ppid);
  in = open (filename, O_RDONLY);
  fcntl (in, F_SETFL, O_NONBLOCK);

  if (!dsp) do_nothing (in);

  do_everything (dsp, in, fragsize, is_pcsp, ppid);
}
/*
 * paradise.sndsrv.c - VoxWare(tm) Compatible Sound - Dec. 1994 
 *                     PC Speaker  Compatible Sound 
 *                     This server is Linux Specific.
 *
 * Copyright 1994-1995 Sujal M. Patel (smpatel@wam.umd.edu)
 * Conditions in "copyright.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "linux_soundcard.h" /* /usr/include/linux/soundcard.h */
#include "linux_pcsp.h"      /* /usr/include/linux/pcsp.h      */
#include <sys/time.h>
#include <signal.h>
#include <string.h>



char *FILENAME[] = {
                     "/explode.raw",
                     "/cloak.raw",
                     "/firetorp.raw",
                     "/phaser.raw",
                     "/plasma.raw",
                     "/shield.raw",
                     "/torphit.raw",
                     "/explode_big.raw",
                     "/paradise.raw",
                     "/thermal.raw",
                     "/redalert.raw"
                   };



/* Terminate: Signal Handler */
void quit ()
{
  exit (0);
}



void init (int argc, char **argv)
{
  int i;
  char s[1024];

  if (argc != 2)
  {
    printf ("This program is only executed by netrek.paradise\n");
    exit (1);
  }

  sleep (1); /* Wait for the Client to Setup the Named Pipe */

  for (i=0; i < (sizeof (FILENAME) / sizeof (char *)); i++)
  {
    s[0] = 0;
    strcat (s, argv[1]);
    if (s[(int)strlen(s) - 1] == '/') FILENAME[i]++;
    strcat (s, FILENAME[i]);
    FILENAME[i] = malloc ((int)strlen (s));
    strcpy (FILENAME[i],s);
  }

  signal(SIGTERM, quit);   /* Setup Terminate Signal Handler */
}



/*
   Setup DSP: Opens /dev/dsp or /dev/pcdsp
              Sets fragment size on VoxWare
              Sets speed to 8000hz
              Should set mono mode
              Error checking                
*/
int setup_dsp (int *fragsize, int *is_pcsp)
{
  int dsp, frag, value;

  dsp = open ("/dev/dsp", O_RDWR);

  if (dsp < 1) dsp = open ("/dev/pcdsp", O_RDWR);
  if (dsp < 1) dsp = open ("/dev/pcsp",  O_RDWR);

  if (dsp < 1)
  {
    fprintf (stderr, "paradise.sndsrv: Couldn't open DSP\n");
    return -1;
  }
 
  *is_pcsp = 0;
  *fragsize = 0;
  frag = 0x00020007;

  if (!ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &frag)) ioctl(dsp, SNDCTL_DSP_GETBLKSIZE, fragsize);

  if (!(*fragsize))
  { 
    /* Don't Assume just because you can't set the fragment, use proper IOCTL */
    fprintf (stderr, "paradise.sndsrv: Couldn't set Fragment Size.\nAssuming PC Speaker!\n");
    *fragsize = 128;
    *is_pcsp = 1;
  }
  
/* 
  *** Should Set to MONO mode, but I can't test this, so..... ***

  value = 0;
  if (dsp) printf ("%d\n", ioctl (dsp, SNDCTL_DSP_STEREO, &value));
*/   
 
  value = 8000;
  if (dsp)
  {
    if (ioctl (dsp, SNDCTL_DSP_SPEED, &value))
    {
      fprintf (stderr, "paradise.sndsrv: Couldn't set DSP rate!\n");
    }
  }

  return dsp;
}



/*
   This just keeps the pipe from breaking...
   Eventually I'll look at the paradise signal handlers and
   just trap this.
*/
int do_nothing (int in)
{
  char buffer[64];

  while (1)
  { 
    read (in, buffer, sizeof (buffer)); /* Make sure this pipe doesn't break */
    sleep (5);                          /* Clean this Crap up!!              */
  } 
}



void do_everything (int dsp, int in, int fragsize, 
                    int is_pcsp, int ppid)
{
  char k;
  int i, j, fd;
  int terminate = -1;             /* Which Sound to Terminate                              */
  int playing[16];                /* Sound numbers that we are playing                     */
  int playnum = 0;                /* Number of sounds currently being played               */
  unsigned char buffer[16][256];  /* Buffer for each sound, MUST be at least Fragsize long */
  unsigned char final[256];       /* Final Mixing Buffer                                   */

  for (;;)
  {
    /* Try to open a new sound if we get an integer on the 'in' pipe */
    if ((read (in, &k, sizeof (k)) > 0) && (playnum < 16))
    {
      /* Negative means terminate the FIRST sound in the buffer */
      if (k < 0) terminate = 0;
      else
      {
        fd = open (FILENAME[(int)k], O_RDONLY);
        if (fd > 0) playing[playnum++] = fd;
#ifdef DEBUG
        else fprintf (stderr, "paradise.sndsrv: The sound %s could not be opened\n", FILENAME[k]);
#endif
      }
    }
#ifdef DEBUG
    else if (playnum >= 16) fprintf (stderr, "paradise.sndsrv: Too many sounds to play!\n");
#endif

    /* Kill the parent with signal zero to see if it's still alive */
    if (kill (ppid, 0)) { kill (getpid(), SIGTERM); } /* Is this an Expensive operation?? */

    /* Of all the active sounds, read the next FRAGSIZE bytes */
    for (i=0; i < playnum; i++)
    {
      j = read (playing[i], buffer[i], fragsize);
      if ((j == 0) || (i == terminate))
      {
        /* Close Sounds that are finished */
        close (playing[i]);
        playnum--;
        for (j = i; j < playnum; j++)
          memcpy (&playing[j], &playing[j+1], sizeof (int)); /* Use a better data structure? */
        terminate = -1;
      }
      /* Silence the rest of the array (to fill up to FRAGSIZE) */
      else for (;j < fragsize; j++) buffer[i][j] = 128;
    }

    if (playnum)
    {
      /* Mix each sound into the final buffer */
      for (i=0; i < fragsize; i++)
      {
        final [i] = 128;
        for (j=0; j < playnum; j++)
        {
           final[i] += (buffer[j][i] - 128);
        }
      }
      /*
         The sound server is in a tight loop, EXCEPT for this
         write which blocks.  Any optimizations in the above
         code would really be helpful.  Right now the server
         takes up to 7% cpu on a 486DX/50.
      */
      write (dsp, final, fragsize);
    }
    else
    {
      /* 
         We have no sounds to play
         Just fill the buffer with silence and maybe play it 
      */
      for (j = 0; j < fragsize; j++) final [j] = 128; /* Avoid Pop on Audio Devices  */
      if (!is_pcsp) write (dsp, final, fragsize);     /* Avoid Static on PC Speaker  */
      else          usleep (8000);                    /* Sleep since we didn't write */
    }
  }
}



void main (argc, argv)
int argc;
char **argv;
{
  int dsp, in, is_pcsp, fragsize, ppid;
  char filename[512];

  init (argc, argv);
  dsp = setup_dsp (&fragsize, &is_pcsp);
  ppid = getppid();

  sprintf (filename, "/tmp/paradise.sound.fifo.pid.%d", ppid);
  in = open (filename, O_RDONLY);
  fcntl (in, F_SETFL, O_NONBLOCK);

  if (!dsp) do_nothing (in);

  do_everything (dsp, in, fragsize, is_pcsp, ppid);
}
/*
 * paradise.sndsrv.c - VoxWare(tm) Compatible Sound - Dec. 1994 
 *                     PC Speaker  Compatible Sound 
 *                     This server is Linux Specific.
 *
 * Copyright 1994-1995 Sujal M. Patel (smpatel@wam.umd.edu)
 * Conditions in "copyright.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "linux_soundcard.h" /* /usr/include/linux/soundcard.h */
#include "linux_pcsp.h"      /* /usr/include/linux/pcsp.h      */
#include <sys/time.h>
#include <signal.h>
#include <string.h>



char *FILENAME[] = {
                     "/explode.raw",
                     "/cloak.raw",
                     "/firetorp.raw",
                     "/phaser.raw",
                     "/plasma.raw",
                     "/shield.raw",
                     "/torphit.raw",
                     "/explode_big.raw",
                     "/paradise.raw",
                     "/thermal.raw",
                     "/redalert.raw"
                   };



/* Terminate: Signal Handler */
void quit ()
{
  exit (0);
}



void init (int argc, char **argv)
{
  int i;
  char s[1024];

  if (argc != 2)
  {
    printf ("This program is only executed by netrek.paradise\n");
    exit (1);
  }

  sleep (1); /* Wait for the Client to Setup the Named Pipe */

  for (i=0; i < (sizeof (FILENAME) / sizeof (char *)); i++)
  {
    s[0] = 0;
    strcat (s, argv[1]);
    if (s[(int)strlen(s) - 1] == '/') FILENAME[i]++;
    strcat (s, FILENAME[i]);
    FILENAME[i] = malloc ((int)strlen (s));
    strcpy (FILENAME[i],s);
  }

  signal(SIGTERM, quit);   /* Setup Terminate Signal Handler */
}



/*
   Setup DSP: Opens /dev/dsp or /dev/pcdsp
              Sets fragment size on VoxWare
              Sets speed to 8000hz
              Should set mono mode
              Error checking                
*/
int setup_dsp (int *fragsize, int *is_pcsp)
{
  int dsp, frag, value;

  dsp = open ("/dev/dsp", O_RDWR);

  if (dsp < 1) dsp = open ("/dev/pcdsp", O_RDWR);
  if (dsp < 1) dsp = open ("/dev/pcsp",  O_RDWR);

  if (dsp < 1)
  {
    fprintf (stderr, "paradise.sndsrv: Couldn't open DSP\n");
    return -1;
  }
 
  *is_pcsp = 0;
  *fragsize = 0;
  frag = 0x00020007;

  if (!ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &frag)) ioctl(dsp, SNDCTL_DSP_GETBLKSIZE, fragsize);

  if (!(*fragsize))
  { 
    /* Don't Assume just because you can't set the fragment, use proper IOCTL */
    fprintf (stderr, "paradise.sndsrv: Couldn't set Fragment Size.\nAssuming PC Speaker!\n");
    *fragsize = 128;
    *is_pcsp = 1;
  }
  
/* 
  *** Should Set to MONO mode, but I can't test this, so..... ***

  value = 0;
  if (dsp) printf ("%d\n", ioctl (dsp, SNDCTL_DSP_STEREO, &value));
*/   
 
  value = 8000;
  if (dsp)
  {
    if (ioctl (dsp, SNDCTL_DSP_SPEED, &value))
    {
      fprintf (stderr, "paradise.sndsrv: Couldn't set DSP rate!\n");
    }
  }

  return dsp;
}



/*
   This just keeps the pipe from breaking...
   Eventually I'll look at the paradise signal handlers and
   just trap this.
*/
int do_nothing (int in)
{
  char buffer[64];

  while (1)
  { 
    read (in, buffer, sizeof (buffer)); /* Make sure this pipe doesn't break */
    sleep (5);                          /* Clean this Crap up!!              */
  } 
}



void do_everything (int dsp, int in, int fragsize, 
                    int is_pcsp, int ppid)
{
  char k;
  int i, j, fd;
  int terminate = -1;             /* Which Sound to Terminate                              */
  int playing[16];                /* Sound numbers that we are playing                     */
  int playnum = 0;                /* Number of sounds currently being played               */
  unsigned char buffer[16][256];  /* Buffer for each sound, MUST be at least Fragsize long */
  unsigned char final[256];       /* Final Mixing Buffer                                   */

  for (;;)
  {
    /* Try to open a new sound if we get an integer on the 'in' pipe */
    if ((read (in, &k, sizeof (k)) > 0) && (playnum < 16))
    {
      /* Negative means terminate the FIRST sound in the buffer */
      if (k < 0) terminate = 0;
      else
      {
        fd = open (FILENAME[(int)k], O_RDONLY);
        if (fd > 0) playing[playnum++] = fd;
#ifdef DEBUG
        else fprintf (stderr, "paradise.sndsrv: The sound %s could not be opened\n", FILENAME[k]);
#endif
      }
    }
#ifdef DEBUG
    else if (playnum >= 16) fprintf (stderr, "paradise.sndsrv: Too many sounds to play!\n");
#endif

    /* Kill the parent with signal zero to see if it's still alive */
    if (kill (ppid, 0)) { kill (getpid(), SIGTERM); } /* Is this an Expensive operation?? */

    /* Of all the active sounds, read the next FRAGSIZE bytes */
    for (i=0; i < playnum; i++)
    {
      j = read (playing[i], buffer[i], fragsize);
      if ((j == 0) || (i == terminate))
      {
        /* Close Sounds that are finished */
        close (playing[i]);
        playnum--;
        for (j = i; j < playnum; j++)
          memcpy (&playing[j], &playing[j+1], sizeof (int)); /* Use a better data structure? */
        terminate = -1;
      }
      /* Silence the rest of the array (to fill up to FRAGSIZE) */
      else for (;j < fragsize; j++) buffer[i][j] = 128;
    }

    if (playnum)
    {
      /* Mix each sound into the final buffer */
      for (i=0; i < fragsize; i++)
      {
        final [i] = 128;
        for (j=0; j < playnum; j++)
        {
           final[i] += (buffer[j][i] - 128);
        }
      }
      /*
         The sound server is in a tight loop, EXCEPT for this
         write which blocks.  Any optimizations in the above
         code would really be helpful.  Right now the server
         takes up to 7% cpu on a 486DX/50.
      */
      write (dsp, final, fragsize);
    }
    else
    {
      /* 
         We have no sounds to play
         Just fill the buffer with silence and maybe play it 
      */
      for (j = 0; j < fragsize; j++) final [j] = 128; /* Avoid Pop on Audio Devices  */
      if (!is_pcsp) write (dsp, final, fragsize);     /* Avoid Static on PC Speaker  */
      else          usleep (8000);                    /* Sleep since we didn't write */
    }
  }
}



void main (argc, argv)
int argc;
char **argv;
{
  int dsp, in, is_pcsp, fragsize, ppid;
  char filename[512];

  init (argc, argv);
  dsp = setup_dsp (&fragsize, &is_pcsp);
  ppid = getppid();

  sprintf (filename, "/tmp/paradise.sound.fifo.pid.%d", ppid);
  in = open (filename, O_RDONLY);
  fcntl (in, F_SETFL, O_NONBLOCK);

  if (!dsp) do_nothing (in);

  do_everything (dsp, in, fragsize, is_pcsp, ppid);
}
