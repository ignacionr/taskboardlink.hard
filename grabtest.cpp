#include "v4l2grab.hpp"
#include "pantilt.hpp"

void help() {
	puts("Usage: grabtest <left|right> steps length");
}

int main(int argc, char *argv[]) {
	
	if (argc != 4)
	{
		help();
		return 1;
	}
	
	auto right = strcmp("left", argv[1]) != 0;
	auto steps = atol(argv[2]);
	auto t = atol(argv[3]);
	
	PanTilt pt;
	Grabber gr;
	gr.Open();
	char fn[100];
	for(int step = 0; step < steps; step++) {
		sprintf(fn, "step%02d.jpg", step);
		if (right) 
			pt.right(t);
		else
			pt.left(t);
		gr.Capture(fn);
	}
	gr.Close();
	return 0;
}
