#include "mathengine/complex_num.h"
#include <sstream>
#include <iomanip>

namespace mathengine {

std::string Complex::to_string() const {
    std::ostringstream ss;
    if (std::abs(im) < 1e-12) {
        ss << re;
    } else if (std::abs(re) < 1e-12) {
        ss << im << "i";
    } else {
        ss << re;
        if (im > 0) ss << " + " << im << "i";
        else ss << " - " << (-im) << "i";
    }
    return ss.str();
}

Complex cexp(const Complex& z) {
    double r = std::exp(z.re);
    return {r * std::cos(z.im), r * std::sin(z.im)};
}

Complex clog(const Complex& z) {
    return {std::log(z.abs()), z.arg()};
}

Complex csin(const Complex& z) {
    return {std::sin(z.re) * std::cosh(z.im), std::cos(z.re) * std::sinh(z.im)};
}

Complex ccos(const Complex& z) {
    return {std::cos(z.re) * std::cosh(z.im), -std::sin(z.re) * std::sinh(z.im)};
}

Complex cpow(const Complex& base, const Complex& exp) {
    if (base.re == 0 && base.im == 0) return {0, 0};
    return cexp(exp * clog(base));
}

Complex csqrt(const Complex& z) {
    double r = std::sqrt(z.abs());
    double theta = z.arg() / 2.0;
    return {r * std::cos(theta), r * std::sin(theta)};
}

// Durand-Kerner method for all complex roots
std::vector<Complex> complex_roots(const std::vector<double>& coeffs) {
    int n = (int)coeffs.size() - 1;
    if (n <= 0) return {};

    // Normalize to monic
    double lead = coeffs[n];
    std::vector<double> c(n + 1);
    for (int i = 0; i <= n; i++) c[i] = coeffs[i] / lead;

    // Initial guesses spread on a circle
    std::vector<Complex> roots(n);
    for (int i = 0; i < n; i++) {
        double angle = 2.0 * M_PI * i / n + 0.1;
        double r = 1.0 + std::abs(c[0]);
        roots[i] = {r * std::cos(angle), r * std::sin(angle)};
    }

    // Evaluate polynomial at z
    auto eval_poly = [&](const Complex& z) -> Complex {
        Complex result = {c[n], 0};
        for (int i = n - 1; i >= 0; i--)
            result = result * z + Complex(c[i], 0);
        return result;
    };

    // Iterate
    for (int iter = 0; iter < 1000; iter++) {
        double max_change = 0;
        for (int i = 0; i < n; i++) {
            Complex num = eval_poly(roots[i]);
            Complex den = {1, 0};
            for (int j = 0; j < n; j++) {
                if (j != i) den = den * (roots[i] - roots[j]);
            }
            if (den.abs() < 1e-30) continue;
            Complex delta = num / den;
            roots[i] = roots[i] - delta;
            max_change = std::max(max_change, delta.abs());
        }
        if (max_change < 1e-14) break;
    }

    return roots;
}

} // namespace mathengine
