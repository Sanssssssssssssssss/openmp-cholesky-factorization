# Figure Plan

This plan is intentionally broader than what should go into the final report. The goal is to create a complete local analysis bundle, then select the best subset for the report.

All size-based comparison plots should be read in two ways:

- aligned granularity: one chosen value per matrix size `n`, useful for fair like-for-like comparison
- unaligned granularity: raw benchmark points without splitting the visual by coarse/fine/large

## Figure Families

### A. Cross-Version Overview

1. `01_overview_runtime_vs_n.png`
   - Question: How does wall-clock runtime evolve across all versions?
   - Source: `sweeps.csv`
   - Message: The whole optimization history at a glance.

2. `03_overview_speedup_vs_v0.png`
   - Question: How much faster is each version than the baseline?
   - Source: `sweeps.csv`
   - Message: Quantifies cumulative benefit.

3. `04_representative_runtime_bars.png`
   - Question: What do the milestone runtimes look like at representative matrix sizes?
   - Source: `sweeps.csv`
   - Message: Good report-ready summary for `n=1000`, `1024`, and `2048`.

4. `05_representative_gflops_bars.png`
   - Question: What do the milestone throughputs look like at representative sizes?
   - Source: `sweeps.csv`
   - Message: Compact summary to pair with runtime bars.

### B. Step-by-Step Optimization Transitions

5. `10_pair_v0_to_v1_runtime.png` and `11_pair_v0_to_v1_gflops.png`
6. `12_pair_v1_to_v2_runtime.png` and `13_pair_v1_to_v2_gflops.png`
7. `14_pair_v2_to_v3_runtime.png` and `15_pair_v2_to_v3_gflops.png`
8. `16_pair_v3_to_final_runtime.png` and `17_pair_v3_to_final_gflops.png`
9. `18_pair_v2_to_final_runtime.png` and `19_pair_v2_to_final_gflops.png`
   - Question: What changed at each step and how large was the gain in runtime and throughput?
   - Source: `sweeps.csv`
   - Message: Each transition is now split into a runtime plot and a GFLOP/s plot, with aligned and raw-point views side by side. The additional `v2` to `v4 Final Version` pair isolates the serial-blocked versus OpenMP-tuned contrast directly.

### C. Parallel Behavior

10. `20_parallel_runtime_v3_v4.png`
   - Question: How do the OpenMP versions compare over matrix size?
   - Source: `sweeps.csv`
   - Message: Shows the parallel-era improvement directly.

11. `21_thread_scaling_runtime.png`
   - Question: How does runtime change with threads for `v3` and the `v4 Final Version`?
   - Source: `thread_scaling.csv`
   - Message: The practical scaling picture.

12. `22_thread_scaling_speedup_efficiency.png`
   - Question: What are the speedup and efficiency trends?
   - Source: `thread_scaling.csv`
   - Message: Highlights diminishing returns and pathological oversubscription.

13. `23_v4_thread_scaling_distribution.png`
   - Question: How noisy are repeated runs at each thread count?
   - Source: `thread_scaling.txt`
   - Message: Helps explain unstable large-thread behavior.

### D. OpenMP Schedule Tuning

14. `30_v4_schedule_study.png`
   - Question: Which schedule/chunk combination is best for the v4 Final Version?
   - Source: `schedule_study.csv`
   - Message: Supports the default `guided,1`.

15. `31_v3_v4_schedule_comparison.png`
   - Question: How does schedule sensitivity differ between `v3` and the v4 Final Version?
   - Source: `schedule_study.csv`
   - Message: Compares sensitivity and balance between the two OpenMP generations without including the mistaken older `v4`.

### E. Boundary and Blocking Effects

16. `40_boundary_sizes_v2_v3_v4.png`
   - Question: What happens around `127/128/129`, `255/256/257`, `511/512/513`, `1023/1024/1025`?
   - Source: `sweeps.csv`
   - Message: Shows cache/block sensitivity and non-smooth size effects.

17. `41_v4_block_repeatability.png`
   - Question: How does runtime vary with block size in the repeatability study?
   - Source: `stability.txt`
   - Message: Identifies the local sweet spot and robustness of block size choices.

18. `42_v4_block_invariance.png`
   - Question: Do different block sizes preserve the same answer, and how much runtime changes?
   - Source: `stability.txt`
   - Message: Correctness stays fixed while performance shifts.

### F. Hardware-Counter Support

19. `50_ipc_vs_n.png`
   - Question: How did instruction throughput evolve across versions?
   - Source: `perf_basic.csv`
   - Message: Strong support for the `v0 -> v1` kernel cleanup story.

20. `51_cache_miss_pct_vs_n.png`
   - Question: How does cache behavior change with size and version?
   - Source: `perf_basic.csv`
   - Message: Supporting evidence for locality claims, used carefully.

21. `52_cycles_vs_n.png`
   - Question: How do raw cycle counts scale across versions?
   - Source: `perf_basic.csv`
   - Message: Lower-level support for the wall-clock story.

22. `53_v0_v1_ipc_vs_n.png`
   - Question: How much does IPC improve in the first serial kernel rewrite?
   - Source: `perf_basic.csv`
   - Message: A focused support figure for the `v0 \rightarrow v1` discussion in the main text.

23. `60_extended_thread_scaling_n256.png`
24. `61_extended_thread_scaling_n1024.png`
25. `62_extended_thread_scaling_n2048.png`
   - Question: How do `v3` and the `v4 Final Version` compare across the extended thread sweep at representative small, medium, and large problem sizes?
   - Source: `thread_scaling_extended.csv`
   - Message: Each figure pairs runtime and speedup, making it easier to discuss where the final OpenMP kernel scales better and where saturation appears.

26. `63_v4_speedup_vs_threads_multi_n.png`
   - Question: How does the final implementation scale with thread count across representative problem sizes?
   - Source: `thread_scaling_extended.csv`
   - Message: Directly answers the OpenMP scaling question for `v4 Final Version` using `n=256`, `1024`, and `2048` on one plot.

## Derived Tables

- `output/tables/cholesky_v3_openmp_thread_scaling_extended_metrics.csv`
- `output/tables/cholesky_v4_parallel_opt_thread_scaling_extended_metrics.csv`
- `output/tables/thread_scaling_extended_appendix_tables.tex`
  - Question: What are the derived scaling metrics behind the extended OpenMP study?
  - Source: `thread_scaling_extended.csv`
  - Message: Provides appendix-ready tables for runtime, GFLOP/s, speedup, efficiency, and Karp-Flatt serial fraction for `v3` and `v4 Final Version`.

## Recommended Report Subset

The final report probably does not need all figures. A likely strong subset is:

- `01_overview_runtime_vs_n.png`
- `03_overview_speedup_vs_v0.png`
- `10_pair_v0_to_v1_runtime.png`
- `12_pair_v1_to_v2_runtime.png`
- `14_pair_v2_to_v3_runtime.png`
- `16_pair_v3_to_final_runtime.png`
- `18_pair_v2_to_final_runtime.png`
- `21_thread_scaling_runtime.png`
- `30_v4_schedule_study.png`
- `40_boundary_sizes_v2_v3_v4.png`
- `50_ipc_vs_n.png`

Everything else is there to help with analysis, pruning, and appendix material.
