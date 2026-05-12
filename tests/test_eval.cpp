#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/parser.h"
#include "mathengine/eval.h"
#include <cmath>

using namespace mathengine;

TEST_CASE("CompiledExpr basic", "[eval]") {
    auto expr = parse("x^2 + 1");
    CompiledExpr compiled(expr);
    REQUIRE(compiled.eval(3.0) == 10.0);
    REQUIRE(compiled.eval(0.0) == 1.0);
    REQUIRE(compiled.eval(-2.0) == 5.0);
}

TEST_CASE("CompiledExpr trig", "[eval]") {
    auto expr = parse("sin(x)");
    CompiledExpr compiled(expr);
    REQUIRE_THAT(compiled.eval(0.0), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(compiled.eval(M_PI / 2), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("CompiledExpr complex", "[eval]") {
    auto expr = parse("sin(x^2) + cos(2*x)");
    CompiledExpr compiled(expr);
    double x = 1.5;
    double expected = std::sin(x*x) + std::cos(2*x);
    REQUIRE_THAT(compiled.eval(x), Catch::Matchers::WithinAbs(expected, 1e-10));
}

TEST_CASE("Evaluate all functions", "[eval]") {
    REQUIRE_THAT(evaluate(parse("exp(0)")), Catch::Matchers::WithinAbs(1.0, 1e-10));
    REQUIRE_THAT(evaluate(parse("ln(1)")), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(evaluate(parse("sqrt(4)")), Catch::Matchers::WithinAbs(2.0, 1e-10));
    REQUIRE_THAT(evaluate(parse("abs(-5)")), Catch::Matchers::WithinAbs(5.0, 1e-10));
}
