#pragma once

#include "expr.h"

namespace mathengine {

// Simplify an expression using algebraic rewrite rules
Expr::Ptr simplify(const Expr::Ptr& expr);

} // namespace mathengine
