typedef std::vector<int> Column;
typedef std::map<int, Column> Grid;
class ImageFeatures : public Grid {
	int _width;
	int _height;
	int	_threshold;
	int _feature_count;
	
	#define MAX_FEATURES	5000
	#define	GRAIN				1
	#define START_THRESHOLD		5

	void paint(unsigned char *data) {
		for(auto feature_row: *this) {
			for (auto feature_x: feature_row.second) {
				unsigned char *p = data + (feature_x + feature_row.first * _width) * 3;
				*p++ = 255;
				*p++ = 255;
				*p = 0;
				p = data + (feature_x - 1 + feature_row.first * _width) * 3;
				*p++ = 255;
				*p++ = 255;
				*p = 0;
				p = data + (feature_x + (feature_row.first-1) * _width) * 3;
				*p++ = 255;
				*p++ = 255;
				*p = 0;
				p = data + (feature_x - 1 + (feature_row.first-1) * _width) * 3;
				*p++ = 255;
				*p++ = 255;
				*p = 0;
			}
		}
	}

public:
	ImageFeatures() {}
	ImageFeatures(int width, int height, unsigned char *data) {
		_width = width;
		_height = height;
		bool tested = false;
		_feature_count = 0;
		for(_threshold = START_THRESHOLD; _feature_count >= MAX_FEATURES || !tested; _threshold++) {
			clear();
			_feature_count = 0;
			int up = *data;
			for (int y = 2; y < height-1 && _feature_count < MAX_FEATURES; y+= GRAIN) {
				int left = data[(1 + y * width) * 3];
				for (int x = 2; x < width-1 && _feature_count < MAX_FEATURES; x+= GRAIN) {
					// given a certain point, use it as a feature if its brightness differs sensibly from right or up
					int point = data[(x + y * width) * 3];
					if (abs(left-point) > _threshold) {
						(*this)[y].push_back(x);
						_feature_count++;
					}
					else if (abs(point - data[(x + (y-1) * width) * 3]) > _threshold) {
						(*this)[y].push_back(x);
						_feature_count++;
					}
					left = point;
				}
			}
			tested = true;
		}
		paint(data);
	}
	
	bool is_usable() { return _feature_count * 20 > MAX_FEATURES; }
	
	struct correction {
		int	off_x;
		int off_y;
		int votes;
		bool operator>(const correction& other) {
			return votes > other.votes;
		}
		friend std::ostream& operator<<(std::ostream& str, const correction& c) {
			str << "Correction: (" << c.off_x << "," << c.off_y << ") voted with " << c.votes;
			return str;
		}
	};

	static correction suggest_correction(int independent, bool independent_is_y, const Grid& from, const Grid& to) {
		correction result;
		if (independent_is_y)
			result.off_y = independent;
		else
			result.off_x = independent;
		std::map<int,int> votes;
		for(auto feature_row: from) {
			auto pother_row = to.find(feature_row.first + independent);
			if (pother_row != to.end()) {
				for(auto this_dep: feature_row.second) {
					for (auto other_dep: (*pother_row).second) {
						votes[this_dep-other_dep]++;
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
			if (independent_is_y)
				result.off_x = (*pelected).first;
			else
				result.off_y = (*pelected).first;
			result.votes = (*pelected).second;
		}
		return result;
	}	
	
	correction suggest_correction_after_pan(const ImageFeatures& other) const {
		correction candidate;
		for(int y = -20; y <= 20; y++) {
			correction this_y = suggest_correction(y, true, *this, other);
			if (this_y > candidate) {
				candidate = this_y;
			}
		}
		return candidate;
	}
	
	static Grid rotate(const Grid& origin) {
		Grid result;
		for(auto row: origin) {
			for (auto x: row.second) {
				result[x].push_back(row.first);
			}
		}
		return result;
	}

	correction suggest_correction_after_tilt(const ImageFeatures& other) const {
		correction candidate;
		auto from = rotate(*this);
		auto to = rotate(other);
		for(int x = -20; x <= 20; x++) {
			auto this_correction = suggest_correction(x, false, from, to);
			if (this_correction > candidate) {
				candidate = this_correction;
			}
		}
		return candidate;
	}
};
