LIBS = `pkg-config --libs gtk+-3.0`

CFLAGS = `pkg-config --cflags gtk+-3.0`

all: tu77 demo

tu77: tu77.c
	gcc -o tu77 tu77.c $(LIBS) $(CFLAGS)

demo: demo.c
	gcc -o demo demo.c
