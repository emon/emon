/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 * started:   2001/06/16
 *
 * $Id: decoder_buf.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _decoder_buf_h_
#define _decoder_buf_h_

#define BUFFERSIZE (1024*50)
#define BUFFERLEN  (15)

struct decoder_buf {
	int             len;	/* available data length in the 'buf', 0:
				 * there are no data -1: end of all data */
        u_int32_t       ts;	/* TimeStamp in RTP */
	u_int8_t       *buf;
};

int             decoder_buf_get_datanum(void);
int             decoder_buf_read(void);
int             decoder_buf_get(void *buf, size_t nbytes,u_int32_t *ret_ts);
int             decoder_buf_prebuf(void);
void            decoder_buf_init(int fd,int buf_size,int buf_num);
void            decoder_buf_rm(int num);

#endif
