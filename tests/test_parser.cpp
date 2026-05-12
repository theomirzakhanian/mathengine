#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/parser.h"
#include "mathengine/eval.h"

using namespace mathengine;

TEST_CASE("Parse simple number", "[parser]") {
    auto expr = parse("42");
    REQUIRE(evaluate(expr) == 42.0);
}

TEST_CASE("Parse addition", "[parser]") {
    auto expr = parse("2 + 3");
    REQUIRE(evaluate(expr) == 5.0);
}

TEST_CASE("Parse operator precedence", "[parser]") {
    auto expr = parse("2 + 3 * 4");
    REQUIRE(evaluate(expr) == 14.0);
}

TEST_CASE("Parse parentheses", "[parser]") {
    auto expr = parse("(2 + 3) * 4");
    REQUIRE(evaluate(expr) == 20.0);
}

TEST_CASE("Parse power (right-associative)", "[parser]") {
    auto expr = parse("2^3^2");
    // 2^(3^2) = 2^9 = 512
    REQUIRE(evaluate(expr) == 512.0);
}

TEST_CASE("Parse unary minus", "[parser]") {
    auto expr = parse("-5");
    REQUIRE(evaluate(expr) == -5.0);
}

TEST_CASE("Parse function", "[parser]") {
    auto expr = parse("sin(0)");
    REQUIRE_THAT(evaluate(expr), Catch::Matchers::WithinAbs(0.0, 1e-10));
}

TEST_CASE("Parse complex expression", "[parser]") {
    auto expr = parse("sin(3.14159265358979 / 2)");
    REQUIRE_THAT(evaluate(expr), Catch::Matchers::WithinAbs(1.0, 1e-6));
}

TEST_CASE("Parse variable", "[parser]") {
    auto expr = parse("x + 1");
    REQUIRE(evaluate(expr, {{"x", 5.0}}) == 6.0);
}

TEST_CASE("Parse implicit multiplication", "[parser]") {
    auto expr = parse("2x");
    REQUIRE(evaluate(expr, {{"x", 3.0}}) == 6.0);
}

TEST_CASE("Parse pi constant", "[parser]") {
    auto expr = parse("pi");
    REQUIRE_THAT(evaluate(expr), Catch::Matchers::WithinAbs(3.14159265358979, 1e-10));
}

TEST_CASE("Pretty print", "[parser]") {
    auto expr = parse("x + 1");
    auto s = to_string(expr);
    REQUIRE(!s.empty());
}
