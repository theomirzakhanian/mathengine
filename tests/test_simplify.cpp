#include <catch2/catch_test_macros.hpp>
#include "mathengine/parser.h"
#include "mathengine/simplify.h"
#include "mathengine/eval.h"

using namespace mathengine;

TEST_CASE("Simplify x + 0", "[simplify]") {
    auto expr = parse("x + 0");
    auto result = simplify(expr);
    REQUIRE(evaluate(result, {{"x", 5.0}}) == 5.0);
    REQUIRE(to_string(result) == "x");
}

TEST_CASE("Simplify 0 * x", "[simplify]") {
    auto result = simplify(parse("0 * x"));
    REQUIRE(to_string(result) == "0");
}

TEST_CASE("Simplify 1 * x", "[simplify]") {
    auto result = simplify(parse("1 * x"));
    REQUIRE(to_string(result) == "x");
}

TEST_CASE("Simplify x^0", "[simplify]") {
    auto result = simplify(parse("x^0"));
    REQUIRE(to_string(result) == "1");
}

TEST_CASE("Simplify x^1", "[simplify]") {
    auto result = simplify(parse("x^1"));
    REQUIRE(to_string(result) == "x");
}

TEST_CASE("Simplify constant folding", "[simplify]") {
    auto result = simplify(parse("2 + 3"));
    REQUIRE(to_string(result) == "5");
}
