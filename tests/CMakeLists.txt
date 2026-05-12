add_executable(math_tests
    test_parser.cpp
    test_eval.cpp
    test_simplify.cpp
    test_calculus.cpp
    test_solver.cpp
    test_matrix.cpp
    test_linalg.cpp
    test_polynomial.cpp
    test_algebra.cpp
    test_symbolic_solve.cpp
    test_taylor.cpp
    test_limits.cpp
    test_desmos.cpp
)

target_link_libraries(math_tests PRIVATE
    mathengine
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(math_tests)
