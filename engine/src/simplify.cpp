#include "mathengine/simplify.h"
#include <cmath>

namespace mathengine {

namespace {

bool is_num(const Expr::Ptr& e, double v) {
    if (auto* n = std::get_if<Expr::Num>(&e->node))
        return n->value == v;
    return false;
}

bool is_num(const Expr::Ptr& e) {
    return std::holds_alternative<Expr::Num>(e->node);
}

double get_num(const Expr::Ptr& e) {
    return std::get<Expr::Num>(e->node).value;
}

} // anonymous namespace

Expr::Ptr simplify(const Expr::Ptr& expr) {
    if (!expr) return nullptr;

    return std::visit([&](const auto& node) -> Expr::Ptr {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num> || std::is_same_v<T, Expr::Var>) {
            return expr;
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            auto inner = simplify(node.operand);
            if (is_num(inner))
                return Expr::num(-get_num(inner));
            // double negation
            if (auto* u = std::get_if<Expr::Unary>(&inner->node))
                return simplify(u->operand);
            return Expr::unary(inner);
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            auto arg = simplify(node.arg);
            // Constant folding
            if (is_num(arg)) {
                double a = get_num(arg);
                const auto& name = node.name;
                if (name == "sin")  return Expr::num(std::sin(a));
                if (name == "cos")  return Expr::num(std::cos(a));
                if (name == "tan")  return Expr::num(std::tan(a));
                if (name == "exp")  return Expr::num(std::exp(a));
                if (name == "ln")   return Expr::num(std::log(a));
                if (name == "log")  return Expr::num(std::log10(a));
                if (name == "sqrt") return Expr::num(std::sqrt(a));
                if (name == "abs")  return Expr::num(std::abs(a));
            }
            return Expr::func(node.name, arg);
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            auto l = simplify(node.lhs);
            auto r = simplify(node.rhs);

            // Constant folding
            if (is_num(l) && is_num(r)) {
                double lv = get_num(l), rv = get_num(r);
                switch (node.op) {
                    case '+': return Expr::num(lv + rv);
                    case '-': return Expr::num(lv - rv);
                    case '*': return Expr::num(lv * rv);
                    case '/': return Expr::num(lv / rv);
                    case '^': return Expr::num(std::pow(lv, rv));
                }
            }

            switch (node.op) {
                case '+':
                    if (is_num(l, 0)) return r;     // 0 + x = x
                    if (is_num(r, 0)) return l;     // x + 0 = x
                    break;
                case '-':
                    if (is_num(r, 0)) return l;     // x - 0 = x
                    if (is_num(l, 0)) return Expr::unary(r); // 0 - x = -x
                    break;
                case '*':
                    if (is_num(l, 0) || is_num(r, 0)) return Expr::num(0);
                    if (is_num(l, 1)) return r;     // 1 * x = x
                    if (is_num(r, 1)) return l;     // x * 1 = x
                    if (is_num(l, -1)) return Expr::unary(r);
                    if (is_num(r, -1)) return Expr::unary(l);
                    break;
                case '/':
                    if (is_num(l, 0)) return Expr::num(0); // 0 / x = 0
                    if (is_num(r, 1)) return l;     // x / 1 = x
                    break;
                case '^':
                    if (is_num(r, 0)) return Expr::num(1); // x^0 = 1
                    if (is_num(r, 1)) return l;     // x^1 = x
                    if (is_num(l, 0)) return Expr::num(0); // 0^x = 0
                    if (is_num(l, 1)) return Expr::num(1); // 1^x = 1
                    break;
            }

            return Expr::binop(node.op, l, r);
        }
        else {
            return expr;
        }
    }, expr->node);
}

} // namespace mathengine
