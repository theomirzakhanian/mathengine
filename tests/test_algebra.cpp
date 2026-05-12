#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/algebra.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"
#include "mathengine/pretty.h"

using namespace mathengine;

TEST_CASE("Expand (x+1)*(x-1)", "[algebra]") {
    auto expr = parse("(x+1)*(x-1)");
    auto expanded = expand(expr);
    // Should be x^2 - 1
    REQUIRE_THAT(evaluate(expanded, {{"x", 0.0}}), Catch::Matchers::WithinAbs(-1.0, 1e-10));
    REQUIRE_THAT(evaluate(expanded, {{"x", 3.0}}), Catch::Matchers::WithinAbs(8.0, 1e-10));
}

TEST_CASE("Expand (x+1)^2", "[algebra]") {
    auto expr = parse("(x+1)^2");
    auto expanded = expand(expr);
    // Should be x^2 + 2x + 1
    REQUIRE_THAT(evaluate(expanded, {{"x", 0.0}}), Catch::Matchers::WithinAbs(1.0, 1e-10));
    REQUIRE_THAT(evaluate(expanded, {{"x", 2.0}}), Catch::Matchers::WithinAbs(9.0, 1e-10));
}

TEST_CASE("Expand (x+1)^3", "[algebra]") {
    auto expr = parse("(x+1)^3");
    auto expanded = expand(expr);
    REQUIRE_THAT(evaluate(expanded, {{"x", 1.0}}), Catch::Matchers::WithinAbs(8.0, 1e-10));
    REQUIRE_THAT(evaluate(expanded, {{"x", 2.0}}), Catch::Matchers::WithinAbs(27.0, 1e-10));
}

TEST_CASE("Factor x^2 - 4", "[algebra]") {
    auto expr = parse("x^2 - 4");
    auto factored = factor(expr, "x");
    // Should factor into (x-2)*(x+2) or equivalent
    REQUIRE_THAT(evaluate(factored, {{"x", 2.0}}), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(evaluate(factored, {{"x", -2.0}}), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(evaluate(factored, {{"x", 0.0}}), Catch::Matchers::WithinAbs(-4.0, 1e-10));
}

TEST_CASE("Factor x^2 + 2x + 1", "[algebra]") {
    auto expr = parse("x^2 + 2*x + 1");
    auto factored = factor(expr, "x");
    // (x+1)^2 — roots at x=-1
    REQUIRE_THAT(evaluate(factored, {{"x", -1.0}}), Catch::Matchers::WithinAbs(0.0, 1e-10));
}

TEST_CASE("Factor x^3 - 6*x^2 + 11*x - 6", "[algebra]") {
    auto expr = parse("x^3 - 6*x^2 + 11*x - 6");
    auto factored = factor(expr, "x");
    // Roots: 1, 2, 3
    REQUIRE_THAT(evaluate(factored, {{"x", 1.0}}), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(evaluate(factored, {{"x", 2.0}}), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(evaluate(factored, {{"x", 3.0}}), Catch::Matchers::WithinAbs(0.0, 1e-10));
}
