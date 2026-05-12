#include "mathengine/statistics.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace mathengine {

double stat_mean(const std::vector<double>& d) {
    if (d.empty()) return 0;
    return std::accumulate(d.begin(), d.end(), 0.0) / d.size();
}

double stat_median(std::vector<double> d) {
    if (d.empty()) return 0;
    std::sort(d.begin(), d.end());
    size_t n = d.size();
    return (n % 2) ? d[n/2] : (d[n/2-1] + d[n/2]) / 2.0;
}

double stat_variance(const std::vector<double>& d) {
    if (d.size() < 2) return 0;
    double m = stat_mean(d);
    double sum = 0;
    for (double x : d) sum += (x - m) * (x - m);
    return sum / (d.size() - 1);
}

double stat_stddev(const std::vector<double>& d) { return std::sqrt(stat_variance(d)); }
double stat_min(const std::vector<double>& d) { return *std::min_element(d.begin(), d.end()); }
double stat_max(const std::vector<double>& d) { return *std::max_element(d.begin(), d.end()); }

double normal_pdf(double x, double mu, double sigma) {
    double z = (x - mu) / sigma;
    return std::exp(-0.5 * z * z) / (sigma * std::sqrt(2.0 * M_PI));
}

double normal_cdf(double x, double mu, double sigma) {
    return 0.5 * std::erfc(-(x - mu) / (sigma * std::sqrt(2.0)));
}

// Cooley-Tukey FFT
std::vector<Complex> fft(const std::vector<Complex>& x) {
    size_t n = x.size();
    if (n <= 1) return x;

    // Pad to power of 2
    size_t m = 1;
    while (m < n) m <<= 1;
    std::vector<Complex> a(m);
    for (size_t i = 0; i < n; i++) a[i] = x[i];

    // Bit-reversal permutation
    for (size_t i = 1, j = 0; i < m; i++) {
        size_t bit = m >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }

    // Butterfly
    for (size_t len = 2; len <= m; len <<= 1) {
        double angle = -2.0 * M_PI / len;
        Complex wlen(std::cos(angle), std::sin(angle));
        for (size_t i = 0; i < m; i += len) {
            Complex w(1, 0);
            for (size_t j = 0; j < len / 2; j++) {
                Complex u = a[i + j];
                Complex v = a[i + j + len/2] * w;
                a[i + j] = u + v;
                a[i + j + len/2] = u - v;
                w = w * wlen;
            }
        }
    }
    return a;
}

std::vector<Complex> ifft(const std::vector<Complex>& x) {
    auto result = x;
    for (auto& c : result) c.im = -c.im;
    result = fft(result);
    double n = result.size();
    for (auto& c : result) { c.re /= n; c.im = -c.im / n; }
    return result;
}

double correlation(const std::vector<double>& x, const std::vector<double>& y) {
    size_t n = std::min(x.size(), y.size());
    double mx = stat_mean(x), my = stat_mean(y);
    double num = 0, dx = 0, dy = 0;
    for (size_t i = 0; i < n; i++) {
        num += (x[i] - mx) * (y[i] - my);
        dx += (x[i] - mx) * (x[i] - mx);
        dy += (y[i] - my) * (y[i] - my);
    }
    if (dx < 1e-15 || dy < 1e-15) return 0;
    return num / std::sqrt(dx * dy);
}

std::pair<double, double> linear_regression(const std::vector<double>& x, const std::vector<double>& y) {
    size_t n = std::min(x.size(), y.size());
    double mx = stat_mean(x), my = stat_mean(y);
    double num = 0, den = 0;
    for (size_t i = 0; i < n; i++) {
        num += (x[i] - mx) * (y[i] - my);
        den += (x[i] - mx) * (x[i] - mx);
    }
    double slope = (den > 1e-15) ? num / den : 0;
    double intercept = my - slope * mx;
    return {slope, intercept};
}

} // namespace mathengine
