#include "mathengine/limits.h"
#include "mathengine/calculus.h"
#include "mathengine/simplify.h"
#include "mathengine/eval.h"
#include <cmath>
#include <stdexcept>

namespace mathengine {

namespace {

// Try to evaluate, returns NaN on failure
double try_eval(const Expr::Ptr& expr, const std::string& var, double val) {
    try {
        double result = evaluate(expr, {{var, val}});
        return result;
    } catch (...) {
        return std::nan("");
    }
}

// Check for known special limits
// Returns true and sets result if matched
bool check_known_limits(const Expr::Ptr& expr, const std::string& var,
                        double point, Expr::Ptr& result) {
    if (std::abs(point) > 1e-10) return false; // only handle x->0 specials

    // sin(x)/x -> 1
    if (auto* bin = std::get_if<Expr::BinOp>(&expr->node)) {
        if (bin->op == '/') {
            if (auto* fn = std::get_if<Expr::Func>(&bin->lhs->node)) {
                if (fn->name == "sin") {
                    if (auto* v = std::get_if<Expr::Var>(&fn->arg->node)) {
                        if (v->name == var) {
                            if (auto* v2 = std::get_if<Expr::Var>(&bin->rhs->node)) {
                                if (v2->name == var) {
                                    result = Expr::num(1.0);
                                    return true;
                                }
                            }
                        }
                    }
                }
                // (1 - cos(x)) / x -> 0
                // tan(x)/x -> 1
                if (fn->name == "tan") {
                    if (auto* v = std::get_if<Expr::Var>(&fn->arg->node)) {
                        if (v->name == var) {
                            if (auto* v2 = std::get_if<Expr::Var>(&bin->rhs->node)) {
                                if (v2->name == var) {
                                    result = Expr::num(1.0);
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

// Detect if expr is a quotient f/g
bool is_quotient(const Expr::Ptr& expr, Expr::Ptr& num_out, Expr::Ptr& den_out) {
    if (auto* bin = std::get_if<Expr::BinOp>(&expr->node)) {
        if (bin->op == '/') {
            num_out = bin->lhs;
            den_out = bin->rhs;
            return true;
        }
    }
    return false;
}

} // anonymous namespace

Expr::Ptr limit(const Expr::Ptr& expr, const std::string& var,
                double point, int point_from) {
    if (!expr) return Expr::num(0);

    auto simplified = simplify(expr);

    // Step 1: Check known special limits
    Expr::Ptr known_result;
    if (check_known_limits(simplified, var, point, known_result))
        return known_result;

    // Step 2: Direct substitution
    double val = try_eval(simplified, var, point);
    if (!std::isnan(val) && !std::isinf(val))
        return Expr::num(val);

    // Step 3: Try approaching from the side (numerically, to detect the limit)
    double eps = 1e-8;
    double approach_val;
    if (point_from >= 0) {
        approach_val = try_eval(simplified, var, point + eps);
    } else {
        approach_val = try_eval(simplified, var, point - eps);
    }

    // Step 4: L'Hopital's for quotients
    Expr::Ptr numerator, denominator;
    if (is_quotient(simplified, numerator, denominator)) {
        double f_val = try_eval(numerator, var, point);
        double g_val = try_eval(denominator, var, point);

        // 0/0 or inf/inf form
        bool zero_zero = (std::abs(f_val) < 1e-10 && std::abs(g_val) < 1e-10);
        bool inf_inf = (std::isinf(f_val) && std::isinf(g_val));
        bool f_nan_g_zero = (std::isnan(f_val) || std::isnan(g_val));

        if (zero_zero || inf_inf || f_nan_g_zero) {
            // Apply L'Hopital's up to 5 times
            Expr::Ptr f = numerator, g = denominator;
            for (int i = 0; i < 5; i++) {
                auto df = simplify(differentiate(f, var));
                auto dg = simplify(differentiate(g, var));

                double dg_val = try_eval(dg, var, point);
                if (std::abs(dg_val) < 1e-15) {
                    // g' is also 0, continue
                    double df_val = try_eval(df, var, point);
                    if (std::abs(df_val) < 1e-15) {
                        f = df;
                        g = dg;
                        continue;
                    }
                    break; // g'=0 but f'!=0 means limit is +/-inf or DNE
                }

                double df_val = try_eval(df, var, point);
                if (!std::isnan(df_val) && !std::isnan(dg_val) && std::abs(dg_val) > 1e-15) {
                    return Expr::num(df_val / dg_val);
                }

                f = df;
                g = dg;
            }
        }
    }

    // Step 5: Numeric approach as fallback
    if (!std::isnan(approach_val) && !std::isinf(approach_val)) {
        // Verify with multiple approach values
        double v1 = try_eval(simplified, var, point + 1e-10);
        double v2 = try_eval(simplified, var, point + 1e-12);
        if (!std::isnan(v1) && !std::isnan(v2) &&
            std::abs(v1 - approach_val) < 1e-4 &&
            std::abs(v2 - approach_val) < 1e-4) {
            // Round to clean number if close
            double rounded = std::round(approach_val * 1e10) / 1e10;
            return Expr::num(rounded);
        }
        return Expr::num(approach_val);
    }

    return nullptr; // limit doesn't exist or can't be computed
}

} // namespace mathengine
