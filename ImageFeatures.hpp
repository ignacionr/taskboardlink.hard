class ImageFeatures : public std::vector<std::pair<int,int>> {
	#define THRESHOLD	20
public:
	ImageFeatures(int width, int height, unsigned char *data) {
		auto up = *data;
		for (int y = 2; y < height-1; y++) {
			auto left = data[(y * width) * 3];
			for (int x = 2; x < width-1; x++) {
				// given a certain point, use it as a feature if its brightness differs sensibly from right or up
				auto point = data[(x + y * width) * 3];
				if (abs(left-point) > THRESHOLD) {
					push_back(std::make_pair(x,y));
				}
				else if (abs(point - data[(x + (y-1) * width) * 3])) {
					push_back(std::make_pair(x,y));
				}
				left = point;
			}
		}
	}
};
