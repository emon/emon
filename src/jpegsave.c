/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 * started:   2001/06/26
 *
 * $Id: jpegsave.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include "def_value.h"
#include "libfec.h"
#include "pipe_hd.h"
#include "debug.h"

#define FMT_MEDIA "%04d.jpg"

#ifndef FASE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif


static struct opt_values {	/* set by getenv and/or getopt */
	char           *fmt_media;
	int             rs_N, rs_F;
	int             rs_K;	/* not identical variable */
	int             plen;
	int             num_save;
	int             num_skip;
	int		fclear;	/* flag: save only clear (no erro) jpeg */
	int		num_clear;
} OPT;

int             debug_level;

int
opt_etc(int argc, char *argv[])
{
	int             ch;	/* for getopt */

	char           *temp;
	char           *opts = "cC:m:n:s:L:N:F:D:";
	char           *helpmes =
	"usage : %s [-c] [-C <n>] [-m fmt_media] [-n <n>] [-s <n>]\n"
	"            [-L packet len] [-N n] [-F n] [-D n]\n"
	" options\n"
	" <general>\n"
	"  c       : only save clear jpeg file (no error)\n"
	"  C <n>   : ignore after <n>-th packet loss\n"
	"  m <str> : Media filename format (e.g. %%04d.jpg)\n"
	"  n <n>   : save <n> frames\n"
	"  s <n>   : skip first <n> frames\n"
	"  L <n>   : length of a element\n"
	" <Read Solomon>\n"
	"  N <n>   : # of total packets\n"
	"  F <n>   : # of RS packets\n"
	" <debug>\n"
	"  D <n>   : debug level\n"
	               ;
	/* set default value */
	OPT.fmt_media = FMT_MEDIA;
	OPT.rs_N = DEF_EMON_RS_N;
	OPT.rs_F = DEF_EMON_RS_F;
	OPT.plen = DEF_EMON_PLEN;
	OPT.num_skip = 0;
	OPT.fclear = FALSE;
	OPT.num_clear = 0;
	
	/* get environment variables */

	if ((temp = getenv("EMON_PLEN")) != NULL)
		OPT.plen = atoi(temp);
	if ((temp = getenv("EMON_RS_N")) != NULL)
		OPT.rs_N = atoi(temp);
	if ((temp = getenv("EMON_RS_F")) != NULL)
		OPT.rs_F = atoi(temp);

	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'c':
			OPT.fclear = TRUE;
			break;
		case 'C':
			OPT.num_clear = atoi(optarg);
			break;
		case 'm':
			OPT.fmt_media = optarg;
			break;
		case 'n':
			OPT.num_save = atoi(optarg);
			break;
		case 's':
			OPT.num_skip = atoi(optarg);
			break;
		case 'L':
			OPT.plen = atoi(optarg);
			break;
		case 'N':	/* RS N */
			OPT.rs_N = atoi(optarg);
			break;
		case 'F':   /* RS F */
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

	OPT.rs_K=OPT.rs_N-OPT.rs_F;
	return 0;
}


int
open_nextfile(void)
{
	static int      fileno = 0;
	static char     name[255];

	int             fd = -1;/* file descriptor */

	if (OPT.num_save && fileno>OPT.num_save) {
		e_printf("stop %s\n", name);
		return -1;
	}
	snprintf(name, sizeof(name), OPT.fmt_media, fileno++);
	fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		e_printf("cannot open %s\n", name);
		return -1;
	}
	d1_printf("open:%s", name);
	return fd;
}


int
main(int argc, char *argv[])
{
	char           *buffer;
	char            header[PIPE_HEADER_LEN];
	int             eras_pos[FEC_N_MAX], no_eras, fd;
	int		s = 0, i = 0;
	int		len, ts, lts = -1;

	if (opt_etc(argc, argv) == -1) {
		return -1;
	}
	if (isatty(STDIN)) {
		e_printf("This program must be used with input"
			 " connecting to pipe.\n");
		return -1;
	}
	if ((buffer = malloc(OPT.rs_N * OPT.plen)) == NULL) {
		e_printf("Cannot malloc for buffer.\n");
		return -1;
	}
	memset(header, 0, PIPE_HEADER_LEN);
	memset(buffer, 0, OPT.rs_N * OPT.plen);

	while (pipe_blocked_read_message_ex(STDIN, header, buffer,
					    eras_pos, &no_eras, OPT.rs_N,
					    OPT.plen)!=PIPE_ERROR) {
		ts = pipe_get_timestamp(header);
		if (OPT.num_skip > 0 && s <= OPT.num_skip){
			if (s < OPT.num_skip){
				s++;
				continue;
			} else {
				d1_printf("skip:%d pics\n", s++);
			}
		}
		if (OPT.fclear == TRUE && no_eras > 0){
			if (eras_pos[0] <= OPT.num_clear){
				d1_printf("skip: broken jpeg err:%d [%d..]"
					  " ts:%u\n",
					  no_eras, eras_pos[0], ts);
				continue;
			}
		}

		if (OPT.num_save > 0 && ++i > OPT.num_save){
			d1_printf("%d files was saved. finish\n", i-1);
			break;
		}
		if ((fd = open_nextfile()) < 0) break;
		if (lts == -1){	/* 1st time */
			lts = ts;
		}
		d1_printf(" err:%d ts:%u (%6d)", no_eras, ts, ts-lts);
		lts = ts;
		if ((len = pipe_get_length(header)) < OPT.rs_K*OPT.plen){
			write(fd, buffer, len);
			d2_printf(" fec:NO  len: %d", len);
		} else {
			len = OPT.rs_K * OPT.plen;
			write(fd, buffer, len);
			d2_printf(" fec:YES len: %d ", len);
		}
		close(fd);
		d1_printf("\n");
	}
	return 0;
}
