#pragma once

#include "matrix.h"
#include <vector>

namespace mathengine {

struct LU { Matrix L, U; std::vector<size_t> perm; };
struct QR { Matrix Q, R; };
struct EigenResult { std::vector<double> eigenvalues; Matrix eigenvectors; };

LU          lu_decompose(const Matrix& A);
QR          qr_decompose(const Matrix& A);
double      determinant(const Matrix& A);
Matrix      inverse(const Matrix& A);
Matrix      solve(const Matrix& A, const Matrix& b);
EigenResult eigenvalues(const Matrix& A);

} // namespace mathengine
