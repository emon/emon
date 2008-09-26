/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/06/19
 *
 * $Id: pipe_hd.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>

#include "pipe_hd.h"
#include "rtp_hd.h"
#include "debug.h"

#define MAX(a,b) ((a)>(b)? (a):(b))
#define MIN(a,b) ((a)>(b)? (b):(a))

static int exit_req=0;

void            pipe_exit_req(){
	exit_req=1;
}

void 
pipe_set_version(void *hd, u_int8_t version)
{
	u_int8_t       *h = (u_int8_t *) hd;
	h[0] = (h[0] & 0x0F) | ((version << 4) & 0xF0);
}

void 
pipe_set_marker(void *hd, u_int8_t marker)
{
	u_int8_t       *h = (u_int8_t *) hd;
	if (marker) {
		h[0] = h[0] | 0x08;
	} else {
		h[0] = h[0] & 0xF7;
	}
}

u_int8_t 
pipe_get_version(void *hd)
{
	u_int8_t       *h = (u_int8_t *) hd;
	return (h[0] >> 4) & 0x0F;
}

u_int8_t 
pipe_get_marker(void *hd)
{
	u_int8_t       *h = (u_int8_t *) hd;
	if (h[0] & 0x8)
		return 1;
	return 0;
}

static ssize_t
pipe_blocked_read_header(int fd, void *hd)
{
	if (fd_read_ignoreEOF(fd, hd, PIPE_HEADER_LEN) != PIPE_HEADER_LEN) {
		e_printf("Can't read EMON message header.\n");
		return PIPE_ERROR;
	}
	if(pipe_get_version(hd)!=1) {
		e_printf("Unavailable EMON message version %d.\n",pipe_get_version(hd));
		return PIPE_ERROR;
	}
	if(pipe_get_length(hd)==END_FLAG) {
		return PIPE_END;
	}
	return PIPE_HEADER_LEN;
}

ssize_t 
pipe_blocked_read_block(int fd, void *hd, void *payload, int maxlen)
{
	ssize_t         io_ret, left_size, ret;
	char           *pos = (char *) payload;

	if ((ret = pipe_blocked_read_header(fd,hd))<0)
		return ret;
	left_size = pipe_get_length(hd);
	d3_printf("left_size=%d\n", left_size);
	if(left_size > maxlen) {
		d_printf("Length over %d(must be less than %d), skip.\n"
			 ,left_size,maxlen);
		pipe_set_length(hd,0);
		io_ret=fd_lseek_ignoreEOF(fd,left_size);
		if (io_ret < 0)
			return PIPE_ERROR;
	}else{
		io_ret=fd_read_ignoreEOF(fd,pos,left_size);
		if (io_ret < 0)
			return PIPE_ERROR;
	}
	return pipe_get_length(hd);
}

ssize_t 
pipe_blocked_read_message(int fd, void *hd, void *payload, int maxlen)
{
	ssize_t         read_size, left_size, total_size = 0, ret;
	char           *pos = (char *) payload;
	int             flag_over=0;
	int             flag_packetloss=0;
	int             ts;
	ts=-1;
	do {
	       
		if ((ret = pipe_blocked_read_header(fd,hd))<0)
			return ret;
		left_size = pipe_get_length(hd);
		if(left_size==0){
			flag_packetloss=1;
		}else{
		   if(ts==-1){
		     ts=pipe_get_timestamp(hd);
		   }else{
		     if(ts!=pipe_get_timestamp(hd))
		       flag_packetloss=1;
		   }
		}
		total_size += left_size;
		if(total_size>maxlen && flag_over ==0&& flag_packetloss==0){
			flag_over=1;
			d_printf("Length over %d(must be less than %d), skip.\n",total_size,maxlen);
		}
		
		if(flag_over||flag_packetloss) {
			read_size=fd_lseek_ignoreEOF(fd,left_size);
			if(read_size<0){
				return PIPE_ERROR;
			}
		}else{
			read_size=fd_read_ignoreEOF(fd,pos,left_size);
			if(read_size<0){
				return PIPE_ERROR;
			}
			pos+=left_size;
		}
	} while (!pipe_get_marker(hd));
	if(flag_over||flag_packetloss)
		pipe_set_length(hd, 0);
	else
		pipe_set_length(hd, total_size);
	return pipe_get_length(hd);
}

ssize_t 
pipe_blocked_read_message_ex(int fd, void *hd, void *payload, int *eras_pos,
			     int *no_eras, int num_packet, int plen)
{
	ssize_t		read_size, left_size, total_size = 0, ret;
	char		*pos = (char *) payload;
	int		idx = 0;

	*no_eras = 0;
	do {
		if ((ret = pipe_blocked_read_header(fd, hd)) < 0 ){
			return ret;
		}
		if (pipe_get_length(hd) == 0) {
			eras_pos[(*no_eras)++] = idx;
			memset(pos, 0, plen);
			total_size += plen;
			pos += plen;
		} else {
			left_size = pipe_get_length(hd);
			total_size += left_size;
			while (left_size > 0) {
				if ((read_size = read(fd, pos, left_size)) < 0)
					return PIPE_ERROR;
				left_size -= read_size;
				pos += read_size;
			}
		}
		idx++;
	} while (!pipe_get_marker(hd) && --num_packet >0);
	pipe_set_length(hd, total_size);
	return pipe_get_length(hd);
}

typedef struct _TRUE_PIPE_CONTEXT {
	int fd;
	int num_packet,packet_len;
	int read_pointer,write_pointer,num_left,flag_marker;
	char header[PIPE_HEADER_LEN];
	char *buffer;
} *TRUE_PIPE_CONTEXT;

PIPE_CONTEXT
pipe_context_init(int fd, int num_packet, int packet_len)
{
	TRUE_PIPE_CONTEXT p=malloc(sizeof(struct _TRUE_PIPE_CONTEXT));
	p->fd=fd;
	p->num_packet=num_packet;
	p->packet_len=packet_len;
	p->read_pointer=p->write_pointer=p->num_left=p->flag_marker=0;
	p->buffer=malloc(num_packet*packet_len);

	return p;
}




ssize_t
pipe_blocked_read_packet_ex(PIPE_CONTEXT pp, void *hd, void *payload)
{
	TRUE_PIPE_CONTEXT p=(TRUE_PIPE_CONTEXT)pp;
	int length,ret;
	int len_sum;

	if(p->num_left==0){
		len_sum=0;
	}else{
		length=fd_read_ignoreEOF(p->fd,payload
				     ,MIN(p->num_left,p->packet_len));
		if(length==-1) return PIPE_ERROR;
		p->num_left-=length;
		len_sum=length;
	}
	/* Now, num_left==0 or len_sum==p->packet_len*/
	while(!p->flag_marker && len_sum < p->packet_len){
		if ((ret = pipe_blocked_read_header(p->fd,p->header))<0)
			return ret;
		p->num_left=pipe_get_length(p->header);
		p->flag_marker=pipe_get_marker(p->header);
		d3_printf("pbrpe: read pipe header num_left:%d marker:%d.\n"
			  ,p->num_left,p->flag_marker);
		length=fd_read_ignoreEOF(p->fd,payload+len_sum
				     ,MIN(p->num_left,p->packet_len-len_sum));
		if(length==-1) return PIPE_ERROR;
		p->num_left-=length;
		len_sum+=length;
	}
        /* fill PIPE header */
	memset(hd, 0, PIPE_HEADER_LEN);
	pipe_set_version(hd,1);
	pipe_set_timestamp(hd,pipe_get_timestamp(p->header));
	pipe_set_length(hd,len_sum);
	if(p->flag_marker&&p->num_left==0){
		pipe_set_marker(hd,1);
		p->flag_marker=0;
	}
	return len_sum;
}

void
pipe_context_uninit(PIPE_CONTEXT pp)
{
	TRUE_PIPE_CONTEXT p=(TRUE_PIPE_CONTEXT)pp;
	free(p->buffer);
	free(p);
}

ssize_t 
pipe_blocked_write_block(int fd, void *hd, void *payload)
{
	ssize_t         write_size, left_size;
	char           *pos = (char *) payload;

	if (write(fd, hd, PIPE_HEADER_LEN) != PIPE_HEADER_LEN)
		return PIPE_ERROR;
	left_size = pipe_get_length(hd);
	while (left_size > 0) {
		if ((write_size = write(fd, pos, left_size)) < 0)
			return PIPE_ERROR;
		left_size -= write_size;
		pos += write_size;
	}
	return pipe_get_length(hd);
}

int
fd_can_read(int fd, int usec)
{
        static fd_set   rset;
        struct timeval  fire;

        FD_ZERO(&rset);
        FD_SET(fd, &rset);
        fire.tv_sec = 0;
        fire.tv_usec = usec;
        select(fd + 1, &rset, NULL, NULL, &fire);
        return FD_ISSET(fd, &rset);
}

int
fd_can_read_blocked(int fd)
{
	static fd_set   rset;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	select(fd + 1, &rset, NULL, NULL, NULL);
	return FD_ISSET(fd, &rset);
}

int
fd_can_write(int fd, int usec)
{
        static fd_set   wset;
        struct timeval  fire;

        FD_ZERO(&wset);
        FD_SET(fd, &wset);
        fire.tv_sec = 0;
        fire.tv_usec = usec;
        select(fd + 1, NULL, &wset, NULL, &fire);
        return FD_ISSET(fd, &wset);
}



ssize_t
fd_read_ignoreEOF(int d, void *buf, size_t nbytes)
{
	ssize_t left;
	ssize_t io_ret;
	char *s = (char*)buf;

	for (left = nbytes; left > 0; left -= io_ret){
		fd_can_read_blocked(d);
		io_ret = read(d, s, left);
		if (io_ret <= 0 || exit_req == 1){
			if (io_ret == 0){
				debug_printf2("PIPE_EOF_EOF\n");
			}
			return PIPE_ERROR;
		}
		s += io_ret;
	}
	return nbytes;
}


ssize_t
fd_lseek_ignoreEOF(int d, size_t nbytes)
{
	u_int8_t *buf[4096];
	ssize_t left;
	ssize_t io_len,io_ret;

	left = nbytes;
	for (;;){
		io_len = MIN(4096, left);
		io_ret = fd_read_ignoreEOF(d, buf, io_len);
		if (io_ret == PIPE_ERROR){
			return PIPE_ERROR;
		}
		left -= io_ret;
		if (left == 0){
			return nbytes;
		}
	}
}
