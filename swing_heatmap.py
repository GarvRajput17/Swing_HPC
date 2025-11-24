#!/usr/bin/env python3
"""
Generate communication heatmaps for the Swing allreduce algorithm.

The heatmap summarizes the total bytes exchanged between every sender/receiver
pair by replaying the exact partner selection logic implemented in
`include/algorithms/SwingAlgorithm.h`.
"""

import argparse
import math
from pathlib import Path
from typing import Literal

import matplotlib.pyplot as plt
import numpy as np


Variant = Literal["bandwidth", "latency"]


def rho(step: int) -> int:
    """ρ(s) = Σ (i=0..s) (-2)^i."""
    total = 0
    for i in range(step + 1):
        total += (1 << i) if i % 2 == 0 else -(1 << i)
    return total


def pi(rank: int, step: int, world_size: int) -> int:
    """π(r,s) – communication partner for rank r at step s."""
    offset = rho(step)
    if rank % 2 == 0:
        return (rank + offset) % world_size
    return (rank - offset) % world_size


def swing_steps(world_size: int) -> int:
    if world_size < 2:
        return 0
    if world_size & (world_size - 1) == 0:
        return int(math.log2(world_size))
    return int(math.log2(world_size)) + 1


def accumulate_step(matrix: np.ndarray, world_size: int, step: int, bytes_per_send: float) -> None:
    for rank in range(world_size):
        peer = pi(rank, step, world_size)
        matrix[rank, peer] += bytes_per_send


def build_matrix(world_size: int, vector_size: int, variant: Variant) -> np.ndarray:
    matrix = np.zeros((world_size, world_size), dtype=float)
    steps = swing_steps(world_size)

    if variant == "bandwidth":
        for s in range(steps):
            data_size = vector_size / (1 << (s + 1))
            accumulate_step(matrix, world_size, s, data_size)
        for s in range(steps - 1, -1, -1):
            data_size = vector_size / (1 << (s + 1))
            accumulate_step(matrix, world_size, s, data_size)
    else:  # latency-optimal: full vector each step
        for s in range(steps):
            accumulate_step(matrix, world_size, s, vector_size)

    return matrix


def draw_heatmap(matrix: np.ndarray, title: str, output_path: Path) -> None:
    fig, ax = plt.subplots(figsize=(10, 9))
    im = ax.imshow(matrix, cmap="viridis")

    process_ids = np.arange(matrix.shape[0])
    ax.set_xticks(process_ids)
    ax.set_yticks(process_ids)
    ax.set_xlabel("Receiver Process ID")
    ax.set_ylabel("Sender Process ID")
    ax.set_title(title)

    vmax = matrix.max() if matrix.max() > 0 else 1.0
    for i in range(matrix.shape[0]):
        for j in range(matrix.shape[1]):
            value = matrix[i, j]
            if value == 0:
                label = "0"
            elif value.is_integer():
                label = f"{int(value)}"
            else:
                label = f"{value:.1f}"
            text_color = "white" if value > vmax * 0.45 else "black"
            ax.text(j, i, label, ha="center", va="center", color=text_color, fontsize=8)

    cbar = fig.colorbar(im, ax=ax)
    cbar.set_label("Communication Volume (Bytes)")

    fig.tight_layout()
    fig.savefig(output_path, dpi=300)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(description="Draw Swing allreduce communication heatmap.")
    parser.add_argument("--processes", type=int, default=16, help="Number of ranks (default: 16)")
    parser.add_argument("--vector-size", type=int, default=14400, help="Vector size in bytes (default: 14400)")
    parser.add_argument(
        "--variant",
        choices=["bandwidth", "latency"],
        default="bandwidth",
        help="Swing variant to visualize (default: bandwidth)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("swing_heatmap.png"),
        help="Output image path (default: swing_heatmap.png)",
    )
    args = parser.parse_args()

    matrix = build_matrix(args.processes, args.vector_size, args.variant)  # type: ignore[arg-type]
    title = f"Swing Allreduce ({args.variant.capitalize()}-Optimal): {args.processes} Processes"
    draw_heatmap(matrix, title, args.output)
    print(f"Saved heatmap to {args.output}")


if __name__ == "__main__":
    main()

