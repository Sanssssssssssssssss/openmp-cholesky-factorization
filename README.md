# OpenMP Cholesky Factorization

This repository contains a CPU-only Cholesky factorization implementation for dense symmetric positive-definite matrices. The code is written in C++, operates in place on a row-major `double*` matrix, and is developed through tagged milestone versions from a serial baseline to an OpenMP-optimized implementation. The repository also includes tests, reproducible benchmark runners, archived milestone snapshots, and CSD3 submission scripts.

The public C++ interface is the header:

```cpp
#include "mphil_dis_cholesky.h"
```

## Quick Start

Build everything needed for the current code and all archived milestone versions:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build-release --target cholesky_all_variants
```

Validate or submit a single milestone from the login node with the convenience shell wrappers:

```bash
CSD3_ACCOUNT=<account> ./scripts/csd3/v0.sh
CSD3_ACCOUNT=<account> ./scripts/csd3/v1.sh
CSD3_ACCOUNT=<account> ./scripts/csd3/v2.sh
CSD3_ACCOUNT=<account> ./scripts/csd3/v3.sh
CSD3_ACCOUNT=<account> ./scripts/csd3/v4.sh
```

The same shell wrappers also support local/login-node validation before submission:

```bash
CSD3_ACCOUNT=<account> ./scripts/csd3/v0.sh --dry-run
CSD3_ACCOUNT=<account> ./scripts/csd3/v4.sh --dry-run
```

Submit the example program on CSD3:

```bash
CSD3_ACCOUNT=<account> ./scripts/csd3/example.sh
```

Submit all milestone versions plus the current final version on CSD3:

```bash
CSD3_ACCOUNT=<account> ./scripts/csd3/run_all_versions.sh
```

Direct `sbatch` entrypoints are also provided for each milestone and the all-version workflow:

```bash
sbatch -A <account> scripts/csd3/v0.slurm
sbatch -A <account> scripts/csd3/v1.slurm
sbatch -A <account> scripts/csd3/v2.slurm
sbatch -A <account> scripts/csd3/v3.slurm
sbatch -A <account> scripts/csd3/v4.slurm
sbatch -A <account> scripts/csd3/example.slurm
sbatch -A <account> scripts/csd3/run_all_versions.slurm
```

The CSD3 wrappers submit to `icelake` by default, run on one exclusive node, and map directly onto the repository’s existing release workflow in `scripts/run_release_suite.sh` and `scripts/run_release_all_versions.sh`.

All tracked milestone source snapshots and benchmark bundles live under `history/`, so a marker can inspect both the archived code and the recorded results without checking out extra branches.

## Version Milestones / Tag Mapping

| Milestone | Tag | Description |
|---|---|---|
| v0 | `cholesky_v0_baseline` | First correct serial baseline. |
| v1 | `cholesky_v1_serial_opt` | Serial loop and access-order improvements. |
| v2 | `cholesky_v2_serial_blocked` | Blocked serial implementation with cache-oriented restructuring. |
| v3 | `cholesky_v3_openmp` | First correct OpenMP implementation. |
| v4 | `cholesky_v4_parallel_opt` | Final tuned parallel implementation. |

## Project Structure

- `include/`: public header for the Cholesky library interface.
- `src/`: current implementation of the Cholesky kernel.
- `example/`: minimal example program using the public API.
- `test/`: correctness, regression, stability, OpenMP consistency, and external-reference validation tools.
- `bench/`: benchmark drivers plus one-line wrappers for single-version and all-version reruns.
- `perf_analysis/`: Python-based post-processing scripts, generated figures, and analysis tables for the report.
- `scripts/`: release wrappers, CSD3 submission scripts, and reporting helpers.
- `scripts/csd3/`: submission-ready `.slurm` and `.sh` entrypoints for CSD3.
- `history/versions/`: archived source snapshots for milestone versions.
- `history/results/`: tracked benchmark/result bundles for milestone versions.
- `report/`: location for the final performance report, expected as `report/report.pdf`.
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

- `.sh` wrappers: convenient login-node helpers that can be used either as a local validation path with `--dry-run` or as a submission helper that calls `sbatch` for you.
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

For a marker or user who wants to sanity-check the submission path without queueing a job immediately, the intended order is:

```bash
CSD3_ACCOUNT=<account> ./scripts/csd3/v2.sh --dry-run
sbatch -A <account> scripts/csd3/v2.slurm
```

Available CSD3 convenience wrappers:

- `scripts/csd3/v0.sh`, `scripts/csd3/v0.slurm`: run the `v0` baseline milestone.
- `scripts/csd3/v1.sh`, `scripts/csd3/v1.slurm`: run the `v1` serial-optimized milestone.
- `scripts/csd3/v2.sh`, `scripts/csd3/v2.slurm`: run the `v2` blocked-serial milestone.
- `scripts/csd3/v3.sh`, `scripts/csd3/v3.slurm`: run the `v3` first-OpenMP milestone.
- `scripts/csd3/v4.sh`, `scripts/csd3/v4.slurm`: run the `v4` tuned parallel milestone.
- `scripts/csd3/example.sh`, `scripts/csd3/example.slurm`: build and run `cholesky_example` only.
- `scripts/csd3/run_version.sh`, `scripts/csd3/run_version.slurm`: generic single-version entrypoints.
- `scripts/csd3/run_all_versions.sh`, `scripts/csd3/run_all_versions.slurm`: rerun all milestone versions plus the final submission state.

For CSD3 submission wrappers, the current final version uses:

- variant: `cholesky`
- label: `cholesky_v4_parallel_opt`

Archived milestones are rerun from `history/versions/<tag>/` through dedicated CMake targets in the current repository, so markers do not need to check out old tags manually.

Recommended CSD3 flow:

1. Log in to CSD3 and clone the repository.
2. Use a convenience `.sh` wrapper on the login node when you want the repository to prepare the matching `sbatch` command for you.
3. Use the matching `.slurm` file directly when you want an explicit batch script for a milestone or for the example program.
4. Use `scripts/csd3/run_all_versions.sh` or `scripts/csd3/run_all_versions.slurm` when you want the complete milestone rerun workflow.
5. Inspect outputs in `history/results/<version>/` after the job completes.

If you want to inspect the code differences between two milestone tags directly, use `git diff`. For example, the serial blocking change introduced in `v2` can be viewed with:

```bash
git diff cholesky_v1_serial_opt cholesky_v2_serial_blocked -- src/cholesky.cpp
```

## Results and Report Locations

- Canonical benchmark bundles live in `history/results/<version>/`.
- Milestone source snapshots live in `history/versions/<version>/`.
- Python analysis scripts and generated performance figures live in `perf_analysis/`.
- The final report should be placed at `report/report.pdf` before submission.
- In other words, the archived deliverables are intentionally concentrated under `history/` for code/results and under `report/` for the written report.
- The `main` branch should be treated as the final submission branch.

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
