/* a simple capture utility for the MotionEye in the Sony Picturebook 

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

int debug=0;
int verbose=0;


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

static void sigint(void)
{
	spic_shutdown(0);
	_exit(1);
}

static void usage(void)
{
	printf("
VAIO PCG-C1XS capture program
Copyright 2000 tridge@linuxcare.com
For the latest version see http://samba.org/picturebook/

capture <options>
  -o outfile       set output filename
  -P               capture as a PPM file
  -C               continuous capture mode to a PPM 
  -V captime       capture an avi video for the specified number of seconds
  -M nframes       capture frames to separate jpeg files
  -O               turn off camera afterwards
  -4               use 1:4 sub-sampling
  -s               go into \"snap\" mode
  -r rate          set framerate in frames per second
  -j               display input from jogger/buttons etc
  --brightness=n   set brightness
  --contrast=n     set contrast
  --hue=n          set hue
  --color=n        set color
  --sharpness=n    set sharpness
  --agc=n          set agc
  --picture=n      set picture
  --explode=fname  explode a AVI file into frame.*
  -h               show usage

");

}

enum {OPT_VERSION, OPT_BRIGHTNESS, OPT_CONTRAST, OPT_HUE, OPT_COLOR, 
      OPT_SHARPNESS, OPT_PICTURE, OPT_AGC, OPT_EXPLODE};

static char *short_options = "to:hPCV:4OM:dvr:sj";

static struct option long_options[] = {
  {"help",        0,     0,    'h'},
  {"version",     0,     0,    OPT_VERSION},
  {"brightness",  1,     0,    OPT_BRIGHTNESS},
  {"contrast",    1,     0,    OPT_CONTRAST},
  {"hue",         1,     0,    OPT_HUE},
  {"color",       1,     0,    OPT_COLOR},
  {"sharpness",   1,     0,    OPT_SHARPNESS},
  {"picture",     1,     0,    OPT_PICTURE},
  {"agc",         1,     0,    OPT_AGC},
  {"explode",     1,     0,    OPT_EXPLODE},
  {0, 0, 0, 0}};

int main(int argc, char *argv[])
{
	char *outfile = NULL;
	char *defout = "cap.jpg";
	int c;
	extern char *optarg;
	int ppm = 0;
	int video = 0;
	int continuous = 0;
	int mjpeg = 0;
	int turnoff = 0;
	double captime=0;
	int numframes=0;
	int snap=0;
	int subsample=0;
	int test=0;
	int option_index;
	int spic_brightness=32, spic_contrast=32, spic_hue=32, spic_color=32, spic_sharpness=32;
	int spic_agc=48, spic_picture=0;
	double framerate=0;

	setlinebuf(stdout);

	while ((c = getopt_long(argc, argv, 
				short_options, long_options, &option_index)) != -1) {
		switch (c) {
		default:
		case OPT_VERSION:
			printf("Capture version %s\nCopyright 2000 tridge@linuxcare.com\n\n", CAPTURE_VERSION);
			exit(0);

		case OPT_BRIGHTNESS:
			spic_brightness = strtol(optarg, NULL, 0);
			break;

		case OPT_CONTRAST:
			spic_contrast = strtol(optarg, NULL, 0);
			break;

		case OPT_HUE:
			spic_hue = strtol(optarg, NULL, 0);
			break;

		case OPT_COLOR:
			spic_color = strtol(optarg, NULL, 0);
			break;

		case OPT_SHARPNESS:
			spic_sharpness = strtol(optarg, NULL, 0);
			break;

		case OPT_PICTURE:
			spic_picture = strtol(optarg, NULL, 0);
			break;

		case OPT_AGC:
			spic_agc = strtol(optarg, NULL, 0);
			break;

		case OPT_EXPLODE:
			lose_privileges();
			avi_explode(optarg);
			exit(0);
			break;

		case 'd':
			debug=1;
			break;

		case 's':
			snap=1;
			break;

		case '4':
			subsample = 1;
			break;

		case 'O':
			turnoff = 1;
			break;

		case 'h':
			usage();
			exit(0);

		case 'o':
			outfile = optarg;
			break;

		case 'P':
			defout = "cap.ppm";
			ppm = 1;
			break;

		case 'C':
			defout = "cap.ppm";
			continuous = 1;
			break;

		case 'V':
			defout = "cap.avi";
			captime = atof(optarg);
			video = 1;
			break;

		case 'r':
			framerate = atof(optarg);
			break;

		case 'v':
			verbose=1;
			break;

		case 'M':
			defout = "cap.jpg.";
			mjpeg = 1;
			numframes = strtol(optarg, NULL, 0);
			break;

		case 't':
			defout = "cap.ppm";
			test=1;
			break;
		case 'j':
			spic_init();
			spic_jogger();
			break;
		}
		
	}

	if (!outfile) outfile = defout;

	spic_init();
	spic_camera_on();
	mchip_init();

	signal(SIGINT, sigint);

	/* just in case someone is silly enough to run it setuid */
	lose_privileges();

	spic_settings(spic_brightness, spic_contrast, spic_hue, spic_color, 
		      spic_sharpness, spic_picture, spic_agc);

	if (verbose) {
		spic_show_settings();
	}

	mchip_subsample(subsample);

	if (test) {
		cmd_test(outfile);
	} else if (continuous) {
		cmd_continuous_out(outfile, framerate);
	} else if (snap) {
		cmd_snap(framerate);
	} else if (video) {
		cmd_avi_capture(outfile, captime, framerate);
	} else if (mjpeg) {
		cmd_mjpeg_capture(outfile, numframes, framerate);
	} else if (ppm) {
		cmd_take_picture(outfile);
	} else {
		cmd_jpeg_capture(outfile);
	}

	mchip_shutdown();
	spic_shutdown(turnoff);

        return 0;
}
