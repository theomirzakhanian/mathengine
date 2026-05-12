#include "mathengine/eval.h"
#include <cmath>
#include <stdexcept>
#include <array>

namespace mathengine {

// --- Tree-walk evaluator ---

double evaluate(const Expr::Ptr& expr, const VarMap& vars) {
    if (!expr) throw std::runtime_error("Null expression");

    return std::visit([&](const auto& node) -> double {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            return node.value;
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            auto it = vars.find(node.name);
            if (it == vars.end())
                throw std::runtime_error("Undefined variable: " + node.name);
            return it->second;
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            double l = evaluate(node.lhs, vars);
            double r = evaluate(node.rhs, vars);
            switch (node.op) {
                case '+': return l + r;
                case '-': return l - r;
                case '*': return l * r;
                case '/': return l / r;
                case '^': return std::pow(l, r);
                default: throw std::runtime_error("Unknown operator");
            }
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            return -evaluate(node.operand, vars);
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            double a = evaluate(node.arg, vars);
            const auto& name = node.name;
            if (name == "sin")  return std::sin(a);
            if (name == "cos")  return std::cos(a);
            if (name == "tan")  return std::tan(a);
            if (name == "asin") return std::asin(a);
            if (name == "acos") return std::acos(a);
            if (name == "atan") return std::atan(a);
            if (name == "exp")  return std::exp(a);
            if (name == "ln")   return std::log(a);
            if (name == "log")  return std::log10(a);
            if (name == "sqrt") return std::sqrt(a);
            if (name == "abs")  return std::abs(a);
            if (name == "sinh") return std::sinh(a);
            if (name == "cosh") return std::cosh(a);
            if (name == "tanh") return std::tanh(a);
            throw std::runtime_error("Unknown function: " + name);
        }
        else {
            throw std::runtime_error("Unknown node type");
        }
    }, expr->node);
}

// --- Bytecode-compiled evaluator ---

void CompiledExpr::compile(const Expr::Ptr& expr) {
    if (!expr) throw std::runtime_error("Null expression");

    std::visit([this](const auto& node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, Expr::Num>) {
            program_.push_back({Op::PushConst, node.value, {}});
        }
        else if constexpr (std::is_same_v<T, Expr::Var>) {
            if (node.name == "x") {
                program_.push_back({Op::PushX, 0.0, {}});
            } else {
                program_.push_back({Op::PushVar, 0.0, node.name});
            }
        }
        else if constexpr (std::is_same_v<T, Expr::BinOp>) {
            compile(node.lhs);
            compile(node.rhs);
            switch (node.op) {
                case '+': program_.push_back({Op::Add}); break;
                case '-': program_.push_back({Op::Sub}); break;
                case '*': program_.push_back({Op::Mul}); break;
                case '/': program_.push_back({Op::Div}); break;
                case '^': program_.push_back({Op::Pow}); break;
            }
        }
        else if constexpr (std::is_same_v<T, Expr::Unary>) {
            compile(node.operand);
            program_.push_back({Op::Neg});
        }
        else if constexpr (std::is_same_v<T, Expr::Func>) {
            compile(node.arg);
            const auto& name = node.name;
            if (name == "sin")       program_.push_back({Op::Sin});
            else if (name == "cos")  program_.push_back({Op::Cos});
            else if (name == "tan")  program_.push_back({Op::Tan});
            else if (name == "asin") program_.push_back({Op::Asin});
            else if (name == "acos") program_.push_back({Op::Acos});
            else if (name == "atan") program_.push_back({Op::Atan});
            else if (name == "exp")  program_.push_back({Op::Exp});
            else if (name == "ln")   program_.push_back({Op::Ln});
            else if (name == "log")  program_.push_back({Op::Log});
            else if (name == "sqrt") program_.push_back({Op::Sqrt});
            else if (name == "abs")  program_.push_back({Op::Abs});
            else throw std::runtime_error("Unknown function: " + name);
        }
    }, expr->node);
}

CompiledExpr::CompiledExpr(const Expr::Ptr& expr) {
    compile(expr);
}

double CompiledExpr::eval(double x) const {
    std::array<double, 64> stack;
    int sp = -1;

    for (const auto& inst : program_) {
        switch (inst.op) {
            case Op::PushConst: stack[++sp] = inst.operand; break;
            case Op::PushX:     stack[++sp] = x; break;
            case Op::PushVar:   stack[++sp] = 0.0; break; // fallback
            case Op::Add: { double r = stack[sp--]; stack[sp] += r; break; }
            case Op::Sub: { double r = stack[sp--]; stack[sp] -= r; break; }
            case Op::Mul: { double r = stack[sp--]; stack[sp] *= r; break; }
            case Op::Div: { double r = stack[sp--]; stack[sp] /= r; break; }
            case Op::Pow: { double r = stack[sp--]; stack[sp] = std::pow(stack[sp], r); break; }
            case Op::Neg: stack[sp] = -stack[sp]; break;
            case Op::Sin:  stack[sp] = std::sin(stack[sp]); break;
            case Op::Cos:  stack[sp] = std::cos(stack[sp]); break;
            case Op::Tan:  stack[sp] = std::tan(stack[sp]); break;
            case Op::Asin: stack[sp] = std::asin(stack[sp]); break;
            case Op::Acos: stack[sp] = std::acos(stack[sp]); break;
            case Op::Atan: stack[sp] = std::atan(stack[sp]); break;
            case Op::Exp:  stack[sp] = std::exp(stack[sp]); break;
            case Op::Ln:   stack[sp] = std::log(stack[sp]); break;
            case Op::Log:  stack[sp] = std::log10(stack[sp]); break;
            case Op::Sqrt: stack[sp] = std::sqrt(stack[sp]); break;
            case Op::Abs:  stack[sp] = std::abs(stack[sp]); break;
        }
    }

    return (sp >= 0) ? stack[0] : 0.0;
}

double CompiledExpr::eval(const VarMap& vars) const {
    std::array<double, 64> stack;
    int sp = -1;

    for (const auto& inst : program_) {
        switch (inst.op) {
            case Op::PushConst: stack[++sp] = inst.operand; break;
            case Op::PushX: {
                auto it = vars.find("x");
                stack[++sp] = (it != vars.end()) ? it->second : 0.0;
                break;
            }
            case Op::PushVar: {
                auto it = vars.find(inst.var_name);
                stack[++sp] = (it != vars.end()) ? it->second : 0.0;
                break;
            }
            case Op::Add: { double r = stack[sp--]; stack[sp] += r; break; }
            case Op::Sub: { double r = stack[sp--]; stack[sp] -= r; break; }
            case Op::Mul: { double r = stack[sp--]; stack[sp] *= r; break; }
            case Op::Div: { double r = stack[sp--]; stack[sp] /= r; break; }
            case Op::Pow: { double r = stack[sp--]; stack[sp] = std::pow(stack[sp], r); break; }
            case Op::Neg: stack[sp] = -stack[sp]; break;
            case Op::Sin:  stack[sp] = std::sin(stack[sp]); break;
            case Op::Cos:  stack[sp] = std::cos(stack[sp]); break;
            case Op::Tan:  stack[sp] = std::tan(stack[sp]); break;
            case Op::Asin: stack[sp] = std::asin(stack[sp]); break;
            case Op::Acos: stack[sp] = std::acos(stack[sp]); break;
            case Op::Atan: stack[sp] = std::atan(stack[sp]); break;
            case Op::Exp:  stack[sp] = std::exp(stack[sp]); break;
            case Op::Ln:   stack[sp] = std::log(stack[sp]); break;
            case Op::Log:  stack[sp] = std::log10(stack[sp]); break;
            case Op::Sqrt: stack[sp] = std::sqrt(stack[sp]); break;
            case Op::Abs:  stack[sp] = std::abs(stack[sp]); break;
        }
    }

    return (sp >= 0) ? stack[0] : 0.0;
}

} // namespace mathengine
