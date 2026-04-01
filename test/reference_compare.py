#!/usr/bin/env python3
import argparse
import math
import subprocess
import sys

import numpy as np


def parse_sizes(text):
    sizes = []
    for item in text.split(","):
        item = item.strip()
        if not item:
            continue
        sizes.append(int(item))
    return sizes


def make_corr_matrix(n):
    matrix = np.zeros((n, n), dtype=np.float64)
    scale = float(n) * float(n)
    for i in range(n):
        for j in range(n):
            delta = float(i - j)
            matrix[i, j] = 0.99 * math.exp(-0.5 * 16.0 * delta * delta / scale)
        matrix[i, i] = 1.0
    return matrix


def parse_driver_output(text):
    values = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()
    return values


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--csv", required=True)
    parser.add_argument(
        "--sizes",
        default="2,8,32,128,512,1024",
        help="comma-separated matrix sizes for the reference comparison",
    )
    parser.add_argument("--rtol", type=float, default=1e-8)
    args = parser.parse_args()

    sizes = parse_sizes(args.sizes)
    rows = []
    failures = 0
    max_abs_error = 0.0
    max_rel_error = 0.0

    with open(args.output, "w") as text_file, open(args.csv, "w") as csv_file:
        csv_file.write("n,cholesky_logdet,numpy_logdet,abs_error,rel_error,elapsed_seconds,status\n")

        for n in sizes:
            driver = subprocess.run(
                [args.driver, str(n)],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
            )
            values = parse_driver_output(driver.stdout)
            if "logdet" not in values or "elapsed_seconds" not in values:
                raise RuntimeError("failed to parse driver output for n={}".format(n))

            cholesky_logdet = float(values["logdet"])
            elapsed_seconds = float(values["elapsed_seconds"])

            reference_matrix = make_corr_matrix(n)
            sign, numpy_logdet = np.linalg.slogdet(reference_matrix)
            if sign <= 0.0:
                raise RuntimeError("reference matrix is not SPD for n={}".format(n))

            abs_error = abs(cholesky_logdet - numpy_logdet)
            rel_error = abs_error / max(1.0, abs(numpy_logdet))
            max_abs_error = max(max_abs_error, abs_error)
            max_rel_error = max(max_rel_error, rel_error)

            ok = rel_error <= args.rtol
            status = "PASS" if ok else "FAIL"
            if not ok:
                failures += 1

            line = (
                "[{status}] n={n} cholesky_logdet={cholesky:.17e} "
                "numpy_logdet={numpy:.17e} abs_error={abs_error:.17e} "
                "rel_error={rel_error:.17e} elapsed_seconds={elapsed:.17e}"
            ).format(
                status=status,
                n=n,
                cholesky=cholesky_logdet,
                numpy=numpy_logdet,
                abs_error=abs_error,
                rel_error=rel_error,
                elapsed=elapsed_seconds,
            )
            text_file.write(line + "\n")
            csv_file.write(
                "{n},{cholesky:.17e},{numpy:.17e},{abs_error:.17e},{rel_error:.17e},{elapsed:.17e},{status}\n".format(
                    n=n,
                    cholesky=cholesky_logdet,
                    numpy=numpy_logdet,
                    abs_error=abs_error,
                    rel_error=rel_error,
                    elapsed=elapsed_seconds,
                    status=status,
                )
            )

        summary = "summary sizes={sizes} failures={failures} max_abs_error={abs_error:.17e} max_rel_error={rel_error:.17e}".format(
            sizes=",".join(str(n) for n in sizes),
            failures=failures,
            abs_error=max_abs_error,
            rel_error=max_rel_error,
        )
        text_file.write(summary + "\n")

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
