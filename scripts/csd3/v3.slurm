#!/usr/bin/env bash
#SBATCH --job-name=c2-v3
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
export C2_VARIANT="cholesky_v3_openmp"
export C2_LABEL="${C2_LABEL:-cholesky_v3_openmp}"
exec "${script_dir}/run_version.slurm"
