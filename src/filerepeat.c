/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/07/31
 *
 * $Id: filerepeat.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include "def_value.h"
#include "pipe_hd.h"
#include "debug.h"
#include "timeval.h"

#include <sys/types.h>
#include <sys/stat.h>



typedef struct opt_values {	/* set by getenv and/or getopt */
	int clock;
	char *filename;
	int fd;
	int repeat_count;
	int realtime_f;		/* wait writeing to pipe ? */
	int cache_limit;
	int fill_lost;
} opt_t;


typedef struct statistics_values{
	int message_count;
} stat_t;


//static stat_t STAT;

int opt_etc(int argc, char *argv[], opt_t *opt);

u_int32_t
freq_check(opt_t *opt)
{
	unsigned int ts1, ts2;
	int len;
	char hd[PIPE_HEADER_LEN];
	int fd = opt->fd;
	
	if (read(fd, hd, sizeof(hd)) < sizeof(hd)){
		e_printf("\ncannot read 1st header\n");
		return -1;
	}
	if (pipe_get_version(hd) != 1){
		e_printf("\nUnavailable EMON message version %d.\n",
			 pipe_get_version(hd));
		return -1;
	}
	ts1 = pipe_get_timestamp(hd);
	len = pipe_get_length(hd);

	if (lseek(fd, len, SEEK_CUR) == -1){
		e_printf("\ncannot read 1st message"
			 "(lseek(%d) return erroe(%d)\n",
			 len, errno);
		return -1;
	}

	if (read(fd, hd, sizeof(hd)) < sizeof(hd)){
		e_printf("\ncannot read 2nd header\n");
		return -1;
	}
	if (pipe_get_version(hd) != 1){
		e_printf("\nUnavailable EMON message version %d.\n",
			 pipe_get_version(hd));
		return -1;
	}
	ts2 = pipe_get_timestamp(hd);

	if (lseek(fd, SEEK_SET, 0) == -1){
		e_printf("\ncannot seek\n");
		return -1;
	}
	
	return (ts2 - ts1);
}

#if 0
int
ts_make_random(int freq)
{
	struct timeval tmp;
	unsigned int seed;
	
	gettimeofday(&tmp, NULL);
	seed = tmp.tv_sec ^ tmp.tv_usec; /* ~:not , ^:xor */
	srand(seed);
		/* random seed */
	return freq * (rand() % 10000);
}
#endif


static int l_emonclk;



int
file2pipe(int freq, opt_t *opt)
{
	int i;
	ssize_t io_ret;
	const int fd = opt->fd;
	char hd_in_buf[PIPE_HEADER_LEN];
	char *hd_in=hd_in_buf;
	char hd_out[PIPE_HEADER_LEN];

	char *buf = NULL;
	int buf_len = 0;
	int frame = 0;
	int total_frame = 0;
	u_int32_t ts_pipe,ts_org,ts_last=0;

	int cache_enable=0;
	int cache_idx=0;
	int cache_len=0;
	char *cache_top=NULL;
#define cache_rest (cache_len - cache_idx)
#define cache_seek(len) (cache_idx+=len)

	int skip_cnt=0;
	int dup_cnt=0;

	struct timeval start_tv,now_tv;

	int64_t      start_ts,now_ts,diff_ts;          
	//	unsigned int seed;
	

	l_emonclk=opt->clock;		/* TODO modify. */

	if (opt->cache_limit!=-1){
	  struct stat finf;
	  fstat(fd,&finf);
	  cache_len=finf.st_size;
	  if (opt->cache_limit==0){ /*finf.st_size< opt->cache_limit<<(20) ||*/
	    cache_top = malloc(cache_len);
	    if(cache_top==NULL){
	      e_printf("cache enabled ,but fail to malloc.\n");
	      return (-1);
	    }
	    lseek(fd, 0, SEEK_SET);
	    io_ret= read(fd,cache_top,cache_len);
	    if(io_ret==-1){
	      e_printf("fail to read %s.\n",opt->filename);
	      return (-1);
	    }
	    cache_enable=1;
	    cache_idx=0;
	    d1_printf("cache setup succeed.\n");
	  }else{
	    /*TODO*/
	  }
	}

	gettimeofday(&start_tv, NULL);
	start_ts = timeval_2ts(&start_tv,opt->clock);

	ts_pipe = start_ts - freq;
	now_ts = start_ts - freq;

	d1_printf ("%d start_ts %x  ts_pipe %x \n",
		   opt->clock,(u_int32_t)start_ts,ts_pipe);
	for (i = 0; opt->repeat_count == 0 || i < opt->repeat_count; i++){
		if(cache_enable){
			cache_idx=0;
		}else{
			lseek(fd, 0, SEEK_SET);
		}
		d1_printf("\nloop:%d start TS=%x", i,ts_pipe);
		for (frame = 0;; frame++, total_frame++){
			int rret;
			int len;
			int tmp;
			int m;	/* marker */

			if(cache_enable){
				if(cache_rest < PIPE_HEADER_LEN){
					rret=0;
				}else{
					rret=PIPE_HEADER_LEN;
					hd_in=cache_top+cache_idx;
					cache_seek(PIPE_HEADER_LEN);
				}
			}else{
				rret = read(fd, hd_in, PIPE_HEADER_LEN);
			}

			if (rret == 0){
				d2_printf("..EOF was found");
				break;
			} else if (rret == -1) {
				e_printf("\nread returned error (%d)", errno);
				break;
			}
			if ((tmp = pipe_get_version(hd_in)) != 1){
				e_printf("\nUnavailable EMON message version"
					 " %d. in frame# %d\n", tmp, frame);
				return -1;
			}

			len = pipe_get_length(hd_in);
			if (len == 0xffffff){
				d1_printf("..End-Message was found");
				break;
			}
			if(cache_enable){
				if(cache_rest < len){
					rret=0;
				}else{
					rret=len;
					buf=cache_top+cache_idx;
					cache_seek(len);
				}
			}else{
				if (len > buf_len){
					free(buf);
					buf = malloc(len);
					buf_len = len;
					if (buf == NULL){
						e_printf("\ncannot allocate memory"
							 "(%d)\n", len);
						return -1;
					}
					d1_printf("\nmalloc(%d) for buf", len);
				}
				rret = read(fd, buf, len);
			}
			if (rret < len){
				e_printf("\ncannot read from file"
					 " (%dbytes)\n", len);
				return -1;
			}

			if (frame == 0){
				dup_cnt=1;
				if(opt->fill_lost){
				}else{
					ts_pipe += freq;
					now_ts +=freq;
				}
				ts_last = pipe_get_timestamp(hd_in);

			} else {
				ts_org   = pipe_get_timestamp(hd_in);
				if(opt->fill_lost){
					if (freq == 0){
						dup_cnt = 1;
					} else {
						dup_cnt = (u_int32_t)((ts_org - ts_last)/freq);
					}
				}else{
					dup_cnt=1;
					ts_pipe +=(u_int32_t)(ts_org - ts_last);
					now_ts  +=(u_int32_t)(ts_org - ts_last);
				}
				if(ts_org < ts_last){
				  d1_printf("DEBUG:rtpTS overflow \n");
				}
				if(ts_org - ts_last>freq){
				  d3_printf("DEBUG:TSdiff %u\n",ts_org - ts_last);
				}
				ts_last  = ts_org;
			}
			while(dup_cnt){

				dup_cnt--;
				if(opt->fill_lost){
					ts_pipe += freq;
					now_ts +=freq;
				}
				gettimeofday(&now_tv , NULL);
				diff_ts=timeval_sub_ts(&now_tv,&start_tv,opt->clock);
				if(diff_ts >
				   now_ts-start_ts
				   + /*delay_limit*/(l_emonclk/1000)*100/*msec*/
					){
					d2_printf("INFO:skip 1packet (%d)(%uTS)\n",skip_cnt,ts_last);
					skip_cnt++;
					continue;
				}
				while((now_ts-start_ts)>diff_ts){
					d2_printf("write too fast,"
						  "sleep 1msec(fr=%5d)\n"
						  ,total_frame);
					usleep(1000);
					gettimeofday(&now_tv , NULL);
					diff_ts=timeval_sub_ts(&now_tv,&start_tv,opt->clock);
				}

				bzero(hd_out, PIPE_HEADER_LEN);
				pipe_set_version(hd_out, 1);
				pipe_set_marker(hd_out, m = pipe_get_marker(hd_in));
				pipe_set_length(hd_out, len);
				pipe_set_timestamp(hd_out, ts_pipe);
				io_ret=pipe_blocked_write_block(STDOUT, hd_out, buf);
				d3_printf("\nts=%d, len=%d, m=%d",
					  ts_pipe, len, m);
				if(io_ret==PIPE_ERROR){
					e_printf("PIPE_ERROR exit..\n");
					break;
				}
			}
		}
	}

	return 0;
}



int
main(int argc, char *argv[])
{
	opt_t OPT;
	int freq;
	//	unsigned int ts;

	if (opt_etc(argc, argv, &OPT) < 0){
		return -1;
	}
	if ((freq = freq_check(&OPT)) < 0){
		return -1;
	}
	d1_printf("freq=%d ", freq);

#if 0
	ts = ts_make_random(freq);
	d1_printf(", 1st ts=%u ", ts);
#endif 
	if (isatty(STDOUT)) {
		e_printf("Standard output must be binded with pipe.\n");
		return -1;
	}

	file2pipe(freq, &OPT);
	
	d1_printf("\n");
	return 0;
}


int
opt_etc(int argc, char *argv[], opt_t *opt)
{
	int ch;
	char *opts = "C:r:wD:cf";
	char *helpmes =
		"usage : %s [options] filename\n"
		" options\n"
		" <general>\n"
		"  -C <n>   : clock base\n"
		" <fileout>\n"
		"  -r <n>   : repeat count <0:endless>\n"
		"  -w       : wait writing data until time\n"
		"  -c       : read all of file to memory before fileout\n"
		"  -f       : fill lost frame\n"
		" <debug>\n"
		"  -D <n>   : debug level\n"
		"\n";
	char *temp;

	opt->clock = DEF_EMON_CLOCK;
	opt->filename = NULL;
	opt->fd = -1;
	opt->repeat_count = 0;
	opt->realtime_f = 0;	/* flag: off */
	opt->cache_limit=-1;  /* flag: off==-1 */
	opt->fill_lost=0;
	if ((temp = getenv("EMON_CLOCK")) != NULL)
		opt->clock = atoi(temp);

	while ((ch = getopt(argc, argv, opts)) != -1){
		switch (ch){
		case 'C':
			opt->clock = atoi(optarg);
			break;
		case 'f':
			opt->fill_lost  = 1;/* flag: on==1 */
			break;
		case 'c':
			opt->cache_limit = 0;/* flag: on==0 */
			break;
		case 'r':
			opt->repeat_count = atoi(optarg);
			break;
		case 'w':
			opt->realtime_f = 1; /* flag: on */
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		default:
			e_printf(helpmes, argv[0]);
			return -1;
		}
	}
	if (argc - optind < 1){
		e_printf(helpmes, argv[0]);
		return -1;
	}
	opt->filename = argv[optind];
	if ((opt->fd = open(opt->filename, O_RDONLY))== -1){
		e_printf("cannot open file \"%s\"\n", opt->filename);
		return -1;
	}

	return 0;
}
