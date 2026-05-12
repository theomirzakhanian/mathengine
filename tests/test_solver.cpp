#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/parser.h"
#include "mathengine/solver.h"

using namespace mathengine;

TEST_CASE("Bisection: x^2 - 4", "[solver]") {
    auto expr = parse("x^2 - 4");
    CompiledExpr compiled(expr);
    auto result = bisect(compiled, 0, 3);
    REQUIRE(result.converged);
    REQUIRE_THAT(result.root, Catch::Matchers::WithinAbs(2.0, 1e-10));
}

TEST_CASE("Newton: x^2 - 4", "[solver]") {
    auto expr = parse("x^2 - 4");
    auto result = newton(expr, "x", 3.0);
    REQUIRE(result.converged);
    REQUIRE_THAT(result.root, Catch::Matchers::WithinAbs(2.0, 1e-10));
}

TEST_CASE("Bisection: sin(x) near pi", "[solver]") {
    auto expr = parse("sin(x)");
    CompiledExpr compiled(expr);
    auto result = bisect(compiled, 2, 4);
    REQUIRE(result.converged);
    REQUIRE_THAT(result.root, Catch::Matchers::WithinAbs(3.14159265358979, 1e-10));
}
