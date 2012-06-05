/*
 * meta.c     - Nick Trown    May 1993
 */

#include "copyright.h"
#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "str.h"
#include "conftime.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#define BUF	4096

/* --------------------------------------------------------------- */
/* this list stuff shouldn't be in this file ... */

typedef struct line_s line_t;

struct line_s {
    char str[LINE + 1];
    line_t *prev;
    line_t *next;
};

typedef struct list_s list_t;

struct list_s {
    line_t *top;
    line_t *bot;
    line_t *curr;
    char    fname[255];
};

/* --------------------------------------------------------------- */

#define DEFMETAFILE ".paradise/metaserver-cache"

int     pid = 1;
int     num_servers = 0;
int	max_servers = 20;
struct  servers *serverlist = NULL;
time_t  last_read;       /* 14-Feb-95: Brandon */
int     failed_conn = 0; /* 14-Feb-95: Brandon */

#define KEY 6
char    *keystrings[] = {
           "OPEN:",
           "Wait queue:",
           "Nobody playing",
           "* No connection",
           "* Garbaged read",
           "* Timed out   "
         };

#define KEY_OPEN	0
#define KEY_QUEUE	1
#define KEY_NOBODY	2
#define KEY_NOCONN	3
#define KEY_GARBAGE	4
#define KEY_TIMEOUT	5

/* --------------------------------------------------------------- */
/* function protos */

static int getnumber(char *string, int start);
static int open_port(char *host, int port, int verbose);
static void add_line(list_t *list, char *str);
static void toss_list(list_t *list);
static list_t *new_list(void);
static void write_list_to_file(list_t *list);
static void read_file_to_list(list_t *list, FILE *fptr);
static void read_socket_to_list(list_t *list, int socket);
static void parse_list_to_serverlist(list_t *list);
static void parse_list_to_serverlist(list_t *list);
static list_t *read_metaserver(int s);
static char *get_servertype(char type);
static void metarefresh(int i);
static void metaaction(W_Event *data);
static void metadone(void);

void openmeta(void);
void metawindow(void);
void metainput(void);

/* --------------------------------------------------------------- */
/* This function finds the next integer after index 'start'.  */
static int
getnumber(char *string, int start)
{
    string += start;
    while(!isdigit(*string) && *string) string++;
    return atoi(string);
}

/* --------------------------------------------------------------- */
/* The connection to the metaserver is by Andy McFadden.
   This calls the metaserver and returns the socket or -1 if failed */
static int
open_port(char *host, int port, int verbose)
{
    struct sockaddr_in addr;
    struct hostent *hp;
    int     s;

    /* shouldn't we have some generic "look up this name" function? */
    /* get numeric form */
    if ((addr.sin_addr.s_addr = inet_addr(host)) == -1) {
	if ((hp = gethostbyname(host)) == NULL) {
	    if (verbose)
		fprintf(stderr, "unknown host '%s'\n", host);
	    return (-1);
	} else {
	    addr.sin_addr.s_addr = *(long *) hp->h_addr;
	}
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	if (verbose)
	    perror("socket");
	return (-1);
    }
    if (connect(s, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
	if (verbose)
	    perror("connect");
	close(s);
	return (-1);
    }
    return (s);
}

/* --------------------------------------------------------------- */
/* Adds a line to the list, if it is acceptable */
static void
add_line(list_t *list, char *str)
{
    line_t *line;

    /* if there is no string, or it doesn't begin with "-h" */
    if (str[0] == 0 || (strncmp(str, "-h", 2) != 0))
        return;

    line = (line_t *) malloc(sizeof(line_t));
    line->next = line->prev = NULL;

    if (strlen(str) >= LINE)
        str[LINE] = '\0';
    if (str[strlen(str) - 1] == '\n')
        str[strlen(str) - 1] = '\0';

    strcpy(line->str, str);

    if (list->top == NULL) {
      list->top = line;
      list->bot = line;
    } else {
      list->bot->next = line;
      line->prev = list->bot;
      list->bot = line;
    }
}

/* --------------------------------------------------------------- */
/* free up memory used by the list */
static void
toss_list(list_t *list)
{
    while (list->top != NULL) {
        list->curr = list->top->prev;
        free(list->top);
        list->top = list->curr;
    }
    free(list);
}

/* --------------------------------------------------------------- */
/* allocate memory for a list */
static list_t *
new_list(void)
{
    list_t *newl;
    newl = (list_t *) malloc(sizeof(list_t));
    newl->top = newl->bot = newl->curr = NULL;
    return newl;
}

/* --------------------------------------------------------------- */
/* write list to file (Pretty self explanatory...) */
static void
write_list_to_file(list_t *list)
{
    FILE *fptr;

    fptr = fopen(list->fname, "w");
    if (fptr == NULL) {
        fprintf(stderr,"Unable to open default metaserver file for writing (%s).\n",
                list->fname);
    } else {
        fprintf(fptr, "%d\n", (int)last_read);
        list->curr = list->top;
        while (list->curr != NULL) {
            fprintf(fptr, "%s\n", list->curr->str);
            list->curr = list->curr->next;
        }
        fclose(fptr);
    }
}

/* --------------------------------------------------------------- */
/* read the file into the list */
static void
read_file_to_list(list_t *list, FILE *fptr)
{
    char    line[BUF];

    if (fptr == NULL) {
        fprintf(stderr, "Unable to open default metaserver file (%s).\n",
                list->fname);
    } else {
        fscanf(fptr, "%d", (int*)&last_read);
        while(1) {
            fgets(line, LINE + 1, fptr);
            if (feof(fptr))
                break;
            add_line(list, line);
        }
    }
}

/* --------------------------------------------------------------- */
/* read from socket into list */
static void
read_socket_to_list(list_t *list, int socket)
{
    int     c, cc, lc;
    char    buf[BUF + 1];
    char    line[80];

    /* loop indefinitely, until we cannot read off the socket */
    while (1) {

        if ((cc = sock_read(socket, buf, BUF)) <= 0) {
            if (cc < 0)
                perror("read");
            sock_close(socket);
            break;
        }

        lc = strlen(line);

        for (c=0; c < cc; c++) {
            if (buf[c] != '\n') {
                line[lc++] = buf[c];
                line[lc] = '\0';
            } else {
                add_line(list, line);
                lc = 0;
                strcpy(line, "");
            }
        }
    }

    last_read = time(NULL);
}

/* --------------------------------------------------------------- */
/* parses the list into the server struct */
static void
parse_list_to_serverlist(list_t *list)
{
    int          tc;
    static char *numstr;

    list->curr = list->bot;

    while (list->curr != NULL) {
        /* scan in the line, following the metaserver output format */
        if (sscanf(list->curr->str, "-h %s -p %d %d",
                   serverlist[num_servers].address,
                   &serverlist[num_servers].port,
                   &serverlist[num_servers].time) == 3) {
            /* get what type of server it is */
            serverlist[num_servers].typeflag
                = list->curr->str[strlen(list->curr->str) - 1];

            /* set the paradise bit first, incase it doesn't have players */
            if (serverlist[num_servers].typeflag == 'P')
                serverlist[num_servers].status = 2;
            else
                serverlist[num_servers].status = -1;

            /* open/responding/players */
            for (tc = 0; tc < KEY; tc++) {
                if ((numstr = strstr(list->curr->str, keystrings[tc]))) {
                    serverlist[num_servers].status = tc;
                    serverlist[num_servers].players = getnumber(numstr, 0);
                }
            }

            /* does it require RSA? */
            if (strrchr(list->curr->str, 'R'))
                serverlist[num_servers].RSA_client = 1;
            else
                serverlist[num_servers].RSA_client = 0;

            /* should we keep it? */
            if ((serverlist[num_servers].status >= 0 &&
                serverlist[num_servers].status < 2) ||
                serverlist[num_servers].typeflag == 'P') {

                serverlist[num_servers].hilited = 0;

                num_servers++;    /* start writing into the next spot */

                /* if serverlist is too small, double its size */
                if(num_servers == max_servers) {
                    max_servers *= 2;
                    serverlist = (struct servers*) realloc(serverlist, 
                                 sizeof(struct servers) * max_servers);
                }
            }
        }

        /* step back the list */
        list->curr = list->curr->prev;

    }

/*    toss_list(list);*/
}

/* --------------------------------------------------------------- */
/* gets output from either the metaserver or file */
static list_t *
read_metaserver(int s)
{
    char   *home;                     /* defaults filename */
    list_t *list;
    FILE   *fptr;

    list = new_list();

    /* figure default metaserver file location */
    home = getenv("HOME");
    strcpy(list->fname, (home == NULL ? "." : home));
    strcat(list->fname, "/");
    strcat(list->fname, DEFMETAFILE);

    fptr = fopen(list->fname, "r");

    if (failed_conn) {
	printf("failed - reading from file...");
        read_file_to_list(list, fptr);
        if (fptr == NULL)
            return NULL;
        else
            fclose(fptr);
    } else {
        printf("connected.\n");
        read_socket_to_list(list, s);
    }

    return list;
}

/* --------------------------------------------------------------- */
/* central function, calls all others */
void
openmeta(void)
{
    int     s;
    list_t *list;

    num_servers = 0;

    if(serverlist)
        free(serverlist);

    serverlist = (struct servers*) malloc(sizeof(struct servers) * max_servers);

    if (serverName) {
	strcpy(serverlist[num_servers].address, serverName);
	serverlist[num_servers].port = xtrekPort;

#ifdef RSA
	serverlist[num_servers].RSA_client = RSA_Client;
#endif

	serverlist[num_servers].status = KEY + 1;
	serverlist[num_servers].players = 0;
	num_servers++;
    }

    printf("connecting to metaserver...");

    if ((s = open_port(metaserverAddress, METAPORT, 1)) <= 0) {
        failed_conn++;
	printf("\nFailed (%s, %d)\n", metaserverAddress, METAPORT);
        printf("Will try previous metaserver log...");
    }

    list = read_metaserver(s);

    puts("\n");

    if (list != NULL)
        parse_list_to_serverlist(list);

    write_list_to_file(list);

    toss_list(list);
}

/* --------------------------------------------------------------- */
/* Show the meta server menu window */
void
metawindow(void)
{
    register int i;
    static int old_num_servers = -1;
    char   buf[255];

    if (old_num_servers != num_servers) {
	if (metaWin)
	    W_DestroyWindow(metaWin);
	metaWin = W_MakeMenu("Metaserver List", winside + 10, -BORDER + 10, 69,
			     num_servers + 3, NULL, 2);
    }

    /* Print the date of the list */
    sprintf(buf, "             ** List is from %s **", ctime(&last_read));
    W_WriteText(metaWin, 0, 0, W_Yellow, buf, strlen(buf),
                W_HighlightFont);

    /* loop the serverlist and print each element */
    for (i = 0; i < num_servers; i++)
	metarefresh(i);

    /* add refresh option [BDyess] */
    W_WriteText(metaWin, (69 - 7)/2, num_servers + 1, W_Yellow, "Refresh",7,0);

    /* Add quit option */
    W_WriteText(metaWin, (69 - 4)/2, num_servers + 2, W_Red, "Quit", 4,
                W_RegularFont);

    /* Map window */
    if (!W_IsMapped(metaWin))
	W_MapWindow(metaWin);
}

/* --------------------------------------------------------------- */
static char *
get_servertype(char type)
{
    switch (type) {
        case 'P':
            return strdup("Paradise");
        case 'B':
            return strdup("Bronco");
        case 'C':
            return strdup("Chaos");
        case 'I':
            return strdup("INL");
        case 'S':
            return strdup("Sturgeon");
        case 'H':
            return strdup("Hockey");
        case 'F':
            return strdup("Dogfight");
    }
    return strdup("Unknown");
}

/* --------------------------------------------------------------- */
/* Refresh item i */
static void
metarefresh(int i)
{
    char    buf[BUFSIZ];
    W_Color color = textColor;

    if (serverlist[i].status < KEY) {
        if (failed_conn) {
            sprintf(buf, "%-40s             %s",
                    serverlist[i].address,
                    get_servertype(serverlist[i].typeflag));
        } else {
            if (serverlist[i].status < 2) {
                sprintf(buf, "%-40s %12s %-5d %s",
                        serverlist[i].address,
                        keystrings[serverlist[i].status],
                        serverlist[i].players,
                        get_servertype(serverlist[i].typeflag));
            } else {
                sprintf(buf, "%-40s %-18s %s",
                        serverlist[i].address,
                        keystrings[serverlist[i].status],
                        get_servertype(serverlist[i].typeflag));
            }
	    /* color-coded metaserver lines [BDyess] */
	    switch(serverlist[i].status) {
	      case KEY_OPEN:
		color = W_White;
		break;
	      case KEY_QUEUE:
		color = W_Cyan;
		break;
	      case KEY_NOBODY:
		color = W_Cyan;
		break;
	      case KEY_NOCONN:
		color = W_Red;
		break;
	      case KEY_GARBAGE:
		color = W_Red;
		break;
	      case KEY_TIMEOUT:
		color = W_Red;
		break;
	      default:
	        printf("Unknown status %d for metaserver line:\n%s\n",
		       serverlist[i].status,buf);
		color = W_White;
	    } 
        }
    } else if (serverlist[i].status == KEY) {
	sprintf(buf, "%-40s  ** Cannot Connect **", serverlist[i].address);
	color = W_Red;
    } else if (serverlist[i].status == KEY + 1) {
	sprintf(buf, "%-40s  DEFAULT SERVER", serverlist[i].address);
    }

    W_WriteText(metaWin, 0, i + 1,
		serverlist[i].hilited ? W_Yellow : color,
		buf, strlen(buf),
		serverlist[i].hilited ? W_HighlightFont : W_RegularFont);
}


/* --------------------------------------------------------------- */
/* Check selection to see if was valid. If it was then we have a winner! */
static void
metaaction(W_Event *data)
{
    int     s;
    static time_t lastRefresh = 0;
    static time_t t;
    int     place;

    place = data->y - 1;

    if ((place >= 0) && (place < num_servers)) {
	xtrekPort = serverlist[place].port;
	serverName = serverlist[place].address;

#ifdef RSA
	RSA_Client = serverlist[place].RSA_client;
#endif
	serverlist[place].hilited = 1;
	metarefresh(place);

	if ((s = open_port(serverName, xtrekPort, 0)) <= 0) {
	    serverlist[place].status = KEY;
	    serverlist[place].hilited = 0;
	    metarefresh(place);
	} else {
	    sock_close(s);
	    /* allow spawning off multiple clients [BDyess] */
	    if (metaFork) {
		/* just blink yellow [BDyess] */
		serverlist[place].hilited = 0;
		metarefresh(place);
		pid = fork();
	    } else
	    {
		pid = 1;
		metadone();
	    }
	}
    } else if (place == num_servers) {	/* refresh [BDyess] */
	/*
	   they can bang on refresh all day, but it won't refresh any faster
	   than once/minute. [BDyess]
	*/
	if ((t = time(NULL)) < lastRefresh + 60)
	    return;
	lastRefresh = t;
	openmeta();		/* connect and parse info */
	metawindow();		/* refresh */
    } else if (place == num_servers + 1) {			/* quit */
	metadone();
	exit(0);
    }
}


/* --------------------------------------------------------------- */
/* Unmap the metaWindow */

static void
metadone(void)
{
    /* Unmap window */
    W_UnmapWindow(metaWin);
}


/* --------------------------------------------------------------- */
/* My own little input() function. I needed this so I don't have
   to use all the bull in the main input(). Plus to use it I'd have
   to call mapAll() first and the client would read in the default
   server and then call it up before I can select a server.  */

void
metainput(void)
{
    W_Event data;

    while (W_IsMapped(metaWin) && pid != 0) {
	W_GetEvent(&data);
	switch ((int) data.type) {
	case W_EV_KEY:
	    if (data.Window == metaWin)
		metaaction(&data);
	    break;
	case W_EV_BUTTON:
	    if (data.Window == metaWin)
		metaaction(&data);
	    break;
	case W_EV_EXPOSE:
	    break;
	default:
	    break;
	}
    }
}
