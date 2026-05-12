#pragma once

#include "expr.h"
#include <string>

namespace mathengine {

// Taylor series of expr around var=center, up to given order.
Expr::Ptr taylor(const Expr::Ptr& expr, const std::string& var,
                 double center, int order);

} // namespace mathengine
