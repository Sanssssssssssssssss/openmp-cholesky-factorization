# Version Archive

This directory stores archived source snapshots and benchmark outputs for major project versions.

## Layout

- `versions/<version>/` archived source for a tagged project version
- `results/<version>/` benchmark outputs for the same version

## Scope

Archive only milestone versions that are worth comparing in the report or revisiting during tuning. Small intermediate edits stay in normal Git history only.

## Version naming

Use simple, stable names that can also be used as Git tags. Recommended pattern:

- `cholesky_v0_baseline`
- `cholesky_v1_serial_opt`
- `cholesky_v2_serial_blocked`
- `cholesky_v3_openmp`
- `cholesky_v4_parallel_opt`

## Required contents

Each archived version should contain the source files that define the Cholesky implementation for that milestone.

Expected layout:

```text
history/versions/<version>/include/mphil_dis_cholesky.h
history/versions/<version>/src/cholesky.cpp
history/results/<version>/
```

## Official results policy

- Each tagged version should have one canonical results directory at `history/results/<version>/`.
- The canonical results directory should contain the formal `Release` benchmark outputs for that version.
- The canonical results directory is part of the tracked project history and should be committed into the repository.
- Canonical results should include correctness output, timing sweeps, structured CSV summaries, runtime metadata, and the first batch of `perf` hardware-counter outputs.
- Temporary exploratory runs such as `*_release`, `*_full`, or `*_suitecheck` may exist locally during development, but they are not the official comparison point once the canonical directory is updated.

## Archive procedure

1. Verify the current version builds and passes its intended checks in `Debug`.
2. Run the formal benchmark workflow for the current version in `Release`.
3. Save the formal benchmark outputs under `history/results/<version>/`.
4. Copy the milestone source files into `history/versions/<version>/`.
5. Create a Git tag with the same version name.

## Compiler flags study

- Keep one primary comparison configuration across versions so algorithmic changes are compared fairly.
- For this project, the primary formal comparison should use the `Release` build configuration.
- Additional studies comparing `-O1`, `-O2`, and `-O3` are useful, but they should be treated as a separate compiler-flags experiment, not mixed into the primary version-to-version comparison.
- For compiler-flags studies, keep the version fixed and vary only the optimization flags.

## Phase mapping

- `cholesky_v0_baseline`: baseline serial, correctness tests, timing, first stable reference point
- `cholesky_v1_serial_opt`: serial loop and arithmetic cleanup
- `cholesky_v2_serial_blocked`: cache-oriented serial optimization, including blocking if used
- `cholesky_v3_openmp`: first correct OpenMP parallel version
- `cholesky_v4_parallel_opt`: parallel tuning such as scheduling, affinity, and false-sharing work

## Build rule

Archived versions are for comparison and reference. They are available as dedicated CMake targets such as `<version>_tests`, `<version>_benchmark`, and `<version>_benchmark_suite`, and can be rerun without checking out older tags.
