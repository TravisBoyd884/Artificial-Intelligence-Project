"""
Plot learning curves and visual complexity from a completed experiment.

Usage:
    python -m rl.plot                        # reads results/experiment.json
    python -m rl.plot --file results/my.json
    python -m rl.plot --save results/fig.png # save instead of showing
"""

import argparse
import json
import sys

import numpy as np

COLORS = {"low": "#4c9be8", "medium": "#f0a500", "high": "#e05c5c"}


def smooth(values, window=50):
    if len(values) < window:
        return np.array(values, dtype=float)
    kernel = np.ones(window) / window
    return np.convolve(values, kernel, mode="valid")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--file", default="results/experiment.json")
    p.add_argument("--save", default=None,
                   help="Save figure to this path instead of showing it")
    p.add_argument("--window", type=int, default=50,
                   help="Smoothing window for learning curves (episodes)")
    args = p.parse_args()

    try:
        import matplotlib.pyplot as plt
        import matplotlib.gridspec as gridspec
    except ImportError:
        print("matplotlib not installed.  Run:  pip install matplotlib")
        sys.exit(1)

    try:
        with open(args.file) as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Results file not found: {args.file}")
        print("Run ./experiment.sh first.")
        sys.exit(1)

    configs    = data["configs"]
    returns    = data["returns"]
    complexity = data["complexity"]

    fig = plt.figure(figsize=(13, 5))
    fig.suptitle("Visual Complexity vs. PPO Learning Speed", fontsize=13,
                 fontweight="bold")
    gs = gridspec.GridSpec(1, 3, figure=fig, width_ratios=[3, 3, 2],
                           wspace=0.35)

    # ------------------------------------------------------------------ #
    # Panel 1 – Learning curves (smoothed episode return)                 #
    # ------------------------------------------------------------------ #
    ax1 = fig.add_subplot(gs[0])
    ax1.set_title("Learning curves")
    ax1.set_xlabel("Episode")
    ax1.set_ylabel("Return (smoothed)")
    ax1.axhline(0, color="grey", linewidth=0.5, linestyle="--")

    for cfg in configs:
        r = returns.get(cfg, [])
        if not r:
            continue
        color = COLORS.get(cfg, "black")
        eps   = np.arange(len(r))
        ax1.plot(eps, r, alpha=0.15, color=color, linewidth=0.6)
        s = smooth(r, args.window)
        ax1.plot(np.arange(len(s)) + args.window // 2, s,
                 label=cfg, color=color, linewidth=2)

    ax1.legend(title="Config")

    # ------------------------------------------------------------------ #
    # Panel 2 – Score % over training                                     #
    # ------------------------------------------------------------------ #
    ax2 = fig.add_subplot(gs[1])
    ax2.set_title("Score % (smoothed)")
    ax2.set_xlabel("Episode")
    ax2.set_ylabel("% episodes scored")
    ax2.set_ylim(0, 105)

    for cfg in configs:
        r = returns.get(cfg, [])
        if not r:
            continue
        scored = [float(v > 0.5) * 100 for v in r]
        color  = COLORS.get(cfg, "black")
        s      = smooth(scored, args.window)
        ax2.plot(np.arange(len(s)) + args.window // 2, s,
                 label=cfg, color=color, linewidth=2)

    ax2.legend(title="Config")

    # ------------------------------------------------------------------ #
    # Panel 3 – Complexity vs. final score %                              #
    # ------------------------------------------------------------------ #
    ax3 = fig.add_subplot(gs[2])
    ax3.set_title("Complexity vs. final score")
    ax3.set_xlabel("Visual complexity (combined)")
    ax3.set_ylabel("Final score % (last 100 eps)")

    for cfg in configs:
        r = returns.get(cfg, [])
        c = complexity.get(cfg, {}).get("combined", 0)
        if not r:
            continue
        last100_pct = 100.0 * sum(v > 0.5 for v in r[-100:]) / min(len(r), 100)
        color = COLORS.get(cfg, "black")
        ax3.scatter([c], [last100_pct], color=color, s=120, zorder=5,
                    label=cfg)
        ax3.annotate(cfg, (c, last100_pct),
                     textcoords="offset points", xytext=(6, 4), fontsize=9)

    ax3.set_xlim(left=0)
    ax3.set_ylim(0, 105)
    ax3.legend(title="Config")

    # ------------------------------------------------------------------ #
    # Print summary table                                                  #
    # ------------------------------------------------------------------ #
    print(f"\n{'Config':<10} {'Complexity':>12} {'Episodes':>10} "
          f"{'Final score%':>14}")
    print("-" * 50)
    for cfg in configs:
        r = returns.get(cfg, [])
        c = complexity.get(cfg, {}).get("combined", float("nan"))
        if r:
            pct = 100.0 * sum(v > 0.5 for v in r[-100:]) / min(len(r), 100)
            print(f"{cfg:<10} {c:>12.4f} {len(r):>10} {pct:>13.1f}%")
        else:
            print(f"{cfg:<10} {c:>12.4f} {'—':>10} {'—':>14}")

    if args.save:
        plt.savefig(args.save, dpi=150, bbox_inches="tight")
        print(f"\nFigure saved to {args.save}")
    else:
        plt.tight_layout()
        plt.show()


if __name__ == "__main__":
    main()
