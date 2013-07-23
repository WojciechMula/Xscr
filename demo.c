#include <stdio.h>
#include <stdlib.h>
#include "Xscr.h"

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

int main(int argc, char* argv[]) {
	int result;
	
	int width, height, x, y;
	uint8_t *data;
	uint8_t *img_row;
	uint8_t *src, *dst;
	FILE* file;

	if (argc < 4) {
		printf("usage: %s width height ppmfile\n", argv[0]);
		return 1;
	}

	width  = atoi(argv[1]);
	if (width % 32) {
		printf("Image width must be multiply of 32\n");
		return 1;
	}
	height = atoi(argv[2]);
	file = fopen(argv[3], "rb");
	if (!file) {
		printf("Can't open '%s'\n", argv[4]);
		return 1;
	}
	data    = (uint8_t*)malloc(width*height*4);
	img_row = (uint8_t*)malloc(width*3);
	if (!data || !img_row) {
		printf("Not enough memory\n");
		return 1;
	}

	fseek(file, -width*height*3, SEEK_END);

	dst = data;
	for (y=0; y < height; y++) {
		if (y == 0)
		fread(img_row, width, 3, file);
		src = img_row;
		for (x=0; x < width; x++) {
			*dst++ = *(src+2);
			*dst++ = *(src+1);
			*dst++ = *(src+0);
			*dst++ = 0x00;
			src += 3;
		}
	}

	fclose(file);
		

	// run mainloop
	result = Xscr_mainloop(
		640, 480, DEPTH_32bpp, False, data, 
		keyboard, motion, buttons,
		"Xscr demo"
	);

	// check exit status
	if (result < 0) {
		printf("Xscr error: %s\n", Xscr_errormsg[-result]);
	}

	free(data);
	free(img_row);
	return 0;
}

// vim: ts=4 sw=4 nowrap
