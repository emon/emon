/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp> 
 * started:   2001/06/26
 *
 * $Id: jpegplay.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

/*
 * test program for libjpeg + SDL
 */

#define REALTIME_PLAY
#define USE_JPEG_MEM_SRC
#define USE_YUV_OVERLAY		/* libjpeg outputs YCbCr, SDL handle YUV
				 * overlay */

#ifndef USE_YUV_OVERLAY		/* USE_RGB_SURFACE */
#define SDL_MASK_RGB_ORDER 0
#endif

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#include <jpeglib.h>
#include "jpegbuf/bufio.h"

#include <SDL.h>
#include <SDL_timer.h>

#include "pipe_hd.h"
#include "timeval.h"
#include "decoder_buf.h"
#include "debug.h"
#include "def_value.h"

#define JPEG_BUF_MAX	(1024*100)	/* all jpeg date load in memory */

#ifndef RGB_PIXELSIZE		/* it's should be defined in jmorecfg.h */
#define RGB_PIXELSIZE 3
#endif
/* JPEG_YCbCr_PITCH means [YUV YUV YUV ..] for each pixels */
#define JPEG_YCbCr_PITCH RGB_PIXELSIZE


/* in ./jpegbuf/jbufsrc.c */
#ifdef USE_JPEG_MEM_SRC
extern          GLOBAL(JOCTET *) jpeg_mem_src_init(j_decompress_ptr, size_t);
extern          GLOBAL(void) jpeg_mem_src(j_decompress_ptr, unsigned char * inbuffer, unsigned long insize);
#else
extern          GLOBAL(void) jpeg_buf_src(j_decompress_ptr, buf_t *);
#endif


static volatile int jpeg_has_error = 0;	/* loop flag in decoding jpeg */

enum disp_format {
	DF_YUV,
	DF_RGB,
	DF_BGR,
};

typedef struct opt_values {
	int             frame_rate;
	int		clock;
	int		freq;
	int             dsize;
	int             delay_limit; /*msec*/
	double          mspf;	/* temprary millisec per frame for adjust */
	double          mspf_org;	/* original millisec per frame */
	int             f_disp;	/* flag */
	int             exec_opt_p;	/* exec options position in argv[] */
	int             output_width;
	int             output_height;
	int             sdl_mask_R, sdl_mask_G, sdl_mask_B, sdl_mask_A;
	enum disp_format dformat;
	char            title[255];
	int             loss_visual;
}               opt_t;



typedef struct statistics_values {
	struct timeval  start;
	int             frame_count;
	int             skip_count;
	int             wait_count;
	int             decode_usec;
	int             f_sigint;
}               stat_t;

static opt_t    OPT;
static stat_t   STAT;


/* prototype  */
static int      opt_etc(int argc, char *argv[], opt_t *opt);
void            statistics_print(stat_t * st);
void            sigint_quit(void);
void            sigint_trap(int sig);


static
void 
wait4rtdisplay(struct timeval *tv_start,u_int64_t ts_limit){
	int64_t ts_diff;
	struct timeval  now;

	while(1){
	  gettimeofday(&now, NULL);		
	  ts_diff=timeval_sub_ts(&now,tv_start,OPT.clock);
	  if(ts_diff >= ts_limit){
	    break;
	  }
	  d3_printf("sleep.\n");
	  decoder_buf_read();	/* check new data */
	}
}


void
my_jpeg_abort_decompress(j_common_ptr cinfo)
{
	d_printf("\nmy_jpeg_abort_decompress called, jpeg_has_error=%d\n",
		 jpeg_has_error);
	jpeg_has_error = 1;
	jpeg_abort_decompress((j_decompress_ptr) cinfo);
	exit(-1);
}



void 
ComplainAndExit(void)
{
	fprintf(stderr, "\nProblem: %s\n", SDL_GetError());
	exit(1);
}


#ifdef USE_YUV_OVERLAY		/* wrap the function */
SDL_Overlay    *
my_SDL_CreateYUVOverlay(int w, int h, SDL_Surface * display)
{
	SDL_Overlay    *sdl_overlay = NULL;
	int             yuv_fmt = 0;
	u_int32_t       YUV_FMT[] = {
		SDL_YV12_OVERLAY,
		SDL_IYUV_OVERLAY,
		//SDL_YUY2_OVERLAY,
		//SDL_UYVY_OVERLAY,
		//SDL_YVYU_OVERLAY,
		0,
	};

	for (yuv_fmt = 0; YUV_FMT[yuv_fmt] != 0; yuv_fmt++) {
		sdl_overlay = SDL_CreateYUVOverlay(w, h,
						 YUV_FMT[yuv_fmt], display);
		if (sdl_overlay == NULL)
			continue;
		if (sdl_overlay->hw_overlay == 1) {
			break;
		} else {
			SDL_FreeYUVOverlay(sdl_overlay);
		}
	}
	if (YUV_FMT[yuv_fmt] == 0) {	/* cannot hardware overlay */
		for (yuv_fmt = 0; YUV_FMT[yuv_fmt] != 0; yuv_fmt++) {
			sdl_overlay =
				SDL_CreateYUVOverlay(w, h,
						 YUV_FMT[yuv_fmt], display);
			if (sdl_overlay != NULL)
				break;
		}
	}
	return sdl_overlay;
}



int 
jpeg_display_yuv(int argc, char *argv[])
{
	u_int8_t       *y_buf = NULL, *u_buf = NULL, *v_buf = NULL;
	u_int8_t       *yuv_buf_tmp;
	u_int8_t       *jpeg_buf;
	buf_t           jpeg_src_buf;
	size_t          read_size;	/* read size from file */
	int             i, x, y, offU, offV;
	int             row_stride;
	int             w, h, d;/* display size and depth(byte) */
	boolean         draw_screen;	/* draw screen surface or another
					 * surface */

	/* for jpeg library */
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	/* for SDL */
	SDL_Surface    *screen;
	SDL_Overlay    *sdl_overlay = NULL;
	SDL_Rect        dstrect;
	u_int32_t       tick_start, tick_now;

	/* for realtime play */
	struct timeval  tv_start,tv_now;
	int64_t         ts_display,ts_diff;
	u_int32_t       ts_nowblk,ts_lastblk;

	/* for debug */
	struct timeval  tv_tmp1, tv_tmp2;

	/* SDL init */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		ComplainAndExit();
	}
	SDL_WM_SetCaption(OPT.title, OPT.title);
	atexit(SDL_Quit);

	/* jpeg library init / get image size and depth */
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = my_jpeg_abort_decompress;
	jpeg_create_decompress(&cinfo);

#ifdef USE_JPEG_MEM_SRC
	jpeg_buf = jpeg_mem_src_init(&cinfo, JPEG_BUF_MAX);	/* init */
#else				/* Mr.Okamura's */
	jpeg_buf = (u_int8_t *) malloc(JPEG_BUF_MAX);
#endif

	decoder_buf_read();	/* pipe -> buffer */
	read_size = decoder_buf_get(jpeg_buf, JPEG_BUF_MAX,&ts_nowblk);	/* buffer -> mem */

#ifdef USE_JPEG_MEM_SRC		/* emon's original */
	jpeg_mem_src(&cinfo, jpeg_buf, read_size);
#else				/* Mr.Okamura's */
	jpeg_src_buf.buf = jpeg_buf;
	jpeg_buf_src(&cinfo, jpeg_buf, read_size);
#endif
	jpeg_read_header(&cinfo, TRUE);
	w = cinfo.image_width;
	h = cinfo.image_height;
	d = cinfo.num_components;
	jpeg_abort_decompress(&cinfo);
	offU = (w * h) * 4 / 4;
	offV = (w * h) * 5 / 4;

	d_printf("\nJPEG info: image=(%d,%d), output=(%d,%d), Bpp=%d\n",
		 cinfo.image_width, cinfo.image_height, w, h, d);

	/* SDL setup */
	draw_screen = FALSE;	/* can I draw image VRAM directly ? */
	if ((screen = SDL_SetVideoMode(w, h, 0, SDL_ASYNCBLIT)) == NULL) {
		ComplainAndExit();
	}
	d1_printf("\nSDL screen  info: bpp=%d, Bpp=%d, "
		  "R/G/B-mask=%06x/%06x/%06x\n",
		  screen->format->BitsPerPixel,
		  screen->format->BytesPerPixel,
		  screen->format->Rmask,
		  screen->format->Gmask,
		  screen->format->Bmask
		);

	if ((sdl_overlay = my_SDL_CreateYUVOverlay(w, h, screen)) == NULL) {
		ComplainAndExit();
	}
	d1_printf("\nSDL surface info: format=0x%08x, planes=%d, "
		  "hw_overlay_flag=%d\n",
		  sdl_overlay->format,
		  sdl_overlay->planes,
		  sdl_overlay->hw_overlay
		);

	row_stride = JPEG_YCbCr_PITCH * w;
	yuv_buf_tmp = malloc(JPEG_YCbCr_PITCH * w * h);

	STAT.skip_count = 0;
	STAT.wait_count = 0;
	tick_start = SDL_GetTicks();	/* get start time */
	gettimeofday(&STAT.start, NULL);

	STAT.f_sigint = 0;
	signal(SIGINT, sigint_trap);
	for (STAT.frame_count = 1;; STAT.frame_count++) {
		int             sleep_usec = 0;
		if (STAT.f_sigint)
			sigint_quit();

		jpeg_has_error = 0;	/* reset flag */

		decoder_buf_read();
		read_size = decoder_buf_get(jpeg_buf, JPEG_BUF_MAX,&ts_nowblk);
#ifdef REALTIME_PLAY
		tick_now = SDL_GetTicks();
		if (tick_now - tick_start > STAT.frame_count * OPT.mspf) {
			/* skip frame because it's too late */
			STAT.skip_count++;
			d2_printf("s");
			continue;
		}
#endif
		if (read_size == 0) {	/* buffer empty */
			usleep(OPT.mspf * (1000 * (9 / 10)));
			STAT.frame_count--;	/* no skip, no display */
			continue;
		} else if (read_size == -1) {
			break;	/* end of all files */
		}
#ifdef REALTIME_PLAY
		sleep_usec =
			((STAT.frame_count - 1) * OPT.mspf
			 - (tick_now - tick_start)) * 1000;
		if (sleep_usec > 0) {
			usleep(sleep_usec);
			STAT.wait_count++;
			//decoder_buf_read();	/* check new data */
		}
#endif
#ifdef USE_JPEG_MEM_SRC
		jpeg_mem_src(&cinfo, jpeg_buf, read_size);
#else
		jpeg_src_buf.buf = jpeg_buf;
		jpeg_buf_src(&cinfo, jpeg_buf, read_size);
#endif
		jpeg_read_header(&cinfo, TRUE);

		cinfo.output_width = w;
		cinfo.output_height = h;
		cinfo.out_color_space = JCS_YCbCr;
		/* more fast decompression */
		cinfo.dct_method = JDCT_FASTEST;
		//cinfo.dct_method = JDCT_FLOAT;
		cinfo.do_fancy_upsampling = FALSE;

		if (!jpeg_has_error) {
			jpeg_start_decompress(&cinfo);
		}
		/* JPEG decode start */
		gettimeofday(&tv_tmp1, NULL);
		while (cinfo.output_scanline < cinfo.output_height &&
		       !jpeg_has_error) {
			JSAMPLE        *yuv_scanline;
			yuv_scanline = &(yuv_buf_tmp[cinfo.output_scanline *
						     row_stride]);
			jpeg_read_scanlines(&cinfo, &yuv_scanline, 1);
		}
		jpeg_finish_decompress(&cinfo);
		/* JPEG decode finish */
		gettimeofday(&tv_tmp2, NULL);
		STAT.decode_usec += timeval_diff_usec(&tv_tmp2, &tv_tmp1);

		decoder_buf_read();	/* check new data */

		switch (sdl_overlay->format) {

		case SDL_YV12_OVERLAY:
#ifdef  SDL_1_1_5		/* for < SDL 1.1.5 */
			y_buf = (u_int8_t *) sdl_overlay->pixels;
			u_buf = &(y_buf[offU]);
			v_buf = &(y_buf[offV]);
#else
			y_buf = sdl_overlay->pixels[0];
			u_buf = sdl_overlay->pixels[1];
			v_buf = sdl_overlay->pixels[2];
#endif
			for (i = 0; i < w * h; i++) {
				y_buf[i] = yuv_buf_tmp[i * JPEG_YCbCr_PITCH];
			}
			for (i = 0, y = 0; y < h; y += 2) {
				for (x = 0; x < w; x += 2, i++) {
					const int       p =
					(y * w + x) * JPEG_YCbCr_PITCH;
					u_buf[i] = yuv_buf_tmp[p + 2];
					v_buf[i] = yuv_buf_tmp[p + 1];
				}
			}
			break;
		case SDL_IYUV_OVERLAY:
#ifdef  SDL_1_1_5		/* for < SDL 1.1.5 */
			y_buf = (u_int8_t *) sdl_overlay->pixels;
			v_buf = &(y_buf[offU]);
			u_buf = &(y_buf[offV]);
#else
			y_buf = sdl_overlay->pixels[0];
			v_buf = sdl_overlay->pixels[1];
			u_buf = sdl_overlay->pixels[2];
#endif
			for (i = 0; i < w * h; i++) {
				y_buf[i] = yuv_buf_tmp[i * JPEG_YCbCr_PITCH];
			}
			for (i = 0, y = 0; y < h; y += 2) {
				for (x = 0; x < w; x += 2, i++) {
					const int       p =
					(y * w + x) * JPEG_YCbCr_PITCH;
					u_buf[i] = yuv_buf_tmp[p + 2];
					v_buf[i] = yuv_buf_tmp[p + 1];
				}
			}
			break;
		default:
			d_printf("\nI'm sorry. not support YUV format: %0x08x\n",
				 sdl_overlay->format);
			exit(1);
			break;
		}

		if (OPT.f_disp) {	/* display flag check */
			dstrect.x = 0;
			dstrect.y = 0;
			dstrect.w = sdl_overlay->w;
			dstrect.h = sdl_overlay->h;

			if (SDL_LockYUVOverlay(sdl_overlay) < 0) {
				ComplainAndExit();
			}
			if (SDL_DisplayYUVOverlay(sdl_overlay, &dstrect) < 0) {
				SDL_FreeYUVOverlay(sdl_overlay);
				ComplainAndExit();
			}
			SDL_UnlockYUVOverlay(sdl_overlay);
		}
	}			/* loop for all jpeg file */

	STAT.frame_count--;	/* because count up from 1 */
	tick_now = SDL_GetTicks();
	if (!draw_screen) {
		SDL_FreeYUVOverlay(sdl_overlay);
	}
	jpeg_destroy_decompress(&cinfo);
	statistics_print(&STAT);
	return 0;
}

#endif				/* #ifdef USE_YUV_OVERLAY */


static void jpeg_fillimg1color(SDL_Surface *img
			      ,u_int8_t *col,int col_len){
	int x,y;
	u_int8_t *p,*p_x0;
	p_x0=img->pixels;
	for(y=0;y<img->h;y++){
		p=p_x0;
		for(x=0;x<img->w;x++){
			memcpy(p,col,col_len);
			p+=col_len;
		}
		p_x0+=img->pitch;
	}
	return;
}
static int jpeg_blitimg2screen(SDL_Surface *img,    /* proper name? */
			       SDL_Surface *screen){

	int ret;
	SDL_Rect        dstrect;

	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0) {
			ComplainAndExit();
		}
	}
	dstrect.x = 0;
	dstrect.y = 0;
	dstrect.w = img->w;
	dstrect.h = img->h;
	ret=0;
	if (SDL_BlitSurface(img, NULL, screen, &dstrect) < 0) {
		ret=1;
		goto fail;
	}
	SDL_UpdateRects(screen, 1, &dstrect);
	/* screen surface unlock */
 fail:		
	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}
	return ret;
}


int 
jpeg_display_rgb(int argc, char *argv[])
{
	u_int8_t       *rgb_buf;/* RGB data */
	u_int8_t       *jpeg_buf;	/* JPEG data */
	buf_t           jpeg_src_buf;
	size_t          read_size;	/* read size from file */
	int             row_stride;
	int             w, h, d;/* display size and depth(byte) */
	boolean         draw_screen;	/* draw screen surface or another
					 * surface */
	int             screen_status; /* 0=general 1=blue 2=red */
	/* for jpeg library */
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	/* for SDL */
	SDL_Surface    *screen;
	SDL_Surface    *sdl_image = NULL;
//	SDL_Rect        dstrect;

	/* for visualize packet loss */
	SDL_Surface    *sdl_img_blue = NULL;
	SDL_Surface    *sdl_img_red  = NULL;
	u_int8_t        pixel_blue[]={0,0,0xff};
	u_int8_t        pixel_red[]={0xff,0,0};
	u_int32_t       tick_start, tick_now;
	int             buf_status; /* -1=full */

	/* for realtime play */
	struct timeval  tv_start,tv_now;
	int64_t         ts_display,ts_diff;
	u_int32_t       ts_nowblk,ts_lastblk=0;
	
	/* for debug */
	struct timeval  tv_tmp1, tv_tmp2;
	

	/* SDL init */
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		ComplainAndExit();
	}
	atexit(SDL_Quit);

	/* jpeg library init / get image size and depth */
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = my_jpeg_abort_decompress;
	jpeg_create_decompress(&cinfo);

#ifdef USE_JPEG_MEM_SRC
	jpeg_buf = jpeg_mem_src_init(&cinfo, JPEG_BUF_MAX);	/* init */
#else
	jpeg_buf = (u_int8_t *) malloc(JPEG_BUF_MAX);
#endif

	decoder_buf_read();
	read_size = decoder_buf_get(jpeg_buf, JPEG_BUF_MAX,&ts_nowblk);

#ifdef USE_JPEG_MEM_SRC
	jpeg_mem_src(&cinfo, jpeg_buf, read_size);	/* read from memory */
#else
	jpeg_src_buf.buf = jpeg_buf;
	jpeg_buf_src(&cinfo, jpeg_buf, read_size);	/* read from memory */
#endif

	jpeg_read_header(&cinfo, TRUE);
	w = cinfo.image_width;
	h = cinfo.image_height;
	d = cinfo.num_components;
	jpeg_abort_decompress(&cinfo);

	d_printf("\nJPEG info: image=(%d,%d), output=(%d,%d), Bpp=%d\n",
		 cinfo.image_width, cinfo.image_height, w, h, d);

	/* SDL setup / cleanup screen-surface */
	screen = SDL_SetVideoMode(w, h, 0, SDL_HWSURFACE);
	if (screen == NULL) {
		ComplainAndExit();
	}
	if (OPT.loss_visual){
		sdl_img_blue = SDL_CreateRGBSurface(
			SDL_HWSURFACE, w, h, 24
			,OPT.sdl_mask_R, OPT.sdl_mask_G
			,OPT.sdl_mask_B, OPT.sdl_mask_A);
		sdl_img_red = SDL_CreateRGBSurface(
			SDL_HWSURFACE, w, h, 24
			,OPT.sdl_mask_R, OPT.sdl_mask_G
			,OPT.sdl_mask_B, OPT.sdl_mask_A);
		jpeg_fillimg1color(sdl_img_red,pixel_red,3);
		jpeg_fillimg1color(sdl_img_blue,pixel_blue,3);
	}
	if (screen->format->BytesPerPixel == RGB_PIXELSIZE &&
	    screen->format->Rmask == OPT.sdl_mask_R &&
	    screen->format->Gmask == OPT.sdl_mask_G &&
	    screen->format->Bmask == OPT.sdl_mask_B) {
		draw_screen = TRUE;
	} else {
		draw_screen = FALSE;
	}

	d1_printf("\nSDL screen  info: bpp=%d, Bpp=%d, "
		  "R/G/B-mask=%06x/%06x/%06x, Direct=%s",
		screen->format->BitsPerPixel, screen->format->BytesPerPixel,
		  screen->format->Rmask, screen->format->Gmask,
		  screen->format->Bmask,
		  draw_screen ? "ON" : "OFF");

	if (draw_screen==TRUE) {
		/* RGB_PIXELSIZE is defined in jmorecfg.h */
		row_stride = screen->pitch;
	        rgb_buf = (u_int8_t *) screen->pixels;
	} else {
		sdl_image = SDL_CreateRGBSurface(
						 SDL_HWSURFACE, w, h, 24,	/* depth (bit per pixel) */
					     OPT.sdl_mask_R, OPT.sdl_mask_G,
					    OPT.sdl_mask_B, OPT.sdl_mask_A);
		row_stride = sdl_image->pitch;
		d1_printf("\nSDL surface info: bpp=%d, Bpp=%d, "
			  "R/G/B-mask=%06x/%06x/%06x\n",
			  sdl_image->format->BitsPerPixel,
			  sdl_image->format->BytesPerPixel,
			  sdl_image->format->Rmask,
			  sdl_image->format->Gmask,
			  sdl_image->format->Bmask
			);
		rgb_buf = (u_int8_t *) sdl_image->pixels;
	}

	STAT.skip_count = 0;
	STAT.wait_count = 0;

	tick_start = SDL_GetTicks();	/* get start time */
	gettimeofday(&STAT.start, NULL);
	gettimeofday(&tv_start, NULL);
	ts_display=0;

	STAT.f_sigint = 0;
	signal(SIGINT, sigint_trap);



	jpeg_fillimg1color(screen,pixel_blue,3);
	screen_status=1;

	for (STAT.frame_count = 1;; STAT.frame_count++) {
		if (STAT.f_sigint)
			sigint_quit();

		jpeg_has_error = 0;	/* reset flag */
		buf_status=decoder_buf_read();
#if 0
		read_size = decoder_buf_get(jpeg_buf, JPEG_BUF_MAX);
#ifdef REALTIME_PLAY
		tick_now = SDL_GetTicks();
		if (tick_now - tick_start > STAT.frame_count * OPT.mspf) {
			/* skip frame because it's too late */
			STAT.skip_count++;
			continue;
		}
#endif
#else
		read_size = decoder_buf_get(jpeg_buf, JPEG_BUF_MAX,&ts_nowblk);
#ifdef REALTIME_PLAY
		if(buf_status==-1){
			/* need more cpu power */
			if(OPT.loss_visual){
				jpeg_blitimg2screen(sdl_img_red,screen);
				screen_status=2;
			}
			d1_printf("INFO:buffer skip\n");
			STAT.skip_count+=decoder_buf_get_datanum()/2;
			STAT.frame_count+=decoder_buf_get_datanum()/2;
			decoder_buf_rm(decoder_buf_get_datanum()/2);
			ts_display=0;
			gettimeofday(&tv_start, NULL);
			continue;
		}
#endif
#endif
		if (read_size == 0) {	/* buffer empty */
			d1_printf("INFO:buffer empty\n");
			STAT.frame_count--;	/* no skip, no display */
			if(OPT.loss_visual && screen_status!=1){
			  jpeg_blitimg2screen(sdl_img_blue,screen);
			  screen_status=1;
			}
			decoder_buf_prebuf();
			ts_display=0;
			gettimeofday(&tv_start, NULL);
			continue;
		} else if (read_size == -2) {
		    break;	/* end of all files */
		}
#ifdef USE_JPEG_MEM_SRC
		jpeg_mem_src(&cinfo, jpeg_buf, read_size);
#else
		jpeg_src_buf.buf = jpeg_buf;
		jpeg_buf_src(&cinfo, jpeg_buf, read_size);
#endif
		jpeg_read_header(&cinfo, TRUE);

		cinfo.output_width = w;
		cinfo.output_height = h;
		cinfo.out_color_space = JCS_RGB;	/* default */
		cinfo.output_components = d;
		/* more fast decompression */
		cinfo.dct_method = JDCT_FASTEST;
		//cinfo.dct_method = JDCT_FLOAT;
		cinfo.do_fancy_upsampling = FALSE;

		jpeg_start_decompress(&cinfo);

		/* screen surface lock */

		if (SDL_MUSTLOCK(screen)) {
			if (SDL_LockSurface(screen) < 0) {
				ComplainAndExit();
			}
		}
		/* JPEG decode start */
		gettimeofday(&tv_tmp1, NULL);
		while (cinfo.output_scanline < cinfo.output_height &&
		       !jpeg_has_error) {
			JSAMPLE        *rgb_scanline;
			rgb_scanline = &(rgb_buf[cinfo.output_scanline *
						 row_stride]);
			jpeg_read_scanlines(&cinfo, &rgb_scanline, 1);
		}
		/* screen surface unlock */
		if (SDL_MUSTLOCK(screen)) {
			SDL_UnlockSurface(screen);
		}
		jpeg_finish_decompress(&cinfo);
		/* JPEG decode finish */
		gettimeofday(&tv_tmp2, NULL);
		STAT.decode_usec += timeval_diff_usec(&tv_tmp2, &tv_tmp1);
#ifdef REALTIME_PLAY
		wait4rtdisplay(&tv_start,ts_display);

		if(ts_display==0){
		  ts_diff=OPT.freq;
		}else{
		  ts_diff=(u_int32_t)(ts_nowblk-ts_lastblk);
		}
		ts_lastblk=ts_nowblk;

		if(ts_diff<OPT.freq){
	 	  e_printf("INFO:reset TS interval\n");
		}
		if(ts_diff>OPT.freq){
		  d2_printf("blue back start for %u TS\n",(u_int32_t)ts_diff-OPT.freq);
		  ts_display+=(ts_diff-OPT.freq);
		  if(OPT.loss_visual && screen_status!=1){
  		    jpeg_blitimg2screen(sdl_img_blue,screen);
		    screen_status=1;
		  }
		  wait4rtdisplay(&tv_start,ts_display);
		  d2_printf("blue back end.\n");
		}else{
		  d3_printf("no blue back .\n");
		}
		ts_display+= OPT.freq;
#endif
		if (draw_screen==TRUE) {
			SDL_UpdateRect(screen, 0, 0, 0, 0);
			screen_status=0;
		} else {
			jpeg_blitimg2screen(sdl_image,screen);
			screen_status=0;
#if 0
			dstrect.x = 0;
			dstrect.y = 0;
			dstrect.w = sdl_image->w;
			dstrect.h = sdl_image->h;
			if (SDL_BlitSurface(sdl_image, NULL, screen, &dstrect) < 0) {
				SDL_FreeSurface(sdl_image);
				ComplainAndExit();
			}
#if 0
			if (SDL_MUSTLOCK(screen)) {
				SDL_UnlockSurface(screen);
			}
#endif
			SDL_UpdateRects(screen, 1, &dstrect);
#endif 
		}
	}			/* loop for all jpeg file */
	STAT.frame_count--;	/* because count from 1 */
	tick_now = SDL_GetTicks();

	if (!draw_screen) {
		SDL_FreeSurface(sdl_image);
	}
	jpeg_destroy_decompress(&cinfo);
	statistics_print(&STAT);
	return 0;
}

#define STDIN 0

int 
main(int argc, char *argv[])
{
	int buf_num;

	if (opt_etc(argc, argv, &OPT) == -1) {
		return -1;	/* quit */
	}
	buf_num=(OPT.frame_rate*OPT.delay_limit)/1000;
	if(buf_num<=2)
		buf_num=2;
	decoder_buf_init(STDIN,OPT.dsize,buf_num);
	decoder_buf_prebuf();

	switch (OPT.dformat) {
	case DF_YUV:
		d1_printf("YUV format");
#ifdef USE_YUV_OVERLAY
		jpeg_display_yuv(argc, argv);
#else
		fprintf(stderr, "cannot support YUV output format\n");
#endif
		break;
	case DF_RGB:
		d1_printf("RGB format");
		OPT.sdl_mask_R = 0xff0000;
		OPT.sdl_mask_G = 0x00ff00;
		OPT.sdl_mask_B = 0x0000ff;
		OPT.sdl_mask_A = 0;
		jpeg_display_rgb(argc, argv);
		break;
	case DF_BGR:
		d1_printf("BGR format");
		OPT.sdl_mask_R = 0x0000ff;
		OPT.sdl_mask_G = 0x00ff00;
		OPT.sdl_mask_B = 0xff0000;
		OPT.sdl_mask_A = 0;
		jpeg_display_rgb(argc, argv);
		break;
	default:
		break;
	}
	fprintf(stdout, "\n");
	return 0;
}



static int
opt_etc(int argc, char *argv[], opt_t *opt)
{
	int             ch;
	int             loop_flag = 1;
	char           *temp;
	char           *opts = "r:nD:d:g:T:bs:W:";
	char           *helpmes =
	"usage : %s [-r <framerate>] [-D <debug level>]"
	" [-d <YUV|RGB|GRB>] [-n]  [-T <window title>]"
	" [-g <xsize>x<ysize> (not supported)] [-b]"
	"\n"
	" -n : no display (decode only)\n"
	" -b : if frame loss,fill screen with blue [require -d BRG]\n"
	" -s <n>   : max frame size\n"
	" -W <n>   : max delay # msec\n"
		;
	int             w, h;

	opt->clock = DEF_EMON_CLOCK;
	opt->freq = DEF_EMON_FREQ;
	opt->f_disp = 1;
	opt->exec_opt_p = 0;
	opt->output_width = 0;
	opt->output_height = 0;
	opt->loss_visual = 0;
#if 0
	opt->dformat = DF_YUV;
#else
	opt->dformat = DF_BGR;
#endif
	opt->dsize   =DEF_JPEGPLAY_DSIZE;
	opt->delay_limit=DEF_JPEGPLAY_DELAY_LIMIT;
	strncpy(opt->title, "jpegplay", sizeof(opt->title) - 1);

	if ((temp = getenv("EMON_CLOCK")) != NULL)
		opt->clock = atoi(temp);
	if ((temp = getenv("EMON_FREQ")) != NULL)
		opt->freq = atoi(temp);
	if ((temp = getenv("JPEGPLAY_DSIZE")) != NULL)
		opt->dsize = atoi(temp);
	if ((temp = getenv("JPEGPLAY_DELAY_LIMIT")) != NULL)
		opt->delay_limit = atoi(temp);

	while ((ch = getopt(argc, argv, opts)) != -1 && loop_flag == 1) {
		switch (ch) {
		case 'r':
			opt->frame_rate = atoi(optarg);
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		case 'd':	/* display format */
			if (strcasecmp(optarg, "YUV") == 0) {
				opt->dformat = DF_YUV;
			} else if (strcasecmp(optarg, "RGB") == 0) {
				opt->dformat = DF_RGB;
			} else if (strcasecmp(optarg, "BGR") == 0) {
				opt->dformat = DF_BGR;
			} else {
				opt->dformat = DF_YUV;
				fprintf(stderr, "invalid option in -d option");
			}
			break;
		case 'g':	/* geometry () */
			sscanf(optarg, "%dx%d", &w, &h);
			if (!((0 < w && w <= 1280) && (0 < h && h <= 1024))) {
				break;
			}
			opt->output_width = w;
			opt->output_height = h;
			break;
		case 'n':
			opt->f_disp = 0;
			break;
		case 'b':
			opt->loss_visual = 1;
			break;
		case 's':
			opt->dsize = atoi(optarg);
			break;
		case 'W':
			opt->delay_limit = atoi(optarg);
			break;
		case 'T':	/* Window Title */
			strncpy(opt->title, optarg, sizeof(opt->title) - 1);
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
	}

	if (opt->freq < 0){
		e_printf("freq (-F or EMON_FREQ) value must be over 0\n");
		return -1;
	}
	opt->frame_rate = opt->clock / opt->freq;

	if (opt->frame_rate == 0) {
		opt->frame_rate=30;
		opt->mspf_org = 1000.0 / 30;
	} else {
		opt->mspf_org = 1000.0 / opt->frame_rate;
	}
	opt->mspf = opt->mspf_org;
	return 0;
}






void
sigint_trap(int sig)
{
	STAT.f_sigint = 1;
	pipe_exit_req();
}


void
sigint_quit(void)
{
	int             pid = getpid();

	printf("\n--- jpegplay (%5d) caught SIGINT ---", pid);
	statistics_print(&STAT);
	printf("\n---\n");

	exit(-1);
}

void
statistics_print(stat_t * st)
{
	struct timeval  now;
	int             diff_msec;
	int             disp_count = st->frame_count - st->skip_count;

	gettimeofday(&now, NULL);
	diff_msec = (now.tv_sec - st->start.tv_sec) * 1000;
	diff_msec += (now.tv_usec - st->start.tv_usec) / 1000;

	fprintf(stdout,
		"\n"
		" display:  %d (skip: %d, wait: %d)\n"
		" framerate: %.2f f/s, (include skip %.2f f/s)\n"
		" decode: %d us/f",
		disp_count,
		st->skip_count,
		st->wait_count,
	    ((float) (st->frame_count - st->skip_count) * 1000) / diff_msec,
		((float)st->frame_count * 1000) / diff_msec,
		st->decode_usec / disp_count
		);
}
