#!/usr/bin/env bash
set -euo pipefail

print_rule() {
    printf '%s\n' "----------------------------------------------------------------------"
}

print_banner() {
    print_rule
    printf '%s\n' "$1"
    print_rule
}

print_subtitle() {
    printf '%s\n' "-- $1"
}

variant="${1:-cholesky}"
label="${2:-$variant}"
repetitions="${3:-7}"
warmup="${4:-1}"

build_dir="${C2_BUILD_DIR:-build-release}"
compiler="${C2_CXX_COMPILER:-g++}"
reference_sizes="${C2_REFERENCE_SIZES:-2,8,32,128,512,1024}"

if [[ -n "${C2_THREADS:-}" ]]; then
    threads="${C2_THREADS}"
elif [[ "${variant}" == "cholesky" || "${variant}" == "cholesky_v3_openmp" || "${variant}" == "cholesky_v4_parallel_opt" ]]; then
    threads="4"
else
    threads="1"
fi
block_size="${CHOLESKY_BLOCK_SIZE:-32}"
if [[ -n "${CHOLESKY_OMP_SCHEDULE:-}" ]]; then
    omp_schedule="${CHOLESKY_OMP_SCHEDULE}"
elif [[ "${variant}" == "cholesky" ]]; then
    omp_schedule="guided"
else
    omp_schedule="static"
fi

if [[ -n "${CHOLESKY_OMP_CHUNK:-}" ]]; then
    omp_chunk="${CHOLESKY_OMP_CHUNK}"
elif [[ "${variant}" == "cholesky" ]]; then
    omp_chunk="1"
else
    omp_chunk="0"
fi
schedule_study_n="${CHOLESKY_SCHEDULE_STUDY_N:-1024}"
schedule_study_threads="${CHOLESKY_SCHEDULE_STUDY_THREADS:-4}"
omp_places="${OMP_PLACES:-cores}"
omp_proc_bind="${OMP_PROC_BIND:-close}"

print_banner "RUNNING VERSION: ${label}"
print_subtitle "variant=${variant}"
print_subtitle "build_dir=${build_dir}"
print_subtitle "threads=${threads} block_size=${block_size} schedule=${omp_schedule} chunk=${omp_chunk}"
print_subtitle "omp_places=${omp_places} omp_proc_bind=${omp_proc_bind}"

print_banner "STAGE 1/4: CONFIGURE + BUILD"
cmake -S . -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="${compiler}"
cmake --build "${build_dir}" --target cholesky_all_variants

mkdir -p "history/results/${label}"

if [[ "${variant}" == "cholesky" ]]; then
    print_banner "STAGE 2/4: VALIDATION TESTS"
    "./${build_dir}/cholesky_tests"

    if [[ -x "./${build_dir}/cholesky_against_v1_tests" ]]; then
        print_subtitle "against_v1 regression"
        "./${build_dir}/cholesky_against_v1_tests" "history/results/${label}/against_v1.txt"
    fi

    if [[ -x "./${build_dir}/cholesky_stability_tests" ]]; then
        print_subtitle "stability regression"
        "./${build_dir}/cholesky_stability_tests" "history/results/${label}/stability.txt"
    fi

    if [[ -x "./${build_dir}/cholesky_openmp_consistency_tests" ]]; then
        print_subtitle "openmp consistency regression"
        "./${build_dir}/cholesky_openmp_consistency_tests" "history/results/${label}/omp_consistency.txt"
    fi

    if [[ -x "./${build_dir}/cholesky_reference_logdet_driver" ]]; then
        print_banner "STAGE 3/4: EXTERNAL REFERENCE CHECK"
        python3 ./test/reference_compare.py \
            --driver "./${build_dir}/cholesky_reference_logdet_driver" \
            --output "history/results/${label}/reference_logdet.txt" \
            --csv "history/results/${label}/reference_logdet.csv" \
            --sizes "${reference_sizes}"
    fi

    print_banner "STAGE 4/4: BENCHMARK SUITE"
    OMP_NUM_THREADS="${threads}" \
    OMP_PLACES="${omp_places}" \
    OMP_PROC_BIND="${omp_proc_bind}" \
    CHOLESKY_BLOCK_SIZE="${block_size}" \
    CHOLESKY_OMP_SCHEDULE="${omp_schedule}" \
    CHOLESKY_OMP_CHUNK="${omp_chunk}" \
    CHOLESKY_SCHEDULE_STUDY_N="${schedule_study_n}" \
    CHOLESKY_SCHEDULE_STUDY_THREADS="${schedule_study_threads}" \
    "./${build_dir}/cholesky_benchmark_suite" "${label}" "${repetitions}" "${warmup}"
else
    print_banner "STAGE 2/4: VARIANT TESTS"
    "./${build_dir}/${variant}_tests"

    if [[ -x "./${build_dir}/${variant}_reference_logdet_driver" ]]; then
        print_banner "STAGE 3/4: EXTERNAL REFERENCE CHECK"
        python3 ./test/reference_compare.py \
            --driver "./${build_dir}/${variant}_reference_logdet_driver" \
            --output "history/results/${label}/reference_logdet.txt" \
            --csv "history/results/${label}/reference_logdet.csv" \
            --sizes "${reference_sizes}"
    fi

    print_banner "STAGE 4/4: BENCHMARK SUITE"
    OMP_NUM_THREADS="${threads}" \
    OMP_PLACES="${omp_places}" \
    OMP_PROC_BIND="${omp_proc_bind}" \
    CHOLESKY_BLOCK_SIZE="${block_size}" \
    CHOLESKY_OMP_SCHEDULE="${omp_schedule}" \
    CHOLESKY_OMP_CHUNK="${omp_chunk}" \
    CHOLESKY_SCHEDULE_STUDY_N="${schedule_study_n}" \
    CHOLESKY_SCHEDULE_STUDY_THREADS="${schedule_study_threads}" \
    "./${build_dir}/${variant}_benchmark_suite" "${label}" "${repetitions}" "${warmup}"
fi

print_banner "STAGE COMPLETE: ${label}"
python3 ./scripts/print_result_summary.py --label "${label}"
