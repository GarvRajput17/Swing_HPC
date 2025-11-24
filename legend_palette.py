#!/usr/bin/env python3
"""
Generate a standalone legend PNG that shows the color mapping for each algorithm.

Usage:
    python legend_palette.py --csv benchmark_results.csv --output legend.png
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import List

import matplotlib.pyplot as plt
import numpy as np


def load_algorithms(csv_path: Path) -> List[str]:
    if not csv_path.exists():
        raise SystemExit(f"Error: {csv_path} not found. Run the C++ simulation first.")

    algorithms: List[str] = []
    with csv_path.open("r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            algo = row["Algorithm"]
            if algo not in algorithms:
                algorithms.append(algo)
    return algorithms


def draw_legend(algorithms: List[str], output_path: Path) -> None:
    if not algorithms:
        raise SystemExit("No algorithms found in CSV; cannot draw legend.")

    colors = plt.cm.tab10(np.linspace(0, 1, len(algorithms)))
    handles = [
        plt.Line2D(
            [0],
            [0],
            color=color,
            linewidth=2.6,
            marker="o",
            markersize=8,
            label=algo,
        )
        for algo, color in zip(algorithms, colors)
    ]

    fig, ax = plt.subplots(figsize=(4.5, max(2, len(algorithms) * 0.4)))
    fig.patch.set_facecolor("#fdf8f1")
    ax.set_axis_off()
    legend = ax.legend(
        handles=handles,
        labels=algorithms,
        loc="center",
        frameon=False,
        borderpad=0.6,
        labelspacing=1.0,
    )
    legend.set_title("Algorithms", prop={"weight": "bold"})

    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_path, dpi=300, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved legend to {output_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Draw color legend for Swing benchmarks.")
    parser.add_argument(
        "--csv",
        type=Path,
        default=Path("benchmark_results.csv"),
        help="Path to benchmark_results.csv (default: benchmark_results.csv)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("animations/legend.png"),
        help="Path to output PNG (default: animations/legend.png)",
    )
    args = parser.parse_args()

    algorithms = load_algorithms(args.csv)
    draw_legend(algorithms, args.output)


if __name__ == "__main__":
    main()

