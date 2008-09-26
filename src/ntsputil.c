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
 * $Id: ntsputil.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include "compat.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ntsputil.h"

#include "rtp_hd.h"
#include "pipe_hd.h"

/*
 *  util_get_sa_len()
 *
 *	Get size of sockaddr.
 */
int
util_get_sa_len(struct sockaddr *sa)
{
#ifdef	HAVE_SA_LEN
	return sa->sa_len;
#else
	switch(sa->sa_family) {
#ifdef	INET
	case AF_INET:	return sizeof(struct sockaddr_in);
#endif
#ifdef	INET6
	case AF_INET6:	return sizeof(struct sockaddr_in6);
#endif
	default:
			return sizeof(struct sockaddr);		/* XXX */
	}
#endif /* HAVE_SA_LEN */
}

/*
 *  util_clear_port()
 *
 *	In case of AF_INET or AF_INET6, clear the 'port' field.
 *	The other protocols are not supported.
 */
int
util_clear_port(struct sockaddr *sa)
{
	int port = 0;

	switch(sa->sa_family) {
//#ifdef INET
	case AF_INET:
		port = ((struct sockaddr_in *)&sa)->sin_port;
		((struct sockaddr_in *)sa)->sin_port = 0;
		break;
//#endif
//#ifdef INET6
	case AF_INET6:
		port = ((struct sockaddr_in6 *)&sa)->sin6_port;
		((struct sockaddr_in6 *)sa)->sin6_port = 0;
		break;
//#endif
	}

	return ntohs(port);
}

/*
 *  util_hostcmp()
 *
 *	compare host address.
 */
int
util_hostcmp(struct sockaddr *addr1, struct sockaddr *addr2)
{
	struct sockaddr_storage sa1_st, sa2_st;
	struct sockaddr * const sa1 = (struct sockaddr *)&sa1_st; 
	struct sockaddr * const sa2 = (struct sockaddr *)&sa2_st; 
	int sa1_len, sa2_len;

	sa1_len = util_get_sa_len(addr1);
	if(sa1_len > sizeof(sa1_st)) sa1_len = sizeof(sa1_st);
	bcopy(addr1, sa1, sa1_len);
	util_clear_port(sa1);

	sa2_len = util_get_sa_len(addr2);
	if(sa2_len > sizeof(sa2_st)) sa2_len = sizeof(sa2_st);
	bcopy(addr2, sa2, sa2_len);
	util_clear_port(sa2);

	return net_addrcmp(sa1, sa2);
}

/*
 *  util_close_virtually()
 *
 *	By connecting to itself, close the socket virtually, that is,
 *	the socket don't receive data any more.
 */
int
util_close_virtually(int so)
{
	struct sockaddr_storage sast;
	struct sockaddr *addr = (struct sockaddr *)&sast;
	int addrlen;
	int error;

	addrlen = sizeof(sast);
	error = getsockname(so, addr, &addrlen);
	if(error) return error;
	
	error = connect(so, addr, addrlen);
	if(error) return error;
	
	return 0;
}

/*
 *  util_make_sametype_socket()
 *
 *	It makes an new socket from a socket.
 *	The new socket have the same type and is not binded nor connected.
 *	In addition, it returns the sockname of the original socket.
 *
 */
int
util_make_sametype_socket(int so, struct sockaddr *addr, int *addrlen)
{
	int newso;
	int af, type;
	struct sockaddr_storage addr_st;
	int addrlen_st;
	int optlen;
	int error;

	if(addr == 0) addr = (struct sockaddr *)&addr_st;
	if(addrlen == 0) addrlen = &addrlen_st;

	*addrlen = sizeof(addr_st);
	error = getsockname(so, addr, addrlen);
	if(error) return error;
	af = addr->sa_family;

	optlen = sizeof(type);
	error = getsockopt(so, SOL_SOCKET, SO_TYPE, &type, &optlen);
	if(error) return error;

	newso = socket(af, type, 0);
	if(newso < 0) return error;

	return newso;
}

/*
 *  util_make_back_socket()
 *
 *	It makes an new socket from connected socket.
 *	The new socket have the same local name as the original one,
 *	but it isn't connected, that is, its foreign name is wildcard.
 *
 *		A:P->B:Q  ==>  A:P->*:*
 */
int
util_make_back_socket(int so)
{
	int newso;
	struct sockaddr_storage addr_st;
	struct sockaddr * const addr = (struct sockaddr *)&addr_st;
	int addrlen;
	int opt;
	int error;

	addrlen = sizeof(addr_st);
	newso = util_make_sametype_socket(so, addr, &addrlen);
	if(newso < 0) return newso;

	opt = 1;
	error = setsockopt(newso, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(error) return error;

	error = bind(newso, addr, addrlen);
	if(error) return error;

	opt = 0;
	error = setsockopt(newso, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(error) return error;

	return newso;
}

/*
 *  util_make_copy_socket()
 *
 *	It makes an new socket from un-connected socket.
 *	The new socket have the same local name as the original one,
 *	but it's not connected to anywhere.
 *
 *		A:P->?:?  ==>  A:P->*:*
 */
int
util_make_copy_socket(int so)
{
	int newso;
	struct sockaddr_storage addr_st;
	struct sockaddr * const addr = (struct sockaddr *)&addr_st;
	int addrlen;
	int opt;
	int error;

	addrlen = sizeof(addr_st);
	newso = util_make_sametype_socket(so, addr, &addrlen);
	if(newso < 0) return newso;

	opt = 1;
	error = setsockopt(so, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	if(error) return error;
	opt = 1;
	error = setsockopt(newso, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	if(error) return error;

	error = bind(newso, addr, addrlen);
	if(error) return error;

	opt = 0;
	error = setsockopt(so, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	if(error) return error;
	opt = 0;
	error = setsockopt(newso, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	if(error) return error;

	return newso;
}

/*
 *  forward()
 *
 *	It forwards just one packet from so_from to so_to.
 */
int
forward(int so_from, int so_to)
{
#define NTSP_BUFSIZE 65536
	char buf[NTSP_BUFSIZE];
	int rsize, wsize;

	errno = 0;
	rsize = read(so_from, buf, sizeof(buf));
	switch(errno) {
	default:
		warn("%s:%d", __FILE__, __LINE__);
	case EINTR:
	case EAGAIN:
	case ECONNREFUSED:
		return -1;
	case 0:		break;
	}

	for(;;) {
		errno = 0;
		wsize = write(so_to, buf, rsize);
		switch(errno) {
		default:
			warn("%s:%d", __FILE__, __LINE__);
		case ECONNREFUSED:
			return -1;
		case EINTR:
		case EAGAIN:
			continue;
		case 0:
			if(rsize == wsize) break;
			warnx("%s:%d:rsize != wsize", __FILE__, __LINE__);
			return -1;
		}
		break;
	}
	return 0;
}

/* #define F_DEBUG */

/*
 *  forward2()
 *
 *	It forwards just one packet from so_from to so_to 
 *      with modifying header
 */
int
forward2(int so_from, int so_to, int type)
{
	char buf0[NTSP_BUFSIZE], *buf;
	int rsize, wsize;
	rtp_hd_t *rhdr = NULL;
	pipe_hd_t *phdr = NULL;
	int len;
#ifdef F_DEBUG
	static int c = 0;
#endif

	errno = 0;
	if (type == NTSP_TOPIPE) {
		buf = buf0 + PIPE_HEADER_LEN;
		phdr = (pipe_hd_t *)buf0;
		rhdr = (rtp_hd_t *)buf;
		rsize = read(so_from, buf, NTSP_BUFSIZE);
	} else { /* type == NTSP_TONET */
		buf = buf0;
		phdr = (pipe_hd_t *)buf;
		rsize = recv(so_from, buf, sizeof(pipe_hd_t), MSG_PEEK);
		if (rsize < 0)
			goto err;
		len = ntohl(phdr->vmlen)&0x00ffffff;
		if (len > NTSP_BUFSIZE) {
			fprintf(stderr, "%s:%d buf overflow len=%d\n", 
				__FILE__, __LINE__, len);
			return (-1);
		}
		rsize = recv(so_from, buf, len + PIPE_HEADER_LEN, 0);
		if (rsize != len + PIPE_HEADER_LEN) {
			fprintf(stderr, 
				"%s:%d size is illeagal len=%d, rsize=%d\n", 
				__FILE__, __LINE__, len, rsize);
			return (-1);
		}
	}
err:
	switch(errno) {
	default:
		warn("%s:%d", __FILE__, __LINE__);
	case EINTR:
	case EAGAIN:
	case ECONNREFUSED:
		return -1;
	case 0:		break;
	}
#ifdef F_DEBUG
	fprintf(stderr, "%d: read %d->%d, size=%d\n",
		c++, so_from, so_to, rsize);
#endif
	if (rsize == PIPE_HEADER_LEN)
		return (0);

	if (type == NTSP_TOPIPE) {
		/* add pipe header */
		phdr->vmlen = htonl((1<<28)|
				    ((rhdr->mp&0x80) ? 1<<27 : 0)|
				    (rsize & 0x00ffffff));
		rsize += PIPE_HEADER_LEN;
		phdr->ts = rhdr->ts;
		buf = buf0;
	} else {
		/* cut pipe header */
		rsize -= PIPE_HEADER_LEN;
		buf = buf0 + PIPE_HEADER_LEN;
	}

	for(;;) {
		errno = 0;
		wsize = write(so_to, buf, rsize);
		switch(errno) {
		default:
			warn("%s:%d", __FILE__, __LINE__);
		case ECONNREFUSED:
			return -1;
		case EINTR:
		case EAGAIN:
			continue;
		case 0:
			if(rsize == wsize) break;
			warnx("%s:%d:rsize != wsize", __FILE__, __LINE__);
			return -1;
		}
		break;
	}
#ifdef F_DEBUG
	fprintf(stderr, "%d: write %d->%d, size=%d\n",
		c++, so_from, so_to, rsize);
#endif
	return 0;
}

/*
 *  set_nonblock()
 *
 *	It sets a file descriptor into non-block mode.
 */
int
set_nonblock(int fd)
{
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}

/* EOF */
