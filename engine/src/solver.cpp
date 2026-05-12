#include "mathengine/solver.h"
#include "mathengine/calculus.h"
#include <cmath>
#include <stdexcept>

namespace mathengine {

RootResult bisect(const CompiledExpr& f, double a, double b, double tol, int max_iter) {
    double fa = f.eval(a), fb = f.eval(b);
    if (fa * fb > 0)
        throw std::runtime_error("bisect: f(a) and f(b) must have opposite signs");

    int iter = 0;
    double mid = a;
    while ((b - a) > tol && iter < max_iter) {
        mid = (a + b) / 2.0;
        double fm = f.eval(mid);
        if (fm == 0.0) break;
        if (fa * fm < 0) { b = mid; fb = fm; }
        else { a = mid; fa = fm; }
        iter++;
    }
    return {(a + b) / 2.0, iter, (b - a) <= tol};
}

RootResult newton(const Expr::Ptr& f, const std::string& var, double x0, double tol, int max_iter) {
    auto df = differentiate(f, var);
    CompiledExpr cf(f), cdf(df);

    double x = x0;
    for (int i = 0; i < max_iter; i++) {
        double fx = cf.eval(x);
        double dfx = cdf.eval(x);
        if (std::abs(dfx) < 1e-15)
            return {x, i, false};
        double x_new = x - fx / dfx;
        if (std::abs(x_new - x) < tol)
            return {x_new, i + 1, true};
        x = x_new;
    }
    return {x, max_iter, false};
}

double minimize_brent(const CompiledExpr& f, double a, double b, double tol) {
    const double golden = 0.3819660112501051;
    double x = a + golden * (b - a);
    double w = x, v = x;
    double fx = f.eval(x), fw = fx, fv = fx;
    double d = 0.0, e = 0.0;

    for (int i = 0; i < 200; i++) {
        double xm = 0.5 * (a + b);
        double tol1 = tol * std::abs(x) + 1e-10;
        double tol2 = 2.0 * tol1;

        if (std::abs(x - xm) <= (tol2 - 0.5 * (b - a)))
            return x;

        double u;
        if (std::abs(e) > tol1) {
            // Parabolic interpolation
            double r = (x - w) * (fx - fv);
            double q = (x - v) * (fx - fw);
            double p = (x - v) * q - (x - w) * r;
            q = 2.0 * (q - r);
            if (q > 0) p = -p;
            q = std::abs(q);
            double e_old = e;
            e = d;
            if (std::abs(p) >= std::abs(0.5 * q * e_old) || p <= q * (a - x) || p >= q * (b - x)) {
                e = (x >= xm) ? a - x : b - x;
                d = golden * e;
            } else {
                d = p / q;
                u = x + d;
                if (u - a < tol2 || b - u < tol2)
                    d = (xm > x) ? tol1 : -tol1;
            }
        } else {
            e = (x >= xm) ? a - x : b - x;
            d = golden * e;
        }

        u = (std::abs(d) >= tol1) ? x + d : x + ((d > 0) ? tol1 : -tol1);
        double fu = f.eval(u);

        if (fu <= fx) {
            if (u >= x) a = x; else b = x;
            v = w; w = x; x = u;
            fv = fw; fw = fx; fx = fu;
        } else {
            if (u < x) a = u; else b = u;
            if (fu <= fw || w == x) {
                v = w; w = u; fv = fw; fw = fu;
            } else if (fu <= fv || v == x || v == w) {
                v = u; fv = fu;
            }
        }
    }
    return x;
}

} // namespace mathengine
