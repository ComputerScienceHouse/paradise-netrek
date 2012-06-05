#include <stdarg.h>

/*
 * prototypes for main.c
 */

#ifndef MAIN_H
#define MAIN_H

#include "config.h"

void err P((char *s, ...));
void err_sys P((char *s, ...));
void GoAway P((int type));

#endif
