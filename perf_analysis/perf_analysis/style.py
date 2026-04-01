import os

import matplotlib

matplotlib.use("Agg")

from matplotlib import pyplot as plt


def apply_matplotlib_style():
    plt.style.use("seaborn-whitegrid")
    plt.rcParams.update({
        "figure.figsize": (10.5, 6.0),
        "figure.dpi": 140,
        "savefig.dpi": 180,
        "savefig.bbox": "tight",
        "axes.facecolor": "#fbfaf7",
        "figure.facecolor": "#fbfaf7",
        "axes.edgecolor": "#d6d0c4",
        "axes.labelcolor": "#24323d",
        "axes.titlesize": 14,
        "axes.titleweight": "bold",
        "axes.labelsize": 11,
        "grid.color": "#ddd6c8",
        "grid.linestyle": "--",
        "grid.alpha": 0.45,
        "xtick.color": "#31424f",
        "ytick.color": "#31424f",
        "legend.frameon": True,
        "legend.facecolor": "#fffdf8",
        "legend.edgecolor": "#d8d2c6",
        "font.family": "DejaVu Serif",
    })


def ensure_parent_dir(path):
    parent = os.path.dirname(path)
    if parent and not os.path.isdir(parent):
        os.makedirs(parent)


def finalize_figure(fig, path):
    ensure_parent_dir(path)
    fig.savefig(path)
    plt.close(fig)
