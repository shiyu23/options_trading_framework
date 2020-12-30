#ifndef DNORM
#define DNORM


#include <cmath>


double dnorm(double x) {
	const double pi = 3.141592653589793;
	return 1/(std::sqrt(2*pi)) * std::exp(-x*x/2);
}


#endif