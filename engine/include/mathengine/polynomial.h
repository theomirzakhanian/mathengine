#pragma once

#include "expr.h"
#include <vector>
#include <optional>
#include <utility>

namespace mathengine {

class Polynomial {
public:
    std::vector<double> coeffs; // coeffs[i] = coefficient of x^i

    Polynomial() : coeffs({0.0}) {}
    explicit Polynomial(std::vector<double> c) : coeffs(std::move(c)) { normalize(); }

    int degree() const;
    double coeff(int deg) const;
    double eval(double x) const; // Horner's method

    Polynomial operator+(const Polynomial& o) const;
    Polynomial operator-(const Polynomial& o) const;
    Polynomial operator*(const Polynomial& o) const;
    Polynomial operator*(double scalar) const;
    Polynomial operator-() const;

    // Euclidean division: {quotient, remainder}
    std::pair<Polynomial, Polynomial> divmod(const Polynomial& divisor) const;

    // Convert to Expr AST
    Expr::Ptr to_expr(const std::string& var) const;

    // Strip trailing near-zero coefficients
    void normalize();

    // Make leading coefficient 1
    Polynomial monic() const;
};

// Try to convert an Expr to a Polynomial in var.
// Returns nullopt if expression is not polynomial in var.
std::optional<Polynomial> expr_to_poly(const Expr::Ptr& expr, const std::string& var);

// GCD of two polynomials (Euclidean algorithm)
Polynomial poly_gcd(const Polynomial& a, const Polynomial& b);

} // namespace mathengine
