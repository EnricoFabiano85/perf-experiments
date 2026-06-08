#!/usr/bin/env python3
"""Plot kernel mean time vs prefetch distance for the indirect_access benchmark.

Usage:
    python plot_prefetch.py results/prefetch_distance.csv results/prefetch_distance.png

Expected CSV columns (header row required):
    distance,scalar_ms,gather_ms
"""

import sys
import csv
import matplotlib.pyplot as plt

COL_DISTANCE = "distance"
COL_SCALAR = "scalar_ms"
COL_GATHER = "simd-gather_ms"


def read_csv(path):
    distance, scalar, gather = [], [], []
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            distance.append(float(row[COL_DISTANCE]))
            scalar.append(float(row[COL_SCALAR]))
            gather.append(float(row[COL_GATHER]))
    return distance, scalar, gather


def main():
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <input.csv> <output.png>", file=sys.stderr)
        sys.exit(1)

    csv_path, png_path = sys.argv[1], sys.argv[2]
    distance, scalar, gather = read_csv(csv_path)

    fig, ax = plt.subplots(figsize=(7, 5))

    ax.plot(distance, scalar, color="#1f77dd", linewidth=2.5, label="Scalar")
    ax.plot(distance, gather, color="black", linewidth=2.5, label="AVX512-gather")

    ax.set_xscale("log")
    ax.set_xlabel("Prefetch distance in elements", fontsize=14, fontweight="bold")
    ax.set_ylabel("Kernel mean time - ms", fontsize=14, fontweight="bold")

    ax.set_ylim(900, 1500)

    ax.tick_params(axis="both", labelsize=12)
    for label in ax.get_xticklabels() + ax.get_yticklabels():
        label.set_fontweight("bold")

    ax.legend(fontsize=12, loc="upper center")

    ax.grid(False)
    for spine in ax.spines.values():
        spine.set_linewidth(1.2)

    fig.tight_layout()
    fig.savefig(png_path, dpi=150)
    print(f"wrote {png_path}")


if __name__ == "__main__":
    main()
