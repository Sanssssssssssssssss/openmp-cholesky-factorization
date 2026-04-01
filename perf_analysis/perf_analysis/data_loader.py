import csv
import io
import os
import re
from .paths import RESULTS_DIR


NUMERIC_RE = re.compile(r"^-?\d+(?:\.\d+)?(?:e[+-]?\d+)?$", re.IGNORECASE)


REPEATABILITY_RE = re.compile(
    r"repeatability block_size=(?P<block_size>\d+) "
    r"n=(?P<n>\d+) "
    r"run1_seconds=(?P<run1>[0-9.eE+-]+) "
    r"run2_seconds=(?P<run2>[0-9.eE+-]+) "
    r"run3_seconds=(?P<run3>[0-9.eE+-]+) "
    r"factor_max_abs_diff_run1_run2=(?P<diff12>[0-9.eE+-]+) "
    r"factor_max_abs_diff_run1_run3=(?P<diff13>[0-9.eE+-]+)"
)


BLOCK_INVARIANCE_RE = re.compile(
    r"block_invariance n=(?P<n>\d+) "
    r"reference_block_size=(?P<reference_block_size>\d+) "
    r"compare_block_size=(?P<compare_block_size>\d+) "
    r"reference_seconds=(?P<reference_seconds>[0-9.eE+-]+) "
    r"candidate_seconds=(?P<candidate_seconds>[0-9.eE+-]+) "
    r"factor_max_abs_diff=(?P<factor_max_abs_diff>[0-9.eE+-]+) "
    r"rebuild_max_abs_error=(?P<rebuild_max_abs_error>[0-9.eE+-]+)"
)


THREAD_HEADER_RE = re.compile(r"===== threads=(\d+)")
RUN_SECONDS_RE = re.compile(r"run_(\d+)_seconds=([0-9.eE+-]+)")


def _coerce(value):
    if value is None:
        return value
    stripped = value.strip()
    if stripped == "":
        return ""
    if NUMERIC_RE.match(stripped):
        if any(ch in stripped for ch in [".", "e", "E"]):
            return float(stripped)
        return int(stripped)
    return stripped


def _read_csv_text(text):
    reader = csv.DictReader(io.StringIO(text))
    rows = []
    for row in reader:
        rows.append({key: _coerce(value) for key, value in row.items()})
    return rows


def read_csv(path):
    with open(path, "r") as handle:
        return _read_csv_text(handle.read())


def load_csv(version, filename):
    return read_csv(os.path.join(RESULTS_DIR, version, filename))


def load_text(version, filename):
    path = os.path.join(RESULTS_DIR, version, filename)
    with open(path, "r") as handle:
        return handle.read()


def parse_stability(version):
    repeatability = []
    block_invariance = []

    for line in load_text(version, "stability.txt").splitlines():
        repeat_match = REPEATABILITY_RE.search(line)
        if repeat_match:
            item = {key: _coerce(value) for key, value in repeat_match.groupdict().items()}
            repeatability.append(item)
            continue

        invariance_match = BLOCK_INVARIANCE_RE.search(line)
        if invariance_match:
            item = {key: _coerce(value) for key, value in invariance_match.groupdict().items()}
            block_invariance.append(item)

    return {
        "repeatability": repeatability,
        "block_invariance": block_invariance,
    }


def parse_thread_scaling_runs(version):
    runs = []
    current_threads = None
    current_run_values = []

    for line in load_text(version, "thread_scaling.txt").splitlines():
        thread_match = THREAD_HEADER_RE.search(line)
        if thread_match:
            if current_threads is not None and current_run_values:
                for run_idx, seconds in current_run_values:
                    runs.append({
                        "threads": current_threads,
                        "run": run_idx,
                        "seconds": seconds,
                    })
            current_threads = int(thread_match.group(1))
            current_run_values = []
            continue

        run_match = RUN_SECONDS_RE.search(line)
        if run_match and current_threads is not None:
            current_run_values.append((int(run_match.group(1)), float(run_match.group(2))))

    if current_threads is not None and current_run_values:
        for run_idx, seconds in current_run_values:
            runs.append({
                "threads": current_threads,
                "run": run_idx,
                "seconds": seconds,
            })

    return runs
