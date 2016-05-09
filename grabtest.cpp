#include "v4l2grab.hpp"
#include "pantilt.hpp"

#include <iostream>

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
		
		if (((r-g) > 30) && (g > b) && ((g-b) > 30)) {
			orangeCount++;
		}
 	}
	 auto factor = (orangeCount * 100.0 / (width * height));
	 // std::cout << "orange: " << orangeCount << ", " << factor << "%" << std::endl;
	 return factor > 3.0;
}

#define X_STEP 30000
#define Y_STEP 30000

int main(int argc, char *argv[]) {
	
	// if (argc != 4)
	// {
	// 	help();
	// 	return 1;
	// }
	
	PanTilt pt;
	Grabber gr;
	gr.Open();
	pt.zero();
	gr.setPreprocessor(selectOrange);
	gr.setOutput(false,false);
	
	// move to the left until we find orange
	bool found = false;
	int step;
	for (step = 0; !found; step++) {
		pt.left(X_STEP);
		found = gr.Capture(NULL);
	}
	auto left = step;
	// follow up and down to get those boundaries
	for(step = 0; found; step++) {
		pt.up(Y_STEP);
		found = gr.Capture(NULL);
	}
	pt.down(Y_STEP);
	for(step = 0; found; step++) {
		pt.down(Y_STEP);
		found = gr.Capture(NULL);
	}
	auto height = step;
	
	std::cout << "left: " << left << " height: " << height << std::endl;
	
	gr.Close();
	return 0;
}
