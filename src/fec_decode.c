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
 * $Id: fec_decode.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

#include "libfec.h"
#include "rs_emon.h"
#include "fec_decode.h"
#include "pipe_hd.h" /* for PIPE_HEADER_LEN */
#include "timeval.h"
#include "debug.h"

#define FEC_BUF_DIV ((sizeof(int)+fecp.elm_size)*(fecp.KK+fecp.FF)+sizeof(int))
#define FEC_BUF_DATA(buffer,pos,idx) ((char*)(buffer)+(pos*FEC_BUF_DIV)+sizeof(int)*(fecp.KK+fecp.FF+1)+((idx)*fecp.elm_size))
#define FEC_BUF_NO_ERAS(buffer,pos) (*(int*)((char*)(buffer)+(pos*FEC_BUF_DIV)))
#define FEC_BUF_ERAS_POS(buffer,pos) ((int*)((char*)(buffer)+(pos*FEC_BUF_DIV)+sizeof(int)))

/* define prototype */
static int      fecr_recover8(unsigned char *data, int *eras_pos, int no_eras);
static int      fecr_recover4(unsigned char *data, int *eras_pos, int no_eras);
static int      fecr_recover(unsigned char *data, int *eras_pos, int no_eras);
static int      fecr_clear(int pos);
static void     fecr_set_data(unsigned char *data);

static char *buffer,*buffer2;
static u_int32_t *ts_buffer;
static int thr_with_fec, thr_without_fec;

static void fecr_wait_marker(){
  char p_header[PIPE_HEADER_LEN];

  while(1){
    pipe_blocked_read_block(fecp.dsc, p_header, buffer2, fecp.elm_size*(fecp.KK+fecp.FF));
    if(pipe_get_marker(p_header)){
      d1_printf("fount marker\n");
      return;
    }
  }
}

int
fecr_buffer_init(int bufnum, int thr_with_fec_, int thr_without_fec_)
{
	fecp.fecq_len = bufnum;
	fecp.fecq_head = fecp.fecq_tail = 0;
	thr_with_fec = thr_with_fec_;
	thr_without_fec = thr_without_fec_;

	buffer=malloc(FEC_BUF_DIV*fecp.fecq_len);
	ts_buffer=malloc(sizeof(u_int32_t)*fecp.fecq_len);
	if(!buffer || ! ts_buffer) {
		e_printf("Can't allocate buffer.\n");
		return -1;
	}
	memset(buffer, 0, FEC_BUF_DIV*fecp.fecq_len);
	buffer2=malloc(fecp.elm_size*(fecp.KK+fecp.FF));
	if(!buffer2) {
		e_printf("Can't allocate buffer.\n");
		return -1;
	}
	memset(buffer2, 0, fecp.elm_size*(fecp.KK+fecp.FF));

	fecr_wait_marker();
	return 0;
}

int
fecr_read_recover(unsigned char *data,u_int32_t *ret_ts)
{
	int             i;
	int             clean_flag = 0;	/* 0: dirty data, 1: clean data */

	char            output[fecp.NN + 1];
	int remain;

	if (fecp.fecq_tail == fecp.fecq_head){
		d3_printf("\nfecr_read: queue is empty (head=tail=%d)", fecp.fecq_head);
		return 0;
	}

	remain = (fecp.fecq_len+fecp.fecq_head-fecp.fecq_tail)	% fecp.fecq_len;
	if (thr_without_fec && remain > ((fecp.fecq_len*(thr_without_fec-1))/thr_without_fec)){
		d2_printf("skip(time limit)");
	} else {
		fecr_set_data(data);
		if (FEC_BUF_NO_ERAS(buffer, fecp.fecq_tail) == 0) {
			clean_flag = 1;
		} else {
			for (i = 0; i < fecp.KK; i++)
				output[i] = '-';
			for (; i < fecp.NN - fecp.FF; i++)
				output[i] = '.';
			for (; i < fecp.NN; i++)
				output[i] = '-';
			for (i = 0; i < FEC_BUF_NO_ERAS(buffer, fecp.fecq_tail) ; i++)
				output[FEC_BUF_ERAS_POS(buffer,fecp.fecq_tail)[i]] = 'X';
			output[fecp.NN] = '\0';
#if 0
			d2_printf("\nTS:%5d, packet loss %s", fecq->ts, output);
#endif
			d2_printf("\n packet loss %s", output);
			
			if (FEC_BUF_ERAS_POS(buffer,fecp.fecq_tail)[0] >= fecp.KK) {	/* only FEC packets lost */
				clean_flag = 1;
				d2_printf(" original data clean");
			} else {
				if (FEC_BUF_NO_ERAS(buffer,fecp.fecq_tail) > fecp.FF) {
				d2_printf(" skip (too many errors)");
				} else {
					d3_printf("remain %d\n",remain);
					if (thr_with_fec && remain > ((fecp.fecq_len*(thr_with_fec-1))/thr_with_fec)){
						d2_printf("recover skip(time limit)");
					} else {
						int ret;
						
						d2_printf(" recover..");
						ret = fecr_recover(data, FEC_BUF_ERAS_POS(buffer, fecp.fecq_tail), FEC_BUF_NO_ERAS(buffer, fecp.fecq_tail));
						if (ret == FEC_BUF_NO_ERAS(buffer,fecp.fecq_tail)) {
							d2_printf("complete(%d)", ret);
							clean_flag = 1;
						} else if (ret < FEC_BUF_NO_ERAS(buffer,fecp.fecq_tail)) {
							d2_printf("maybe OK(%d)", ret);
							clean_flag = 1;
						} else {
							d2_printf("fail!!!!(%d)", ret);
						}
					}
				}
			}
		}		
	}
	*ret_ts=ts_buffer[fecp.fecq_tail];
	fecr_clear(fecp.fecq_tail);
	fecp.fecq_tail=QueueNext(fecp.fecq_tail,fecp.fecq_len);

	return clean_flag;
}

static int
fecr_clear(int pos) {
	memset(&FEC_BUF_NO_ERAS(buffer,pos),0,FEC_BUF_DIV);
	return 0;
}

static void
fecr_set_data(unsigned char *data)
{
	memset(data, 0, fecp.elm_size * fecp.NN);
	memcpy(data, FEC_BUF_DATA(buffer, fecp.fecq_tail, 0), fecp.elm_size * fecp.KK);
	memcpy(data+fecp.elm_size*(fecp.NN-fecp.FF), FEC_BUF_DATA(buffer,fecp.fecq_tail, fecp.KK), fecp.elm_size * fecp.FF);
}

int fecr_check(void)
{
	return (fecp.fecq_head != fecp.fecq_tail);
}

int
fecr_recover(unsigned char *data, int* eras_pos, int no_eras)
{
	int             ret = -1;

	switch (fecp.MM) {
	case 4:
		ret = fecr_recover4(data, eras_pos, no_eras);
		break;
	case 8:
		ret = fecr_recover8(data, eras_pos, no_eras);
		break;
	default:
		e_printf("Now this libraly can handle "
			 "only when MM==4 or MM=8\n");
		break;
	}
	return ret;
}


static int
fecr_recover8(unsigned char *data, int *eras_pos, int no_eras)
{
	int             i;
	const int       plen = fecp.elm_size;
	int             num_cor = 0;

	for (i = 0; i < plen; i++) {
		dtype          *data_step = (dtype *) (data + i);

		num_cor = eras_dec_rs8_step(data_step, plen, eras_pos, no_eras);
		if (num_cor != no_eras) {
#if 0
			d3_printf("\nfec_recover8: can not correct all errors."
				 "i=%d, # err=%d, # correct=%d",
				 i, no_eras, num_cor);
#endif
		}
		if ((i % (plen/16)) == 0){
			if (fd_can_read(fecp.dsc, 0)){
				fecr_recv();
			}
		}
	}
	return num_cor;
}


static int
fecr_recover4(unsigned char *data, int *eras_pos, int no_eras)
{
	int             i;
	const int       plen = fecp.elm_size;
	int             num_cor = 0;

	for (i = 0; i < plen; i++) {
		dtype          *data_step = (dtype *) (data + i);

		num_cor = eras_dec_rs4_step(data_step, plen, eras_pos, no_eras);

		if (num_cor != no_eras) {
#if 0
			d3_printf("\nfec_recover4: can not correct all errors."
				 "i=%d, # err=%d, # correct=%d",
				 i, no_eras, num_cor);
#endif
		}
		if ((i % (plen/16)) == 0){
			if (fd_can_read(fecp.dsc, 0)){
				fecr_recv();
			}
		}
	}
	return num_cor;
}


int
fecr_recv(void)
{
	static int idx;
	char p_header[PIPE_HEADER_LEN];

	pipe_blocked_read_block(fecp.dsc, p_header, buffer2, fecp.elm_size*(fecp.KK+fecp.FF));

	if(pipe_get_length(p_header)==0) { /* error */
		d3_printf("timestamp %u block %d packet read as error to pos %d.\n",pipe_get_timestamp(p_header),idx,fecp.fecq_head);
		FEC_BUF_ERAS_POS(buffer,fecp.fecq_head)[FEC_BUF_NO_ERAS(buffer, fecp.fecq_head)++]=idx;
	} else if(pipe_get_length(p_header) == fecp.elm_size*(fecp.KK+fecp.FF)) {
		d3_printf("timestamp %u block read to pos %d.\n",pipe_get_timestamp(p_header),fecp.fecq_head);
		FEC_BUF_NO_ERAS(buffer, fecp.fecq_head)=0;
		memcpy(FEC_BUF_DATA(buffer,fecp.fecq_head,0), buffer2, fecp.elm_size*(fecp.KK+fecp.FF));
		ts_buffer[fecp.fecq_head]=pipe_get_timestamp(p_header);
		idx=fecp.KK+fecp.FF-1;
	} else {
		d3_printf("timestamp %u block %d packet read to pos %d.\n",pipe_get_timestamp(p_header),idx,fecp.fecq_head);
		memcpy(FEC_BUF_DATA(buffer,fecp.fecq_head,idx), buffer2, fecp.elm_size);
		ts_buffer[fecp.fecq_head]=pipe_get_timestamp(p_header);
	}

	idx++;
	if(idx == fecp.KK + fecp.FF) {
		idx = 0;
		fecp.fecq_head = QueueNext(fecp.fecq_head, fecp.fecq_len);
		if(fecp.fecq_head == fecp.fecq_tail) {
			fecr_clear(fecp.fecq_tail);
			fecp.fecq_tail = QueueNext(fecp.fecq_tail,fecp.fecq_len);
			d3_printf("queue full, move tail %d->%d\n", fecp.fecq_head, fecp.fecq_tail);
		}
	}

	return 0;
}

void
fecr_set_fd(int d)
{
	fecp.dsc = d;
}

