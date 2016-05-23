#include <iostream>
#include <fstream>

#define WIDTH	100
#define HEIGHT	22

#include "v4l2grab.hpp"
#include "pantilt.hpp"
#include "mongoose.h"

#include "Cam.hpp"

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

Cam cam;
std::string ok_response("OK");

void ev_handler(struct mg_connection *nc, int ev, void *p) {
	auto hm = (struct http_message *) p;
	if (ev == MG_EV_HTTP_REQUEST) {
		if (mg_vcmp(&hm->uri, "/current.jpg") == 0) {
			cam.grabber().Capture("./web/current.jpg");
			mg_serve_http(nc, hm, s_http_server_opts);
		}
		else if (mg_vcmp(&hm->uri, "/up") == 0) {
			cam.pantilt().up();
			mg_send_head(nc, 200, ok_response.size(), NULL);
			mg_send(nc, ok_response.c_str(), ok_response.size());
		}
		else if (mg_vcmp(&hm->uri, "/down") == 0) {
			cam.pantilt().down();
			mg_send_head(nc, 200, ok_response.size(), NULL);
			mg_send(nc, ok_response.c_str(), ok_response.size());
		}
		else if (mg_vcmp(&hm->uri, "/left") == 0) {
			cam.pantilt().left();
			mg_send_head(nc, 200, ok_response.size(), NULL);
			mg_send(nc, ok_response.c_str(), ok_response.size());
		}
		else if (mg_vcmp(&hm->uri, "/right") == 0) {
			cam.pantilt().right();
			mg_send_head(nc, 200, ok_response.size(), NULL);
			mg_send(nc, ok_response.c_str(), ok_response.size());
		}
		else {
			mg_serve_http(nc, hm, s_http_server_opts);
		}
	}
}

int main(int argc, char *argv[]) {
	struct mg_mgr mgr;
	struct mg_connection *nc;

	mg_mgr_init(&mgr, NULL);
	nc = mg_bind(&mgr, s_http_port, ev_handler);

	// Set up HTTP server parameters
	mg_set_protocol_http_websocket(nc);
	s_http_server_opts.document_root = "./web";  // Serve current directory

	printf("Starting web server on port %s\n", s_http_port);
	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);

	return 0;
}	

void grab_rectangle(PanTilt &pt, Grabber &gr) {
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
}
