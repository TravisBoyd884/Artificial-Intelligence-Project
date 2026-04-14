#!/usr/bin/env bash
# Plot learning curves from a completed experiment.
#
# Usage:
#   ./plot.sh                  # save to results/plot.png and open it
#   ./plot.sh results/fig.png  # save to a specific path

set -e
cd "$(dirname "$0")"

OUT="${1:-results/plot.png}"
mkdir -p "$(dirname "$OUT")"

python -m rl.plot --save "$OUT"
echo "Opening $OUT"
xdg-open "$OUT" 2>/dev/null || open "$OUT" 2>/dev/null || echo "Open $OUT manually to view the plot."
