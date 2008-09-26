/*
 *  getnameinfo.c for notasip/compat
 *
 *	$IPhone: getaddrinfo.c,v 1.2 2000/09/12 10:27:35 cas Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef HAVE_GETADDRINFO
/*
 *  getaddrinfo()
 *
 */
int
getaddrinfo(name, service, req, pai)
	const char *name, *service;
	const struct addrinfo *req;
	struct addrinfo **pai;
{
	/* not yet */
	return -1;
}

void
freeaddrinfo(ai)
	struct addrinfo *ai;
{
	struct addrinfo *ai_next;

	while(ai) {
		if(ai->ai_canonname) free(ai->ai_canonname);
		if(ai->ai_addr) free(ai->ai_addr);
		ai_next = ai->ai_next;
		free(ai);
		ai = ai_next;
	}
	return;
}

char *
gai_strerror(ecode)
	int ecode;
{
	return "gai_strerror()";
}
#endif /* !HAVE_GETADDRINFO */

/* EOF */
