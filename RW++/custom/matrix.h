#pragma once

#include <vector>
#include <stdexcept>
#include <iostream>

//This is a neat little trick - since using vector of vectors is a completely unnecessary overkill
//we instead make a single vector (we do not know the size of data at compilation) with total length
//corresponding to Xsize * Ysize, and then do some simple math to map [x][y] coordinates to 1D

class Matrix {
public:
    Matrix(size_t cols, size_t rows)
        : rows_(rows), cols_(cols), data_(rows * cols) {}

    int& at(size_t col, size_t row) {
        if (row >= rows_ || col >= cols_) {
            throw std::out_of_range("Matrix indices out of range.");
        }
        return data_[row * cols_ + col];
    }

    const int& at(size_t col, size_t row) const {
        if (row >= rows_ || col >= cols_) {
            throw std::out_of_range("Matrix indices out of range.");
        }
        return data_[row * cols_ + col];
    }

    size_t getRows() const {
        return rows_;
    }

    size_t getCols() const {
        return cols_;
    }

    void print() const {
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t j = 0; j < cols_; ++j) {
                std::cout << at(j, i) << " ";
            }
            std::cout << "\n";
        }
    }

private:
    size_t rows_, cols_;
    std::vector<int> data_;
};