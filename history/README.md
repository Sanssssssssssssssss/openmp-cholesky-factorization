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

## Archive procedure

1. Verify the current version builds and passes its intended checks.
2. Run the benchmark workflow for the current version.
3. Save benchmark outputs under `history/results/<version>/`.
4. Copy the milestone source files into `history/versions/<version>/`.
5. Create a Git tag with the same version name.

## Phase mapping

- `cholesky_v0_baseline`: baseline serial, correctness tests, timing, first stable reference point
- `cholesky_v1_serial_opt`: serial loop and arithmetic cleanup
- `cholesky_v2_serial_blocked`: cache-oriented serial optimization, including blocking if used
- `cholesky_v3_openmp`: first correct OpenMP parallel version
- `cholesky_v4_parallel_opt`: parallel tuning such as scheduling, affinity, and false-sharing work

## Build rule

Archived versions are for comparison and reference. They must not be added to the default build unless explicitly selected for a dedicated comparison run.
