/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 * started:   2001/06/16
 *
 * $Id: jpegcat.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <time.h>

#include "def_value.h"
#include "pipe_hd.h"
#include "debug.h"

#define BUFFERSIZE (1024 * 100)
#define FILE_MAX 417

#define FMT_MEDIA "%04d.jpg"

static struct opt_values {
	char           *fmt_media;
	int             clock, freq;
}               OPT;

int 
opt_etc(int argc, char *argv[])
{
	int             ch;
	char           *temp;
	char           *opts = "m:C:R:D:";
	char           *helpmes =
	"usage : %s [-m fmt_media] [-C clock] [-R freq]"
	" [-D verbose_level]\n"
	" options\n"
	" m <str> : media filename format (e.g. %%04d.jpg)\n"
	" C <n>   : clock base\n"
	" R <n>   : frequency\n"
	"<debug>\n" 
    " D <n>   : debug level\n"
	               ;

	/* set default value */
	OPT.fmt_media = FMT_MEDIA;
	OPT.clock = DEF_EMON_CLOCK;
	OPT.freq = DEF_EMON_FREQ;

	/* get environment variables */
	if ((temp = getenv("EMON_CLOCK")) != NULL)
		OPT.clock = atoi(temp);
	if ((temp = getenv("EMON_FREQ")) != NULL)
		OPT.freq = atoi(temp);


	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'm':
			OPT.fmt_media = optarg;
			break;
		case 'C':
			OPT.clock = atoi(optarg);
			break;
		case 'R':
			OPT.freq = atoi(optarg);
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
open_nextfile(void)
{
	static int      fileno = 0;
	static char     name[255];

	int             fd = -1;/* file descriptor */

	for (;;) {
		snprintf(name, sizeof(name), OPT.fmt_media, fileno);
		fd = open(name, O_RDONLY);
		if (fd == -1) {
			if (fileno == 0) {
				e_printf("cannot open %s\n", name);
				return -1;
			} else {
				d1_printf(" reset file seqnum");
				fileno = 0;
				continue;
			}
		} else {
			d1_printf("\njpegcat: open %s", name);
			break;	/* == return fd */
		}
	}
	fileno++;
	return fd;
}

int
read_next(void *buf, size_t nbytes)
{
	int             ret;

	int             rfd;

	if ((rfd = open_nextfile()) == -1) {
		return -1;
	}
	ret = read(rfd, buf, nbytes);
	d2_printf("\njpegcat: read %d", ret);
	close(rfd);

	return ret;
}

int 
main(int argc, char *argv[])
{
	char            buffer[BUFFERSIZE];
	char            p_hd[PIPE_HEADER_LEN];
	int             size, timestamp;

	if (opt_etc(argc, argv) == -1) {
		return -1;	/* quit */
	}
	if (isatty(STDOUT)) {
		e_printf("This program must be used with output connecting to pipe\n");
		return -1;
	}
	srand(time(NULL));
	memset(p_hd, 0, PIPE_HEADER_LEN);
	pipe_set_version(p_hd, 1);
	pipe_set_marker(p_hd, 1);
	timestamp = rand();
	for (;;) {
		if ((size = read_next(buffer, BUFFERSIZE)) < 0) {
			return -1;
		}
		pipe_set_length(p_hd, size);
		pipe_set_timestamp(p_hd, timestamp);
		pipe_blocked_write_block(STDOUT, p_hd, buffer);

		timestamp += OPT.freq;
	}

	return 0;
}
