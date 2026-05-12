#include "mathengine/calculus.h"
#include "mathengine/simplify.h"
#include "mathengine/eval.h"
#include <cmath>
#include <stdexcept>

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

Expr::Ptr differentiate(const Expr::Ptr& expr, const std::string& var) {
    if (!expr) return Expr::num(0);

    auto result = std::visit([&](const auto& node) -> Expr::Ptr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            return Expr::num(0);
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            return (node.name == var) ? Expr::num(1) : Expr::num(0);
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            return Expr::unary(differentiate(node.operand, var));
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            auto dl = differentiate(node.lhs, var);
            auto dr = differentiate(node.rhs, var);

            switch (node.op) {
                case '+': return Expr::binop('+', dl, dr);
                case '-': return Expr::binop('-', dl, dr);
                case '*':
                    // Product rule: d(f*g) = f'*g + f*g'
                    return Expr::binop('+',
                        Expr::binop('*', dl, node.rhs),
                        Expr::binop('*', node.lhs, dr));
                case '/':
                    // Quotient rule: d(f/g) = (f'*g - f*g') / g^2
                    return Expr::binop('/',
                        Expr::binop('-',
                            Expr::binop('*', dl, node.rhs),
                            Expr::binop('*', node.lhs, dr)),
                        Expr::binop('^', node.rhs, Expr::num(2)));
                case '^': {
                    bool base_dep = depends_on(node.lhs, var);
                    bool exp_dep = depends_on(node.rhs, var);

                    if (!base_dep && !exp_dep) {
                        return Expr::num(0);
                    }
                    if (base_dep && !exp_dep) {
                        // Power rule: d(f^n) = n * f^(n-1) * f'
                        return Expr::binop('*',
                            Expr::binop('*', node.rhs,
                                Expr::binop('^', node.lhs,
                                    Expr::binop('-', node.rhs, Expr::num(1)))),
                            dl);
                    }
                    if (!base_dep && exp_dep) {
                        // Exponential rule: d(a^g) = a^g * ln(a) * g'
                        return Expr::binop('*',
                            Expr::binop('*', expr, Expr::func("ln", node.lhs)),
                            dr);
                    }
                    // General case: d(f^g) = f^g * (g' * ln(f) + g * f'/f)
                    return Expr::binop('*', expr,
                        Expr::binop('+',
                            Expr::binop('*', dr, Expr::func("ln", node.lhs)),
                            Expr::binop('*', node.rhs,
                                Expr::binop('/', dl, node.lhs))));
                }
                default:
                    throw std::runtime_error("Unknown operator in differentiate");
            }
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            auto da = differentiate(node.arg, var);
            Expr::Ptr outer;

            const auto& name = node.name;
            if (name == "sin")
                outer = Expr::func("cos", node.arg);
            else if (name == "cos")
                outer = Expr::unary(Expr::func("sin", node.arg));
            else if (name == "tan")
                outer = Expr::binop('^', Expr::func("cos", node.arg), Expr::num(-2));
            else if (name == "exp")
                outer = Expr::func("exp", node.arg);
            else if (name == "ln")
                outer = Expr::binop('/', Expr::num(1), node.arg);
            else if (name == "sqrt")
                outer = Expr::binop('/', Expr::num(1),
                    Expr::binop('*', Expr::num(2), Expr::func("sqrt", node.arg)));
            else if (name == "asin")
                outer = Expr::binop('/', Expr::num(1),
                    Expr::func("sqrt", Expr::binop('-', Expr::num(1),
                        Expr::binop('^', node.arg, Expr::num(2)))));
            else if (name == "acos")
                outer = Expr::unary(Expr::binop('/', Expr::num(1),
                    Expr::func("sqrt", Expr::binop('-', Expr::num(1),
                        Expr::binop('^', node.arg, Expr::num(2))))));
            else if (name == "atan")
                outer = Expr::binop('/', Expr::num(1),
                    Expr::binop('+', Expr::num(1),
                        Expr::binop('^', node.arg, Expr::num(2))));
            else if (name == "abs")
                outer = Expr::binop('/', node.arg, Expr::func("abs", node.arg));
            else
                throw std::runtime_error("Cannot differentiate function: " + name);

            // Chain rule: d(f(g)) = f'(g) * g'
            return Expr::binop('*', outer, da);
        }
        else {
            return Expr::num(0);
        }
    }, expr->node);

    return simplify(result);
}

Expr::Ptr integrate(const Expr::Ptr& expr, const std::string& var) {
    // Basic integration for simple forms
    if (!expr) return nullptr;

    return std::visit([&](const auto& node) -> Expr::Ptr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            // integral of c = c*x
            return Expr::binop('*', expr, Expr::var(var));
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            if (node.name == var) {
                // integral of x = x^2/2
                return Expr::binop('/', Expr::binop('^', expr, Expr::num(2)), Expr::num(2));
            }
            // integral of y dx = y*x
            return Expr::binop('*', expr, Expr::var(var));
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            if (node.op == '+' || node.op == '-') {
                auto li = integrate(node.lhs, var);
                auto ri = integrate(node.rhs, var);
                if (li && ri) return simplify(Expr::binop(node.op, li, ri));
            }
            if (node.op == '*') {
                // c * f(x)
                if (!depends_on(node.lhs, var)) {
                    auto ri = integrate(node.rhs, var);
                    if (ri) return simplify(Expr::binop('*', node.lhs, ri));
                }
                if (!depends_on(node.rhs, var)) {
                    auto li = integrate(node.lhs, var);
                    if (li) return simplify(Expr::binop('*', node.rhs, li));
                }
            }
            if (node.op == '^') {
                // x^n where n is constant
                if (auto* v = std::get_if<Expr::Var>(&node.lhs->node)) {
                    if (v->name == var && !depends_on(node.rhs, var)) {
                        if (auto* n = std::get_if<Expr::Num>(&node.rhs->node)) {
                            if (n->value != -1.0) {
                                auto np1 = Expr::binop('+', node.rhs, Expr::num(1));
                                return simplify(Expr::binop('/', Expr::binop('^', node.lhs, np1), np1));
                            } else {
                                return Expr::func("ln", Expr::func("abs", Expr::var(var)));
                            }
                        }
                    }
                }
            }
            // Integration by parts: poly * {exp, sin, cos}
            if (node.op == '*') {
                // Check if one side is polynomial and other is exp/sin/cos
                auto try_ibp = [&](const Expr::Ptr& u_candidate, const Expr::Ptr& dv_candidate) -> Expr::Ptr {
                    // u should depend on var (polynomial-like), dv should be exp/sin/cos
                    if (!depends_on(u_candidate, var)) return nullptr;
                    if (auto* fn = std::get_if<Expr::Func>(&dv_candidate->node)) {
                        if (fn->name == "exp" || fn->name == "sin" || fn->name == "cos") {
                            if (auto* v_node = std::get_if<Expr::Var>(&fn->arg->node)) {
                                if (v_node->name == var) {
                                    // IBP: integral u dv = u*v - integral v du
                                    auto du = simplify(differentiate(u_candidate, var));
                                    auto v_int = integrate(dv_candidate, var);
                                    if (!v_int) return nullptr;
                                    auto uv = Expr::binop('*', u_candidate, v_int);
                                    auto vdu = Expr::binop('*', v_int, du);
                                    auto vdu_int = integrate(simplify(vdu), var);
                                    if (!vdu_int) return nullptr;
                                    return simplify(Expr::binop('-', uv, vdu_int));
                                }
                            }
                        }
                    }
                    return nullptr;
                };
                auto r1 = try_ibp(node.lhs, node.rhs);
                if (r1) return r1;
                auto r2 = try_ibp(node.rhs, node.lhs);
                if (r2) return r2;
            }

            // 1/(x^2 + a^2) -> (1/a)*atan(x/a)
            if (node.op == '/') {
                if (auto* num_node = std::get_if<Expr::Num>(&node.lhs->node)) {
                    if (std::abs(num_node->value - 1.0) < 1e-10) {
                        if (auto* den = std::get_if<Expr::BinOp>(&node.rhs->node)) {
                            if (den->op == '+') {
                                // Check for x^2 + a^2
                                auto is_var_sq = [&](const Expr::Ptr& e) -> bool {
                                    if (auto* b = std::get_if<Expr::BinOp>(&e->node)) {
                                        if (b->op == '^') {
                                            if (auto* v = std::get_if<Expr::Var>(&b->lhs->node)) {
                                                if (v->name == var) {
                                                    if (auto* n = std::get_if<Expr::Num>(&b->rhs->node))
                                                        return std::abs(n->value - 2.0) < 1e-10;
                                                }
                                            }
                                        }
                                    }
                                    return false;
                                };
                                auto get_const = [&](const Expr::Ptr& e) -> double {
                                    if (auto* n = std::get_if<Expr::Num>(&e->node)) return n->value;
                                    return -1;
                                };
                                if (is_var_sq(den->lhs) && !depends_on(den->rhs, var)) {
                                    double a2 = get_const(den->rhs);
                                    if (a2 > 0) {
                                        double a = std::sqrt(a2);
                                        return simplify(Expr::binop('*', Expr::num(1.0/a),
                                            Expr::func("atan", Expr::binop('/', Expr::var(var), Expr::num(a)))));
                                    }
                                }
                                if (is_var_sq(den->rhs) && !depends_on(den->lhs, var)) {
                                    double a2 = get_const(den->lhs);
                                    if (a2 > 0) {
                                        double a = std::sqrt(a2);
                                        return simplify(Expr::binop('*', Expr::num(1.0/a),
                                            Expr::func("atan", Expr::binop('/', Expr::var(var), Expr::num(a)))));
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return nullptr; // can't integrate
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            if (!depends_on(node.arg, var)) {
                return Expr::binop('*', expr, Expr::var(var));
            }

            // Check if arg is a linear function of var: a*var + b
            auto is_linear = [&](const Expr::Ptr& e, double& a_out, double& b_out) -> bool {
                // Try: just var (a=1, b=0)
                if (auto* v = std::get_if<Expr::Var>(&e->node)) {
                    if (v->name == var) { a_out = 1; b_out = 0; return true; }
                }
                // Try: a*var + b or a*var
                try {
                    double at0 = evaluate(e, {{var, 0.0}});
                    double at1 = evaluate(e, {{var, 1.0}});
                    double at2 = evaluate(e, {{var, 2.0}});
                    double a = at1 - at0;
                    double b = at0;
                    // Verify linearity
                    if (std::abs((a * 2 + b) - at2) < 1e-10 && std::abs(a) > 1e-15) {
                        a_out = a; b_out = b;
                        return true;
                    }
                } catch (...) {}
                return false;
            };

            double a_coeff, b_coeff;
            if (is_linear(node.arg, a_coeff, b_coeff)) {
                // integral f(ax+b) dx = F(ax+b) / a
                const auto& name = node.name;
                Expr::Ptr antideriv;
                if (name == "sin")
                    antideriv = Expr::unary(Expr::func("cos", node.arg));
                else if (name == "cos")
                    antideriv = Expr::func("sin", node.arg);
                else if (name == "exp")
                    antideriv = Expr::func("exp", node.arg);
                else if (name == "tan")
                    antideriv = Expr::unary(Expr::func("ln", Expr::func("abs", Expr::func("cos", node.arg))));

                if (antideriv) {
                    if (std::abs(a_coeff - 1.0) < 1e-10)
                        return simplify(antideriv);
                    return simplify(Expr::binop('/', antideriv, Expr::num(a_coeff)));
                }
            }
            return nullptr;
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            auto inner = integrate(node.operand, var);
            if (inner) return Expr::unary(inner);
            return nullptr;
        }
        else {
            return nullptr;
        }
    }, expr->node);
}

} // namespace mathengine
