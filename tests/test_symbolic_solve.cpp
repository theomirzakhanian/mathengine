#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/symbolic_solve.h"
#include "mathengine/parser.h"
#include "mathengine/eval.h"

using namespace mathengine;

TEST_CASE("Solve linear: 2x + 6 = 0", "[solve]") {
    auto expr = parse("2*x + 6");
    auto roots = symbolic_solve(expr, "x");
    REQUIRE(roots.size() == 1);
    REQUIRE_THAT(evaluate(roots[0]), Catch::Matchers::WithinAbs(-3.0, 1e-10));
}

TEST_CASE("Solve quadratic: x^2 - 4 = 0", "[solve]") {
    auto expr = parse("x^2 - 4");
    auto roots = symbolic_solve(expr, "x");
    REQUIRE(roots.size() == 2);
    // Roots should be 2 and -2 (in some order)
    double r1 = evaluate(roots[0]);
    double r2 = evaluate(roots[1]);
    double lo = std::min(r1, r2), hi = std::max(r1, r2);
    REQUIRE_THAT(lo, Catch::Matchers::WithinAbs(-2.0, 1e-10));
    REQUIRE_THAT(hi, Catch::Matchers::WithinAbs(2.0, 1e-10));
}

TEST_CASE("Solve quadratic: x^2 + 2x - 8 = 0", "[solve]") {
    auto expr = parse("x^2 + 2*x - 8");
    auto roots = symbolic_solve(expr, "x");
    REQUIRE(roots.size() == 2);
    double r1 = evaluate(roots[0]);
    double r2 = evaluate(roots[1]);
    double lo = std::min(r1, r2), hi = std::max(r1, r2);
    REQUIRE_THAT(lo, Catch::Matchers::WithinAbs(-4.0, 1e-10));
    REQUIRE_THAT(hi, Catch::Matchers::WithinAbs(2.0, 1e-10));
}

TEST_CASE("Solve cubic: x^3 - 6x^2 + 11x - 6 = 0", "[solve]") {
    auto expr = parse("x^3 - 6*x^2 + 11*x - 6");
    auto roots = symbolic_solve(expr, "x");
    REQUIRE(roots.size() == 3);
    std::vector<double> vals;
    for (auto& r : roots) vals.push_back(evaluate(r));
    std::sort(vals.begin(), vals.end());
    REQUIRE_THAT(vals[0], Catch::Matchers::WithinAbs(1.0, 1e-8));
    REQUIRE_THAT(vals[1], Catch::Matchers::WithinAbs(2.0, 1e-8));
    REQUIRE_THAT(vals[2], Catch::Matchers::WithinAbs(3.0, 1e-8));
}

TEST_CASE("Solve linear system", "[solve]") {
    auto sol = solve_linear_system("2*x + 3*y = 5; x - y = 1");
    REQUIRE(sol.solved);
    REQUIRE(sol.var_names.size() == 2);
    // Find x and y indices
    double x_val = 0, y_val = 0;
    for (size_t i = 0; i < sol.var_names.size(); i++) {
        if (sol.var_names[i] == "x") x_val = sol.values[i];
        if (sol.var_names[i] == "y") y_val = sol.values[i];
    }
    // 2x + 3y = 5 and x - y = 1 => x = 8/5 = 1.6, y = 3/5 = 0.6
    REQUIRE_THAT(x_val, Catch::Matchers::WithinAbs(1.6, 1e-10));
    REQUIRE_THAT(y_val, Catch::Matchers::WithinAbs(0.6, 1e-10));
}
