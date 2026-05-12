#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace mathengine {

struct Expr {
    using Ptr = std::shared_ptr<Expr>;

    struct Num   { double value; };
    struct Var   { std::string name; };
    struct BinOp { char op; Ptr lhs, rhs; };  // +, -, *, /, ^
    struct Unary { Ptr operand; };             // negation
    struct Func  { std::string name; Ptr arg; };

    std::variant<Num, Var, BinOp, Unary, Func> node;

    // Factory helpers
    static Ptr num(double v) {
        return std::make_shared<Expr>(Expr{Num{v}});
    }
    static Ptr var(const std::string& name) {
        return std::make_shared<Expr>(Expr{Var{name}});
    }
    static Ptr binop(char op, Ptr lhs, Ptr rhs) {
        return std::make_shared<Expr>(Expr{BinOp{op, std::move(lhs), std::move(rhs)}});
    }
    static Ptr unary(Ptr operand) {
        return std::make_shared<Expr>(Expr{Unary{std::move(operand)}});
    }
    static Ptr func(const std::string& name, Ptr arg) {
        return std::make_shared<Expr>(Expr{Func{name, std::move(arg)}});
    }
};

// Pretty-print an expression to string
std::string to_string(const Expr::Ptr& expr);

} // namespace mathengine
