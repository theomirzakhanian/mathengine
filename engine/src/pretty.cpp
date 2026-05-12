#include "mathengine/pretty.h"
#include <cmath>
#include <sstream>

namespace mathengine {

namespace {

// Precedence levels
int precedence(const Expr::Ptr& expr) {
    if (!expr) return 100;
    return std::visit([](const auto& node) -> int {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Expr::Num>) return 100;
        else if constexpr (std::is_same_v<T, Expr::Var>) return 100;
        else if constexpr (std::is_same_v<T, Expr::Func>) return 100;
        else if constexpr (std::is_same_v<T, Expr::Unary>) return 50;
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            switch (node.op) {
                case '+': case '-': return 10;
                case '*': case '/': return 20;
                case '^': return 30;
                default: return 0;
            }
        }
        else return 0;
    }, expr->node);
}

std::string format_num(double v) {
    if (v == (int)v && std::abs(v) < 1e12) {
        return std::to_string((long long)v);
    }
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

std::string pp(const Expr::Ptr& expr, int parent_prec, bool is_right_of_minus_or_div) {
    if (!expr) return "?";

    return std::visit([&](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            if (node.value < 0 && parent_prec > 10)
                return "(" + format_num(node.value) + ")";
            return format_num(node.value);
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            return node.name;
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            std::string inner = pp(node.operand, 50, false);
            if (parent_prec > 10)
                return "(-" + inner + ")";
            return "-" + inner;
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            return node.name + "(" + pp(node.arg, 0, false) + ")";
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            int my_prec = precedence(expr);
            std::string l = pp(node.lhs, my_prec, false);
            bool right_needs_extra = (node.op == '-' || node.op == '/');
            std::string r = pp(node.rhs, my_prec, right_needs_extra);

            // Right operand of - or / needs parens if same precedence
            if (right_needs_extra && precedence(node.rhs) == my_prec)
                r = "(" + pp(node.rhs, 0, false) + ")";

            std::string op_str;
            switch (node.op) {
                case '+': op_str = " + "; break;
                case '-': op_str = " - "; break;
                case '*': op_str = "*"; break;
                case '/': op_str = "/"; break;
                case '^': op_str = "^"; break;
                default: op_str = "?"; break;
            }

            std::string result = l + op_str + r;

            if (my_prec < parent_prec)
                return "(" + result + ")";
            if (is_right_of_minus_or_div && my_prec == parent_prec)
                return "(" + result + ")";

            return result;
        }
        else {
            return "?";
        }
    }, expr->node);
}

} // anonymous namespace

std::string pretty_string(const Expr::Ptr& expr) {
    return pp(expr, 0, false);
}

} // namespace mathengine
