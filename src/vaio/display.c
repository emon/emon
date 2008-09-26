#include "capture.h"

#if WITH_DISPLAY
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <Imlib.h>

void display_rgb(u8 *rgb, int width, int height, char *name)
{
	XSetWindowAttributes attr;
	static int initialised;
	static Display *disp;
	static ImlibData *id;
	static Window win;
	static ImlibImage *im;

	if (!initialised) {
		if (!rgb) return;

		initialised = 1;
		if (!disp) {
			disp=XOpenDisplay(NULL);
		}
		id=Imlib_init(disp);

		win=XCreateWindow(disp,DefaultRootWindow(disp),0,0,
				  width,height,0,id->x.depth,
				  InputOutput,id->x.visual,0,&attr);
		XStoreName(disp, win, name);
		XMapWindow(disp,win);
		XSync(disp,False);
	}

	if (!rgb) {
		XDestroyWindow(disp, win);
		initialised=0;
		XSync(disp,False);
		return;
	}

	im = Imlib_create_image_from_data(id, rgb, NULL, width,height);
	Imlib_paste_image(id,im,win,0,0,im->rgb_width,im->rgb_height);
	Imlib_kill_image(id, im);
	XSync(disp,False);
}

void display_image(char *fname)
{
	XSetWindowAttributes attr;
	int w,h;
	static int initialised;
	static Display *disp;
	static ImlibData *id;
	static Window win;
	static ImlibImage *im;

	if (!initialised) {
		initialised = 1;
		disp=XOpenDisplay(NULL);
		id=Imlib_init(disp);

		im=Imlib_load_image(id,fname);
		w=im->rgb_width;
		h=im->rgb_height;
		win=XCreateWindow(disp,DefaultRootWindow(disp),0,0,w,h,0,id->x.depth,
				  InputOutput,id->x.visual,0,&attr);
		Imlib_kill_image(id, im);
		XMapWindow(disp,win);
		XSync(disp,False);
	}

	XClearWindow(disp, win);
	im=Imlib_load_image(id,fname);
	Imlib_paste_image(id,im,win,0,0,im->rgb_width,im->rgb_height);
	Imlib_kill_image(id, im);
	XStoreName(disp, win, fname);
	XSync(disp,False);
}




#else
void display_image(char *fname)
{
}
#endif
