#define X_STEP 35000
#define Y_STEP 70000

class Cam {
	PanTilt pt;
	Grabber gr;
public:
	Cam(): pt(X_STEP,Y_STEP) {
		gr.Open();
	}
	PanTilt &pantilt() { return pt; }
	Grabber &grabber() { return gr; }
};
