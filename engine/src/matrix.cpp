#include "mathengine/matrix.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace mathengine {

Matrix::Matrix(size_t rows, size_t cols)
    : rows_(rows), cols_(cols), data_(rows * cols, 0.0) {}

Matrix::Matrix(size_t rows, size_t cols, std::initializer_list<double> data)
    : rows_(rows), cols_(cols), data_(data) {
    if (data_.size() != rows * cols)
        throw std::runtime_error("Matrix data size mismatch");
}

double& Matrix::operator()(size_t r, size_t c) {
    return data_[r * cols_ + c];
}

double Matrix::operator()(size_t r, size_t c) const {
    return data_[r * cols_ + c];
}

Matrix Matrix::operator+(const Matrix& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_)
        throw std::runtime_error("Matrix dimension mismatch for addition");
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); i++)
        result.data_[i] = data_[i] + other.data_[i];
    return result;
}

Matrix Matrix::operator-(const Matrix& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_)
        throw std::runtime_error("Matrix dimension mismatch for subtraction");
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); i++)
        result.data_[i] = data_[i] - other.data_[i];
    return result;
}

Matrix Matrix::operator*(const Matrix& other) const {
    if (cols_ != other.rows_)
        throw std::runtime_error("Matrix dimension mismatch for multiplication");
    Matrix result(rows_, other.cols_);
    for (size_t i = 0; i < rows_; i++)
        for (size_t k = 0; k < cols_; k++)
            for (size_t j = 0; j < other.cols_; j++)
                result(i, j) += (*this)(i, k) * other(k, j);
    return result;
}

Matrix Matrix::operator*(double scalar) const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); i++)
        result.data_[i] = data_[i] * scalar;
    return result;
}

Matrix Matrix::transpose() const {
    Matrix result(cols_, rows_);
    for (size_t i = 0; i < rows_; i++)
        for (size_t j = 0; j < cols_; j++)
            result(j, i) = (*this)(i, j);
    return result;
}

Matrix Matrix::identity(size_t n) {
    Matrix result(n, n);
    for (size_t i = 0; i < n; i++)
        result(i, i) = 1.0;
    return result;
}

std::string Matrix::to_string() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4);
    for (size_t i = 0; i < rows_; i++) {
        ss << "[ ";
        for (size_t j = 0; j < cols_; j++) {
            ss << std::setw(10) << (*this)(i, j);
            if (j + 1 < cols_) ss << ", ";
        }
        ss << " ]\n";
    }
    return ss.str();
}

} // namespace mathengine
