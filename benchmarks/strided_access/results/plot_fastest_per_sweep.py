#!/usr/bin/env python3
"""Plot fastest loop for each sweep direction vs grid size N.

    X-sweep -> k-j-i,  Y-sweep -> k-j-i,  Z-sweep -> j-k-i

Reads every strided_access_<N>.csv in this folder.
"""
import csv
import glob
import os
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))

REPRESENTATIVE = {"X-sweep": "k-j-i", "Y-sweep": "k-j-i", "Z-sweep": "j-k-i"}

EXCLUDE = ("pencil",)

def load_rows():
    rows = []
    for path in sorted(glob.glob(os.path.join(HERE, "strided_access_*.csv"))):
        with open(path, newline="") as f:
            for r in csv.DictReader(f, skipinitialspace=True):
                if any(x in r["Kernel"] for x in EXCLUDE):
                    continue
                sweep, ordering = r["Kernel"].split(maxsplit=1)
                rows.append({
                    "sweep": sweep,
                    "ordering": ordering,
                    "N": int(r["N"]),
                    "mean": float(r["mean"]),
                    "std": float(r["std-dev"]),
                })
    if not rows:
        raise SystemExit("no strided_access_*.csv files found in " + HERE)
    return rows


def warn_if_beaten(rows):
    by_key = defaultdict(list)  # (sweep, N) -> [rows]
    for r in rows:
        by_key[(r["sweep"], r["N"])].append(r)
    for (sweep, N), group in sorted(by_key.items()):
        rep = REPRESENTATIVE.get(sweep)
        rep_row = next((g for g in group if g["ordering"] == rep), None)
        if rep_row is None:
            print(f"[warn] {sweep} N={N}: representative '{rep}' not measured")
            continue
        best = min(group, key=lambda g: g["mean"])
        if best["ordering"] != rep and best["mean"] < rep_row["mean"]:
            print(f"[warn] {sweep} N={N}: {best['ordering']} ({best['mean']:.1f} ms) "
                  f"beat plotted {rep} ({rep_row['mean']:.1f} ms)")


def main():
    rows = load_rows()
    warn_if_beaten(rows)

    series = defaultdict(list)  # sweep -> [(N, mean, std)]
    for r in rows:
        if r["ordering"] == REPRESENTATIVE.get(r["sweep"]):
            series[r["sweep"]].append((r["N"], r["mean"], r["std"]))
    for s in series.values():
        s.sort()

    print(f"\n{'sweep':8} {'kernel':8} {'N':>6} {'mean[ms]':>10} {'std':>8}")
    for sweep in sorted(series):
        for N, mean, std in series[sweep]:
            print(f"{sweep:8} {REPRESENTATIVE[sweep]:8} {N:6d} {mean:10.1f} {std:8.2f}")

    fig, ax = plt.subplots(figsize=(7, 5))
    for sweep in sorted(series):
        Ns, means, stds = zip(*series[sweep])
        ax.errorbar(Ns, means, yerr=stds, marker="o", capsize=3,
                    label=f"{sweep} ({REPRESENTATIVE[sweep]})")

    ax.set_xlabel("Grid size (N x N x N)")
    ax.set_ylabel("Mean time [ms]")
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.grid(True, which="both", alpha=0.3)
    ax.legend()
    ax.set_title("Kernel wall-clock time vs grid size")
    fig.tight_layout()
    out = os.path.join(HERE, "grid_sensitivity.png")
    fig.savefig(out, dpi=150)


if __name__ == "__main__":
    main()
