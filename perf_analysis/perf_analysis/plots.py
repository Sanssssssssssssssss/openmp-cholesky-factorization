import os

import numpy as np
from matplotlib import pyplot as plt

from .data_loader import (
    load_csv,
    parse_stability,
    parse_thread_scaling_runs,
)
from .paths import FIGURES_DIR, VERSION_COLORS, VERSION_LABELS, VERSIONS
from .style import finalize_figure


SWEEP_PRIORITY = {
    "coarse": 1,
    "large": 2,
    "fine": 3,
}


ALIGNED_SIZES = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048]


def _title(ax, title, subtitle=None):
    if subtitle:
        ax.set_title("%s\n%s" % (title, subtitle), loc="left")
    else:
        ax.set_title(title, loc="left")


def _panel_title(ax, panel_index, title, subtitle=None):
    prefix = "(%s) " % chr(ord("a") + panel_index)
    _title(ax, prefix + title, subtitle)


def _line(ax, rows, x_key, y_key, color, label, marker="o", linestyle="-", alpha=1.0):
    xs = [row[x_key] for row in rows]
    ys = [row[y_key] for row in rows]
    ax.plot(xs, ys, marker=marker, linewidth=2.2, markersize=4.5, color=color, label=label, linestyle=linestyle, alpha=alpha)


def _scatter(ax, rows, x_key, y_key, color, label, alpha=0.7, s=28):
    xs = [row[x_key] for row in rows]
    ys = [row[y_key] for row in rows]
    ax.scatter(xs, ys, s=s, color=color, label=label, alpha=alpha)


def _group_rows(rows, key):
    groups = {}
    for row in rows:
        groups.setdefault(row[key], []).append(row)
    return groups


def _combined_sweeps(rows, allowed_sweeps):
    return [row for row in rows if row["sweep"] in allowed_sweeps]


def _merged_size_rows(rows, deduplicate):
    merged = [row for row in rows if row["sweep"] in ["coarse", "fine", "large"]]
    if not deduplicate:
        return sorted(merged, key=lambda item: (item["n"], item["sweep"]))

    best_by_n = {}
    for row in merged:
        current = best_by_n.get(row["n"])
        if current is None or SWEEP_PRIORITY[row["sweep"]] > SWEEP_PRIORITY[current["sweep"]]:
            best_by_n[row["n"]] = dict(row)
    ordered = [best_by_n[key] for key in sorted(best_by_n)]
    return ordered


def _aligned_rows(rows):
    merged = _merged_size_rows(rows, deduplicate=True)
    return [row for row in merged if row["n"] in ALIGNED_SIZES]


def _unaligned_rows(rows):
    merged = _merged_size_rows(rows, deduplicate=False)
    return [row for row in merged if row["n"] not in ALIGNED_SIZES]


def _aligned_pair_rows(rows_a, rows_b):
    merged_a = _aligned_rows(rows_a)
    merged_b = _aligned_rows(rows_b)
    lookup_a = {row["n"]: row for row in merged_a}
    lookup_b = {row["n"]: row for row in merged_b}
    common_n = sorted(set(lookup_a) & set(lookup_b))
    return [lookup_a[n] for n in common_n], [lookup_b[n] for n in common_n]


def _plot_compare_runtime_aligned(ax, version_rows_map, metric_key, title, subtitle):
    for version, rows in version_rows_map.items():
        merged = _aligned_rows(rows)
        _line(ax, merged, "n", metric_key, VERSION_COLORS[version], VERSION_LABELS[version])
    ax.set_xlabel("Matrix size n")


def _plot_compare_runtime_unaligned(ax, version_rows_map, metric_key, title, subtitle):
    for version, rows in version_rows_map.items():
        merged = _unaligned_rows(rows)
        _line(ax, merged, "n", metric_key, VERSION_COLORS[version], "_nolegend_", marker="None", alpha=0.42)
        _scatter(ax, merged, "n", metric_key, VERSION_COLORS[version], VERSION_LABELS[version], alpha=0.58, s=24)
    ax.set_xlabel("Matrix size n")


def _plot_speedup_aligned(ax, baseline_rows, version_rows_map, title, subtitle):
    baseline_merged = _aligned_rows(baseline_rows)
    baseline_lookup = {row["n"]: row for row in baseline_merged}
    for version, rows in version_rows_map.items():
        merged = _aligned_rows(rows)
        speedup_rows = []
        for row in merged:
            if row["n"] in baseline_lookup:
                speedup_rows.append({
                    "n": row["n"],
                    "speedup": baseline_lookup[row["n"]]["median_seconds"] / row["median_seconds"],
                })
        _line(ax, speedup_rows, "n", "speedup", VERSION_COLORS[version], VERSION_LABELS[version])
    ax.set_xlabel("Matrix size n")
    ax.set_ylabel("Speedup")


def _plot_speedup_unaligned(ax, baseline_rows, version_rows_map, title, subtitle):
    baseline_by_sweep_n = {(row["sweep"], row["n"]): row for row in _combined_sweeps(baseline_rows, ["coarse", "fine", "large"])}
    for version, rows in version_rows_map.items():
        speedup_rows = []
        for row in _unaligned_rows(rows):
            key = (row["sweep"], row["n"])
            if key in baseline_by_sweep_n:
                speedup_rows.append({
                    "n": row["n"],
                    "speedup": baseline_by_sweep_n[key]["median_seconds"] / row["median_seconds"],
                })
        if speedup_rows:
            _line(ax, speedup_rows, "n", "speedup", VERSION_COLORS[version], "_nolegend_", marker="None", alpha=0.42)
            _scatter(ax, speedup_rows, "n", "speedup", VERSION_COLORS[version], VERSION_LABELS[version], alpha=0.58, s=24)
    ax.set_xlabel("Matrix size n")
    ax.set_ylabel("Speedup")


def plot_overview_runtime():
    fig, axes = plt.subplots(1, 2, figsize=(15, 5.8))
    version_rows_map = {version: load_csv(version, "sweeps.csv") for version in VERSIONS}
    _plot_compare_runtime_aligned(axes[0], version_rows_map, "median_seconds", "Runtime vs matrix size", "Aligned sizes: powers of two from 1 to 2048")
    _plot_compare_runtime_unaligned(axes[1], version_rows_map, "median_seconds", "Runtime vs matrix size", "Unaligned sizes: boundary and local sweep points")
    _panel_title(axes[0], 0, "Runtime vs matrix size", "Aligned sizes: powers of two from 1 to 2048")
    _panel_title(axes[1], 1, "Runtime vs matrix size", "Unaligned sizes: boundary and local sweep points")
    for ax in axes:
        ax.set_ylabel("Median runtime (s)")
        ax.set_yscale("log")
        ax.legend(ncol=2, fontsize=8)

    finalize_figure(fig, os.path.join(FIGURES_DIR, "01_overview_runtime_vs_n.png"))


def plot_overview_speedup_vs_v0():
    fig, axes = plt.subplots(1, 2, figsize=(15, 5.8))
    baseline_rows = load_csv("cholesky_v0_baseline", "sweeps.csv")
    version_rows_map = {version: load_csv(version, "sweeps.csv") for version in VERSIONS[1:]}
    _plot_speedup_aligned(axes[0], baseline_rows, version_rows_map, "Speedup against v0 baseline", "Aligned sizes: powers of two from 1 to 2048")
    _plot_speedup_unaligned(axes[1], baseline_rows, version_rows_map, "Speedup against v0 baseline", "Unaligned sizes: boundary and local sweep points")
    _panel_title(axes[0], 0, "Speedup against v0 baseline", "Aligned sizes: powers of two from 1 to 2048")
    _panel_title(axes[1], 1, "Speedup against v0 baseline", "Unaligned sizes: boundary and local sweep points")
    for ax in axes:
        ax.legend(ncol=2, fontsize=8)

    finalize_figure(fig, os.path.join(FIGURES_DIR, "03_overview_speedup_vs_v0.png"))


def plot_representative_bars():
    targets = [("coarse", 1000), ("fine", 1024), ("large", 2048)]
    x = np.arange(len(targets))
    width = 0.14

    fig, axes = plt.subplots(1, 2, figsize=(15, 5.8))

    for axis_index, metric in enumerate(["median_seconds", "median_gflops"]):
        ax = axes[axis_index]
        for idx, version in enumerate(VERSIONS):
            sweeps = load_csv(version, "sweeps.csv")
            values = []
            for sweep, n in targets:
                row = [item for item in sweeps if item["sweep"] == sweep and item["n"] == n][0]
                values.append(row[metric])
            ax.bar(
                x + (idx - 2) * width,
                values,
                width=width,
                color=VERSION_COLORS[version],
                label=VERSION_LABELS[version],
            )
        ax.set_xticks(x)
        ax.set_xticklabels(["%s/%d" % (s, n) for s, n in targets])
        ax.legend()

    _panel_title(axes[0], 0, "Representative runtime comparison", "Targets: coarse/1000, fine/1024, large/2048")
    axes[0].set_ylabel("Median runtime (s)")
    axes[0].set_yscale("log")

    _panel_title(axes[1], 1, "Representative throughput comparison", "Targets: coarse/1000, fine/1024, large/2048")
    axes[1].set_ylabel("Median GFLOP/s")

    finalize_figure(fig, os.path.join(FIGURES_DIR, "04_representative_runtime_bars.png"))

    # Save a second figure with just GFLOP/s for easier report use.
    fig2, ax2 = plt.subplots(figsize=(10.5, 6.0))
    for idx, version in enumerate(VERSIONS):
        sweeps = load_csv(version, "sweeps.csv")
        values = []
        for sweep, n in targets:
            row = [item for item in sweeps if item["sweep"] == sweep and item["n"] == n][0]
            values.append(row["median_gflops"])
        ax2.bar(
            x + (idx - 2) * width,
            values,
            width=width,
            color=VERSION_COLORS[version],
            label=VERSION_LABELS[version],
        )
    _title(ax2, "Representative GFLOP/s comparison", "Targets: coarse/1000, fine/1024, large/2048")
    ax2.set_xticks(x)
    ax2.set_xticklabels(["%s/%d" % (s, n) for s, n in targets])
    ax2.set_ylabel("Median GFLOP/s")
    ax2.legend()
    finalize_figure(fig2, os.path.join(FIGURES_DIR, "05_representative_gflops_bars.png"))


def _pairwise_transition_metric(version_a, version_b, filename, metric_key, ylabel, title_suffix):
    rows_a = load_csv(version_a, "sweeps.csv")
    rows_b = load_csv(version_b, "sweeps.csv")
    fig, axes = plt.subplots(1, 2, figsize=(15.2, 5.8))

    aligned_a, aligned_b = _aligned_pair_rows(rows_a, rows_b)
    _line(axes[0], aligned_a, "n", metric_key, VERSION_COLORS[version_a], VERSION_LABELS[version_a])
    _line(axes[0], aligned_b, "n", metric_key, VERSION_COLORS[version_b], VERSION_LABELS[version_b])
    _panel_title(axes[0], 0, "Pairwise %s: %s to %s" % (title_suffix, VERSION_LABELS[version_a], VERSION_LABELS[version_b]), "Aligned sizes: powers of two from 1 to 2048")
    axes[0].set_ylabel(ylabel)
    axes[0].set_xlabel("Matrix size n")
    axes[0].legend()

    raw_a = _unaligned_rows(rows_a)
    raw_b = _unaligned_rows(rows_b)
    _line(axes[1], raw_a, "n", metric_key, VERSION_COLORS[version_a], "_nolegend_", marker="None", alpha=0.42)
    _line(axes[1], raw_b, "n", metric_key, VERSION_COLORS[version_b], "_nolegend_", marker="None", alpha=0.42)
    _scatter(axes[1], raw_a, "n", metric_key, VERSION_COLORS[version_a], VERSION_LABELS[version_a], alpha=0.58, s=24)
    _scatter(axes[1], raw_b, "n", metric_key, VERSION_COLORS[version_b], VERSION_LABELS[version_b], alpha=0.58, s=24)
    _panel_title(axes[1], 1, "Pairwise %s: %s to %s" % (title_suffix, VERSION_LABELS[version_a], VERSION_LABELS[version_b]), "Unaligned sizes: boundary and local sweep points")
    axes[1].set_ylabel(ylabel)
    axes[1].set_xlabel("Matrix size n")
    axes[1].legend()

    if metric_key == "median_seconds":
        axes[0].set_yscale("log")
        axes[1].set_yscale("log")

    finalize_figure(fig, os.path.join(FIGURES_DIR, filename))


def plot_pairwise_transitions():
    transitions = [
        ("cholesky_v0_baseline", "cholesky_v1_serial_opt", "v0_to_v1"),
        ("cholesky_v1_serial_opt", "cholesky_v2_serial_blocked", "v1_to_v2"),
        ("cholesky_v2_serial_blocked", "cholesky_v3_openmp", "v2_to_v3"),
        ("cholesky_v3_openmp", "cholesky_v4_parallel_opt", "v3_to_final"),
        ("cholesky_v2_serial_blocked", "cholesky_v4_parallel_opt", "v2_to_final"),
    ]
    for index, (version_a, version_b, slug) in enumerate(transitions):
        runtime_file = "%02d_pair_%s_runtime.png" % (10 + index * 2, slug)
        gflops_file = "%02d_pair_%s_gflops.png" % (11 + index * 2, slug)
        _pairwise_transition_metric(version_a, version_b, runtime_file, "median_seconds", "Median runtime (s)", "runtime")
        _pairwise_transition_metric(version_a, version_b, gflops_file, "median_gflops", "Median GFLOP/s", "GFLOP/s")


def plot_parallel_runtime_v3_v4():
    fig, axes = plt.subplots(1, 2, figsize=(15.0, 5.8))
    version_rows_map = {
        "cholesky_v3_openmp": load_csv("cholesky_v3_openmp", "sweeps.csv"),
        "cholesky_v4_parallel_opt": load_csv("cholesky_v4_parallel_opt", "sweeps.csv"),
    }
    _plot_compare_runtime_aligned(axes[0], version_rows_map, "median_seconds", "Parallel-era runtime comparison", "Aligned sizes: powers of two from 1 to 2048")
    _plot_compare_runtime_unaligned(axes[1], version_rows_map, "median_seconds", "Parallel-era runtime comparison", "Unaligned sizes: boundary and local sweep points")
    _panel_title(axes[0], 0, "Parallel-era runtime comparison", "Aligned sizes: powers of two from 1 to 2048")
    _panel_title(axes[1], 1, "Parallel-era runtime comparison", "Unaligned sizes: boundary and local sweep points")
    for ax in axes:
        ax.set_ylabel("Median runtime (s)")
        ax.set_yscale("log")
        ax.legend(ncol=2, fontsize=8)
    finalize_figure(fig, os.path.join(FIGURES_DIR, "20_parallel_runtime_v3_v4.png"))


def plot_thread_scaling():
    fig, axes = plt.subplots(1, 2, figsize=(15.0, 5.8))
    data = {}
    for version in ["cholesky_v3_openmp", "cholesky_v4_parallel_opt"]:
        rows = load_csv(version, "thread_scaling.csv")
        data[version] = rows
        _line(axes[0], rows, "requested_threads", "median_seconds", VERSION_COLORS[version], VERSION_LABELS[version])

        baseline = rows[0]["median_seconds"]
        speedup_rows = []
        efficiency_rows = []
        for row in rows:
            speedup = baseline / row["median_seconds"]
            efficiency_rows.append({"requested_threads": row["requested_threads"], "efficiency": speedup / row["requested_threads"]})
            speedup_rows.append({"requested_threads": row["requested_threads"], "speedup": speedup})
        _line(axes[1], speedup_rows, "requested_threads", "speedup", VERSION_COLORS[version], "%s speedup" % VERSION_LABELS[version])
        _line(axes[1], efficiency_rows, "requested_threads", "efficiency", VERSION_COLORS[version], "%s efficiency" % VERSION_LABELS[version], linestyle="--")

    _panel_title(axes[0], 0, "Thread scaling runtime", "Canonical n=1024 thread sweep")
    axes[0].set_xlabel("Threads")
    axes[0].set_ylabel("Median runtime (s)")
    axes[0].set_xscale("log", basex=2)
    axes[0].legend()

    _panel_title(axes[1], 1, "Thread scaling quality", "Speedup and efficiency from the same sweep")
    axes[1].set_xlabel("Threads")
    axes[1].set_ylabel("Value")
    axes[1].set_xscale("log", basex=2)
    axes[1].legend()

    finalize_figure(fig, os.path.join(FIGURES_DIR, "21_thread_scaling_runtime.png"))

    fig2, ax2 = plt.subplots(figsize=(10.8, 6.0))
    for version in ["cholesky_v3_openmp", "cholesky_v4_parallel_opt"]:
        rows = data[version]
        baseline = rows[0]["median_seconds"]
        speedup_rows = []
        efficiency_rows = []
        for row in rows:
            speedup = baseline / row["median_seconds"]
            speedup_rows.append({"requested_threads": row["requested_threads"], "speedup": speedup})
            efficiency_rows.append({"requested_threads": row["requested_threads"], "efficiency": speedup / row["requested_threads"]})
        _line(ax2, speedup_rows, "requested_threads", "speedup", VERSION_COLORS[version], "%s speedup" % VERSION_LABELS[version])
        _line(ax2, efficiency_rows, "requested_threads", "efficiency", VERSION_COLORS[version], "%s efficiency" % VERSION_LABELS[version], linestyle="--")
    _title(ax2, "Thread scaling speedup and efficiency", "v4 Final Version should be read with care above 4 threads on the current host")
    ax2.set_xlabel("Threads")
    ax2.set_ylabel("Value")
    ax2.set_xscale("log", basex=2)
    ax2.legend()
    finalize_figure(fig2, os.path.join(FIGURES_DIR, "22_thread_scaling_speedup_efficiency.png"))


def plot_v4_thread_scaling_distribution():
    runs = parse_thread_scaling_runs("cholesky_v4_parallel_opt")
    fig, ax = plt.subplots(figsize=(10.8, 6.0))
    threads = sorted(set(item["threads"] for item in runs))
    for idx, thread_count in enumerate(threads):
        series = [item["seconds"] for item in runs if item["threads"] == thread_count]
        xs = np.full(len(series), idx + 1, dtype=float)
        jitter = np.linspace(-0.09, 0.09, len(series))
        ax.scatter(xs + jitter, series, s=42, color=VERSION_COLORS["cholesky_v4_parallel_opt"], alpha=0.82)
    ax.set_xticks(range(1, len(threads) + 1))
    ax.set_xticklabels([str(item) for item in threads])
    ax.set_yscale("log")
    ax.set_xlabel("Threads")
    ax.set_ylabel("Per-run runtime (s)")
    _title(ax, "v4 Final Version thread-scaling run distribution", "Repeated runs reveal the instability at 16 and 32 threads")
    finalize_figure(fig, os.path.join(FIGURES_DIR, "23_v4_thread_scaling_distribution.png"))


def plot_schedule_studies():
    v4_rows = load_csv("cholesky_v4_parallel_opt", "schedule_study.csv")
    fig, axes = plt.subplots(1, 2, figsize=(15.0, 5.8))

    labels = ["static:default", "static:1", "static:8", "dynamic:1", "dynamic:8", "guided:1", "guided:8"]
    order = [
        ("static", 0),
        ("static", 1),
        ("static", 8),
        ("dynamic", 1),
        ("dynamic", 8),
        ("guided", 1),
        ("guided", 8),
    ]
    medians = []
    mins = []
    maxs = []
    for schedule, chunk in order:
        row = [item for item in v4_rows if item["omp_schedule"] == schedule and item["omp_chunk"] == chunk][0]
        medians.append(row["median_seconds"])
        mins.append(row["min_seconds"])
        maxs.append(row["max_seconds"])

    x = np.arange(len(labels))
    error = [np.array(medians) - np.array(mins), np.array(maxs) - np.array(medians)]
    axes[0].bar(x, medians, color="#457b9d")
    axes[0].errorbar(x, medians, yerr=error, fmt="none", ecolor="#1d3557", capsize=4)
    axes[0].set_xticks(x)
    axes[0].set_xticklabels(labels, rotation=20, ha="right")
    axes[0].set_ylabel("Median runtime (s)")
    _panel_title(axes[0], 0, "v4 Final Version schedule study", "Error bars show min/max around the median")

    best = min(medians)
    relative = [value / best for value in medians]
    axes[1].bar(x, relative, color="#8ecae6")
    axes[1].axhline(1.0, color="#1d3557", linewidth=1.2, linestyle="--")
    axes[1].set_xticks(x)
    axes[1].set_xticklabels(labels, rotation=20, ha="right")
    axes[1].set_ylabel("Slowdown vs best")
    _panel_title(axes[1], 1, "v4 Final Version schedule study", "Relative to the fastest configuration")

    finalize_figure(fig, os.path.join(FIGURES_DIR, "30_v4_schedule_study.png"))

    fig2, ax2 = plt.subplots(figsize=(10.8, 6.0))
    v3_rows = load_csv("cholesky_v3_openmp", "schedule_study.csv")
    comparisons = [
        ("cholesky_v3_openmp", v3_rows, VERSION_COLORS["cholesky_v3_openmp"]),
        ("cholesky_v4_parallel_opt", v4_rows, VERSION_COLORS["cholesky_v4_parallel_opt"]),
    ]
    for label_key, rows, color in comparisons:
        xs = []
        ys = []
        for idx, (schedule, chunk) in enumerate(order):
            row = [item for item in rows if item["omp_schedule"] == schedule and item["omp_chunk"] == chunk][0]
            xs.append(idx)
            ys.append(row["median_seconds"])
        ax2.plot(xs, ys, marker="o", linewidth=2.2, markersize=5, color=color, label=VERSION_LABELS[label_key])
    ax2.set_xticks(range(len(labels)))
    ax2.set_xticklabels(labels, rotation=20, ha="right")
    ax2.set_ylabel("Median runtime (s)")
    _title(ax2, "Schedule sensitivity across OpenMP generations", "v3 vs v4 Final Version")
    ax2.legend()
    finalize_figure(fig2, os.path.join(FIGURES_DIR, "31_v3_v4_schedule_comparison.png"))


def plot_boundary_sizes():
    boundary_targets = [128, 256, 512, 1024]
    fig, axes = plt.subplots(1, 3, figsize=(18.0, 5.8), sharey=False)
    for ax, version in zip(axes, ["cholesky_v2_serial_blocked", "cholesky_v3_openmp", "cholesky_v4_parallel_opt"]):
        rows = [row for row in load_csv(version, "sweeps.csv") if row["n"] in boundary_targets]
        rows = sorted(rows, key=lambda item: item["n"])
        _line(ax, rows, "n", "median_seconds", VERSION_COLORS[version], VERSION_LABELS[version])
        for boundary in [128, 256, 512, 1024]:
            ax.axvline(boundary, color="#bdb8aa", linewidth=1.0, linestyle="--")
        _panel_title(ax, ["cholesky_v2_serial_blocked", "cholesky_v3_openmp", "cholesky_v4_parallel_opt"].index(version), "Aligned reference sizes", VERSION_LABELS[version])
        ax.set_xlabel("Matrix size n")
        ax.set_ylabel("Median runtime (s)")
    finalize_figure(fig, os.path.join(FIGURES_DIR, "40_boundary_sizes_v2_v3_v4.png"))


def plot_v4_stability():
    stability = parse_stability("cholesky_v4_parallel_opt")

    fig, axes = plt.subplots(1, 2, figsize=(15.0, 5.8))

    repeatability = stability["repeatability"]
    repeat_groups = _group_rows(repeatability, "n")
    for n_value in sorted(repeat_groups):
        rows = sorted(repeat_groups[n_value], key=lambda item: item["block_size"])
        median_rows = []
        for row in rows:
            median_rows.append({
                "block_size": row["block_size"],
                "median_runtime": np.median([row["run1"], row["run2"], row["run3"]]),
            })
        _line(axes[0], median_rows, "block_size", "median_runtime", VERSION_COLORS["cholesky_v4_parallel_opt"], "n=%d" % n_value)
    _panel_title(axes[0], 0, "v4 Final Version block-size repeatability", "Median of the three runs in stability.txt")
    axes[0].set_xlabel("Block size")
    axes[0].set_ylabel("Median runtime (s)")
    axes[0].legend()

    invariance = stability["block_invariance"]
    invariance_groups = _group_rows(invariance, "n")
    for n_value in sorted(invariance_groups):
        rows = sorted(invariance_groups[n_value], key=lambda item: item["compare_block_size"])
        speed_rows = []
        error_rows = []
        for row in rows:
            speed_rows.append({
                "compare_block_size": row["compare_block_size"],
                "relative_time": row["candidate_seconds"] / row["reference_seconds"],
            })
            error_rows.append({
                "compare_block_size": row["compare_block_size"],
                "rebuild_max_abs_error": row["rebuild_max_abs_error"],
            })
        _line(axes[1], speed_rows, "compare_block_size", "relative_time", VERSION_COLORS["cholesky_v4_parallel_opt"], "n=%d time ratio" % n_value)
        axes[1].scatter(
            [row["compare_block_size"] for row in error_rows],
            [max(row["rebuild_max_abs_error"], 1.0e-18) for row in error_rows],
            s=38,
            color="#d62828",
            alpha=0.7,
        )
    _panel_title(axes[1], 1, "v4 Final Version block invariance", "Lines: runtime ratio. Red dots: rebuild error floor")
    axes[1].set_xlabel("Compared block size")
    axes[1].set_ylabel("Candidate/reference runtime ratio")
    axes[1].legend(fontsize=8)
    finalize_figure(fig, os.path.join(FIGURES_DIR, "41_v4_block_repeatability.png"))

    fig2, ax2 = plt.subplots(figsize=(10.8, 6.0))
    for n_value in sorted(invariance_groups):
        rows = sorted(invariance_groups[n_value], key=lambda item: item["compare_block_size"])
        speed_rows = []
        for row in rows:
            speed_rows.append({
                "compare_block_size": row["compare_block_size"],
                "relative_time": row["candidate_seconds"] / row["reference_seconds"],
            })
        _line(ax2, speed_rows, "compare_block_size", "relative_time", VERSION_COLORS["cholesky_v4_parallel_opt"], "n=%d" % n_value)
    _title(ax2, "v4 Final Version block-size sensitivity", "Relative runtime against the reference block size 32")
    ax2.set_xlabel("Compared block size")
    ax2.set_ylabel("Candidate/reference runtime ratio")
    ax2.legend()
    finalize_figure(fig2, os.path.join(FIGURES_DIR, "42_v4_block_invariance.png"))


def plot_perf_counters():
    metrics = {
        "ipc": {"event": "instructions:u", "value_key": "metric_value", "filename": "50_ipc_vs_n.png", "ylabel": "Instructions per cycle", "title": "IPC over matrix size"},
        "cache_miss_pct": {"event": "cache-misses:u", "value_key": "metric_value", "filename": "51_cache_miss_pct_vs_n.png", "ylabel": "Cache misses (% of references)", "title": "Cache-miss percentage over matrix size"},
        "cycles": {"event": "cycles:u", "value_key": "count", "filename": "52_cycles_vs_n.png", "ylabel": "Cycles", "title": "Raw cycle count over matrix size"},
    }

    for metric_name, config in metrics.items():
        fig, ax = plt.subplots(figsize=(10.8, 6.0))
        for version in VERSIONS:
            rows = [row for row in load_csv(version, "perf_basic.csv") if row["event"] == config["event"]]
            rows = [row for row in rows if row["n"] in ALIGNED_SIZES]
            _line(ax, rows, "n", config["value_key"], VERSION_COLORS[version], VERSION_LABELS[version])
        _title(ax, config["title"], "Aligned sizes only")
        ax.set_xlabel("Matrix size n")
        ax.set_ylabel(config["ylabel"])
        if metric_name in ["cycles"]:
            ax.set_yscale("log")
        ax.legend()
        finalize_figure(fig, os.path.join(FIGURES_DIR, config["filename"]))


def plot_v0_v1_ipc():
    fig, ax = plt.subplots(figsize=(10.8, 6.0))
    for version in ["cholesky_v0_baseline", "cholesky_v1_serial_opt"]:
        rows = [row for row in load_csv(version, "perf_basic.csv") if row["event"] == "instructions:u"]
        rows = [row for row in rows if row["n"] in ALIGNED_SIZES]
        _line(ax, rows, "n", "metric_value", VERSION_COLORS[version], VERSION_LABELS[version])
    _title(ax, "IPC over matrix size", "v0 baseline vs v1 serial opt at aligned sizes")
    ax.set_xlabel("Matrix size n")
    ax.set_ylabel("Instructions per cycle")
    ax.legend()
    finalize_figure(fig, os.path.join(FIGURES_DIR, "53_v0_v1_ipc_vs_n.png"))


def plot_extended_thread_scaling_pairs():
    for index, n_value in enumerate([256, 1024, 2048]):
        fig, axes = plt.subplots(1, 2, figsize=(15.0, 5.8))
        for version in ["cholesky_v3_openmp", "cholesky_v4_parallel_opt"]:
            rows = [row for row in load_csv(version, "thread_scaling_extended.csv") if row["n"] == n_value]
            rows = sorted(rows, key=lambda row: row["requested_threads"])
            _line(axes[0], rows, "requested_threads", "median_seconds", VERSION_COLORS[version], VERSION_LABELS[version])

            baseline = rows[0]["median_seconds"]
            speedup_rows = []
            for row in rows:
                speedup_rows.append({
                    "requested_threads": row["requested_threads"],
                    "speedup": baseline / row["median_seconds"],
                })
            _line(axes[1], speedup_rows, "requested_threads", "speedup", VERSION_COLORS[version], VERSION_LABELS[version])

        ideal_threads = [1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 64]
        axes[1].plot(ideal_threads, ideal_threads, linestyle="--", linewidth=1.5, color="#6c757d", label="Ideal speedup")

        _panel_title(axes[0], 0, "Extended thread scaling at n=%d" % n_value, "Runtime vs thread count for v3 and v4 Final Version")
        axes[0].set_xlabel("Threads")
        axes[0].set_ylabel("Median runtime (s)")
        axes[0].set_yscale("log")
        axes[0].legend()

        _panel_title(axes[1], 1, "Extended thread scaling at n=%d" % n_value, "Speedup from the 1-thread runtime at fixed n")
        axes[1].set_xlabel("Threads")
        axes[1].set_ylabel("Speedup")
        axes[1].legend()

        finalize_figure(fig, os.path.join(FIGURES_DIR, "%02d_extended_thread_scaling_n%d.png" % (60 + index, n_value)))


def plot_v4_speedup_vs_threads_multi_n():
    fig, ax = plt.subplots(figsize=(10.8, 6.0))
    color_by_n = {
        256: "#457b9d",
        1024: "#d1495b",
        2048: "#2a9d8f",
    }
    rows = load_csv("cholesky_v4_parallel_opt", "thread_scaling_extended.csv")
    for n_value in [256, 1024, 2048]:
        n_rows = [row for row in rows if row["n"] == n_value]
        n_rows = sorted(n_rows, key=lambda row: row["requested_threads"])
        baseline = n_rows[0]["median_seconds"]
        speedup_rows = []
        for row in n_rows:
            speedup_rows.append({
                "requested_threads": row["requested_threads"],
                "speedup": baseline / row["median_seconds"],
            })
        _line(ax, speedup_rows, "requested_threads", "speedup", color_by_n[n_value], "n=%d" % n_value)
    threads = [1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 64]
    ax.plot(threads, threads, linestyle="--", linewidth=1.5, color="#6c757d", label="Ideal speedup")
    _title(ax, "v4 Final Version speedup vs threads", "Three representative problem sizes from the extended thread sweep")
    ax.set_xlabel("Threads")
    ax.set_ylabel("Speedup")
    ax.legend()
    finalize_figure(fig, os.path.join(FIGURES_DIR, "63_v4_speedup_vs_threads_multi_n.png"))


def build_all_figures():
    plot_overview_runtime()
    plot_overview_speedup_vs_v0()
    plot_representative_bars()
    plot_pairwise_transitions()
    plot_parallel_runtime_v3_v4()
    plot_thread_scaling()
    plot_v4_thread_scaling_distribution()
    plot_schedule_studies()
    plot_boundary_sizes()
    plot_v4_stability()
    plot_perf_counters()
    plot_v0_v1_ipc()
    plot_extended_thread_scaling_pairs()
    plot_v4_speedup_vs_threads_multi_n()
