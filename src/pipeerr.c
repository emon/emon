/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 * started:   2001/06/26
 *
 * $Id: pipeerr.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <string.h>

#include "def_value.h"
#include "pipe_hd.h"
#include "timeval.h"
#include "debug.h"

static struct opt_values {	/* set by getenv and/or getopt */
	int             plen;
	int             rs_N;
	int             send_err_rate;
	int             send_err_pos;
	int             send_double;
}               OPT;

struct timeval  total_start, total_end;
int             total_send = 0;
int             total_drop = 0;

int             debug_level;

int
opt_etc(int argc, char *argv[])
{
	int             ch;	/* for getopt */

	char           *temp;
	char           *opts = "L:N:D:e:E:123";
	char           *helpmes =
	"usage : %s [-L <packet len>] [-N <n>] [-D <debug level>]"
    " [-e <error rate>] [-E <error pos>] [-123]\n"
	" options\n"
	" <general>\n"
	"  L <n>   : length of a element\n"
	" <Read Solomon>\n"
	"  N <n>   : # of total packets\n"
	" <debug>\n"
	"  D <n>   : debug level\n"
	"  e <n>   : error rate (100 -> 1/100, 1000 -> 1/1000 ...)\n"
    "  E <n>   : make error at this index in a message(1 origin)\n"
	"  1/2/3   : packet rate normal/double/triple\n"
	               ;
	/* set default value */
	OPT.rs_N = DEF_EMON_RS_N;
	OPT.plen = DEF_EMON_PLEN;
	OPT.send_double=1;

	/* get environment variables */

	if ((temp = getenv("EMON_PLEN")) != NULL)
		OPT.plen = atoi(temp);
	if ((temp = getenv("EMON_RS_N")) != NULL)
		OPT.rs_N = atoi(temp);

	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'L':
			OPT.plen = atoi(optarg);
			break;
		case 'N':	/* RS N */
			OPT.rs_N = atoi(optarg);
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		case 'e':
			OPT.send_err_rate=atoi(optarg);
			break;
		case 'E':
			OPT.send_err_pos=atoi(optarg);
			break;
		case '1':
		case '2':
		case '3':
			OPT.send_double=ch-'0';
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
		e_printf("\nOPT.send_error_rate=%d, err_pos=%d\n", OPT.send_err_rate, OPT.send_err_pos);
	}
	return 0;
}


int
main(int argc, char *argv[])
{
	char           *buffer;
	char            header[PIPE_HEADER_LEN];
	int             i;

	if (opt_etc(argc, argv) == -1) {
		return -1;
	}
	if (isatty(STDIN)) {
		e_printf("This program must be used with input connecting to pipe\n");
		return -1;
	}
	if (isatty(STDOUT)) {
		e_printf("This program must be used with output connecting to pipe\n");
		return -1;
	}
	if ((buffer = malloc(OPT.rs_N * OPT.plen)) == NULL) {
		e_printf("cannot malloc for buffer\n");
		return -1;
	}
	memset(header, 0, PIPE_HEADER_LEN);
	memset(buffer, 0, OPT.rs_N * OPT.plen);

	srand(time(NULL));

	while (pipe_blocked_read_message(STDIN, header, buffer, OPT.rs_N * OPT.plen) != PIPE_ERROR) {
		for(i=0;i<OPT.rs_N;i++) {
			if(i==OPT.rs_N-1)
				pipe_set_marker(header,1);
			else
				pipe_set_marker(header,0);
			if(i+1==OPT.send_err_pos || (OPT.send_err_rate>0 && (random() % OPT.send_err_rate) == 0)){
				e_printf("%s: drop\n", argv[0]);
				pipe_set_length(header,0);
			} else
				pipe_set_length(header, OPT.plen);
			pipe_blocked_write_block(STDOUT, header, buffer+i*OPT.plen);
		}
		memset(buffer, 0, OPT.rs_N * OPT.plen);
	}

	return 0;
}
