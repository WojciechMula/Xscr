/*
	$Date: 2008-06-21 18:27:18 $, $Revision: 1.4 $
	
	Simple direct-screen abstraction for X Window [header].
	
	Author: Wojciech Mu³a
	e-mail: wojciech_mula@poczta.onet.pl
	www:    http://www.republika.pl/wmula/
	
	License: public domain
*/
#ifndef __XSCR_H_INCLUDED__
#define __XSCR_H_INCLUDED__

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdint.h>


/*************************************************************************/
// Enums

// supported depths
typedef enum {
	DEPTH_15bpp = 15,
	DEPTH_16bpp = 16,
	DEPTH_32bpp = 24,
	DEPTH_gray  = 8
} BitsPerPixel;

// state of key or mouse button
typedef enum {Pressed, Released} KeyOrButtonState;


/*************************************************************************/
// Callbacks passed to Xscr_mainloop

typedef void (*Xscr_motion_callback)(
	int x,		// mouse position
	int y,
	Time time,	// time
	unsigned int kb_mask // certain bits indicate state of special
	                     // keys/modifiers (use ShiftMask, LockMask etc.)
);

typedef void (*Xscr_keyboard_callback)(
	int x, int y, Time time,
	KeySym c,			// key symbol
	KeyOrButtonState s,	// key has been pressed/released
	unsigned int kb_mask
);

typedef void (*Xscr_buttons_callback)(
	int x, int y, Time time,
	unsigned int button,	// button number (0 - LBM, 2 - RMB)
	KeyOrButtonState s,		// button has been pressed/released
	unsigned int kb_mask
);



/*************************************************************************/
// Public functions

// Function runs main loop, that process some events send by server.
// If user pass callbacks (keyboard, motion, buttons -- not all are
// required) then events are dispatched to them.
//
// Only one Xscr_mainloop may run within a program.
// 
// Parameters:
//    width, height - dimension of virtual screen
//                    width MUST be multiply of 32
//    screen_depth  - depth user screen; X may run with different
//                    depth, and then automatic conversion are done
//                    (if supported)
//    require_exact_depth - if user depth doesn't match X depth, then
//                    error is reported
//    screen_data   - screen data defined (and later modified) by user
//
//    Note: callbacks are not mandatory, pass NULL if you don't
//          need certain callback
//
//    keyboard_callback - callback called on key events
//    motion_callback   - callback called on pointer motion events
//    buttons_callback  - callback called on mouse button events
//
//    app_name - application name, displayed on status bar by window
//               manager; if app_name is NULL, then default name
//               is used
//
// Result:
//    equal 0      - success
//    less then 0  - error, use -result to get error message
//                   from Xscr_errormsg array
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
);

// These function should be called inside callbacks

// stop Xscr_mainloop
void Xscr_quit();

// Inform that user_data has been changed and thus
// window should be refreshed.
void Xscr_redraw();

// Like Xscr_redraw, but forces X11 to refresh
// window instantly
void Xscr_redraw_now();

// Remove all events from X11 event queue.
void Xscr_discard_events();


// Error messages
#define Xscr_maxerror 8
static char* Xscr_errormsg[Xscr_maxerror] = {
	/* 0 */ "no error",
	/* 1 */ "can't open display (is $DISPLAY set?)",
	/* 2 */ "screen depth is different then required depth",
	/* 3 */ "unsupported screen depth conversions",
	/* 4 */ "not enough memory for backbuffer",
	/* 5 */ "can't create Ximage",
	/* 6 */ "Xscr_mainloop is already running",
	/* 7 */ "width isn't multiply of 32",
};

// vim: ts=4 sw=4 nowrap noexpandtab
#endif
