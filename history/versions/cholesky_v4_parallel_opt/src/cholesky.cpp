#include "mphil_dis_cholesky.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <omp.h>  // OpenMP is used for the first parallel version in the trailing-update hotspot.
#include <string>

namespace {

int resolve_positive_env_or_default(const char* name, int default_value) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return default_value;
    }

    const int parsed = std::atoi(value);
    return parsed > 0 ? parsed : default_value;
}

int resolve_block_size() {
    return resolve_positive_env_or_default("CHOLESKY_BLOCK_SIZE", 32);
}

struct OpenMPScheduleConfig {
    omp_sched_t kind = omp_sched_guided;
    int chunk = 1;
};

OpenMPScheduleConfig resolve_openmp_schedule() {
    OpenMPScheduleConfig config;

    if (const char* schedule_value = std::getenv("CHOLESKY_OMP_SCHEDULE")) {
        const std::string schedule(schedule_value);
        if (schedule == "dynamic") {
            config.kind = omp_sched_dynamic;
        } else if (schedule == "guided") {
            config.kind = omp_sched_guided;
        } else if (schedule == "auto") {
            config.kind = omp_sched_auto;
        } else {
            config.kind = omp_sched_static;
        }
    }

    if (const char* chunk_value = std::getenv("CHOLESKY_OMP_CHUNK")) {
        const int parsed_chunk = std::atoi(chunk_value);
        if (parsed_chunk > 0) {
            config.chunk = parsed_chunk;
        }
    }

    return config;
}

}  // namespace

double cholesky(double* c, int n) {
    if (c == nullptr || n < 0 || n > 100000) {
        return -1.0;
    }

    const std::size_t stride = static_cast<std::size_t>(n);
    const int block_size = resolve_block_size();
    const OpenMPScheduleConfig openmp_schedule = resolve_openmp_schedule();

    // Program the OpenMP runtime from the tuned default schedule, while still allowing the v4 workflow to override policy through the environment.
    omp_set_schedule(openmp_schedule.kind, openmp_schedule.chunk);

    auto row_ptr = [c, stride](int row) -> double* {
        return c + static_cast<std::size_t>(row) * stride;
    };

    const auto start = std::chrono::steady_clock::now();

    for (int block_start = 0; block_start < n; block_start += block_size) {
        const int block_end = std::min(block_start + block_size, n);

        // Factor the diagonal panel with an unblocked lower-triangular kernel while preserving the mirrored upper triangle required by the coursework.
        for (int p = block_start; p < block_end; ++p) {
            double* const row_p = row_ptr(p);
            const double diag_entry = row_p[p];
            if (diag_entry <= 0.0) {
                return -1.0;
            }

            const double diag = std::sqrt(diag_entry);
            const double inv_diag = 1.0 / diag;
            row_p[p] = diag;

            for (int i = p + 1; i < block_end; ++i) {
                double* const row_i = row_ptr(i);
                row_i[p] *= inv_diag;
                row_p[i] = row_i[p];
            }

            for (int j = p + 1; j < block_end; ++j) {
                double* const row_j = row_ptr(j);
                const double panel_value = row_j[p];

                for (int i = j; i < block_end; ++i) {
                    double* const row_i = row_ptr(i);
                    const double updated = row_i[j] - row_i[p] * panel_value;
                    row_i[j] = updated;
                    if (i != j) {
                        row_j[i] = updated;
                    }
                }
            }
        }

        // Solve the block column below the panel after panel factorization; rows below the panel are independent, so this is a safe OpenMP region.
        // This `parallel for` distributes independent rows of the solved block column to reduce the panel-to-trailing handoff cost.
#pragma omp parallel for schedule(runtime) if (n - block_end > block_size)
        for (int i = block_end; i < n; ++i) {
            double* const row_i = row_ptr(i);

            for (int j = block_start; j < block_end; ++j) {
                double* const row_j = row_ptr(j);
                double dot = 0.0;

                // This `simd` reduction vectorizes the panel dot product because each term is independent and feeds one scalar accumulation.
 #pragma omp simd reduction(+:dot)
                for (int p = block_start; p < j; ++p) {
                    dot += row_i[p] * row_j[p];
                }

                row_i[j] = (row_i[j] - dot) / row_j[j];
                row_j[i] = row_i[j];
            }
        }

        // Update the trailing matrix tile by tile; each outer i-block owns disjoint row ranges in the lower triangle, making it the main OpenMP hotspot.
        // This `parallel for` spreads independent trailing tiles across threads while preserving deterministic writes to disjoint lower-triangular regions.
#pragma omp parallel for schedule(runtime) if (n - block_end > block_size)
        for (int i_block = block_end; i_block < n; i_block += block_size) {
            const int i_end = std::min(i_block + block_size, n);

            for (int j_block = block_end; j_block <= i_block; j_block += block_size) {
                const int j_end = std::min(j_block + block_size, n);

                for (int i = i_block; i < i_end; ++i) {
                    double* const row_i = row_ptr(i);
                    const int j_limit = std::min(j_end, i + 1);

                    for (int j = j_block; j < j_limit; ++j) {
                        double* const row_j = row_ptr(j);
                        double dot = 0.0;

                        // This `simd` reduction vectorizes the rank-k accumulation inside each tile because the dot-product terms are independent.
 #pragma omp simd reduction(+:dot)
                        for (int p = block_start; p < block_end; ++p) {
                            dot += row_i[p] * row_j[p];
                        }

                        row_i[j] -= dot;
                        if (i != j) {
                            row_j[i] = row_i[j];
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
