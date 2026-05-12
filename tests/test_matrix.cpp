#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "mathengine/matrix.h"

using namespace mathengine;

TEST_CASE("Matrix creation", "[matrix]") {
    Matrix m(2, 2, {1, 2, 3, 4});
    REQUIRE(m(0, 0) == 1);
    REQUIRE(m(0, 1) == 2);
    REQUIRE(m(1, 0) == 3);
    REQUIRE(m(1, 1) == 4);
}

TEST_CASE("Matrix addition", "[matrix]") {
    Matrix a(2, 2, {1, 2, 3, 4});
    Matrix b(2, 2, {5, 6, 7, 8});
    auto c = a + b;
    REQUIRE(c(0, 0) == 6);
    REQUIRE(c(1, 1) == 12);
}

TEST_CASE("Matrix multiplication", "[matrix]") {
    Matrix a(2, 2, {1, 2, 3, 4});
    Matrix b(2, 2, {5, 6, 7, 8});
    auto c = a * b;
    REQUIRE(c(0, 0) == 19);
    REQUIRE(c(0, 1) == 22);
    REQUIRE(c(1, 0) == 43);
    REQUIRE(c(1, 1) == 50);
}

TEST_CASE("Matrix transpose", "[matrix]") {
    Matrix m(2, 3, {1, 2, 3, 4, 5, 6});
    auto t = m.transpose();
    REQUIRE(t.rows() == 3);
    REQUIRE(t.cols() == 2);
    REQUIRE(t(0, 0) == 1);
    REQUIRE(t(1, 0) == 2);
    REQUIRE(t(0, 1) == 4);
}

TEST_CASE("Matrix identity", "[matrix]") {
    auto I = Matrix::identity(3);
    REQUIRE(I(0, 0) == 1);
    REQUIRE(I(1, 1) == 1);
    REQUIRE(I(2, 2) == 1);
    REQUIRE(I(0, 1) == 0);
}
