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
- The benchmark suite also writes `sweeps.csv` for analysis-ready summary rows.

## Performance guidance

- Build a release configuration for any meaningful timing work.
- Treat the routine as in-place: benchmark runs must start from a fresh copy of the original matrix each time.
- Use contiguous row-major storage exactly matching the required `double*` interface.
- Compare multiple matrix sizes, including both coarse growth and fine-grained cases around cache-sensitive sizes.
- Use compute nodes rather than login nodes for serious performance measurements.
