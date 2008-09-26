/* capture commands

   Tridge, July 2000
*/
/* 
   Copyright (C) Andrew Tridgell 2000
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "capture.h"


static void save_ppm(u8 *rgb, int xsize, int ysize, char *fname)
{
	FILE *f = fopen(fname,"w");
	if (!f) {
		perror(fname);
		return;
	}
	fprintf(f,"P6\n%d %d\n255\n", xsize, ysize);
	fwrite(rgb, 3, xsize*ysize, f);
	fclose(f);

	printf("%s saved\n", fname);
}


static int write_file(char *fname, u8 *img, int size)
{
	int fd = open(fname,O_WRONLY|O_CREAT,0644);
	if (fd == -1) return -1;

	write(fd, img, size);
	close(fd);
	return 0;
}

static char *find_next_file(char *template)
{
	static char f[100];
	int i;
	for (i=0; ; i++) {
		struct stat st;
		snprintf(f, sizeof(f)-1, template, i);
		if (stat(f, &st) == -1 && errno == ENOENT) return f;
	}
	return NULL;
}

static void frame_delay(double framerate, double t)
{
	#define SLEEP_RESOLUTION (2.0/100)
	if (framerate <= 0.0) return;

	while (timer_end()-t < 1.0/framerate - SLEEP_RESOLUTION) sdelay(100);
}

void cmd_take_picture(char *fname)
{
	u8 buf[640*480*2];
	u8 rgb[640*480*3];
	mchip_take_picture();
	mchip_get_picture(buf, mchip_hsize()*mchip_vsize()*2);
	yuv_convert(buf, rgb, mchip_hsize(), mchip_vsize());
	save_ppm(rgb, mchip_hsize(), mchip_vsize(), fname);
}

void cmd_continuous_out(char *fname, double framerate)
{
	u8 buf[640*480*2];
	u8 rgb[640*480*3];

	mchip_set_framerate(framerate);

	mchip_continuous_start();
	while (1) {
		timer_start();
		mchip_continuous_read(buf, mchip_hsize()*mchip_vsize()*2);
		yuv_convert(buf, rgb, mchip_hsize(), mchip_vsize());
		save_ppm(rgb, mchip_hsize(), mchip_vsize(), fname);
		display_image(fname);
		frame_delay(framerate, 0);
	}
}


/* compress a captured image as a jpeg */
void cmd_jpeg_capture(char *fname)
{
	int n, fd;
	u8 buf[0x40000];

	mchip_take_picture();
	n = mchip_compress_frame(buf, sizeof(buf));

	/* save the resulting jpeg */
	fd = open(fname,O_WRONLY|O_CREAT,0644);
	if (fd == -1) {
		perror(fname);
		return;
	}
	write(fd, buf, n);
	close(fd);

	printf("wrote jpeg %s of size %d\n", fname, n);
}


void cmd_avi_capture(char *fname, double captime, double framerate)
{
	int fd;
	u8 buf[0x40000];
	int n, i;

	fd = open(fname,O_WRONLY|O_CREAT,0644);

	avi_start(fd);

	printf("capturing %3.1f seconds to %s\n", captime, fname);

	mchip_set_framerate(framerate);

	timer_start();
	mchip_cont_compression_start();

	for (i=0; timer_end() < captime; i++) {
		double t1 = timer_end();
		n = mchip_cont_compression_read(buf, sizeof(buf));
		avi_add(fd, buf, n);
		printf("."); fflush(stdout); 
		frame_delay(framerate, t1);
	}
	avi_end(fd, mchip_hsize(), mchip_vsize(), i/timer_end());
	printf("\ncaptured %d frames\n", i);
	close(fd);
}


void cmd_mjpeg_capture(char *fname, int numframes, double framerate)
{
	int fd;
	u8 buf[0x40000];
	int n, i;
	char f2[200];

	mchip_set_framerate(framerate);

	printf("capturing %d frames to frame %sN\n", numframes, fname);

	for (i=0; i<numframes; i++) {
		timer_start();
		mchip_take_picture();
		n = mchip_compress_frame(buf, sizeof(buf));
		snprintf(f2, sizeof(f2)-1, "%s%d", fname, i);
		f2[sizeof(f2)-1]=0;
		fd = open(f2, O_WRONLY|O_CREAT, 0644);
		write(fd, buf, n);
		close(fd);
		printf("."); fflush(stdout);
		frame_delay(framerate, 0);
	}
	printf("\ncaptured %d frames\n", i);
}


static void snap_capture(int mode)
{
	int n, frames;
	u8 img[100*1024];

	if (mode < 2) {
		char *f;

		mchip_take_picture();
		n = mchip_compress_frame(img, sizeof(img));
		
		f = find_next_file("snap.%d.jpg");
		write_file(f, img, n);
		display_image(f);
		printf("captured to %s\n", f);
		while (spic_capture_pressed()) sdelay(1);
	} else {
		char *f;
		int fd;
		
		f = find_next_file("snap.%d.avi");
		
		fd = open(f,O_WRONLY|O_CREAT,0644);
		mchip_cont_compression_start();
		avi_start(fd);
		frames=0;
		do {
			n = mchip_cont_compression_read(img, sizeof(img));
			if (n > 0) avi_add(fd, img, n);
			frames++;
			printf("."); fflush(stdout);
		} while (spic_capture_pressed());
		avi_end(fd, mchip_hsize(), mchip_vsize(), 
			frames/timer_end());
		close(fd);
		printf("\ncaptured %d frames to %s\n", frames, f);
	}
}


void cmd_snap(double framerate)
{
	int frames;
	u8 buf[640*480*2];
	u8 rgb[640*480*3];
	int mode = 1;

	mchip_subsample(mode & 1);
	mchip_set_framerate(framerate);

	mchip_continuous_start();

	printf("
started snap mode
 press capture button to take photo
 rotate jogger to change mode
 press jogger to exit
\n");

	sdelay(100);
	timer_start();

	frames = 0;

	while (1) {
		double t1 = timer_end();
		mchip_continuous_read(buf, mchip_hsize()*mchip_vsize()*2);
		yuv_convert(buf, rgb, mchip_hsize(), mchip_vsize());
		display_rgb(rgb, mchip_hsize(), mchip_vsize(), "ViewFinder");

		if (spic_jogger_pressed()) {
			printf("snap finished\n");
			break;
		}

		if (spic_jogger_turned()) {
			char *modes[] = {"JPEG large", "JPEG small", 
					 "AVI large", "AVI small"};
			mode = (mode+1)%4;
			mchip_hic_stop();
			mchip_subsample(mode & 1);
			printf("mode set to %s capture\n", modes[mode]);
			mchip_continuous_start();
			display_rgb(NULL, 0, 0, NULL);
		}

		if (spic_capture_pressed()) {
			snap_capture(mode);
			mchip_continuous_start();
		}

		frame_delay(framerate, t1);
		frames++;
		if (timer_end() > 5) {
			printf("fps=%g\n", frames/timer_end());
			frames=0;
			timer_start();
		}
	}
}

	

void cmd_test(char *fname)
{
	int n;
	u8 img[100*1024];

	while (1) {
		n = mchip_cont_compression_read(img, sizeof(img));
		printf("n=%d\n", n);
	}
}

	
