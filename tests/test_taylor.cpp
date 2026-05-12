#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/taylor.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"

using namespace mathengine;

TEST_CASE("Taylor sin(x) at 0, order 5", "[taylor]") {
    auto expr = parse("sin(x)");
    auto series = taylor(expr, "x", 0.0, 5);
    // sin(x) ~ x - x^3/6 + x^5/120
    REQUIRE_THAT(evaluate(series, {{"x", 0.0}}), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(evaluate(series, {{"x", 0.1}}), Catch::Matchers::WithinAbs(std::sin(0.1), 1e-8));
    REQUIRE_THAT(evaluate(series, {{"x", 0.5}}), Catch::Matchers::WithinAbs(std::sin(0.5), 1e-4));
}

TEST_CASE("Taylor exp(x) at 0, order 4", "[taylor]") {
    auto expr = parse("exp(x)");
    auto series = taylor(expr, "x", 0.0, 4);
    // e^x ~ 1 + x + x^2/2 + x^3/6 + x^4/24
    REQUIRE_THAT(evaluate(series, {{"x", 0.0}}), Catch::Matchers::WithinAbs(1.0, 1e-10));
    REQUIRE_THAT(evaluate(series, {{"x", 0.5}}), Catch::Matchers::WithinAbs(std::exp(0.5), 1e-3));
}

TEST_CASE("Taylor cos(x) at 0, order 4", "[taylor]") {
    auto expr = parse("cos(x)");
    auto series = taylor(expr, "x", 0.0, 4);
    REQUIRE_THAT(evaluate(series, {{"x", 0.0}}), Catch::Matchers::WithinAbs(1.0, 1e-10));
    REQUIRE_THAT(evaluate(series, {{"x", 0.3}}), Catch::Matchers::WithinAbs(std::cos(0.3), 1e-5));
}

TEST_CASE("Taylor around non-zero center", "[taylor]") {
    auto expr = parse("exp(x)");
    auto series = taylor(expr, "x", 1.0, 3);
    // Should approximate exp(x) near x=1
    REQUIRE_THAT(evaluate(series, {{"x", 1.0}}), Catch::Matchers::WithinAbs(std::exp(1.0), 1e-10));
    REQUIRE_THAT(evaluate(series, {{"x", 1.1}}), Catch::Matchers::WithinAbs(std::exp(1.1), 1e-3));
}
