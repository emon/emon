/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/10/03
 *
 * $Id: pipesave.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include <signal.h>

#include "debug.h"
#include "pipe_hd.h"


#define ENDIAN_LITTLE

static char *DEFAULT_DUMPFILE="/tmp/dump.emon";

typedef struct opt_valuses {
	char           *dumpfile;
	int  blklimit;
}               opt_t;		/* as audiocap_t */

static opt_t    OPT;

int             opt_etc(int argc, char *argv[]);

static int exit_req;

void cb_sig(int signo){
  exit_req=1;
}


int
main(int argc, char **argv)
{
	char           *malloc_tmp;
	int             dumpfd;	/* audio fd */
	int             len;
	char            p_header[PIPE_HEADER_LEN];
	int             blkcnt;
	int             wait_marker;
	sigset_t        sigset;

	sigfillset(&sigset);
	sigdelset(&sigset,SIGINT);
	if (sigprocmask(SIG_BLOCK,&sigset,NULL)!=0){
	  e_printf("sigprocmask fail.\n");
	  exit(0);
	}
	signal(SIGINT,cb_sig);

	if (opt_etc(argc, argv) < 0) {
		return -1;
	}

	if ((dumpfd = open(OPT.dumpfile,O_CREAT|O_WRONLY,S_IRWXU)) < 0) {
		e_printf("%s cannot open %s . exit.\n",argv[0],OPT.dumpfile);
		return -1;
	}
	malloc_tmp = malloc(1 << 20);
	if (malloc_tmp == NULL) {
		e_printf("cannot malloc\n");
		exit(-1);
	}

	exit_req=0;
	blkcnt=0;
	wait_marker=1;
	for (;;) {
		len = pipe_blocked_read_block(STDIN_FILENO, p_header
					      ,malloc_tmp, 1<<20);
		if (len < 0) {
		  e_printf("ERROR:pipedump read error.\n");
		  exit(1);
		}
		if(wait_marker==1){
		  if(pipe_get_marker(p_header)==1){
		    wait_marker=0;
		  }
		  continue;
		}
		len =pipe_blocked_write_block(STDOUT_FILENO, p_header
					      ,malloc_tmp);
		if (len < 0) {
		  e_printf("ERROR:pipedump write error.\n");
		  exit(1);
		}
		len =pipe_blocked_write_block(dumpfd, p_header
					      ,malloc_tmp);
		if (len < 0) {
		  e_printf("ERROR:pipedump write error.\n");
		  exit(1);
		}
		if(pipe_get_marker(p_header)==1){
		  blkcnt++;
		  if(OPT.blklimit!=0 && blkcnt==OPT.blklimit){
		    e_printf ("pipedump succeeded. (%d blocks)\n",blkcnt);
		    exit(0);
		  }
		  if(exit_req==1){
		    e_printf ("pipedump hangup. (%d blocks)\n",blkcnt);
		    exit(0);
		  }
		}
	}
	return 0;
}



int
opt_etc(int argc, char *argv[])

{
	int             f_loop = 1;
	int             c;
/* 	int             multiple = 1; */
/* 	char           *tmpstr; */
	char           *opts = "f:b:";
	char           *helpmes =
	"usage : %s [-f <path>]\n"
	" options\n"
	"  f <path>   : Dump file to\n"
	"  b <num>    : block limiit (default 0=unlimited)\n"
	               ;

	OPT.dumpfile= DEFAULT_DUMPFILE;
	OPT.blklimit= 0;
	while ((c = getopt(argc, argv, opts)) != -1 && f_loop == 1) {
		switch (c) {
		case 'f':
			OPT.dumpfile = strdup(optarg);
			break;
		case 'b':
			OPT.blklimit = atoi(optarg);
			break;
		case '?':
		default:
			printf(helpmes, argv[0]);
			return -1;
		}
	}
	return 0;
}
