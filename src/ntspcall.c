/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): NAKANO Hiroaki <cas@trans-nt.com>
 *            FUJIKAWA Kenji <fujikawa@i.kyoto-u.ac.jp>
 * started:   2001/08/03
 *
 * $Id: ntspcall.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include "compat.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ntsputil.h"

RETSIGTYPE call_sigchld();

RETSIGTYPE
call_sigchld(type)
	int type;
{
	int status;
	int pid;
	int shouldexit = 0;
	
	for(;;) {
		pid = waitpid(-1, &status, WNOHANG);
		if(pid == 0) break;
		if(! WIFSTOPPED(status)) shouldexit = 1;
	}

	if(shouldexit) exit(WEXITSTATUS(status));
	return;
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *hostname;
	char *portname;
	struct addrinfo aih, *ai0, *ai;

	int so_net, so_net2;
	int so_child[2][2];
	/* 
	 * parent [0][0] <- child [0][1]
	 * parent [1][0] -> child [1][1]
	 */
	int pid_child;

	fd_set rfds;
	int maxfd;
	int notasip_established;
	const static struct timeval notasip_timeout = { 30, 0 };
	struct timeval time_last_received;

	int error;

	if(argc != 4 && argc != 5) {
		fprintf(stderr, 
			"usage: ntspcall <hostname> <port> <capturing and playing process>\n"
			"       ntspcall <hostname> <port> <capturing process> <playing process>\n");
		exit(1);
	}
	hostname = argv[1];
	portname = argv[2];

	error = socketpair(PF_LOCAL, SOCK_STREAM, 0, so_child[0]);
	if(error) err(1, "%s:%d", __FILE__, __LINE__);

	error = socketpair(PF_LOCAL, SOCK_STREAM, 0, so_child[1]);
	if(error) err(1, "%s:%d", __FILE__, __LINE__);

	bzero(&aih, sizeof(aih));
	aih.ai_socktype = SOCK_DGRAM;
	error = getaddrinfo(hostname, portname, &aih, &ai0);
	if(error) errx(1, "%s", gai_strerror(error));

	for(ai = ai0; ai; ai = ai->ai_next) {
		so_net = socket(ai->ai_family, ai->ai_socktype,
							ai->ai_protocol);
		if(so_net < 0) continue;
		if(connect(so_net, ai->ai_addr, ai->ai_addrlen) == 0) break;
	}
	if(ai == 0) err(1, "%s:%d", __FILE__, __LINE__);
	freeaddrinfo(ai0);

	so_net2 = util_make_back_socket(so_net);
	if(so_net2 < 0) err(1, "%s:%d", __FILE__, __LINE__);

	if(set_nonblock(so_net)) err(1, "%s:%d", __FILE__, __LINE__);
	if(set_nonblock(so_net2)) err(1, "%s:%d", __FILE__, __LINE__);

	if(signal(SIGCHLD, call_sigchld) == SIG_ERR) {
		err(1, "%s:%d", __FILE__, __LINE__);
	}
	if (argc == 4) {
		pid_child = fork();
		if(pid_child < 0) err(1, "%s:%d", __FILE__, __LINE__);
		if(pid_child == 0) {
			int i, dtabsize;

			/* capturing and playing process */
			error = dup2(so_child[1][1], 0);
			if(error < 0) err(1, "%s:%d", __FILE__, __LINE__);
			error = dup2(so_child[0][1], 1);
			if(error < 0) err(1, "%s:%d", __FILE__, __LINE__);
			dtabsize = getdtablesize();
			for (i = 3; i < dtabsize; i++) 
				close(i);

			execl("/bin/sh", "sh", "-c", argv[3], NULL);
			err(1, "%s:%d", __FILE__, __LINE__);
			_exit(1);
		}
	} else {
		pid_child = fork();
		if(pid_child < 0) err(1, "%s:%d", __FILE__, __LINE__);
		if(pid_child == 0) {
			int i, dtabsize;

			/* capturing process */
			error = dup2(so_child[0][1], 1);
			if (error < 0) 
				err(1, "%s:%d", __FILE__, __LINE__);
			dtabsize = getdtablesize();
			for (i = 3; i < dtabsize; i++) 
				close(i);

			execl("/bin/sh", "sh", "-c", argv[3], NULL);
			err(1, "%s:%d", __FILE__, __LINE__);
			_exit(1);
		}
		pid_child = fork();
		if(pid_child < 0) err(1, "%s:%d", __FILE__, __LINE__);
		if(pid_child == 0) {
			int i, dtabsize;

			/* playing process */
			error = dup2(so_child[1][1], 0);
			if (error < 0)
				err(1, "%s:%d", __FILE__, __LINE__);
			dtabsize = getdtablesize();
			for (i = 3; i < dtabsize; i++) 
				close(i);

			execl("/bin/sh", "sh", "-c", argv[4], NULL);
			err(1, "%s:%d", __FILE__, __LINE__);
			_exit(1);
		}
	}
	close(so_child[0][1]);
	close(so_child[1][1]);

	FD_ZERO(&rfds);
	FD_SET(so_child[0][0], &rfds);	maxfd = so_child[0][0];
	FD_SET(so_net, &rfds);		if(maxfd < so_net) maxfd = so_net;
	FD_SET(so_net2, &rfds);		if(maxfd < so_net2) maxfd = so_net2;

	notasip_established = 0;
	for(;;) {
		fd_set r = rfds;
		const static struct timeval select_timeout = { 1, 0 };
		struct timeval timo = select_timeout;

		if(notasip_established) {
			struct timeval now, interval;
			gettimeofday(&now, 0);
			timersub(&now, &time_last_received, &interval);
			if(timercmp(&interval, &notasip_timeout, >)) {
				fprintf(stderr, "hung up.(timeout)\n");
				fflush(stderr);
				exit(1);
			}
		}

		error = select(maxfd + 1, &r, 0, 0, &timo);

		if(FD_ISSET(so_child[0][0], &r)) {
			if(forward2(so_child[0][0], so_net, NTSP_TONET)) {
				if(errno != EINTR && errno != EAGAIN) {
					warn("%s:%d", __FILE__, __LINE__);
				}
			}
			continue;
		}
#if 0
		{
			static int c;

			fprintf(stderr, "%d FD debug\n", c);
			if (FD_ISSET(so_net, &r)) {
				fprintf(stderr, "%d so_net is set\n", c);
			}
			if (FD_ISSET(so_net2, &r)) {
				fprintf(stderr, "%d so_net2 is set\n", c);
			}
			if (notasip_established) {
				fprintf(stderr, "%d established\n", c);
			}
			fprintf(stderr, "%d\n");
			c++;
		}
#endif
		if(FD_ISSET(so_net, &r)) {
			if(forward2(so_net, so_child[1][0], NTSP_TOPIPE)) {
				if(errno == EINTR || errno == EAGAIN) continue;
				if(errno != ECONNREFUSED) {
					warn("%s:%d", __FILE__, __LINE__);
					continue;
				}

				signal(SIGCHLD, SIG_IGN);
				close(so_child[0][0]);
				close(so_child[1][0]);
				kill(pid_child, SIGHUP);
				waitpid(pid_child, 0, 0);
				fprintf(stderr, notasip_established
							? "hung up.\n"
							: "busy.\n");
				exit(0);
			}

			gettimeofday(&time_last_received, 0);
			continue;
		}
		if(FD_ISSET(so_net2, &r) && ! notasip_established) {
			char buf[65536];
			int rsize;
			struct sockaddr_storage from_st;
			struct sockaddr * const from = (void *)&from_st;
			int fromlen = sizeof(from_st);
			struct sockaddr_storage peer_st;
			struct sockaddr * const peer = (void *)&peer_st;
			int peerlen = sizeof(peer_st);

			rsize = recvfrom(so_net2, buf, sizeof(buf), MSG_PEEK,
							from, &fromlen);

			error = getpeername(so_net, peer, &peerlen);
			if(error) err(1, "%s:%d", __FILE__, __LINE__);
			if(util_hostcmp(from, peer) != 0) {
				fprintf(stderr, "attacked?\n");
				/* unknown packet.  attack ?? */
				continue;
			}

			fprintf(stderr, "connected.\n");
			fflush(stderr);

			error = connect(so_net, from, fromlen);
			if(error) err(1, "%s:%d", __FILE__, __LINE__);

			notasip_established = 1;
			gettimeofday(&time_last_received, 0);

			forward2(so_net2, so_child[1][0], NTSP_TOPIPE);

			continue;
		}
		if(FD_ISSET(so_net2, &r) && notasip_established) {
			gettimeofday(&time_last_received, 0);
			forward2(so_net2, so_child[1][0], NTSP_TOPIPE);
			continue;
		}
	}

	/* not reach */
	return 0;
}
