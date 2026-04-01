#ifndef CHOLESKY_TEST_SUITE_H
#define CHOLESKY_TEST_SUITE_H

#include <iosfwd>

struct CholeskyTestSummary {
    int total = 0;
    int failures = 0;
};

CholeskyTestSummary run_cholesky_tests(std::ostream& out, std::ostream& err);
double compute_factorized_logdet_for_tests(const double* matrix, int n);

#endif
