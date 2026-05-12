#pragma once

#include "expr.h"
#include <string>
#include <stdexcept>

namespace mathengine {

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Parse a math expression string into an AST.
// Throws ParseError on invalid input.
// Supported: +, -, *, /, ^ (right-assoc), unary minus,
//   parentheses, variables, functions (sin, cos, tan, exp, ln, sqrt, abs, asin, acos, atan, log)
// Implicit multiplication: 2x, 2(x+1)
// Constants: pi, e
Expr::Ptr parse(const std::string& input);

} // namespace mathengine
