/*
 * paradise.sndsrv.c - USS-Lite Compatible Sound - July 1996
 *                     This server is HP/UX Specific.
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
 *      $Id: paradise.sndsrv.hp.c,v 1.1 2000/01/06 21:26:45 ahn Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/audio.h>
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

#define NUM_SOUNDS      (sizeof(FILENAME)/sizeof(char*))

signed char *sound_buffer[NUM_SOUNDS];
int sound_size[NUM_SOUNDS];
#define fragsize (256)


/* Terminate: Signal Handler */
void quit ()
{
  exit (0);
}



void init (int argc, char **argv)
{
  int i;
  char s[1024];

  if (argc != 3)
  {
    printf ("This program is only executed by Paradise\n");
    exit (1);
  }

  for (i=0; i < NUM_SOUNDS; i++)
  {
    s[0] = 0;
    strcat (s, argv[1]);
    if (s[(int)strlen(s) - 1] == '/') FILENAME[i]++;
    strcat (s, FILENAME[i]);
    FILENAME[i] = malloc ((int)strlen (s)+1);
    strcpy (FILENAME[i],s);
    sound_buffer[i]=NULL;
    sound_size[i]=0;
  }

  signal(SIGTERM, quit);   /* Setup Terminate Signal Handler */
}


/*
   Setup DSP: Opens /dev/audio
              Sets fragment size to 512
              Error checking                
*/
int setup_dsp (char *dspdev)
{
  int dsp, frag, value;
  int mixer;

  dsp = open(dspdev, O_RDWR);
  if (dsp < 1)
  {
    fprintf (stderr, "paradise.sndsrv: Couldn't open device %s\n",dspdev);
    return -1;
  }
 
	if (-1 == ioctl(dsp, AUDIO_SET_DATA_FORMAT, AUDIO_FORMAT_LINEAR16BIT)) {
		fprintf(stderr,"paradise.sndsrv: parameter setting failed");
		return;
	}

	if (-1 == ioctl(dsp, AUDIO_SET_CHANNELS, 1)) {
		fprintf(stderr,"paradise.sndsrv: parameter setting failed");
		return;
	}

	if (-1 == ioctl(dsp, AUDIO_SET_SAMPLE_RATE, 16000)) {
		fprintf(stderr,"paradise.sndsrv: parameter setting failed");
		return -1;
	}

	if (-1 == ioctl(dsp, AUDIO_SET_OUTPUT, AUDIO_OUT_INTERNAL)) {
		fprintf(stderr,"paradise.sndsrv: parameter setting failed");
		return -1;
	}
	
  
  return dsp;
}

/*
   This just keeps the pipe from breaking...
   Eventually I'll look at the paradise signal handlers and
   just trap this.
*/
int do_nothing(void)
{
    while(1)  sleep (5);
}

int read_sound(int k)
{
  int i,fd,size;
	unsigned char * b;
	short * d;
	
  /* fprintf(stderr,"loading sound %d, %s\n",k,FILENAME[k]); */

  fd = open(FILENAME[k], O_RDONLY);
  if(fd<=0) 
  {
    fprintf (stderr, "paradise.sndsrv: The sound %s could not be opened\n", FILENAME[k]);
    sound_size[k]=-1;
    return(0);
  };
  size=lseek(fd,0,SEEK_END);
  sound_size[k]=((size)/fragsize)+1;   /*size in fragments*/
  sound_buffer[k]=malloc(sound_size[k]*sizeof(short)*fragsize);
  if(sound_buffer[k]==NULL)
  {
    fprintf(stderr,"paradise.sndsrv: couldn't malloc memory for sound\n");
    sound_size[k]=-1;
    close(fd);
    return(0);
  };
  lseek(fd,0,SEEK_SET);
  read(fd,sound_buffer[k],size);
  
  b = ((unsigned char *)sound_buffer[k])+sound_size[k]*fragsize;
  d = sound_buffer[k] + sound_size[k]*fragsize;
	/* fprintf(stderr,"size = %d\n",sound_size[k]*fragsize); */
  for (i=0; i < sound_size[k]*fragsize; i++) {
    *(--d) = (((short)*(--b))-128) << 8;
  }
  
  close(fd);
  bzero(((char *)sound_buffer[k])+size, sound_size[k]*sizeof(short)*fragsize-size);
  
  /* fprintf(stderr,"buba! sound has been loaded, %d bytes\n",size);*/ /*DEBUG*/
  return(1);
}


void do_everything (int dsp)
{
  char k;
  int i, j ;
  int terminate = -1;             /* Which Sound to Terminate                              */
  int playing[16];                /* Sound numbers that we are playing                     */
  int position[16];		  /* Current position in each sound file */
  int playnum = 0;                /* Number of sounds currently being played               */
  short final[fragsize];       /* Final Mixing Buffer                                   */
  short *sample;

  for(;;)  {
    terminate = -1;
    /* Try to open a new sound if we get an integer on the 'in' pipe */
    i=read(STDIN_FILENO,&k,sizeof(k));
    if(i==0)  {   /* EOF on pipe means parent has closed its end */
        /*fprintf(stderr,"paradise.sndsrv: shutting down\n"); */
        kill(getpid(), SIGTERM);
    };
    if(i!=-1)  {  /* there was something in the pipe */
        /*fprintf(stderr,"Just read a %d from pipe\n",(int)k);*/ /*DEBUG*/
        /* Negative means terminate the FIRST sound in the buffer */
        if(k<0)  {
            fprintf(stderr,"terminating sound\n"); /*DEBUG*/
            terminate = 0;
        } else {
            if(sound_size[(int)k]==0) read_sound(k);
            if(sound_size[(int)k]>0 && playnum<16)  {
	        position[playnum]=0;
                playing[playnum++]=k;
               /* fprintf(stderr,"sound %d added to play queue\n",playnum-1);*/ /*DEBUG*/
            };
        };
    };

    /* terminate a sound if necessary */
    for(i=0;i<playnum;i++)
    {
      if((position[i]==sound_size[playing[i]]) || (terminate==i))
      {
     /*   fprintf(stderr,"finished playing sound %d\n",i); */ /*DEBUG*/
	/* fprintf(stderr,"is was at position %d\n",position[i]); */ /*DEBUG*/
        bcopy(playing+i+1,playing+i,(playnum-i)*sizeof(int));
        bcopy(position+i+1,position+i,(playnum-i)*sizeof(int));
        playnum--;i--;
      };
    };

    memset(final, 0, sizeof(final));
    if(playnum)  {
        /* Mix each sound into the final buffer */
        for(i=0;i<playnum;i++)  {
        		short * f = final;
            sample=sound_buffer[playing[i]]+position[i]*fragsize;
            for(j=0;j<fragsize;j++)  {
            	 long s = *f;
            	 s += *sample++;
            	 if (s < -32768) {
            	 	s = -32768;
            	 }
            	 if (s > 32767) {
            	 	s = 32767;
            	 }
                *f++ = (short)s;
            };
            position[i]++;
        }
    } else {
        /* 
           We have no sounds to play
           Just fill the buffer with silence and maybe play it 
        */
    };
    write (dsp, final, fragsize*sizeof(short));
    /*
       The sound server is in a tight loop, EXCEPT for this
       write which blocks.  Any optimizations in the above
       code would really be helpful.  Right now the server
       takes up to 7% cpu on a 486DX/50.
    */
  }
}



void main (argc, argv)
int argc;
char **argv;
{
  int dsp;

  fcntl(STDIN_FILENO,F_SETFL,O_NONBLOCK);
  init (argc, argv);
  dsp = setup_dsp (argv[2]);

  if (dsp < 1) do_nothing();

  do_everything (dsp);
}
