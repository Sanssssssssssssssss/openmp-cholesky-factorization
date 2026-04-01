#!/usr/bin/env bash
set -euo pipefail

variant="${1:-cholesky}"
label="${2:-$variant}"
repetitions="${3:-7}"
warmup="${4:-1}"

build_dir="${C2_BUILD_DIR:-build-release}"
compiler="${C2_CXX_COMPILER:-g++}"
threads="${C2_THREADS:-${OMP_NUM_THREADS:-1}}"
block_size="${CHOLESKY_BLOCK_SIZE:-32}"

cmake -S . -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="${compiler}"
cmake --build "${build_dir}" --target cholesky_all_variants

mkdir -p "history/results/${label}"

if [[ "${variant}" == "cholesky" ]]; then
    "./${build_dir}/cholesky_tests"

    if [[ -x "./${build_dir}/cholesky_against_v1_tests" ]]; then
        "./${build_dir}/cholesky_against_v1_tests" "history/results/${label}/against_v1.txt"
    fi

    if [[ -x "./${build_dir}/cholesky_stability_tests" ]]; then
        "./${build_dir}/cholesky_stability_tests" "history/results/${label}/stability.txt"
    fi

    if [[ -x "./${build_dir}/cholesky_openmp_consistency_tests" ]]; then
        "./${build_dir}/cholesky_openmp_consistency_tests" "history/results/${label}/omp_consistency.txt"
    fi

    OMP_NUM_THREADS="${threads}" \
    CHOLESKY_BLOCK_SIZE="${block_size}" \
    "./${build_dir}/cholesky_benchmark_suite" "${label}" "${repetitions}" "${warmup}"
else
    "./${build_dir}/${variant}_tests"

    OMP_NUM_THREADS="${threads}" \
    CHOLESKY_BLOCK_SIZE="${block_size}" \
    "./${build_dir}/${variant}_benchmark_suite" "${label}" "${repetitions}" "${warmup}"
fi
