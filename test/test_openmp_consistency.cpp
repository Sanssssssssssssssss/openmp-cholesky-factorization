#include "mphil_dis_cholesky.h"
#include "cholesky_v2_reference.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#ifdef _OPENMP
#include <omp.h>  // OpenMP runtime control is used here to force deterministic thread counts in the regression test.
#endif
#include <sstream>
#include <string>
#include <vector>

namespace {

bool nearly_equal(double lhs, double rhs, double tolerance = 1e-8) {
    const double scale = std::max({1.0, std::abs(lhs), std::abs(rhs)});
    return std::abs(lhs - rhs) <= tolerance * scale;
}

std::string trim_trailing_newlines(const std::string& text) {
    const std::size_t last = text.find_last_not_of('\n');
    if (last == std::string::npos) {
        return "";
    }
    return text.substr(0, last + 1);
}

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

std::vector<double> extract_lower(const std::vector<double>& matrix, int n) {
    std::vector<double> lower(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= i; ++j) {
            lower[static_cast<std::size_t>(i) * n + j] =
                matrix[static_cast<std::size_t>(i) * n + j];
        }
    }

    return lower;
}

std::vector<double> multiply_lower_by_transpose(const std::vector<double>& lower, int n) {
    std::vector<double> product(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            const int limit = std::min(i, j);
            for (int k = 0; k <= limit; ++k) {
                sum += lower[static_cast<std::size_t>(i) * n + k] *
                       lower[static_cast<std::size_t>(j) * n + k];
            }
            product[static_cast<std::size_t>(i) * n + j] = sum;
        }
    }

    return product;
}

double max_abs_difference(const std::vector<double>& lhs, const std::vector<double>& rhs) {
    double max_error = 0.0;

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        max_error = std::max(max_error, std::abs(lhs[index] - rhs[index]));
    }

    return max_error;
}

bool matrices_close(const std::vector<double>& lhs,
                    const std::vector<double>& rhs,
                    double tolerance = 1e-8) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (!nearly_equal(lhs[index], rhs[index], tolerance)) {
            return false;
        }
    }

    return true;
}

bool run_current_with_threads(int threads,
                              int n,
                              const std::vector<double>& original,
                              std::vector<double>& factorized,
                              double& elapsed) {
    factorized = original;
#ifdef _OPENMP
    // Disable dynamic team resizing so the regression test exercises the exact thread count we request.
    omp_set_dynamic(0);
    // Force a specific OpenMP team size so single-thread and multi-thread results can be compared directly.
    omp_set_num_threads(threads);
#else
    (void)threads;
#endif
    elapsed = cholesky(factorized.data(), n);
    return elapsed >= 0.0;
}

bool run_reference_v2(int n,
                      const std::vector<double>& original,
                      std::vector<double>& factorized,
                      double& elapsed) {
    factorized = original;
    elapsed = cholesky_v2_reference(factorized.data(), n);
    return elapsed >= 0.0;
}

bool run_case(int threads, int n, std::ostream& out, std::ostream& err) {
    const std::vector<double> original = make_corr_matrix(n);
    std::vector<double> current;
    std::vector<double> current_repeat;
    std::vector<double> reference;
    double current_elapsed = 0.0;
    double current_repeat_elapsed = 0.0;
    double reference_elapsed = 0.0;

    if (!run_current_with_threads(threads, n, original, current, current_elapsed) ||
        !run_current_with_threads(threads, n, original, current_repeat, current_repeat_elapsed) ||
        !run_reference_v2(n, original, reference, reference_elapsed)) {
        err << "[FAIL] threads=" << threads << " n=" << n << " factorization failed\n";
        return false;
    }

    const auto current_lower = extract_lower(current, n);
    const auto current_repeat_lower = extract_lower(current_repeat, n);
    const auto reference_lower = extract_lower(reference, n);

    const auto current_rebuilt = multiply_lower_by_transpose(current_lower, n);
    const double rebuild_error = max_abs_difference(current_rebuilt, original);
    const double factor_diff_vs_v2 = max_abs_difference(current_lower, reference_lower);
    const double repeat_diff = max_abs_difference(current_lower, current_repeat_lower);

    const bool ok = matrices_close(current_rebuilt, original, 1e-8) &&
                    matrices_close(current_lower, reference_lower, 1e-7) &&
                    matrices_close(current_lower, current_repeat_lower, 1e-12);

    std::ostringstream detail;
    detail.setf(std::ios::scientific);
    detail.precision(6);
    detail << "threads=" << threads
           << " n=" << n
           << " current_seconds=" << current_elapsed
           << " current_repeat_seconds=" << current_repeat_elapsed
           << " reference_v2_seconds=" << reference_elapsed
           << " rebuild_max_abs_error=" << rebuild_error
           << " factor_max_abs_diff_vs_v2=" << factor_diff_vs_v2
           << " repeat_factor_max_abs_diff=" << repeat_diff;

    if (ok) {
        out << "[PASS] " << detail.str() << '\n';
    } else {
        err << "[FAIL] " << detail.str() << '\n';
    }

    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    std::ofstream report_file;
    if (argc >= 2) {
        report_file.open(argv[1]);
        if (!report_file) {
            std::cerr << "failed to open report path: " << argv[1] << '\n';
            return 1;
        }
    }

    auto log_out = [&](const std::string& line) {
        const std::string trimmed = trim_trailing_newlines(line);
        std::cout << trimmed << '\n';
        if (report_file) {
            report_file << trimmed << '\n';
        }
    };

    auto log_err = [&](const std::string& line) {
        const std::string trimmed = trim_trailing_newlines(line);
        std::cerr << trimmed << '\n';
        if (report_file) {
            report_file << trimmed << '\n';
        }
    };

    int failures = 0;
    int total = 0;

    const std::vector<int> threads_to_check = {1, 2, 4};
    const std::vector<int> sizes = {65, 129, 257};

    for (const int threads : threads_to_check) {
        for (const int n : sizes) {
            ++total;
            std::ostringstream pass_stream;
            std::ostringstream fail_stream;
            if (!run_case(threads, n, pass_stream, fail_stream)) {
                log_err(fail_stream.str());
                ++failures;
            } else {
                log_out(pass_stream.str());
            }
        }
    }

    if (failures == 0) {
        log_out("All OpenMP consistency tests passed.");
        return 0;
    }

    log_err(std::to_string(failures) + " / " + std::to_string(total) +
            " OpenMP consistency tests failed.");
    return 1;
}
