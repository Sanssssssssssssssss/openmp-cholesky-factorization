#include "cholesky_benchmark_support.h"

#include "mphil_dis_cholesky.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <ostream>
#include <vector>

namespace {

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

double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());

    const std::size_t middle = values.size() / 2;
    if (values.size() % 2 == 0) {
        return 0.5 * (values[middle - 1] + values[middle]);
    }

    return values[middle];
}

double estimate_cholesky_flops(int n) {
    const double dimension = static_cast<double>(n);
    return dimension * dimension * dimension / 3.0;
}

double estimate_matrix_bytes(int n) {
    const double dimension = static_cast<double>(n);
    return dimension * dimension * static_cast<double>(sizeof(double));
}

}  // namespace

bool validate_benchmark_config(const CholeskyBenchmarkConfig& config, std::ostream& err) {
    if (config.n <= 0 || config.repetitions <= 0 || config.warmup < 0) {
        err << "invalid benchmark config: n > 0, repetitions > 0, warmup >= 0 required\n";
        return false;
    }

    return true;
}

CholeskyBenchmarkResult run_cholesky_benchmark(const CholeskyBenchmarkConfig& config, std::ostream& err) {
    CholeskyBenchmarkResult result;
    result.label = config.label;
    result.n = config.n;
    result.requested_threads = 1;
    result.repetitions = config.repetitions;
    result.warmup = config.warmup;

    if (!validate_benchmark_config(config, err)) {
        result.min_seconds = -1.0;
        return result;
    }

    const std::vector<double> original = make_corr_matrix(config.n);

    for (int run = 0; run < config.warmup; ++run) {
        std::vector<double> working = original;
        if (cholesky(working.data(), config.n) < 0.0) {
            err << "warmup run failed\n";
            result.min_seconds = -1.0;
            return result;
        }
    }

    result.times.reserve(static_cast<std::size_t>(config.repetitions));

    for (int run = 0; run < config.repetitions; ++run) {
        std::vector<double> working = original;
        const double elapsed = cholesky(working.data(), config.n);

        if (elapsed < 0.0) {
            err << "benchmark run failed\n";
            result.min_seconds = -1.0;
            return result;
        }

        result.times.push_back(elapsed);
    }

    const auto [min_it, max_it] = std::minmax_element(result.times.begin(), result.times.end());
    result.min_seconds = *min_it;
    result.max_seconds = *max_it;
    result.median_seconds = median(result.times);
    result.mean_seconds =
        std::accumulate(result.times.begin(), result.times.end(), 0.0) /
        static_cast<double>(result.times.size());
    result.estimated_flops = estimate_cholesky_flops(config.n);
    result.median_gflops = result.median_seconds > 0.0
                               ? result.estimated_flops / result.median_seconds / 1.0e9
                               : 0.0;
    result.matrix_bytes = estimate_matrix_bytes(config.n);
    result.matrix_mib = result.matrix_bytes / (1024.0 * 1024.0);

    return result;
}

void write_benchmark_result(std::ostream& out, const CholeskyBenchmarkResult& result) {
    out.setf(std::ios::fixed);
    out.precision(9);
    out << "label=" << result.label << '\n';
    out << "generator=" << result.generator << '\n';
    out << "n=" << result.n << '\n';
    out << "requested_threads=" << result.requested_threads << '\n';
    out << "repetitions=" << result.repetitions << '\n';
    out << "warmup=" << result.warmup << '\n';
    for (std::size_t index = 0; index < result.times.size(); ++index) {
        out << "run_" << (index + 1) << "_seconds=" << result.times[index] << '\n';
    }
    out << "min_seconds=" << result.min_seconds << '\n';
    out << "median_seconds=" << result.median_seconds << '\n';
    out << "mean_seconds=" << result.mean_seconds << '\n';
    out << "max_seconds=" << result.max_seconds << '\n';
    out << "estimated_flops=" << result.estimated_flops << '\n';
    out << "median_gflops=" << result.median_gflops << '\n';
    out << "matrix_bytes=" << result.matrix_bytes << '\n';
    out << "matrix_mib=" << result.matrix_mib << '\n';
}

void write_benchmark_csv_header(std::ostream& out) {
    out << "sweep,label,generator,n,requested_threads,repetitions,warmup,min_seconds,median_seconds,mean_seconds,max_seconds,"
           "estimated_flops,median_gflops,matrix_bytes,matrix_mib\n";
}

void write_benchmark_csv_row(std::ostream& out, const std::string& sweep, const CholeskyBenchmarkResult& result) {
    out.setf(std::ios::fixed);
    out.precision(9);
    out << sweep << ','
        << result.label << ','
        << result.generator << ','
        << result.n << ','
        << result.requested_threads << ','
        << result.repetitions << ','
        << result.warmup << ','
        << result.min_seconds << ','
        << result.median_seconds << ','
        << result.mean_seconds << ','
        << result.max_seconds << ','
        << result.estimated_flops << ','
        << result.median_gflops << ','
        << result.matrix_bytes << ','
        << result.matrix_mib << '\n';
}
