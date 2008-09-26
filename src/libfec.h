/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 * started:   2001/06/16
 *
 * $Id: libfec.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _libfec_h_
#define _libfec_h_

#include <sys/time.h>

#define FEC_TYPE_RS 1
#define FEC_TYPE_RS_STR "RS"

#if 0
const char     *fec_type_table[] =
{
	FEC_TYPE_RS_STR
};
#endif

#define MTU 1280

#define DEFAULT_FEC_M 4
#define DEFAULT_FEC_K 12
#define DEFAULT_FEC_F 3
#if 0
#define DEFAULT_FEC_S ((MTU)-(RTP_HEADER_LEN))
#endif
#define DEFAULT_FEC_S 1200
#define FEC_N_MAX	254	/* # of array of eras_pos */

/* ---------- typedef and global variable ---------- */

typedef struct _fec_queue_ {
	int             flag;	/* 0:available struct, 1:unavailable */
	int             ts;	/* time stamp for RTP */

	struct timeval  fire;	/* when the data was began to sent when the
				 * data will be recovered */
#if 0
    struct timeval send_finish;	/* estimate time all
							 * data will be sent */
#endif
	/* int PPS; */

	unsigned char  *data;
	unsigned char  *recv_flags;	/* only for receive check */
	/* int data_len;			not used ? */
}               fec_queue_t;

typedef struct _fec_param_ {	/* some parameters for FEC */
	int             MM;
	int             NN;	/* NN = (1<<MM)-1 */
	int             KK;	/* KK == Pnum */
	int             FF;	/* amount of all data is KK+FF */
	int             elm_size;	/* element size : length of a packet */
	int             dsc;	/* file descriptor for read/write */

	fec_queue_t    *fecq;
	int             fecq_len;	/* fec queue length */
	int             fecq_head;	/* fec queue head on sender  : write
					 * by application on receiver:
					 * receive from network */
	int             fecq_tail;	/* fec queue tail on sender  : read
					 * by this library and send on
					 * receiver: read by application */
	/* for interval receiving */
}               fec_param_t;

extern fec_param_t fecp;

/*
 * ---------- figure 1: relation among MM, NN, KK and nn ----------
 * 
 * |<------------- 2**MM-1 ------------>| |<--------------- NN --------------->|
 * |<------------ nn ------------>| |<-------- KK ------>|< nn-KK >|
 * |<-------- KK ------>|<- FF -->| |<-------- KK ------>|     |< nn-KK >|
 * |<-------- KK ------>|     |<- FF-- >| |<----- NN - nn + KK ----->|
 * 
 * example: (200,210) => KK=200, nn=210 -> MM=8, NN=254
 * 
 * 
 * ---------- figure 2: elm_size and KK, FF, NN ----------
 * 
 * |<------   elm_size  ------>| |============================| --- ---   (one
 * line = one packet) |============================|  |   |
 * |============================| KK   | |============================|  |
 * | |============================| ---  | |............................|
 * NN |............................|      | |============================|
 * ---  | |============================| FF   |
 * |============================| --- ---
 * 
 */

#define QueueNext(p,max)	(((p)+1) % (max))
#define QueuePrevious(p, max)	(((p)+(max)-1) % (max))
#define QueueSeek(p,max)	(((max)+(p)) % (max))

/* ---------- libfec.c ---------- */

double          fec_E_calc(int k, int n, double e);
int             fec_param_calc(int kk, double org_err, double acv_err, int *mm, int *ff);
int             fec_param_set(int mm, int kk, int ff, int elm_size);

int             fecs_fecadd(unsigned char *data);

#if 0
/* ---------- libfecs.c (for sender)---------- */
void            schedule_calc(struct timeval * start, struct timeval * target, int c, int r);
int             fecs_buffer_init(int buf_num);
int 
fecs_fecadd_send(int dsc, unsigned char *data, int timestamp,
		 struct timeval * sch, int frame_rate, int err_rate);
unsigned char  *fecs_buffer_get(void);	/* fecq_head */
void            fecs_buffer_free(void);	/* fecq_tail */

/* ---------- libfecr.c (for receiver)---------- */
int             fecr_buffer_init(int buf_num);
int             fecr_read(u_int32_t * ts, void *buf);
int             fecr_skip(void);
int             fecr_check(void);
void            fecr_recv_start(int dsc);
#endif

#endif				/* _libfec_h_ */
