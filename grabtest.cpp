#include "v4l2grab.hpp"
#include "pantilt.hpp"

int main(int argc, char *argv[]) {
	PanTilt pt;	
	Grabber gr;
	gr.Open();
	gr.Capture("test1.jpg");
	pt.up(20000);
	gr.Capture("test2.jpg");
	gr.Close();
}