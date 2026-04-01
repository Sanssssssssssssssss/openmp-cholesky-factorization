#include "mphil_dis_cholesky.h"

#include <chrono>
#include <cmath>
#include <cstddef>

double cholesky(double* c, int n) {
    if (c == nullptr || n < 0 || n > 100000) {
        return -1.0;
    }

    const std::size_t stride = static_cast<std::size_t>(n);
    const auto start = std::chrono::steady_clock::now();

    for (int p = 0; p < n; ++p) {
        const std::size_t p_index = static_cast<std::size_t>(p);
        const std::size_t diag_offset = p_index * stride + p_index;
        double* const row_p = c + p_index * stride;
        const double diag_entry = c[diag_offset];

        if (diag_entry <= 0.0) {
            return -1.0;
        }

        const double diag = std::sqrt(diag_entry);
        const double inv_diag = 1.0 / diag;
        c[diag_offset] = diag;

        for (int j = p + 1; j < n; ++j) {
            const std::size_t j_offset = static_cast<std::size_t>(j);
            row_p[j_offset] *= inv_diag;
        }

        for (int i = p + 1; i < n; ++i) {
            const std::size_t i_offset = static_cast<std::size_t>(i);
            c[i_offset * stride + p_index] *= inv_diag;
        }

        for (int i = p + 1; i < n; ++i) {
            const std::size_t i_offset = static_cast<std::size_t>(i);
            double* const row_i = c + i_offset * stride;
            const double col_value = row_i[p_index];

            for (int j = p + 1; j < n; ++j) {
                const std::size_t j_offset = static_cast<std::size_t>(j);
                row_i[j_offset] -= col_value * row_p[j_offset];
            }
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}
