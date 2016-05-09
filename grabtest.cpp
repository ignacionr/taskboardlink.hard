#include "v4l2grab.hpp"
#include "pantilt.hpp"

void help() {
	puts("Usage: grabtest <left|right> steps length");
}

void turnOrangeIntoBlack(int width, int height, unsigned char *yuv) {
	for (int x = 0; x < width; x++)
	for (int y = 0; y < height; y++)
	{
		double Y = (double) (*yuv++);
		double Cb = (double) (*yuv++);
		double Cr = (double) (*yuv++);

		int r = (int) (Y + 1.40200 * (Cr - 0x80));
		int g = (int) (Y - 0.34414 * (Cb - 0x80) - 0.71414 * (Cr - 0x80));
		int b = (int) (Y + 1.77200 * (Cb - 0x80));
		
		if (r > 10 && b < r && abs(r-g) < 20) {
			*(yuv-3) = 0;
			*(yuv-2) = 0;
			*(yuv-1) = 0;
		}
 	}
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
	gr.setPreprocessor(turnOrangeIntoBlack);
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
