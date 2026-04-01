#!/usr/bin/env bash
#SBATCH --job-name=c2-one
#SBATCH --partition=icelake
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --output=slurm-%x-%j.out
#SBATCH --error=slurm-%x-%j.err

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"

variant="${C2_VARIANT:-}"
label="${C2_LABEL:-}"
repetitions="${C2_REPETITIONS:-7}"
warmup="${C2_WARMUP:-1}"

if [[ -z "${variant}" ]]; then
    echo "error: C2_VARIANT is not set" >&2
    exit 1
fi

if [[ -z "${label}" ]]; then
    label="${variant}"
fi

echo "======================================================================"
echo "C2 CSD3 VERSION RUN"
echo "======================================================================"
echo "partition=${SLURM_JOB_PARTITION:-unset}"
echo "node=${SLURMD_NODENAME:-unset}"
echo "nodelist=${SLURM_NODELIST:-unset}"
echo "variant=${variant}"
echo "label=${label}"
echo "repetitions=${repetitions}"
echo "warmup=${warmup}"
echo "build_dir=${C2_BUILD_DIR:-build-release}"
echo "threads=${C2_THREADS:-default}"
echo "omp_schedule=${CHOLESKY_OMP_SCHEDULE:-default}"
echo "omp_chunk=${CHOLESKY_OMP_CHUNK:-default}"
echo "======================================================================"

cd "${repo_root}"

if [[ "${C2_CSD3_DRY_RUN:-0}" == "1" ]]; then
    echo "[DRY-RUN] ./bench/run_version.sh ${variant} ${label} ${repetitions} ${warmup}"
    exit 0
fi

./bench/run_version.sh "${variant}" "${label}" "${repetitions}" "${warmup}"
