#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/limits.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"

using namespace mathengine;

TEST_CASE("Limit: direct substitution", "[limits]") {
    auto expr = parse("x^2 + 1");
    auto result = limit(expr, "x", 2.0);
    REQUIRE(result != nullptr);
    REQUIRE_THAT(evaluate(result), Catch::Matchers::WithinAbs(5.0, 1e-10));
}

TEST_CASE("Limit: sin(x)/x as x->0", "[limits]") {
    auto expr = parse("sin(x)/x");
    auto result = limit(expr, "x", 0.0);
    REQUIRE(result != nullptr);
    REQUIRE_THAT(evaluate(result), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("Limit: (x^2-1)/(x-1) as x->1", "[limits]") {
    auto expr = parse("(x^2-1)/(x-1)");
    auto result = limit(expr, "x", 1.0);
    REQUIRE(result != nullptr);
    REQUIRE_THAT(evaluate(result), Catch::Matchers::WithinAbs(2.0, 1e-6));
}

TEST_CASE("Limit: (exp(x)-1)/x as x->0", "[limits]") {
    auto expr = parse("(exp(x)-1)/x");
    auto result = limit(expr, "x", 0.0);
    REQUIRE(result != nullptr);
    REQUIRE_THAT(evaluate(result), Catch::Matchers::WithinAbs(1.0, 1e-6));
}

TEST_CASE("Limit: tan(x)/x as x->0", "[limits]") {
    auto expr = parse("tan(x)/x");
    auto result = limit(expr, "x", 0.0);
    REQUIRE(result != nullptr);
    REQUIRE_THAT(evaluate(result), Catch::Matchers::WithinAbs(1.0, 1e-10));
}
