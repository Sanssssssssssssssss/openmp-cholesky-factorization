# C2 HPC Coursework: OpenMP Cholesky Factorization

This repository contains a CPU-only Cholesky factorization implementation for the Cambridge MPhil DIS C2 HPC coursework. The code is written in C++, operates in place on a row-major `double*` matrix, and is developed through tagged milestone versions from a serial baseline to an OpenMP-optimized implementation. The repository also includes tests, reproducible benchmark runners, archived milestone snapshots, and CSD3 submission scripts.

The coursework-facing public interface is the header:

```cpp
#include "mphil_dis_cholesky.h"
```

## Quick Start

Build everything needed for the current code and all archived milestone versions:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build-release --target cholesky_all_variants
```

Submit one chosen milestone on CSD3 with the convenience shell wrapper:

```bash
CSD3_ACCOUNT=<account> ./scripts/csd3/v4.sh
```

Submit all milestone versions plus the current final version on CSD3:

```bash
CSD3_ACCOUNT=<account> ./scripts/csd3/run_all_versions.sh
```

Direct `sbatch` entrypoints are also provided:

```bash
sbatch -A <account> scripts/csd3/v4.slurm
sbatch -A <account> scripts/csd3/run_all_versions.slurm
```

The CSD3 wrappers submit to `icelake` by default, run on one exclusive node, and map directly onto the repository’s existing release workflow in `scripts/run_release_suite.sh` and `scripts/run_release_all_versions.sh`.

## Version Milestones / Tag Mapping

| Milestone | Tag | Description |
|---|---|---|
| v0 | `cholesky_v0_baseline` | First correct serial baseline. |
| v1 | `cholesky_v1_serial_opt` | Serial loop and access-order improvements. |
| v2 | `cholesky_v2_serial_blocked` | Blocked serial implementation with cache-oriented restructuring. |
| v3 | `cholesky_v3_openmp` | First correct OpenMP implementation. |
| v4 | `cholesky_v4_parallel_opt` | Final tuned parallel implementation. |

## Project Structure

- `include/`: public header for the coursework library interface.
- `src/`: current implementation of the Cholesky kernel.
- `example/`: minimal example program using the public API.
- `test/`: correctness, regression, stability, OpenMP consistency, and external-reference validation tools.
- `bench/`: benchmark drivers plus one-line wrappers for single-version and all-version reruns.
- `scripts/`: release wrappers, CSD3 submission scripts, and reporting helpers.
- `scripts/csd3/`: submission-ready `.slurm` and `.sh` entrypoints for CSD3.
- `history/versions/`: archived source snapshots for milestone versions.
- `history/results/`: tracked benchmark/result bundles for milestone versions.
- `report/`: location for the final coursework report, expected as `report/report.pdf`.
- `.dev_memory/`: local-only project memory used during development and not intended for submission.

## Build and Run

Release build:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build-release --target cholesky_all_variants
```

Run the current final implementation locally through the canonical release wrapper:

```bash
./scripts/run_release_suite.sh cholesky cholesky_v4_parallel_opt 7 1
```

Run all milestone versions locally through the canonical release wrapper:

```bash
./scripts/run_release_all_versions.sh 7 1
```

Equivalent benchmark-facing shortcuts:

```bash
./bench/run_version.sh cholesky cholesky_v4_parallel_opt 7 1
./bench/run_all_versions.sh 7 1
```

Useful direct executables after building:

```bash
./build-release/cholesky_example
./build-release/cholesky_tests
./build-release/cholesky_against_v1_tests
./build-release/cholesky_stability_tests
./build-release/cholesky_openmp_consistency_tests
./build-release/cholesky_benchmark_suite cholesky_v4_parallel_opt 7 1
```

## CSD3 Submission

The repository now includes two kinds of CSD3 entrypoints:

- `.sh` wrappers: convenient login-node submission helpers that call `sbatch` for you.
- `.slurm` scripts: direct batch scripts for use with `sbatch`.

Default assumptions in the CSD3 scripts:

- partition: `icelake`
- nodes: `1`
- tasks: `1`
- CPUs per task: `32`
- exclusive node allocation: enabled
- single-version wall time: `02:00:00`
- all-version wall time: `04:00:00`

You can override the account, partition, CPU count, time limit, and benchmark parameters from the shell wrappers:

```bash
CSD3_ACCOUNT=<account> ./scripts/csd3/v3.sh --cpus 32 --time 03:00:00
CSD3_ACCOUNT=<account> ./scripts/csd3/run_all_versions.sh --repetitions 7 --warmup 1
```

You can also pass the repository’s existing runtime controls through the environment, for example:

```bash
CSD3_ACCOUNT=<account> \
C2_THREADS=16 \
CHOLESKY_BLOCK_SIZE=32 \
CHOLESKY_OMP_SCHEDULE=guided \
CHOLESKY_OMP_CHUNK=1 \
./scripts/csd3/v4.sh
```

Available CSD3 convenience wrappers:

- `scripts/csd3/v0.sh`, `scripts/csd3/v0.slurm`
- `scripts/csd3/v1.sh`, `scripts/csd3/v1.slurm`
- `scripts/csd3/v2.sh`, `scripts/csd3/v2.slurm`
- `scripts/csd3/v3.sh`, `scripts/csd3/v3.slurm`
- `scripts/csd3/v4.sh`, `scripts/csd3/v4.slurm`
- `scripts/csd3/run_version.sh`, `scripts/csd3/run_version.slurm`
- `scripts/csd3/run_all_versions.sh`, `scripts/csd3/run_all_versions.slurm`

For CSD3 submission wrappers, the current final version uses:

- variant: `cholesky`
- label: `cholesky_v4_parallel_opt`

Archived milestones are rerun from `history/versions/<tag>/` through dedicated CMake targets in the current repository, so markers do not need to check out old tags manually.

## Results and Report Locations

- Canonical benchmark bundles live in `history/results/<version>/`.
- Milestone source snapshots live in `history/versions/<version>/`.
- The final report should live at `report/report.pdf`.

Typical result bundles contain:

- correctness and regression outputs
- timing sweeps
- thread scaling and schedule studies for the OpenMP versions
- `metadata.txt`
- `perf_basic.txt` and `perf_basic.csv`
- `reference_logdet.txt` and `reference_logdet.csv`

## Tests and Benchmarking

The repository includes:

- `cholesky_tests`: baseline correctness checks.
- `cholesky_against_v1_tests`: regression against archived `v1`.
- `cholesky_stability_tests`: repeatability and block-size invariance checks.
- `cholesky_openmp_consistency_tests`: threaded consistency checks against archived `v2`.
- `reference_compare.py` + per-variant drivers: external correctness assessment using `numpy.linalg.slogdet`.
- `cholesky_benchmark`: single-size benchmark driver.
- `cholesky_benchmark_suite`: full benchmark suite producing result bundles.

Benchmark metadata records the build configuration and, when available, Slurm partition and node information. The release workflow also defaults to `OMP_PLACES=cores` and `OMP_PROC_BIND=close` unless you explicitly override them.

## License

This repository is currently distributed under the MIT License. See [LICENSE](LICENSE).
