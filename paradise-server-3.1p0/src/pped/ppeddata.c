#include <stdlib.h>
#include "db.h"

struct plnode *firstEnt = NULL;
struct plnode *lastEnt = NULL;

int numDBentries = 0;

int dbDirty = 0;

int numLines = 24;
char *clrStr = NULL;
