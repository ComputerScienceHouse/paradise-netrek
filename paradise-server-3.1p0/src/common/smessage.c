/*------------------------------------------------------------------
  Copyright 1989		Kevin P. Smith
				Scott Silvey

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies.

  NETREK II -- Paradise

  Permission to use, copy, modify, and distribute this software and
  its documentation, or any derivative works thereof,  for any 
  NON-COMMERCIAL purpose and without fee is hereby granted, provided
  that this copyright notice appear in all copies.  No
  representations are made about the suitability of this software for
  any purpose.  This software is provided "as is" without express or
  implied warranty.

	Xtrek Copyright 1986			Chris Guthrie
	Netrek (Xtrek II) Copyright 1989	Kevin P. Smith
						Scott Silvey
	Paradise II (Netrek II) Copyright 1993	Larry Denys
						Kurt Olsen
						Brandon Gillespie
		                Copyright 2000  Bob Glamm

--------------------------------------------------------------------*/

#include "config.h"
#include "proto.h"
#include "data.h"
#include "shmem.h"

/*-----------------------------VISIBLE FUNCTIONS--------------------------*/

/*---------------------------------PMESSAGE-------------------------------*/
/*  This function sends a message.  It marks a message as being sent from
God.  */

/* args:
    char   *str;		the message
    int     recip;		who will receive it
    int     group;		the group (type of recipient)
    char   *address;		attached to front of message */
void 
pmessage(char *str, int recip, int group, char *address)
{
    pmessage2(str, recip, group, address, 255);
}




/*--------------------------------PMESSAGE2--------------------------------*/
/*  This function sends a message.  It places the message in the array of
messages.  */

/* args:
    char   *str;		the message
    int     recip;		who will receive it
    int     group;		the group (type of recipient)
    char   *address;		attached to front of message
    unsigned char from;		who the message is from */
void
pmessage2(char *str, int recip, int group, char *address, unsigned char from)
{
    struct message *cur;	/* to point to where to put message */
    int     mesgnum;		/* to hold index number in message array */

    if ((mesgnum = ++(mctl->mc_current)) >= MAXMESSAGE) {
	mctl->mc_current = 0;	/* get index of where to put the message */
	mesgnum = 0;		/* roll it index number over if need be */
    }
    cur = &messages[mesgnum];	/* get address of message structure in array */
    cur->m_no = mesgnum;	/* set the message number */
    cur->m_flags = group;	/* set group or type of recipient */
    cur->m_recpt = recip;	/* set the recipient */
    cur->m_from = from;		/* set who it was from */
    (void) sprintf(cur->m_data, "%-9s ", address);
    strncat(cur->m_data, str, sizeof(cur->m_data)-strlen(cur->m_data));
    cur->m_flags |= MVALID;	/* set messages status as valid */
}

/*-------------------------------------------------------------------------*/


/*-------END OF FILE--------*/
