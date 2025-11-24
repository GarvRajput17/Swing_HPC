#!/usr/bin/env python3
"""
Generate animated latency and goodput plots for the Swing benchmarks.

Each animation gradually draws the per-algorithm curves (left-to-right) so you
can drop the resulting MP4/GIF straight into a presentation.

Usage:
    python animate_performance.py --output-dir animations --format mp4
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List

import matplotlib.animation as animation
import matplotlib.pyplot as plt
import numpy as np


def load_data(csv_path: Path) -> List[Dict[str, float]]:
    if not csv_path.exists():
        raise SystemExit(f"Error: {csv_path} not found. Run the C++ simulation first.")

    rows: List[Dict[str, float]] = []
    with csv_path.open("r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(
                {
                    "VectorSizeBytes": float(row["VectorSizeBytes"]),
                    "Algorithm": row["Algorithm"],
                    "Time_us": float(row["Time_us"]),
                    "Goodput_Gbps": float(row["Goodput_Gbps"]),
                }
            )

    rows.sort(key=lambda r: r["VectorSizeBytes"])
    return rows


def prepare_series(rows: List[Dict[str, float]], value_key: str):
    by_algo: Dict[str, List[tuple]] = {}
    for row in rows:
        by_algo.setdefault(row["Algorithm"], []).append(
            (row["VectorSizeBytes"], row[value_key])
        )

    algorithms = list(by_algo.keys())
    colors = plt.cm.tab10(np.linspace(0, 1, len(algorithms)))

    series = []
    for algo, color in zip(algorithms, colors):
        points = sorted(by_algo[algo], key=lambda p: p[0])
        x_vals = [p[0] for p in points]
        y_vals = [p[1] for p in points]
        series.append({"label": algo, "color": color, "x": x_vals, "y": y_vals})

    max_points = max(len(s["x"]) for s in series) if series else 0
    return series, max_points



def animate_plot(ax, series, frames, y_label, title, yscale="log"):
    lines = []

    for data in series:
        (line,) = ax.plot(
            [],
            [],
            color=data["color"],
            linewidth=2.4,
            marker="o",
            markersize=5,
        )
        lines.append(line)

    ax.set_title(title, fontsize=14, fontweight="bold")
    ax.set_xlabel("Message Size (Bytes)")
    ax.set_ylabel(y_label)
    ax.set_xscale("log", base=2)

    positive_y = [val for data in series for val in data["y"] if val > 0]
    if yscale == "log" and positive_y:
        ax.set_yscale("log")
        y_min = min(positive_y) * 0.8
        y_max = max(positive_y) * 1.2
    else:
        ax.set_yscale("linear")
        y_vals = [val for data in series for val in data["y"]]
        if y_vals:
            y_min = min(y_vals) - 0.1 * abs(min(y_vals))
            y_max = max(y_vals) + 0.1 * abs(max(y_vals))
        else:
            y_min, y_max = 0, 1
    ax.set_ylim(y_min, y_max)

    ax.grid(which="major", linestyle="--", linewidth=0.8, alpha=0.4)
    ax.grid(which="minor", linestyle=":", linewidth=0.5, alpha=0.3)
    ax.set_facecolor("#f8f6f2")
    ax.figure.patch.set_facecolor("#fdf8f1")

    x_min = min(data["x"][0] for data in series)
    x_max = max(data["x"][-1] for data in series)
    ax.set_xlim(x_min * 0.8, x_max * 1.2)

    def init():
        for line in lines:
            line.set_data([], [])
        return lines

    def update(frame):
        for data, line in zip(series, lines):
            idx = min(frame + 1, len(data["x"]))
            line.set_data(data["x"][:idx], data["y"][:idx])
        return lines

    return animation.FuncAnimation(
        ax.figure,
        update,
        init_func=init,
        frames=frames,
        interval=500,
        blit=True,
        repeat=False,
    )


def save_animation(anim, output_path: Path, fmt: str):
    output_path.parent.mkdir(parents=True, exist_ok=True)
    if fmt == "gif":
        writer = animation.PillowWriter(fps=2)
        anim.save(output_path, writer=writer)
    else:
        writer = animation.FFMpegWriter(fps=2, bitrate=1800)
        anim.save(output_path, writer=writer)
    print(f"Saved {output_path}")


def main():
    plt.style.use("seaborn-v0_8-whitegrid")
    parser = argparse.ArgumentParser(description="Animate Swing benchmark graphs.")
    parser.add_argument(
        "--csv",
        type=Path,
        default=Path("benchmark_results.csv"),
        help="Path to benchmark_results.csv (default: benchmark_results.csv)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("animations"),
        help="Directory to store animations (default: animations)",
    )
    parser.add_argument(
        "--format",
        choices=["mp4", "gif"],
        default="mp4",
        help="Animation output format (default: mp4)",
    )
    args = parser.parse_args()

    df = load_data(args.csv)

    latency_series, latency_frames = prepare_series(df, "Time_us")
    fig1, ax1 = plt.subplots(figsize=(10, 5))
    latency_anim = animate_plot(
        ax1,
        latency_series,
        frames=latency_frames,
        y_label="Time (microseconds)",
        title="Allreduce Latency (Lower is Better)",
        yscale="log",
    )
    latency_path = args.output_dir / f"latency_animation.{args.format}"
    save_animation(latency_anim, latency_path, args.format)
    plt.close(fig1)

    goodput_series, goodput_frames = prepare_series(df, "Goodput_Gbps")
    fig2, ax2 = plt.subplots(figsize=(10, 5))
    goodput_anim = animate_plot(
        ax2,
        goodput_series,
        frames=goodput_frames,
        y_label="Goodput (Gbps)",
        title="Allreduce Goodput (Higher is Better)",
        yscale="linear",
    )
    goodput_path = args.output_dir / f"goodput_animation.{args.format}"
    save_animation(goodput_anim, goodput_path, args.format)
    plt.close(fig2)


if __name__ == "__main__":
    main()

