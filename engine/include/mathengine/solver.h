#pragma once

#include "expr.h"
#include "eval.h"
#include <string>

namespace mathengine {

struct RootResult {
    double root;
    int iterations;
    bool converged;
};

// Bisection method on compiled expression
RootResult bisect(const CompiledExpr& f, double a, double b, double tol = 1e-12, int max_iter = 1000);

// Newton's method using symbolic derivative
RootResult newton(const Expr::Ptr& f, const std::string& var, double x0, double tol = 1e-12, int max_iter = 100);

// Brent's method for minimization
double minimize_brent(const CompiledExpr& f, double a, double b, double tol = 1e-8);

} // namespace mathengine
