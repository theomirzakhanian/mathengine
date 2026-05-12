#pragma once

#include "expr.h"

namespace mathengine {

// Minimal-parentheses pretty print
std::string pretty_string(const Expr::Ptr& expr);

} // namespace mathengine
