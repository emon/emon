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
 * $Id: ntspwait.c,v 1.1 2008/09/26 15:10:34 emon Exp $
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

#undef	DEBUG

#ifdef DEBUG
#define	WARNX(a)		warnx a
#define	WARNX_ACCEPTING(a)	warnx_accepting a
#else
#define	WARNX(a)
#define	WARNX_ACCEPTING(a)
#endif

char *
sprint_addr(struct sockaddr *addr, int addrlen)
{
	static char name[NI_MAXHOST + NI_MAXSERV];
	char hostname[NI_MAXHOST], portname[NI_MAXSERV];
	int error;

	error = getnameinfo(addr, addrlen, hostname, sizeof(hostname),
		portname, sizeof(portname), NI_NUMERICHOST | NI_NUMERICSERV);
	if(error) {
		strcpy(name, "error");
		return name;
	}
	strcpy(name, hostname);
	strcat(name, ":");
	strcat(name, portname);
	return name;
}

static RETSIGTYPE waitcall_sigchld();

RETSIGTYPE
waitcall_sigchld(type)
	int type;
{
	int status;
	int pid;
	
	for(;;) {
		pid = waitpid(-1, &status, WNOHANG);
		if(pid == 0) break;
		if(pid < 0) {
			if(errno == ECHILD) break;
			err(1, "waitcall_sigchld()");
		}
	}

	return;
}

#define	ACCEPTING_TABLE_SIZE	10
struct accepting {
	int last;
	int valid;

	struct sockaddr_storage peer_st;
	struct sockaddr *peer;
	int peerlen;

	struct timeval time_last_received;

	enum { CLEANING, CONNECTED, DETACHED } state;

	int so_child[2][2];	/* connected to child (0:myself, 1:child)*/
	/* 
	 * parent [0][0] <- child [0][1]
	 * parent [1][0] -> child [1][1]
	 */

	int so_net;		/* connected to peer */
	int so_net2;		/* connected to peer (overlap with so_wait) */

	int pid_child;
} accepting_table[ACCEPTING_TABLE_SIZE];
int accepting_table_size = ACCEPTING_TABLE_SIZE;

#ifdef DEBUG
void
warnx_accepting(struct accepting *a)
{
	struct sockaddr_storage addr_st;
	struct sockaddr * const addr = (void *)&addr_st;
	int addrlen;

	warnx("--- dump accepting #%d", a - accepting_table);
	warnx("last=%d valid=%d state=%s(%d)", a->last, a->valid,
		a->state == CLEANING ? "CLEANING" :
		a->state == CONNECTED ? "CONNECTED" :
		a->state == DETACHED ? "DETACHED" : "unknown", a->state);

	addrlen = sizeof(addr_st);
	getsockname(a->so_net, addr, &addrlen);
	warnx("peer=%s", sprint_addr(addr, addrlen));

/*
	warnx("so_child[]={%d,%d}", a->so_child[0], a->so_child[1]);
*/

	addrlen = sizeof(addr_st);
	getsockname(a->so_net, addr, &addrlen);
	warnx("so_net=%d local=%s", a->so_net, sprint_addr(addr, addrlen));
	addrlen = sizeof(addr_st);
	getpeername(a->so_net, addr, &addrlen);
	warnx("so_net=%d remote=%s", a->so_net, sprint_addr(addr, addrlen));

	addrlen = sizeof(addr_st);
	getsockname(a->so_net2, addr, &addrlen);
	warnx("so_net2=%d local=%s", a->so_net2, sprint_addr(addr, addrlen));
	addrlen = sizeof(addr_st);
	getpeername(a->so_net2, addr, &addrlen);
	warnx("so_net2=%d remote=%s", a->so_net2, sprint_addr(addr, addrlen));
}
#endif /* DEBUG */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *hostname;
	char *portname;
	struct addrinfo aih, *ai0, *ai;

	int so_wait, error, i;

	bzero(&accepting_table, sizeof(accepting_table));
	accepting_table[accepting_table_size - 1].last = 1;

	if(argc != 4 && argc != 5) {
		fprintf(stderr, 
			"usage: ntspwait <hostname> <port> <capturing and playing process>\n"
			"       ntspwait <hostname> <port> <capturing process> <playing process>\n");
		exit(1);
	}
	hostname = argv[1];
	portname = argv[2];

	bzero(&aih, sizeof(aih));
	aih.ai_flags = AI_PASSIVE;
	aih.ai_socktype = SOCK_DGRAM;
	error = getaddrinfo(hostname, portname, &aih, &ai0);
	if(error) errx(1, "%s", gai_strerror(error));

	for(ai = ai0; ai; ai = ai->ai_next) {
		so_wait = socket(ai->ai_family, ai->ai_socktype,
							ai->ai_protocol);
		if(so_wait < 0) continue;
		if(bind(so_wait, ai->ai_addr, ai->ai_addrlen) == 0) break;
	}
	if(ai == 0) err(1, "%s:%d", __FILE__, __LINE__);
WARNX(("so_wait is waiting on %s", sprint_addr(ai->ai_addr, ai->ai_addrlen)));
	freeaddrinfo(ai0);

	if(signal(SIGCHLD, waitcall_sigchld) == SIG_ERR) {
		err(1, "%s:%d", __FILE__, __LINE__);
	}
	for(;;) {
		fd_set r;
		int maxfd;
		int so;
		struct accepting *acp, *bcp;
		struct timeval *timo, timo_zero;
		static const struct timeval tv_zero = {0, 0};

		timo = 0;
		timo_zero = tv_zero;

		FD_ZERO(&r);
		FD_SET(so_wait, &r);		maxfd = so_wait;
		for(acp = accepting_table; !acp->last; acp++) {
			if(! acp->valid) continue;
			FD_SET(acp->so_child[0][0], &r);
			if(maxfd < acp->so_child[0][0]) 
				maxfd = acp->so_child[0][0];
			FD_SET(acp->so_net, &r);
			if(maxfd < acp->so_net) maxfd = acp->so_net;
			FD_SET(acp->so_net2, &r);
			if(maxfd < acp->so_net2) maxfd = acp->so_net2;

			if(acp->state != DETACHED) timo = &timo_zero;
		}

		error = select(maxfd + 1, &r, 0, 0, timo);

		bcp = accepting_table - 1;
		so = so_wait;
		do {
			char buf[65536]; /* 1 is enouth... */
			struct sockaddr_storage from_st;
			struct sockaddr * const from = (void *)&from_st;
			int fromlen = sizeof(from_st);
			struct sockaddr_storage nsck_st;
			struct sockaddr * const nsck = (void *)&nsck_st;
			int nscklen = sizeof(nsck_st);

			if(bcp >= accepting_table) {
				if(! bcp->valid) continue;
				if(bcp->state != CLEANING) continue;
				so = bcp->so_net2;
			}

			if(so == so_wait && ! FD_ISSET(so, &r)) {
				for(acp = accepting_table; !acp->last; acp++) {
					if(! acp->valid) continue;
					if(acp->state != CONNECTED) continue;
					acp->state = DETACHED;
WARNX(("DETACHED so=%d,%d", acp->so_net, acp->so_net2));
				}
				continue;
			}
			if(so != so_wait && ! FD_ISSET(so, &r)) {
				error = connect(so, bcp->peer, bcp->peerlen);
				if(error < 0) {
					err(1, "%s:%d", __FILE__, __LINE__);
				}
				bcp->state = CONNECTED;
WARNX(("CONNECTED so=%d", so));
				continue;
			}

			error = recvfrom(so, buf, sizeof(buf), MSG_PEEK,
							from, &fromlen);
			if(error < 0) err(1, "%s:%d", __FILE__, __LINE__);

			/* forward to child process if possible */
			for(acp = accepting_table; !acp->last; acp++) {
				if(! acp->valid) continue;
				if(net_addrcmp(from, acp->peer)) continue;
				break;
			}
			if(! acp->last) {
				forward2(so, acp->so_child[1][0], NTSP_TOPIPE);
				gettimeofday(&acp->time_last_received, 0);
				continue;
			}

			/* accepting new connection */
WARNX(("accepting new connection..."));
			for(acp = accepting_table; !acp->last; acp++) {
				if(! acp->valid) break;
			}
			if(acp->last) {
				warnx("Accepting table is full. ignored.");
				continue;
			}
			acp->peer = (struct sockaddr *)&acp->peer_st;
			bcopy(from, acp->peer, fromlen);
			acp->peerlen = fromlen;
			acp->state = CLEANING;

			acp->so_net = util_make_sametype_socket(so_wait, 0, 0);
			if(acp->so_net < 0) err(1, "%s:%d", __FILE__,__LINE__);
			error = connect(acp->so_net, from, fromlen);
			if(error) err(1, "%s:%d", __FILE__, __LINE__);
			error = getsockname(acp->so_net, nsck, &nscklen);
			if(error) err(1, "%s:%d", __FILE__, __LINE__);

			acp->so_net2 = util_make_copy_socket(so_wait);
			if(acp->so_net2 < 0) err(1, "%s:%d", __FILE__,__LINE__);
			error = connect(acp->so_net2, nsck, nscklen);
			if(error) err(1, "%s:%d", __FILE__, __LINE__);

			for (i = 0; i < 2; i++) {
				error = socketpair(PF_LOCAL, SOCK_STREAM, 0,
						   acp->so_child[i]);
				if (error) 
					err(1, "%s:%d", __FILE__, __LINE__);
			}
			if (argc == 4) {
				acp->pid_child = fork();
				if(acp->pid_child < 0) {
					err(1, "%s:%d", __FILE__, __LINE__);
				}
				if(acp->pid_child == 0) {
					int dtabsize;
					int i;
					
					/* capturing and playing process */
					error = dup2(acp->so_child[1][1], 0);
					if(error < 0) {
						err(1, "%s:%d", 
						    __FILE__, __LINE__);
					}
					error = dup2(acp->so_child[0][1], 1);
					if(error < 0) {
						err(1, "%s:%d", 
						    __FILE__, __LINE__);
					}
					dtabsize = getdtablesize();
					for(i = 3; i < dtabsize; i++) close(i);

					execl("/bin/sh", "sh", "-c", 
					      argv[3], NULL);
					err(1, "%s:%d", __FILE__, __LINE__);
				}
			} else {
				acp->pid_child = fork();
				if(acp->pid_child == 0) {
					int dtabsize;
					int i;

					/* capturing process */
					error = dup2(acp->so_child[0][1], 1);
					if(error < 0) {
						err(1, "%s:%d", 
						    __FILE__, __LINE__);
					}
					dtabsize = getdtablesize();
					for(i = 3; i < dtabsize; i++) close(i);

					execl("/bin/sh", "sh", "-c", 
					      argv[3], NULL);
					err(1, "%s:%d", __FILE__, __LINE__);
				}

				acp->pid_child = fork(); /* XXX */
				if(acp->pid_child < 0) {
					err(1, "%s:%d", __FILE__, __LINE__);
				}
				if(acp->pid_child == 0) {
					int dtabsize;
					int i;
					
					/* playing process */
					error = dup2(acp->so_child[1][1], 0);
					if(error < 0) {
						err(1, "%s:%d", 
						    __FILE__, __LINE__);
					}
					dtabsize = getdtablesize();
					for(i = 3; i < dtabsize; i++) close(i);
					
					execl("/bin/sh", "sh", "-c", 
					      argv[4], NULL);
					err(1, "%s:%d", __FILE__, __LINE__);
				}
			}

			if(set_nonblock(acp->so_net)) {
				err(1, "%s:%d", __FILE__, __LINE__);
			}
			if(set_nonblock(acp->so_net2)) {
				err(1, "%s:%d", __FILE__, __LINE__);
			}
#if 0
			if(set_nonblock(acp->so_child[0])) {
				err(1, "%s:%d", __FILE__, __LINE__);
			}
#endif

			acp->valid = 1;
WARNX_ACCEPTING(acp);
			forward2(so, acp->so_child[1][0], NTSP_TOPIPE);
			gettimeofday(&acp->time_last_received, 0);

			break;

		} while(! (++bcp)->last);
		
		for(bcp = accepting_table; !bcp->last; bcp++) {
			if(! bcp->valid) continue;
			if(! FD_ISSET(bcp->so_child[0][0], &r)) continue;

			forward2(bcp->so_child[0][0],bcp->so_net, NTSP_TONET); 
			gettimeofday(&bcp->time_last_received, 0);
			WARNX_ACCEPTING((bcp));
		}

		for(bcp = accepting_table; !bcp->last; bcp++) {
			if(! bcp->valid) continue;
			if(! FD_ISSET(bcp->so_net, &r)) continue;

			error = forward2(bcp->so_net, bcp->so_child[1][0],
					 NTSP_TOPIPE);
			if(error && errno == ECONNREFUSED) {
				kill(bcp->pid_child, SIGHUP);
				close(bcp->so_net);
				close(bcp->so_net2);
				close(bcp->so_child[0][0]);
				close(bcp->so_child[0][1]);
				close(bcp->so_child[1][0]);
				close(bcp->so_child[1][1]);
				bcp->valid = 0;
				fprintf(stderr, "hung up. so=%d\n",
								bcp->so_net);
				fflush(stderr);
			}
			gettimeofday(&bcp->time_last_received, 0);
			WARNX_ACCEPTING((bcp));
		}

		for(bcp = accepting_table; !bcp->last; bcp++) {
			if(! bcp->valid) continue;
			if(bcp->state == CLEANING) continue;
			if(! FD_ISSET(bcp->so_net2, &r)) continue;

			forward2(bcp->so_net2, bcp->so_child[1][0], NTSP_TOPIPE);
			gettimeofday(&bcp->time_last_received, 0);
			WARNX_ACCEPTING((bcp));
		}
	}

	/* not reach */
	return 0;
}
