/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/0?/??
 *
 * $Id: audiocapt.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif
#ifdef HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#endif
#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "aucodec.h"
#include "debug.h"
#include "pipe_hd.h"
#include "def_value.h"
#include "multicast.h"
//#include "timeval.h"
#include "time.h"
#include "string.h"


#define MAX(a,b) (a>b? a:b)
#define MIN(a,b) (a>b? b:a)
       

#define ENDIAN_LITTLE
/*0x0000ffff log(len)  [0=>E(0=>1<<E) es1371_OSS]*/
/*0xffff0000 # of frag (is ignored.)*/
#define DEFAULT_DSP_FRAGMENT_ARG  (0xfff0006) 
#define DEFAULT_DSP_FRAGMENT_SIZE (1<<6) 
#define DSP_SETFRAGMENT_BYARG	/* 1:ARG 2:SIZE */

#define DEFAULT_DSP_DEVICE      "/dev/dsp"
#define DEFAULT_DSP_FD          (-1)

#define DEFAULT_AUDIOCAPT_ENCODER_INIT (aucodec_l16_enc_init)
#define DEFAULT_AUDIOCAPT_RATE         (44100)
#define DEFAULT_AUDIOCAPT_CHANNEL      (2)

#define DSP_RATE_FIX(hz) (hz)
/*for ESS Maestoro OSS / Toshiba DynaBookSS3380V*/
//#define DSP_RATE_FIX(hz) ((hz)+(hz)/58)
/* for test */
//#define DSP_RATE_FIX(hz) ((hz)+(hz)/20)

/*from argv*/
typedef struct _audiocapt_arg_t{
	char              *dsp_dev;
	int                emon_clk; 
	int                emon_freq;
	int                marker_type; 
        int                plen;
	int                dsp_rate;
	int                dsp_ch;
        aucodec_enc_init_f encoder_init;

	int             fragment_size;
#if 0
	int             fragment_num;
#endif

	int                dsp_fd;  /*if not==-1,dsp opened */
	int                dsp_fmt;
	int                cclk_fd;
/* TODO;to  array or list */
	int                prior_fd;
}audiocapt_arg_t;


static audiocapt_arg_t     OPT;
static aucodec_enc_t      *encoder;

u_int32_t audiocapt_parsearg(int argc, char *argv[],audiocapt_arg_t *opt);

int             open_dsp(void);

static void audiocapt_db_printibuf(int fd);
static ssize_t audiocapt_readspl(int prior_fd,int dsp_fd
				 ,void *buf, size_t nbytes);
static time_clkofemon_t clkofemon;
static u_int32_t        dsp_byterate;


/* 
   NORMAL
      +-(timer)-- clkofemon => ts 
      |           +---(dsp)--> last_ts + dsp_buf
      |           |           +--(timer)-->clkofemon =>ts + delay_limit;
      |           |           |
      V           V           V                
   ===================================>TS

   NEED more samples,dup some of them.
      +---(dsp)--> last_ts + dsp_buf =A
      |           +-(timer)-- clkofemon => ts =B
      |           |           
      |           |           
      V           V                           
   ===================================>TS
   dup [(B-A)==ts/emon_freq +1] times.

   TOO many samples ,drop some of them.
      +--(timer)-->clkofemon =>ts + delay_limit;
      |           +---(dsp)--> last_ts + dsp_buf B
      |           |           
      |           |           
      V           V                           
   ===================================>TS
   skip [(B-A)==ts/emon_freq +1] times.

 */
int main(int argc, char **argv)
{
	u_int8_t        p_header[PIPE_HEADER_LEN];

	void           *pcmbuf;   /* raw audio buf */ 
	void           *codebuf; /* encoded audio buf */
	int             afd;	  /* audio fd */

        /* modify ts */
	time_tv_t       timeofemon_tv;
	u_int32_t       pipe_ts,dsp_buf_ts,timeofemon_ts;
	int32_t         diff_ts; /* pipe_ts - timeofemon_ts  */
	int32_t         delay_limit_ts;	/* will cmp diff_ts */
	int32_t         delay_limit_dsp;/* will cmp dsp_buflen */

        /* pipe writing mode */
	u_int32_t       adjust_mode;  /* 0:skip 1:normal 2:dup */
	u_int32_t       adjust_fire =100;
	u_int32_t       adjust_step =50;
	u_int32_t       adjust_stage;
	
	/* for stat */
	time_tv_t       wclk_start_tv;
	ssize_t         io_ret;        /* return val for read write seek ..*/
 	int             dsp_ossbuflen; /* if >0,dsp drv is not OSS */
	int             pps; /* emon packet per second */

	if (audiocapt_parsearg(argc, argv,&OPT) > 0) {
		return -1;
	}
	  debug_printf1(
		   "emon clock %d freq %d plen %d\n"
		   "dsp fmt %d rate %d channel %d %d\n"
		   ,OPT.emon_clk, OPT.emon_freq,OPT.plen
		   ,OPT.dsp_fmt, OPT.dsp_rate, OPT.dsp_ch
		   ,(OPT.dsp_rate*OPT.emon_freq) / OPT.emon_clk);
	  if (isatty(STDOUT_FILENO)) {
                e_printf("Standard output must be binded with pipe.\n");
                return -1;
        }
        if (((OPT.dsp_rate*OPT.emon_freq) % OPT.emon_clk) != 0){
	  e_printf("Samples per FEC block"
		   " (=sampling_rate*(emon_freq/emon_clock))"
		   " must be integer.\n");
                return -1;
        }
	if ((afd = open_dsp()) < 0) {
	  debug_printf1(
		   "emon clock %d freq %d plen %d\n"
		   "dsp fmt %d rate %d channel %d\n"
		   ,OPT.emon_clk, OPT.emon_freq,OPT.plen
		   ,OPT.dsp_fmt, OPT.dsp_rate, OPT.dsp_ch);
	  return -1;
	}
	afd=OPT.dsp_fd;
	pcmbuf = malloc(encoder->pcmblk_size);
	codebuf= malloc(encoder->codeblk_maxsize);
	if (pcmbuf ==NULL || codebuf == NULL) {
		e_printf("fail to  malloc %d bytes\n"
			 ,encoder->pcmblk_size+encoder->codeblk_maxsize);
		return -1;
	}
	audiocapt_db_printibuf(afd);
	debug_printf1("pcm %d code %d\n"
			 ,encoder->pcmblk_size,encoder->codeblk_maxsize);

	bzero(p_header, PIPE_HEADER_LEN);
	pipe_set_version(p_header, 1);
	pipe_set_marker(p_header, 1);

	pps= OPT.emon_clk/OPT.emon_freq;
	dsp_byterate=OPT.dsp_rate*OPT.dsp_ch*2;
	{
		audio_buf_info aubuf_info;
		/* if read dsp too late , drop samples. */
		ioctl(afd, SNDCTL_DSP_GETISPACE, &aubuf_info);
		/* todo : should be based on OPT.delay_limit */
		dsp_ossbuflen=aubuf_info.bytes;
		debug_printf1("audiocapt startup.auinfo.bytes=%d\n",dsp_ossbuflen);
	}
	/* setup ts and wclk */
	time_resetclkofemon(&clkofemon);
	debug_printf1("wait4cclk update \n")
	while(time_updateclkofemon(OPT.cclk_fd,&clkofemon)!=1);
	debug_printf1("cclk updated \n");

		
	wclk_start_tv=time_gettimeofday();
	timeofemon_tv=time_gettimeofemon(clkofemon);
	timeofemon_ts=time_tvto32(timeofemon_tv,OPT.emon_clk);
	pipe_ts=timeofemon_ts;

	delay_limit_ts =OPT.emon_freq;
	delay_limit_dsp=time_32to32(delay_limit_ts,OPT.emon_freq,dsp_byterate);
	delay_limit_dsp-=delay_limit_dsp%(OPT.dsp_ch*2);
	debug_printf1("loop start\n");
	for (;;) {
		audio_buf_info aubuf_info;
		int        len;
		int        diff;
		int        cnt;

	  /* read number of samples in dspbuf */
	  ioctl(afd, SNDCTL_DSP_GETISPACE, &aubuf_info);
	  if(dsp_ossbuflen==0){
		  len=aubuf_info.bytes;
	  }else{
		  len=dsp_ossbuflen - aubuf_info.bytes;
	  }
	  audiocapt_db_printibuf(afd);
	  debug_printf3("dsp_buflen %d\n",len);

	  /* if there is too much sample in dspbuf ,drop old sample.
	     TODO:this function should be done by set 
	     proper fragment size.
	  */
	  diff=len-delay_limit_dsp;
	  if(diff>0){
		  debug_printf2("drop %u samples (dspdelay)\n",len);
		  /* 
		     if the first read(dsp) is not called,
		     fd_lseek_ignoreEOF make dead lock.
		  */
		  io_ret = fd_lseek_ignoreEOF(OPT.dsp_fd,diff);
		  len=delay_limit_dsp;
	  }
	  /* 
	      KETTEISURU 
	      whether drop samples or dup samples or normal processing
	      for synchronize rtc and dsp clock
	  */
	  dsp_buf_ts=time_32to32(len,dsp_byterate,OPT.emon_clk);
	  timeofemon_tv=time_gettimeofemon(clkofemon);
	  timeofemon_ts=time_tvto32(timeofemon_tv,OPT.emon_clk);
	  diff_ts=(pipe_ts+dsp_buf_ts)-timeofemon_ts;
	  adjust_mode=1;	/* normal */
	  adjust_stage=0;
	  if(delay_limit_ts < diff_ts){
		  /* too many sample in dspbuf,remove some of them */
		  adjust_mode=0; /* drop */
		  debug_printf2("drop samples, diff_ts=%d\n",diff_ts);
		  debug_printf2("drop samples, delay_limit_ts=%d\n",delay_limit_ts);
	  }
	  if(diff_ts<0){
		  /* need more sample in dspbuf,dup some of them */
		  adjust_mode=2; /* dup */
		  debug_printf2("dup samples, diff_ts=%d\n",diff_ts);
	  }
	  /* loop for 1 minute */
	  debug_printf3("loop 1 minute\n");
	  for(cnt=0;cnt<pps;cnt++){
		u_int32_t codeblk_size;

		time_updateclkofemon(OPT.cclk_fd,&clkofemon);
#if 1
		io_ret = audiocapt_readspl(OPT.prior_fd,OPT.dsp_fd
					   ,pcmbuf,encoder->pcmblk_size);
#else
		io_ret = read(OPT.dsp_fd,pcmbuf,encoder->pcmblk_size);
#endif
		time_updateclkofemon(OPT.cclk_fd,&clkofemon);
#if 0
		audiocapt_db_printibuf(afd);
#endif
		if (io_ret < 0) {
			e_printf("fail to read sample.\n");
			continue;
		}
		if (io_ret != encoder->pcmblk_size) {
			e_printf("fail to read samples %d.\n",io_ret);
			continue;
		}
		if(adjust_mode==0){ /* drop mode */
			adjust_stage+=adjust_step;
			if(adjust_stage>=adjust_fire){
				/* drop */
				debug_printf3("drop 1\n");
				diff_ts-=OPT.emon_freq;
				if(diff_ts<=delay_limit_ts){
					adjust_mode=1;
				}else{
					adjust_stage-=adjust_fire;
				}
//BUG:===>	                pipe_ts+=OPT.emon_freq;
				continue;
			}
		}
		encoder->enc(encoder,pcmbuf,codebuf,&codeblk_size);
		pipe_set_length(p_header, codeblk_size);
		while(1){
			pipe_set_timestamp(p_header,(u_int32_t)pipe_ts);
			pipe_blocked_write_block(STDOUT_FILENO
						 ,p_header,codebuf);
			pipe_ts += OPT.emon_freq;
			if(adjust_mode!=2){ /* !(dup mode) */
				break;
			}
			/* dup mode */
			adjust_stage+=adjust_step;
			if(adjust_stage<adjust_fire){
				break;
			}
			/* dup */
			debug_printf3("dup 1\n");
			diff_ts+=OPT.emon_freq;
			if(diff_ts>0){
				adjust_mode=1;
			}else{
				adjust_stage-=adjust_fire;
			}
		}
	  }
	  if(OPT.marker_type==0){ /* RFC1890 */
		  pipe_set_marker(p_header, 0);
	  }
	}
	d_printf("\n");

	return 0;
}





int
open_dsp(void)
{
	int             afd;
	int             val;
	u_int32_t       ret;
	aucodec_dspfmt_t dspfmt_avail,dspfmt;

	/*audiocaptplay*/
	if (OPT.dsp_fd!=-1){
		dspfmt_avail=aucodec_dspfmt_oss2dspfmt(OPT.dsp_fmt);
		ret=OPT.encoder_init(dspfmt_avail,
				 OPT.dsp_ch,
				 (OPT.dsp_rate*OPT.emon_freq)/OPT.emon_clk,
				 &dspfmt,
				 &encoder);
		if(ret!=0){
			e_printf("fail to initialize encoder.\n");
			return -1;
		}
		OPT.dsp_fmt=aucodec_dspfmt_dspfmt2oss(dspfmt);
		return OPT.dsp_fd;
	}
	
	/* open audio device */
	if ((afd = open(OPT.dsp_dev, O_RDONLY, 0)) < 0) {
		e_printf("cannot open audio device\n");
		return -1;
	}
	OPT.dsp_fd = afd;

#ifdef DSP_SETFRAGMENT_BYARG
	val = DEFAULT_DSP_FRAGMENT_ARG;
	if (ioctl(afd, SNDCTL_DSP_SETFRAGMENT, &val) < 0) {
		e_printf("SNDCTL_DSP_SETFRAGMENT: cannot set audio device \n");
		return -1;
	}
#endif
	/* get supported format. */
	val = AFMT_QUERY;
	if (ioctl(afd, SNDCTL_DSP_GETFMTS, &val) < 0) {
		e_printf("SNDCTL_DSP_GETFMTS: cannot get format mask.\n");
		return -1;
	}
	debug_printf2("SNDCTL_DSP_GETFMTS ret 0x%x\n", val);

	dspfmt_avail=aucodec_dspfmt_oss2dspfmt(val);
	ret=OPT.encoder_init(dspfmt_avail,
			 OPT.dsp_ch,
			 (OPT.dsp_rate*OPT.emon_freq)/OPT.emon_clk,
			 &dspfmt,
			 &encoder);

	if(ret!=0){
		e_printf("fail to initialize encoder.\n");
		return -1;
	}

	/* set audio device */
	val = OPT.dsp_ch;
	/* old FreeBSD don't support SNDCTL_DSP_CHANNELS  */
	if (ioctl(afd, SOUND_PCM_WRITE_CHANNELS, &val) < 0) {
		printf("SNDCTL_DSP_CHANNELS: cannot set channel\n");
		return -1;
	}

	OPT.dsp_fmt=aucodec_dspfmt_dspfmt2oss(dspfmt);
	val=OPT.dsp_fmt;
	if (ioctl(afd, SNDCTL_DSP_SETFMT, &val) < 0) {
		e_printf("SNDCTL_DSP_SETFMT: cannot set format. \n");
		return -1;
	}

	val = DSP_RATE_FIX(OPT.dsp_rate);
	if (ioctl(afd, SNDCTL_DSP_SPEED, &val) < 0) {
		e_printf("SNDCTL_DSP_SPEED: cannot set dsp speed.\n");
		return -1;
	}
	if (val !=DSP_RATE_FIX(OPT.dsp_rate)) {
		e_printf("cannot set SNDCTL_DSP_SPEED to %d (%d)\n", OPT.dsp_rate,val);
		return -1;
	}
#ifndef DSP_SETFRAGMENT_BYARG
	val = OPT.fragment_size;/* fragment size  (is not for DMA ...) */
	if (ioctl(afd, SNDCTL_DSP_SETBLKSIZE, &val) < 0) {
		e_printf("SNDCTL_DSP_SETBLKSIZE: cannot set audio device \n");
		return -1;
	}
#endif
	return afd;
}

static ssize_t audiocapt_readspl(int prior_fd,int dsp_fd,
				 void *buf, size_t nbytes){
	ssize_t io_ret;
	ssize_t ret;
	size_t  rest;
	int8_t  *s;

	if(nbytes==0){
		return 0;
	}

	ret=0;
	s=buf;
	rest=nbytes;

loop_depthA_start:
	if(prior_fd>0 && fd_can_read(prior_fd,0)){
		io_ret=read(prior_fd,s,rest);
		if(io_ret==0){
			/* 
			   EOF on the condition of no open process for write.
			   Switch to DSP.
			*/
			goto dsp;
		}
		if(io_ret<0){ /* IO_ERRO */
			return -1;
		}
		/* for synchronize prior_fd and dsp */
		fd_lseek_ignoreEOF(dsp_fd,io_ret);
	}else{
	dsp:
		io_ret=read(dsp_fd,s,rest);
		if(io_ret==0){
			e_printf("Why EOF? (fd=%d)\n",prior_fd);
		}
		if(io_ret<0){ /* IO_ERRO */
			return -1;
		}
	}
	s+=io_ret;
	rest-=io_ret;
	if(rest==0){
		return nbytes;
	}
	goto loop_depthA_start;
}

u_int32_t audiocapt_parsearg(int argc, char *argv[],audiocapt_arg_t *opt){
	char            c;
	char           *tmpstr;	/* for getenv */
	char            *cclk_dev;

	char           *opts = "d:C:R:T:L:f:r:c:m:p:D:X:Y:h"; /*X,Y for captplay*/
	char           *helpmes =
	"usage : %s [options ...]\n"
	" options\n"
	"  -C <num> : EMON clock[44100]\n"
	"  -R <num> : EMON frequency[1470]\n"
	"  -f <str> : audio format {L8,L16,PCMU,DVI4}[L16]\n"
	"  -c <num> : # of chanel[2]\n"
	"  -r <num> : sampling rate[44100]\n"
	"  -m <num> : marker type {0:rfc1890,1:frame}[1]\n"
	"  -d <path>: dsp path[/dev/dsp]\n"
	"  -p <path>: source prior to the dsp which passed by \"-d\"[NULL]\n"
	" untested option\n"
	"  -T <path>: common timer path[NULL]\n"
	"  -L <num> : emon payload length limit[1280]\n"
	" debug option\n"
	"  -D <num> : debug message level[0]\n"
	" only for audiocaptplay\n"
	"  -X <num> : opened dsp fd[NULL]\n"
	"  -Y <num> : opened dsp fmt[NULL]\n"
		;
	opt->dsp_dev   = DEFAULT_DSP_DEVICE;
	opt->dsp_fd    = DEFAULT_DSP_FD;


	cclk_dev       = NULL; 
	opt->cclk_fd   = -1;

	opt->emon_clk  = DEF_EMON_CLOCK;
	opt->emon_freq = DEF_EMON_FREQ;
	opt->plen      = DEF_EMON_PLEN;

	opt->dsp_rate  = DEFAULT_AUDIOCAPT_RATE;
	opt->dsp_ch    = DEFAULT_AUDIOCAPT_CHANNEL;
	opt->encoder_init   = DEFAULT_AUDIOCAPT_ENCODER_INIT;

        opt->fragment_size=DEFAULT_DSP_FRAGMENT_SIZE;
	opt->marker_type = 1;
	opt->prior_fd = -1;

	debug_level=0;	
	if ((tmpstr = getenv("EMON_CLOCK")) != NULL)
		opt->emon_clk = atoi(tmpstr);
	if ((tmpstr = getenv("EMON_FREQ")) != NULL)
		opt->emon_freq = atoi(tmpstr);
	if ((tmpstr = getenv("EMON_PLEN")) != NULL)
		opt->plen = atoi(tmpstr);

	if ((tmpstr = getenv("AUDIOCAPT_RATE")) != NULL)
		opt->dsp_rate = atoi(tmpstr);
	if ((tmpstr = getenv("AUDIOCAPT_CHANNEL")) != NULL)
		opt->dsp_ch = atoi(tmpstr);
	if ((tmpstr = getenv("AUDIOCAPT_FORMAT")) != NULL) {
		opt->encoder_init=aucodec_enc_str2init(tmpstr);
		if(opt->encoder_init==NULL){
			e_printf("AUDIOCAPT_FORMAT=%s is not supported.\n"
			       , tmpstr);
			return 2;
		}
	}

	while ((c = getopt(argc, argv, opts)) != -1) {
		switch (c) {
		case 'd':
			opt->dsp_dev=optarg;
			break;
		case 'C':
			opt->emon_clk = atoi(optarg);
			break;
		case 'R':
			opt->emon_freq = atoi(optarg);
			break;
		case 'L':
			opt->plen = atoi(optarg);
			break;
		case 'T':
			cclk_dev=optarg;
			break;
		case 'm':
			opt->marker_type=atoi(optarg);
			break;
		case 'f':
			opt->encoder_init=aucodec_enc_str2init(optarg);
			if(opt->encoder_init==NULL){
				printf("AUDIOCAPT_FORMAT %s is not supported.\n"
				       , tmpstr);
				return 2;
			}
			break;
		case 'r':
			opt->dsp_rate = atoi(optarg);
			break;
		case 'c':
			opt->dsp_ch = atoi(optarg);
			break;
		case 'p':
			opt->prior_fd = open(optarg, O_RDONLY, 0);
			if (opt->prior_fd < 0){
				e_printf("audiocapt:cannot open %s\n",optarg);
				return 3;
			}
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		case 'X':
			opt->dsp_fd=atoi(optarg);
			break;
		case 'Y':
			opt->dsp_fmt=atoi(optarg);
			break;
		case '?':
		default:
		        e_printf(helpmes, argv[0]);
			return 1;
		}
	}
	if(cclk_dev!=NULL){
		opt->cclk_fd = open(cclk_dev,O_RDONLY,0);
	}else{
		opt->cclk_fd = -1;
	}
	return 0;
}



void audiocapt_db_printibuf(int fd){
	audio_buf_info aubuf_info;

	ioctl(fd, SNDCTL_DSP_GETISPACE, &aubuf_info);
	debug_printf3("ibuf fragments %d fragstotal %d" 
		  "fragsize %d bytes %d\n",
		  aubuf_info.fragments,
		  aubuf_info.fragstotal,
		  aubuf_info.fragsize,
		  aubuf_info.bytes);
}
