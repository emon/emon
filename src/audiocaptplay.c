/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/09/25
 *
 * $Id: audiocaptplay.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

/*
  example:
    audiocaptplay -f 0 -- ./audioplay -f l8 -- ./audioplay -f l8
   ./udprecv |./audiocaptplay -- ./audioplay -- ./audiocapt| ./udpsend
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

#include <sys/wait.h>
#include <string.h>

#include "aucodec.h"
#include "debug.h"
#include "pipe_hd.h"
#include "def_value.h"


#define MAX(a,b) (a>b? a:b)
#define MIN(a,b) (a>b? b:a)
       

#define ENDIAN_LITTLE


#define DEFAULT_DSP_DEVICE      "/dev/dsp"

/*0x0000ffff log(len)  [0=>E(0=>1<<E) es1371_OSS]*/
/*0xffff0000 # of frag (is ignored.)*/
#define DEFAULT_DSP_FRAGMENT_ARG  (0xffff0006) 

#define DEFAULT_DSP_FMT          (AFMT_S16_LE)
#define DEFAULT_DSP_RATE         (44100)
#define DEFAULT_DSP_CHANNEL      (2)

#define DSP_RATE_FIX(hz) (hz)
/*for ESS Maestoro OSS / Toshiba DynaBookSS3380V*/
//#define DSP_RATE_FIX(hz) ((hz)+(hz)/58)
/* for test */
//#define DSP_RATE_FIX(hz) ((hz)+(hz)/10)

typedef struct _arg_t{
	char           *dsp_dev;
	int             dsp_rate;
	int             dsp_ch;
	int             dsp_fmt;
	int             capt2play;
#if 0
	int                emon_clk; 
	int                emon_freq;
        int                plen;
	int                dsp_rate;
	int                dsp_ch;
#endif
	char*           play_path;
	char*           capt_path;

	char**          play_argv;
	char**          capt_argv;
}arg_t;

static arg_t   OPT;
static int     afd;
static u_int32_t parsearg(int argc, char *argv[],arg_t *opt);
static int       open_dsp();
static int       start_captplay(pid_t *play,pid_t *capt);

static	pid_t  capt_pid,play_pid;


int 
main(int argc, char **argv)
{

	int status;
	pid_t pid;
	int  pid_cnt;

	if (parsearg(argc, argv,&OPT) > 0) {
		return -1;
	}
#if 0
	e_printf("optarg end\n");
	e_printf("dev %s rate %d ch %d ossfmt 0x%x\n"
		 "play_path %s\ncapt_path %s"
		 "play_argv1 %s capt_argv1 %s\n"
		 ,OPT.dsp_dev,OPT.dsp_rate,OPT.dsp_ch
		 ,OPT.dsp_fmt,OPT.play_path,OPT.capt_path
		 ,OPT.play_argv[0],OPT.capt_argv[0]);
	return 0;
#endif

	if ((afd = open_dsp()) < 0) {
		e_printf("fail to set up dsp\n");
		return -1;
	}
	if (start_captplay(&play_pid,&capt_pid)<0){
		return -1;
	}
	pid_cnt=2;
	while(pid_cnt>0){
		pid=wait(&status);
		if(pid==capt_pid){
			capt_pid=0;
			pid_cnt--;
		}
		else if (pid==play_pid){
			play_pid=0;
			pid_cnt--;
		}
	}
	return 0;
}

int       start_captplay(pid_t *play,pid_t *capt){
	char  *argv[0x100];
	char  buf1[0x100];
	char  buf2[0x100];
	char  arg_str1[]="-X";
	char  arg_str2[]="-Y";
	int   pipe_capt2play[2];
	int cnt;
	pid_t pid;
	
	sprintf(buf1,"%d",afd);
	sprintf(buf2,"%d",OPT.dsp_fmt);
	if(OPT.capt2play){
		pipe(pipe_capt2play);
	}

	cnt=0;
	while(OPT.play_argv[cnt]!=NULL){
		argv[cnt]=OPT.play_argv[cnt];
		cnt++;
	}
	argv[cnt]=arg_str1;	/* -X for dsp fd */
	cnt++;
	argv[cnt]=buf1;		/* dsp fd */
	cnt++;
	argv[cnt]=arg_str2;	/* -Y for dsp fmt */
	cnt++;
	argv[cnt]=buf2;		/* dsp fmt */
	cnt++;
	argv[cnt]=NULL;

        pid=fork();
	if(pid==0){
		/*play*/
		if(OPT.capt2play){
			e_printf("audiopcaptplay dup2\n");
			close(pipe_capt2play[1]);
			close(STDIN_FILENO);
			dup2(pipe_capt2play[0],STDIN_FILENO);
		}
		execvp(argv[0],argv);
		e_printf("audiocaptplay:audioplay fail.\n");
		exit(1);
	}
	if(pid==-1){
		e_printf("audiocaptplay:fork fail.\n");
		return -1;
	}
	*play=pid;

	cnt=0;
	while(OPT.capt_argv[cnt]!=NULL){
		argv[cnt]=OPT.capt_argv[cnt];
		cnt++;
	}
	argv[cnt]=arg_str1;
	cnt++;
	argv[cnt]=buf1;
	cnt++;
	argv[cnt]=arg_str2;
	cnt++;
	argv[cnt]=buf2;
	cnt++;
	argv[cnt]=NULL;

        pid=fork();
	if(pid==0){
		/*capt*/
		if(OPT.capt2play){
			close(pipe_capt2play[0]);
			close(STDOUT_FILENO);
			dup2(pipe_capt2play[1],STDOUT_FILENO);
		}
		execvp(argv[0],argv);
		e_printf("audiocaptplay:audiocapt fail.\n");
		exit(1);
	}
	if(pid==-1){
		e_printf("audiocaptplay:fork fail.\n");
		return -1;
	}
	*capt=pid;
	return 0;
}

int
open_dsp(void)
{
	int             val;
	u_int32_t       ret;

	ret=0;


	/* open audio device */
	if ((afd = open(OPT.dsp_dev,O_RDWR, 0)) < 0) {
		e_printf("cannot open audio device\n");
		return -1;
	}
	ioctl(afd, SNDCTL_DSP_SETDUPLEX, 0);
	ioctl(afd, SNDCTL_DSP_GETCAPS, &val);
	if((val&DSP_CAP_DUPLEX) ==0){
		close(afd);
		e_printf("%s is not support full-duplex.\n",OPT.dsp_dev);
		return -1;
	}

#if 1
	val = DEFAULT_DSP_FRAGMENT_ARG;
	if (ioctl(afd, SNDCTL_DSP_SETFRAGMENT, &val) < 0) {
		e_printf("SNDCTL_DSP_SETFRAGMENT: cannot set audio device \n");
		return -1;
	}
#else
	val = OPT.fragment_size;/* fragment size  (is not for DMA ...) */
	if (ioctl(afd, SNDCTL_DSP_SETBLKSIZE, &val) < 0) {
		e_printf("SNDCTL_DSP_SETBLKSIZE: cannot set audio device \n");
		return -1;
	}
#endif


	/* set audio device */
	val = OPT.dsp_ch;
	/* old FreeBSD don't support SNDCTL_DSP_CHANNELS  */
	if (ioctl(afd, SOUND_PCM_WRITE_CHANNELS, &val) < 0) {
		printf("SNDCTL_DSP_CHANNELS: cannot set channel\n");
		return -1;
	}

	
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
	return afd;
}



u_int32_t parsearg(int argc, char *argv[],arg_t *opt){
//	int             loop;
	char            c;
//	char           *tmpstr;
	char           *opts = "d:C:R:L:f:r:c:D:t";
	char           *helpmes =
	"usage : %s [-f <au_fmt>] "
	"[-r <dsp_rate>] [-c <chanel>] [-t] -- [play args] -- [capt args]\n"
	" options\n"
	"  d <s>   : dsp path [" DEFAULT_DSP_DEVICE "]\n"
	"  f <n>   : audio format {0:L8,1:L16,DVI4,PCMU}[1]\n"
	"  r <n>   : sampling rate[44100]\n"
	"  c <n>   : # of chanel[2]\n"
	"  t       : (mainly for loopback test) connect capt to play(optional)\n"
	" debug option\n"
	"  D <n>   : debug message level[0]\n"
	"example: ./audiocaptplay -c1 -- ./audioplay -c1 -- ./audiocapt -c1\n"
		;
	opt->dsp_dev   = DEFAULT_DSP_DEVICE;

	opt->dsp_rate  = DEFAULT_DSP_RATE;
	opt->dsp_ch    = DEFAULT_DSP_CHANNEL;
	opt->dsp_fmt   = DEFAULT_DSP_FMT;
	opt->capt2play = 0;
	while ((c = getopt(argc, argv, opts)) != -1 ){
		switch (c) {
 		case 'd': 
 			opt->dsp_dev=optarg; 
 			break; 
		case 'f':
			opt->dsp_fmt= 8<<atoi(optarg);
			if(atoi(optarg)>1){
				e_printf("AUDIO_FORMAT %s is not supported.\n"
				       ,optarg);
				return 2;
			}
			break;
		case 'r':
			opt->dsp_rate = atoi(optarg);
			break;
		case 'c':
			opt->dsp_ch = atoi(optarg);
			break;
		case 't':
			opt->capt2play=1;
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		case '?':
		default:
			e_printf(helpmes, argv[0]);
			return 1;
		}
	}
	if(strcmp(argv[optind-1],"--")!=0){
		e_printf("\"--\" needed.\n");
		return 1;
	}
	OPT.play_argv=argv+optind;
	while(argv[optind]!=NULL && strcmp(argv[optind],"--")!=0){
		optind++;
	}
	if(argv[optind]==NULL){
		e_printf("second \"--\" needed.\n");
		return 1;
	}
	argv[optind]=NULL;
	OPT.capt_argv=argv+optind+1;

	return 0;
}

