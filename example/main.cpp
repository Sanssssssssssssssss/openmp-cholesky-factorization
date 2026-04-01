#include "mphil_dis_cholesky.h"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

// Builds a deterministic symmetric positive-definite matrix from a synthetic
// lower-triangular factor so the example works for any n >= 1.
std::vector<double> make_example_spd_matrix(int n) {
    std::vector<double> lower(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);
    std::vector<double> matrix(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        lower[static_cast<std::size_t>(i) * n + i] = 2.0 + 0.25 * static_cast<double>(i);
        for (int j = 0; j < i; ++j) {
            lower[static_cast<std::size_t>(i) * n + j] = 0.1 * static_cast<double>((i + j) % 5 + 1);
        }
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= i; ++j) {
            double sum = 0.0;
            for (int k = 0; k <= j; ++k) {
                sum += lower[static_cast<std::size_t>(i) * n + k] *
                       lower[static_cast<std::size_t>(j) * n + k];
            }
            matrix[static_cast<std::size_t>(i) * n + j] = sum;
            matrix[static_cast<std::size_t>(j) * n + i] = sum;
        }
    }

    return matrix;
}

double compute_logdet_from_factor(const std::vector<double>& matrix, int n) {
    double logdet = 0.0;
    for (int i = 0; i < n; ++i) {
        logdet += 2.0 * std::log(matrix[static_cast<std::size_t>(i) * n + i]);
    }
    return logdet;
}

}  // namespace

int main(int argc, char** argv) {
    int matrix_size = 4;
    if (argc > 1) {
        const int parsed = std::atoi(argv[1]);
        if (parsed > 0) {
            matrix_size = parsed;
        }
    }

    // Allocate a row-major n x n matrix and fill it with a deterministic SPD example.
    std::vector<double> matrix = make_example_spd_matrix(matrix_size);

    const double elapsed = cholesky(matrix.data(), matrix_size);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "n=" << matrix_size << '\n';
    std::cout << "elapsed_seconds=" << elapsed << '\n';
    if (elapsed < 0.0) {
        std::cout << "factorization_failed" << '\n';
        return 1;
    }

    std::cout << "logdet_from_diagonal=" << compute_logdet_from_factor(matrix, matrix_size) << '\n';
    std::cout << "matrix_after=" << '\n';

    for (int i = 0; i < matrix_size; ++i) {
        for (int j = 0; j < matrix_size; ++j) {
            std::cout << matrix[static_cast<std::size_t>(i) * matrix_size + j];
            if (j + 1 < matrix_size) {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }

    return 0;
}
