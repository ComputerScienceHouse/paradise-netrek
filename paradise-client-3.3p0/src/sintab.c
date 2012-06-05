/* $Id: sintab.c,v 1.2 2000/02/03 00:33:12 glamm Exp $ */

/*
 * sintab.c
 *
 * Also initialize blk_giwdth to 100000 and blk_windgwidth
 * to WINSIDE/blk_gwidth
 */
#include "copyright.h"
#include "config.h"
#include <math.h>
#include <assert.h>
#include <stdlib.h>

double *Cos = NULL;
double *Sin = NULL;

#define WINSIDE 500
extern int blk_gwidth;
extern float blk_windgwidth;

void
inittrigtables(void)
{
  int i;
  double a;

  Sin = (double *)malloc(sizeof(double) * (256 + 64));
  assert(Sin != NULL);
  Cos = Sin + 64;
  for(i = -64; i < 256; i++)
  {
    a = 2.0 * M_PI * ((double)(i) / 256.0);
    Sin[i + 64] = sin(a);
  }
  blk_gwidth = 100000;
  blk_windgwidth = ((float) WINSIDE) / blk_gwidth;
}
