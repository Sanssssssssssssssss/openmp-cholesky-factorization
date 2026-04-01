#include "mphil_dis_cholesky.h"

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

std::string trim_trailing_newlines(const std::string& text) {
    const std::size_t last = text.find_last_not_of('\n');
    if (last == std::string::npos) {
        return "";
    }
    return text.substr(0, last + 1);
}

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

bool run_current(int block_size,
                 int n,
                 const std::vector<double>& original,
                 std::vector<double>& factorized,
                 double& elapsed) {
    factorized = original;
    const std::string block_size_text = std::to_string(block_size);
    setenv("CHOLESKY_BLOCK_SIZE", block_size_text.c_str(), 1);
    elapsed = cholesky(factorized.data(), n);
    unsetenv("CHOLESKY_BLOCK_SIZE");
    return elapsed >= 0.0;
}

bool repeatability_case(int block_size, int n, std::ostream& out, std::ostream& err) {
    const std::vector<double> original = make_corr_matrix(n);
    std::vector<double> run1;
    std::vector<double> run2;
    std::vector<double> run3;
    double elapsed1 = 0.0;
    double elapsed2 = 0.0;
    double elapsed3 = 0.0;

    if (!run_current(block_size, n, original, run1, elapsed1) ||
        !run_current(block_size, n, original, run2, elapsed2) ||
        !run_current(block_size, n, original, run3, elapsed3)) {
        err << "[FAIL] repeatability block_size=" << block_size << " n=" << n
            << " factorization failed\n";
        return false;
    }

    const auto lower1 = extract_lower(run1, n);
    const auto lower2 = extract_lower(run2, n);
    const auto lower3 = extract_lower(run3, n);
    const double diff12 = max_abs_difference(lower1, lower2);
    const double diff13 = max_abs_difference(lower1, lower3);

    const bool ok = matrices_close(lower1, lower2, 1e-12) &&
                    matrices_close(lower1, lower3, 1e-12);

    std::ostringstream detail;
    detail.setf(std::ios::scientific);
    detail.precision(6);
    detail << "repeatability"
           << " block_size=" << block_size
           << " n=" << n
           << " run1_seconds=" << elapsed1
           << " run2_seconds=" << elapsed2
           << " run3_seconds=" << elapsed3
           << " factor_max_abs_diff_run1_run2=" << diff12
           << " factor_max_abs_diff_run1_run3=" << diff13;

    if (ok) {
        out << "[PASS] " << detail.str() << '\n';
    } else {
        err << "[FAIL] " << detail.str() << '\n';
    }

    return ok;
}

bool block_invariance_case(int n,
                           int reference_block_size,
                           int compare_block_size,
                           std::ostream& out,
                           std::ostream& err) {
    const std::vector<double> original = make_corr_matrix(n);
    std::vector<double> reference;
    std::vector<double> candidate;
    double reference_elapsed = 0.0;
    double candidate_elapsed = 0.0;

    if (!run_current(reference_block_size, n, original, reference, reference_elapsed) ||
        !run_current(compare_block_size, n, original, candidate, candidate_elapsed)) {
        err << "[FAIL] block_invariance n=" << n
            << " reference_block_size=" << reference_block_size
            << " compare_block_size=" << compare_block_size
            << " factorization failed\n";
        return false;
    }

    const auto reference_lower = extract_lower(reference, n);
    const auto candidate_lower = extract_lower(candidate, n);
    const auto rebuilt = multiply_lower_by_transpose(candidate_lower, n);
    const double factor_max_diff = max_abs_difference(reference_lower, candidate_lower);
    const double rebuild_max_error = max_abs_difference(rebuilt, original);

    const bool ok = matrices_close(reference_lower, candidate_lower, 1e-7) &&
                    matrices_close(rebuilt, original, 1e-8);

    std::ostringstream detail;
    detail.setf(std::ios::scientific);
    detail.precision(6);
    detail << "block_invariance"
           << " n=" << n
           << " reference_block_size=" << reference_block_size
           << " compare_block_size=" << compare_block_size
           << " reference_seconds=" << reference_elapsed
           << " candidate_seconds=" << candidate_elapsed
           << " factor_max_abs_diff=" << factor_max_diff
           << " rebuild_max_abs_error=" << rebuild_max_error;

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

    const std::vector<int> repeatability_block_sizes = {16, 31, 32, 64, 128};
    const std::vector<int> repeatability_sizes = {65, 129, 257};

    for (const int block_size : repeatability_block_sizes) {
        for (const int n : repeatability_sizes) {
            ++total;
            std::ostringstream pass_stream;
            std::ostringstream fail_stream;
            if (!repeatability_case(block_size, n, pass_stream, fail_stream)) {
                log_err(fail_stream.str());
                ++failures;
            } else {
                log_out(pass_stream.str());
            }
        }
    }

    const int reference_block_size = 32;
    const std::vector<int> compare_block_sizes = {16, 31, 64, 128};
    const std::vector<int> invariance_sizes = {65, 129, 257};

    for (const int compare_block_size : compare_block_sizes) {
        for (const int n : invariance_sizes) {
            ++total;
            std::ostringstream pass_stream;
            std::ostringstream fail_stream;
            if (!block_invariance_case(n,
                                       reference_block_size,
                                       compare_block_size,
                                       pass_stream,
                                       fail_stream)) {
                log_err(fail_stream.str());
                ++failures;
            } else {
                log_out(pass_stream.str());
            }
        }
    }

    if (failures == 0) {
        log_out("All stability tests passed.");
        return 0;
    }

    log_err(std::to_string(failures) + " / " + std::to_string(total) +
            " stability tests failed.");
    return 1;
}
