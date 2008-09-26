/*
 * $Id: compat.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef HAVE_NET_ADDRCMP
int net_addrcmp(struct sockaddr *addr1, struct sockaddr *addr2);
#endif /* !HAVE_NET_ADDRCMP */


#ifndef HAVE_GETADDRINFO
int getaddrinfo(const char *name, const char *service, const struct addrinfo *req, struct addrinfo **pai);
void freeaddrinfo(struct addrinfo *ai);
char *gai_strerror(int ecode);
#endif /* !HAVE_GETADDRINFO */


#ifndef HAVE_GETNAMEINFO
int getnameinfo(const struct sockaddr *addr, size_t addrlen, char *host, size_t hostlen, char *serv, size_t servlen, int flags);
#endif /* !HAVE_GETNAMEINFO */


/* EOF */
