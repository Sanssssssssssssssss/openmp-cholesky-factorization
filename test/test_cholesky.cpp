#include "cholesky_test_suite.h"

#include <iostream>

int main() {
    const CholeskyTestSummary summary = run_cholesky_tests(std::cout, std::cerr);
    return summary.failures == 0 ? 0 : 1;
}
