#pragma once

#include <vector>
#include <string>
#include "complex_num.h"

namespace mathengine {

// Descriptive stats
double stat_mean(const std::vector<double>& data);
double stat_median(std::vector<double> data);
double stat_stddev(const std::vector<double>& data);
double stat_variance(const std::vector<double>& data);
double stat_min(const std::vector<double>& data);
double stat_max(const std::vector<double>& data);

// Distributions
double normal_pdf(double x, double mu, double sigma);
double normal_cdf(double x, double mu, double sigma);

// FFT (Cooley-Tukey, power-of-2 sizes)
std::vector<Complex> fft(const std::vector<Complex>& x);
std::vector<Complex> ifft(const std::vector<Complex>& x);

// Correlation & regression
double correlation(const std::vector<double>& x, const std::vector<double>& y);
std::pair<double, double> linear_regression(const std::vector<double>& x, const std::vector<double>& y);

} // namespace mathengine
