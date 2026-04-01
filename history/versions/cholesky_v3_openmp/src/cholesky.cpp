#include "mphil_dis_cholesky.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <cstdlib>
#include <omp.h>  // OpenMP is used for the first parallel version in the trailing-update hotspot.

namespace {

int resolve_block_size() {
    constexpr int default_block_size = 32;
    const char* value = std::getenv("CHOLESKY_BLOCK_SIZE");
    if (value == nullptr) {
        return default_block_size;
    }

    const int parsed = std::atoi(value);
    return parsed > 0 ? parsed : default_block_size;
}

}  // namespace

double cholesky(double* c, int n) {
    if (c == nullptr || n < 0 || n > 100000) {
        return -1.0;
    }

    const std::size_t stride = static_cast<std::size_t>(n);
    const auto start = std::chrono::steady_clock::now();
    const int panel_block_size = resolve_block_size();
    const int trailing_block_size = panel_block_size;

    auto at = [c, stride](int row, int col) -> double& {
        return c[static_cast<std::size_t>(row) * stride + static_cast<std::size_t>(col)];
    };

    for (int block_start = 0; block_start < n; block_start += panel_block_size) {
        const int block_end = std::min(block_start + panel_block_size, n);

        // Factor the diagonal panel one column at a time while keeping it mirrored.
        for (int p = block_start; p < block_end; ++p) {
            const double diag_entry = at(p, p);
            if (diag_entry <= 0.0) {
                return -1.0;
            }

            const double diag = std::sqrt(diag_entry);
            const double inv_diag = 1.0 / diag;
            at(p, p) = diag;

            for (int i = p + 1; i < block_end; ++i) {
                const double value = at(i, p) * inv_diag;
                at(i, p) = value;
                at(p, i) = value;
            }

            for (int j = p + 1; j < block_end; ++j) {
                const double panel_value = at(j, p);
                for (int i = j; i < block_end; ++i) {
                    const double updated = at(i, j) - at(i, p) * panel_value;
                    at(i, j) = updated;
                    if (i != j) {
                        at(j, i) = updated;
                    }
                }
            }
        }

        // Solve the block column below the panel using the newly factorized block.
        for (int i = block_end; i < n; ++i) {
            for (int j = block_start; j < block_end; ++j) {
                double value = at(i, j);
                for (int p = block_start; p < j; ++p) {
                    value -= at(i, p) * at(j, p);
                }
                value /= at(j, j);
                at(i, j) = value;
                at(j, i) = value;
            }
        }

        // Update the trailing matrix with blocked rank-k style tiles.
        // OpenMP is introduced here first because these trailing tiles are independent for a fixed panel and dominate the runtime.
#pragma omp parallel for schedule(static) if (n - block_end > trailing_block_size)
        for (int i_block = block_end; i_block < n; i_block += trailing_block_size) {
            const int i_end = std::min(i_block + trailing_block_size, n);

            for (int j_block = block_end; j_block <= i_block; j_block += trailing_block_size) {
                const int j_end = std::min(j_block + trailing_block_size, n);

                for (int i = i_block; i < i_end; ++i) {
                    const int j_limit = std::min(j_end, i + 1);
                    for (int j = j_block; j < j_limit; ++j) {
                        double value = at(i, j);
                        for (int p = block_start; p < block_end; ++p) {
                            value -= at(i, p) * at(j, p);
                        }
                        at(i, j) = value;
                        if (i != j) {
                            at(j, i) = value;
                        }
                    }
                }
            }
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}
