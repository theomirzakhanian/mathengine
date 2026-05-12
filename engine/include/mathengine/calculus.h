#pragma once

#include "expr.h"
#include <string>

namespace mathengine {

// Symbolic differentiation of expr with respect to var
Expr::Ptr differentiate(const Expr::Ptr& expr, const std::string& var);

// Symbolic integration (best-effort, returns nullptr if unable)
Expr::Ptr integrate(const Expr::Ptr& expr, const std::string& var);

} // namespace mathengine
