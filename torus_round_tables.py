#!/usr/bin/env python3
"""
Visualize reduce-scatter and allgather rounds on a 1D torus (ring).

For each round we render a PNG that combines:
  • a mini torus diagram with arrows showing the send direction and chunk id
  • a table summarizing, per process, who it talks to, how many bytes move,
    which chunk travels, and what data the process holds afterwards

The simulation follows the ring allreduce schedule (reduce-scatter followed
by allgather) on a small 1D torus.
"""

from __future__ import annotations

import argparse
import copy
from pathlib import Path
from typing import Dict, List, Tuple

import matplotlib.pyplot as plt
import numpy as np


def human_bytes(num_bytes: int) -> str:
    units = ["B", "KB", "MB", "GB"]
    value = float(num_bytes)
    unit = 0
    while value >= 1024 and unit < len(units) - 1:
        value /= 1024.0
        unit += 1
    return f"{value:.0f}{units[unit]}"


def format_chunk(chunk: Dict) -> str:
    contributors = "+".join(f"P{p}" for p in sorted(chunk["contributors"]))
    return f"C{chunk['idx']}[{contributors}]"


def summarize_holdings(holdings: Dict[int, Dict]) -> str:
    ordered = [holdings[idx] for idx in sorted(holdings.keys())]
    return "\n".join(format_chunk(ch) for ch in ordered)


def draw_ring(ax, num_procs: int, messages: List[Tuple[int, int, str, str]]) -> None:
    xs = np.arange(num_procs)
    line_start = xs[0] - 0.18
    line_end = xs[-1] + 0.18
    ax.plot(
        [line_start, line_end],
        [0, 0],
        color="#111111",
        linewidth=6,
        alpha=0.75,
        solid_capstyle="round",
    )

    for r, x in enumerate(xs):
        ax.add_patch(plt.Circle((x, 0), 0.18, color="#2e8b57"))
        ax.text(x, 0, f"P{r}", ha="center", va="center", color="white", fontsize=11)

    for sender, dest, label, color in messages:
        start = sender
        end = dest
        if end == 0 and sender == num_procs - 1:
            # wrap arrow over top
            ax.annotate(
                "",
                xy=(end, 0.25),
                xytext=(start, 0.25),
                arrowprops=dict(arrowstyle="-|>", color=color, linewidth=2.4),
            )
            ax.text(
                (start + end) / 2.0 + 0.2,
                0.4,
                label,
                ha="center",
                va="bottom",
                fontsize=10,
                color=color,
            )
        else:
            ax.annotate(
                "",
                xy=(end, 0.15),
                xytext=(start, 0.15),
                arrowprops=dict(arrowstyle="-|>", color=color, linewidth=2.4),
            )
            ax.text(
                (start + end) / 2.0,
                0.28,
                label,
                ha="center",
                va="bottom",
                fontsize=10,
                color=color,
            )

    ax.set_xlim(-0.5, num_procs - 0.5)
    ax.set_ylim(-0.2, 0.7)
    ax.axis("off")


def render_step(step_idx: int, rows: List[Dict], phase: str, num_procs: int, output_dir: Path) -> None:
    color_palette = plt.cm.tab10(np.linspace(0, 1, num_procs))
    table_rows = []
    messages = []
    for row in rows:
        chunk_index = int(row["send_chunk"][1:])
        chunk_color = color_palette[chunk_index % len(color_palette)]
        table_rows.append(
            [
                f"P{row['process']}",
                f"P{row['send_to']}",
                f"{row['send_chunk']} ({row['send_label']})",
                human_bytes(row['bytes_sent']),
                f"P{row['recv_from']}",
                f"{row['recv_chunk']} ({row['recv_label']})",
                human_bytes(row['bytes_recv']),
                row['data_held'],
            ]
        )
        messages.append((row["process"], row["send_to"], row["send_chunk"], chunk_color))

    fig = plt.figure(figsize=(14, 5 + num_procs * 0.4))
    fig.patch.set_facecolor("#fdf8f1")
    gs = fig.add_gridspec(2, 1, height_ratios=[1.3, max(2, num_procs * 0.45)])
    ax_ring = fig.add_subplot(gs[0])
    ax_table = fig.add_subplot(gs[1])
    ax_table.axis("off")

    draw_ring(ax_ring, num_procs, messages)
    ax_ring.set_title(f"{phase} – Round {step_idx}", fontsize=14, fontweight="bold")

    col_labels = [
        "Process",
        "Send →",
        "Chunk Sent",
        "Bytes Sent",
        "Receive ←",
        "Chunk Received",
        "Bytes Recv",
        "Data Held After",
    ]

    table = ax_table.table(
        cellText=table_rows,
        colLabels=col_labels,
        loc="center",
        cellLoc="center",
    )
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1.3, 1.6)

    output_dir.mkdir(parents=True, exist_ok=True)
    outfile = output_dir / f"{phase.lower().replace(' ', '_')}_round_{step_idx}.png"
    fig.tight_layout()
    fig.savefig(outfile, dpi=300)
    plt.close(fig)
    print(f"Saved {outfile}")


def simulate_reduce_scatter(num_procs: int, vector_size: int, output_dir: Path):
    chunk_bytes = vector_size // num_procs
    holdings = {r: {r: {"idx": r, "contributors": {r}}} for r in range(num_procs)}
    last_received = {r: r for r in range(num_procs)}

    for step in range(num_procs - 1):
        transmissions: Dict[int, Dict] = {}
        rows: List[Dict] = []

        for r in range(num_procs):
            send_to = (r + 1) % num_procs
            recv_from = (r - 1 + num_procs) % num_procs
            send_idx = (r - step) % num_procs
            chunk = holdings[r].pop(send_idx)
            transmissions[send_to] = {
                "sender": r,
                "chunk_idx": send_idx,
                "chunk": chunk,
            }
            rows.append(
                {
                    "process": r,
                    "send_to": send_to,
                    "recv_from": recv_from,
                    "send_chunk": f"C{send_idx}",
                    "send_label": format_chunk(chunk),
                    "bytes_sent": chunk_bytes,
                }
            )

        for row in rows:
            r = row["process"]
            recv_idx = (r - step - 1) % num_procs
            incoming = transmissions[r]
            chunk = incoming["chunk"]
            chunk["contributors"].add(r)
            holdings[r][recv_idx] = chunk
            last_received[r] = recv_idx

            row.update(
                {
                    "recv_chunk": f"C{recv_idx}",
                    "recv_label": format_chunk(chunk),
                    "bytes_recv": chunk_bytes,
                    "data_held": summarize_holdings(holdings[r]),
                }
            )

        render_step(step, rows, "Reduce-Scatter", num_procs, output_dir)

    return holdings, last_received


def simulate_allgather(
    num_procs: int,
    vector_size: int,
    output_dir: Path,
    initial_holdings: Dict[int, Dict[int, Dict]],
    last_received: Dict[int, int],
):
    chunk_bytes = vector_size // num_procs
    holdings = {
        r: {idx: copy.deepcopy(chunk) for idx, chunk in chunks.items()}
        for r, chunks in initial_holdings.items()
    }

    for step in range(num_procs - 1):
        transmissions: Dict[int, Dict] = {}
        rows: List[Dict] = []

        for r in range(num_procs):
            send_to = (r + 1) % num_procs
            recv_from = (r - 1 + num_procs) % num_procs
            send_idx = last_received[r]
            chunk = holdings[r][send_idx]

            transmissions[send_to] = {
                "sender": r,
                "chunk_idx": send_idx,
                "chunk": copy.deepcopy(chunk),
            }
            rows.append(
                {
                    "process": r,
                    "send_to": send_to,
                    "recv_from": recv_from,
                    "send_chunk": f"C{send_idx}",
                    "send_label": format_chunk(chunk),
                    "bytes_sent": chunk_bytes,
                }
            )

        for row in rows:
            r = row["process"]
            incoming = transmissions[r]
            recv_chunk = incoming["chunk"]
            recv_idx = incoming["chunk_idx"]

            if recv_idx not in holdings[r]:
                holdings[r][recv_idx] = recv_chunk
            last_received[r] = recv_idx

            row.update(
                {
                    "recv_chunk": f"C{recv_idx}",
                    "recv_label": format_chunk(recv_chunk),
                    "bytes_recv": chunk_bytes,
                    "data_held": summarize_holdings(holdings[r]),
                }
            )

        render_step(step, rows, "Allgather", num_procs, output_dir)


def main():
    parser = argparse.ArgumentParser(description="Render 1D torus communication tables per round.")
    parser.add_argument("--processes", type=int, default=8, help="Number of torus processes (default: 8)")
    parser.add_argument(
        "--vector-size",
        type=int,
        default=16384,
        help="Vector size in bytes (default: 16384)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("torus_rounds"),
        help="Directory for generated PNGs (default: torus_rounds)",
    )
    args = parser.parse_args()

    holdings, last_recv = simulate_reduce_scatter(args.processes, args.vector_size, args.output_dir)
    simulate_allgather(args.processes, args.vector_size, args.output_dir, holdings, last_recv)


if __name__ == "__main__":
    main()

