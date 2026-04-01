#!/usr/bin/env bash
set -euo pipefail
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${script_dir}/run_version.sh" --variant cholesky --label cholesky_v4_parallel_opt "$@"
