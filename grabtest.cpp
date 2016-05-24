#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <functional>
#include <vector>
#include <algorithm>

#define WIDTH	100
#define HEIGHT	22

#include "v4l2grab.hpp"
#include "pantilt.hpp"
#include "mongoose.h"

#include "Cam.hpp"
#include "ImageFeatures.hpp"

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

std::map<std::string, std::function<void()>> cam_moves;
PanTilt* _pantilt;
Grabber* _grabber;

void report_status(struct mg_connection *nc) {
	std::stringstream msgs;
	msgs << "{ x: " << _pantilt->x() << ", y: " << _pantilt->y() << " }";
	auto msg = msgs.str();
	mg_send_head(nc, 200, msg.size(), NULL);
	mg_send(nc, msg.c_str(), msg.size());
}

void ev_handler(struct mg_connection *nc, int ev, void *p) {
	auto hm = (struct http_message *) p;
	if (ev == MG_EV_HTTP_REQUEST) {
		std::string uri(hm->uri.p, hm->uri.len);
		auto move_op = cam_moves.find(uri);
		if (move_op != cam_moves.end()) {
			(*move_op).second();
			report_status(nc);
		}
		// else if (mg_vcmp(&hm->uri, "/current.jpg") == 0) {
		// 	_grabber->Capture("./web/current.jpg");
		// 	mg_serve_http(nc, hm, s_http_server_opts);
		// }
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
	
	Cam cam;
	ImageFeatures current_features;
	_grabber = &cam.grabber();
	
	cam_moves["/up"] 	= [&] { cam.pantilt().up(); };
	cam_moves["/down"] 	= [&] { cam.pantilt().down(); };
	cam_moves["/left"] 	= [&] {
		// take a picture
		cam.grabber().Capture("./web/current.jpg");
		// save the features
		ImageFeatures left = current_features;
		// move to the left
		cam.pantilt().left();
		cam.grabber().Capture("./web/current.jpg");
		// determine the correction to the current features
		auto correction = left.suggest_correction_after_pan(current_features);
		std::cout << correction << std::endl;
		// set the correction as coordinates (for now, just to show)
		cam.pantilt().setX(correction.off_x);
		cam.pantilt().setY(correction.off_y);
	};
	cam_moves["/right"] = [&] { cam.pantilt().right(); };
	cam_moves["/zero"] 	= [&] { cam.pantilt().zero(); };
	
	_grabber->setPreprocessor( [&] (int width, int height, unsigned char *data) {
		ImageFeatures features(width,height,data);
		current_features = features;
		return true;
	});
	_pantilt = &cam.pantilt();

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
