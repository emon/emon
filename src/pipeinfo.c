/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): FUJIKAWA Kenji <fujikawa@real-internet.org>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/08/12
 *
 * $Id: pipeinfo.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <machine/limits.h>

#include "pipe_hd.h"
#include "rtp_hd.h"

#define HTYPE_MAX	3	/* number of types */
#define EPI_BUFSIZE 	0x10000 /* EMON PIPEINFO BUFSIZE */

enum hdr_type {
	HTYPE_EMON_RTP,
	HTYPE_EMON,
	HTYPE_RTP		/* it does not work, no length field */
};

typedef struct opt_values {
	enum hdr_type type;
	int hstep;		/* print header interval */
	pipe_hd_t *phdr;	/* pipe(EMON) header */
	rtp_hd_t *rhdr;		/* ptr RTP header */
} opt_t;


static char *hdr_mes[HTYPE_MAX] = {
	"P:len m timestamp ( diff)  R: m seq#    ts\n",
	"P:len m timestamp ( diff)\n",
	"R: m seq#    ts\n"
};


void usage();
void print_header(int type);
static int opt_etc(int argc, char *argv[], opt_t *opt);


void
print_header(int type)
{
	if (type < 0 || HTYPE_MAX < type){
		fprintf(stderr, "-\n");
		return;
	}
	fprintf(stderr, hdr_mes[type]);
	return;
}


int
anlys_emon_rtp(opt_t *opt)
{
	int len, len2, m;
	unsigned long pts;

	static unsigned long lpts = 0;
	
	len = fd_read_ignoreEOF(0, opt->phdr, PIPE_HEADER_LEN);

	if (len != PIPE_HEADER_LEN) {
		fprintf(stderr, "EMON header size error (len=%d)\n", len);
		exit(1);
	}
	len = ntohl(opt->phdr->vmlen)&0x00ffffff;
	if (PIPE_HEADER_LEN + len > EPI_BUFSIZE) {
		fprintf(stderr, "buffer overflow (len=%d)\n", len);
		exit(1);
	}
	if ((len2 = fd_read_ignoreEOF(0, opt->rhdr, len)) != len){
		fprintf(stderr, 
			"size is illeagal len=%d, len2=%d\n", 
			len, len2);
		exit(1);
	}
	m = pipe_get_marker(opt->phdr);
	pts = ntohl(opt->phdr->ts);
	if (lpts == 0){		/* first time */
		lpts = pts;
	}
		
	fprintf(stderr,
		"%5d %1d %010lu(%5ld) 0x%02x 0x%02x %07d %010lu\n",
		len, m, pts, pts-lpts,
		opt->rhdr->vpxc, opt->rhdr->mp, ntohs(opt->rhdr->seq),
		ntohl(opt->rhdr->ts));
	lpts = pts;
	return 0;
}


int
anlys_emon(opt_t *opt)
{
	int len, len2, m;
	unsigned long pts;

	static unsigned long lpts = 0;
	
	len = fd_read_ignoreEOF(0, opt->phdr, PIPE_HEADER_LEN);

	if (len != PIPE_HEADER_LEN) {
		fprintf(stderr, "EMON header size error (len=%d)\n", len);
		exit(1);
	}

	len = ntohl(opt->phdr->vmlen)&0x00ffffff; /* payload length */
	if (PIPE_HEADER_LEN + len > EPI_BUFSIZE) {
		fprintf(stderr, "buffer overflow (len=%d)\n", len);
		exit(1);
	}
	if ((len2 = fd_read_ignoreEOF(0, opt->phdr+1, len)) != len){
		fprintf(stderr, 
			"size is illeagal len=%d, len2=%d\n", 
			len, len2);
		exit(1);
	}
	m = pipe_get_marker(opt->phdr);
	pts = ntohl(opt->phdr->ts);
	if (lpts == 0){		/* first time */
		lpts = pts;
	}
	fprintf(stderr,
		"%5d %1d %010lu(%5ld)\n",
		len, m, pts, pts-lpts);
	lpts = pts;
	return 0;
}

				

int
main(int argc, char *argv[])
{
	char buf[EPI_BUFSIZE];
	unsigned long i;
	opt_t opt;
	
	opt_etc(argc, argv, &opt);

	switch (opt.type){
	case HTYPE_EMON_RTP:
		opt.phdr = (pipe_hd_t*)buf;
		opt.rhdr = (rtp_hd_t*)(opt.phdr+1);
		break;
	case HTYPE_EMON:
		opt.phdr = (pipe_hd_t*)buf;
		opt.rhdr = NULL;
		break;
	case HTYPE_RTP:
		opt.phdr = NULL;
		opt.rhdr = (rtp_hd_t*)buf;
	default:
		fprintf(stderr, "invalid type %d\n", opt.type);
		break;
	}

	/*fcntl(0, F_SETFL, fcntl(0, F_GETFL)|O_NONBLOCK);*/
	for (i = 0;; i++) {
		if (opt.hstep > 0 && i % opt.hstep == 0){
			print_header(opt.type);

			if (i >= ULONG_MAX - opt.hstep){
				fprintf(stderr, "# reset i (%lu to 0)\n", i);
				i = 0;
			}
		}
#if 0
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		select(0+1, &rfds, NULL, NULL, NULL);
#endif
		switch (opt.type){
		case HTYPE_EMON_RTP:
			anlys_emon_rtp(&opt);
			break;
		case HTYPE_EMON:
			anlys_emon(&opt);
			break;
		case HTYPE_RTP:
			break;
		}
	}
	return (0);
}


static int
opt_etc(int argc, char *argv[], opt_t *opt)
{
	int ch;
	char *opts = "t:s:";
	char *helpmes =
		"usage : %s [-t b|e|r] [-s <n>]\n"
		" (read stream from STDIN)\n"
		" options:\n"
		" -t b\tEMON + RTP header\n"
		" -t e\tEMON header\n"
		" -t r\tRTP header\n"
		;

	opt->type = HTYPE_EMON_RTP;	/* default */
	opt->hstep = 25;	/* skip 25 lines */
	
	while ((ch = getopt(argc, argv, opts)) != -1){
		switch (ch){
		case 't':
			switch (tolower(optarg[0])){
			case 'b':
				opt->type = HTYPE_EMON_RTP;
				break;
			case 'e':
				opt->type = HTYPE_EMON;
				break;
			case 'r':
				opt->type = HTYPE_RTP;
				break;
			default:
				fprintf(stderr, helpmes, argv[0]);
				return -1;
			}
			break;
		case 's':
			opt->hstep = atoi(optarg);
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
	}
	return 0;
}

