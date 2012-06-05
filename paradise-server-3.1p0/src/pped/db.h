#ifndef DB_H
#define DB_H

#include "config.h"
#include "defs.h"
#include "struct.h"

struct plnode {
	struct statentry player;
	struct plnode    *next;
	struct plnode    *prev;
	int status;
};

/* prototypes for db.c */
void initDB P((void));
void addDB P((struct statentry *));
struct plnode *GetNode P((int));
void DeleteNode P((int));
int GetByName P((char *));

#endif
