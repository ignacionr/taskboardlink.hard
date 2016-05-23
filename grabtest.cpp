#include "v4l2grab.hpp"
#include "pantilt.hpp"

#include <iostream>

#define X_STEP 35000
#define Y_STEP 70000
#define WIDTH	100
#define HEIGHT	22

int main(int argc, char *argv[]) {
	PanTilt pt(X_STEP,Y_STEP);
	if (argc == 2) {
		if (0 == strcmp(argv[1], "up"))
			pt.up();
		else if (0 == strcmp(argv[1], "down"))
			pt.down();
		else if (0 == strcmp(argv[1], "left"))
			pt.left();
		else if (0 == strcmp(argv[1], "right"))
			pt.right();
		else if (0 == strcmp(argv[1], "zero"))
			pt.zero();
		else
			puts("I didn't get that.");
		return 0;
	}
	
	Grabber gr;
	gr.Open();
	pt.zero();
	pt.setX(WIDTH/2);
	pt.setY(HEIGHT);
	
	char fname[50];
	for(int y = 0; y < HEIGHT; y++) {
		pt.tiltTo(y);
		for(int x = 0; x < WIDTH; x++)
		{
			int rx = y%2 ? WIDTH - x : x;
			pt.panTo(rx);
			sprintf(fname, "pic_%03d_%03d.jpg", y, rx);
			gr.Capture(fname);
			std::cout <<(fname) << std::endl;
		}	
	}
	
	gr.Close();
	return 0;
}
