#ifndef VECTOR_CAL
#define VECTOR_CAL


#include <vector>


using namespace std;

double vsum(vector<double>& v) {
	int n = v.size();
	double _sum = 0;
	for (int i = 0; i < n; i++) {
		_sum += v[i];
	}
	return _sum;
}

double vavg(vector<double>& v) {
	int n = v.size();
	double _sum = vsum(v);
	return _sum / n;
}

double vmin(vector<double>& v) {
	int n = v.size();
	double _min = v[0];
	for (int i = 0; i < n; i++) {
		if (v[i] < _min) {
			_min = v[i];
		}
	}
	return _min;
}

double vmax(vector<double>& v) {
	int n = v.size();
	double _max = v[0];
	for (int i = 0; i < n; i++) {
		if (v[i] > _max) {
			_max = v[i];
		}
	}
	return _max;
}

int vargmin(vector<double>& v) {
	int n = v.size();
	double _min = vmin(v);
	int arg = 0;
	for (int i = 0; i < n; i++) {
		if (v[i] == _min) {
			arg = i;
			break;
		}
	}
	return arg;
}

int vargmax(vector<double>& v) {
	int n = v.size();
	double _max = vmax(v);
	int arg = 0;
	for (int i = 0; i < n; i++) {
		if (v[i] == _max) {
			arg = i;
			break;
		}
	}
	return arg;
}


#endif