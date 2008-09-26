/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 *            Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 * started:   2001/06/16
 *
 * $Id: multicast.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>

#include "debug.h"
#include "multicast.h"

int
check_ipver(const char *name)
{
	if (gethostbyname2(name, AF_INET6)!= NULL){
		return 6;
	} else {
		return 4;
	}
}


/* JOIN multicast group */

/*
 * addr: host name or IP address(IPv4 or IPv6)
 * port: port number
 * ifname: network interface name
 * ipver: IP version (0, 4 or 6: 0=auto)
 * 
 * However join to multicast grouiip FAILURE if the host has no interface, this
 * function behaves successfully.
 */
int
recvsock_setup(const char *addr, const int port,
	       const char *ifname, int ipver)
/* ipver = 4, 6 or 0. (0 = auto select) */
{
	int ret = 0;

	switch (ipver){
	case 0:			/* auto select IP version */
		if (addr == NULL){
			ret = recvsock6_setup(addr, port, ifname);
		}else if (check_ipver(addr)==6){
			ret = recvsock6_setup(addr, port, ifname);
		} else {
			ret = recvsock4_setup(addr, port, ifname);
		}
		break;
	case 4:
		ret = recvsock4_setup(addr, port, ifname);
		break;
	case 6:
		ret = recvsock6_setup(addr, port, ifname);
		break;
	default:
		d_printf("\nunknown IP version: %d.", ipver);
		break;
	}
	if (ret < 0){
		e_printf("\nrecvsock_setup: exit, because cannot open socket\n");
		exit(-1);
	}
	return ret;
}


int
recvsock6_setup(const char *addr, const int port, const char *ifname)
{
	struct sockaddr_in6 sa;
	socklen_t sal = sizeof(sa);
	int fd;
	const int on = 1;
	struct ipv6_mreq	mreq6;
	
	if ((fd = socket(AF_INET6, SOCK_DGRAM, 0))< 0){
		e_printf("\ncannot make socket\n");
		return -1;
	}
	bzero(&sa, sal);
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(port);

	if (addr == NULL){	/* in addr any */
		extern const struct in6_addr in6addr_any;

		sa.sin6_addr = in6addr_any; /* copy struct to struct */
		d2_printf("set sin6_addr to 'in6addr_any'\n");

	} else {
		int r;
		
		/* specified address (maybe multicast address) */
		/* you cannot specify multicast NAME, now */
		r = inet_pton(AF_INET6, addr, &sa.sin6_addr);
		if (r != 1){
			e_printf("\ninet_pton returns %d\n", r);
			return -1;
		}
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))== -1){
		e_printf("\ncannot setsockopt SO_REUSEADDR");
		return -1;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))== -1){
		e_printf("\ncannot setsockopt SO_REUSEPORT");
		return -1;
	}
#endif
	if (bind(fd, (struct sockaddr *)&sa, sal) == -1){
		e_printf("\ncannot bind %s", addr);
		return -1;
	}

	if (IN6_IS_ADDR_MULTICAST(&sa.sin6_addr)) {
		memcpy(&mreq6.ipv6mr_multiaddr,
		       &sa.sin6_addr, sizeof(struct in6_addr));
		if (ifname == NULL){
			mreq6.ipv6mr_interface = 0; /* autoselect */
		}else {
			mreq6.ipv6mr_interface = if_nametoindex(ifname);
			if (mreq6.ipv6mr_interface == 0){
				/* errno = ENXIO; */
				/* i/f name not found */
				return -1;
			}
		}

		if (setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
			       &mreq6, sizeof(mreq6)) == -1){
			d_printf("\ncannot setsockopt IPV6_JOIN_GROUP, "
				 "bind: %s/%d", addr, port);
		} else {
			d_printf("\njoin multicast: %s/%d", addr, port);
		}
	} else {
		char raddr[INET6_ADDRSTRLEN];
		const char *r;
		
		r = inet_ntop(AF_INET6, &sa.sin6_addr, raddr, sizeof(raddr));
		if (r == NULL){
			d_printf("\nerror occurs in inet_ntop");
		} else {
			d_printf("\nreceiving unicast: %s/%d", raddr, port);
		}
	}

	return fd;
}


int
recvsock4_setup(const char *addr, const int port,
	       const char *ifname)
{
	struct sockaddr_in sa;
	socklen_t sal = sizeof(sa);
	int fd;
	const int on = 1;
	struct ip_mreq mreq;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		e_printf("\ncannot make socket\n");
		exit(-1);
	}
	bzero(&sa, sal);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	if (inet_pton(AF_INET, addr, &sa.sin_addr) != 1) {
		e_printf("inet_pton does not returns 1\n");
		close(fd);
		return -1;
	}
	d2_printf("\nsockaddr_in.sin_addr=%d.%d.%d.%d",
		  ((unsigned char *) &sa.sin_addr)[0],
		  ((unsigned char *) &sa.sin_addr)[1],
		  ((unsigned char *) &sa.sin_addr)[2],
		  ((unsigned char *) &sa.sin_addr)[3]);

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		e_printf("\ncannot setsockopt SO_REUSEADDR");
		exit(-1);
	}
#ifdef SO_REUSEPORT
	d3_printf("\nSO_REUSEPORT OK");
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) == -1) {
		e_printf("\ncannot setsockopt SO_REUSEPORT");
		exit(-1);
	}
#endif
	if (bind(fd, (struct sockaddr *) & sa, sal) == -1){
		e_printf("\ncannot bind %s", addr);
		return -1;
	}
	if (IN_MULTICAST(htonl(sa.sin_addr.s_addr))){
		d2_printf("\nIN_MULTICAST");
		memcpy(&mreq.imr_multiaddr, &sa.sin_addr,
		       sizeof(struct in_addr));
		if (ifname == NULL) {
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		} else {
			struct ifreq    ifreq;

			strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
			if (ioctl(fd, SIOCGIFADDR, &ifreq) < 0) {
				e_printf("\ncannot ioctl SIOCGIFADDR");
				exit(-1);
			}
			memcpy(&mreq.imr_interface,
			       &((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr,
			       sizeof(struct in_addr));
		}
		d2_printf("\nip_mreq.imr_multiaddr=%d.%d.%d.%d"
			  "\nip_mreq.imr_interface=%d.%d.%d.%d",
			  ((unsigned char *) &mreq.imr_multiaddr.s_addr)[0],
			  ((unsigned char *) &mreq.imr_multiaddr.s_addr)[1],
			  ((unsigned char *) &mreq.imr_multiaddr.s_addr)[2],
			  ((unsigned char *) &mreq.imr_multiaddr.s_addr)[3],

			  ((unsigned char *) &mreq.imr_interface.s_addr)[0],
			  ((unsigned char *) &mreq.imr_interface.s_addr)[1],
			  ((unsigned char *) &mreq.imr_interface.s_addr)[2],
			  ((unsigned char *) &mreq.imr_interface.s_addr)[3]);

		if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			       &mreq, sizeof(mreq)) == 0) {
			d_printf("\njoin multicast: %s/%d", addr, port);
		} else {
			d_printf("\ncannot setsockopt IP_ADD_MEMBERSHIP, "
				 "bind: %s/%d", addr, port);
		}
	} else {		/* if IN_MULTICAST */
		d_printf("\nreceiving unicast: %s/%d", addr, port);
	}
	if (ifname != NULL) {
		d_printf(", if=%s", ifname);
	}
	return fd;
}


/* LEAVE from multicast group */
/*
 * addr: host name or IP address(IPv4 or IPv6)
 * port: port number
 * ifname: network interface name
 * ipver: IP version (0, 4 or 6: 0=auto)
 * 
 * However join to multicast group FAILURE if the host has no interface, this
 * function behaves successfully.
 */
int
multicast_leave(const int fd, const char *addr,
		const char *ifname, int ipver)
{
	int ret;

	if ((ipver == 6 )||
	    (ipver == 0  && check_ipver(addr)==6)){
		ret = multicast6_leave(fd, addr, ifname);
	} else {
		ret = multicast4_leave(fd, addr, ifname);
	}
	return ret;
}


int
multicast6_leave(const int fd, const char *addr, const char *ifname)
{
	fprintf(stderr, "multicast6_leave called\n");
	return 0;
}


int
multicast4_leave(const int fd, const char *addr, const char *ifname)
{
	struct sockaddr_in sa;
	socklen_t       sal = sizeof(sa);
	struct ip_mreq  mreq;
	int             mlen = sizeof(mreq);

	bzero(&sa, sal);
	if (inet_pton(AF_INET, addr, &sa.sin_addr) != 1) {
		e_printf("\ninet_pton does not returns 1");
		exit(-1);
	}
	memcpy(&mreq.imr_multiaddr, &sa.sin_addr, sizeof(struct in_addr));
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	return (setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, mlen));
}
