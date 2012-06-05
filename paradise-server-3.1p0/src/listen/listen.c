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

/*------------------------------------------------------------------------
Startup program for netrek.  Listens for connections, then forks off
servers.  Based on code written by Brett McCoy, but heavily modified.

Note that descriptor 2 is duped to descriptor 1, so that stdout and
stderr go to the same file.
--------------------------------------------------------------------------*/

#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "config.h"
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include "proto.h"
#include "data.h"

/* #define DEF_PORT	2592*/	/* port to listen on */
/* #define NTSERV		"bin/ntserv"

#define METASERVER	"metaserver.netrek.org"
*/
/*
 * Since the metaserver address is a nameserver alias, we can't just
 * compare the peer hostname with the setting of METASERVER.  The peer
 * hostname returned by gethostbyaddr() will be the machine's real name,
 * which will be different (and could change).
 *
 * So, we'll do a gethostbyname() on METASERVER to get an IP address, then
 * we can compare that with the connecting peer.
 *
 * It'd be overkill to get the metaserver's IP address with every connection;
 * and it may be inadequate to get it just once at startup, since listen
 * can exist for long periods of time, and the metaserver's IP address could
 * be changed in that time.
 *
 * So, we'll grab the IP address at least every META_UPDATE_TIME seconds
 */
#define META_UPDATE_TIME (5*60*60)	/* five hours */

int     listenSock;
short   port = PORT;
char   *program;
int     meta_addr;

/*
 * Error reporting functions ripped from my library.
 */

int     metaserverflag;
char   *leagueflag = 0;
char   *observerflag = 0;

#define NEA	10
char   *extraargs[NEA];
int     extraargc = 0;

char	*ntserv_binary=NTSERV;

/***********************************************************************
 * Returns a string containing the date and time.  String area is static
 * and reused.
 */

static char *
dateTime(void)
{
    time_t  t;
    char   *s;

    time(&t);
    s = ctime(&t);
    s[24] = '\0';		/* wipe-out the newline */
    return s;
}

/***********************************************************************
 * Close all file descriptors except the ones specified in the argument list.
 * The list of file descriptors is terminated with -1 as the last arg.
 */

static void
multClose(int *fds_not_to_close)
{
    int     nfds, ts, i, j, fds[100] = {0};

    /* get all descriptors to be saved into the array fds */
    for (nfds = 0; nfds < 99 && fds_not_to_close[nfds] != -1; nfds++) {
        fds[nfds] = fds_not_to_close[nfds];
    }

#ifdef HAVE_SYSCONF
    ts = sysconf(_SC_OPEN_MAX);
    if (ts < 0)			/* value for OPEN_MAX is indeterminate, */
	ts = 32;		/* so make a guess */
#else
    ts = getdtablesize();
#endif

    /*
       close all descriptors, but first check the fds array to see if this
       one is an exception
    */
    for (i = 0; i < ts; i++) {
	for (j = 0; j < nfds; j++)
	    if (i == fds[j])
		break;
	if (j == nfds)
	    close(i);
    }
}

/***********************************************************************
 * Error reporting functions taken from my library.
 */

static void
err(char *func, char *fmt, va_list args)
{
  if (program)
    fprintf(stderr, "%s", program);

  if (func && strcmp(func, ""))
    fprintf(stderr, "(%s)", func);

  fprintf(stderr, ": ");

  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  fflush(stderr);
}

static void
syserr(int exit_code, char *func, char *fmt, ...)
{
  va_list vp;

  va_start(vp, fmt);
  err(func, fmt, vp);
  if(errno < sys_nerr)
    fprintf(stderr, "     system message: %s\n", sys_errlist[errno]);
  va_end(vp);

  exit(exit_code);
}

static void
warnerr(char *func, char *fmt, ...)
{
  va_list vp;

  va_start(vp, fmt);
  err(func, fmt, vp);
  va_end(vp);
}

static void
fatlerr(int exit_code, char *func, char *fmt, ...)
{
  va_list vp;

  va_start(vp, fmt);
  err(func, fmt, vp);
  va_end(vp);

  exit(exit_code);
}


/***********************************************************************
 * Detach process in various ways.
 */

static void 
detach(void)
{
    int     fd, rc, mode;
    char   *fname;
    int     fds_not_to_close[] = {1, 2, -1};

    mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR;
    fname = build_path("logs/startup.log");
    if ((fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, mode)) == -1)
	syserr(1, "detach", "couldn't open log file. [%s]", dateTime());
    dup2(fd, 2);
    dup2(fd, 1);
    multClose(fds_not_to_close);	/* close all other file descriptors */
    warnerr(NULL, "started at %s on port %d.", dateTime(), port);

    /* fork once to escape the shells job control */
    if ((rc = fork()) > 0)
	exit(0);
    else if (rc < 0)
	syserr(1, "detach", "couldn't fork. [%s]", dateTime());

    /* now detach from the controlling terminal */
    /* NOTE -- newer versions of setsid() automatically release the
       controlling terminal of the process.  If for some reason yours
       doesn't, define SETSID_DOESNT_DETACH_TERMINAL and add or modify
       the code below.  At this point you're on your own. */
#ifdef SETSID_DOESNT_DETACH_TERMINAL
#ifdef _SEQUENT_
    if ((fd = open("/dev/tty", O_RDWR | O_NOCTTY, 0)) >= 0) {
	(void) close(fd);
    }
#else
    if ((fd = open("/dev/tty", O_RDWR, 0)) == -1) {
	warnerr("detach", "couldn't open tty, assuming still okay. [%s]",
		dateTime());
	return;
    }
#if defined(SYSV) && defined(TIOCTTY)
    {
	int     zero = 0;
	ioctl(fd, TIOCTTY, &zero);
    }
#else
    ioctl(fd, TIOCNOTTY, 0);
#endif
    close(fd);
#endif				/* _SEQUENT_ */
#endif

    setsid();			/* make us a new process group/session */
}

/***********************************************************************
 */

static void 
getListenSock(void)
{
    struct sockaddr_in addr;

    if ((listenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	syserr(1, "getListenSock", "can't create listen socket. [%s]",
	       dateTime());

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    {
/* added this so we could handle nuking listen. KAO 3/26/93 */
	int     foo = 1;
	if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char *) &foo,
		       sizeof(foo)) == -1) {
	    fprintf(stderr, "Error setting socket options in listen.\n");
	    exit(1);
	}
    }
    if (bind(listenSock, (struct sockaddr *) & addr, sizeof(addr)) < 0)
	syserr(1, "getListenSock", "can't bind listen socket. [%s]",
	       dateTime());

    if (listen(listenSock, 5) != 0)
	syserr(1, "getListenSock", "can't listen to socket. [%s]", dateTime());
}

/***********************************************************************
 */

static void
getConnections(void)
{
    int     len, sock, pid, pa;
    struct sockaddr_in addr;
    struct hostent *he;
    char    host[100];
    int     fds_not_to_close[] = {0, 1, 2, -1};

    len = sizeof(addr);
    while ((sock = accept(listenSock, (struct sockaddr *) & addr, &len)) < 0) {
	/* if we got interrupted by a dying child, just try again */
	if (errno == EINTR)
	    continue;
	else
	    syserr(1, "getConnections",
		   "accept() on listen socket failed. [%s]", dateTime());
    }

    /* get the host name */
    he = gethostbyaddr((char *) &addr.sin_addr.s_addr,
		       sizeof(addr.sin_addr.s_addr), AF_INET);
    if (he != 0) {
	strcpy(host, he->h_name);
	pa = *((int *) he->h_addr);
    }
    else {
	strcpy(host, inet_ntoa( addr.sin_addr ));
	pa = (int) addr.sin_addr.s_addr;
    }

    if (pa == meta_addr)
	metaserverflag = 1;
    /* else */
    warnerr(NULL, "connect: %-33s[%s][%d]", host, dateTime(), metaserverflag);

    /* fork off a server */
    if ((pid = fork()) == 0) {
	char   *newargv[10];
	int     newargc = 0;
	int     i;
	char   *binname;
	binname = build_path(ntserv_binary);
	dup2(sock, 0);
	multClose(fds_not_to_close);	/* close everything else */

	newargv[newargc++] = "ntserv";
	if (metaserverflag == 1)
	    newargv[newargc++] = "-M";
	for (i = 0; i < extraargc; i++)
	    newargv[newargc++] = extraargs[i];
	newargv[newargc++] = host;
	newargv[newargc] = 0;

	execv(binname, newargv);

	syserr(1, "getConnections",
	       "couldn't execv %s as the server. [%s]",
	       binname, dateTime());
	exit(1);
    }
    else if (pid < 0)
	syserr(1, "getConnections", "can't fork. [%s]", dateTime());

    close(sock);
    metaserverflag = 0;
}

/***********************************************************************
 * Adds "var=value" to environment list
 */

static void
set_env(char *var, char *value)
{
    char   *buf;
    buf = malloc(strlen(var) + strlen(value) + 2);
    if (!buf)
	syserr(1, "set_env", "malloc() failed. [%s]", dateTime());

    strcpy(buf, var);
    strcat(buf, "=");
    strcat(buf, value);

    putenv(buf);
}


/***********************************************************************
 * Handler for SIGTERM.  Closes and shutdowns everything.
 */

static RETSIGTYPE 
terminate(int unused)
{
    int     s;

    fatlerr(1, "terminate", "killed. [%s]", dateTime());
#ifdef HAVE_SYSCONF
    s = sysconf(_SC_OPEN_MAX);
    if (s < 0)			/* value for OPEN_MAX is indeterminate, */
	s = 32;			/* so make a guess */
#else
    s = getdtablesize();
#endif
    /* shutdown and close everything */
    for (; s >= 0; s--) {
	shutdown(s, 2);
	close(s);
    }
}

/***********************************************************************
 * Waits on zombie children.
 */

static RETSIGTYPE
reaper(int unused)
{
#ifdef HAVE_WAIT3
    while (wait3(0, WNOHANG, 0) > 0);
#else
    while (waitpid(0, 0, WNOHANG) > 0);
#endif				/* SVR4 */
}

/*---------------------[ prints the usage of listen ]---------------------*/

static void
printlistenUsage(char *name)
{
    int x;
    char message[][255] = {
        "\n\t'%s [options]'\n\n",
        "Options:\n",
        "\t-h      help (this usage message)\n",
        "\t-p      port other than default port\n",
        "\t-k n    use n as shared memory key number\n\n",
        "Any unrecognized options are passed through to ntserv. For an up\n",
        "to date listing of available ntserv options check the binary.\n",
        "\nNOTE: in league play you must start up two listen processes, ",
        "one for each port.\n\n",
        "\0"
    };

    fprintf(stderr, "-- NetrekII (Paradise), %s --\n", PARAVERS);
    for (x=0; *message[x] != '\0'; x++)
        fprintf(stderr, message[x], name);

    exit(1);
}

/*--------------------------[ printlistenUsage ]--------------------------*/

/* set the pid logfile (BG) */
static void
print_pid(void)
{
	char *fname;
	FILE *fptr;

	fname = build_path("logs/listen.pid");
	fptr = fopen(fname, "w+");
	fprintf(fptr, "%d", getpid());
	fclose(fptr);
}

int 
main(argc, argv)
    int     argc;
    char   *argv[];
{
    int     i, key;
    int     nogo = 0;
    struct hostent *he;
    struct timeval tv;
    time_t  stamp, now;

    metaserverflag = 0;

    for (i=1; i < argc; i++) {
        if (*argv[i] == '-') {
            switch (argv[i][1]) {
              case 'p':
                 port = atoi(argv[i + 1]);
                 break;
              case 'k':
                 if (++i < argc && sscanf(argv[i], "%d", &key) > 0 && key > 0)
                     set_env("TREKSHMKEY", argv[i]);
                 else
                     nogo++;
                 break;
              case 'b':
                 ntserv_binary = argv[++i];
                 break;
              case 'h':
              case 'u': /* for old times sake, the others don't do this,
                           but this one does so it doesn't pass to ntserv */
              case '-': /* same with this */
                 nogo++;
                 break;
              default:
                 /* all unknown arguments are passed through to ntserv. */
                 extraargs[extraargc++] = argv[i];
            }
        }
        /* else just ignore non flags */
    }

    if ((program = strrchr(argv[0], '/')))
        ++program;
    else
        program = argv[0];      /* let err functions know our name */

    if (nogo)
	printlistenUsage(program);

    detach();			/* detach from terminal, close files, etc. */
    print_pid();		/* log the new PID */
    getListenSock();
    r_signal(SIGCHLD, reaper);
    r_signal(SIGTERM, terminate);
    r_signal(SIGHUP, SIG_IGN);

    meta_addr = 0;
    stamp = 0;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (1) {
	now = time(0);
	if (now - stamp > META_UPDATE_TIME) {
	    he = gethostbyname(METASERVER);
	    if (he)
		meta_addr = *((int *) he->h_addr);
	    stamp = now;
	}
	getConnections();
	select(0, 0, 0, 0, &tv);/* wait one sec between each connection */
    }
}

