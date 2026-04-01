#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

cd "${repo_root}"

repetitions="${1:-7}"
warmup="${2:-1}"

./scripts/run_release_all_versions.sh "${repetitions}" "${warmup}"
