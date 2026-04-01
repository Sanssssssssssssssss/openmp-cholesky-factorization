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
