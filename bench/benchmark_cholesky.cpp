#include "cholesky_benchmark_support.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    CholeskyBenchmarkConfig config;
    config.n = 512;
    config.repetitions = 5;
    config.warmup = 1;
    config.label = "cholesky_v0_baseline";

    if (argc >= 2) {
        config.n = std::atoi(argv[1]);
    }
    if (argc >= 3) {
        config.repetitions = std::atoi(argv[2]);
    }
    if (argc >= 4) {
        config.warmup = std::atoi(argv[3]);
    }
    if (argc >= 5) {
        config.label = argv[4];
    }

    if (!validate_benchmark_config(config, std::cerr)) {
        std::cerr << "usage: cholesky_benchmark <n> [repetitions] [warmup] [label]\n";
        return 1;
    }

    const auto result = run_cholesky_benchmark(config, std::cerr);
    if (result.min_seconds < 0.0) {
        return 1;
    }

    write_benchmark_result(std::cout, result);

    return 0;
}
