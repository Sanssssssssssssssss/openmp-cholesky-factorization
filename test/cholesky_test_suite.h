#ifndef CHOLESKY_TEST_SUITE_H
#define CHOLESKY_TEST_SUITE_H

#include <iosfwd>

struct CholeskyTestSummary {
    int total = 0;
    int failures = 0;
};

CholeskyTestSummary run_cholesky_tests(std::ostream& out, std::ostream& err);

#endif
