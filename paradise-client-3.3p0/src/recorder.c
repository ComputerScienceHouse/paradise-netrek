
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "packets.h"
#include "gppackets.h"

int     record_total;
int     lastTeamReq = -1;
int     recfp = -1, playfp = -1;

void pb_skip P((int frames));

void
startRecorder(void)
{
    if ((recordGame = booleanDefault("recordGame", recordGame))) {
	recordFile = stringDefault("recordFile", "/tmp/netrek.record");
	recordFile = expandFilename(recordFile);
	maxRecord = intDefault("maxRecord", maxRecord);
	recfp = open(recordFile, O_WRONLY | O_TRUNC | O_APPEND | O_CREAT, S_IRWXU);
	if (recfp >= 0)
	    printf("Game being recorded to %s.  Max size is %d\n", recordFile, maxRecord);
	else {
	    perror("startRecorder");
	    printf("Can't open file %s for recording\n", recordFile);
	    recordGame = 0;
	}
    }
}

void
stopRecorder(void)
{
    char    buf[80];

    close(recfp);
    recfp = -1;
    recordGame = 0;
    sprintf(buf, "Recording stopped, %d bytes (%d updates) written to %s\n",
	   record_total, udcounter, recordFile ? recordFile : "(none)");
    warning(buf);
}

void
recordPacket(char *data, int len)
{
    int     res;

    if (recfp >= 0 && len > 0) {
	switch (*data) {
	case SP_PICKOK:	/* playback needs to know what team! */
	    data[2] = (char) lastTeamReq;
	    break;
	case SP_RSA_KEY:
	case SP_MOTD:
	case SP_MOTD_PIC:
	case SP_NEW_MOTD:
	    return;
	default:
	    break;
	}
	res = write(recfp, data, len);
	record_total += res;
	if ((maxRecord && (record_total > maxRecord)) || (res <= 0))
	    stopRecorder();
    }
}

void
writeUpdateMarker(void)
{
    unsigned char update_buf[4];

    update_buf[0] = REC_UPDATE;
    /*
       record stuff not normally sent by server.  Otherwise tractors and lock
       markers don't work during playback
    */
    update_buf[1] = (unsigned char) me->p_tractor;
    update_buf[2] = (unsigned char) ((me->p_flags & PFPLOCK) ? me->p_playerl : me->p_planet);
    /* one more byte here, any ideas? */
    record_total += write(recfp, update_buf, 4);
}

int
startPlayback(void)
{
    if (playback || (playback = booleanDefault("playback", 0))) {
	if (!playFile)
	    playFile = stringDefault("playFile", "/tmp/netrek.record");
	playFile = expandFilename(playFile);

	playfp = open(playFile, O_RDONLY, 0);
	if (playfp < 0) {
	    perror("startPlayback");
	    printf("Couldn't open playback file %s\n", playFile);
	    exit(0);
	}
	printf("Replaying %s\n", playFile);
	return 1;
    }
    return 0;
}

int
readRecorded(int fp, char *data, int len)
{
    int     ret;

    if (!playback || len < 0 || playfp < 0)
	return -1;
    if (len > 4)		/* make sure we don't skip updates */
	len = 4;
    ret = read(playfp, data, len);
    if (ret <= 0)
	EXIT(0);
    return ret;
}

void
pb_dokey(W_Event *event)
{
    switch (event->key&0xff) {
    case 'p':
    case 'P':
	paused = !paused;
	pb_scan=0;
	if (paused)
	    pb_advance = 0;
	break;
    case 'r':
    case 'R':
	pb_advance = PB_REDALERT;
	paused = 0;
	break;
    case 'y':
    case 'Y':
	pb_advance = PB_YELLOWALERT;
	paused = 0;
	break;
    case 'd':
    case 'D':
	pb_advance = PB_DEATH;
	paused = 0;
	break;
    case ' ':
	if(paused) {
	    if(pb_scan)
		pb_skip(pb_advance);
	    else {
		pb_advance=1;
		paused=0;
	    }
	} else {
	    paused = 1;
	    pb_advance=0;
	}
	break;
    case 'f':
    case 'F': /* fast forward */
	pb_scan=!pb_scan;
	if(pb_scan) {
	    pb_advance=10;
	    pb_slow=0;
	    paused=0;
	    warning("1-9: set speed. P-Pause, F-Fast Fwd off");
	} else {
	    pb_advance=0;
	    pb_slow=0;
	    paused=0;
	    warning("Playback mode, keys: P)ause R)ed Y)ellow D)eath F)ast S)low(toggle) Q)uit");
	}
	break;
    case 's': /* Slow mode */
    case 'S':
	if(pb_scan || paused || !pb_slow)
	    pb_slow=1;
	else
	    pb_slow=0;
	pb_scan=0;
	paused=0;
	pb_advance=0;
	break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	if(!pb_scan)
	    pb_advance += (event->key - '0') * 10;
	else
	    pb_advance = (event->key - '0') * 10;
	paused = 0;
	break;
    case 'q':
    case 'Q':
	exit(0);
    default:
	warning("Playback mode, keys: P)ause R)ed Y)ellow D)eath F)ast S)low(toggle) Q)uit");
    }
}

void
pb_skip(int frames)
{
    while(frames) {
	pb_update=0;
	readFromServer();
	frames-=pb_update;
	udcounter+=pb_update;
    }
}

void
pb_framectr(int xloc, int yloc)
{
    char buf[20];

    if(paused) 
	strcpy(buf,"PAU");
    else if(pb_scan)
	strcpy(buf,"FFW");
    else if(pb_advance) {
	switch(pb_advance) {
	case PB_REDALERT:
	    strcpy(buf,"RED");
	    break;
	case PB_YELLOWALERT:
	    strcpy(buf,"YLW");
	    break;
	case PB_DEATH:
	    strcpy(buf,"DTH");
	    break;
	default:
	    buf[0]='+';
	    buf[1]=(pb_advance / 10) + '0';
	    buf[2]=(pb_advance % 10) + '0';
	    break;
	}
    } else if(pb_slow)
	strcpy(buf,"SLW");
    else
	strcpy(buf,"NRM");

    sprintf((buf+3),":%8d",udcounter);

    W_WriteText(tstatw, xloc, yloc, textColor, buf, 12, W_RegularFont);
}

void
pb_input(void)
{
    W_Event data;
    fd_set  readfds;
    int     wsock = W_Socket();

    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    intrupt(); /* draws the first frame */

    while (me->p_status == PALIVE ||
	   me->p_status == PEXPLODE ||
	   me->p_status == PDEAD ||
	   me->p_status == POBSERVE) {

	if (keepInfo > 0) {
	    if (infowin_up >= 0 &&
		--infowin_up == -1 &&
		infomapped) {
		destroyInfo();
		infowin_up = -2;
	    }
	}
	exitInputLoop = 0;
	while (W_EventsPending() && !exitInputLoop) {
	    fastQuit = 0;	/* any event cancel fastquit */
	    W_NextEvent(&data);
	    dispatch_W_event(&data);
	}
	if (!paradise)  /* stars are ok, it's just a recording.
			   zoom doesn't make sense. */
	    blk_zoom = 0;

	FD_ZERO(&readfds);
	FD_SET(wsock, &readfds);
	if (paused) {
	    select(32, &readfds, (fd_set *) 0, (fd_set *) 0, 0);
	    continue;
	} else if(pb_slow && !pb_scan) {
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 200000;

	    select(32, &readfds, (fd_set *) 0, (fd_set *) 0,&timeout);
	}
	/* otherwise, in full blast mode, don't wait on anything. */

	/* at this point, we're sure it's time for at least one frame. */
	intrupt();
    }
}

