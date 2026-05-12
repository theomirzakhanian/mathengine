#include "mathengine/taylor.h"
#include "mathengine/calculus.h"
#include "mathengine/simplify.h"
#include "mathengine/eval.h"
#include <cmath>
#include <stdexcept>

namespace mathengine {

Expr::Ptr taylor(const Expr::Ptr& expr, const std::string& var,
                 double center, int order) {
    if (order < 0 || order > 20)
        throw std::runtime_error("Taylor order must be 0-20");

    Expr::Ptr current_deriv = expr;
    Expr::Ptr result = Expr::num(0);
    double factorial = 1.0;

    for (int k = 0; k <= order; k++) {
        // Evaluate k-th derivative at center
        auto simplified = simplify(current_deriv);
        double val = 0.0;
        try {
            val = evaluate(simplified, {{var, center}});
        } catch (...) {
            val = 0.0; // undefined at this point
        }

        if (std::isnan(val) || std::isinf(val))
            val = 0.0;

        double coeff = val / factorial;

        if (std::abs(coeff) > 1e-15) {
            Expr::Ptr term;
            if (k == 0) {
                term = Expr::num(coeff);
            } else {
                // (x - center)^k
                Expr::Ptr dx;
                if (std::abs(center) < 1e-15) {
                    dx = Expr::var(var); // Maclaurin: just x
                } else {
                    dx = Expr::binop('-', Expr::var(var), Expr::num(center));
                }

                Expr::Ptr power;
                if (k == 1)
                    power = dx;
                else
                    power = Expr::binop('^', dx, Expr::num(k));

                if (std::abs(coeff - 1.0) < 1e-15)
                    term = power;
                else
                    term = Expr::binop('*', Expr::num(coeff), power);
            }

            result = Expr::binop('+', result, term);
        }

        // Differentiate for next iteration
        if (k < order) {
            current_deriv = simplify(differentiate(current_deriv, var));
        }
        factorial *= (k + 1);
    }

    return simplify(result);
}

} // namespace mathengine
