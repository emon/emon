/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/06/16
 *
 * $Id: rtpdec.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <err.h>
#include <wlresvapi.h>
#include "def_value.h"
#ifdef RTPRECV
#include "multicast.h"
#endif /* RTPRECV */
#include "rtp_hd.h"
#include "pipe_hd.h"
#include "timeval.h"
#include "debug.h"


#define UINT_ADD(a,b,lastp1) (((a)+(b))%(lastp1))
#define UINT_SUB(a,b,lastp1) (((lastp1)+(a)-(b))%(lastp1))

#define SEQ_ADD(a,b) UINT_ADD(a,b,1<<16)
#define SEQ_SUB(a,b) UINT_SUB(a,b,1<<16) 
#define SEQ_DIF(a,b) ((int16_t)(a)-(int16_t)(b))


#define STATISTICS_INTERVAL (5)

/* if no packet is received for this time,
   we treat the last received packet as the last one of the sequence. */
#define PACKETINTERVAL_LIMIT (3) /* second */


typedef struct _pkt_t{
	struct _pkt_t *next;
	u_int8_t       marker;
	u_int32_t      ts;
	u_int32_t      plen;
	u_int16_t      seq;
	struct timeval waitlimit;
	u_int8_t       emonmsg[1]; 
	/* include marker,ts and length. if length==0 ,pkt is empty.
	   emonmsg size = opt.plen+{PIPE,RTP}_HEADER_LEN*/
} pkt_t;
#define PKT_T_SIZE(msg_len) (sizeof(pkt_t)-1+msg_len)

typedef struct _rtpbuf_t{
	/*last_xx mean xx of the last wrote packet .*/
	u_int8_t       last_maker; /* ==2 ,if last packet is empty packet and it's maker is unknown. */
	u_int32_t      last_ts;
	u_int16_t      last_seq;

        /* when the last recved pkt of seq was recv   */
	struct timeval last_recv_tv; 
        /* buf[now_idx] will be write at the first in buf */
	u_int32_t      now_idx;  

	/* wait_idx: the index of the first recieved packet in buf. 
	   buf[wait_idx] wait buf[now_idx...wait_idx-1] will be 
	   receive and write. */
	u_int32_t      wait_idx; 
	int32_t        wait_num;/* of packet is waiting in buf. */
	                        /* if wait_num == -1 , I wait for new rtp packet sequence. */
	pkt_t          **buf;  /*buf[A] == NULL, last_seq+1+A is not received .*/
	pkt_t          *empty; /*pkt_t list.
				 if you want to know the number of empty pkt_t,
				 you will get (opt.bufnum-wait_num).
				 so, rtpbuf.empty don't inform the number of empty pkt_t.
				.*/
} rtpbuf_t;

typedef struct _opt_t{	/* set by getopt */
#ifdef RTPRECV
	char           *addr;
	int             port;
	char           *ifname;
	int		ipver;
	int             fd;
	char           *raddr;
	int             rport;
	int             lambdano;
	int             lambdano_enable;
#endif /* RTPRECV */
	int             clock, freq, plen;
	u_int32_t       wait;
	u_int16_t       bufnum;
	int             rs_N;
	int             payload_type;
}opt_t;

static void statistics_print();
static void statistics_init();

static int rtpdec_minit(opt_t *opt,rtpbuf_t **ret_buf);
static int rtpdec_read(rtpbuf_t *buf,opt_t *opt);
/*
  if now_seq + opt->bufnum =< seq of recieved packet(new_seq) 
   if wait_num==0 
      reset rtpbuf;last_maker=1 last_seq=new_seq+1 now_seq=new_seq now_idx=wait_idx=0 wait_num=0;
   else 
      drop new packet;
  else 
      insert new packet to rtpbuf;
*/

static ssize_t rtpdec_bufpktwrite(rtpbuf_t *buf,u_int32_t idx);
static int rtpdec_pktwrite(pkt_t *p);
static int rtpdec_write(rtpbuf_t *buf,opt_t *opt);

/* 
   After write now_idx ... wait_idx , delete buf[wait_idx] and update last_ , wait_* and  now_* .
 */

static u_int64_t 
        total_pktrecv,		/* # of received packet */
	total_pktdrop,		/* # of packets which is received buf didn't inserted buf. */
	total_seqlen,		/* length of output packet including emtpy or skipped packet */
	total_seqskip;		/* length of skipped packet */

static u_int64_t
        statistics_nextprinttime;


int
opt_etc(int argc, char *argv[],opt_t *opt)
{
	int             ch;	/* for getopt */
	char           *temp;
#ifdef RTPRECV
	char           *opts = "C:R:L:N:y:W:B:A:P:I:V:D:a:p:l:";
#else /* RTPRECV */
	char           *opts = "C:R:L:N:y:W:B:D:";
#endif /* RTPRECV */
	char           *helpmes =
	"usage : %s [-C <clock>] [-R <freq>] [-L <packet len>] [-N <n>]\n"
	" [-y <payload type>] [-W <wait>] [-B <buffer num>]\n"
#ifdef RTPRECV
	" [-A <IP addr>] [-P <port>] [-I <interface>] [-V <IP-ver>]\n"
	" [-a <remote IP addr>] [-p <remote port>] [-l <lambdano>]\n"
#endif
	" [-D <debug level>]\n"
	" options\n"
	" <general>\n"
	"  C <n>   : clock base\n"
	"  R <n>   : frequency\n"
	"  L <n>   : length of a element\n"
	" <Read Solomon>\n"
	"  N <n>   : # of total packets\n"
	" <RTP>\n"
	"  y <n>   : RTP payload type\n"
	" <reorder>\n"
	"  W <n>   : maximum delay from stdin to stdout (msec)\n"
	"  B <n>   : buffer num (1sec=450)\n"
#ifdef RTPRECV
	" <network>\n"
	"  A <s>   : IP address\n"
	"  P <n>   : port #\n"
	"  I <s>   : receive network interface\n"
	"  V <n>   : IP version # (4 or 6)\n"
	"  a <s>   : remote IP address\n"
	"  p <n>   : remote port #\n"
	"  l <n>   : lambdano to reserve\n"
#endif /* RTPRECV */
	" <debug>\n"
	"  D <n>   : debug level\n"
	               ;
	/* set default value */
	opt->clock = DEF_EMON_CLOCK;
	opt->freq = DEF_EMON_FREQ;
	opt->plen = DEF_EMON_PLEN;
	opt->rs_N = DEF_EMON_RS_N;
	opt->payload_type = DEF_EMON_PT;
	opt->wait = 50;
	opt->bufnum = 450;
#ifdef RTPRECV
	opt->addr = DEF_RTPRECV_ADDR;
	opt->port = DEF_RTPRECV_PORT;
	opt->ifname = DEF_RTPRECV_IF;
	opt->ipver = 0;
	opt->raddr = NULL;
	opt->rport = 0;
	opt->lambdano_enable = 0;
#endif

	/* get environment variables */

	if ((temp = getenv("EMON_CLOCK")) != NULL)
		opt->clock = atoi(temp);
	if ((temp = getenv("EMON_FREQ")) != NULL)
		opt->freq = atoi(temp);
	if ((temp = getenv("EMON_PLEN")) != NULL)
		opt->plen = atoi(temp);
	if ((temp = getenv("EMON_RS_N")) != NULL)
		opt->rs_N = atoi(temp);
	if ((temp = getenv("EMON_PT")) != NULL)
		opt->payload_type = atoi(temp);
#ifdef RTPRECV
	if ((temp = getenv("RTPRECV_ADDR")) != NULL)
		opt->addr = temp;
	if ((temp = getenv("RTPRECV_PORT")) != NULL)
		opt->port = atoi(temp);
	if ((temp = getenv("RTPRECV_IF")) != NULL)
		opt->ifname = temp;
#endif /* RTPRECV */

	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'C':
			opt->clock = atoi(optarg);
			break;
		case 'R':
			opt->freq = atoi(optarg);
			break;
		case 'L':
			opt->plen = atoi(optarg);
			break;
		case 'N':
			opt->rs_N = atoi(optarg);
			break;
		case 'y':
			opt->payload_type = atoi(optarg);
			break;
		case 'W':
			opt->wait = atoi(optarg);
			break;
		case 'B':
			opt->bufnum = atoi(optarg);
			break;
#ifdef RTPRECV
		case 'A':	/* IP address */
			opt->addr = optarg;
			break;
		case 'P':	/* port# */
			opt->port = atoi(optarg);
			break;
		case 'I':
			opt->ifname = optarg;
			break;
		case 'V':
			opt->ipver = atoi(optarg);
			break;
		case 'a':
			opt->raddr = optarg;
			break;
		case 'p':
			opt->rport = atoi(optarg);
			break;
		case 'l':
			opt->lambdano = atoi(optarg);
			opt->lambdano_enable = 1;
			break;
#endif /* RTPRECV */
		case 'D':
			debug_level = atoi(optarg);
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
	}
	opt->wait *=1000;/* milisec => microsec */
	d1_printf("rtpdec/rtprecv: pt=%d\n", opt->payload_type);
	return 0;
}

int
main(int argc, char *argv[])
{
	int             mfd;
	struct timeval  nowtv;
	struct timeval  selecttv;
	rtpbuf_t        *buf;
	opt_t            opt;
	fd_set           fds_r;
	int              nfds,select_ret;

	if (opt_etc(argc, argv,&opt) == -1) {
		return -1;
	}
#ifndef RTPRECV
	if (isatty(STDIN)) {
		e_printf("%s:Standard input must be binded with pipe.\n",
			 argv[0]);
		return -1;
	}
#endif /* !RTPRECV */
	if (isatty(STDOUT)) {
		e_printf("%s:Standard output must be binded with pipe.\n",
			 argv[0]);
		return -1;
	}
	if(rtpdec_minit(&opt,&buf)){
		e_printf("need more memory\n");
	}
	

#ifdef RTPRECV
	mfd = recvsock_setup(opt.addr, opt.port, opt.ifname, opt.ipver);
	e_printf("\n");
	if (opt.raddr) {
		struct sockaddr_in sin;
		socklen_t slen = sizeof (sin);

		bzero(&sin, sizeof (sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons (opt.rport);
		inet_pton (AF_INET, opt.raddr, &sin.sin_addr);
		if (connect(mfd, (struct sockaddr *)&sin, slen) != 0) {
			e_printf ("\ncannot connect\n");
			return -1;
		}
	}
	if (opt.lambdano_enable) {
		struct wl_resv resv;
		struct wl_reserve_status status;
		socklen_t status_len;
		int ret;

		resv.wlr_flags = WL_RESV_FLAG_RECV;
		resv.wlr_lambda_no = opt.lambdano;
		ret = setsockopt (mfd, IPPROTO_IP, WL_ADD_RESV, &resv,
				  sizeof (resv));
		if (ret == -1) {
			d_printf ("setsockopt WL_ADD_RESV error");
			exit (-1);
		}

		status_len = sizeof (status);
		ret = getsockopt (mfd, IPPROTO_IP, WL_RESERVE_STATUS,
				  &status, &status_len);
		if (ret == -1) {
			d_printf ("getsockopt WL_RESERVE_STATUS error");
			exit (-1);
		}

		if (status.status != WL_RESV_STATUS_RESERVED){
			d_printf ("can't reserve lambdano");
			exit (-1);
		}
	}
	opt.fd=mfd;
#else /* RTPRECV */
	mfd = STDIN_FILENO;
#endif /* RTPRECV */
	nfds=mfd+1;
	statistics_init();
	while(1){
		switch(buf->wait_num){
		case -1:/* wait new sequence of packets, unlimited wait */
			selecttv.tv_sec=PACKETINTERVAL_LIMIT;
			selecttv.tv_usec=0;
			break;
		case 0: /* wait next sequential packet */
			selecttv.tv_sec=PACKETINTERVAL_LIMIT;
			selecttv.tv_usec=0;
			gettimeofday(&nowtv,NULL);
			timeval_sub(&nowtv,&(buf->last_recv_tv));
			/* 
			   nowtv==   timeofday-last
			   selecttv==    limit-last
			*/
			if(timeval_comp(&nowtv,&selecttv)>0){
				/* last <limit <timeofday */
				statistics_print();
				d1_printf("last seq/rtp=%u.\n\n",buf->last_seq);
				d1_printf("start to wait new packet sequence .\n");
				buf->wait_num=-1;
				continue;
			}
                        /*  last < timeofday <limit*/
			timeval_sub(&selecttv,&nowtv);
			break;
		default:/* wait next sequential packet until waitlimit of waiting packet have been received */
			gettimeofday(&nowtv,NULL);
			if(timeval_comp(&nowtv,&(buf->buf[buf->wait_idx]->waitlimit))>0){
				/* already exceeded waitlimit of waiting packet .*/

				rtpdec_write(buf,&opt);
				continue;
			}
			selecttv=timeval_sub_timeval(&(buf->buf[buf->wait_idx]->waitlimit),&nowtv);
			break;
		}
		FD_ZERO(&fds_r);
		FD_SET(mfd,&fds_r);
		select_ret=select(nfds,&fds_r,NULL,NULL,&selecttv);
		if(select_ret==-1){
			d1_printf("interrupt select \n");
			warn("interrupt select");
			continue;
		}
		if(FD_ISSET(mfd,&fds_r)){
			switch(rtpdec_read(buf,&opt)){
			case 0:	/* io_error */
				e_printf("failt to read packet \n");
				goto break_loop;
			case 1: /* recv and write first pkt of new seq */
			case 2: /* drop packet */
			case 3: /* recv pkt[idx], idx>now_idx. */
				d3_printf("append new packet .\n");
				break;
			case 4:	/*  recv packet[now_idx] */
				d3_printf("update waited packet .\n");
				rtpdec_write(buf,&opt);
				break;
			default :
				e_printf("read rtpdec_read usage .\n");
				return(0);
			}
			if(total_pktrecv%10000==0){
				statistics_print();
			}
			continue;
		}
		if(select_ret==0){
			switch(buf->wait_num){
			case -1: /* continue to wait new sequence */
				d2_printf("...wait new packet sequence \n");
				break;
			}
			continue;
		}
	}
	break_loop:
#ifdef RTPRECV
	multicast_leave(mfd, opt.addr, opt.ifname, opt.ipver);
	close(mfd);
#endif /* RTPRECV */

	return 0;
}



static int rtpdec_minit(opt_t *opt,rtpbuf_t **ret_buf){
	u_int32_t cnt;
	u_int8_t  *m;
	pkt_t     *pkt;
	size_t     s;
	rtpbuf_t  *buf; 
	size_t     msg_size;

#ifdef RTPRECV
	msg_size=opt->plen+RTP_HEADER_LEN;
#else
	msg_size=opt->plen+PIPE_HEADER_LEN+RTP_HEADER_LEN;
#endif
	s=0;
	s+=sizeof(rtpbuf_t);
	s+=(sizeof(pkt_t*)+PKT_T_SIZE(msg_size))*opt->bufnum;

	m=malloc(s);
	if(m==NULL){
		return 1;
	}
	*ret_buf=(rtpbuf_t*)m;

	buf=(rtpbuf_t*)m;
	m+=sizeof(rtpbuf_t);
	buf->buf=(pkt_t**)m;
	m+=sizeof(pkt_t*)*opt->bufnum;
	buf->now_idx=0;
	buf->wait_idx=0;
	buf->wait_num=-1;

	pkt=(pkt_t*)m;
	buf->empty=pkt;

	for(cnt=0;cnt<opt->bufnum;cnt++){
		buf->buf[cnt]=NULL;

		pkt=(pkt_t*)m;
		m+=PKT_T_SIZE(msg_size);
		pkt->next=(pkt_t*)m;
	}
	pkt->next=NULL;
	return 0;
}


/* 
   ret=rtpdec_read(buf,opt);
   switch(ret){
   case 0: #* io_error  *#
   case 1: #* recv and write first pkt of new seq *#
   case 2: #* drop packet *#
   case 3: #* recv pkt[idx], idx>now_idx. *#
   case 4: #* recv pkt[now_idx] and wait_idx=now_idx,so you write pkt[now_idx] *#
   }
 */
static int rtpdec_read(rtpbuf_t *buf,opt_t *opt){
	int             idx;
	int16_t         seq_dif; /* BUG=> int or int32  */
	ssize_t         io_ret;
	pkt_t          *pkt;
	int             ret;
        /* assert buf->empty !=NULL */
	pkt=buf->empty;
#ifdef RTPRECV
	io_ret=recv(opt->fd,pkt->emonmsg,opt->plen+RTP_HEADER_LEN,0);
	if(io_ret==-1){
		return 0;
	}
	pkt->marker=rtp_get_marker(pkt->emonmsg);
	pkt->ts=rtp_get_timestamp(pkt->emonmsg);
	pkt->plen=io_ret-RTP_HEADER_LEN;
	pkt->seq=rtp_get_seqnum(pkt->emonmsg);
#else
	io_ret=pipe_blocked_read_block(STDIN_FILENO,pkt->emonmsg,pkt->emonmsg+PIPE_HEADER_LEN,opt->plen+RTP_HEADER_LEN);
//fd_read_ignoreEOF(STDIN_FILENO,pkt->emonmsg,opt->plen+PIPE_HEADER_LEN+RTP_HEADER_LEN);
	if(io_ret==-1){
		return 0;
	}
	pkt->marker=rtp_get_marker(pkt->emonmsg+PIPE_HEADER_LEN);
	pkt->ts=rtp_get_timestamp(pkt->emonmsg+PIPE_HEADER_LEN);
	pkt->plen=io_ret-RTP_HEADER_LEN;
	pkt->seq=rtp_get_seqnum(pkt->emonmsg+PIPE_HEADER_LEN);
#endif
	total_pktrecv++;
//	d_printf("DDD ts%x plen%u seq%u\n",pkt->ts,pkt->plen,pkt->seq);	
//	d_printf("DDD io%d seq%d lastseq%d\n",io_ret,pkt->seq ,buf->last_seq);

	if(buf->wait_num==-1){ /* first packet of new sequence */
		d1_printf("start new packet sequence \n");
		d1_printf("first seq/rtp=%u.\n",pkt->seq);
		total_seqlen=1;
		total_seqskip=0;
		buf->now_idx=0;
		buf->wait_idx=0;
		buf->buf[0]=pkt;
		buf->empty=pkt->next;
		io_ret=rtpdec_bufpktwrite(buf,0);
		if(io_ret <0){
			return 0;
		}
		buf->wait_num=0;
		ret=1;goto recv;
	}
	seq_dif=SEQ_DIF(pkt->seq,buf->last_seq);
	if(seq_dif<=0 || seq_dif >opt->bufnum){
		/* drop */
		d2_printf("discard out of scope seq=%u\n",pkt->seq);
		total_pktdrop++;
		return 2;
	}
        /* assert (seq_dif >=1) */
	idx = UINT_ADD(buf->now_idx, seq_dif-1, opt->bufnum);
	if (buf->buf[idx]!= NULL){
		d2_printf("discard dup packet seq=%d\n", pkt->seq);
		total_pktdrop++;
		return 2;
        }
	buf->wait_num++;
	buf->buf[idx]=pkt;
	buf->empty=pkt->next;
/*
	d3_printf("\n[set:buf[idx] seq=%d seq_dif=%d idx=%d now_idx=%d wait_idx=%d wait_num=%d]",
		  pkt->seq, seq_dif, idx, buf->now_idx, buf->wait_idx, buf->wait_num);
*/
	if(buf->now_idx==idx){
		buf->wait_idx=idx;
		ret=4;goto recv;
	}
	gettimeofday(&(pkt->waitlimit),NULL);
	pkt->waitlimit.tv_sec +=(pkt->waitlimit.tv_usec+opt->wait)/(1000*1000);
	pkt->waitlimit.tv_usec =(pkt->waitlimit.tv_usec+opt->wait)%(1000*1000);
	if(buf->wait_num==1){
		buf->wait_idx=idx;
	}
	ret=3;
	goto recv;
recv:
	gettimeofday(&(buf->last_recv_tv),NULL);
	return ret;
}


static ssize_t rtpdec_pktwrite(pkt_t *p){
	struct iovec    iov[2];
	u_int8_t        p_hd[PIPE_HEADER_LEN];

	bzero(p_hd,PIPE_HEADER_LEN);
	pipe_set_version(p_hd,1);
	pipe_set_marker(p_hd,p->marker);
	pipe_set_timestamp(p_hd,p->ts);
	pipe_set_length(p_hd,p->plen);
	
	iov[0].iov_base = p_hd;
	iov[0].iov_len  = PIPE_HEADER_LEN;
#ifdef RTPRECV
	iov[1].iov_base = p->emonmsg+RTP_HEADER_LEN;
#else /* if RTPSDEC */	
	iov[1].iov_base = p->emonmsg+PIPE_HEADER_LEN+RTP_HEADER_LEN;
#endif
	iov[1].iov_len = p->plen;
	return(writev(STDOUT_FILENO,iov,2));
}


	
static ssize_t rtpdec_bufpktwrite(rtpbuf_t *buf,u_int32_t idx){
	pkt_t           *pkt;
	ssize_t          io_ret;
	
	pkt=buf->buf[idx];
	io_ret=rtpdec_pktwrite(pkt);
	if(io_ret<0){
		return io_ret;
	}
	d3_printf("[:bufpktw:wait_num--=%d]", buf->wait_num);
	buf->wait_num--;
	buf->last_maker=pkt->marker;
	buf->last_ts=pkt->ts;
	buf->last_seq=pkt->seq;

	buf->buf[idx]=NULL;
	pkt->next=buf->empty;
	buf->empty=pkt;
	return io_ret;
}


/* 
   write  pkt[now_idx...wait_idx];
   write  pkt[wait_idx+1,wait_idx+2,...,] while pkt[wait_idx+XX]!=NULL ;
   update now_idx,wait_idx;

   ret=rtpdec_pktwrite(buf,opt);
   switch(ret){
   case 0: #* io_error  *#
   case 1: #* succeeded *#
   }
 */
static int rtpdec_write(rtpbuf_t *buf,opt_t *opt){
	u_int8_t        p_hd[PIPE_HEADER_LEN];
	ssize_t         io_ret;

	pkt_t           *pkt;
	u_int32_t        idx;
	u_int32_t        nextidx;
	u_int32_t        rest;
	u_int32_t        sidx;
	struct timeval   tv;

	bzero(p_hd,PIPE_HEADER_LEN);
	pipe_set_version(p_hd,1);
	pipe_set_length(p_hd,0);

	//assert (buf->wait_num>0)
	nextidx=buf->now_idx;
	do{
		idx=nextidx;
		nextidx=UINT_ADD(idx,1,opt->bufnum);
		pkt=buf->buf[idx];
		if(pkt==NULL){	/* pkt is not received,but has exceeded wait limit. */
			pkt_t           *nextpkt;
			nextpkt=buf->buf[nextidx]; 
			if(1&&
			   buf->last_maker==0 &&
			   nextpkt!=NULL &&
			   buf->last_ts != nextpkt->ts){
				buf->last_maker=1;
				pipe_set_marker(p_hd,1);
				d2_printf("recover maker\n");
			}else{
				buf->last_maker=2;
				pipe_set_marker(p_hd,0);
			}
			pipe_set_timestamp(p_hd,buf->last_ts); 
			//buf->last_ts=buf->last_ts;
			//buf->last_seq=UINT_ADD(buf->last_seq,1);
			io_ret=write(STDOUT_FILENO,p_hd,PIPE_HEADER_LEN);
			if(io_ret==-1){
				return 0;
			}
			total_seqskip++;
		}else{		/* pkt was received and now write it. */
			io_ret=rtpdec_bufpktwrite(buf,idx);
			if(io_ret==-1){
				return 0;
			}
			total_seqlen++;
		}
	}while(idx!=buf->wait_idx);

	/* write packet sequence after buf[wait_idx]. */
	/* assert (idx==buf->wait_idx); */
	while(1){
		idx=UINT_ADD(idx,1,opt->bufnum);
		pkt=buf->buf[idx];
		if(pkt==NULL){
			break;
		}
		io_ret=rtpdec_bufpktwrite(buf,idx);
		if(io_ret==-1){
			return 0;
		}
		total_seqlen++;
	}
	/*assert(buf->buf[idx]==NULL)*/
	buf->now_idx=idx;
	if(buf->wait_num==0){
		return 1;
	}
	/* search buf->buf for wait_idx */
	tv.tv_sec=(0x7fffffff);
	tv.tv_usec=0;
	rest=buf->wait_num;
	d2_printf("\n[rest=%d, bufnum=%d, now_idx=%d, wait_idx=%d, wait_num=%d]", 
		  rest, opt->bufnum, buf->now_idx, buf->wait_idx, buf->wait_num);
	sidx=idx;
	do{
		idx=UINT_ADD(idx,1,opt->bufnum);
		pkt=buf->buf[idx];
		if(pkt==NULL){
			if (idx==sidx){
				d3_printf("no pkt in buf [idx=%d]\n", idx);
				buf->wait_idx=idx;
				return 1;
			}
			continue;
		}
		rest--;
		if(timeval_comp(&(pkt->waitlimit),&tv)<0){ 
                        /* waitlimit < tv */
			buf->wait_idx=idx;
			tv.tv_sec =pkt->waitlimit.tv_sec;
			tv.tv_usec=pkt->waitlimit.tv_usec;
		}
	}while(rest);
	return 1;
}

static void statistics_print(){
	d1_printf(
		"rtpdec:(recv%10llu)(drop%10llu)(seq%10llu)(seqskip%10llu)\n"
		,total_pktrecv,total_pktdrop,total_seqlen,total_seqskip);
	return;
}

static void statistics_init(){
        total_pktrecv=0;
	total_pktdrop=0;
	total_seqlen=0;	
	total_seqskip=0;
	statistics_nextprinttime=0;
}
