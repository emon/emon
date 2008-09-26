/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): ?
 *            FUJIKAWA Kenji <fujikawa@i.kyoto-u.ac.jp>
 * started:   2001/08/03
 *
 * $Id: ntsputil.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _NTSPUTIL_H_
#define _NTSPUTIL_H_

#define NTSP_TOPIPE   0
#define NTSP_TONET    1

int util_get_sa_len(struct sockaddr *sa);
int util_clear_port(struct sockaddr *sa);
int util_addrcmp(struct sockaddr *addr1, struct sockaddr *addr2);
int util_hostcmp(struct sockaddr *addr1, struct sockaddr *addr2);

int util_close_virtually(int so);
int util_make_sametype_socket(int so, struct sockaddr *addr, int *addrlen);
int util_make_back_socket(int so);
int util_make_copy_socket(int so);

int forward(int so_from, int so_to);
int forward2(int so_from, int so_to, int type);

int set_nonblock(int fd);

#endif /* _NTSPUTIL_H_ */

/* EOF */
