#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdint.h>

#define true	1
#define false	(!True)

typedef enum {DEPTH_15bpp=15, DEPTH_16bpp=16, DEPTH_32bpp=32, DEPTH_gray=8} BitsPerPixel;

typedef Bool bool;

typedef enum {Pressed, Released} State;

typedef void (*Xscr_motion_callback)(
	int x,
	int y,
	Time time,
	unsigned int kb_mask
);

typedef void (*Xscr_keyboard_callback)(
	int x, int y, Time time,
	KeySym c,
	State s,
	unsigned int kb_mask
);

typedef void (*Xscr_buttons_callback)(
	int x, int y, Time time,
	unsigned int button,
	State s,
	unsigned int kb_mask
);

static bool quit   = false;
static bool redraw = true;

void Xscr_quit()   {quit = true;}
void Xscr_redraw() {redraw = true;}

char* Xscr_errormsg[] = {
	/* 0 */ "no error",
	/* 1 */ "cant't open display (is $DISPLAY set?)",
	/* 2 */ "",
	/* 3 */ "",
	/* 4 */ "",
	/* 5 */ "",
	/* 6 */ "",
	/* 7 */ "",
	/* 8 */ "",
	/* 9 */ "",
	/* 10 */ "",
	/* 11 */ "",
	/* 12 */ "",
	/* 13 */ ""
};


int Xscr_mainloop(
	unsigned int width,
	unsigned int height,
	BitsPerPixel screen_depth,
	uint8_t* screen_data,
	bool require_exact_depth,

	Xscr_keyboard_callback	keyboard_callback,
	Xscr_motion_callback	motion_callback,
	Xscr_buttons_callback	buttons_callback,
	const char* app_name
) {
	// X variables
	Display* display;
	Window   window;
	int      screen;
	GC       defaultGC;
	XEvent	 event;


	// priv variables
	uint8_t* real_screen_data = NULL;
	XImage*  image;

	
	// open display
	display = XOpenDisplay(NULL);
	if (!display) return -1;

	// get screen
	screen = DefaultScreen(display);

	// create window
	window = XCreateSimpleWindow( // make a simple window
		display, 
		RootWindow(display, screen),
		0, 0,
		width, height,
		1,
		BlackPixel(display, screen),
		WhitePixel(display, screen)
	);

	if (app_name)
		XStoreName(display, window, app_name);
	else
		XStoreName(display, window, "Xscr application");

	XMapWindow(display, window); // show window it on the screen

	// setup graphics context
	defaultGC = XCreateGC(display, window, 0, NULL);
	XSetForeground(display, defaultGC, BlackPixel(display, screen) );
	XSetBackground(display, defaultGC, WhitePixel(display, screen) );
		
	image = XCreateImage(display, DefaultVisual(display, screen),
		16, ZPixmap, 0, (char*)screen_data,
		width, height, 16, 0);
	
	// start reading messages
	long event_mask = ExposureMask;

	if (keyboard_callback) {
		event_mask |= KeyPressMask;
		event_mask |= KeyReleaseMask;
	}
	if (motion_callback) 
		event_mask |= PointerMotionMask;
	if (buttons_callback) {
		event_mask |= ButtonPressMask;
		event_mask |= ButtonReleaseMask;
	}

	XSelectInput(display, window, event_mask);

	quit   = false;
	redraw = false;
	while (!quit) {
		if (redraw) {
			XPutImage(display, window, defaultGC, image, 0, 0, 0, 0, width, height);
			XFlush(display);
			redraw = 0;
		}

		XNextEvent(display, &event);
		switch (event.type) {
			case Expose:
				XPutImage(display, window, defaultGC, image,
				          event.xexpose.x, event.xexpose.y,
				          event.xexpose.x, event.xexpose.y,
				          event.xexpose.width, event.xexpose.height
				);
				break;
			case MotionNotify:
				motion_callback(
					event.xmotion.x,
					event.xmotion.y,
					event.xmotion.time,
					event.xmotion.state
				);
				break;
			case KeyPress:
				keyboard_callback(
					event.xkey.x,
					event.xkey.y,
					event.xkey.time,
					XKeycodeToKeysym(display, event.xkey.keycode, 0),
					Pressed,
					event.xkey.state
				);
				break;
			case KeyRelease:
				keyboard_callback(
					event.xkey.x,
					event.xkey.y,
					event.xkey.time,
					XKeycodeToKeysym(display, event.xkey.keycode, 0),
					Released,
					event.xkey.state
				);
				break;
			case ButtonPress:
				buttons_callback(
					event.xbutton.x,
					event.xbutton.y,
					event.xbutton.time,
					Pressed,
					event.xbutton.button,
					event.xbutton.state
				);
				break;
			case ButtonRelease:
				buttons_callback(
					event.xbutton.x,
					event.xbutton.y,
					event.xbutton.time,
					Released,
					event.xbutton.button,
					event.xbutton.state
				);
				break;
			default:
				break;
		}
	}
	XCloseDisplay(display);
	return 0;
}



#define TEST_XSCR
#ifdef TEST_XSCR
#include <stdio.h>

void motion(int x, int y) {
	printf("mouse position: %d, %d\n", x, y);
}

void buttons(int x, int y, unsigned int button, State s) {
	printf("state=%s, button=%d\n", s==Pressed ? "Pressed" : "Released", button);
}

void keyboard(int x, int y, KeySym c, State s) {
	printf("state=%s, key=%s\n", s==Pressed ? "Pressed" : "Released", XKeysymToString(c));
}

int main() {
	uint8_t *data;
	uint16_t *pix, R, G, B;
	uint8_t row[640*3];
	int x, y;
	FILE* file;

	file = fopen("a.ppm", "rb");
	if (!file) return 1;
	data = pix = malloc(640*480*2);

	fseek(file, -640*480*3, SEEK_END);
	for (y=0; y < 480; y++) {
		fread(row, 640, 3, file);
		for (x=0; x < 640*3; x += 3) {
			R = row[x+0] >> 3;
			G = row[x+1] >> 2;
			B = row[x+2] >> 3;
			*pix++ = (R << 11) | (G << 5) | B;
		}
	}
	fclose(file);

	Xscr_mainloop(
		640, 480, DEPTH_32bpp, data, false, 
		//NULL, motion, buttons
		NULL, NULL, NULL,
		"Xscr demo"
	);
	free(data);
}
#endif

// vim: ts=4 sw=4 nowrap
