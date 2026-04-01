import csv
import os

from .data_loader import load_csv
from .paths import TABLES_DIR


EXTENDED_VERSIONS = [
    ("cholesky_v3_openmp", "v3 OpenMP"),
    ("cholesky_v4_parallel_opt", "v4 Final Version"),
]

EXTENDED_SIZES = [256, 1024, 2048]


def _fmt_runtime(value):
    return "%.6f" % float(value)


def _fmt_gflops(value):
    return "%.4f" % float(value)


def _fmt_metric(value):
    if value is None:
        return "--"
    return "%.6f" % float(value)


def _karp_flatt(speedup, threads):
    if threads <= 1 or speedup <= 0:
        return None
    return ((1.0 / speedup) - (1.0 / threads)) / (1.0 - (1.0 / threads))


def _extended_rows(version):
    rows = load_csv(version, "thread_scaling_extended.csv")
    rows.sort(key=lambda row: (int(row["n"]), int(row["requested_threads"])))
    return rows


def _derived_rows(version):
    rows = _extended_rows(version)
    grouped = {}
    for row in rows:
        grouped.setdefault(int(row["n"]), []).append(row)

    derived = []
    for n_value in EXTENDED_SIZES:
        n_rows = sorted(grouped[n_value], key=lambda row: int(row["requested_threads"]))
        baseline = float(n_rows[0]["median_seconds"])
        for row in n_rows:
            threads = int(row["requested_threads"])
            runtime = float(row["median_seconds"])
            gflops = float(row["median_gflops"])
            speedup = baseline / runtime
            efficiency = speedup / threads
            epsilon = _karp_flatt(speedup, threads)
            derived.append({
                "n": n_value,
                "threads": threads,
                "median_seconds": runtime,
                "median_gflops": gflops,
                "speedup": speedup,
                "efficiency": efficiency,
                "karp_flatt": epsilon,
            })
    return derived


def _write_csv(path, rows):
    with open(path, "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["n", "threads", "median_seconds", "median_gflops", "speedup", "efficiency", "karp_flatt_serial_fraction"])
        for row in rows:
            writer.writerow([
                row["n"],
                row["threads"],
                _fmt_runtime(row["median_seconds"]),
                _fmt_gflops(row["median_gflops"]),
                _fmt_metric(row["speedup"]),
                _fmt_metric(row["efficiency"]),
                _fmt_metric(row["karp_flatt"]),
            ])


def _write_tex(path, version_rows):
    lines = [
        r"% Requires \usepackage{longtable,booktabs}",
        r"\section{Extended thread-scaling derived metrics}",
        "",
    ]
    for version_label, rows in version_rows:
        label_slug = version_label.lower().replace(" ", "_").replace("-", "_")
        lines.extend([
            r"\begin{longtable}{rrccccc}",
            r"\caption{Extended thread-scaling metrics for %s at $n \in \{256, 1024, 2048\}$. Speedup is computed against the 1-thread runtime at fixed $n$, efficiency is $S(p)/p$, and the Karp--Flatt serial fraction is $\varepsilon(p)=\frac{1/S(p)-1/p}{1-1/p}$ for $p>1$.}\\"
            % version_label,
            r"\toprule",
            r"\textbf{$n$} & \textbf{Threads} & \textbf{Runtime (s)} & \textbf{GFLOP/s} & \textbf{Speedup} & \textbf{Efficiency} & \textbf{Karp--Flatt $\varepsilon$} \\",
            r"\midrule",
            r"\endfirsthead",
            r"\toprule",
            r"\textbf{$n$} & \textbf{Threads} & \textbf{Runtime (s)} & \textbf{GFLOP/s} & \textbf{Speedup} & \textbf{Efficiency} & \textbf{Karp--Flatt $\varepsilon$} \\",
            r"\midrule",
            r"\endhead",
        ])
        current_n = None
        for row in rows:
            n_text = str(row["n"]) if row["n"] != current_n else ""
            current_n = row["n"]
            lines.append(
                "%s & %d & %s & %s & %s & %s & %s \\\\" % (
                    n_text,
                    row["threads"],
                    _fmt_runtime(row["median_seconds"]),
                    _fmt_gflops(row["median_gflops"]),
                    _fmt_metric(row["speedup"]),
                    _fmt_metric(row["efficiency"]),
                    _fmt_metric(row["karp_flatt"]),
                )
            )
        lines.extend([
            r"\bottomrule",
            r"\label{tab:%s_thread_scaling_extended}" % label_slug,
            r"\end{longtable}",
            "",
        ])
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def generate_thread_scaling_appendix_tables():
    if not os.path.isdir(TABLES_DIR):
        os.makedirs(TABLES_DIR)

    version_rows = []
    for version, label in EXTENDED_VERSIONS:
        rows = _derived_rows(version)
        version_rows.append((label, rows))
        _write_csv(os.path.join(TABLES_DIR, "%s_thread_scaling_extended_metrics.csv" % version), rows)

    _write_tex(os.path.join(TABLES_DIR, "thread_scaling_extended_appendix_tables.tex"), version_rows)

