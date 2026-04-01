#include "build_info.h"
#include "cholesky_benchmark_support.h"
#include "cholesky_test_suite.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/utsname.h>
#include <unistd.h>
#include <vector>
#ifdef _OPENMP
#include <omp.h>  // The suite controls explicit thread counts for the fixed-size scaling sweep.
#endif

namespace {

std::string shell_quote(const std::string& value) {
    std::string quoted = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            quoted += "'\"'\"'";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

std::string benchmark_path_from_suite_path(const char* argv0) {
    std::filesystem::path executable_path = std::filesystem::absolute(std::filesystem::path(argv0));
    const std::string suite_name = executable_path.filename().string();
    constexpr const char* suite_suffix = "_benchmark_suite";

    if (suite_name.size() > std::char_traits<char>::length(suite_suffix) &&
        suite_name.compare(suite_name.size() - std::char_traits<char>::length(suite_suffix),
                           std::char_traits<char>::length(suite_suffix),
                           suite_suffix) == 0) {
        const std::string benchmark_name =
            suite_name.substr(0, suite_name.size() - std::char_traits<char>::length(suite_suffix)) +
            "_benchmark";
        executable_path.replace_filename(benchmark_name);
        return executable_path.string();
    }

    return {};
}

std::string sibling_executable_path_from_suite_path(const char* argv0,
                                                    const std::string& desired_name) {
    std::filesystem::path executable_path = std::filesystem::absolute(std::filesystem::path(argv0));
    executable_path.replace_filename(desired_name);
    return executable_path.string();
}

std::string suite_name_from_path(const char* argv0) {
    return std::filesystem::path(argv0).filename().string();
}

bool suite_supports_openmp_studies(const char* argv0) {
    const std::string suite_name = suite_name_from_path(argv0);
    return suite_name == "cholesky_benchmark_suite" ||
           suite_name == "cholesky_v3_openmp_benchmark_suite" ||
           suite_name == "cholesky_v4_parallel_opt_benchmark_suite";
}

std::string now_utc_iso8601() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time_value = std::chrono::system_clock::to_time_t(now);
    std::tm tm_value{};
#if defined(_POSIX_VERSION)
    gmtime_r(&time_value, &tm_value);
#else
    tm_value = *std::gmtime(&time_value);
#endif
    std::ostringstream out;
    out << std::put_time(&tm_value, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::string hostname_string() {
    char buffer[256] = {};
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        return std::string(buffer);
    }
    return "unknown";
}

std::string uname_string() {
    struct utsname info {};
    if (uname(&info) == 0) {
        return std::string(info.sysname) + " " + info.release + " " + info.machine;
    }
    return "unknown";
}

bool perf_available() {
    const int status = std::system("perf stat -x, -e cycles true >/dev/null 2>/dev/null");
    return status == 0;
}

int current_or_default_int(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return fallback;
    }

    const int parsed = std::atoi(value);
    return parsed > 0 ? parsed : fallback;
}

std::vector<int> parse_int_list(const std::string& value) {
    std::string normalized = value;
    for (char& ch : normalized) {
        if (ch == ',' || ch == ';' || ch == ':') {
            ch = ' ';
        }
    }

    std::vector<int> parsed_values;
    std::stringstream stream(normalized);
    int parsed = 0;
    while (stream >> parsed) {
        if (parsed > 0) {
            parsed_values.push_back(parsed);
        }
    }

    return parsed_values;
}

std::vector<int> current_or_default_int_list(const char* name, const std::vector<int>& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return fallback;
    }

    const std::vector<int> parsed_values = parse_int_list(value);
    return parsed_values.empty() ? fallback : parsed_values;
}

class ScopedEnvironmentOverride {
  public:
    ScopedEnvironmentOverride(const char* name, const std::optional<std::string>& value)
        : name_(name) {
        const char* current = std::getenv(name_);
        if (current != nullptr) {
            had_previous_ = true;
            previous_value_ = current;
        }

        if (value.has_value()) {
            setenv(name_, value->c_str(), 1);
        } else {
            unsetenv(name_);
        }
    }

    ~ScopedEnvironmentOverride() {
        if (had_previous_) {
            setenv(name_, previous_value_.c_str(), 1);
        } else {
            unsetenv(name_);
        }
    }

  private:
    const char* name_;
    bool had_previous_ = false;
    std::string previous_value_;
};

struct ScheduleStudyCase {
    std::string schedule;
    int chunk = 0;
};

void write_metadata_file(const std::filesystem::path& output_path,
                         const std::string& label,
                         const std::string& suite_path,
                         const std::string& benchmark_path,
                         int repetitions,
                         int warmup,
                         bool perf_is_available) {
    std::ofstream file(output_path);
    if (!file) {
        throw std::runtime_error("failed to open metadata file");
    }

    const char* omp_threads = std::getenv("OMP_NUM_THREADS");
    const char* omp_places = std::getenv("OMP_PLACES");
    const char* omp_proc_bind = std::getenv("OMP_PROC_BIND");
    const char* slurm_job_partition = std::getenv("SLURM_JOB_PARTITION");
    const char* slurmd_nodename = std::getenv("SLURMD_NODENAME");
    const char* slurm_nodelist = std::getenv("SLURM_NODELIST");
    const char* cholesky_block_size = std::getenv("CHOLESKY_BLOCK_SIZE");
    const char* cholesky_omp_schedule = std::getenv("CHOLESKY_OMP_SCHEDULE");
    const char* cholesky_omp_chunk = std::getenv("CHOLESKY_OMP_CHUNK");
    const char* schedule_study_n = std::getenv("CHOLESKY_SCHEDULE_STUDY_N");
    const char* schedule_study_threads = std::getenv("CHOLESKY_SCHEDULE_STUDY_THREADS");
    const char* coarse_sizes = std::getenv("CHOLESKY_COARSE_SIZES");
    const char* fine_sizes = std::getenv("CHOLESKY_FINE_SIZES");
    const char* large_sizes = std::getenv("CHOLESKY_LARGE_SIZES");
    const char* perf_sizes = std::getenv("CHOLESKY_PERF_SIZES");
    const char* thread_scaling_counts = std::getenv("CHOLESKY_THREAD_SCALING_COUNTS");
    const char* thread_scaling_extended_sizes = std::getenv("CHOLESKY_THREAD_SCALING_EXTENDED_SIZES");
    const char* thread_scaling_extended_counts = std::getenv("CHOLESKY_THREAD_SCALING_EXTENDED_COUNTS");

    file << "label=" << label << '\n';
    file << "timestamp_utc=" << now_utc_iso8601() << '\n';
    file << "hostname=" << hostname_string() << '\n';
    file << "platform=" << uname_string() << '\n';
    file << "compiler_id=" << CHOLESKY_BUILD_COMPILER_ID << '\n';
    file << "compiler_version=" << CHOLESKY_BUILD_COMPILER_VERSION << '\n';
    file << "build_type=" << CHOLESKY_BUILD_TYPE << '\n';
    file << "benchmark_suite_executable=" << suite_path << '\n';
    file << "benchmark_executable=" << benchmark_path << '\n';
    file << "repetitions=" << repetitions << '\n';
    file << "warmup=" << warmup << '\n';
    file << "omp_num_threads=" << (omp_threads != nullptr ? omp_threads : "unset") << '\n';
    file << "omp_places=" << (omp_places != nullptr ? omp_places : "unset") << '\n';
    file << "omp_proc_bind=" << (omp_proc_bind != nullptr ? omp_proc_bind : "unset") << '\n';
    file << "slurm_job_partition=" << (slurm_job_partition != nullptr ? slurm_job_partition : "unset") << '\n';
    file << "slurmd_nodename=" << (slurmd_nodename != nullptr ? slurmd_nodename : "unset") << '\n';
    file << "slurm_nodelist=" << (slurm_nodelist != nullptr ? slurm_nodelist : "unset") << '\n';
    file << "cholesky_block_size=" << (cholesky_block_size != nullptr ? cholesky_block_size : "unset") << '\n';
    file << "cholesky_omp_schedule=" << (cholesky_omp_schedule != nullptr ? cholesky_omp_schedule : "static") << '\n';
    file << "cholesky_omp_chunk=" << (cholesky_omp_chunk != nullptr ? cholesky_omp_chunk : "0") << '\n';
    file << "schedule_study_n=" << (schedule_study_n != nullptr ? schedule_study_n : "1024") << '\n';
    file << "schedule_study_threads=" << (schedule_study_threads != nullptr ? schedule_study_threads : "4") << '\n';
    file << "coarse_sizes=" << (coarse_sizes != nullptr ? coarse_sizes : "default") << '\n';
    file << "fine_sizes=" << (fine_sizes != nullptr ? fine_sizes : "default") << '\n';
    file << "large_sizes=" << (large_sizes != nullptr ? large_sizes : "default") << '\n';
    file << "perf_sizes=" << (perf_sizes != nullptr ? perf_sizes : "default") << '\n';
    file << "thread_scaling_counts=" << (thread_scaling_counts != nullptr ? thread_scaling_counts : "default") << '\n';
    file << "thread_scaling_extended_sizes="
         << (thread_scaling_extended_sizes != nullptr ? thread_scaling_extended_sizes : "default") << '\n';
    file << "thread_scaling_extended_counts="
         << (thread_scaling_extended_counts != nullptr ? thread_scaling_extended_counts : "default") << '\n';
    file << "perf_available=" << (perf_is_available ? "yes" : "no") << '\n';
    file << "perf_events=cycles,instructions,cache-references,cache-misses\n";
}

bool collect_perf_stat(const std::filesystem::path& text_output_path,
                       const std::filesystem::path& csv_output_path,
                       const std::string& benchmark_path,
                       const std::vector<int>& sizes,
                       int repetitions,
                       int warmup,
                       const std::string& label) {
    std::ofstream text_file(text_output_path);
    std::ofstream csv_file(csv_output_path);
    if (!text_file || !csv_file) {
        std::cerr << "failed to open perf output files\n";
        return false;
    }

    if (!perf_available()) {
        text_file << "status=skipped\nreason=perf_unavailable\n";
        csv_file << "status,event,n,count,unit,runtime_ns,enabled_pct,metric_value,metric_unit\n";
        csv_file << "skipped,,,,,,,,\n";
        std::cout << "[INFO] perf stat unavailable, skipped perf sweep\n";
        return true;
    }

    csv_file << "status,event,n,count,unit,runtime_ns,enabled_pct,metric_value,metric_unit\n";
    text_file << "benchmark=" << benchmark_path << '\n';
    text_file << "events=cycles,instructions,cache-references,cache-misses\n";
    text_file << "repetitions=" << repetitions << '\n';
    text_file << "warmup=" << warmup << '\n';

    if (!std::filesystem::exists(benchmark_path)) {
        text_file << "status=skipped\nreason=benchmark_executable_missing\n";
        csv_file << "skipped,,,,,,,,\n";
        return true;
    }

    for (const int n : sizes) {
        const std::string command =
            "perf stat -x, -e cycles,instructions,cache-references,cache-misses " +
            shell_quote(benchmark_path) + " " +
            std::to_string(n) + " " +
            std::to_string(repetitions) + " " +
            std::to_string(warmup) + " " +
            shell_quote(label + "_perf") +
            " 2>&1 >/dev/null";

        std::cout << "[INFO] Running perf sweep for n=" << n << '\n';
        text_file << "===== n=" << n << '\n';
        text_file << "command=" << command << '\n';

        FILE* pipe = popen(command.c_str(), "r");
        if (pipe == nullptr) {
            text_file << "status=failed\nreason=popen_failed\n\n";
            csv_file << "failed,," << n << ",,,,,,\n";
            return false;
        }

        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            const std::string line(buffer);
            text_file << line;

            std::stringstream line_stream(line);
            std::vector<std::string> fields;
            std::string field;
            while (std::getline(line_stream, field, ',')) {
                fields.push_back(field);
            }

            if (fields.size() < 3) {
                continue;
            }

            if (fields[2].find(':') == std::string::npos && fields[2] != "cycles" &&
                fields[2] != "instructions" && fields[2] != "cache-references" &&
                fields[2] != "cache-misses") {
                continue;
            }

            const std::string event = fields[2];
            const std::string count = fields.size() > 0 ? fields[0] : "";
            const std::string unit = fields.size() > 1 ? fields[1] : "";
            const std::string runtime_ns = fields.size() > 3 ? fields[3] : "";
            const std::string enabled_pct = fields.size() > 4 ? fields[4] : "";
            const std::string metric_value = fields.size() > 5 ? fields[5] : "";
            const std::string metric_unit = fields.size() > 6 ? fields[6] : "";

            csv_file << "ok,"
                     << event << ','
                     << n << ','
                     << count << ','
                     << unit << ','
                     << runtime_ns << ','
                     << enabled_pct << ','
                     << metric_value << ','
                     << metric_unit << '\n';
        }

        const int status = pclose(pipe);
        text_file << "exit_status=" << status << "\n\n";
        if (status != 0) {
            csv_file << "failed,," << n << ",,,,,,\n";
            return false;
        }
    }

    return true;
}

bool run_and_record_sweep(const std::filesystem::path& output_path,
                          std::ostream& csv_out,
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
        write_benchmark_csv_row(csv_out, suite_name, result);

        std::cout << "[INFO] n=" << n << " median_seconds=" << result.median_seconds
                  << " median_gflops=" << result.median_gflops << '\n';
    }

    return true;
}

bool run_thread_scaling_sweep(const std::filesystem::path& text_output_path,
                              const std::filesystem::path& csv_output_path,
                              int n,
                              const std::vector<int>& thread_counts,
                              int repetitions,
                              int warmup,
                              const std::string& label) {
    std::ofstream text_file(text_output_path);
    std::ofstream csv_file(csv_output_path);
    if (!text_file || !csv_file) {
        std::cerr << "failed to open thread scaling outputs\n";
        return false;
    }

    write_benchmark_csv_header(csv_file);
    text_file << "n=" << n << '\n';

#ifdef _OPENMP
    const int previous_dynamic = omp_get_dynamic();
    const int previous_max_threads = omp_get_max_threads();
    // Disable dynamic team resizing so each scaling point uses the exact requested thread count.
    omp_set_dynamic(0);
#endif

    for (const int requested_threads : thread_counts) {
        CholeskyBenchmarkConfig config;
        config.n = n;
        config.repetitions = repetitions;
        config.warmup = warmup;
        config.label = label + "_t" + std::to_string(requested_threads);

#ifdef _OPENMP
        // Force a specific OpenMP team size so the thread-scaling sweep measures fixed thread counts on the same problem size.
        omp_set_num_threads(requested_threads);
#endif

        std::cerr.clear();
        CholeskyBenchmarkResult result = run_cholesky_benchmark(config, std::cerr);
        if (result.min_seconds < 0.0) {
#ifdef _OPENMP
            omp_set_num_threads(previous_max_threads);
            omp_set_dynamic(previous_dynamic);
#endif
            return false;
        }

        result.requested_threads = requested_threads;
        text_file << "===== threads=" << requested_threads << '\n';
        write_benchmark_result(text_file, result);
        text_file << '\n';
        write_benchmark_csv_row(csv_file, "thread_scaling", result);

        std::cout << "[INFO] threads=" << requested_threads
                  << " n=" << n
                  << " median_seconds=" << result.median_seconds
                  << " median_gflops=" << result.median_gflops << '\n';
    }

#ifdef _OPENMP
    omp_set_num_threads(previous_max_threads);
    omp_set_dynamic(previous_dynamic);
#endif

    return true;
}

bool run_thread_scaling_extended_sweep(const std::filesystem::path& text_output_path,
                                       const std::filesystem::path& csv_output_path,
                                       const std::vector<int>& sizes,
                                       const std::vector<int>& thread_counts,
                                       int repetitions,
                                       int warmup,
                                       const std::string& label) {
    std::ofstream text_file(text_output_path);
    std::ofstream csv_file(csv_output_path);
    if (!text_file || !csv_file) {
        std::cerr << "failed to open extended thread scaling outputs\n";
        return false;
    }

    write_benchmark_csv_header(csv_file);

    text_file << "sizes=";
    for (std::size_t index = 0; index < sizes.size(); ++index) {
        if (index != 0) {
            text_file << ',';
        }
        text_file << sizes[index];
    }
    text_file << '\n';

    text_file << "thread_counts=";
    for (std::size_t index = 0; index < thread_counts.size(); ++index) {
        if (index != 0) {
            text_file << ',';
        }
        text_file << thread_counts[index];
    }
    text_file << "\n\n";

#ifdef _OPENMP
    const int previous_dynamic = omp_get_dynamic();
    const int previous_max_threads = omp_get_max_threads();
    omp_set_dynamic(0);
#endif

    for (const int n : sizes) {
        std::cout << "[INFO] Running extended thread scaling sweep for n=" << n << '\n';
        for (const int requested_threads : thread_counts) {
            CholeskyBenchmarkConfig config;
            config.n = n;
            config.repetitions = repetitions;
            config.warmup = warmup;
            config.label =
                label + "_thread_ext_n" + std::to_string(n) + "_t" + std::to_string(requested_threads);

#ifdef _OPENMP
            omp_set_num_threads(requested_threads);
#endif

            std::cerr.clear();
            CholeskyBenchmarkResult result = run_cholesky_benchmark(config, std::cerr);
            if (result.min_seconds < 0.0) {
#ifdef _OPENMP
                omp_set_num_threads(previous_max_threads);
                omp_set_dynamic(previous_dynamic);
#endif
                return false;
            }

            result.requested_threads = requested_threads;
            text_file << "===== n=" << n << " threads=" << requested_threads << '\n';
            write_benchmark_result(text_file, result);
            text_file << '\n';
            write_benchmark_csv_row(csv_file,
                                    "thread_scaling_extended_n" + std::to_string(n),
                                    result);

            std::cout << "[INFO] extended threads=" << requested_threads
                      << " n=" << n
                      << " median_seconds=" << result.median_seconds
                      << " median_gflops=" << result.median_gflops << '\n';
        }
    }

#ifdef _OPENMP
    omp_set_num_threads(previous_max_threads);
    omp_set_dynamic(previous_dynamic);
#endif

    return true;
}

bool run_schedule_study(const std::filesystem::path& text_output_path,
                        const std::filesystem::path& csv_output_path,
                        int n,
                        int requested_threads,
                        const std::vector<ScheduleStudyCase>& study_cases,
                        int repetitions,
                        int warmup,
                        const std::string& label) {
    std::ofstream text_file(text_output_path);
    std::ofstream csv_file(csv_output_path);
    if (!text_file || !csv_file) {
        std::cerr << "failed to open schedule study outputs\n";
        return false;
    }

    write_benchmark_csv_header(csv_file);
    text_file << "n=" << n << '\n';
    text_file << "requested_threads=" << requested_threads << '\n';

#ifdef _OPENMP
    const int previous_dynamic = omp_get_dynamic();
    const int previous_max_threads = omp_get_max_threads();
    // Disable dynamic team resizing so schedule comparisons reuse a fixed team size on the same problem size.
    omp_set_dynamic(0);
    // Force a fixed team size so schedule and chunk-size comparisons isolate runtime policy rather than thread-count drift.
    omp_set_num_threads(requested_threads);
#endif

    ScopedEnvironmentOverride thread_override("OMP_NUM_THREADS", std::to_string(requested_threads));

    for (const ScheduleStudyCase& study_case : study_cases) {
        ScopedEnvironmentOverride schedule_override("CHOLESKY_OMP_SCHEDULE", study_case.schedule);
        ScopedEnvironmentOverride chunk_override(
            "CHOLESKY_OMP_CHUNK",
            study_case.chunk > 0 ? std::optional<std::string>(std::to_string(study_case.chunk))
                                 : std::optional<std::string>("0"));

        CholeskyBenchmarkConfig config;
        config.n = n;
        config.repetitions = repetitions;
        config.warmup = warmup;
        config.label = label + "_" + study_case.schedule +
                       (study_case.chunk > 0 ? "_c" + std::to_string(study_case.chunk) : "_default");

        std::cerr.clear();
        CholeskyBenchmarkResult result = run_cholesky_benchmark(config, std::cerr);
        if (result.min_seconds < 0.0) {
#ifdef _OPENMP
            omp_set_num_threads(previous_max_threads);
            omp_set_dynamic(previous_dynamic);
#endif
            return false;
        }

        result.requested_threads = requested_threads;
        text_file << "===== schedule=" << study_case.schedule << " chunk=" << study_case.chunk << '\n';
        write_benchmark_result(text_file, result);
        text_file << '\n';
        write_benchmark_csv_row(csv_file, "schedule_study", result);

        std::cout << "[INFO] schedule=" << study_case.schedule
                  << " chunk=" << study_case.chunk
                  << " threads=" << requested_threads
                  << " n=" << n
                  << " median_seconds=" << result.median_seconds
                  << " median_gflops=" << result.median_gflops << '\n';
    }

#ifdef _OPENMP
    omp_set_num_threads(previous_max_threads);
    omp_set_dynamic(previous_dynamic);
#endif

    return true;
}

bool run_optional_report_test(const std::string& executable_path,
                              const std::filesystem::path& report_path,
                              const std::string& display_name) {
    if (!std::filesystem::exists(executable_path)) {
        std::cout << "[INFO] Skipping " << display_name << " -> executable not found\n";
        return true;
    }

    const std::string command =
        shell_quote(executable_path) + " " + shell_quote(report_path.string()) + " >/dev/null";
    std::cout << "[INFO] Running " << display_name << " -> " << report_path << '\n';
    const int status = std::system(command.c_str());
    return status == 0;
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
    const bool supports_openmp_studies = suite_supports_openmp_studies(argv[0]);
    const std::string benchmark_path = benchmark_path_from_suite_path(argv[0]);
    const std::string against_v1_test_path =
        sibling_executable_path_from_suite_path(argv[0], "cholesky_against_v1_tests");
    const std::string stability_test_path =
        sibling_executable_path_from_suite_path(argv[0], "cholesky_stability_tests");
    const std::string openmp_consistency_test_path =
        sibling_executable_path_from_suite_path(argv[0], "cholesky_openmp_consistency_tests");
    const std::filesystem::path csv_path = output_dir / "sweeps.csv";
    std::ofstream csv_file(csv_path);
    if (!csv_file) {
        std::cerr << "failed to open " << csv_path << '\n';
        return 1;
    }
    write_benchmark_csv_header(csv_file);

    try {
        write_metadata_file(output_dir / "metadata.txt",
                            label,
                            std::filesystem::absolute(std::filesystem::path(argv[0])).string(),
                            benchmark_path,
                            repetitions,
                            warmup,
                            perf_available());
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }

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

    if (!run_optional_report_test(against_v1_test_path,
                                  output_dir / "against_v1.txt",
                                  "against_v1 regression")) {
        std::cerr << "against_v1 regression failed\n";
        return 1;
    }

    if (!run_optional_report_test(stability_test_path,
                                  output_dir / "stability.txt",
                                  "stability suite")) {
        std::cerr << "stability suite failed\n";
        return 1;
    }

    if (!run_optional_report_test(openmp_consistency_test_path,
                                  output_dir / "omp_consistency.txt",
                                  "OpenMP consistency suite")) {
        std::cerr << "OpenMP consistency suite failed\n";
        return 1;
    }

    const std::vector<int> coarse_sizes = current_or_default_int_list(
        "CHOLESKY_COARSE_SIZES",
        {1, 2, 4, 8, 10, 16, 32, 64, 100, 124, 127, 128, 129, 255, 256, 257, 511, 512, 513, 1000, 1024});
    const std::vector<int> fine_sizes = current_or_default_int_list(
        "CHOLESKY_FINE_SIZES",
        {992, 1000, 1008, 1013, 1023, 1024, 1025, 1032});
    const std::vector<int> large_sizes = current_or_default_int_list(
        "CHOLESKY_LARGE_SIZES",
        {1152, 1280, 1536, 1792, 2048, 2304});
    const std::vector<int> thread_scaling_counts = current_or_default_int_list(
        "CHOLESKY_THREAD_SCALING_COUNTS",
        {1, 2, 4, 16, 32});
    const std::vector<int> thread_scaling_extended_sizes = current_or_default_int_list(
        "CHOLESKY_THREAD_SCALING_EXTENDED_SIZES",
        {256, 1024, 2048});
    const std::vector<int> thread_scaling_extended_counts = current_or_default_int_list(
        "CHOLESKY_THREAD_SCALING_EXTENDED_COUNTS",
        {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 64});
    const int schedule_study_threads = current_or_default_int("CHOLESKY_SCHEDULE_STUDY_THREADS", 4);
    const int schedule_study_n = current_or_default_int("CHOLESKY_SCHEDULE_STUDY_N", 1024);
    const std::vector<ScheduleStudyCase> schedule_study_cases = {
        {"static", 0},
        {"static", 1},
        {"static", 8},
        {"dynamic", 1},
        {"dynamic", 8},
        {"guided", 1},
        {"guided", 8},
    };

    if (!run_and_record_sweep(output_dir / "time_coarse.txt",
                              csv_file,
                              "coarse",
                              coarse_sizes,
                              repetitions,
                              warmup,
                              label)) {
        return 1;
    }

    if (!run_and_record_sweep(output_dir / "time_fine.txt",
                              csv_file,
                              "fine",
                              fine_sizes,
                              repetitions,
                              warmup,
                              label)) {
        return 1;
    }

    if (!run_and_record_sweep(output_dir / "time_large.txt",
                              csv_file,
                              "large",
                              large_sizes,
                              repetitions,
                              warmup,
                              label)) {
        return 1;
    }

    if (supports_openmp_studies) {
        if (!run_thread_scaling_sweep(output_dir / "thread_scaling.txt",
                                      output_dir / "thread_scaling.csv",
                                      1024,
                                      thread_scaling_counts,
                                      repetitions,
                                      warmup,
                                      label)) {
            return 1;
        }

        if (!run_thread_scaling_extended_sweep(output_dir / "thread_scaling_extended.txt",
                                               output_dir / "thread_scaling_extended.csv",
                                               thread_scaling_extended_sizes,
                                               thread_scaling_extended_counts,
                                               repetitions,
                                               warmup,
                                               label)) {
            return 1;
        }

        if (!run_schedule_study(output_dir / "schedule_study.txt",
                                output_dir / "schedule_study.csv",
                                schedule_study_n,
                                schedule_study_threads,
                                schedule_study_cases,
                                repetitions,
                                warmup,
                                label)) {
            return 1;
        }
    }

    const std::vector<int> perf_sizes = current_or_default_int_list(
        "CHOLESKY_PERF_SIZES",
        {128, 256, 512, 1000, 1024, 1536, 1792, 2048, 2304});
    if (!benchmark_path.empty()) {
        if (!collect_perf_stat(output_dir / "perf_basic.txt",
                               output_dir / "perf_basic.csv",
                               benchmark_path,
                               perf_sizes,
                               repetitions,
                               warmup,
                               label)) {
            return 1;
        }
    } else {
        std::ofstream text_file(output_dir / "perf_basic.txt");
        std::ofstream perf_csv_file(output_dir / "perf_basic.csv");
        if (!text_file || !perf_csv_file) {
            std::cerr << "failed to open perf fallback files\n";
            return 1;
        }
        text_file << "status=skipped\nreason=benchmark_executable_path_unresolved\n";
        perf_csv_file << "status,event,n,count,unit,runtime_ns,enabled_pct,metric_value,metric_unit\n";
        perf_csv_file << "skipped,,,,,,,,\n";
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
    summary_file << "files=correctness.txt,against_v1.txt,stability.txt,omp_consistency.txt,time_coarse.txt,time_fine.txt,time_large.txt";
    if (supports_openmp_studies) {
        summary_file << ",thread_scaling.txt,thread_scaling.csv,thread_scaling_extended.txt,thread_scaling_extended.csv,schedule_study.txt,schedule_study.csv";
    }
    summary_file << ",sweeps.csv,metadata.txt,perf_basic.txt,perf_basic.csv,reference_logdet.txt,reference_logdet.csv\n";

    std::cout << "[INFO] Wrote suite results to " << output_dir << '\n';
    return 0;
}
