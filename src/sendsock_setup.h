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
 * $Id: sendsock_setup.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _sendsock_setup_h_
#define _sendsock_setup_h_

int
sendsock_setup(const char *mcast_name, const int port, const char *ifname,
	       const char *src_addr, const int src_port,
	       const u_char ttl, const int ver);

int
sendsock4_setup(const char *mcast_name, const int port, const char *ifname,
		const char *src_addr, const int src_port, const u_char ttl);

int
sendsock6_setup(const char *mcast_name, const int port, const char *ifname,
		const char *src_addr, const int src_port, const u_char ttl);

#endif
