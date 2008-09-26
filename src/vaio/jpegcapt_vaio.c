/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 *            TONOIKE Masatsugu <tonoike@kuis.kyoto-u.ac.jp>
 * started:   2001/07/10
 *
 * $Id: jpegcapt_vaio.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */


//#define WAIT_CAPTURE		/* or comment-out this line */

#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include <jpeglib.h>

#include "pipe_hd.h"
#include "timeval.h"
#include "debug.h"
#include "def_value.h"
#include "jpegbuf/bufio.h"

#include "capture_vaio.h"

typedef struct opt_values {	/* set by getenv and/or getopt */
	int clock;
	int freq;
	int width, height;
	char *dev;
	int field;
	int quality;
	int dsize;
	int fps_opt;
	int fps_dev;
	int max_frame;
	int stat_freq;
} opt_t;

typedef struct statistics_values {
	struct timeval start;
	struct timeval usleep_total;
	struct timeval usleepr_total;
	struct timeval capt_total;
	struct timeval jpgenc_total;
	int usleep_count;
	int skip_count;
	int capt_count;
	int out_count;
} stat_t;

static stat_t STAT;

static int opt_etc(int argc, char *argv[], opt_t *opt);
void stat_init(stat_t *stat);

void capt_init(opt_t *opt);
int  capt_capt(opt_t *opt);
void capt_end(opt_t *opt);

extern GLOBAL(void) jpeg_buf_dest(j_compress_ptr, buf_t *);


/* functions */

void
stat_init(stat_t *stat)
{
	stat->start.tv_sec = 0;
	stat->start.tv_usec = 0;

	stat->usleep_total.tv_sec = 0;
	stat->usleep_total.tv_usec = 0;

	stat->capt_total.tv_sec = 0;
	stat->capt_total.tv_usec = 0;

	stat->jpgenc_total.tv_sec = 0;
	stat->jpgenc_total.tv_usec = 0;

	stat->skip_count = 0;
	stat->capt_count = 0;
	stat->out_count = 0;

	return;
}


void
stat_print(stat_t *stat, unsigned int frame_num)
{
	static int first = 1;
	struct timeval now;
	struct timeval avg_usleep;
	struct timeval avg_usleepr;
	struct timeval avg_capt;
	struct timeval avg_jpgenc;

	gettimeofday(&now, NULL);
	avg_usleep = timeval_average_timeval(&stat->usleep_total, frame_num);
	avg_usleepr = timeval_average_timeval(&stat->usleepr_total, frame_num);
	avg_capt = timeval_average_timeval(&stat->capt_total, frame_num);
	avg_jpgenc = timeval_average_timeval(&stat->jpgenc_total, frame_num);

	if (first){
		first = 0;
		d1_printf("\njpegcapt stat[time]"
			  " frame#(out+capt+skip)"
			  " | avg usec{slp, slpr, capt, jpgenc}"
				  /* capt = only capturing, no output */
			  );
	}
	d1_printf("\njpegcapt stat[%ld.%06ld]"
		  " %u(%d+%d+%d)"
		  " | %ld, %ld, %ld, %ld" ,
		  now.tv_sec, now.tv_usec,
		  frame_num,
		  stat->out_count,
		  stat->capt_count - stat->out_count,
		 /* capt = only capturing, no output */
		  stat->skip_count,
		  avg_usleep.tv_usec,
		  avg_usleepr.tv_usec,
		  avg_capt.tv_usec,
		  avg_jpgenc.tv_usec
		  );
	return;
}
		  

int
jpeg_encode(unsigned char *src, unsigned char *dst,
	    int width, int height, int quality)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int row_stride;
	buf_t buf;		/* from jpegbuf/bufio.h */

	buf.writecnt = 0;
	buf.readcnt = 0;
	buf.buf = dst;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpeg_buf_dest(&cinfo, &buf);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	cinfo.dct_method = JDCT_IFAST;
//	cinfo.dct_method = JDCT_ISLOW;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

        row_stride = width * 3;

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &src[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	return (buf.writecnt);
}


int
wait_proper_time(struct timeval *start, int fps,
		 unsigned int frame_num, int wait_flag)
/*
 * wait_flag :  0 => return -1 or 0
 *	       !0 => return -1, 0 or 1>
 *
 * return -1, 0 or over then 1
 *     -1 : no wait because too late
 *	0 : no wait, just in time
 *      1>: wait some second
 */
{
	struct timeval now;
	struct timeval elapse;
	struct timeval estimate;
	struct timeval now2;
	int diff_usec;
	int slp_c = 0;

		/* start of time check */
	for (;;){
		gettimeofday(&now, NULL);
		elapse = timeval_sub_timeval(&now, start);

		estimate.tv_sec = frame_num / fps;
		estimate.tv_usec = ((frame_num % fps)*1000*1000) / fps;

		diff_usec = timeval_diff_usec(&estimate, &elapse);

		if (!wait_flag){
			break;
		}
		if (diff_usec <= 0){
			break;
		} else {
			usleep(diff_usec);
			timeval_add_usec(&STAT.usleep_total, diff_usec);

			gettimeofday(&now2, NULL);
			diff_usec = timeval_diff_usec(&now2, &now);
			timeval_add_usec(&STAT.usleepr_total, diff_usec);

			d3_printf(" us:%d", diff_usec);
			slp_c++;
			/* break; */ 
				/* check again! (=loop again!) */
				/* to cape with too short usleep */
		}
	}
	if (diff_usec < -(1000*1000/fps)) {	/* too late, skip */
		d3_printf("\nfram=%u: skip", frame_num);
		return -1;
	}
	d3_printf(" [%d]", slp_c);

	STAT.usleep_count += slp_c;
	return slp_c;
}


static void sigint(void)
{
	spic_shutdown(0);
	_exit(1);
}


static void lose_privileges(void)
{
	uid_t uid;
	uid = getuid();
	if (uid != 0) {
		setuid(0);
		setuid(uid);
		if (geteuid() != uid || getuid() != uid) {
			printf("Failed to drop privileges - exiting\n");
			exit(1);
		}
	}
}


void
capt_init(opt_t *opt)
{
	int subsample = 1;		/* 0: 640x480, 1: 320x240 */
	int spic_brightness=32, spic_contrast=32, spic_hue=32;
	int spic_color=32, spic_sharpness=32;
	int spic_agc=48, spic_picture=0;

	spic_init();
	spic_camera_on();
	mchip_init();

	signal(SIGINT, sigint);

	lose_privileges();
	spic_settings(spic_brightness, spic_contrast, spic_hue, spic_color, 
		      spic_sharpness, spic_picture, spic_agc);

	if (debug_level >= 1) {
		spic_show_settings();
	}
	mchip_subsample(subsample & 1);
	mchip_set_framerate(opt->fps_opt);
	return;
}


int
capt_capt(opt_t *opt)
{
	int frame_size;
	unsigned int frame_num;
	unsigned char *frame_buf_yuv, *frame_buf_rgb, *jpeg_buf;
	struct timeval start;
	const int fps = opt->fps_opt;
	const int wait_flag = (opt->fps_opt < opt->fps_dev);
	const int w = opt->width, h = opt->height;
	int q = 75;		/* jpeg compress quality */
	int len;
	char mh[PIPE_HEADER_LEN]; /* EMON system Message Header */
	unsigned int ts;	/* timestamp in Message Header */

	frame_size = mchip_hsize() * mchip_vsize() * 3;
	frame_buf_yuv = (unsigned char*)malloc(frame_size);
	frame_buf_rgb = (unsigned char*)malloc(frame_size);
	jpeg_buf = (unsigned char*)malloc(frame_size);
		/* allocate enougth memory for jpeg_buf */
	if (frame_buf_yuv == NULL || frame_buf_rgb == NULL ||jpeg_buf == NULL){
		e_printf("cannot malloc for frame_buf or jpeg_buf\n");
		return -1;
	}

	stat_init(&STAT);
	ts = rand() * opt->freq;
	mchip_continuous_start();
	gettimeofday(&start, NULL);
	STAT.start = start;	/* copy struct timeval */

	d2_printf("\njpegcapt: %ld.%06ld: wait_flag=%d",
		  start.tv_sec, start.tv_usec, wait_flag);
	
	for (frame_num = 0; opt->max_frame == 0 || frame_num < opt->max_frame; frame_num++, ts += opt->freq){
		struct timeval c, b, a; /* capture, before(encoding), after */
		int d1, d2;

		if (debug_level > 0 && (frame_num % opt->stat_freq)== 0){
			stat_print(&STAT, frame_num);
		}

		if (wait_proper_time(&start, fps, frame_num, 1) < 0){
			STAT.skip_count++;
			continue; /* skip capture because it's too late */
		}
		STAT.capt_count++;
		gettimeofday(&c, NULL);
		mchip_continuous_read(frame_buf_yuv,
				      mchip_hsize()*mchip_vsize()*2);
		yuv_convert(frame_buf_yuv, frame_buf_rgb,
			    mchip_hsize(), mchip_vsize());
		gettimeofday(&b, NULL);
		len = jpeg_encode(frame_buf_rgb, jpeg_buf, w, h, q);
		gettimeofday(&a, NULL);
		d1 = timeval_diff_usec(&b, &c);
		d2 = timeval_diff_usec(&a, &b);
		timeval_add_usec(&STAT.capt_total, d1);
		timeval_add_usec(&STAT.jpgenc_total, d2);
		d3_printf("\n frame=%d, ts=%d, jpg_len=%d, q=%d"
			  ", t1=%d, t2=%d",
			  frame_num, ts, len, q, d1, d2);

		if (len > opt->dsize ){
			q *= 0.75;
			continue; /* skip this picture */
		}else if (len < opt->dsize * 0.9 && q < 90) {
			q++;
		}
		bzero(&mh, PIPE_HEADER_LEN);
		pipe_set_version(&mh, 1);
		pipe_set_marker(&mh, 1);
		pipe_set_length(&mh, len);
		pipe_set_timestamp(&mh, ts);
		if (pipe_blocked_write_block(STDOUT, &mh, jpeg_buf)
		    ==PIPE_ERROR){
			d1_printf("\npipe_blocked_write_block error!!"
				  "len=%d, ts=%ud", len, ts);
		} else {
			STAT.out_count++;
		}
	}
	if (debug_level > 0){
		stat_print(&STAT, frame_num);
	}
	return 0;
}


void
capt_end(opt_t *opt)
/*
 * send End of Messages Header
 * and output statistics
 */
{
	char mh[PIPE_HEADER_LEN]; /* EMON system Message Header */
	int len = 0xffffff;	/* end of messages */
	int ts = 0;
	
	bzero(&mh, PIPE_HEADER_LEN);
	pipe_set_version(&mh, 1);
	pipe_set_marker(&mh, 1);
	pipe_set_length(&mh, len);
	pipe_set_timestamp(&mh, ts);
	if (pipe_blocked_write_block(STDOUT, &mh, NULL)
	    ==PIPE_ERROR){
		d1_printf("\npipe_blocked_write_block error!!"
			  "len=%d, ts=%ud", len, ts);
	}
	return;
}



int
main(int argc, char *argv[])
{
	opt_t OPT;

	if (opt_etc(argc, argv, &OPT) < 0){
		return -1;
	}
	if ((OPT.clock % OPT.freq) != 0){
		e_printf("Frames per sec(=clock/freq) must be integar.\n");
		return -1;
	}
	if (isatty(STDOUT)) {
		e_printf("Standard output must be binded with pipe.\n");
		return -1;
	}
	capt_init(&OPT);
	capt_capt(&OPT);
	capt_end(&OPT);

	return 0;
}


static int
opt_etc(int argc, char *argv[], opt_t *opt)
{
	int ch;
	char *opts = "C:R:w:h:d:f:q:s:n:D:S:";
	char *helpmes =
		"usage : %s\n"
		" options\n"
		" <general>\n"
		"  -C <n>   : clock base\n"
		"  -R <n>   : frequency\n"
		" <jpeg capt>\n"
		"  -w <n>   : image width\n"
		"  -h <n>   : image height\n"
		"  -d <s>   : input device filename\n"
		"  -f <n>   : capture fileds (0:both fileds, 1:even , 2:odd)\n"
		"  -q <n>   : JPEG compression quality\n"
		"  -s <n>   : max data size\n"
		"  -n <n>   : # of capture frames\n"
		" <debug>\n"
		"  -D <n>   : debug level\n"
		"  -S <n>   : status output freq. (output per [S]-frames)\n"
		"\n";
	char *temp;
	
	opt->clock = DEF_EMON_CLOCK;
	opt->freq = DEF_EMON_FREQ;
	opt->width = DEF_JPEGCAPT_WIDTH;
	opt->height = DEF_JPEGCAPT_HEIGHT;
	opt->dev = DEF_JPEGCAPT_DEV;
	opt->field = DEF_JPEGCAPT_FIELD;
	opt->quality = DEF_JPEGCAPT_QUALITY;
	opt->dsize = DEF_JPEGCAPT_DSIZE;
	opt->max_frame = 0;	/* unlimited [-n <n>] */
	opt->stat_freq = 300;	/* 1stat / 300frames */
	
	/* get environment variables */

	if ((temp = getenv("EMON_CLOCK")) != NULL)
		opt->clock = atoi(temp);
	if ((temp = getenv("EMON_FREQ")) != NULL)
		opt->freq = atoi(temp);
	if ((temp = getenv("JPEGCAPT_WIDTH")) != NULL)
		opt->width = atoi(temp);
	if ((temp = getenv("JPEGCAPT_HEIGHT")) != NULL)
		opt->height = atoi(temp);
	if ((temp = getenv("JPEGCAPT_DEV")) != NULL)
		opt->dev = temp;
	if ((temp = getenv("JPEGCAPT_FIELD")) != NULL)
		opt->field = atoi(temp);
	if ((temp = getenv("JPEGCAPT_QUALITY")) != NULL)
		opt->quality = atoi(temp);
	if ((temp = getenv("JPEGCAPT_DSIZE")) != NULL)
		opt->dsize = atoi(temp);

	/* get options */
	while ((ch = getopt(argc, argv, opts)) != -1){
		switch (ch){
		case 'C':
			opt->clock = atoi(optarg);
			break;
		case 'R':
			opt->freq = atoi(optarg);
			break;
		case 'w':
			opt->width = atoi(optarg);
			break;
		case 'h':
			opt->height = atoi(optarg);
			break;
		case 'd':
			opt->dev = optarg;
			break;
		case 'f':
			opt->field = atoi(optarg);
			break;
		case 'q':
			opt->quality = atoi(optarg);
			break;
		case 's':
			opt->dsize = atoi(optarg);
			break;
		case 'n':
			opt->max_frame = atoi(optarg);
			break;
		case 'S':
			opt->stat_freq = atoi(optarg);
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
	}
	opt->fps_opt = opt->clock / opt->freq;

	d1_printf("\nclock=%d, freq=%d, fps=%d, size=(%d,%d)\n"
		  "input device=%s, capture field=%d",
		  opt->clock, opt->freq, opt->fps_opt, opt->width, opt->height,
		  opt->dev, opt->field
		  );
	return 0;
}
