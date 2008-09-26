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
 * $Id: sendsock_setup.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>

#include "debug.h"
#include "sendsock_setup.h"

int
check_ipver(const char *name)
{
	if (gethostbyname2(name, AF_INET6)!= NULL){
		return 6;
	} else {
		return 4;
	}
}


int
sendsock_setup(const char *dst_name, const int dst_port,
	       const char *ifname, const char *src_addr,
	       const int src_port, const u_char ttl, const int ipver)
/* ipver = 4, 6 or 0. (0 = auto select) */
{
	int ret = 0;

	switch (ipver){
	case 0:
		if (check_ipver(dst_name)==6){
			d_printf("\ncall sendsock6_setup");
			ret = sendsock6_setup(dst_name, dst_port, ifname,
					      src_addr, src_port, ttl);
		} else {
			ret = sendsock4_setup(dst_name, dst_port, ifname,
					      src_addr, src_port, ttl);
		}
		break;
	case 4:
		ret = sendsock4_setup(dst_name, dst_port, ifname,
				      src_addr, src_port, ttl);
		break;
	case 6:
		ret = sendsock6_setup(dst_name, dst_port, ifname,
				      src_addr, src_port, ttl);
		break;
	default:
		d_printf("\nunknown IP version: %d.", ipver);
		break;
	}
	return ret;
}

int
sendsock6_setup(const char *dst_name, const int dst_port,
		const char *ifname, const char *src_addr,
		const int src_port, const u_char hop)
{
	int fd = -1;
	struct sockaddr_in6 s_addr;
	const int s_addr_len = sizeof(s_addr);
	const int on = 1;

	struct addrinfo hints, *res, *res0;
	int error;
	char servname[6];
	
	bzero(&hints, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	sprintf(servname, "%d", dst_port);
	error = getaddrinfo(dst_name, servname, &hints, &res0);

	for (res = res0; res; res = res->ai_next){
		fd = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
		if (fd < 0){
			continue;
		}

		/* setup src-address or src-port */
		if (src_addr != NULL || src_port > 0){
			bzero(&s_addr, s_addr_len);
			s_addr.sin6_family = AF_INET6;
			if (src_addr != NULL){
				inet_pton(AF_INET6, src_addr,
					  &s_addr.sin6_addr);
			}
			if (src_port > 0){
				s_addr.sin6_port = htons(src_port);
				if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
					       &on, sizeof(on)) == -1){
					e_printf("\ncannot setsockopt"
						 " SO_REUSEADDR");
				}
			}
			if (bind(fd,(struct sockaddr*)&s_addr, s_addr_len)!=0){
				e_printf("\ncannot bind src address"
					 " or src port");
				continue;
			}
		}

			/* bind interface (not work) */
		if (ifname != NULL){
			unsigned int ifid;
			if ((ifid = if_nametoindex(ifname))==0){
				e_printf("\ncannot if_nametoindex(%s)",
					 ifname);
			}
			if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
				       &ifid, sizeof(ifid)) != 0){
				e_printf("\ncan not set multicast_if: %s[%d]",
					 ifname, ifid);
				continue;
			}
			d_printf("\nset multicast_if: %s, id=%d",
				 ifname, ifid);
		}
		if (connect(fd, res->ai_addr, res->ai_addrlen)!= 0){
			e_printf("\ncannot connect\n");
			continue;
		}
		if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
			       &hop, sizeof(hop)) == -1) {
			d_printf("\ncannot setsockopt"
				 " IP_MULTICAST_TTL ttl=%d", hop);
		}
		
		break;		/* okay we got one */
	}
	freeaddrinfo(res0);
	e_printf("sendsock6_setup return with fd=%d\n", fd);
	
	return fd;
}


int
sendsock4_setup(const char *dst_name, const int dst_port,
		const char *ifname, const char *src_addr,
		const int src_port, const u_char ttl)
{
	int             fd;
	struct sockaddr_in s_addr;
	const int       s_addr_len = sizeof(s_addr);
	struct ifreq    ifr;
	const int       on = 1;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (src_addr != NULL || src_port > 0){
		bzero(&s_addr, s_addr_len);
		s_addr.sin_family = AF_INET;
		if (src_addr != NULL){
//			inet_pton(AF_INET, src_addr, &s_addr.sin_addr.s_addr);
			inet_pton(AF_INET, src_addr, &s_addr.sin_addr);
		}
		if (src_port > 0){
			s_addr.sin_port = htons(src_port);
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
				       &on, sizeof(on)) == -1){
				e_printf("\ncannot setsockopt SO_REUSEADDR");
				exit(-1);
			}
#ifdef SO_REUSEPORT
			d3_printf("\nSO_REUSEPORT OK");
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
				       &on, sizeof(on)) == -1) {
				e_printf("\ncannot setsockopt SO_REUSEPORT");
				exit(-1);
			}
#endif
		}
		if (bind(fd, (struct sockaddr *) & s_addr,s_addr_len)!= 0){
			e_printf("\ncannot bind src address or src port");
		}
	}
	if (ifname != NULL) {
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		ioctl(fd, SIOCGIFADDR, &ifr);
		if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
			       &((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr,
			       sizeof(struct in_addr)) == -1) {
			d_printf("\ncannot setsockopt"
				 " IP_MULTICAST_IF interface %s", ifname);
		}
	}
	bzero(&s_addr, s_addr_len);
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(dst_port);
	inet_pton(AF_INET, dst_name, &s_addr.sin_addr);
	if (connect(fd, (struct sockaddr *) & s_addr, s_addr_len)!= 0){
		e_printf("\ncannot connect\n");
		exit(-1);
	}
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
		       &ttl, sizeof(ttl)) == -1) {
		d_printf("\ncannot setsockopt"
			 " IP_MULTICAST_TTL ttl=%d", ttl);
	}
	return fd;
}
