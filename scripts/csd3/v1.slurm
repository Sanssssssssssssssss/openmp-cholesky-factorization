#!/usr/bin/env bash
#SBATCH --job-name=c2-v1
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
label="${C2_LABEL:-cholesky_v1_serial_opt}"
repetitions="${C2_REPETITIONS:-7}"
warmup="${C2_WARMUP:-1}"
compiler="${C2_CXX_COMPILER:-g++}"
threads="${C2_THREADS:-1}"
block_size="${CHOLESKY_BLOCK_SIZE:-32}"

cd "${repo_root}"

echo "======================================================================"
echo "C2 CSD3 MILESTONE RUN"
echo "======================================================================"
echo "tag=cholesky_v1_serial_opt"
echo "partition=${SLURM_JOB_PARTITION:-unset}"
echo "node=${SLURMD_NODENAME:-unset}"
echo "nodelist=${SLURM_NODELIST:-unset}"
echo "label=${label}"
echo "repetitions=${repetitions}"
echo "warmup=${warmup}"
echo "threads=${threads}"
echo "build_dir=build-release"
echo "======================================================================"

if [[ "${C2_CSD3_DRY_RUN:-0}" == "1" ]]; then
    echo "[DRY-RUN] cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=${compiler}"
    echo "[DRY-RUN] cmake --build build-release --target cholesky_tests cholesky_benchmark_suite"
    echo "[DRY-RUN] ./build-release/cholesky_tests"
    echo "[DRY-RUN] OMP_NUM_THREADS=${threads} OMP_PLACES=cores OMP_PROC_BIND=close CHOLESKY_BLOCK_SIZE=${block_size} CHOLESKY_OMP_SCHEDULE=static CHOLESKY_OMP_CHUNK=0 ./build-release/cholesky_benchmark_suite ${label} ${repetitions} ${warmup}"
    exit 0
fi

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="${compiler}"
cmake --build build-release --target cholesky_tests cholesky_benchmark_suite
./build-release/cholesky_tests
OMP_NUM_THREADS="${threads}" OMP_PLACES=cores OMP_PROC_BIND=close CHOLESKY_BLOCK_SIZE="${block_size}" CHOLESKY_OMP_SCHEDULE="static" CHOLESKY_OMP_CHUNK="0" ./build-release/cholesky_benchmark_suite "${label}" "${repetitions}" "${warmup}"
