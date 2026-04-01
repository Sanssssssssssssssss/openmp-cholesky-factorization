#!/usr/bin/env bash
#SBATCH --job-name=c2-example
#SBATCH --partition=icelake
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --exclusive
#SBATCH --time=00:30:00
#SBATCH --output=slurm-%x-%j.out
#SBATCH --error=slurm-%x-%j.err

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
example_n="${C2_EXAMPLE_N:-4}"

echo "======================================================================"
echo "C2 CSD3 EXAMPLE RUN"
echo "======================================================================"
echo "partition=${SLURM_JOB_PARTITION:-unset}"
echo "node=${SLURMD_NODENAME:-unset}"
echo "nodelist=${SLURM_NODELIST:-unset}"
echo "build_dir=${C2_BUILD_DIR:-build-release}"
echo "example_n=${example_n}"
echo "======================================================================"

cd "${repo_root}"

if [[ "${C2_CSD3_DRY_RUN:-0}" == "1" ]]; then
    echo "[DRY-RUN] cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++"
    echo "[DRY-RUN] cmake --build build-release --target cholesky_example"
    echo "[DRY-RUN] ./build-release/cholesky_example ${example_n}"
    exit 0
fi

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build-release --target cholesky_example
./build-release/cholesky_example "${example_n}"
