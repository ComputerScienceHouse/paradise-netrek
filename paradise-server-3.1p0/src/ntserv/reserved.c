
/* reserved.c

 * Kevin P. Smith   7/3/89
 */
#include "config.h"
#include <stdlib.h>

#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "packets.h"
#include "gppackets.h"

#ifdef AUTHORIZE
void
makeReservedPacket(struct reserved_spacket *packet)
{
    int     i;

    for (i = 0; i < 16; i++) {
	packet->data[i] = random() % 256;
    } packet->type = SP_RESERVED;
}

void
encryptReservedPacket(struct reserved_spacket *spacket, 
                      struct reserved_cpacket *cpacket, 
		      char *server, int pno)
{

    memcpy(cpacket->data, spacket->data, 16);
    memcpy(cpacket->resp, spacket->data, 16);
    cpacket->type = CP_RESERVED;

    /*
       Encryption algorithm goes here. Take the 16 bytes in cpacket->data,
       and create cpacket->resp, which you require the client to also do.  If
       he fails, he gets kicked out.
    */
}
#endif
