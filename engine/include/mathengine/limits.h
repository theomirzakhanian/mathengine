#pragma once

#include "expr.h"
#include <string>

namespace mathengine {

// Compute limit of expr as var -> point.
// point_from: 0 = two-sided, -1 = left, +1 = right
Expr::Ptr limit(const Expr::Ptr& expr, const std::string& var,
                double point, int point_from = 0);

} // namespace mathengine
