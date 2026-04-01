# C2 HPC Coursework - OpenMP Cholesky Factorization

Coursework repository for a CPU-based Cholesky factorization routine written in C++ for single-node execution.

## Requirements

- CMake 3.16 or newer
- A C++17 compiler

## Quick start

Build the current version:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target cholesky_all_variants
```

Or use the one-command wrapper for the current version:

```bash
./scripts/run_release_suite.sh cholesky cholesky_v3_openmp 7 1
```

Run the most useful entrypoints:

```bash
./build-release/cholesky_example
./build-release/cholesky_against_v1_tests
./build-release/cholesky_stability_tests
OMP_NUM_THREADS=1 ./build-release/cholesky_benchmark_suite cholesky_v3_openmp 7 1
```

What these give you:

- `cholesky_example` shows the basic in-place API on the coursework 2x2 case.
- `cholesky_against_v1_tests` checks the current implementation against archived `v1`, including block-boundary sizes.
- `cholesky_stability_tests` checks repeatability and block-size invariance across representative non-divisible sizes.
- `cholesky_benchmark_suite` writes a full result bundle to `history/results/<label>/`, including `against_v1.txt`, `stability.txt`, `omp_consistency.txt`, and a fixed `n=1024` thread-scaling study.
- `scripts/run_release_suite.sh` wraps the formal release configure, build, current-version tests, and benchmark suite into one command.

For `cholesky_benchmark_suite <label> 7 1`, the result directory contains:

- `correctness.txt`
- `against_v1.txt`
- `stability.txt`
- `omp_consistency.txt`
- `thread_scaling.txt`
- `thread_scaling.csv`
- `time_coarse.txt`
- `time_fine.txt`
- `time_large.txt`
- `sweeps.csv`
- `metadata.txt`
- `perf_basic.txt`
- `perf_basic.csv`
- `summary.txt`

For the current OpenMP version, you can also set the block size explicitly:

```bash
OMP_NUM_THREADS=1 CHOLESKY_BLOCK_SIZE=32 ./build-release/cholesky_benchmark_suite cholesky_v3_openmp 7 1
```

## Build options

Debug build for development checks:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Release build for formal performance runs:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target cholesky_all_variants
```

This produces the static library `build-release/libcholesky.a` together with the example, test, and benchmark executables.

## Common commands

Debug correctness:

```bash
ctest --test-dir build --output-on-failure
./build/cholesky_tests
./build/cholesky_against_v1_tests
./build/cholesky_stability_tests
```

Single benchmark run:

```bash
OMP_NUM_THREADS=1 CHOLESKY_BLOCK_SIZE=32 ./build-release/cholesky_benchmark 1024 5 1 cholesky_v3_openmp
```

Formal suite run:

```bash
OMP_NUM_THREADS=1 CHOLESKY_BLOCK_SIZE=32 ./build-release/cholesky_benchmark_suite cholesky_v3_openmp 7 1
```

One-command formal run for the current version:

```bash
./scripts/run_release_suite.sh cholesky cholesky_v3_openmp 7 1
```

Archived version run:

```bash
OMP_NUM_THREADS=1 ./build-release/cholesky_v1_serial_opt_benchmark_suite cholesky_v1_serial_opt 7 1
```

One-command archived version run:

```bash
./scripts/run_release_suite.sh cholesky_v1_serial_opt cholesky_v1_serial_opt 7 1
```

## What to read next

- If you want to use the library in your own code, go to `Build and use the library`.
- If you want to compare versions or rerun archived milestones, go to `Rerunning archived versions`.
- If you want the benchmark/result format, go to `Validation and benchmarking`.

## Build and use the library

Public header:

```cpp
#include "mphil_dis_cholesky.h"
```

Function interface:

```cpp
double cholesky(double* c, int n);
```

Minimal usage:

```cpp
double matrix[] = {
    4.0, 2.0,
    2.0, 26.0,
};

double elapsed = cholesky(matrix, 2);
```

After the call, `matrix` is overwritten in place with the Cholesky factor in the diagonal and lower triangle, and the mirrored transpose values in the upper triangle.

If you want to compile and link manually after building, use the generated static library and the public include directory, for example:

```bash
icpx -std=c++17 -Iinclude your_program.cpp build/libcholesky.a -o your_program
```

## Validation and benchmarking

- `cholesky_tests` runs the baseline correctness checks.
- `cholesky_against_v1_tests` compares the current implementation against the archived `v1` reference on block-boundary sizes.
- `cholesky_stability_tests [optional_report_path]` checks repeatability and verifies that representative block-size changes do not change the factorization result.
- `cholesky_openmp_consistency_tests [optional_report_path]` compares the current OpenMP-enabled implementation against archived `v2` while forcing thread counts such as `1`, `2`, and `4`.
- `cholesky_benchmark <n> [repetitions] [warmup] [label]` runs the serial benchmark driver on a generated SPD matrix.
- `cholesky_benchmark_suite [label] [repetitions] [warmup]` runs correctness plus archived-`v1` regression, stability checks, OpenMP consistency checks, coarse/fine/large timing sweeps, and a fixed `n=1024` thread-scaling sweep at thread counts `1`, `2`, `4`, `16`, and `32`, then writes the outputs to `history/results/<label>/`.
- `scripts/run_release_suite.sh [variant] [label] [repetitions] [warmup]` is the canonical one-command release entrypoint for both the current implementation and archived milestone variants.
- The benchmark suite also writes `sweeps.csv`, `metadata.txt`, `perf_basic.txt`, and `perf_basic.csv`.
- Canonical results under `history/results/<label>/` are part of the tracked repository history for each formal version.
- Archived versions in `history/versions/` are also buildable targets, so older tagged code can be rerun without checking out a different branch or tag.

## Performance guidance

- Use `build/` for `Debug` checks and `build-release/` for formal performance runs.
- `build-release/` remains the canonical release build directory regardless of which compiler is selected for that build.
- Build a release configuration for any meaningful timing work.
- Set `OMP_NUM_THREADS=1` for serial benchmark runs so the runtime environment is explicit.
- Treat the routine as in-place: benchmark runs must start from a fresh copy of the original matrix each time.
- Use contiguous row-major storage exactly matching the required `double*` interface.
- Compare multiple matrix sizes, including both coarse growth and fine-grained cases around cache-sensitive sizes.
- Use compute nodes rather than login nodes for serious performance measurements.
- Keep one primary compiler setting for cross-version comparisons; use separate runs if you want to study `-O1`, `-O2`, and `-O3`.
- The current suite records `cycles`, `instructions`, `cache-references`, and `cache-misses` through `perf stat` when available.
- If your environment defaults to an `icpx` toolchain that fails while linking the test binaries, configure a dedicated test build with `-DCMAKE_CXX_COMPILER=g++` and use that build directory for the test executables.
- The standard wrapper script uses `g++` by default so the formal release workflow stays one-command and avoids the current environment-specific linker problem.

## Rerunning archived versions

Once a version has been copied into `history/versions/<version>/`, CMake also exposes dedicated targets for it:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target cholesky_v0_baseline_tests cholesky_v0_baseline_benchmark_suite
OMP_NUM_THREADS=1 ./build-release/cholesky_v0_baseline_tests
OMP_NUM_THREADS=1 ./build-release/cholesky_v0_baseline_benchmark_suite cholesky_v0_baseline 7 1
```

The same pattern applies to later archived versions such as `cholesky_v1_serial_opt`.

To build the current version together with all archived version tests and benchmark executables in one step, use:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target cholesky_all_variants
```

The aggregate targets also exist separately as:

- `cholesky_all_tests`
- `cholesky_all_benchmarks`
- `cholesky_all_benchmark_suites`
- `cholesky_all_variants`

Compiler-flags study for a fixed version should be run separately from the main version-to-version comparison.

## Repository layout

- `bench/` benchmark programs
- `history/` archived versions and benchmark outputs
- `include/` public headers
- `src/` source files
- `example/` example programs
- `test/` tests
- `report/` report files
- `scripts/csd3/` job scripts for CSD3
