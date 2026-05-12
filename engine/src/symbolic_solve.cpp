#include "mathengine/symbolic_solve.h"
#include "mathengine/polynomial.h"
#include "mathengine/algebra.h"
#include "mathengine/simplify.h"
#include "mathengine/eval.h"
#include "mathengine/parser.h"
#include "mathengine/linalg.h"
#include "mathengine/matrix.h"
#include <cmath>
#include <set>
#include <sstream>
#include <algorithm>

namespace mathengine {

namespace {

// Check if expr structurally matches BinOp('-', Func(name, inner), Num(k))
// i.e., f(inner) - k = 0 => inner = f_inv(k)
bool match_func_minus_const(const Expr::Ptr& expr, std::string& func_name,
                            Expr::Ptr& inner, double& k) {
    if (auto* bin = std::get_if<Expr::BinOp>(&expr->node)) {
        if (bin->op == '-') {
            if (auto* fn = std::get_if<Expr::Func>(&bin->lhs->node)) {
                if (auto* num = std::get_if<Expr::Num>(&bin->rhs->node)) {
                    func_name = fn->name;
                    inner = fn->arg;
                    k = num->value;
                    return true;
                }
            }
        }
    }
    // Also check: Func(...) alone (= 0, so k=0)
    if (auto* fn = std::get_if<Expr::Func>(&expr->node)) {
        func_name = fn->name;
        inner = fn->arg;
        k = 0.0;
        return true;
    }
    return false;
}

// Recursively solve inner_expr = target for var
std::vector<Expr::Ptr> solve_inner(const Expr::Ptr& inner_expr, const Expr::Ptr& target,
                                    const std::string& var) {
    // If inner is just the variable, done
    if (auto* v = std::get_if<Expr::Var>(&inner_expr->node)) {
        if (v->name == var) return {target};
    }
    // If inner is a*x + b (linear in var), solve
    auto poly_opt = expr_to_poly(inner_expr, var);
    if (poly_opt && poly_opt->degree() == 1) {
        double a = poly_opt->coeff(1);
        double b = poly_opt->coeff(0);
        // a*x + b = target_val
        // We need target as numeric
        try {
            double t = evaluate(target, {});
            double x = (t - b) / a;
            return {Expr::num(x)};
        } catch (...) {
            // Symbolic: x = (target - b) / a
            return {simplify(Expr::binop('/', Expr::binop('-', target, Expr::num(b)), Expr::num(a)))};
        }
    }
    return {};
}

} // anonymous namespace

std::vector<Expr::Ptr> symbolic_solve(const Expr::Ptr& expr, const std::string& var) {
    auto expanded = simplify(expand(expr));

    // --- Polynomial path ---
    auto poly_opt = expr_to_poly(expanded, var);
    if (poly_opt) {
        auto& poly = *poly_opt;
        int deg = poly.degree();

        if (deg == 0) {
            // Constant: either always 0 (infinite solutions) or never 0
            return {};
        }

        if (deg == 1) {
            // ax + b = 0 => x = -b/a
            double a = poly.coeff(1), b = poly.coeff(0);
            return {simplify(Expr::num(-b / a))};
        }

        if (deg == 2) {
            // Quadratic formula
            double a = poly.coeff(2), b = poly.coeff(1), c = poly.coeff(0);
            double disc = b * b - 4 * a * c;
            if (disc < -1e-10) return {}; // no real roots
            if (std::abs(disc) < 1e-10) {
                // One repeated root
                return {simplify(Expr::num(-b / (2 * a)))};
            }
            double sq = std::sqrt(disc);
            double r1 = (-b + sq) / (2 * a);
            double r2 = (-b - sq) / (2 * a);
            // Return as clean expressions if they're nice numbers
            std::vector<Expr::Ptr> results;
            // Check if roots are "nice" (integer or simple fraction)
            auto make_root = [&](double r) -> Expr::Ptr {
                if (std::abs(r - std::round(r)) < 1e-10)
                    return Expr::num(std::round(r));
                return Expr::num(r);
            };
            results.push_back(make_root(r1));
            if (std::abs(r1 - r2) > 1e-10)
                results.push_back(make_root(r2));
            return results;
        }

        if (deg == 3) {
            // Try rational roots first
            std::vector<Expr::Ptr> roots;
            Polynomial remaining = poly;

            int a0 = std::max(1, (int)std::round(std::abs(remaining.coeff(0))));
            int an = std::max(1, (int)std::round(std::abs(remaining.coeff(remaining.degree()))));
            if (a0 > 100) a0 = 100;
            if (an > 100) an = 100;

            auto pf = [](int n) {
                std::vector<int> f;
                n = std::abs(n);
                if (n == 0) n = 1;
                for (int i = 1; i <= n; i++) if (n % i == 0) f.push_back(i);
                return f;
            };

            bool found = true;
            while (found && remaining.degree() >= 2) {
                found = false;
                a0 = std::max(1, (int)std::round(std::abs(remaining.coeff(0))));
                an = std::max(1, (int)std::round(std::abs(remaining.coeff(remaining.degree()))));
                if (a0 > 100) a0 = 100;
                if (an > 100) an = 100;
                for (int p : pf(a0)) {
                    for (int q : pf(an)) {
                        for (int s : {1, -1}) {
                            double c = s * (double)p / q;
                            if (std::abs(remaining.eval(c)) < 1e-8) {
                                roots.push_back(Expr::num(std::round(c * 1e10) / 1e10));
                                auto [quot, rem] = remaining.divmod(Polynomial({-c, 1.0}));
                                remaining = quot;
                                found = true;
                                goto found_cubic;
                            }
                        }
                    }
                }
                found_cubic:;
            }

            // Solve remaining (should be quadratic or linear)
            if (remaining.degree() == 2) {
                double a = remaining.coeff(2), b = remaining.coeff(1), c = remaining.coeff(0);
                double disc = b * b - 4 * a * c;
                if (disc >= -1e-10) {
                    double sq = std::sqrt(std::max(0.0, disc));
                    roots.push_back(Expr::num((-b + sq) / (2 * a)));
                    if (disc > 1e-10)
                        roots.push_back(Expr::num((-b - sq) / (2 * a)));
                }
            } else if (remaining.degree() == 1) {
                roots.push_back(Expr::num(-remaining.coeff(0) / remaining.coeff(1)));
            }

            return roots;
        }

        // Degree 4+: try rational roots, then numeric
        if (deg >= 4) {
            std::vector<Expr::Ptr> roots;
            Polynomial remaining = poly;

            auto pf = [](int n) {
                std::vector<int> f;
                n = std::abs(n);
                if (n == 0) n = 1;
                for (int i = 1; i <= n; i++) if (n % i == 0) f.push_back(i);
                return f;
            };

            bool found = true;
            while (found && remaining.degree() >= 2) {
                found = false;
                int a0v = std::max(1, (int)std::round(std::abs(remaining.coeff(0))));
                int anv = std::max(1, (int)std::round(std::abs(remaining.coeff(remaining.degree()))));
                if (a0v > 100) a0v = 100;
                if (anv > 100) anv = 100;
                for (int p : pf(a0v)) {
                    for (int q : pf(anv)) {
                        for (int s : {1, -1}) {
                            double c = s * (double)p / q;
                            if (std::abs(remaining.eval(c)) < 1e-8) {
                                roots.push_back(Expr::num(c));
                                auto [quot, rem] = remaining.divmod(Polynomial({-c, 1.0}));
                                remaining = quot;
                                found = true;
                                goto found_high;
                            }
                        }
                    }
                }
                found_high:;
            }

            // Solve remaining quadratic if applicable
            if (remaining.degree() == 2) {
                double a = remaining.coeff(2), b = remaining.coeff(1), c = remaining.coeff(0);
                double disc = b * b - 4 * a * c;
                if (disc >= -1e-10) {
                    double sq = std::sqrt(std::max(0.0, disc));
                    roots.push_back(Expr::num((-b + sq) / (2 * a)));
                    if (disc > 1e-10)
                        roots.push_back(Expr::num((-b - sq) / (2 * a)));
                }
            } else if (remaining.degree() == 1) {
                roots.push_back(Expr::num(-remaining.coeff(0) / remaining.coeff(1)));
            }

            return roots;
        }
    }

    // --- Transcendental patterns ---
    auto simplified = simplify(expanded);
    std::string func_name;
    Expr::Ptr inner;
    double k;

    if (match_func_minus_const(simplified, func_name, inner, k)) {
        // f(inner) = k => inner = f_inv(k)
        Expr::Ptr target;
        if (func_name == "exp") target = Expr::num(std::log(k));
        else if (func_name == "ln" && k >= 0) target = Expr::num(std::exp(k));
        else if (func_name == "sin" && k >= -1 && k <= 1) target = Expr::num(std::asin(k));
        else if (func_name == "cos" && k >= -1 && k <= 1) target = Expr::num(std::acos(k));
        else if (func_name == "tan") target = Expr::num(std::atan(k));
        else if (func_name == "sqrt" && k >= 0) target = Expr::num(k * k);

        if (target) {
            return solve_inner(inner, target, var);
        }
    }

    return {}; // couldn't solve
}

// --- Systems of linear equations ---

SystemSolution solve_linear_system(const std::string& input) {
    SystemSolution sol;

    // Split on ; or newline
    std::vector<std::string> equations;
    std::stringstream ss(input);
    std::string segment;
    while (std::getline(ss, segment, ';')) {
        // Also split on newlines
        std::stringstream ss2(segment);
        std::string line;
        while (std::getline(ss2, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) continue;
            size_t end = line.find_last_not_of(" \t\n\r");
            line = line.substr(start, end - start + 1);
            if (!line.empty()) equations.push_back(line);
        }
    }

    if (equations.empty()) {
        sol.error = "No equations provided";
        return sol;
    }

    // Parse each equation: split on '=', parse both sides, form lhs - rhs
    std::vector<Expr::Ptr> eqns;
    std::set<std::string> all_vars;

    for (auto& eq_str : equations) {
        size_t eq_pos = eq_str.find('=');
        if (eq_pos == std::string::npos) {
            sol.error = "Missing '=' in equation: " + eq_str;
            return sol;
        }
        std::string lhs_str = eq_str.substr(0, eq_pos);
        std::string rhs_str = eq_str.substr(eq_pos + 1);

        try {
            auto lhs = parse(lhs_str);
            auto rhs = parse(rhs_str);
            auto eq_expr = simplify(Expr::binop('-', lhs, rhs));
            eqns.push_back(eq_expr);

            // Collect variables (walk AST)
            std::function<void(const Expr::Ptr&)> collect = [&](const Expr::Ptr& e) {
                if (!e) return;
                std::visit([&](const auto& node) {
                    using T = std::decay_t<decltype(node)>;
                    if constexpr (std::is_same_v<T, Expr::Var>) {
                        all_vars.insert(node.name);
                    } else if constexpr (std::is_same_v<T, Expr::BinOp>) {
                        collect(node.lhs);
                        collect(node.rhs);
                    } else if constexpr (std::is_same_v<T, Expr::Unary>) {
                        collect(node.operand);
                    } else if constexpr (std::is_same_v<T, Expr::Func>) {
                        collect(node.arg);
                    }
                }, e->node);
            };
            collect(eq_expr);
        } catch (const std::exception& e) {
            sol.error = std::string("Parse error: ") + e.what();
            return sol;
        }
    }

    sol.var_names.assign(all_vars.begin(), all_vars.end());
    std::sort(sol.var_names.begin(), sol.var_names.end());
    size_t n = sol.var_names.size();
    size_t m = eqns.size();

    if (m < n) {
        sol.error = "Underdetermined system: " + std::to_string(m) + " equations, " + std::to_string(n) + " unknowns";
        return sol;
    }

    // Build coefficient matrix and constant vector
    // For each equation, coeff of var_j = eval(eq, {var_j=1, others=0}) - eval(eq, {all=0})
    // Constant = -eval(eq, {all=0})
    Matrix A(m, n);
    Matrix b(m, 1);

    for (size_t i = 0; i < m; i++) {
        VarMap all_zero;
        for (auto& v : sol.var_names) all_zero[v] = 0.0;

        double c0;
        try {
            c0 = evaluate(eqns[i], all_zero);
        } catch (...) {
            sol.error = "Non-linear equation detected";
            return sol;
        }
        b(i, 0) = -c0;

        for (size_t j = 0; j < n; j++) {
            VarMap vars = all_zero;
            vars[sol.var_names[j]] = 1.0;
            double val;
            try {
                val = evaluate(eqns[i], vars);
            } catch (...) {
                sol.error = "Non-linear equation detected";
                return sol;
            }
            A(i, j) = val - c0;
        }
    }

    // Solve using existing LU solver
    try {
        // If overdetermined, use first n equations
        Matrix A_sq(n, n);
        Matrix b_sq(n, 1);
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++)
                A_sq(i, j) = A(i, j);
            b_sq(i, 0) = b(i, 0);
        }

        Matrix x = solve(A_sq, b_sq);
        sol.values.resize(n);
        for (size_t i = 0; i < n; i++)
            sol.values[i] = x(i, 0);
        sol.solved = true;
    } catch (const std::exception& e) {
        sol.error = std::string("Solve failed: ") + e.what();
    }

    return sol;
}

} // namespace mathengine
