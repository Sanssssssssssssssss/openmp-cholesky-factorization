#!/usr/bin/env python3
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = REPO_ROOT / "perf_analysis" / "output" / "tables"
OUT_PATH = OUT_DIR / "appendix_code_versions.tex"

VERSIONS = [
    {
        "tag": "cholesky_v0_baseline",
        "label": "v0",
        "title": "v0: Baseline serial kernel",
        "source": REPO_ROOT / "history" / "versions" / "cholesky_v0_baseline" / "src" / "cholesky.cpp",
        "summary": (
            "This version is a direct serial implementation of the baseline algorithm. "
            "It serves as the initial correctness and timing reference before any restructuring or parallelisation."
        ),
        "annotate_openmp": False,
    },
    {
        "tag": "cholesky_v1_serial_opt",
        "label": "v1",
        "title": "v1: Serial kernel clean-up",
        "source": REPO_ROOT / "history" / "versions" / "cholesky_v1_serial_opt" / "src" / "cholesky.cpp",
        "summary": (
            "This version keeps the same mathematical algorithm as v0, but reduces loop overhead, "
            "reuses row pointers, and replaces repeated division with multiplication by a precomputed reciprocal."
        ),
        "annotate_openmp": False,
    },
    {
        "tag": "cholesky_v2_serial_blocked",
        "label": "v2",
        "title": "v2: Blocked serial formulation",
        "source": REPO_ROOT / "history" / "versions" / "cholesky_v2_serial_blocked" / "src" / "cholesky.cpp",
        "summary": (
            "This version introduces panel factorisation, block-column solve, and tiled trailing-matrix updates. "
            "The purpose is to improve locality while remaining fully serial."
        ),
        "annotate_openmp": False,
    },
    {
        "tag": "cholesky_v3_openmp",
        "label": "v3",
        "title": "v3: First OpenMP version",
        "source": REPO_ROOT / "history" / "versions" / "cholesky_v3_openmp" / "src" / "cholesky.cpp",
        "summary": (
            "This version keeps the blocked structure of v2 and introduces OpenMP in the trailing update. "
            "That region is the natural first parallel hotspot because tiles are independent for a fixed panel."
        ),
        "annotate_openmp": True,
    },
    {
        "tag": "cholesky_v4_parallel_opt",
        "label": "v4",
        "title": "v4: Final tuned OpenMP kernel",
        "source": REPO_ROOT / "src" / "cholesky.cpp",
        "summary": (
            "This is the final accepted implementation. It extends the OpenMP version with runtime-configurable scheduling, "
            "parallel block-column solve, and SIMD-assisted inner kernels."
        ),
        "annotate_openmp": True,
    },
]


STYLE_BLOCK = r"""
% Requires in the main preamble:
% \usepackage{listings}
% \usepackage{xcolor}
%
% If not already defined in the report preamble, the following style block can
% remain here safely.
\definecolor{codebg}{RGB}{248,248,248}
\definecolor{codeframe}{RGB}{220,220,220}
\definecolor{codecomment}{RGB}{76,114,176}
\definecolor{codekeyword}{RGB}{180,60,40}
\definecolor{codestring}{RGB}{34,139,34}

\lstdefinestyle{appendixcode}{
  language=C++,
  basicstyle=\ttfamily\scriptsize,
  keywordstyle=\color{codekeyword}\bfseries,
  commentstyle=\color{codecomment},
  stringstyle=\color{codestring},
  numbers=left,
  numberstyle=\tiny,
  stepnumber=1,
  numbersep=8pt,
  showstringspaces=false,
  tabsize=4,
  breaklines=true,
  breakatwhitespace=false,
  columns=fullflexible,
  keepspaces=true,
  frame=single,
  rulecolor=\color{codeframe},
  backgroundcolor=\color{codebg},
  xleftmargin=1.2em,
  framexleftmargin=1.0em,
  aboveskip=0.8em,
  belowskip=0.8em,
  captionpos=b
}
""".strip()


def tex_escape(text: str) -> str:
    replacements = {
        "\\": r"\textbackslash{}",
        "&": r"\&",
        "%": r"\%",
        "$": r"\$",
        "#": r"\#",
        "_": r"\_",
        "{": r"\{",
        "}": r"\}",
        "~": r"\textasciitilde{}",
        "^": r"\textasciicircum{}",
    }
    for key, value in replacements.items():
        text = text.replace(key, value)
    return text


def annotate_openmp_lines(code: str) -> str:
    annotated = []
    for line in code.splitlines():
        stripped = line.strip()
        if stripped == "#include <omp.h>":
            annotated.append("// Appendix note: OpenMP header included so this version can use runtime control and parallel directives.")
        if "struct OpenMPScheduleConfig" in stripped:
            annotated.append("// Appendix note: This helper stores the runtime OpenMP schedule policy and chunk size used by the tuned final kernel.")
        if stripped.startswith("omp_sched_t kind"):
            annotated.append("// Appendix note: The schedule kind is kept explicit so the benchmarking workflow can compare static, dynamic, and guided policies.")
        if "omp_set_schedule(" in stripped:
            annotated.append("// Appendix note: The OpenMP runtime is configured here so benchmark runs can tune schedule policy without recompiling.")
        if stripped.startswith("#pragma omp parallel for"):
            annotated.append("// Appendix note: This OpenMP parallel-for distributes independent work across threads in the main parallel region.")
        if stripped.startswith("#pragma omp simd"):
            annotated.append("// Appendix note: This OpenMP SIMD reduction asks the compiler to vectorise the inner dot product safely.")
        annotated.append(line)
    return "\n".join(annotated)


def build_version_section(version):
    code = version["source"].read_text()
    if version["annotate_openmp"]:
        code = annotate_openmp_lines(code)
    return "\n".join([
        r"\subsection{%s}" % tex_escape(version["title"]),
        "",
        "Tag: \\texttt{%s}." % tex_escape(version["tag"]),
        "",
        version["summary"],
        "",
        r"\begin{lstlisting}[style=appendixcode]",
        code.rstrip(),
        r"\end{lstlisting}",
        "",
    ])


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    parts = [
        r"\section{Code evolution across milestone versions}",
        "",
        STYLE_BLOCK,
        "",
        (
            "This appendix presents the Cholesky implementation for each milestone version using the archived "
            "source snapshots stored in the repository. For v0--v3 the listings are taken from "
            "\\texttt{history/versions/<tag>/src/cholesky.cpp}, while the v4 listing is taken from the current "
            "\\texttt{src/cholesky.cpp}. For the OpenMP versions, short appendix-only explanatory comments are "
            "inserted around OpenMP-specific lines to make the parallel intent clear without rewriting project history."
        ),
        "",
    ]
    for version in VERSIONS:
        parts.append(build_version_section(version))

    OUT_PATH.write_text("\n".join(parts) + "\n")
    print("Wrote", OUT_PATH)


if __name__ == "__main__":
    main()
