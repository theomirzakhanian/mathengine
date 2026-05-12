add_library(mathengine STATIC
    src/parser.cpp
    src/eval.cpp
    src/simplify.cpp
    src/calculus.cpp
    src/solver.cpp
    src/ode.cpp
    src/matrix.cpp
    src/linalg.cpp
    src/polynomial.cpp
    src/algebra.cpp
    src/symbolic_solve.cpp
    src/taylor.cpp
    src/limits.cpp
    src/pretty.cpp
    src/steps.cpp
    src/complex_num.cpp
    src/number_theory.cpp
    src/statistics.cpp
    src/units.cpp
    src/design.cpp
)

target_include_directories(mathengine PUBLIC include)
target_compile_features(mathengine PUBLIC cxx_std_17)
