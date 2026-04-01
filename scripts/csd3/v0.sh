#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
slurm_script="${script_dir}/v0.slurm"

account="${CSD3_ACCOUNT:-}"
partition="${CSD3_PARTITION:-icelake}"
cpus="${CSD3_CPUS:-32}"
time_limit="${CSD3_TIME:-02:00:00}"
repetitions="${C2_REPETITIONS:-7}"
warmup="${C2_WARMUP:-1}"
dry_run=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --account) account="$2"; shift 2 ;;
        --partition) partition="$2"; shift 2 ;;
        --cpus) cpus="$2"; shift 2 ;;
        --time) time_limit="$2"; shift 2 ;;
        --repetitions) repetitions="$2"; shift 2 ;;
        --warmup) warmup="$2"; shift 2 ;;
        --dry-run) dry_run=1; shift ;;
        *) echo "unknown argument: $1" >&2; exit 1 ;;
    esac
done

if [[ -z "${account}" ]]; then
    echo "error: set CSD3_ACCOUNT or pass --account <account>" >&2
    exit 1
fi

cmd=(
    sbatch
    --account "${account}"
    --partition "${partition}"
    --nodes 1
    --ntasks 1
    --cpus-per-task "${cpus}"
    --exclusive
    --time "${time_limit}"
    --job-name "c2-cholesky_v0_baseline"
    --export "ALL,C2_LABEL=cholesky_v0_baseline,C2_REPETITIONS=${repetitions},C2_WARMUP=${warmup}"
    "${slurm_script}"
)

echo "Submitting cholesky_v0_baseline CSD3 job:"
printf '  %q' "${cmd[@]}"
printf '\n'

if [[ "${dry_run}" == "1" ]]; then
    exit 0
fi

"${cmd[@]}"
