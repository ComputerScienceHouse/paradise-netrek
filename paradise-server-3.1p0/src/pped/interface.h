/*
 * Prototypes for interface.c
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include "config.h"
#include "db.h"

RETSIGTYPE getTTYinfo P((int));
void cls P((void));
void Interface P((void));
void Edit P((int));
int Verify P((char *));
void Report P((char *));
void Change P((int, struct plnode *));
void Display P((struct plnode *, int, int));
int CheckChanged P((void));
void ClearChanged P((void));
char *Strip P((char *));

#endif
