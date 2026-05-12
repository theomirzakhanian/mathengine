#include "mathengine/algebra.h"
#include "mathengine/polynomial.h"
#include "mathengine/simplify.h"
#include <cmath>
#include <vector>
#include <set>

namespace mathengine {

// --- Expand ---

Expr::Ptr expand(const Expr::Ptr& expr) {
    if (!expr) return nullptr;

    return std::visit([&](const auto& node) -> Expr::Ptr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num> || std::is_same_v<T, Expr::Var>) {
            return expr;
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            auto inner = expand(node.operand);
            // Distribute negation into sums: -(a+b) = -a + -b
            if (auto* bin = std::get_if<Expr::BinOp>(&inner->node)) {
                if (bin->op == '+')
                    return expand(Expr::binop('+', Expr::unary(bin->lhs), Expr::unary(bin->rhs)));
                if (bin->op == '-')
                    return expand(Expr::binop('-', Expr::unary(bin->lhs), Expr::unary(bin->rhs)));
            }
            return simplify(Expr::unary(inner));
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            return Expr::func(node.name, expand(node.arg));
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            auto l = expand(node.lhs);
            auto r = expand(node.rhs);

            if (node.op == '+' || node.op == '-') {
                return simplify(Expr::binop(node.op, l, r));
            }

            if (node.op == '*') {
                // Distribute: a * (b + c) = a*b + a*c
                if (auto* rb = std::get_if<Expr::BinOp>(&r->node)) {
                    if (rb->op == '+')
                        return expand(Expr::binop('+',
                            Expr::binop('*', l, rb->lhs),
                            Expr::binop('*', l, rb->rhs)));
                    if (rb->op == '-')
                        return expand(Expr::binop('-',
                            Expr::binop('*', l, rb->lhs),
                            Expr::binop('*', l, rb->rhs)));
                }
                // (a + b) * c = a*c + b*c
                if (auto* lb = std::get_if<Expr::BinOp>(&l->node)) {
                    if (lb->op == '+')
                        return expand(Expr::binop('+',
                            Expr::binop('*', lb->lhs, r),
                            Expr::binop('*', lb->rhs, r)));
                    if (lb->op == '-')
                        return expand(Expr::binop('-',
                            Expr::binop('*', lb->lhs, r),
                            Expr::binop('*', lb->rhs, r)));
                }
                return simplify(Expr::binop('*', l, r));
            }

            if (node.op == '^') {
                // Expand integer powers by repeated multiplication
                if (auto* n = std::get_if<Expr::Num>(&r->node)) {
                    int exp = (int)std::round(n->value);
                    if (exp >= 2 && exp <= 10 && std::abs(n->value - exp) < 1e-10) {
                        Expr::Ptr result = l;
                        for (int i = 1; i < exp; i++)
                            result = expand(Expr::binop('*', result, l));
                        return result;
                    }
                }
                return simplify(Expr::binop('^', l, r));
            }

            return simplify(Expr::binop(node.op, l, r));
        }
        else {
            return expr;
        }
    }, expr->node);
}

// --- Factor ---

namespace {

// Get integer factors of |n|
std::vector<int> int_factors(int n) {
    n = std::abs(n);
    if (n == 0) return {0};
    std::vector<int> f;
    for (int i = 1; i <= n; i++)
        if (n % i == 0) f.push_back(i);
    return f;
}

} // anonymous namespace

Expr::Ptr factor(const Expr::Ptr& expr, const std::string& var) {
    auto expanded = expand(expr);
    auto poly_opt = expr_to_poly(expanded, var);
    if (!poly_opt) return expanded; // not a polynomial, return expanded

    Polynomial poly = *poly_opt;
    int deg = poly.degree();
    if (deg <= 1) return poly.to_expr(var);

    // Extract common numeric factor
    double common = poly.coeff(0);
    for (int i = 1; i <= deg; i++) {
        double c = poly.coeff(i);
        if (std::abs(c) > 1e-10) {
            if (std::abs(common) < 1e-10) common = c;
        }
    }

    // Find rational roots using Rational Root Theorem
    // Candidates: +/- (factors of constant term) / (factors of leading coefficient)
    std::vector<Expr::Ptr> linear_factors;
    Polynomial remaining = poly;

    int a0 = (int)std::round(std::abs(remaining.coeff(0)));
    int an = (int)std::round(std::abs(remaining.coeff(remaining.degree())));
    if (a0 == 0) a0 = 1;
    if (an == 0) an = 1;
    // Clamp to avoid huge search
    if (a0 > 1000) a0 = 1000;
    if (an > 1000) an = 1000;

    auto p_factors = int_factors(a0);
    auto q_factors = int_factors(an);

    std::set<double> tested;
    bool found = true;
    while (found && remaining.degree() >= 1) {
        found = false;
        // Recalculate candidates for current polynomial
        a0 = std::max(1, (int)std::round(std::abs(remaining.coeff(0))));
        an = std::max(1, (int)std::round(std::abs(remaining.coeff(remaining.degree()))));
        if (a0 > 1000) a0 = 1000;
        if (an > 1000) an = 1000;
        p_factors = int_factors(a0);
        q_factors = int_factors(an);

        for (int p : p_factors) {
            for (int q : q_factors) {
                for (int sign : {1, -1}) {
                    double candidate = sign * (double)p / (double)q;
                    if (tested.count(candidate)) continue;
                    tested.insert(candidate);

                    if (std::abs(remaining.eval(candidate)) < 1e-8) {
                        // Found a root! Divide out (x - candidate)
                        Polynomial linear({-candidate, 1.0});
                        auto [quot, rem] = remaining.divmod(linear);
                        remaining = quot;
                        linear_factors.push_back(
                            Expr::binop('-', Expr::var(var), Expr::num(candidate)));
                        found = true;
                        goto next_root;
                    }
                }
            }
        }
        next_root:;
    }

    // Build result: product of linear factors * remaining polynomial
    Expr::Ptr result = nullptr;

    // Leading coefficient of original / product of linear factors
    if (remaining.degree() > 0 || std::abs(remaining.coeff(0) - 1.0) > 1e-10) {
        result = remaining.to_expr(var);
    }

    for (auto& lf : linear_factors) {
        if (!result)
            result = lf;
        else
            result = Expr::binop('*', result, lf);
    }

    if (!result) result = Expr::num(0);

    return simplify(result);
}

} // namespace mathengine
