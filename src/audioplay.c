/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/0?/??
 *
 * $Id: audioplay.c,v 1.1 2008/09/26 15:10:34 emon Exp $
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
#include "timeval.h"

#define ENDIAN_LITTLE

/*0x0000ffff log(len)  [0=>E(0=>1<<E) es1371_OSS]*/
/*0xffff0000 # of frag (is ignored.)*/
#define DEFAULT_DSP_FRAGMENT_ARG  (0xfff0006) 
#define DEFAULT_DSP_FRAGMENT_SIZE (1<<6) 
#define DSP_SETFRAGMENT_BYARG	/* 1:ARG 2:SIZE */

#define DEFAULT_DSP_DEVICE      "/dev/dsp"
#define DEFAULT_DSP_FD          (-1)

#define DEFAULT_AUDIOPLAY_DECODER_INIT (aucodec_l16_dec_init)
#define DEFAULT_AUDIOPLAY_RATE         (44100)
#define DEFAULT_AUDIOPLAY_CHANNEL      (2)

#define DEFAULT_EMON_DELAY      (OPT.emon_freq+1)
#define DEFAULT_EMONAUBUF_NUM   (3)

#define DSP_RATE_FIX(hz) (hz)
/*for ESS Maestoro OSS / Toshiba DynaBookSS3380V*/
//#define DSP_RATE_FIX(hz) ((hz)+(hz)/58)
/* for test */
//#define DSP_RATE_FIX(hz) ((hz)+(hz)/10)
	
typedef struct _audioplay_arg_t {
	char              *dsp_dev;
	int                emon_clk; 
	int                emon_freq;
	int                marker_type;
	int                dspbuf_msgnum;

        int                plen;
	int                dsp_rate;
	int                dsp_ch;
        aucodec_dec_init_f decoder_init;
        int32_t          constdelay_ts;

	int                dsp_fd;  /*if not==-1,dsp opened */
	int                dsp_fmt;
} audioplay_arg_t;

static audioplay_arg_t     OPT;
static aucodec_dec_t      *decoder;

u_int32_t audioplay_parsearg(int argc, char *argv[],audioplay_arg_t *opt);
int             open_dsp(void);

static void audioplay_db_printobuf(int fd);

#define MIN(a,b) ((a)<(b)? (a):(b))
#define MAX(a,b) ((a)>(b)? (a):(b))

int
main(int argc, char **argv)
{
	unsigned int    timestamp;
	u_int8_t        p_header[PIPE_HEADER_LEN];

	void           *pcmbuf;
	void           *codebuf;
	int             afd;	/* audio fd */
	u_int64_t       dclk_tsnow;      /* dsp clock */
	u_int32_t       wclk_tsstart;
	struct timeval  wclk_ps_tvstart; /* the timeval before start capture */
	struct timeval  wclk_tvstart, wclk_tvnow;
	ssize_t         io_ret; /* return value for read write seek ..*/ 

	u_int64_t       frame_cnt;
	u_int32_t       dsp_ossbuflen;
	u_int32_t       dup_cnt;
	u_int32_t       dup_max=5; /*  */

	if (audioplay_parsearg(argc, argv,&OPT) > 0) {
		return -1;
	}
	d1_printf(
		"emon clock %d freq %d plen %d\n"
		"dsp rate %d dsp channel %d\n"
		,OPT.emon_clk, OPT.emon_freq,OPT.plen
		,OPT.dsp_rate, OPT.dsp_ch);
        if (((OPT.dsp_rate*OPT.emon_freq) % OPT.emon_clk) != 0){
	  e_printf("Samples per FEC block"
		   " (=sampling_rate*(emon_freq/emon_clock))"
		   " must be integer.\n");
                return -1;
        }
	if ((afd = open_dsp()) < 0) {
		return -1;
	}
	d1_printf(
		"fmt=%s dspfmt 0x%x dspbuf %u decbuf %u\n"
		,aucodec_dec_init2str(OPT.decoder_init),OPT.dsp_fmt
		,decoder->pcmblk_size,decoder->codeblk_maxsize);


	afd=OPT.dsp_fd;
	pcmbuf = malloc(decoder->pcmblk_size);
	codebuf= malloc(decoder->codeblk_maxsize);
	if (pcmbuf ==NULL || codebuf == NULL) {
		e_printf("fail to  malloc %d bytes\n"
			 ,decoder->pcmblk_size+decoder->codeblk_maxsize);
		return -1;
	}
	{
		audio_buf_info aubuf_info;
		/* if read dsp too late , drop samples. */
		ioctl(afd, SNDCTL_DSP_GETOSPACE, &aubuf_info);
		/* todo : should be based on OPT.delay_limit */
		dsp_ossbuflen=aubuf_info.bytes;
		d1_printf("audioplay startup.auinfo.bytes=%d\n",dsp_ossbuflen);
	}
	{
		int trig;
		trig=PCM_ENABLE_OUTPUT;
		ioctl(afd, SNDCTL_DSP_SETTRIGGER, &trig);
		debug_printf2("DSP_SETTRIGER ret=0x%x\n",trig);
	}
#if 0
	{
		int kk;
		struct _snd_capabilities cap;
		bzero(&cap,sizeof(cap));
		ioctl(afd, AIOGCAP, &cap);		
		debug_printf2(
			"SNDCAP rate%lu->%lu \n"
			"DMA buf%lu\n",
			cap.rate_min,cap.rate_max,
			cap.bufsize
			);

	}
#endif

	gettimeofday(&wclk_ps_tvstart, NULL);
#if 0
	audioplay_db_printobuf(afd);
	sleep(1);
#endif
	if(OPT.constdelay_ts<0){
	d1_printf("audioplay start loop.\n");
	dup_cnt=dup_max;
	for (;;) {
		int32_t   read_len;
		int32_t   write_len;
		ssize_t   io_ret;
		int       update=1; 
		d3_printf("audioplay pipe_blocked_read_message.\n");
		if(OPT.marker_type==0){	
			/* RFC1890 */
			read_len = pipe_blocked_read_block(STDIN_FILENO, p_header
							   ,codebuf
							   ,decoder->codeblk_maxsize);
		}else{
			read_len = pipe_blocked_read_message(STDIN_FILENO, p_header
							     ,codebuf
							     ,decoder->codeblk_maxsize);
		}
		if (read_len < 0) {
			e_printf("audioplay:fail to read sample.\n");
			break;
		}
		if (read_len == 0) {
			d1_printf("packet loss. ts=%d\n",
				 pipe_get_timestamp(p_header));
			if(dup_cnt>=dup_max){
				continue;
			}
			dup_cnt++;
		}else{
			decoder->dec(decoder,codebuf,read_len,pcmbuf);
			dup_cnt=0;
		}
		{
			audio_buf_info aubuf_info;
			int dsp_buf_ts;
			int dsp_buf_len;
			ioctl(afd, SNDCTL_DSP_GETOSPACE, &aubuf_info);
			if(dsp_ossbuflen!=0){
				/* OSS */
				dsp_buf_len=dsp_ossbuflen-aubuf_info.bytes;
			}else{
				/* faked OSS */
				dsp_buf_len=aubuf_info.bytes;
			}
			debug_printf3("dsp_buf_len=%d\n",dsp_buf_len);
			write_len=MIN(decoder->pcmblk_size*OPT.dspbuf_msgnum-dsp_buf_len
				      ,decoder->pcmblk_size);
			debug_printf3("0write_len=%d\n",write_len);
			write_len=MAX((int)decoder->pcmblk_size*(2)-dsp_buf_len
				      ,write_len);
			debug_printf3("1write_len=%d\n",write_len);
			if(write_len <0){
				continue;
			}
			if(write_len<decoder->pcmblk_size){
				debug_printf2("drop %dbyte\n",
					      decoder->pcmblk_size-write_len);
			}
		}
		while(write_len>0){
			int len;
			len=MIN(decoder->pcmblk_size,write_len);
			audioplay_db_printobuf(afd);
			io_ret = write(afd, pcmbuf,len);
			if(len!=io_ret){
				e_printf("dsp obuf full?? %d=>%d\n"
					 ,decoder->pcmblk_size,write_len);
				audioplay_db_printobuf(afd);
			}
			write_len-=len;
		}
		//	ioctl(afd, SNDCTL_DSP_POST, NULL);
		
	}
	d_printf("\n");
	return 0;
	}
	frame_cnt=0;
	while(1){
	  u_int64_t  wclk_diff,dclk_diff;
	  u_int32_t  wclk_tsnow; 
	  int32_t  ts_diff; 
	  
	  audio_buf_info aubuf_info;
	  int        len;
	  int        cnt;

		io_ret = pipe_blocked_read_message(STDIN_FILENO, p_header
						   ,codebuf
						   ,decoder->codeblk_maxsize);
		if (io_ret < 0) {
			e_printf("fail to read sample.\n");
			continue;
		}
		if (io_ret == 0) {
			e_printf("packet loss. ts=%d\n",
				 pipe_get_timestamp(p_header));
			continue;
		}
		ioctl(afd, SNDCTL_DSP_GETOSPACE, &aubuf_info);
		gettimeofday(&wclk_tvnow, NULL);
		wclk_tsnow=timeval_2ts(&wclk_tvnow,OPT.emon_clk);
		wclk_tsnow+=(dsp_ossbuflen-aubuf_info.bytes)*OPT.emon_freq
			     /decoder->pcmblk_size;
		
		ts_diff=pipe_get_timestamp(p_header)+OPT.constdelay_ts
		  - wclk_tsnow ;

		if((int32_t)ts_diff>4*OPT.emon_freq){
				/* need more sample */
			len=2;
			d1_printf(" dup  fr %6llu diff%d\n",frame_cnt
				  ,ts_diff);
		}else if((int32_t)ts_diff<-4*OPT.emon_freq){
				/* too much sample */
			len=0;
			d1_printf(" skip fr %6llu diff%d\n",frame_cnt
				  ,ts_diff);
		}else{
			len=1;
		}
		decoder->dec(decoder,codebuf,io_ret,pcmbuf);
		for(cnt=0;cnt<len;cnt++){
			write(afd, pcmbuf,decoder->pcmblk_size);
		}
		frame_cnt++;
	}
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
		ret=OPT.decoder_init(dspfmt_avail,
				 OPT.dsp_ch,
				 (OPT.dsp_rate*OPT.emon_freq)/OPT.emon_clk,
				 &dspfmt,
				 &decoder);
		if(ret!=0){
			e_printf("fail to initialize decoder.\n");
			return -1;
		}
		OPT.dsp_fmt=aucodec_dspfmt_dspfmt2oss(dspfmt);
		return OPT.dsp_fd;
	}
	
	/* open audio device */
	if ((afd = open(OPT.dsp_dev, O_WRONLY, 0)) < 0) {
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
	d1_printf("SNDCTL_DSP_GETFMTS ret 0x%x\n", val);

	dspfmt_avail=aucodec_dspfmt_oss2dspfmt(val);
	ret=OPT.decoder_init(dspfmt_avail,
			 OPT.dsp_ch,
			 (OPT.dsp_rate*OPT.emon_freq)/OPT.emon_clk,
			 &dspfmt,
			 &decoder);
	if(ret!=0){
		e_printf("fail to initialize decoder.\n");
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
	d1_printf("SNDCTL_DSP_SETFMT 0x%x\n", val);
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
		printf("cannot set SNDCTL_DSP_SPEED to %d\n", OPT.dsp_rate);
		return -1;
	}
#ifndef DSP_SETFRAGMENT_BYARG
	val = DEFAULT_DSP_FRAGMENT_SIZE;/* fragment size  (is not for DMA ...) */
	if (ioctl(afd, SNDCTL_DSP_SETBLKSIZE, &val) < 0) {
		e_printf("SNDCTL_DSP_SETBLKSIZE: cannot set audio device \n");
		return -1;
	}
#endif

	return afd;
}


u_int32_t audioplay_parsearg(int argc, char *argv[],audioplay_arg_t *opt){
        int32_t         constdelay;
	int32_t         stdin2dac_delay;
	char            c;
	char           *tmpstr;
	char           *opts = "d:C:R:L:W:f:r:c:m:t:D:X:Y:h"; /*X,Y for recplay*/
	char           *helpmes =
	"usage : %s [options ...]\n"
	" options\n"
	"  -W <num> : maximum delay from stdin to dac (msec)[60]\n"
	"  -C <num> : EMON clock[44100]\n"
	"  -R <num> : EMON frequency[1470]\n"
	"  -f <str> : audio format {L8,L16,PCMU,DVI4}[L16]\n"
	"  -c <num> : # of chanel[2]\n"
	"  -r <num> : sampling rate[44100]\n"
	"  -m <num> : marker type {0:rfc1890,1:frame}[0]\n"
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
	
	opt->emon_clk  = DEF_EMON_CLOCK;
	opt->emon_freq = DEF_EMON_FREQ;
	opt->plen      = DEF_EMON_PLEN;

	opt->dsp_rate  = DEFAULT_AUDIOPLAY_RATE;
	opt->dsp_ch    = DEFAULT_AUDIOPLAY_CHANNEL;
	opt->decoder_init   = DEFAULT_AUDIOPLAY_DECODER_INIT;

	opt->dspbuf_msgnum=4;
	opt->marker_type = 0;
	constdelay=-1;
	stdin2dac_delay=50;

	if ((tmpstr = getenv("EMON_CLOCK")) != NULL)
		opt->emon_clk = atoi(tmpstr);
	if ((tmpstr = getenv("EMON_FREQ")) != NULL)
		opt->emon_freq = atoi(tmpstr);
	if ((tmpstr = getenv("EMON_PLEN")) != NULL)
		opt->plen = atoi(tmpstr);

	if ((tmpstr = getenv("AUDIOPLAY_RATE")) != NULL)
		opt->dsp_rate = atoi(tmpstr);
	if ((tmpstr = getenv("AUDIOPLAY_CHANNEL")) != NULL)
		opt->dsp_ch = atoi(tmpstr);
	if ((tmpstr = getenv("AUDIOPLAY_FORMAT")) != NULL) {
		opt->decoder_init=aucodec_dec_str2init(tmpstr);
		if(opt->decoder_init==NULL){
			e_printf("AUDIOPLAY_FORMAT=%s is not supported.\n"
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
		case 'W':
			stdin2dac_delay= atoi(optarg);
			break;
		case 'm':
			opt->marker_type = atoi(optarg);
			break;
		case 'f':
			opt->decoder_init=aucodec_dec_str2init(optarg);
			if(opt->decoder_init==NULL){
				printf("AUDIOPLAY_FORMAT %s is not supported.\n"
				       , optarg);
				return 2;
			}
			break;
		case 'r':
			opt->dsp_rate = atoi(optarg);
			break;
		case 'c':
			opt->dsp_ch = atoi(optarg);
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
		case 't':
			constdelay=atoi(optarg);
			break;
		case '?':
		default:
			e_printf(helpmes, argv[0]);
			return 1;
		}
	}
	if(constdelay<0){
	  opt->constdelay_ts=-1;
	}else{
		d1_printf("constant delay=%dmsec\n",constdelay);
		opt->constdelay_ts=constdelay*opt->emon_clk/1000;
	}
				
	stdin2dac_delay*=opt->emon_clk;
	stdin2dac_delay/=1000; /* ms => emon sec */
	                       /* emon sec => FREQ (Hz/(emon_freq time))*/
	stdin2dac_delay/=opt->emon_freq;
	opt->dspbuf_msgnum=MAX(2,stdin2dac_delay);

	debug_printf1("bufnum %d\n",opt->dspbuf_msgnum);
	return 0;
}


void audioplay_db_printobuf(int fd){
	audio_buf_info aubuf_info;

	ioctl(fd, SNDCTL_DSP_GETOSPACE, &aubuf_info);
	if(aubuf_info.bytes ==0 ){
		debug_printf2("buffer empty\n");

	}
	d3_printf("obuf fragments %d fragstotal %d" 
		 "fragsize %d bytes %d\n",
		 aubuf_info.fragments,
		 aubuf_info.fragstotal,
		 aubuf_info.fragsize,
		 aubuf_info.bytes);
}
