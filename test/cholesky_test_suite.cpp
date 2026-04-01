#include "cholesky_test_suite.h"

#include "mphil_dis_cholesky.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <vector>

namespace {

bool nearly_equal(double lhs, double rhs, double tolerance = 1e-10) {
    const double scale = std::max({1.0, std::abs(lhs), std::abs(rhs)});
    return std::abs(lhs - rhs) <= tolerance * scale;
}

std::vector<double> extract_lower_factor(const std::vector<double>& matrix, int n) {
    std::vector<double> lower(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= i; ++j) {
            lower[static_cast<std::size_t>(i) * n + j] = matrix[static_cast<std::size_t>(i) * n + j];
        }
    }

    return lower;
}

std::vector<double> multiply_lower_by_transpose(const std::vector<double>& lower, int n) {
    std::vector<double> product(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            const int limit = (i < j) ? i : j;

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
                    double tolerance = 1e-10) {
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

bool output_has_mirrored_triangles(const std::vector<double>& matrix,
                                   int n,
                                   double tolerance = 1e-10) {
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

double logdet_from_factorized_matrix(const std::vector<double>& matrix, int n) {
    double sum = 0.0;

    for (int i = 0; i < n; ++i) {
        sum += std::log(matrix[static_cast<std::size_t>(i) * n + i]);
    }

    return 2.0 * sum;
}

std::vector<double> make_diagonal_matrix(const std::vector<double>& diagonal) {
    const int n = static_cast<int>(diagonal.size());
    std::vector<double> matrix(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        matrix[static_cast<std::size_t>(i) * n + i] = diagonal[static_cast<std::size_t>(i)];
    }

    return matrix;
}

std::vector<double> make_dense_factor(int n) {
    std::vector<double> lower(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= i; ++j) {
            if (i == j) {
                lower[static_cast<std::size_t>(i) * n + j] = 1.5 + 0.25 * static_cast<double>(i);
            } else {
                const double sign = ((i + j) % 2 == 0) ? 1.0 : -1.0;
                lower[static_cast<std::size_t>(i) * n + j] =
                    sign * (0.2 + 0.05 * static_cast<double>((i + 1) * (j + 1)));
            }
        }
    }

    return lower;
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

bool test_zero_size_matrix() {
    double placeholder = 0.0;
    return cholesky(&placeholder, 0) >= 0.0;
}

bool test_one_by_one_matrix() {
    double matrix[] = {9.0};
    const double elapsed = cholesky(matrix, 1);
    return elapsed >= 0.0 && nearly_equal(matrix[0], 3.0);
}

bool test_coursework_example() {
    std::vector<double> matrix = {
        4.0, 2.0,
        2.0, 26.0,
    };

    const double elapsed = cholesky(matrix.data(), 2);
    const std::vector<double> expected = {
        2.0, 1.0,
        1.0, 5.0,
    };

    return elapsed >= 0.0 && matrices_close(matrix, expected) &&
           output_has_mirrored_triangles(matrix, 2);
}

bool test_coursework_example_logdet() {
    std::vector<double> matrix = {
        4.0, 2.0,
        2.0, 26.0,
    };

    if (cholesky(matrix.data(), 2) < 0.0) {
        return false;
    }

    return nearly_equal(logdet_from_factorized_matrix(matrix, 2), std::log(100.0));
}

bool test_diagonal_matrix_factorization() {
    const std::vector<double> diagonal = {4.0, 9.0, 16.0, 25.0};
    std::vector<double> matrix = make_diagonal_matrix(diagonal);

    if (cholesky(matrix.data(), static_cast<int>(diagonal.size())) < 0.0) {
        return false;
    }

    const std::vector<double> expected = {
        2.0, 0.0, 0.0, 0.0,
        0.0, 3.0, 0.0, 0.0,
        0.0, 0.0, 4.0, 0.0,
        0.0, 0.0, 0.0, 5.0,
    };

    return matrices_close(matrix, expected);
}

bool test_known_three_by_three_factor() {
    const int n = 3;
    const std::vector<double> lower = {
        2.0, 0.0, 0.0,
        1.0, 3.0, 0.0,
        4.0, -2.0, 1.5,
    };

    const std::vector<double> original = multiply_lower_by_transpose(lower, n);
    std::vector<double> matrix = original;

    const double elapsed = cholesky(matrix.data(), n);
    const std::vector<double> expected = {
        2.0,  1.0,  4.0,
        1.0,  3.0, -2.0,
        4.0, -2.0,  1.5,
    };

    if (!(elapsed >= 0.0) || !matrices_close(matrix, expected) ||
        !output_has_mirrored_triangles(matrix, n)) {
        return false;
    }

    const auto rebuilt = multiply_lower_by_transpose(extract_lower_factor(matrix, n), n);
    return matrices_close(rebuilt, original);
}

bool test_known_four_by_four_rebuild() {
    const int n = 4;
    const std::vector<double> lower = make_dense_factor(n);
    const std::vector<double> original = multiply_lower_by_transpose(lower, n);
    std::vector<double> matrix = original;

    if (cholesky(matrix.data(), n) < 0.0) {
        return false;
    }

    if (!output_has_mirrored_triangles(matrix, n)) {
        return false;
    }

    const auto rebuilt = multiply_lower_by_transpose(extract_lower_factor(matrix, n), n);
    return matrices_close(rebuilt, original, 1e-9);
}

bool test_corr_matrix_rebuild_and_logdet() {
    const int n = 6;
    const std::vector<double> original = make_corr_matrix(n);
    std::vector<double> matrix = original;

    const double elapsed = cholesky(matrix.data(), n);
    if (!(elapsed >= 0.0) || !output_has_mirrored_triangles(matrix, n)) {
        return false;
    }

    const auto lower = extract_lower_factor(matrix, n);
    const auto rebuilt = multiply_lower_by_transpose(lower, n);
    if (!matrices_close(rebuilt, original, 1e-9)) {
        return false;
    }

    const double logdet = logdet_from_factorized_matrix(matrix, n);
    return std::isfinite(logdet);
}

bool test_invalid_input_rejected() {
    double value = 1.0;
    std::vector<double> non_spd = {
        1.0, 2.0,
        2.0, 1.0,
    };

    return cholesky(nullptr, 1) < 0.0 &&
           cholesky(&value, 100001) < 0.0 &&
           cholesky(non_spd.data(), 2) < 0.0;
}

bool test_subdiagonal_entries_become_positive_for_corr_matrix() {
    const int n = 8;
    std::vector<double> matrix = make_corr_matrix(n);

    if (cholesky(matrix.data(), n) < 0.0) {
        return false;
    }

    for (int i = 0; i < n; ++i) {
        const double diagonal = matrix[static_cast<std::size_t>(i) * n + i];
        if (!(diagonal > 0.0) || !std::isfinite(diagonal)) {
            return false;
        }
    }

    return true;
}

bool test_input_with_negative_diagonal_fails() {
    std::vector<double> matrix = {
        -1.0, 0.0,
         0.0, 1.0,
    };

    return cholesky(matrix.data(), 2) < 0.0;
}

}  // namespace

CholeskyTestSummary run_cholesky_tests(std::ostream& out, std::ostream& err) {
    struct TestCase {
        const char* name;
        bool (*run)();
    };

    const TestCase cases[] = {
        {"zero_size_matrix", test_zero_size_matrix},
        {"one_by_one_matrix", test_one_by_one_matrix},
        {"coursework_example", test_coursework_example},
        {"coursework_example_logdet", test_coursework_example_logdet},
        {"diagonal_matrix_factorization", test_diagonal_matrix_factorization},
        {"known_three_by_three_factor", test_known_three_by_three_factor},
        {"known_four_by_four_rebuild", test_known_four_by_four_rebuild},
        {"corr_matrix_rebuild_and_logdet", test_corr_matrix_rebuild_and_logdet},
        {"corr_matrix_positive_diagonal", test_subdiagonal_entries_become_positive_for_corr_matrix},
        {"invalid_input_rejected", test_invalid_input_rejected},
        {"negative_diagonal_fails", test_input_with_negative_diagonal_fails},
    };

    CholeskyTestSummary summary;
    summary.total = static_cast<int>(sizeof(cases) / sizeof(cases[0]));

    for (const auto& test_case : cases) {
        const bool passed = test_case.run();
        out << (passed ? "[PASS] " : "[FAIL] ") << test_case.name << '\n';
        summary.failures += passed ? 0 : 1;
    }

    if (summary.failures != 0) {
        err << summary.failures << " test(s) failed.\n";
    } else {
        out << "All correctness tests passed.\n";
    }

    return summary;
}

double compute_factorized_logdet_for_tests(const double* matrix, int n) {
    if (matrix == nullptr || n < 0) {
        return -std::numeric_limits<double>::infinity();
    }

    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += std::log(matrix[static_cast<std::size_t>(i) * n + i]);
    }
    return 2.0 * sum;
}
