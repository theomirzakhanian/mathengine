#include "mathengine/linalg.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace mathengine {

LU lu_decompose(const Matrix& A) {
    size_t n = A.rows();
    if (n != A.cols()) throw std::runtime_error("LU requires square matrix");

    Matrix L = Matrix::identity(n);
    Matrix U = A;
    std::vector<size_t> perm(n);
    for (size_t i = 0; i < n; i++) perm[i] = i;

    for (size_t k = 0; k < n; k++) {
        // Partial pivoting
        size_t max_row = k;
        double max_val = std::abs(U(k, k));
        for (size_t i = k + 1; i < n; i++) {
            if (std::abs(U(i, k)) > max_val) {
                max_val = std::abs(U(i, k));
                max_row = i;
            }
        }
        if (max_row != k) {
            std::swap(perm[k], perm[max_row]);
            for (size_t j = 0; j < n; j++) {
                std::swap(U(k, j), U(max_row, j));
            }
            for (size_t j = 0; j < k; j++) {
                std::swap(L(k, j), L(max_row, j));
            }
        }

        if (std::abs(U(k, k)) < 1e-15) continue;

        for (size_t i = k + 1; i < n; i++) {
            L(i, k) = U(i, k) / U(k, k);
            for (size_t j = k; j < n; j++) {
                U(i, j) -= L(i, k) * U(k, j);
            }
        }
    }

    return {L, U, perm};
}

QR qr_decompose(const Matrix& A) {
    size_t m = A.rows(), n = A.cols();
    Matrix Q = Matrix::identity(m);
    Matrix R = A;

    for (size_t k = 0; k < std::min(m - 1, n); k++) {
        // Compute Householder vector
        double norm = 0;
        for (size_t i = k; i < m; i++)
            norm += R(i, k) * R(i, k);
        norm = std::sqrt(norm);

        if (norm < 1e-15) continue;

        double sign = (R(k, k) >= 0) ? 1.0 : -1.0;
        double alpha = -sign * norm;

        std::vector<double> v(m, 0.0);
        v[k] = R(k, k) - alpha;
        for (size_t i = k + 1; i < m; i++)
            v[i] = R(i, k);

        double v_norm = 0;
        for (size_t i = k; i < m; i++)
            v_norm += v[i] * v[i];
        if (v_norm < 1e-30) continue;

        // Apply H = I - 2*v*v'/||v||^2 to R
        for (size_t j = k; j < n; j++) {
            double dot = 0;
            for (size_t i = k; i < m; i++)
                dot += v[i] * R(i, j);
            for (size_t i = k; i < m; i++)
                R(i, j) -= 2.0 * v[i] * dot / v_norm;
        }

        // Apply H to Q
        for (size_t j = 0; j < m; j++) {
            double dot = 0;
            for (size_t i = k; i < m; i++)
                dot += v[i] * Q(i, j);
            for (size_t i = k; i < m; i++)
                Q(i, j) -= 2.0 * v[i] * dot / v_norm;
        }
    }

    Q = Q.transpose();
    return {Q, R};
}

double determinant(const Matrix& A) {
    auto [L, U, perm] = lu_decompose(A);
    double det = 1.0;
    for (size_t i = 0; i < A.rows(); i++)
        det *= U(i, i);
    // Count permutation sign
    int swaps = 0;
    std::vector<size_t> p = perm;
    for (size_t i = 0; i < p.size(); i++) {
        while (p[i] != i) {
            std::swap(p[i], p[p[i]]);
            swaps++;
        }
    }
    return (swaps % 2 == 0) ? det : -det;
}

Matrix inverse(const Matrix& A) {
    size_t n = A.rows();
    if (n != A.cols()) throw std::runtime_error("inverse requires square matrix");

    // Solve A * X = I column by column
    Matrix result(n, n);
    for (size_t col = 0; col < n; col++) {
        Matrix e(n, 1);
        e(col, 0) = 1.0;
        Matrix x = solve(A, e);
        for (size_t row = 0; row < n; row++)
            result(row, col) = x(row, 0);
    }
    return result;
}

Matrix solve(const Matrix& A, const Matrix& b) {
    size_t n = A.rows();
    auto [L, U, perm] = lu_decompose(A);

    // Permute b
    Matrix pb(n, 1);
    for (size_t i = 0; i < n; i++)
        pb(i, 0) = b(perm[i], 0);

    // Forward substitution: L * y = pb
    Matrix y(n, 1);
    for (size_t i = 0; i < n; i++) {
        y(i, 0) = pb(i, 0);
        for (size_t j = 0; j < i; j++)
            y(i, 0) -= L(i, j) * y(j, 0);
    }

    // Back substitution: U * x = y
    Matrix x(n, 1);
    for (int i = (int)n - 1; i >= 0; i--) {
        x(i, 0) = y(i, 0);
        for (size_t j = i + 1; j < n; j++)
            x(i, 0) -= U(i, j) * x(j, 0);
        if (std::abs(U(i, i)) < 1e-15)
            throw std::runtime_error("Matrix is singular");
        x(i, 0) /= U(i, i);
    }
    return x;
}

EigenResult eigenvalues(const Matrix& A) {
    size_t n = A.rows();
    if (n != A.cols()) throw std::runtime_error("eigenvalues requires square matrix");

    // QR iteration
    Matrix T = A;
    Matrix V = Matrix::identity(n);

    for (int iter = 0; iter < 200; iter++) {
        auto [Q, R] = qr_decompose(T);
        T = R * Q;
        V = V * Q;

        // Check convergence (sub-diagonal elements)
        double off = 0;
        for (size_t i = 1; i < n; i++)
            off += std::abs(T(i, i - 1));
        if (off < 1e-10 * n) break;
    }

    std::vector<double> evals(n);
    for (size_t i = 0; i < n; i++)
        evals[i] = T(i, i);

    return {evals, V};
}

} // namespace mathengine
