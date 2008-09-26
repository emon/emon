/*
 *  net_addrcmp.c for notasip/compat
 *
 *	$IPhone: net_addrcmp.c,v 1.1 2000/09/12 06:29:39 cas Exp $
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../ntsputil.h"

#ifndef HAVE_NET_ADDRCMP
/*
 *  net_addrcmp()
 *
 *	compare socket address.
 */
int
net_addrcmp(struct sockaddr *addr1, struct sockaddr *addr2)
{
	int addrlen;

	addrlen = util_get_sa_len(addr1);
	if(addrlen != util_get_sa_len(addr2)) return 1;
	if(addr1->sa_family != addr2->sa_family) return 1;

	return bcmp(addr1, addr2, addrlen);
}
#endif /* !HAVE_NET_ADDRCMP */

/* EOF */
