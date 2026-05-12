#pragma once

#include "expr.h"
#include <string>

namespace mathengine {

// Distribute products over sums, expand integer powers
Expr::Ptr expand(const Expr::Ptr& expr);

// Factor a polynomial expression with respect to var
Expr::Ptr factor(const Expr::Ptr& expr, const std::string& var);

} // namespace mathengine
