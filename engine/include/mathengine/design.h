#pragma once

#include "expr.h"
#include "polynomial.h"
#include <vector>
#include <string>

namespace mathengine {

struct Point2D { double x, y; };

// Fit a polynomial to points (least squares)
Polynomial poly_fit(const std::vector<Point2D>& points, int degree);

// Fit a Fourier series to points: a0 + sum(a_k*cos(k*x) + b_k*sin(k*x))
struct FourierFit {
    double a0 = 0;
    double freq = 1.0;         // base frequency (2*pi / data_range)
    std::vector<double> a_cos; // cosine coefficients
    std::vector<double> b_sin; // sine coefficients
    int order = 0;

    double eval(double x) const;
    Expr::Ptr to_expr() const;
    std::string to_desmos() const;
};

FourierFit fourier_fit(const std::vector<Point2D>& points, int order);

// Convert any expression to Desmos-compatible string
std::string to_desmos(const Expr::Ptr& expr);

// Convert polynomial to Desmos string
std::string poly_to_desmos(const Polynomial& p);

// Parametric stroke fitting: fit x(t), y(t) from a sequence of points
// t is normalized to [0, 1] based on cumulative arc length
struct ParametricFit {
    Polynomial px; // x(t) polynomial
    Polynomial py; // y(t) polynomial

    // Evaluate at parameter t (0 to 1)
    Point2D eval(double t) const;

    // Export as Desmos parametric: (px(t), py(t))
    std::string to_desmos() const;
};

ParametricFit parametric_fit(const std::vector<Point2D>& points, int degree);

} // namespace mathengine
