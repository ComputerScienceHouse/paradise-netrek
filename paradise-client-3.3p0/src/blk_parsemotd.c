/*
 *
 */

#include "config.h"
#include "str.h"

#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

void
blk_parsemotd(char *line)
{
    /*
       Verify it's our line.
    */

    if (strncmp("BLK: ", line, 5) != 0)
	return;

    /*
       See if it's a refit string.
    */

    if (strncmp(&line[5], "REFIT", 5) == 0) {
	strncpy(blk_refitstring, &line[10], 79);
	blk_refitstring[79] = '\0';
    }
    /*
       Check to see if it's a borgish feature being enabled.
    */
    else if (strncmp(&line[5], "BORGISH ", 8) == 0) {
	if (strncmp(&line[13], "FRCLOAK", 7) == 0)
	    blk_friendlycloak = 1;
    }
}
