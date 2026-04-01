# Performance Plotting Project

This directory is a standalone local plotting project for the C2 coursework benchmark history.

It does not build or modify the Cholesky library itself. It only reads committed benchmark outputs from:

- `history/results/cholesky_v0_baseline/`
- `history/results/cholesky_v1_serial_opt/`
- `history/results/cholesky_v2_serial_blocked/`
- `history/results/cholesky_v3_openmp/`
- `history/results/cholesky_v4_parallel_opt/`

## What it generates

Running the project creates a large figure bundle under `perf_analysis/output/figures/`, a Markdown index at `perf_analysis/output/figure_index.md`, and derived table outputs under `perf_analysis/output/tables/`.

The bundle includes:

- overall runtime over matrix size
- speedup against the baseline
- pairwise runtime and GFLOP/s plots for each optimization step
- thread scaling and parallel efficiency plots
- extended thread-scaling comparison plots for `n=256`, `1024`, and `2048`
- OpenMP schedule comparison plots
- boundary-size plots around cache- and block-sensitive sizes
- stability and block-size sensitivity plots
- hardware-counter support plots such as IPC and cache-miss rate
- appendix-ready extended thread-scaling tables with speedup, efficiency, and Karp-Flatt serial fraction

For every cross-version size-based comparison, the plotting code now produces both:

- an aligned-granularity view: one chosen measurement per matrix size `n`
- an unaligned-granularity view: raw benchmark points without splitting the visual by coarse/fine/large

This keeps the figures representative without hiding the original benchmark density.

The plotting project intentionally excludes the mistaken intermediate `old v4` branch of analysis. All final-version figures refer only to the accepted implementation and are labeled as `v4 Final Version`.

## Requirements

The scripts are written for the environment already available in this repo:

- Python `3.6+`
- `matplotlib`
- `numpy`

No `pandas` dependency is required.

## Run

From the repository root:

```bash
python3 perf_analysis/run_all.py
```

The output directory will be refreshed in place.

## Notes

- The plotting code assumes the canonical benchmark files remain in their current format.
- The figures intentionally separate "canonical cross-version comparison" from more focused pairwise and diagnostic plots so the report can stay selective while the analysis remains comprehensive.
