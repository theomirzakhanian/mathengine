#include "mathengine/steps.h"
#include "mathengine/calculus.h"
#include "mathengine/simplify.h"
#include "mathengine/symbolic_solve.h"
#include "mathengine/polynomial.h"
#include "mathengine/algebra.h"
#include "mathengine/pretty.h"
#include "mathengine/eval.h"
#include <cmath>

namespace mathengine {

namespace {

bool depends_on(const Expr::Ptr& expr, const std::string& var) {
    if (!expr) return false;
    return std::visit([&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Expr::Num>) return false;
        else if constexpr (std::is_same_v<T, Expr::Var>) return node.name == var;
        else if constexpr (std::is_same_v<T, Expr::BinOp>)
            return depends_on(node.lhs, var) || depends_on(node.rhs, var);
        else if constexpr (std::is_same_v<T, Expr::Unary>)
            return depends_on(node.operand, var);
        else if constexpr (std::is_same_v<T, Expr::Func>)
            return depends_on(node.arg, var);
        else return false;
    }, expr->node);
}

} // anonymous namespace

// --- Step-by-step differentiation ---

Expr::Ptr differentiate_steps(const Expr::Ptr& expr, const std::string& var, StepLog& steps) {
    if (!expr) return Expr::num(0);

    steps.push_back({"Start", "d/d" + var + " [" + pretty_string(expr) + "]"});

    auto result = std::visit([&](const auto& node) -> Expr::Ptr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            steps.push_back({"Constant rule", "d/d" + var + "(constant) = 0"});
            return Expr::num(0);
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            if (node.name == var) {
                steps.push_back({"Variable rule", "d/d" + var + "(" + var + ") = 1"});
                return Expr::num(1);
            }
            steps.push_back({"Constant rule", var + " doesn't appear, derivative = 0"});
            return Expr::num(0);
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            switch (node.op) {
                case '+': {
                    steps.push_back({"Sum rule", "d/d" + var + "(f + g) = f' + g'"});
                    auto dl = differentiate(node.lhs, var);
                    auto dr = differentiate(node.rhs, var);
                    return Expr::binop('+', dl, dr);
                }
                case '-': {
                    steps.push_back({"Difference rule", "d/d" + var + "(f - g) = f' - g'"});
                    auto dl = differentiate(node.lhs, var);
                    auto dr = differentiate(node.rhs, var);
                    return Expr::binop('-', dl, dr);
                }
                case '*': {
                    steps.push_back({"Product rule", "d/d" + var + "(f*g) = f'*g + f*g'"});
                    auto dl = differentiate(node.lhs, var);
                    auto dr = differentiate(node.rhs, var);
                    auto result = Expr::binop('+',
                        Expr::binop('*', dl, node.rhs),
                        Expr::binop('*', node.lhs, dr));
                    steps.push_back({"Apply", pretty_string(simplify(result))});
                    return result;
                }
                case '/': {
                    steps.push_back({"Quotient rule", "d/d" + var + "(f/g) = (f'g - fg') / g^2"});
                    auto dl = differentiate(node.lhs, var);
                    auto dr = differentiate(node.rhs, var);
                    return Expr::binop('/',
                        Expr::binop('-',
                            Expr::binop('*', dl, node.rhs),
                            Expr::binop('*', node.lhs, dr)),
                        Expr::binop('^', node.rhs, Expr::num(2)));
                }
                case '^': {
                    bool base_dep = depends_on(node.lhs, var);
                    bool exp_dep = depends_on(node.rhs, var);
                    if (base_dep && !exp_dep) {
                        steps.push_back({"Power rule", "d/d" + var + "(f^n) = n*f^(n-1)*f'"});
                        auto dl = differentiate(node.lhs, var);
                        return Expr::binop('*',
                            Expr::binop('*', node.rhs,
                                Expr::binop('^', node.lhs,
                                    Expr::binop('-', node.rhs, Expr::num(1)))), dl);
                    }
                    if (!base_dep && exp_dep) {
                        steps.push_back({"Exponential rule", "d/d" + var + "(a^g) = a^g * ln(a) * g'"});
                        auto dr = differentiate(node.rhs, var);
                        return Expr::binop('*',
                            Expr::binop('*', expr, Expr::func("ln", node.lhs)), dr);
                    }
                    steps.push_back({"General power rule", "d/d" + var + "(f^g) = f^g*(g'*ln(f) + g*f'/f)"});
                    auto dl = differentiate(node.lhs, var);
                    auto dr = differentiate(node.rhs, var);
                    return Expr::binop('*', expr,
                        Expr::binop('+',
                            Expr::binop('*', dr, Expr::func("ln", node.lhs)),
                            Expr::binop('*', node.rhs,
                                Expr::binop('/', dl, node.lhs))));
                }
                default:
                    return Expr::num(0);
            }
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            const auto& name = node.name;
            std::string outer_rule;
            Expr::Ptr outer;
            if (name == "sin") {
                outer_rule = "d/d" + var + "(sin(u)) = cos(u) * u'";
                outer = Expr::func("cos", node.arg);
            } else if (name == "cos") {
                outer_rule = "d/d" + var + "(cos(u)) = -sin(u) * u'";
                outer = Expr::unary(Expr::func("sin", node.arg));
            } else if (name == "tan") {
                outer_rule = "d/d" + var + "(tan(u)) = sec^2(u) * u'";
                outer = Expr::binop('^', Expr::func("cos", node.arg), Expr::num(-2));
            } else if (name == "exp") {
                outer_rule = "d/d" + var + "(exp(u)) = exp(u) * u'";
                outer = Expr::func("exp", node.arg);
            } else if (name == "ln") {
                outer_rule = "d/d" + var + "(ln(u)) = (1/u) * u'";
                outer = Expr::binop('/', Expr::num(1), node.arg);
            } else if (name == "sqrt") {
                outer_rule = "d/d" + var + "(sqrt(u)) = 1/(2*sqrt(u)) * u'";
                outer = Expr::binop('/', Expr::num(1),
                    Expr::binop('*', Expr::num(2), Expr::func("sqrt", node.arg)));
            } else {
                outer_rule = "Differentiate " + name;
                outer = Expr::num(0);
            }

            steps.push_back({"Chain rule", outer_rule});
            auto da = differentiate(node.arg, var);
            return Expr::binop('*', outer, da);
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            steps.push_back({"Negation rule", "d/d" + var + "(-f) = -f'"});
            return Expr::unary(differentiate(node.operand, var));
        }
        else {
            return Expr::num(0);
        }
    }, expr->node);

    auto simplified = simplify(result);
    steps.push_back({"Simplify", pretty_string(simplified)});
    return simplified;
}

// --- Step-by-step simplification ---

Expr::Ptr simplify_steps(const Expr::Ptr& expr, StepLog& steps) {
    steps.push_back({"Input", pretty_string(expr)});

    auto result = simplify(expr);
    if (pretty_string(expr) != pretty_string(result)) {
        // Try to identify what changed
        auto check_rule = [&](const Expr::Ptr& e) {
            if (!e) return;
            std::visit([&](const auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, Expr::BinOp>) {
                    auto* ln = std::get_if<Expr::Num>(&node.lhs->node);
                    auto* rn = std::get_if<Expr::Num>(&node.rhs->node);
                    if (ln && rn) {
                        steps.push_back({"Constant folding",
                            pretty_string(node.lhs) + " " + std::string(1, node.op) + " " +
                            pretty_string(node.rhs) + " = " + pretty_string(simplify(e))});
                    }
                    if (ln && ln->value == 0 && node.op == '+')
                        steps.push_back({"Identity: 0 + x = x", ""});
                    if (rn && rn->value == 0 && node.op == '+')
                        steps.push_back({"Identity: x + 0 = x", ""});
                    if (ln && ln->value == 1 && node.op == '*')
                        steps.push_back({"Identity: 1 * x = x", ""});
                    if (rn && rn->value == 1 && node.op == '*')
                        steps.push_back({"Identity: x * 1 = x", ""});
                    if (rn && rn->value == 0 && node.op == '^')
                        steps.push_back({"Power: x^0 = 1", ""});
                    if (rn && rn->value == 1 && node.op == '^')
                        steps.push_back({"Power: x^1 = x", ""});
                }
            }, e->node);
        };
        check_rule(expr);
        steps.push_back({"Result", pretty_string(result)});
    } else {
        steps.push_back({"No simplification", "Expression is already in simplest form"});
    }
    return result;
}

// --- Step-by-step solve ---

std::vector<Expr::Ptr> solve_steps(const Expr::Ptr& expr, const std::string& var, StepLog& steps) {
    steps.push_back({"Start", "Solve " + pretty_string(expr) + " = 0"});

    auto expanded = simplify(expand(expr));
    if (pretty_string(expr) != pretty_string(expanded)) {
        steps.push_back({"Expand & simplify", pretty_string(expanded)});
    }

    auto poly_opt = expr_to_poly(expanded, var);
    if (poly_opt) {
        int deg = poly_opt->degree();
        steps.push_back({"Polynomial degree", "Degree " + std::to_string(deg)});

        if (deg == 1) {
            double a = poly_opt->coeff(1), b = poly_opt->coeff(0);
            steps.push_back({"Linear equation", std::to_string(a) + "*" + var + " + " + std::to_string(b) + " = 0"});
            steps.push_back({"Isolate " + var, var + " = " + std::to_string(-b) + "/" + std::to_string(a)});
            double root = -b / a;
            steps.push_back({"Solution", var + " = " + std::to_string(root)});
            return {Expr::num(root)};
        }

        if (deg == 2) {
            double a = poly_opt->coeff(2), b = poly_opt->coeff(1), c = poly_opt->coeff(0);
            steps.push_back({"Quadratic equation",
                std::to_string(a) + "*" + var + "^2 + " + std::to_string(b) + "*" + var + " + " + std::to_string(c) + " = 0"});
            double disc = b * b - 4 * a * c;
            steps.push_back({"Discriminant", "b^2 - 4ac = " + std::to_string(disc)});

            if (disc < -1e-10) {
                steps.push_back({"No real roots", "Discriminant < 0"});
                return {};
            }

            steps.push_back({"Quadratic formula", var + " = (-b +/- sqrt(disc)) / (2a)"});
            double sq = std::sqrt(std::max(0.0, disc));
            double r1 = (-b + sq) / (2 * a);
            double r2 = (-b - sq) / (2 * a);
            std::vector<Expr::Ptr> roots;
            roots.push_back(Expr::num(r1));
            steps.push_back({"Root 1", var + " = " + std::to_string(r1)});
            if (std::abs(r1 - r2) > 1e-10) {
                roots.push_back(Expr::num(r2));
                steps.push_back({"Root 2", var + " = " + std::to_string(r2)});
            }
            return roots;
        }

        steps.push_back({"Higher degree", "Using rational root theorem + factoring"});
    }

    // Fall through to general solver
    auto roots = symbolic_solve(expr, var);
    for (size_t i = 0; i < roots.size(); i++)
        steps.push_back({"Root " + std::to_string(i+1), var + " = " + pretty_string(roots[i])});
    return roots;
}

} // namespace mathengine
