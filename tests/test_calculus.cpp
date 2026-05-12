#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/parser.h"
#include "mathengine/calculus.h"
#include "mathengine/eval.h"

using namespace mathengine;

TEST_CASE("Differentiate constant", "[calculus]") {
    auto result = differentiate(parse("5"), "x");
    REQUIRE(evaluate(result) == 0.0);
}

TEST_CASE("Differentiate x", "[calculus]") {
    auto result = differentiate(parse("x"), "x");
    REQUIRE(evaluate(result) == 1.0);
}

TEST_CASE("Differentiate x^2", "[calculus]") {
    auto result = differentiate(parse("x^2"), "x");
    // d/dx(x^2) = 2x
    REQUIRE_THAT(evaluate(result, {{"x", 3.0}}),
                 Catch::Matchers::WithinAbs(6.0, 1e-10));
}

TEST_CASE("Differentiate sin(x)", "[calculus]") {
    auto result = differentiate(parse("sin(x)"), "x");
    // d/dx(sin(x)) = cos(x)
    REQUIRE_THAT(evaluate(result, {{"x", 0.0}}),
                 Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("Differentiate product", "[calculus]") {
    auto result = differentiate(parse("x * sin(x)"), "x");
    // d/dx(x*sin(x)) = sin(x) + x*cos(x)
    double x = 1.0;
    double expected = std::sin(x) + x * std::cos(x);
    REQUIRE_THAT(evaluate(result, {{"x", x}}),
                 Catch::Matchers::WithinAbs(expected, 1e-8));
}

TEST_CASE("Integrate x", "[calculus]") {
    auto result = integrate(parse("x"), "x");
    REQUIRE(result != nullptr);
    // integral of x = x^2/2
    REQUIRE_THAT(evaluate(result, {{"x", 4.0}}),
                 Catch::Matchers::WithinAbs(8.0, 1e-10));
}
