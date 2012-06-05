/*
 * ranklist.c
 *
 * Kevin P. Smith 12/5/88
 *
 */
#include "copyright2.h"

#include "config.h"
#include <stdio.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

/* Prototypes */
static void print_ranks_paradise P((void));


void
ranklist(void)
{
    register int i;
    char    buf[80];

    /*
       W_ClearWindow(rankw);
    */
    if (!paradise) {
	(void) strcpy(buf, "  Rank       Hours  Defense  Ratings      DI");
	W_WriteText(rankw, 1, 1, textColor, buf, strlen(buf), W_BoldFont);
	for (i = 0; i < NUMRANKS; i++) {
	    sprintf(buf, "%-11.11s %5.0f %8.2f %8.2f   %7.2f",
		    ranks[i].name,
		    ranks[i].hours,
		    ranks[i].defense,
		    ranks[i].ratings,
		    ranks[i].ratings * ranks[i].hours);
	    if (mystats->st_rank == i) {
		W_WriteText(rankw, 1, i + 2, W_Cyan, buf, strlen(buf), W_BoldFont);
	    } else {
		W_WriteText(rankw, 1, i + 2, textColor, buf, strlen(buf),
			    W_RegularFont);
	    }
	}
	strcpy(buf, "To achieve a rank, you need a high enough defense, and");
	W_WriteText(rankw, 1, i + 3, textColor, buf, strlen(buf), W_RegularFont);
	strcpy(buf, "either enough hours, and bombing + planet + offense ratings");
	W_WriteText(rankw, 1, i + 4, textColor, buf, strlen(buf), W_RegularFont);
	strcpy(buf, "above shown ratings, or too few hours, and a DI rating above");
	W_WriteText(rankw, 1, i + 5, textColor, buf, strlen(buf), W_RegularFont);
	strcpy(buf, "the shown DI rating.");
	W_WriteText(rankw, 1, i + 6, textColor, buf, strlen(buf), W_RegularFont);
    } else {			/* else we are in a paradise server */
	print_ranks_paradise();
    }
}



static void
print_ranks_paradise(void)
{
    register int i;
    char    buf[80];

    W_ResizeText(rankw, 65, nranks2 + 8);

    (void) strcpy(buf, "  Rank       genocides  DI    battle strategy  special ships");
    W_WriteText(rankw, 1, 1, textColor, buf, strlen(buf), W_BoldFont);
    for (i = 0; i < nranks2; i++) {
	sprintf(buf, "%-11.11s %5d %8.2f %8.2f %8.2f   %7.2f",
		ranks2[i].name,
		ranks2[i].genocides,
		ranks2[i].di,
		ranks2[i].battle,
		ranks2[i].strategy,
		ranks2[i].specship);
	if (mystats->st_rank == i) {
	    W_WriteText(rankw, 1, i + 2, W_Cyan, buf, strlen(buf), W_BoldFont);
	} else {
	    W_WriteText(rankw, 1, i + 2, textColor, buf, strlen(buf), W_RegularFont);
	}
    }
    strcpy(buf, "To achieve a rank, you need a high enough number of");
    W_WriteText(rankw, 1, i + 3, textColor, buf, strlen(buf), W_RegularFont);
    strcpy(buf, "genocides, a high enough DI, a high enough battle");
    W_WriteText(rankw, 1, i + 4, textColor, buf, strlen(buf), W_RegularFont);
    strcpy(buf, "rating, a high enough strategy rating, and a high");
    W_WriteText(rankw, 1, i + 5, textColor, buf, strlen(buf), W_RegularFont);
    strcpy(buf, "enough special ship rating");
    W_WriteText(rankw, 1, i + 6, textColor, buf, strlen(buf), W_RegularFont);
}
