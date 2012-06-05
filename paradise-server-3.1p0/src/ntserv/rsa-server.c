/* rsa_key.c

 * Mike Polek   11/92
 */
/*#include "copyright2.h"*/
#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "proto.h"

#ifdef AUTHORIZE

void
makeRSAPacket(struct rsa_key_spacket *packet)
{
    int     i;

    for (i = 0; i < KEY_SIZE; i++)
	packet->data[i] = random() % 256;
    /* make sure it is less than the global key */
    for (i = KEY_SIZE - 10; i < KEY_SIZE; i++)
	packet->data[i] = 0;
    packet->type = SP_RSA_KEY;
}

 /* returns 1 if the user verifies incorrectly */

int
decryptRSAPacket(struct rsa_key_spacket *spacket, 
                 struct rsa_key_cpacket *cpacket, 
		 char *serverName)
{
    struct rsa_key key;
    struct sockaddr_in saddr;
    unsigned char temp[KEY_SIZE], *data;
    char   *fname;
    char    format[128];
    int     fd;
    FILE   *logfile;
    int     done, found, curtime, len;

    strcpy(RSA_client_type, "unacceptable key");

    len = sizeof(saddr);
    if (getsockname(sock, &saddr, &len) < 0) {
	perror("getsockname(sock)");
	exit(1);
    }
    /* replace the first few bytes of the message */
    /* will be the low order bytes of the number */
    data = spacket->data;
    memcpy(data, &saddr.sin_addr.s_addr, sizeof(saddr.sin_addr.s_addr));
    data += sizeof(saddr.sin_addr.s_addr);
    memcpy(data, &saddr.sin_port, sizeof(saddr.sin_port));

    fname = build_path(RSA_KEY_FILE);
    fd = open(fname, O_RDONLY);
    if (fd < 0) {
	fprintf(stderr, "Can't open the RSA key file %s\n", fname);
	strcpy(RSA_client_type, "unable to verify key");
	return 1;
    }
    done = found = 0;
    do {
	if (read(fd, &key, sizeof(struct rsa_key)) != sizeof(struct rsa_key))
	    done = 1;
	if (!(memcmp(key.global, cpacket->global, KEY_SIZE) ||
	      memcmp(key.public, cpacket->public, KEY_SIZE)))
	    done = found = 1;
    } while (!done);

    close(fd);

    /* If he wasn't in the file, kick him out */
    if (!found) {
	pmessage2("Key not in catalog", 0, MALL, "GOD->ALL", me->p_no);
	warning("Key not in catalog");
	return 1;
    }
    rsa_encode(temp, cpacket->resp, key.public, key.global, KEY_SIZE);

    /* If we don't get the right answer, kick him out */
    if (memcmp(temp, spacket->data, KEY_SIZE)) {
	pmessage2("incorrect encryption", 0, MALL, "GOD->ALL", me->p_no);
	warning("incorrect encryption");
	return 1;
    }

    fname = build_path(LOGFILENAME);
    logfile = fopen(fname, "a");
    if (logfile) {
	curtime = time(NULL);
	sprintf(format, "Client: %%.%ds. Arch: %%.%ds. Player: %%s. Login: %%s. at %%s",
		KEY_SIZE, KEY_SIZE);
	fprintf(logfile, format,
		key.client_type,
		key.architecture,
		me->p_name,
		me->p_login,
		ctime(&curtime));

	fclose(logfile);
    }

    sprintf(RSA_client_type, "%s / %s",	/* LAB 4/1/93 */
	    key.client_type, key.architecture);
    RSA_client_type[64] = '\0';

    return 0;
}

#endif
