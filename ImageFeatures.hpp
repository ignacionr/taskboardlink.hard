typedef std::vector<int> Column;

class ImageFeatures : public std::map<int, Column> {
	int	_threshold;
	#define MAX_FEATURES	10000
	#define	GRAIN				5
	#define START_THRESHOLD		5

	void paint(int width, unsigned char *data) {
		for(auto feature_row: *this) {
			for (auto feature_x: feature_row.second) {
				unsigned char *p = data + (feature_x + feature_row.first * width) * 3;
				*p++ = 255;
				*p++ = 255;
				*p = 0;
				p = data + (feature_x - 1 + feature_row.first * width) * 3;
				*p++ = 255;
				*p++ = 255;
				*p = 0;
				p = data + (feature_x + (feature_row.first-1) * width) * 3;
				*p++ = 255;
				*p++ = 255;
				*p = 0;
				p = data + (feature_x - 1 + (feature_row.first-1) * width) * 3;
				*p++ = 255;
				*p++ = 255;
				*p = 0;
			}
		}
	}

public:
	ImageFeatures() {}
	ImageFeatures(int width, int height, unsigned char *data) {
		bool tested = false;
		int total_count = 0;
		for(_threshold = START_THRESHOLD; total_count >= MAX_FEATURES || !tested; _threshold++) {
			clear();
			total_count = 0;
			int up = *data;
			for (int y = 2; y < height-1 && total_count < MAX_FEATURES; y+= GRAIN) {
				int left = data[(1 + y * width) * 3];
				for (int x = 2; x < width-1 && total_count < MAX_FEATURES; x+= GRAIN) {
					// given a certain point, use it as a feature if its brightness differs sensibly from right or up
					int point = data[(x + y * width) * 3];
					if (abs(left-point) > _threshold) {
						(*this)[y].push_back(x);
						total_count++;
					}
					else if (abs(point - data[(x + (y-1) * width) * 3]) > _threshold) {
						(*this)[y].push_back(x);
						total_count++;
					}
					left = point;
				}
			}
			tested = true;
		}
		paint(width, data);
	}
	
	struct correction {
		int	off_x;
		int off_y;
		int votes;
		bool operator>(const correction& other) {
			return votes > other.votes;
		}
	};

	correction suggest_correction_after_pan(const ImageFeatures& other, int y) const {
		correction result;
		result.off_y = y;
		std::map<int,int> votes;
		for(auto feature_row: *this) {
			auto pother_row = other.find(feature_row.first + y);
			if (pother_row != other.end()) {
				for(auto this_x: feature_row.second) {
					for (auto other_x: (*pother_row).second) {
						votes[this_x-other_x]++;
					}
				}
			}
		}
		auto pelected = std::max_element(votes.begin(), votes.end(), 
			[] (const std::pair<int,int>&left, const std::pair<int,int>&right) {
				return left.second < right.second;
			});
		if (pelected == votes.end()) {
			result.votes = 0;
		}
		else {
			result.off_x = (*pelected).first;
			result.votes = (*pelected).second;
		}
		return result;
	}
	
	correction suggest_correction_after_pan(const ImageFeatures& other) const {
		correction candidate;
		for(int y = -20; y <= 20; y++) {
			correction this_y = suggest_correction_after_pan(other, y);
			if (this_y > candidate) {
				candidate = this_y;
			}
		}
		return candidate;
	}	
};
