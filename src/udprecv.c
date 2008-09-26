/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 * started:   2001/06/16
 *
 * $Id: udprecv.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "def_value.h"
#include "multicast.h"
#include "pipe_hd.h"
#include "debug.h"

static struct opt_values {	/* set by getopt */
	char           *addr;
	int             port;
	char           *ifname;
	int		ipver;
	int		plen;
}               OPT;

int             debug_level;

int
opt_etc(int argc, char *argv[])
{
	int             ch;	/* for getopt */
	char           *temp;
	char           *opts = "A:P:V:I:D:";
	char           *helpmes =
	"usage : %s [-A <IP-address>] [-P <port>] [-I <interface>]"
	" [-V <IP-ver>] [-D <debug_level>]\n"
	" options\n"
	" <network>\n"
	"  A <s>   : IP address\n"
	"  P <n>   : port #\n"
	"  I <s>   : multicast interface\n"
	"  V <n>   : IP version # (4 or 6)\n"
	" <debug>\n"
	"  D <n>   : debug level\n"
	               ;
	/* set default value */
	OPT.addr = DEF_RTPRECV_ADDR;
	OPT.port = DEF_RTPRECV_PORT;
	OPT.ifname = DEF_RTPRECV_IF;
	OPT.plen = DEF_EMON_PLEN;
	OPT.ipver = 0;
	
	/* get environment variables */

	if ((temp = getenv("RTPRECV_ADDR")) != NULL)
		OPT.addr = temp;
	if ((temp = getenv("RTPRECV_PORT")) != NULL)
		OPT.port = atoi(temp);
	if ((temp = getenv("RTPRECV_IF")) != NULL)
		OPT.ifname = temp;
	if ((temp = getenv("EMON_PLEN")) != NULL)
		OPT.plen = temp;

	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'A':	/* IP address */
			OPT.addr = optarg;
			break;
		case 'P':	/* port# */
			OPT.port = atoi(optarg);
			break;
		case 'I':
			OPT.ifname = optarg;
			break;
		case 'V':
			OPT.ipver = atoi(optarg);
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	char            *buffer, p_header[PIPE_HEADER_LEN];
	int             mfd;	/* multicast fd */
	int             size;

	if (opt_etc(argc, argv) == -1) {
		return -1;	/* quit */
	}
	if ((buffer = malloc(OPT.plen))== NULL){
		e_printf("cannot malloc for buffer\n");
		return -1;
	}
	if (isatty(STDOUT)) {
		e_printf("Standard output must be connected with pipe\n");
		return -1;
	}
#if 0
	if (OPT.ifname)
		printf("%s\n", OPT.ifname);
	else
		printf("NULL\n");
#endif

	mfd = recvsock_setup(OPT.addr, OPT.port, OPT.ifname, OPT.ipver);

	memset(p_header, 0, PIPE_HEADER_LEN);
	pipe_set_version(p_header, 1);

	while (fd_can_read_blocked(mfd)) {
		size = read(mfd, buffer, sizeof(buffer));
#if 0
		/* Assume RTP packet */
		pipe_set_marker(rtp_get_marker(buffer));
		pipe_set_timestamp(rtp_get_timestamp(buffer));
#endif
		pipe_set_length(p_header, size);
		pipe_blocked_write_block(STDOUT, p_header, buffer);
		d3_printf("read %d and write\n", size);
	}

	multicast_leave(mfd, OPT.addr, OPT.ifname, OPT.ipver);
	close(mfd);

	return 0;
}
