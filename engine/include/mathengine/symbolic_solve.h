#pragma once

#include "expr.h"
#include <vector>
#include <string>

namespace mathengine {

// Solve expr = 0 for var. Returns symbolic solutions.
std::vector<Expr::Ptr> symbolic_solve(const Expr::Ptr& expr, const std::string& var);

// System of linear equations
struct SystemSolution {
    std::vector<std::string> var_names;
    std::vector<double> values;
    bool solved = false;
    std::string error;
};

// Parse and solve "2x + 3y = 5; x - y = 1"
SystemSolution solve_linear_system(const std::string& input);

} // namespace mathengine
