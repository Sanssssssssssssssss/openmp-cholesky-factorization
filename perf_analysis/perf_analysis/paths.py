import os


PACKAGE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(PACKAGE_DIR)
REPO_ROOT = os.path.dirname(PROJECT_DIR)
RESULTS_DIR = os.path.join(REPO_ROOT, "history", "results")
OUTPUT_DIR = os.path.join(PROJECT_DIR, "output")
FIGURES_DIR = os.path.join(OUTPUT_DIR, "figures")
TABLES_DIR = os.path.join(OUTPUT_DIR, "tables")


VERSIONS = [
    "cholesky_v0_baseline",
    "cholesky_v1_serial_opt",
    "cholesky_v2_serial_blocked",
    "cholesky_v3_openmp",
    "cholesky_v4_parallel_opt",
]


VERSION_LABELS = {
    "cholesky_v0_baseline": "v0 baseline",
    "cholesky_v1_serial_opt": "v1 serial opt",
    "cholesky_v2_serial_blocked": "v2 blocked",
    "cholesky_v3_openmp": "v3 OpenMP",
    "cholesky_v4_parallel_opt": "v4 Final Version",
}


VERSION_COLORS = {
    "cholesky_v0_baseline": "#5c6b73",
    "cholesky_v1_serial_opt": "#2a9d8f",
    "cholesky_v2_serial_blocked": "#e9c46a",
    "cholesky_v3_openmp": "#f4a261",
    "cholesky_v4_parallel_opt": "#d1495b",
}
