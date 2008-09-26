/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 *            Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 * started:   2001/06/26
 *
 * $Id: fec_decode.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _fec_decode_h_
#define _fec_decode_h_

int fecr_buffer_init(int bufnum, int thr_with_fec, int thr_without_fec);
int fecr_read_recover(unsigned char *data,u_int32_t *ret_ts);
int fecr_check(void);
int fecr_recv(void);
void fecr_set_fd(int d);

#endif
