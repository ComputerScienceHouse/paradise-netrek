/*
 * file.c
 */

#include "config.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "struct.h"
#include "common.h"
#include "db.h"
#include "main.h"
#include "file.h"
#include "ppeddata.h"


int
ReadIt(char *fn)
{
	struct statentry player;
	int plfd;

	initDB();	/* set up linked-list */

	plfd = open(fn, O_RDONLY, 0644);
	if(plfd < 0) {
		err_sys("Could not open %s for read", fn);	
		return 1;
	}

	while (read(plfd, (void *)&player, sizeof(struct statentry)) ==
			sizeof(struct statentry))
		addDB(&player); /* add to list */

	close(plfd);
	return 0;
}

int
SaveIt(char *fn)
{
	struct plnode *p;
	int plfd, cc;

	plfd = open(fn, O_CREAT | O_TRUNC | O_WRONLY, 0600);
	if(plfd < 0) {
		err_sys("Could not open %s for write", fn);
		return 1;
	}

	p = firstEnt;
	while(p) {
		cc = write(plfd, (void *)(&p->player), (int)sizeof(struct statentry));
		if(cc != (int)sizeof(struct statentry)) {
			err_sys("Write error");
			return 1;
		}
		p = p->next;
	}
	return 0;
}

int
DoSave(int mode)
{
	char name[100], *c;
	extern char *playerFile;

	printf("Warning! Do not write over the server .players file if there\nare people logged in!\n");
	printf("Enter filename, or press return to cancel\n");
	printf("   -->"); fflush(stdout);

	if(!fgets(name, 100, stdin)) {
		err_sys("fgets fail (in DoSave)");
		return 1;
	}
	if(c = strrchr(name, '\n'))
		*c = (char)0;

	if(!name || !(*name)) return 1;

	if(SaveIt(name)) {
		Report("");
		return 1;
	}

	ClearChanged();
	Report("Player database saved.");
	return(0);
}

int
DoLoad(int mode)
{
}

