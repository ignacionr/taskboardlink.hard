class ImageFeatures : public std::vector<std::pair<int,int>> {
	int	_threshold;
	#define MAX_FEATURES	10000
	#define	GRAIN			5
public:
	ImageFeatures(int width, int height, unsigned char *data) {
		bool tested = false;
		for(_threshold = 10; size() >= MAX_FEATURES || !tested; _threshold++) {
			clear();
			int up = *data;
			for (int y = 2; y < height-1 && size() < MAX_FEATURES; y+= GRAIN) {
				int left = data[(1 + y * width) * 3];
				for (int x = 2; x < width-1 && size() < MAX_FEATURES; x+= GRAIN) {
					// given a certain point, use it as a feature if its brightness differs sensibly from right or up
					int point = data[(x + y * width) * 3];
					if (abs(left-point) > _threshold) {
						push_back(std::make_pair(x,y));
					}
					else if (abs(point - data[(x + (y-1) * width) * 3]) > _threshold) {
						push_back(std::make_pair(x,y));
					}
					left = point;
				}
			}
			tested = true;
		}
		for(auto feature_point: *this) {
			unsigned char *p = data + (feature_point.first + feature_point.second * width) * 3;
			*p++ = 255;
			*p++ = 255;
			*p = 0;
			p = data + (feature_point.first - 1 + feature_point.second * width) * 3;
			*p++ = 255;
			*p++ = 255;
			*p = 0;
			p = data + (feature_point.first + (feature_point.second-1) * width) * 3;
			*p++ = 255;
			*p++ = 255;
			*p = 0;
			p = data + (feature_point.first - 1 + (feature_point.second-1) * width) * 3;
			*p++ = 255;
			*p++ = 255;
			*p = 0;
		}
	}
};
