#include "mphil_dis_cholesky.h"
#include "cholesky_v1_reference.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool nearly_equal(double lhs, double rhs, double tolerance = 1e-8) {
    const double scale = std::max({1.0, std::abs(lhs), std::abs(rhs)});
    return std::abs(lhs - rhs) <= tolerance * scale;
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

double max_abs_difference(const std::vector<double>& lhs, const std::vector<double>& rhs) {
    double max_error = 0.0;

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        max_error = std::max(max_error, std::abs(lhs[index] - rhs[index]));
    }

    return max_error;
}

bool mirrored(const std::vector<double>& matrix, int n, double tolerance = 1e-8) {
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (!nearly_equal(matrix[static_cast<std::size_t>(i) * n + j],
                              matrix[static_cast<std::size_t>(j) * n + i],
                              tolerance)) {
                return false;
            }
        }
    }

    return true;
}

bool run_case(int block_size, int n, std::ostream& out, std::ostream& err) {
    const std::vector<double> original = make_corr_matrix(n);
    std::vector<double> current = original;
    std::vector<double> reference = original;

    const std::string block_size_text = std::to_string(block_size);
    setenv("CHOLESKY_BLOCK_SIZE", block_size_text.c_str(), 1);
    const double current_elapsed = cholesky(current.data(), n);
    unsetenv("CHOLESKY_BLOCK_SIZE");

    const double reference_elapsed = cholesky_v1_reference(reference.data(), n);

    if (current_elapsed < 0.0 || reference_elapsed < 0.0) {
        err << "[FAIL] block_size=" << block_size << " n=" << n << " factorization failed\n";
        return false;
    }

    const auto current_lower = extract_lower(current, n);
    const auto reference_lower = extract_lower(reference, n);
    const auto current_rebuilt = multiply_lower_by_transpose(current_lower, n);
    const auto reference_rebuilt = multiply_lower_by_transpose(reference_lower, n);
    const double current_rebuild_error = max_abs_difference(current_rebuilt, original);
    const double reference_rebuild_error = max_abs_difference(reference_rebuilt, original);
    const double factor_max_diff = max_abs_difference(current_lower, reference_lower);

    const bool ok = mirrored(current, n) &&
                    mirrored(reference, n) &&
                    matrices_close(current_rebuilt, original) &&
                    matrices_close(reference_rebuilt, original) &&
                    matrices_close(current_lower, reference_lower, 1e-7);

    std::ostringstream detail;
    detail.setf(std::ios::scientific);
    detail.precision(6);
    detail << "block_size=" << block_size
           << " n=" << n
           << " current_seconds=" << current_elapsed
           << " reference_seconds=" << reference_elapsed
           << " current_rebuild_max_abs_error=" << current_rebuild_error
           << " reference_rebuild_max_abs_error=" << reference_rebuild_error
           << " factor_max_abs_diff=" << factor_max_diff;

    if (ok) {
        out << "[PASS] " << detail.str() << '\n';
    } else {
        err << "[FAIL] " << detail.str() << '\n';
    }

    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<int> block_sizes = {32, 64, 128};
    const std::vector<int> sizes = {31, 32, 33, 63, 64, 65, 127, 128, 129};
    std::ofstream report_file;

    if (argc >= 2) {
        report_file.open(argv[1]);
        if (!report_file) {
            std::cerr << "failed to open report path: " << argv[1] << '\n';
            return 1;
        }
    }

    auto log_out = [&](const std::string& line) {
        std::cout << line << '\n';
        if (report_file) {
            report_file << line << '\n';
        }
    };

    auto log_err = [&](const std::string& line) {
        std::cerr << line << '\n';
        if (report_file) {
            report_file << line << '\n';
        }
    };

    int failures = 0;
    int total = 0;

    for (const int block_size : block_sizes) {
        for (const int n : sizes) {
            ++total;
            std::ostringstream pass_stream;
            std::ostringstream fail_stream;
            if (!run_case(block_size, n, pass_stream, fail_stream)) {
                const std::string fail_text = fail_stream.str();
                if (!fail_text.empty()) {
                    log_err(fail_text.substr(0, fail_text.find_last_not_of('\n') + 1));
                }
                ++failures;
            } else {
                const std::string pass_text = pass_stream.str();
                if (!pass_text.empty()) {
                    log_out(pass_text.substr(0, pass_text.find_last_not_of('\n') + 1));
                }
            }
        }
    }

    if (failures == 0) {
        log_out("All v2-v1 regression tests passed.");
        return 0;
    }

    log_err(std::to_string(failures) + " / " + std::to_string(total) +
            " v2-v1 regression tests failed.");
    return 1;
}
