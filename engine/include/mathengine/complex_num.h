#pragma once

#include <string>
#include <cmath>
#include <vector>

namespace mathengine {

struct Complex {
    double re = 0, im = 0;

    Complex() = default;
    Complex(double r) : re(r), im(0) {}
    Complex(double r, double i) : re(r), im(i) {}

    Complex operator+(const Complex& o) const { return {re + o.re, im + o.im}; }
    Complex operator-(const Complex& o) const { return {re - o.re, im - o.im}; }
    Complex operator*(const Complex& o) const { return {re*o.re - im*o.im, re*o.im + im*o.re}; }
    Complex operator/(const Complex& o) const {
        double d = o.re*o.re + o.im*o.im;
        return {(re*o.re + im*o.im)/d, (im*o.re - re*o.im)/d};
    }
    Complex operator-() const { return {-re, -im}; }

    double abs() const { return std::sqrt(re*re + im*im); }
    double arg() const { return std::atan2(im, re); }
    Complex conj() const { return {re, -im}; }

    std::string to_string() const;
};

Complex cexp(const Complex& z);
Complex clog(const Complex& z);
Complex csin(const Complex& z);
Complex ccos(const Complex& z);
Complex cpow(const Complex& base, const Complex& exp);
Complex csqrt(const Complex& z);

// Solve polynomial with complex roots (Durand-Kerner method)
std::vector<Complex> complex_roots(const std::vector<double>& coeffs);

} // namespace mathengine
