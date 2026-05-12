#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/linalg.h"

using namespace mathengine;

TEST_CASE("Determinant 2x2", "[linalg]") {
    Matrix m(2, 2, {1, 2, 3, 4});
    REQUIRE_THAT(determinant(m), Catch::Matchers::WithinAbs(-2.0, 1e-10));
}

TEST_CASE("Determinant 3x3", "[linalg]") {
    Matrix m(3, 3, {1, 2, 3, 0, 1, 4, 5, 6, 0});
    REQUIRE_THAT(determinant(m), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("Solve Ax=b", "[linalg]") {
    Matrix A(2, 2, {2, 1, 5, 3});
    Matrix b(2, 1, {4, 7});
    auto x = solve(A, b);
    REQUIRE_THAT(x(0, 0), Catch::Matchers::WithinAbs(5.0, 1e-10));
    REQUIRE_THAT(x(1, 0), Catch::Matchers::WithinAbs(-6.0, 1e-10));
}

TEST_CASE("Inverse 2x2", "[linalg]") {
    Matrix m(2, 2, {1, 2, 3, 4});
    auto inv = inverse(m);
    auto product = m * inv;
    REQUIRE_THAT(product(0, 0), Catch::Matchers::WithinAbs(1.0, 1e-10));
    REQUIRE_THAT(product(0, 1), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(product(1, 0), Catch::Matchers::WithinAbs(0.0, 1e-10));
    REQUIRE_THAT(product(1, 1), Catch::Matchers::WithinAbs(1.0, 1e-10));
}

TEST_CASE("Eigenvalues 2x2", "[linalg]") {
    Matrix m(2, 2, {2, 1, 1, 2});
    auto result = eigenvalues(m);
    // Eigenvalues should be 3 and 1
    auto& evals = result.eigenvalues;
    double e1 = std::min(evals[0], evals[1]);
    double e2 = std::max(evals[0], evals[1]);
    REQUIRE_THAT(e1, Catch::Matchers::WithinAbs(1.0, 1e-6));
    REQUIRE_THAT(e2, Catch::Matchers::WithinAbs(3.0, 1e-6));
}
