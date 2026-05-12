#pragma once

#include <vector>
#include <cstddef>
#include <initializer_list>
#include <string>

namespace mathengine {

class Matrix {
public:
    Matrix(size_t rows, size_t cols);
    Matrix(size_t rows, size_t cols, std::initializer_list<double> data);

    double& operator()(size_t r, size_t c);
    double  operator()(size_t r, size_t c) const;

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }

    Matrix operator+(const Matrix& other) const;
    Matrix operator-(const Matrix& other) const;
    Matrix operator*(const Matrix& other) const;
    Matrix operator*(double scalar) const;
    Matrix transpose() const;

    static Matrix identity(size_t n);

    std::string to_string() const;

private:
    size_t rows_, cols_;
    std::vector<double> data_;
};

} // namespace mathengine
