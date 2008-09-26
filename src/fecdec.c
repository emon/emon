/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/06/19
 *
 * $Id: fecdec.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h> /* pow() */

#include "def_value.h"
#include "libfec.h"
#include "fec_decode.h"
#include "pipe_hd.h"
#include "debug.h"

static struct opt_values {	/* set by getenv and/or getopt */
	int             fec_type;
	int             rs_M, rs_N, rs_F;
	int             rs_K, rs_NN;	/* not identical variable */
	int             clock, freq, plen;
	int             bufnum, thr_with_fec, thr_without_fec, maxnum;
	int             delF;
}               OPT;

static int fecdec_delFout();

int
opt_etc(int argc, char *argv[])
{
	int             ch;	/* for getopt */

	char           *fec_type_name, *temp;
	char           *opts = "T:C:R:L:M:N:F:B:Q:q:s:D:n";
	char           *helpmes =
	  "usage : %s [-n] [-T <fec type name>] [-C <clock>] [-R <freq>]"
	  " [-L <packet len>] [-M <n>] [-N <n>] [-F <n>]"
	  " [-B <bufnum>] [-Q <thr_with_fec>] [-q <thr_without_fec>] [-s <maxnum>] [-D n] \n"
	  " options\n"
	  "  n       : don't error collection.\n"
	  " <general>\n"
	  "  T <str> : FEC type name (\"RS\" only)\n"
	  "  C <n>   : clock base\n"
	  "  R <n>   : frequency\n"
	  "  L <n>   : length of a element\n"
	  " <Read Solomon>\n"
	  "  M <n>   : GF(2^M), No RS if M=0\n"
	  "  N <n>   : # of total packets\n"
	  "  F <n>   : # of RS packets\n"
	  " <FEC decode>\n"
	  "  B <n>   : # of buffer slots\n"
	  "  Q <n>   : threshold of skipping fec decode\n"
	  "  q <n>   : threshold of skipping output\n"
	  "  s <n>   : maximum # of continuous reading\n"
	  " <debug>\n"
	  "  D <n>   : debug level\n"
	  ;
	/* set default value */
	OPT.rs_M = DEF_EMON_RS_M;
	OPT.rs_N = DEF_EMON_RS_N;
	OPT.rs_F = DEF_EMON_RS_F;
	OPT.plen = DEF_EMON_PLEN;

	OPT.bufnum = 10;
	OPT.thr_with_fec = 3;
	OPT.thr_without_fec = 0;
	OPT.maxnum = 5;
	OPT.delF  = 0;

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
 		case 'B':
			OPT.bufnum = atoi(optarg);
			break;
 		case 'Q':
			OPT.thr_with_fec = atoi(optarg);
			break;
 		case 'q':
			OPT.thr_without_fec = atoi(optarg);
			break;
 		case 's':
			OPT.maxnum = atoi(optarg);
			break;
 		case 'n':
			OPT.delF  =1;
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

#define STDIN 0
#define STDOUT 1

int
main(int argc, char *argv[])
{
	char           *buffer;
	char            header[PIPE_HEADER_LEN];
	int             maxnum;
	u_int32_t       ts;
	
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

	if(OPT.delF){
		fecdec_delFout();
		exit(0);
	}

	if ((buffer = malloc(OPT.rs_NN * OPT.plen)) == NULL) {
		e_printf("cannot malloc for buffer\n");
		return -1;
	}

	memset(header, 0, PIPE_HEADER_LEN);
	memset(buffer, 0, OPT.rs_NN * OPT.plen);
	pipe_set_version(header, 1);
	pipe_set_marker(header, 1);
	pipe_set_length(header, OPT.rs_K * OPT.plen);

	fecr_buffer_init(OPT.bufnum, OPT.thr_with_fec, OPT.thr_without_fec);
	fecr_set_fd(STDIN);

	while(1) {
		maxnum=OPT.maxnum;
		while(maxnum-->0 && fd_can_read(STDIN,0))
			fecr_recv();

		if(fecr_check()) {
		  if(fecr_read_recover(buffer,&ts)){
		    pipe_set_timestamp(header,ts);
		    pipe_blocked_write_block(STDOUT, header, buffer);
		  }
		}else{
		  fd_can_read(STDIN,1000);
		}
	}
	
	free(buffer);

	return 0;
}


static int fecdec_delFout(){
	char           *buf;
	char            p_header[PIPE_HEADER_LEN];
	int             len;
	int             buf_len;
	int             blkcnt;
	int             wait_marker;
	int             Kcnt; 
	
	d2_printf("start fecdec delFout\n");

	buf_len=OPT.rs_NN * OPT.plen;
	if ((buf = malloc(buf_len)) == NULL) {
		e_printf("cannot malloc for buf\n");
		return -1;
	}
	blkcnt=0;
	wait_marker=1;
	Kcnt=0;
	for (;;) {
		len = pipe_blocked_read_block(STDIN_FILENO, p_header
					      ,buf,buf_len);
		d3_printf("read %d.\n",len);
		if (len < 0) {
			e_printf("ERROR:pipedump read error.\n");
			exit(1);
		}
		if(wait_marker==1){
			if(pipe_get_marker(p_header)==1){
				d1_printf("found marker.\n");
				Kcnt=0;
				wait_marker=0;
			}
			continue;
		}
		if(Kcnt==OPT.rs_K){
			pipe_set_marker(p_header,1);
		}
		else if(Kcnt>OPT.rs_K){
			d2_printf("Kcnt%d OPT.rs_K %d \n"
				  ,Kcnt,OPT.rs_K);
			if(pipe_get_marker(p_header)==1){
				Kcnt=0;
			}
			continue;
		}
		Kcnt++;
		len =pipe_blocked_write_block(STDOUT_FILENO, p_header
					      ,buf);
		d3_printf("write %d.\n",len);
		if (len < 0) {
		  e_printf("ERROR:pipedump write error.\n");
		  exit(1);
		}
	}
	return 0;
}
