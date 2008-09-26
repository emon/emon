/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 * started:   2001/06/16
 *
 * $Id: fecenc.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h> /* pow() */

#include "def_value.h"
#include "libfec.h"
#include "rs_emon.h"
#include "pipe_hd.h"
#include "timeval.h"
#include "debug.h"

static struct opt_values {	/* set by getenv and/or getopt */
	int             fec_type;
	int             rs_M, rs_N, rs_F;
	int             rs_K, rs_NN;	/* not identical variable */
	int             clock, freq, plen;
}               OPT;

struct timeval  total_start, total_end;
int             total_send = 0;
int             total_drop = 0;

int
opt_etc(int argc, char *argv[])
{
	int             ch;	/* for getopt */

	char           *fec_type_name, *temp;
	char           *opts = "T:C:R:L:M:N:F:D:";
	char           *helpmes =
	"usage : %s [-T fec type name] [-C clock] [-R freq]\n"
	"                 [-L packet len] [-M n] [-N n] [-F n] [-D n]\n"
	" options\n"
	" <general>\n"
	"  T <str> : FEC type name (\"RS\" only)\n"
	"  C <n>   : clock base\n"
	"  R <n>   : frequency\n"
	"  L <n>   : length of a element\n"
	" <Read Solomon>\n"
	"  M <n>   : GF(2^M), No RS if M=0\n"
	"  N <n>   : # of total packets\n"
	"  F <n>   : # of RS packets\n"
	" <debug>\n"
	"  D <n>   : debug level\n"
	               ;
	/* set default value */
	OPT.rs_M = DEF_EMON_RS_M;
	OPT.rs_N = DEF_EMON_RS_N;
	OPT.rs_F = DEF_EMON_RS_F;
	OPT.plen = DEF_EMON_PLEN;

	/* get environment variables */

	if ((temp = getenv("EMON_FEC_TYPE")) != NULL)
		fec_type_name = temp;
	if ((temp = getenv("EMON_CLOCK")) != NULL)
		OPT.clock = atoi(temp);
	if ((temp = getenv("EMON_FREQ")) != NULL)
		OPT.freq = atoi(temp);
	if ((temp = getenv("EMON_PLEN")) != NULL)
		OPT.plen = atoi(temp);
	if ((temp = getenv("EMON_RS_M")) != NULL)
		OPT.rs_M = atoi(temp);
	if ((temp = getenv("EMON_RS_N")) != NULL)
		OPT.rs_N = atoi(temp);
	if ((temp = getenv("EMON_RS_F")) != NULL)
		OPT.rs_F = atoi(temp);

	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'T':
			break;
		case 'C':
			OPT.clock = atoi(optarg);
			break;
		case 'R':
			OPT.freq = atoi(optarg);
			break;
		case 'L':
			OPT.plen = atoi(optarg);
			break;
		case 'M':	/* RS M */
			OPT.rs_M = atoi(optarg);
			break;
		case 'N':	/* RS N */
			OPT.rs_N = atoi(optarg);
			break;
		case 'F':	/* RS F */
			OPT.rs_F = atoi(optarg);
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
	}

	if (OPT.rs_M == 0) {
		OPT.rs_NN = OPT.rs_N;
		OPT.rs_F = 0;
	} else {
		OPT.rs_NN = pow(2, OPT.rs_M) - 1;
	}
	OPT.rs_K = OPT.rs_N - OPT.rs_F;
	fec_param_set(OPT.rs_M, OPT.rs_K, OPT.rs_F, OPT.plen);
	d_printf(", M/K/F/S=%d/%d/%d/%d\n",
		 OPT.rs_M, OPT.rs_K, OPT.rs_F, OPT.plen);

	return 0;
}

int
next_fec_get(char *buf)
{
	static int      c = 0;
	static int      fecadd_usec = 0;
	struct timeval  tv1, tv2;

	if (OPT.rs_M == 0)
		return 0;	/* do nothing */

	gettimeofday(&tv1, NULL);
	fecs_fecadd(buf);
	gettimeofday(&tv2, NULL);
	fecadd_usec += timeval_diff_usec(&tv2, &tv1);
	if (++c >= 100) {
		d1_printf("\nfecadd time=%dusec", fecadd_usec / 100);
		c = 0;
		fecadd_usec = 0;
	}
	return 0;
}

#define STDIN 0
#define STDOUT 1

int
main(int argc, char *argv[])
{
	char           *buffer;
	char            header[PIPE_HEADER_LEN];

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
	if ((buffer = malloc(OPT.rs_NN * OPT.plen)) == NULL) {
		e_printf("cannot malloc for buffer\n");
		return -1;
	}
	memset(header, 0, PIPE_HEADER_LEN);
	memset(buffer, 0, OPT.rs_NN * OPT.plen);

	while (pipe_blocked_read_message(STDIN, header, buffer, OPT.rs_NN*OPT.plen)>=0) {
		int             ret;

		if(pipe_get_length(header)>(OPT.rs_N-OPT.rs_F)*OPT.plen) {
			d1_printf("skip too large data (%d)\n",pipe_get_length(header));
			continue;
		}

		ret = next_fec_get(buffer);
		if (ret == -1) {
			e_printf("\nnext_fec_get failed");
			return 1;
		}

		pipe_set_length(header, OPT.rs_N * OPT.plen);
		memcpy(buffer+OPT.rs_K*OPT.plen,buffer+(OPT.rs_NN-OPT.rs_F)*OPT.plen,OPT.rs_F*OPT.plen);
		pipe_blocked_write_block(STDOUT, header, buffer);
		memset(buffer, 0, OPT.rs_NN * OPT.plen);
	}

	free(buffer);

	return 0;
}
