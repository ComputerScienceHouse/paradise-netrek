/*
 * db.c
 */


#include <stdio.h>
#include <stdlib.h>
#include <struct.h>
#include "common.h"
#include "db.h"
#include "main.h"
#include "ppeddata.h"

void
initDB(void)
{
	struct plnode *p, *pn;

	/* deallocate previous db */
	p = firstEnt;
	if(numDBentries) {
		while(numDBentries) {
			pn = p->next;

			if(!pn) err_fatal("initDB: numDBentries too large!");

			free(p);
			p = pn;
			numDBentries--;
		}
		if(!p) err_fatal("initDB: numDBentries too small!");
	}

	firstEnt = 0;
	lastEnt = 0;
	numDBentries = 0;
	dbDirty = 0;
}

void
addDB(struct statentry *player)
{
	struct plnode *p;

	p = (struct plnode *)malloc(sizeof(struct plnode));

	if(p == NULL)
		err_sys_fatal("addDB: malloc error");

	p->player = *player;
	p->next = 0;
	p->status = 0;

	if(!firstEnt) {
		p->prev = 0;
		firstEnt = p;
		lastEnt = p;
	} else {
		p->prev = lastEnt;
		lastEnt->next = p;
		lastEnt = p;
	}
	numDBentries++;
}

struct plnode *
GetNode(int n)
{
	struct plnode *p, *pn;
	int i = 0;

	p = firstEnt;

	while(p) {
		if(i++ == n) return(p);
		pn = p->next;
		p = pn;
	}
	return(NULL);
}

void
DeleteNode(int n)
{
	struct plnode *p, *pn, *pp;
	int i;

	if(!numDBentries) return;

	p = firstEnt;
	if(!p) return;

	for(i = 0; i < n; i++) {
		p = p->next;
		if(!p) return;
	}
	pn = p->next;
	pp = p->prev;
	if(pn) pn->prev = pp;
	if(pp) pp->next = pn;

	if(!n) firstEnt = pn;
	numDBentries--;

	dbDirty = 1;

	free(p);
}

int
GetByName(char *name)
{
	struct plnode *p;
	int i;

	if(!numDBentries) return -1;

	p = firstEnt;
	if(!p) return -1;

	for(i = 0; p; i++) {
		if(!strncmp(name, p->player.name, 16))
			return i;
		p = p->next;
	}
	return -1;
}

