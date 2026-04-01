# Benchmark Directory

This directory is reserved for the benchmark driver and related helper code used to evaluate Cholesky versions under a consistent workflow.

Current entrypoint:

```bash
./build/cholesky_benchmark <n> [repetitions] [warmup] [label]
```

Example:

```bash
./build/cholesky_benchmark 512 5 1 baseline-serial
```

The driver generates the coursework-style correlation matrix, runs the in-place factorization repeatedly, and prints both per-run timings and summary statistics.

Unified suite entrypoint:

```bash
./build/cholesky_benchmark_suite [label] [repetitions] [warmup]
```

This suite runs the correctness checks first, then writes coarse, fine, and large timing sweeps to `history/results/<label>/`.

One-line release wrappers from this directory:

```bash
./bench/run_version.sh cholesky_v2_serial_blocked cholesky_v2_serial_blocked 7 1
./bench/run_version.sh cholesky cholesky_v4_parallel_opt 7 1
./bench/run_all_versions.sh 7 1
```

These wrappers forward to the canonical release workflow, so they:

- configure and build `build-release/`
- run the appropriate tests
- write outputs into `history/results/<label>/`
- keep archived versions and the current version on the same protocol
- refresh canonical result directories by default when running all versions together
- use the standard cross-version size range, which now reaches `n=2304`

Generated outputs include:

- `correctness.txt`
- `against_v1.txt`
- `stability.txt`
- `omp_consistency.txt`
- `thread_scaling.txt`
- `thread_scaling.csv`
- `thread_scaling_extended.txt`
- `thread_scaling_extended.csv`
- `schedule_study.txt`
- `schedule_study.csv`
- `time_coarse.txt`
- `time_fine.txt`
- `time_large.txt`
- `sweeps.csv`
- `metadata.txt`
- `perf_basic.txt`
- `perf_basic.csv`
- `summary.txt`

When `perf` is available, the suite currently records:

- `cycles`
- `instructions`
- `cache-references`
- `cache-misses`
