#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

cd "${repo_root}"

variant="${1:-cholesky}"
label="${2:-$variant}"
repetitions="${3:-7}"
warmup="${4:-1}"

./scripts/run_release_suite.sh "${variant}" "${label}" "${repetitions}" "${warmup}"
