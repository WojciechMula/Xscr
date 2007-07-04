#include "Xscr.h"
#include <stdlib.h>

// global variables
Display* display;
Window   window;

static Bool running = False;
static Bool quit;
static char redraw;

void Xscr_quit()       {quit = True;}
void Xscr_redraw()     {redraw = 1;}
void Xscr_redraw_now() {redraw = 2;}
void Xscr_discard_events() {XSync(display, True);}

// conversion routines
void Xscr_prepare_lookups();
void Xscr_convert_gray_32bpp(void*, void*, unsigned int, unsigned int);
void Xscr_convert_gray_16bpp(void*, void*, unsigned int, unsigned int);
void Xscr_convert_gray_15bpp(void*, void*, unsigned int, unsigned int);
void Xscr_convert_16bpp_32bpp(void*, void*, unsigned int, unsigned int);
void Xscr_convert_15bpp_32bpp(void*, void*, unsigned int, unsigned int);
void Xscr_convert_32bpp_16bpp(void*, void*, unsigned int, unsigned int);


int Xscr_mainloop(
	unsigned int width,
	unsigned int height,
	BitsPerPixel screen_depth,
	Bool require_exact_depth,
	uint8_t* screen_data,

	Xscr_keyboard_callback	keyboard_callback,
	Xscr_motion_callback	motion_callback,
	Xscr_buttons_callback	buttons_callback,
	const char* app_name
) {
	// X variables
	int      screen;
	GC       defaultGC;
	XEvent	 event;

	// priv variables
	uint8_t* real_screen_data = NULL;
	XImage*  image;
	int      depth;
	XSizeHints size_hints;
	XWMHints   WM_hints;
	void	   (*conv_func)(void*, void*, unsigned int, unsigned int) = NULL;

	// 0. is Xscr_mainloop running 
	if (running)
		return -6;
	else
		running = True;
	
	// 1. is width is multiply of 32
	if (width % 32)
		return -7;
	
	// 2. open display
	display = XOpenDisplay(NULL);
	if (!display) return -1;

	// 3. get screen & depth
	screen = DefaultScreen(display);
	depth  = DefaultDepth(display, screen);

	// 4. setup conversion routins if needed
	if (depth != (int)screen_depth) {
		if (require_exact_depth)
			return -2;

		// select conversion function
		switch ((int)screen_depth) {
			case 8:
				switch (depth) {
					case 24: conv_func = Xscr_convert_gray_32bpp; break;
					case 16: conv_func = Xscr_convert_gray_16bpp; break;
					case 15: conv_func = Xscr_convert_gray_15bpp; break;
					default: return -3;
				}
				break;

			case 16:
				switch (depth) {
					case 24: conv_func = Xscr_convert_16bpp_32bpp; break;
					default: return -3;
				}
				break;

			case 15:
				switch (depth) {
					case 24: conv_func = Xscr_convert_15bpp_32bpp; break;
					default: return -3;
				}
				break;

			case 24:
				switch (depth) {
					case 16: conv_func = Xscr_convert_32bpp_16bpp; break;
					default: return -3;
				}
				break;

			default:
				return -3;
		}

		// allocate mem for backbuffer
		switch (depth) {
			case 24:
				real_screen_data = malloc(width * height * 4);
				break;
			case 15:
			case 16:
				real_screen_data = malloc(width * height * 2);
				break;
			case 8:
				real_screen_data = malloc(width * height);
				break;
		}

		if (real_screen_data == NULL) return -4;
		image = XCreateImage(display, DefaultVisual(display, screen),
			depth, ZPixmap, 0, (char*)real_screen_data,
			width, height, 8, 0);
	
		Xscr_prepare_lookups();
	}
	else {
		puts("dupa");
		image = XCreateImage(display, DefaultVisual(display, screen),
			depth, ZPixmap, 0, (char*)screen_data,
			width, height, 8, 0);
		if (image == NULL)
			return -5;
		}

	// 5. create window
	window = XCreateSimpleWindow( // make a simple window
		display, 
		RootWindow(display, screen),
		0, 0,
		width, height,
		1,
		BlackPixel(display, screen),
		WhitePixel(display, screen)
	);

	// 6. set application name
	if (app_name)
		XStoreName(display, window, app_name);
	else
		XStoreName(display, window, "Xscr application");

	// 7a. set hints
	WM_hints.flags = InputHint | StateHint;
	WM_hints.input = True;
	WM_hints.initial_state = NormalState;
	XSetWMHints(display, window, &WM_hints);

	// 7b. set size hints (inhibit resizing)
	size_hints.flags = PMinSize | PMaxSize | PSize;
	size_hints.width = size_hints.min_width = size_hints.max_width = width;
	size_hints.height = size_hints.min_height = size_hints.max_height = height;
	XSetWMNormalHints(display, window, &size_hints);

	
	// 8. setup graphics context
	defaultGC = XCreateGC(display, window, 0, NULL);
	XSetForeground(display, defaultGC, BlackPixel(display, screen) );
	XSetBackground(display, defaultGC, WhitePixel(display, screen) );
	
	// 9. shot window on the screen
	XMapWindow(display, window);


	// 10. decide what events we should read
	long event_mask = ExposureMask;

	if (motion_callback) 
		event_mask |= PointerMotionMask;
	if (keyboard_callback) {
		event_mask |= KeyPressMask;
		event_mask |= KeyReleaseMask;
	}
	if (buttons_callback) {
		event_mask |= ButtonPressMask;
		event_mask |= ButtonReleaseMask;
	}

	XSelectInput(display, window, event_mask);

	// 11. start reading messages
	quit   = False;
	redraw = 1;

	while (!quit) {
		if (redraw) {
			if (conv_func)
				(*conv_func)((void*)screen_data, (void*)real_screen_data, width, height);
			
			XPutImage(display, window, defaultGC, image, 0, 0, 0, 0, width, height);
			
			if (redraw == 2)
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

	if (real_screen_data != NULL) {
		free(real_screen_data);
	}
	return 0;
}


uint32_t __conv_gray_32bpp[256];
uint16_t __conv_gray_15bpp[256];
uint16_t __conv_gray_16bpp[256];
uint32_t __conv_15bpp_32bpp_lo[256];
uint32_t __conv_15bpp_32bpp_hi[256];
uint32_t __conv_16bpp_32bpp_lo[256];
uint32_t __conv_16bpp_32bpp_hi[256];

void Xscr_prepare_lookups() {
	uint32_t i, i5, i6;

	for (i=0; i < 256; i++) {
		i5 = i >> 3;
		i6 = i >> 2;

		// gray -> direct color
		__conv_gray_32bpp[i] =  (i << 16) |  (i << 8) | i;
		__conv_gray_16bpp[i] = (i5 << 11) | (i6 << 5) | i5;
		__conv_gray_15bpp[i] = (i5 << 10) | (i5 << 5) | i5;


		// 16bpp -> 32bpp
		// |rrrrrggg gggbbbbb|
		
		// |00000000 00000000 000ggg00 bbbbb000|
		__conv_16bpp_32bpp_lo[i] = ((i & 0xe0) << 5) | ((i & 0x1f) << 3);
		
		// |00000000 rrrrr000 ggg00000 00000000|
		__conv_16bpp_32bpp_hi[i] = ((i & 0x07) << 13) | ((i & 0xf8) << 16);
		
		
		// 15bpp -> 32bpp
		// |0rrrrrgg gggbbbbb|
		//
		// |00000000 rrrrr000 ggggg000 bbbbb000|
		
		// |00000000 00000000 000gg000 bbbbb000|
		__conv_15bpp_32bpp_lo[i] = ((i & 0xe0) << 5) | ((i & 0x1f) << 3);
		
		// |00000000 rrrrr000 ggg00000 00000000|
		__conv_15bpp_32bpp_hi[i] = ((i & 0x07) << 13) | ((i & 0x78) << 17);
	}
}



void Xscr_convert_gray_32bpp(
	void *gray,
	void *true_color,
	unsigned int width,
	unsigned int height
) {
	int x, y;
	uint8_t  *src;
	uint32_t *dst;

	src = gray;
	dst = true_color;
	for (y=0; y < height; y++)
		for (x=0; x < width; x++)
			*dst++ = __conv_gray_32bpp[*src++];
}


void Xscr_convert_gray_16bpp(
	void *gray,
	void *true_color,
	unsigned int width,
	unsigned int height
) {
	int x, y;
	uint8_t  *src;
	uint16_t *dst;

	src = gray;
	dst = true_color;
	for (y=0; y < height; y++)
		for (x=0; x < width; x++)
			*dst++ = __conv_gray_16bpp[*src++];
}


void Xscr_convert_gray_15bpp(
	void *gray,
	void *true_color,
	unsigned int width,
	unsigned int height
) {
	int x, y;
	uint8_t  *src;
	uint16_t *dst;

	src = gray;
	dst = true_color;
	for (y=0; y < height; y++)
		for (x=0; x < width; x++)
			*dst++ = __conv_gray_15bpp[*src++];
}


void Xscr_convert_16bpp_32bpp(
	void *pix16bpp,
	void *true_color,
	unsigned int width,
	unsigned int height
) {
	int n;
	uint16_t *src;
	uint32_t *dst;

	src = pix16bpp;
	dst = true_color;
	n   = width * height;
	while (n-- >= 0) {
		*dst++ = __conv_16bpp_32bpp_lo[*src & 0x00ff] |
				 __conv_16bpp_32bpp_hi[*src >> 8];
		src++;
	}
}


void Xscr_convert_15bpp_32bpp(
	void *pix15bpp,
	void *true_color,
	unsigned int width,
	unsigned int height
) {
	int x, y;
	uint16_t  *src;
	uint32_t *dst;

	src = pix15bpp;
	dst = true_color;
	for (y=0; y < height; y++)
		for (x=0; x < width; x++) {
			*dst++ = __conv_15bpp_32bpp_lo[*src & 0x00ff] |
			         __conv_15bpp_32bpp_hi[*src >> 8];
			src++;
		}
}


#ifdef XSCR_X86conv
void Xscr_convert_32bpp_16bpp(
	void *pix32bpp,
	void *pix16bpp,
	unsigned int width,
	unsigned int height
) {
	asm(
		"    xorl %%eax, %%eax                    \n\t"
		"    xorl %%ebx, %%ebx                    \n\t"
		".1:                                      \n\t"
		"    movl 0(%%esi), %%eax                 \n\t"
		"    movl 4(%%esi), %%ebx                 \n\t"
		"    shrb $2, %%ah                        \n\t"
		"    shrb $2, %%bh                        \n\t"

		"    shrl $3, %%eax                       \n\t"
		"    shrl $3, %%ebx                       \n\t"

		"    shlw $5, %%ax                        \n\t"
		"    shlw $5, %%bx                        \n\t"

		"    shrl $5, %%eax                       \n\t"
		"    shll $11, %%ebx                      \n\t"
		"                                         \n\t"
		"    addl $8, %%esi                       \n\t"
		"    orl  %%ebx, %%eax                    \n\t"
		"                                         \n\t"
		"    movl %%eax, (%%edi)                  \n\t"
		"    addl $4, %%edi                       \n\t"
		
		"    subl $1, %%ecx                       \n\t"
		"    jnz  .1                              \n\t"
	: /* no output */
	: "S" (pix32bpp), "D" (pix16bpp), "c" (width/2*height)
	: "%eax", "%ebx", "memory"
	);
	return;
}
#else
void Xscr_convert_32bpp_16bpp(
	void *pix32bpp,
	void *pix16bpp,
	unsigned int width,
	unsigned int height
) {
	int x, y;
	uint8_t *src;
	uint16_t  R, G, B;
	uint16_t *dst;

	src = pix32bpp;
	dst = pix16bpp;

	for (y=0; y < height; y++)
		for (x=0; x < width; x++) {
			B = *src++ >> 3;
			G = (uint16_t)(*src++ & 0xfc) << 3;
			R = (uint16_t)(*src++ & 0xf8) << 8;
			src++;

			*dst++ = R | G | B;
		}
}
#endif

////////////////////////////////////////////////////////////////////////

//#define TEST_XSCR
#ifdef TEST_XSCR
#include <stdio.h>
#include "load_ppm.h"

void motion(
	int x, int y, Time t,
	unsigned int kb_mask
) {
	printf("mouse position: %d, %d\n", x, y);
}

void buttons(
	int x, int y, Time t,
	unsigned int button, 
	KeyOrButtonState s,
	unsigned int kb_mask
) {
	printf("state=%s, button=%d\n",
	        s==Pressed ? "Pressed" : "Released",
	        button
	);
}

void keyboard(
	int x, int y, Time t,
	KeySym c,
	KeyOrButtonState s,
	unsigned int state
) {
	printf("state=%s, key=%s\n",
	       s==Pressed ? "Pressed" : "Released",
	       XKeysymToString(c)
	);
	if (c == XK_q || c == XK_Q)
		Xscr_quit();
}

int main() {
	uint8_t *data;
	int width, height, maxval, result;
	FILE* file;

	file = fopen("a.ppm", "rb");
	if (!file) return 1;
	result = ppm_load_32bpp(file, &width, &height, &maxval, &data, 0);
	printf("PPM: %dx%d, %dbpp\n", width, height, maxval < 256 ? 3*8 : 3*16);
	fclose(file);
	if (result < 0) {
		printf("PPM load error: %s\n", PPM_errormsg[-result]);
		return 1;
	}

	result = Xscr_mainloop(
		640, 480, DEPTH_32bpp, False, data, 
		keyboard, motion, buttons,
		"Xscr demo"
	);
	if (result < 0) {
		printf("Xscr error: %s\n", Xscr_errormsg[-result]);
	}

	free(data);
	return 0;
}
#endif

// vim: ts=4 sw=4 nowrap
