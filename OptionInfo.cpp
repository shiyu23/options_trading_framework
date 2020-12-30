#include "OptionInfo.h"
#include "BSAnalytics.h"
#include "cnorm.h"
#include "dnorm.h"
#include <cassert>
#include <functional>
#include <Windows.h>


double rfbisect(std::function<double(double)> f, double a, double b, double tol)
{
	assert(a < b && f(a) * f(b) < 0);
	double c = 0;
	while ((b - a) / 2 > tol) {
		c = (a + b) / 2.0;
		if (abs(f(c)) < tol)
			return c;
		else {
			if (f(a) * f(c) < 0)
				b = c;
			else
				a = c;
		}
	}
	return c;
}

bool RootBracketing(std::function<double(double)> f, double& a, double& b)
{
	const int NTRY = 50;
	const double FACTOR = 1.6;
	if (a >= b) throw("wrong input a and b in RootBracketing");
	double f1 = f(a);
	double f2 = f(b);
	for (int j = 0; j < NTRY; j++) {
		if (f1 * f2 < 0.0) return true;
		if (abs(f1) < abs(f2))
			f1 = f(a += FACTOR * (a - b));
		else
			f2 = f(b += FACTOR * (b - a));
	}
	return false;
}

double OptionInfo::iv() {
    auto fun = [=](double vol) {return bsPricer(oty, K, T, S, vol) - midbidaskspread(); }; 
	double a = 0.0001, b = 3;
	RootBracketing(fun, a, b);
	return rfbisect(fun, a, b, 1e-6);
}

double OptionInfo::iv_s(double s) {
	auto fun = [=](double vol) {return bsPricer(oty, K, T, s, vol) - midbidaskspread(); };
	double a = 0.0001, b = 3;
	RootBracketing(fun, a, b);
	return rfbisect(fun, a, b, 1e-6);
}

double OptionInfo::iv_p(double p) {
	auto fun = [=](double vol) {return bsPricer(oty, K, T, S, vol) - p; };
	double a = 0.0001, b = 3;
	RootBracketing(fun, a, b);
	return rfbisect(fun, a, b, 1e-6);
}

double OptionInfo::midbidaskspread()
{
	return (ask + bid) / 2;
}

double OptionInfo::delta() {
    double sigma = iv();
    if (oty == OptionType::C)
        return cnorm((std::log(S / K) + 0) / (sigma * std::sqrt(T)) + 0.5 * sigma * std::sqrt(T));
    else
        return -cnorm((std::log(S / K) + 0) / (sigma * std::sqrt(T)) + 0.5 * sigma * std::sqrt(T));
}

double OptionInfo::vega() {
    double sigma = iv();
    return S * std::sqrt(T) * dnorm((std::log(S / K) + 0) / (sigma * std::sqrt(T)) + 0.5 * sigma * std::sqrt(T));
}

double OptionInfo::gamma() {
	double sigma = iv();
	return dnorm(std::log(S / K) / (sigma * std::sqrt(T)) + 0.5 * sigma * std::sqrt(T)) / S / sigma / std::sqrt(T);
}

double OptionInfo::theta() {
	double sigma = iv();
	return -S * dnorm(std::log(S / K) / (sigma * std::sqrt(T)) + 0.5 * sigma * std::sqrt(T)) * sigma / 2 / std::sqrt(T);
}