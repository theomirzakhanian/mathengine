#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/polynomial.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"

using namespace mathengine;

TEST_CASE("Polynomial construction", "[polynomial]") {
    Polynomial p({1, 2, 3}); // 1 + 2x + 3x^2
    REQUIRE(p.degree() == 2);
    REQUIRE(p.coeff(0) == 1.0);
    REQUIRE(p.coeff(1) == 2.0);
    REQUIRE(p.coeff(2) == 3.0);
}

TEST_CASE("Polynomial eval (Horner)", "[polynomial]") {
    Polynomial p({1, 0, 1}); // 1 + x^2
    REQUIRE(p.eval(0) == 1.0);
    REQUIRE(p.eval(2) == 5.0);
    REQUIRE(p.eval(-3) == 10.0);
}

TEST_CASE("Polynomial addition", "[polynomial]") {
    Polynomial a({1, 2}); // 1 + 2x
    Polynomial b({3, 0, 1}); // 3 + x^2
    auto c = a + b;
    REQUIRE(c.coeff(0) == 4.0);
    REQUIRE(c.coeff(1) == 2.0);
    REQUIRE(c.coeff(2) == 1.0);
}

TEST_CASE("Polynomial multiplication", "[polynomial]") {
    Polynomial a({1, 1}); // 1 + x = (x+1)
    Polynomial b({-1, 1}); // -1 + x = (x-1)
    auto c = a * b; // x^2 - 1
    REQUIRE(c.degree() == 2);
    REQUIRE_THAT(c.coeff(0), Catch::Matchers::WithinAbs(-1.0, 1e-10));
    REQUIRE_THAT(c.coeff(1), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(c.coeff(2), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("Polynomial divmod", "[polynomial]") {
    Polynomial p({-1, 0, 1}); // x^2 - 1
    Polynomial d({-1, 1}); // x - 1
    auto [q, r] = p.divmod(d);
    // (x^2-1)/(x-1) = x+1 remainder 0
    REQUIRE(q.degree() == 1);
    REQUIRE_THAT(q.coeff(0), Catch::Matchers::WithinAbs(1.0, 1e-10));
    REQUIRE_THAT(q.coeff(1), Catch::Matchers::WithinAbs(1.0, 1e-10));
    REQUIRE_THAT(r.coeff(0), Catch::Matchers::WithinAbs(0.0, 1e-10));
}

TEST_CASE("expr_to_poly basic", "[polynomial]") {
    auto expr = parse("x^2 + 2*x + 1");
    auto poly = expr_to_poly(expr, "x");
    REQUIRE(poly.has_value());
    REQUIRE(poly->degree() == 2);
    REQUIRE_THAT(poly->coeff(0), Catch::Matchers::WithinAbs(1.0, 1e-10));
    REQUIRE_THAT(poly->coeff(1), Catch::Matchers::WithinAbs(2.0, 1e-10));
    REQUIRE_THAT(poly->coeff(2), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("expr_to_poly rejects non-polynomial", "[polynomial]") {
    auto expr = parse("sin(x)");
    auto poly = expr_to_poly(expr, "x");
    REQUIRE_FALSE(poly.has_value());
}

TEST_CASE("Polynomial to_expr roundtrip", "[polynomial]") {
    Polynomial p({-4, 0, 1}); // x^2 - 4
    auto expr = p.to_expr("x");
    REQUIRE(expr != nullptr);
    REQUIRE_THAT(evaluate(expr, {{"x", 3.0}}), Catch::Matchers::WithinAbs(5.0, 1e-10));
}
