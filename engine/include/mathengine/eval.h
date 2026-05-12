#pragma once

#include "expr.h"
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace mathengine {

using VarMap = std::unordered_map<std::string, double>;

// Tree-walk evaluator — simple, general purpose
double evaluate(const Expr::Ptr& expr, const VarMap& vars = {});

// Bytecode-compiled evaluator — fast path for graphing
class CompiledExpr {
public:
    explicit CompiledExpr(const Expr::Ptr& expr);

    // Evaluate with x as the single variable
    double eval(double x) const;

    // Evaluate with named variables
    double eval(const VarMap& vars) const;

private:
    enum class Op : uint8_t {
        PushConst, PushX, PushVar,
        Add, Sub, Mul, Div, Pow, Neg,
        Sin, Cos, Tan, Exp, Ln, Sqrt, Abs,
        Asin, Acos, Atan, Log
    };

    struct Instruction {
        Op op;
        double operand = 0.0;        // for PushConst
        std::string var_name;         // for PushVar
    };

    std::vector<Instruction> program_;

    void compile(const Expr::Ptr& expr);
};

} // namespace mathengine
