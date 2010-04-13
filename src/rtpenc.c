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
 * $Id: rtpenc.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include <wlresvapi.h>

#include "def_value.h"
#ifdef RTPSEND
#include "sendsock_setup.h"
#endif /* RTPSEND */
#include "rtp_hd.h"
#include "pipe_hd.h"
#include "timeval.h"
#include "debug.h"


#ifdef RTPSEND
static void statistics_display();
#endif

static struct opt_values {	/* set by getopt */
#ifdef RTPSEND
	int             send_err_rate;
	int             sfd;
        int             shaping_lev; /* 0:frame 1:packet with dups 2:packet*/
#endif /* RTPSEND */
        int             dup2dummy; /* 0:only dup 1:dup to dummy*/
	int             send_double;
	int             compatible_mode;
	int             clock, freq, plen;
	int             rs_N;
	int             payload_type;
	int		ipver;
}               OPT;

struct timeval  wclk_ps_tvstart;
#ifdef RTPSEND
struct timeval  wclk_tvstart, wclk_tvnow;
u_int64_t       rtsend_tslimit;
u_int32_t       pkt_tslast,pkt_tsnow;

int             display_time_all_packet = 0;	/* flag */
u_int64_t       total_send = 0;
u_int64_t       total_drop = 0;
u_int16_t       last_seq;

char           *addr;	/* IP address or domainname */
int             port;
char           *src_addr;
int             src_port;
int		lambdano;
int		lambdano_enable;
#endif /* RTPSEND */

int
opt_etc(int argc, char *argv[])
{
	int             ch;	/* for getopt */
	char           *temp;

#ifdef RTPSEND
	u_char          ttl;
	char           *ifname;
	char           *opts = "C:R:L:N:y:t:I:A:P:a:p:V:D:e:s:l:d0123";
#else
	char           *opts = "C:R:L:N:y:D:0123d";
#endif /* RTPSEND */
	char           *helpmes =
	"usage : %s [-C <clock>] [-R <freq>] [-L <packet len>]\n"
	" [-N <n>] [-y <payload type>] [-t <ttl>] [-I <interface>]\n"
#ifdef RTPSEND
	" [-A <IP-addr>] [-P <port>] [-a <src IP-addr>] [-p <src port>]\n"
	" [-V <IP-ver>] [-D <debug level>] [-e <error rate>] [-0123s]\n"
	" [-l lambdano]\n"
#else
	" [-D <debug level>] [-0]\n"
#endif /* RTPSEND */
	" options\n"
	" <general>\n"
	"  C <n>   : clock base\n"
	"  R <n>   : frequency\n"
	"  L <n>   : length of a element\n"
	" <Reed Solomon>\n"
	"  N <n>   : # of total packets\n"
	" <RTP>\n"
	"  y <n>   : RTP payload type\n"
#ifdef RTPSEND
	" <network>\n"
	"  A <s>   : dst IP address\n"
	"  P <n>   : dst port #\n"
	"  a <s>   : src IP address\n"
	"  p <n>   : src port #\n"
	"  t <n>   : IP ttl\n"
	"  I <s>   : send network interface name \n"
	"  V <n>   : IP version # (4 or 6)\n"
	"  s       : shaping level (default=2  0:frame 1:packet and dups 2:packet)\n"
	"  l <n>   : lambdano to reserve\n"
#endif
	"  d       : duped packet to dummy \n"
	" <debug>\n"
	"  D <n>   : debug level\n"
#ifdef RTPSEND
	"  e <n>   : error rate (100 -> 1/100, 1000 -> 1/1000 ...)\n"
#endif
	"  0       : compatible mode (sequence number starts from 0)\n"
	"  1/2/3   : packet rate normal/double/triple\n"
	               ;
	/* set default value */
#ifdef RTPSEND
	addr = DEF_RTPSEND_ADDR;
	port = DEF_RTPSEND_PORT;
	src_addr = NULL;
	src_port = -1;
	ifname = DEF_RTPSEND_IF;
	ttl = DEF_RTPSEND_TTL;
	OPT.shaping_lev=2;
	lambdano_enable = 0;
#endif /* RTPSEND */
	OPT.clock = DEF_EMON_CLOCK;
	OPT.freq = DEF_EMON_FREQ;
	OPT.plen = DEF_EMON_PLEN;
	OPT.rs_N = DEF_EMON_RS_N;
	OPT.payload_type = DEF_EMON_PT;
	OPT.dup2dummy=0;
	OPT.send_double = 1;
	OPT.ipver = 0;
#ifdef RTPSEND
	OPT.send_err_rate = 0;
#endif /* RTPSEND */

	/* get environment variables */

	if ((temp = getenv("EMON_CLOCK")) != NULL)
		OPT.clock = atoi(temp);
	if ((temp = getenv("EMON_FREQ")) != NULL)
		OPT.freq = atoi(temp);
	if ((temp = getenv("EMON_PLEN")) != NULL)
		OPT.plen = atoi(temp);
	if ((temp = getenv("EMON_RS_N")) != NULL)
		OPT.rs_N = atoi(temp);
	if ((temp = getenv("EMON_PT")) != NULL)
		OPT.payload_type = atoi(temp);
#ifdef RTPSEND
	if ((temp = getenv("RTPSEND_ADDR")) != NULL)
		addr = temp;
	if ((temp = getenv("RTPSEND_PORT")) != NULL)
		port = atoi(temp);
	if ((temp = getenv("RTPSEND_SRCADDR")) != NULL)
		src_addr = temp;
	if ((temp = getenv("RTPSEND_SRCPORT")) != NULL)
		src_port = atoi(temp);
	if ((temp = getenv("RTPSEND_TTL")) != NULL)
		ttl = atoi(temp);
	if ((temp = getenv("RTPSEND_IF")) != NULL)
		ifname = temp;
#endif /* RTPSEND */

	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'C':
			OPT.clock = atoi(optarg);
			break;
		case 'R':
			OPT.freq = atoi(optarg);
			break;
		case 'L':
			OPT.plen = atoi(optarg);
			break;
		case 'N':
			OPT.rs_N = atoi(optarg);
			break;
		case 'y':
			OPT.payload_type = atoi(optarg);
			break;
		case 'd':
			OPT.dup2dummy=1;
			break;
#ifdef RTPSEND
		case 'A':	/* IP address */
			addr = optarg;
			break;
		case 'P':	/* port# */
			port = atoi(optarg);
			break;
		case 'a':
			src_addr = optarg;
			break;
		case 'p':	/* src_port# */
			src_port = atoi(optarg);
			break;
		case 'V':
			OPT.ipver = atoi(optarg);
			break;
		case 't':
			ttl = atoi(optarg);
			break;
		case 'I':
			ifname = optarg;
			break;
		case 's':
			OPT.shaping_lev=atoi(optarg);
			break;
		case 'l':
			lambdano = atoi(optarg);
			lambdano_enable = 1;
			break;
#endif /* RTPSEND */
		case 'D':
			debug_level = atoi(optarg);
			break;
#ifdef RTPSEND
		case 'e':
			OPT.send_err_rate = atoi(optarg);
			break;
#endif /* RTPSEND */
		case '0':
			OPT.compatible_mode = 1;
			break;
		case '1':
		case '2':
		case '3':
			OPT.send_double = ch - '0';
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
	}

#ifdef RTPSEND
	d_printf("\nrtpsend: pt=%d, IP=%s, port=%d",
		 OPT.payload_type, addr, port);
	if (src_addr != NULL){
		d_printf(", src-IP=%s", src_addr);
	}
	if (src_port > 0){
		d_printf(", src-port=%d", src_port);
	}
	OPT.sfd = sendsock_setup(addr, port, ifname, src_addr, src_port,
				 ttl, OPT.ipver);
	if (lambdano_enable) {
		struct wl_resv resv;
		struct wl_reserve_status status;
		socklen_t status_len;
		int ret;

		resv.wlr_flags = WL_RESV_FLAG_SEND;
		resv.wlr_lambda_no = lambdano;
		ret = setsockopt (OPT.sfd, IPPROTO_IP, WL_ADD_RESV, &resv,
				  sizeof (resv));
		if (ret == -1) {
			d_printf ("setsockopt WL_ADD_RESV error");
			exit (-1);
		}

		status_len = sizeof (status);
		ret = getsockopt (OPT.sfd, IPPROTO_IP, WL_RESERVE_STATUS,
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
#else
	d_printf("\nrtpenc: pt=%d", OPT.payload_type);
#endif /* RTPSEND */

	return 0;
}

int
next_mediadata_get(char *p_hd, char *payload)
{
	int             len;
	if ((len = pipe_blocked_read_block(0, p_hd, payload, OPT.plen)) == PIPE_ERROR)
		return -1;
	return len;
}

#ifdef RTPSEND

#if 0 
void 
wait4rtsend(struct timeval *blk_start,int blk_pktcnt){
	int64_t difflimit_ts,diff_ts;
	struct timeval  now;
	static int  cnt=0;

	if (blk_pktcnt==0){
	  cnt++;
	}
	difflimit_ts=(blk_pktcnt * OPT.freq/ OPT.rs_N)
		- 3/* msec */ * OPT.clock /1000;
	for(;;){
		gettimeofday(&now, NULL);
		diff_ts=timeval_sub_ts(&now,blk_start,OPT.clock);
		if(diff_ts >difflimit_ts){
			break;
		}else{
			d1_printf("wait4rtsend(%5d)%3d.\n",cnt,blk_pktcnt);
			usleep(1000);
		}
	}
}

#else
void 
wait4rtsend(struct timeval *wclk_tvstart,u_int64_t *rtsend_tslimit
	    ,u_int32_t blk_pktcnt,u_int32_t waitperframe){

	u_int64_t diff_ts,limit_ts;
	struct timeval  now;
	static int  cnt=0;

	if (blk_pktcnt==0){
	  cnt++;
	}
	d2_printf("rtsend_tslimit %llu\n",*rtsend_tslimit);

	limit_ts=(*rtsend_tslimit)+(OPT.freq*blk_pktcnt)/waitperframe;

	gettimeofday(&now, NULL);
	diff_ts=timeval_sub_ts(&now,wclk_tvstart,OPT.clock);
	d2_printf("limit_ts %llu diff_ts %llu \n",limit_ts,diff_ts);
	if (blk_pktcnt==0){
	  if(limit_ts<diff_ts // write too late
	     ||diff_ts+OPT.freq/waitperframe<limit_ts) 
	    {
			d2_printf("modify tv rtsend_tslimit=%llu\n"
				  ,*rtsend_tslimit);
			gettimeofday(wclk_tvstart, NULL);
			(*rtsend_tslimit)=0;
			return; 
		}
	}
	if(diff_ts <limit_ts){
	      d2_printf("wait4rtsend(%5d)%3d.\n",cnt,blk_pktcnt);
	      usleep((limit_ts-diff_ts)*1000/OPT.clock*1000);
	}
}
#endif 
#endif /* RTPSEND */

int exit_req=0;

void cb_sig(int signo){
	pipe_exit_req();
	e_printf("caught signal %d\n",signo);
	exit_req=1;
}

int
main(int argc, char *argv[])
{
#ifndef RTPESND
	char            p_header[PIPE_HEADER_LEN];
#endif RTPSEND
	char            r_header[RTP_HEADER_LEN];
	char           *buffer;
	struct iovec    iov[3];
	ssize_t         length;
	u_int16_t       seq;
	PIPE_CONTEXT    p;
	int             dummy_cnt; /* dummy(very old packet)cnt */
#ifdef RTPSEND
	int             count=0; /* for Debug */
	struct timeval  blk_start_tv;
	int             blk_wait4start=1;
	int             blk_pktcnt = 0;
	u_int32_t       waitperframe;
#endif
	sigset_t        sigset;
	ssize_t         io_ret;
/* 	sigfillset(&sigset); */
/* 	sigdelset(&sigset,SIGINT); */
/* 	sigdelset(&sigset,SIGTSTP); */
	if (sigprocmask(SIG_BLOCK,&sigset,NULL)!=0){
	  e_printf("sigprocmask fail.\n");
	  exit(0);
	}
	signal(SIGINT,cb_sig);
	signal(SIGTSTP,cb_sig);
	signal(SIGPIPE,cb_sig);

	if (opt_etc(argc, argv) == -1) {
		return -1;
	}
	if (isatty(STDIN)) {
		e_printf("Standard input must be binded with pipe.\n");
		return -1;
	}
#ifndef RTPSEND
	if (isatty(STDOUT)) {
		e_printf("Standard output must be binded with pipe.\n");
		return -1;
	}
#endif /* ! RTPSEND */
	if ((buffer = malloc(OPT.plen)) == NULL) {
		e_printf("cannot malloc for buffer\n");
		return -1;
	}
	if (OPT.compatible_mode) {
		seq = 0;
	} else {
		srand(time(NULL));
		seq = rand() & 0xffff;
	}
	d1_printf("first seq/rtp=%u\n",seq);

	rtp_reset(r_header, RTP_HEADER_LEN);
	rtp_set_version(r_header, 1);
	rtp_set_ptype(r_header, OPT.payload_type);

	memset(buffer, 0, OPT.plen);
#ifdef RTPSEND
	switch(OPT.shaping_lev)
	{
	case 0:
	default:
		OPT.shaping_lev=0;
		waitperframe=1;
		d1_printf("shaping_mode : frame\n");
		break;
	case 1:
		waitperframe=OPT.rs_N;
		d1_printf("shaping_mode : packet with dups\n");
		break;
	case 2:
		waitperframe=OPT.rs_N*OPT.send_double;
		d1_printf("shaping_mode : all packet(include dups)\n");
		break;
	}
	iov[0].iov_base = r_header;
	iov[0].iov_len = RTP_HEADER_LEN;
	iov[1].iov_base = buffer;
/*	iov[1].iov_len = OPT.plen;*/
#else /* RTPSEND */
	iov[0].iov_base = p_header;
	iov[0].iov_len = PIPE_HEADER_LEN;
	iov[1].iov_base = r_header;
	iov[1].iov_len = RTP_HEADER_LEN;
	iov[2].iov_base = buffer;
/*	iov[2].iov_len = OPT.plen;*/
#endif /* RTPSEND */
	p=pipe_context_init(STDIN,1 /*OPT.rs_N*/, OPT.plen);
	wclk_ps_tvstart.tv_sec=0;
	while ((length = pipe_blocked_read_packet_ex(p, p_header, buffer))>=0){
#ifdef RTPSEND
		if(wclk_ps_tvstart.tv_sec==0){
				/* recode start time */
				gettimeofday(&wclk_ps_tvstart, NULL);
		}
		count++;
		if(blk_wait4start){
			gettimeofday(&blk_start_tv,NULL);
			blk_wait4start=0;
			blk_pktcnt = 0;
		}else{
			blk_pktcnt++;
		}
		blk_wait4start=pipe_get_marker(p_header);
#endif
#ifdef RTPSEND
		iov[1].iov_len =length;
		pipe_set_length(p_header,length);
#else 
		iov[2].iov_len =length;
		pipe_set_length(p_header,length + RTP_HEADER_LEN);
#endif /* RTPSEND*/
		rtp_set_timestamp(r_header, pipe_get_timestamp(p_header));
		rtp_set_marker(r_header, pipe_get_marker(p_header));
		rtp_set_seqnum(r_header, seq++);
		if(seq>=0x10000) seq-=0x10000;
		d3_printf("rtpenc: marker %d : seq %d\n",pipe_get_marker(p_header),seq-1);
#ifdef RTPSEND
		if (display_time_all_packet) {
			d_printf("\n%d:", seq);
		}
		rtsend_tslimit+=(u_int32_t)(pipe_get_timestamp(p_header) - pkt_tslast);
		pkt_tslast=pipe_get_timestamp(p_header);
#if 0
		wait4rtsend(&wclk_tvstart,blk_pktcnt);
#else

		if(!(OPT.shaping_lev==0 && blk_pktcnt>=1)){
			wait4rtsend(&wclk_tvstart,&rtsend_tslimit,blk_pktcnt,waitperframe);
		}
#endif 
		if (OPT.send_err_rate > 0) {
			if ((rand() % OPT.send_err_rate) == 0) {
				total_drop++;
				continue;
			}
		}
		d2_printf(" write %d bytes", pipe_get_length(p_header));
		if (writev(OPT.sfd, iov, 2) == -1) {
		    total_drop++;
		    d2_printf(" writev-NG(ts/sq=%d/%d)\n", pipe_get_timestamp(p_header), count);
		} else {
		    d2_printf(" writev-OK(ts/sq=%d/%d)\n", pipe_get_timestamp(p_header), count);
		    total_send++;
		}

		if(OPT.dup2dummy){
			rtp_set_seqnum(r_header, (seq+0x10000-0x100)&0xffff);
		}
		for(dummy_cnt=1;dummy_cnt<OPT.send_double;dummy_cnt++){
			if(OPT.shaping_lev==2){
				blk_pktcnt++;
				wait4rtsend(&wclk_tvstart,&rtsend_tslimit,blk_pktcnt,waitperframe);
			}
			d2_printf(" write dup(%d/%d) \n",dummy_cnt,OPT.send_double);
			io_ret=writev(OPT.sfd, iov, 2);
			if ( io_ret == -1) {
			    total_drop++;
			    d2_printf(" writev-NG(ts/sq=%d/%d)\n", pipe_get_timestamp(p_header), count);
			} else {
			    d2_printf(" writev-OK(ts/sq=%d/%d)\n", pipe_get_timestamp(p_header), count);
			    total_send++;
			}
		}
#else /* RTPSEND */
		d2_printf(" write pkt\n");
		writev(STDOUT, iov, 3);
		if(OPT.dup2dummy){
			rtp_set_seqnum(r_header, (seq+0x10000-0x100)&0xffff);
		}
		for(dummy_cnt=1;dummy_cnt<OPT.send_double;dummy_cnt++){
		  d2_printf(" write dup(%d/%d) \n",dummy_cnt,OPT.send_double);
		  io_ret=writev(STDOUT, iov, 3);
		  if(io_ret==-1){
			  exit_req=1;
			  break;
		  }
		}
#endif /* RTPSEND */
		if(exit_req==1){
		  break;
		}
	}
#ifdef RTPSEND
	last_seq=seq-1;
	statistics_display();
	sleep(1);
#endif
	return 0;
}

#ifdef RTPSEND
static 
void statistics_display(){
  struct    timeval  now;
  u_int64_t msec;

  gettimeofday(&now,NULL);
  msec=timeval_sub_ts(&now,&wclk_ps_tvstart,1000);

  d1_printf(
	   
	   "===s<%s:%u> d<%s:%u>===\n"
	   "rtpsend: packet (send[%llu]+drop[%llu]=%llu)\n"
	   "         pps    (send[%f]+drop[%f]=%f)\n"
	   "         last_seq=%d\n"
	   ,src_addr,src_port,addr,port
	   ,total_send,total_drop,total_send+total_drop
  /*SIGFPE is masked*/
	   ,((double)total_send*1000)/msec,((double)total_drop*1000)/msec
	   ,((double)(total_send+total_drop)*1000)/msec
	   ,last_seq);
}
#endif
