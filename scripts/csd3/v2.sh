#!/usr/bin/env bash
set -euo pipefail
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${script_dir}/run_version.sh" --variant cholesky_v2_serial_blocked --label cholesky_v2_serial_blocked "$@"
