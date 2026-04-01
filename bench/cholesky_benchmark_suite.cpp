#include "cholesky_benchmark_support.h"
#include "cholesky_test_suite.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool run_and_record_sweep(const std::filesystem::path& output_path,
                          const std::string& suite_name,
                          const std::vector<int>& sizes,
                          int repetitions,
                          int warmup,
                          const std::string& label) {
    std::ofstream file(output_path);
    if (!file) {
        std::cerr << "failed to open " << output_path << '\n';
        return false;
    }

    std::cout << "[INFO] Running " << suite_name << " sweep -> " << output_path << '\n';

    for (const int n : sizes) {
        CholeskyBenchmarkConfig config;
        config.n = n;
        config.repetitions = repetitions;
        config.warmup = warmup;
        config.label = label;

        std::cerr.clear();
        const auto result = run_cholesky_benchmark(config, std::cerr);
        if (result.min_seconds < 0.0) {
            return false;
        }

        file << "===== n=" << n << '\n';
        write_benchmark_result(file, result);
        file << '\n';

        std::cout << "[INFO] n=" << n << " median_seconds=" << result.median_seconds
                  << " median_gflops=" << result.median_gflops << '\n';
    }

    return true;
}

}  // namespace

int main(int argc, char** argv) {
    std::string label = "cholesky_v0_baseline";
    int repetitions = 7;
    int warmup = 1;

    if (argc >= 2) {
        label = argv[1];
    }
    if (argc >= 3) {
        repetitions = std::stoi(argv[2]);
    }
    if (argc >= 4) {
        warmup = std::stoi(argv[3]);
    }

    const std::filesystem::path output_dir = std::filesystem::path("history") / "results" / label;
    std::filesystem::create_directories(output_dir);

    {
        const std::filesystem::path correctness_path = output_dir / "correctness.txt";
        std::ofstream correctness_file(correctness_path);
        if (!correctness_file) {
            std::cerr << "failed to open " << correctness_path << '\n';
            return 1;
        }

        std::cout << "[INFO] Running correctness suite -> " << correctness_path << '\n';
        const CholeskyTestSummary summary = run_cholesky_tests(correctness_file, correctness_file);
        correctness_file << "total=" << summary.total << '\n';
        correctness_file << "failures=" << summary.failures << '\n';

        if (summary.failures != 0) {
            std::cerr << "correctness suite failed\n";
            return 1;
        }
    }

    const std::vector<int> coarse_sizes = {1, 2, 4, 8, 10, 16, 32, 64, 100, 124, 128, 256, 512, 1000, 1024};
    const std::vector<int> fine_sizes = {992, 1000, 1008, 1013, 1024, 1032};
    const std::vector<int> large_sizes = {1152, 1280};

    if (!run_and_record_sweep(output_dir / "time_coarse.txt",
                              "coarse",
                              coarse_sizes,
                              repetitions,
                              warmup,
                              label)) {
        return 1;
    }

    if (!run_and_record_sweep(output_dir / "time_fine.txt",
                              "fine",
                              fine_sizes,
                              repetitions,
                              warmup,
                              label)) {
        return 1;
    }

    if (!run_and_record_sweep(output_dir / "time_large.txt",
                              "large",
                              large_sizes,
                              repetitions,
                              warmup,
                              label)) {
        return 1;
    }

    const std::filesystem::path summary_path = output_dir / "summary.txt";
    std::ofstream summary_file(summary_path);
    if (!summary_file) {
        std::cerr << "failed to open " << summary_path << '\n';
        return 1;
    }

    summary_file << "label=" << label << '\n';
    summary_file << "repetitions=" << repetitions << '\n';
    summary_file << "warmup=" << warmup << '\n';
    summary_file << "correctness=passed\n";
    summary_file << "files=correctness.txt,time_coarse.txt,time_fine.txt,time_large.txt\n";

    std::cout << "[INFO] Wrote suite results to " << output_dir << '\n';
    return 0;
}
