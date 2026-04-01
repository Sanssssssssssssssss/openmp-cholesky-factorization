#!/usr/bin/env bash
#SBATCH --job-name=c2-all
#SBATCH --partition=icelake
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --exclusive
#SBATCH --time=04:00:00
#SBATCH --output=slurm-%x-%j.out
#SBATCH --error=slurm-%x-%j.err

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"

repetitions="${C2_REPETITIONS:-7}"
warmup="${C2_WARMUP:-1}"

echo "======================================================================"
echo "C2 CSD3 ALL-VERSION RUN"
echo "======================================================================"
echo "partition=${SLURM_JOB_PARTITION:-unset}"
echo "node=${SLURMD_NODENAME:-unset}"
echo "nodelist=${SLURM_NODELIST:-unset}"
echo "repetitions=${repetitions}"
echo "warmup=${warmup}"
echo "current_label_base=${C2_CURRENT_LABEL_BASE:-cholesky_v4_parallel_opt}"
echo "use_canonical_labels=${C2_USE_CANONICAL_LABELS:-1}"
echo "variants=${C2_VARIANTS:-default}"
echo "build_dir=${C2_BUILD_DIR:-build-release}"
echo "threads_override=${C2_THREADS:-default}"
echo "======================================================================"

cd "${repo_root}"

if [[ "${C2_CSD3_DRY_RUN:-0}" == "1" ]]; then
    echo "[DRY-RUN] ./bench/run_all_versions.sh ${repetitions} ${warmup}"
    exit 0
fi

./bench/run_all_versions.sh "${repetitions}" "${warmup}"
