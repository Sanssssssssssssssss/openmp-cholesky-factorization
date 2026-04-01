#include "mphil_dis_cholesky.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

std::vector<double> make_corr_matrix(int n) {
    std::vector<double> matrix(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            const double delta = static_cast<double>(i - j);
            matrix[static_cast<std::size_t>(i) * n + j] =
                0.99 * std::exp(-0.5 * 16.0 * delta * delta / (static_cast<double>(n) * n));
        }
        matrix[static_cast<std::size_t>(i) * n + i] = 1.0;
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
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <n>\n";
        return 1;
    }

    const int n = std::atoi(argv[1]);
    if (n <= 0) {
        std::cerr << "n must be positive\n";
        return 1;
    }

    std::vector<double> matrix = make_corr_matrix(n);
    const double elapsed = cholesky(matrix.data(), n);
    if (elapsed < 0.0) {
        std::cerr << "factorization failed\n";
        return 2;
    }

    std::cout << std::setprecision(17);
    std::cout << "n=" << n << '\n';
    std::cout << "elapsed_seconds=" << elapsed << '\n';
    std::cout << "logdet=" << compute_logdet_from_factor(matrix, n) << '\n';
    return 0;
}
