#pragma once

#include "expr.h"
#include <vector>
#include <string>

namespace mathengine {

struct Step {
    std::string rule;       // e.g. "Power rule", "Chain rule"
    std::string expression; // the expression at this step
};

using StepLog = std::vector<Step>;

// Differentiate with step-by-step logging
Expr::Ptr differentiate_steps(const Expr::Ptr& expr, const std::string& var, StepLog& steps);

// Simplify with step-by-step logging
Expr::Ptr simplify_steps(const Expr::Ptr& expr, StepLog& steps);

// Solve with step-by-step logging
std::vector<Expr::Ptr> solve_steps(const Expr::Ptr& expr, const std::string& var, StepLog& steps);

} // namespace mathengine
