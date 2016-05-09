#include "v4l2grab.hpp"
#include "pantilt.hpp"

void help() {
	puts("Usage: grabtest <left|right> steps length");
}

bool selectOrange(int width, int height, unsigned char *yuv) {
	auto orangeCount = 0;
	for (int x = 0; x < width; x++)
	for (int y = 0; y < height; y++)
	{
		double Y = (double) (*yuv++);
		double Cb = (double) (*yuv++);
		double Cr = (double) (*yuv++);

		int r = (int) (Y + 1.40200 * (Cr - 0x80));
		int g = (int) (Y - 0.34414 * (Cb - 0x80) - 0.71414 * (Cr - 0x80));
		int b = (int) (Y + 1.77200 * (Cb - 0x80));
		
		if ((r > 180) && (r > g) && ((r-g) > 40) && (g > b)) {
			orangeCount++;
		}
 	}
	 return (orangeCount * 100.0 / (width * height)) > 1.0; // more than 1% orange
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
	gr.setPreprocessor(selectOrange);
	// gr.setOutput(false);
	char fn[100];
	for (int step = 0; step < steps; step++) {
		sprintf(fn, "step%02d.jpg", step);
		// sprintf(fn, "step%02d.yuv444", step);
		if (right) 
			pt.right(t);
		else
			pt.left(t);
		gr.Capture(fn);
	}
	gr.Close();
	return 0;
}
