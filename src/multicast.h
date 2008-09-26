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
 * $Id: multicast.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _multicast_h_
#define _multicast_h_

int
recvsock_setup(const char *addr, const int port,
	       const char *ifname, const int ipver);

int
recvsock6_setup(const char *addr, const int port, const char *ifname);
int
recvsock4_setup(const char *addr, const int port, const char *ifname);

int
multicast_leave(const int fd, const char *mcast_name,
		const char *ifname, const int ipver) ;
int
multicast4_leave(const int fd, const char *mcast_name,
		const char *ifname) ;
int
multicast6_leave(const int fd, const char *mcast_name,
		 const char *ifname);

#endif
