#!/usr/bin/env python3
import argparse
import os
import re


def parse_key_value_file(path):
    values = {}
    if not os.path.exists(path):
        return values
    with open(path, "r") as handle:
        for line in handle:
            line = line.strip()
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
    return values


def parse_reference_summary(path):
    result = {"failures": "missing", "max_abs_error": "missing", "max_rel_error": "missing"}
    if not os.path.exists(path):
        return result

    summary_line = None
    with open(path, "r") as handle:
        for line in handle:
            if line.startswith("summary "):
                summary_line = line.strip()

    if summary_line is None:
        return result

    for key in ("failures", "max_abs_error", "max_rel_error"):
        match = re.search(r"%s=([^ ]+)" % key, summary_line)
        if match:
            result[key] = match.group(1)
    return result


def parse_best_thread(path):
    if not os.path.exists(path):
        return ("missing", "missing", "missing")

    best_threads = "missing"
    best_seconds = None
    best_gflops = "missing"
    current_threads = None
    current_seconds = None
    current_gflops = None

    with open(path, "r") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if line.startswith("===== threads="):
                if current_threads is not None and current_seconds is not None:
                    if best_seconds is None or current_seconds < best_seconds:
                        best_seconds = current_seconds
                        best_threads = current_threads
                        best_gflops = current_gflops if current_gflops is not None else "missing"
                current_threads = line.replace("===== threads=", "", 1)
                current_seconds = None
                current_gflops = None
            elif line.startswith("median_seconds="):
                current_seconds = float(line.split("=", 1)[1])
            elif line.startswith("median_gflops="):
                current_gflops = line.split("=", 1)[1]

    if current_threads is not None and current_seconds is not None:
        if best_seconds is None or current_seconds < best_seconds:
            best_seconds = current_seconds
            best_threads = current_threads
            best_gflops = current_gflops if current_gflops is not None else "missing"

    if best_seconds is None:
        return ("missing", "missing", "missing")
    return (best_threads, "{:.9f}".format(best_seconds), best_gflops)


def parse_best_thread_by_size(path):
    if not os.path.exists(path):
        return {}

    best_by_size = {}
    current_n = None
    current_threads = None
    current_seconds = None
    current_gflops = None

    with open(path, "r") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if line.startswith("===== n=") and " threads=" in line:
                if current_n is not None and current_threads is not None and current_seconds is not None:
                    existing = best_by_size.get(current_n)
                    if existing is None or current_seconds < existing[1]:
                        best_by_size[current_n] = (current_threads, current_seconds, current_gflops)

                match = re.match(r"===== n=([^ ]+) threads=(.+)", line)
                current_n = match.group(1) if match else None
                current_threads = match.group(2) if match else None
                current_seconds = None
                current_gflops = None
            elif line.startswith("median_seconds="):
                current_seconds = float(line.split("=", 1)[1])
            elif line.startswith("median_gflops="):
                current_gflops = line.split("=", 1)[1]

    if current_n is not None and current_threads is not None and current_seconds is not None:
        existing = best_by_size.get(current_n)
        if existing is None or current_seconds < existing[1]:
            best_by_size[current_n] = (current_threads, current_seconds, current_gflops)

    normalized = {}
    for n, (threads, seconds, gflops) in best_by_size.items():
        normalized[n] = (threads, "{:.9f}".format(seconds), gflops if gflops is not None else "missing")
    return normalized


def parse_best_schedule(path):
    if not os.path.exists(path):
        return ("missing", "missing")

    best_label = "missing"
    best_seconds = None
    current_schedule = None
    current_chunk = None
    current_seconds = None

    with open(path, "r") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if line.startswith("===== schedule="):
                if current_schedule is not None and current_seconds is not None:
                    if best_seconds is None or current_seconds < best_seconds:
                        best_seconds = current_seconds
                        best_label = "%s,%s" % (current_schedule, current_chunk)
                parts = line.replace("===== ", "").split()
                current_schedule = parts[0].split("=", 1)[1]
                current_chunk = parts[1].split("=", 1)[1]
                current_seconds = None
            elif line.startswith("median_seconds="):
                current_seconds = float(line.split("=", 1)[1])

    if current_schedule is not None and current_seconds is not None:
        if best_seconds is None or current_seconds < best_seconds:
            best_seconds = current_seconds
            best_label = "%s,%s" % (current_schedule, current_chunk)

    if best_seconds is None:
        return ("missing", "missing")
    return (best_label, "{:.9f}".format(best_seconds))


def parse_last_large(path):
    if not os.path.exists(path):
        return ("missing", "missing", "missing")

    blocks = []
    current_n = None
    current_seconds = None
    current_gflops = None
    with open(path, "r") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if line.startswith("===== n="):
                if current_n is not None:
                    blocks.append((current_n, current_seconds, current_gflops))
                current_n = line.replace("===== n=", "", 1)
                current_seconds = None
                current_gflops = None
            elif line.startswith("median_seconds="):
                current_seconds = line.split("=", 1)[1]
            elif line.startswith("median_gflops="):
                current_gflops = line.split("=", 1)[1]
        if current_n is not None:
            blocks.append((current_n, current_seconds, current_gflops))

    if not blocks:
        return ("missing", "missing", "missing")
    return blocks[-1]


def print_table(rows):
    width_key = max(len(row[0]) for row in rows)
    width_value = max(len(str(row[1])) for row in rows)
    rule = "+" + "-" * (width_key + 2) + "+" + "-" * (width_value + 2) + "+"
    print(rule)
    for key, value in rows:
        print("| {key:<{w1}} | {value:<{w2}} |".format(key=key, value=value, w1=width_key, w2=width_value))
        print(rule)


def summarize_label(label):
    result_dir = os.path.join("history", "results", label)
    summary = parse_key_value_file(os.path.join(result_dir, "summary.txt"))
    metadata = parse_key_value_file(os.path.join(result_dir, "metadata.txt"))
    reference = parse_reference_summary(os.path.join(result_dir, "reference_logdet.txt"))
    best_threads, best_thread_seconds, best_thread_gflops = parse_best_thread(
        os.path.join(result_dir, "thread_scaling.txt")
    )
    best_threads_extended = parse_best_thread_by_size(
        os.path.join(result_dir, "thread_scaling_extended.txt")
    )
    best_schedule, best_schedule_seconds = parse_best_schedule(
        os.path.join(result_dir, "schedule_study.txt")
    )
    last_large_n, last_large_seconds, last_large_gflops = parse_last_large(
        os.path.join(result_dir, "time_large.txt")
    )

    print("----------------------------------------------------------------------")
    print("RESULT SUMMARY: {}".format(label))
    print("----------------------------------------------------------------------")
    rows = [
        ("label", label),
        ("correctness", summary.get("correctness", "missing")),
        ("reference_failures", reference["failures"]),
        ("reference_max_abs_error", reference["max_abs_error"]),
        ("reference_max_rel_error", reference["max_rel_error"]),
        ("partition", metadata.get("slurm_job_partition", "unset")),
        ("node", metadata.get("slurmd_nodename", metadata.get("hostname", "missing"))),
        ("omp_num_threads", metadata.get("omp_num_threads", "missing")),
        ("omp_places", metadata.get("omp_places", "missing")),
        ("omp_proc_bind", metadata.get("omp_proc_bind", "missing")),
        ("kernel_schedule", metadata.get("cholesky_omp_schedule", "missing")),
        ("kernel_chunk", metadata.get("cholesky_omp_chunk", "missing")),
        ("best_thread_count@1024", best_threads),
        ("best_thread_median_seconds", best_thread_seconds),
        ("best_thread_median_gflops", best_thread_gflops),
        ("best_schedule@1024", best_schedule),
        ("best_schedule_seconds", best_schedule_seconds),
        ("largest_n", last_large_n),
        ("largest_n_median_seconds", last_large_seconds),
        ("largest_n_median_gflops", last_large_gflops),
    ]

    for n in ("256", "1024", "2048"):
        if n in best_threads_extended:
            best_threads_n, best_seconds_n, best_gflops_n = best_threads_extended[n]
            rows.extend(
                [
                    ("best_thread_count@{}_ext".format(n), best_threads_n),
                    ("best_thread_seconds@{}_ext".format(n), best_seconds_n),
                    ("best_thread_gflops@{}_ext".format(n), best_gflops_n),
                ]
            )
    print_table(rows)


def compare_labels(labels):
    print("======================================================================")
    print("ALL-VERSION SUMMARY")
    print("======================================================================")
    header = (
        "| Version | Partition | Threads | RefFail | BestThread@1024 | BestThreadGFLOPS | "
        "BestSchedule@1024 | LargestN | LargestNGFLOPS |"
    )
    rule = "|" + "---|" * 9
    print(header)
    print(rule)
    for label in labels:
        result_dir = os.path.join("history", "results", label)
        metadata = parse_key_value_file(os.path.join(result_dir, "metadata.txt"))
        reference = parse_reference_summary(os.path.join(result_dir, "reference_logdet.txt"))
        best_threads, _, best_thread_gflops = parse_best_thread(os.path.join(result_dir, "thread_scaling.txt"))
        best_threads_extended = parse_best_thread_by_size(os.path.join(result_dir, "thread_scaling_extended.txt"))
        best_schedule, _ = parse_best_schedule(os.path.join(result_dir, "schedule_study.txt"))
        last_large_n, _, last_large_gflops = parse_last_large(os.path.join(result_dir, "time_large.txt"))
        print(
            "| {label} | {partition} | {threads} | {failures} | {best_threads} | {best_thread_gflops} | "
            "{best_schedule} | {largest_n} | {largest_gflops} |".format(
                label=label,
                partition=metadata.get("slurm_job_partition", "unset"),
                threads=metadata.get("omp_num_threads", "missing"),
                failures=reference["failures"],
                best_threads=best_threads,
                best_thread_gflops=best_thread_gflops,
                best_schedule=best_schedule,
                largest_n=last_large_n,
                largest_gflops=last_large_gflops,
            )
        )

    if any(parse_best_thread_by_size(os.path.join("history", "results", label, "thread_scaling_extended.txt"))
           for label in labels):
        print("")
        print("======================================================================")
        print("EXTENDED THREAD-SCALING SUMMARY")
        print("======================================================================")
        print("| Version | Best@256 | Best@1024 | Best@2048 |")
        print("|---|---|---|---|")
        for label in labels:
            best_threads_extended = parse_best_thread_by_size(
                os.path.join("history", "results", label, "thread_scaling_extended.txt")
            )
            def format_best(n):
                if n not in best_threads_extended:
                    return "n/a"
                threads, seconds, gflops = best_threads_extended[n]
                return "{} thr / {} GFLOPS".format(threads, gflops)

            print(
                "| {label} | {b256} | {b1024} | {b2048} |".format(
                    label=label,
                    b256=format_best("256"),
                    b1024=format_best("1024"),
                    b2048=format_best("2048"),
                )
            )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--label")
    parser.add_argument("--compare", nargs="*")
    args = parser.parse_args()

    if args.label:
        summarize_label(args.label)
    if args.compare:
        compare_labels(args.compare)


if __name__ == "__main__":
    main()
