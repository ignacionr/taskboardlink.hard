#include "v4l2grab.hpp"

int main(int argc, char *argv[]) {
	Grabber gr;
	gr.Open();
	gr.Capture("test1.jpg");
	sleep(2);
	gr.Capture("test2.jpg");
	gr.Close();
}