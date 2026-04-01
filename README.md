# C2 HPC Coursework - OpenMP Cholesky Factorization

Coursework repository for a CPU-based Cholesky factorization routine written in C++ for single-node execution.

## Requirements

- CMake 3.16 or newer
- A C++17 compiler

## Configure

```bash
cmake -S . -B build
cmake --build build
```

This produces the static library `build/libcholesky.a` together with the example, test, and benchmark executables.

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

## Run the current utilities

```bash
./build/cholesky_example
ctest --test-dir build --output-on-failure
./build/cholesky_benchmark 512 5 1 cholesky_v0_baseline
./build/cholesky_benchmark_suite cholesky_v0_baseline 7 1
```

## Repository layout

- `bench/` benchmark programs
- `history/` archived versions and benchmark outputs
- `include/` public headers
- `src/` source files
- `example/` example programs
- `test/` tests
- `report/` report files
- `scripts/csd3/` job scripts for CSD3

## Public interface

The Cholesky routine is exposed through `include/mphil_dis_cholesky.h`.

## Validation and benchmarking

- `cholesky_tests` runs the baseline correctness checks.
- `cholesky_benchmark <n> [repetitions] [warmup] [label]` runs the serial benchmark driver on a generated SPD matrix.
- `cholesky_benchmark_suite [label] [repetitions] [warmup]` runs correctness plus coarse, fine, and large timing sweeps and writes the outputs to `history/results/<label>/`.
- The benchmark suite also writes `sweeps.csv`, `metadata.txt`, `perf_basic.txt`, and `perf_basic.csv`.
- Canonical results under `history/results/<label>/` are part of the tracked repository history for each formal version.
- Archived versions in `history/versions/` are also buildable targets, so older tagged code can be rerun without checking out a different branch or tag.

## Performance guidance

- Use `build/` for `Debug` checks and `build-release/` for formal performance runs.
- Build a release configuration for any meaningful timing work.
- Set `OMP_NUM_THREADS=1` for serial benchmark runs so the runtime environment is explicit.
- Treat the routine as in-place: benchmark runs must start from a fresh copy of the original matrix each time.
- Use contiguous row-major storage exactly matching the required `double*` interface.
- Compare multiple matrix sizes, including both coarse growth and fine-grained cases around cache-sensitive sizes.
- Use compute nodes rather than login nodes for serious performance measurements.
- Keep one primary compiler setting for cross-version comparisons; use separate runs if you want to study `-O1`, `-O2`, and `-O3`.
- The current suite records `cycles`, `instructions`, `cache-references`, and `cache-misses` through `perf stat` when available.

## Recommended formal run workflow

Debug validation:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/cholesky_tests
```

Release results for a formal version:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
./build-release/cholesky_tests
OMP_NUM_THREADS=1 ./build-release/cholesky_benchmark_suite cholesky_v1_serial_opt 7 1
```

Compiler-flags study for a fixed version should be run separately from the main version-to-version comparison.

## Rerunning archived versions

Once a version has been copied into `history/versions/<version>/`, CMake also exposes dedicated targets for it:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target cholesky_v0_baseline_tests cholesky_v0_baseline_benchmark_suite
OMP_NUM_THREADS=1 ./build-release/cholesky_v0_baseline_tests
OMP_NUM_THREADS=1 ./build-release/cholesky_v0_baseline_benchmark_suite cholesky_v0_baseline 7 1
```

The same pattern applies to later archived versions such as `cholesky_v1_serial_opt`.
