/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 *            Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/06/16
 *
 * $Id: decoder_buf.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include "pipe_hd.h"
#include "decoder_buf.h"

#include "debug.h"

#define BUFFER_NEXT(x)	(((x)+1) % buf_num)
#define BUFFER_PREV(x)	(((x)+buf_num-1) % buf_num)

static struct decoder_buf *dbuf;
static int      head, tail;
static int fd;
static int buf_size,buf_num;


int
decoder_buf_get_datanum(void)
{
	return ((head + buf_num - tail) % buf_num);
}


/* read from socket and write to buffer */
int
decoder_buf_read(void)
{
	int             len = 0;
	int             count = 0;
	char header[PIPE_HEADER_LEN];
#define MAX_LOOP 1

#if 0
	fprintf(stderr, "decoder_buf_read\n");
#endif	
	len=0;
	memset(header, 0, PIPE_HEADER_LEN);
	while (fd_can_read(fd,1000) && count++ < MAX_LOOP) {
		if (BUFFER_NEXT(head) == tail) {	/* buffer full */
			return -1;
		}
		/* TODO:呼出す関数はfd_canでもブロックされ得る，要改良 */
		len = pipe_blocked_read_message(fd, header,dbuf[head].buf,buf_size);

#if 1
		{
			static int c=0;
			d3_printf(" (%3d): len=%d\n", c++, len);
		}
#endif
		if (len > 0) {
			dbuf[head].len = len;
			dbuf[head].ts=pipe_get_timestamp(header);
			d3_printf("     TS%8x\n",dbuf[head].ts);
			head = BUFFER_NEXT(head);
		}
#if 0
		else if (len < 0) {
			dbuf[head].len = -2;	/* end marker */
			head = BUFFER_NEXT(head);
			//sock_close();
		} else {	/* len == 0 */
			dbuf[head].len = 0;
			/* ????? */
			head = BUFFER_NEXT(head);
			break;
		}
#else
		else if(len==PIPE_END || len==PIPE_ERROR){
			len=0;
			dbuf[head].len = -1;	/* end marker */
			head = BUFFER_NEXT(head);
			break;
		}
#endif
		
	}
	return len;
}


/* read from buffer and write to user's buf */
int
decoder_buf_get(void *buf, size_t nbytes,u_int32_t *ret_ts){
	int             len;

	if (tail == head) {	/* buffer empty */
		return 0;
	}
	len = dbuf[tail].len;
	if (len == -1) {	/* end of all data */
		return -1;
	} else if (len == 0) {
		d1_printf("\ndecoder_buf_get: data size = 0");
		return 0;
	}
	if (len > buf_size) {
		e_printf("\ndbuf[tail].len > buf_size, tail=%d, len=%d",
			 tail, len);
		exit(-1);
	}
	*ret_ts=dbuf[tail].ts;
	memcpy(buf, dbuf[tail].buf, len);
	tail = BUFFER_NEXT(tail);

	//d_printf("<%d>", decoder_buf_get_datanum());

	return len;
}

void decoder_buf_rm(int num){
  tail=(((tail)+buf_num+num) % buf_num);
  return;
}

int
decoder_buf_prebuf(void)
{
	head = tail = 0;
	do {
		decoder_buf_read();
	}while(head==tail);
	head = tail = 0;
	while (decoder_buf_get_datanum() < buf_num / 2) {
		decoder_buf_read();
	}
	return decoder_buf_get_datanum();
}


int
decoder_buf_capacity(void)
/*
 * return percentage of buffer capacity (0 - 100) 0 : empty 100: full
 */
{
	int             per;
	int             num;
	int             max = buf_num;

	num = (head + buf_num - tail) % buf_num;
	per = 100 - (num * 100 / max);
	return per;
}



void
decoder_buf_init(int fd_,int buf_size_,int buf_num_)
{
	int             i;
	u_int8_t       *p;
	fd=fd_;
	buf_size=buf_size_;
	buf_num=buf_num_;

	d1_printf("decoder_buf_init fd=%d dsize=%d dnum=%d\n"
		  ,fd,buf_size,buf_num);

	
	dbuf = (struct decoder_buf *) malloc(sizeof(struct decoder_buf)* buf_num);
	p= malloc(buf_size*buf_num);
	if (dbuf == NULL || p== NULL) {
		d_printf("decode_buff: cannot malloc for buf\n");
		exit(-1);
	}
	for (i = 0; i < buf_num; i++) {
		dbuf[i].buf=p+i*buf_size;
	}
	head = tail = 0;
	return;
}
