/*
 *  getnameinfo.c for notasip/compat
 *
 *	$IPhone: getnameinfo.c,v 1.1 2000/09/12 06:29:39 cas Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef HAVE_GETNAMEINFO
int
getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags)
	const struct sockaddr *addr;
	char *host, *serv;
	size_t addrlen, hostlen, servlen;
	int flags;
{
	/* not yet */
	return -1;
}
#endif /* !HAVE_GETNAMEINFO */

/* EOF */
