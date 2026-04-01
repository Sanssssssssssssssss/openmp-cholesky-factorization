#ifndef CHOLESKY_BENCHMARK_SUPPORT_H
#define CHOLESKY_BENCHMARK_SUPPORT_H

#include <iosfwd>
#include <string>
#include <vector>

struct CholeskyBenchmarkConfig {
    int n = 512;
    int repetitions = 5;
    int warmup = 1;
    std::string label = "cholesky_v0_baseline";
};

struct CholeskyBenchmarkResult {
    std::string label;
    std::string generator = "corr";
    int n = 0;
    int repetitions = 0;
    int warmup = 0;
    std::vector<double> times;
    double min_seconds = 0.0;
    double median_seconds = 0.0;
    double mean_seconds = 0.0;
    double max_seconds = 0.0;
    double estimated_flops = 0.0;
    double median_gflops = 0.0;
};

bool validate_benchmark_config(const CholeskyBenchmarkConfig& config, std::ostream& err);
CholeskyBenchmarkResult run_cholesky_benchmark(const CholeskyBenchmarkConfig& config, std::ostream& err);
void write_benchmark_result(std::ostream& out, const CholeskyBenchmarkResult& result);

#endif
