#!/usr/bin/env bash
set -euo pipefail

print_rule() {
    printf '%s\n' "======================================================================"
}

repetitions="${1:-7}"
warmup="${2:-1}"

use_canonical_labels="${C2_USE_CANONICAL_LABELS:-1}"
label_suffix="${C2_LABEL_SUFFIX:-_rerun}"
current_label_base="${C2_CURRENT_LABEL_BASE:-cholesky_v4_parallel_opt}"
variants="${C2_VARIANTS:-cholesky_v0_baseline cholesky_v1_serial_opt cholesky_v2_serial_blocked cholesky_v3_openmp cholesky}"
labels=""

print_rule
printf '%s\n' "STARTING ALL-VERSION RELEASE RUN"
print_rule
printf '%s\n' "repetitions=${repetitions} warmup=${warmup}"
printf '%s\n' "variants=${variants}"
print_rule

for variant in ${variants}; do
    if [[ "${use_canonical_labels}" == "1" ]]; then
        if [[ "${variant}" == "cholesky" ]]; then
            label="${current_label_base}"
        else
            label="${variant}"
        fi
    else
        if [[ "${variant}" == "cholesky" ]]; then
            label="${current_label_base}${label_suffix}"
        else
            label="${variant}${label_suffix}"
        fi
    fi

    echo "[INFO] Running variant=${variant} label=${label}"
    ./scripts/run_release_suite.sh "${variant}" "${label}" "${repetitions}" "${warmup}"
    labels="${labels} ${label}"
done

print_rule
printf '%s\n' "FINAL ALL-VERSION SUMMARY"
print_rule
python3 ./scripts/print_result_summary.py --compare ${labels}
