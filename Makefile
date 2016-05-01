.PHONY: demo

demo: demo.c Xscr.c Xscr.h
	gcc -O3 Xscr.c demo.c -lX11 -o $@
