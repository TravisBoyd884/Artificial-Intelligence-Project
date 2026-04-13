#!/usr/bin/env bash
# Train a PPO agent on a Glitter config.
#
# Usage:
#   ./train.sh                        # low config, 500k steps, curriculum
#   ./train.sh medium                 # medium config
#   ./train.sh high 1000000           # high config, 1M steps
#   ./train.sh low 500000 --render    # watch training in real-time (slower)
#   ./train.sh low 500000 --no-curriculum  # skip phase 1 (not recommended)
#
# Curriculum (default): Phase 1 trains from a fixed spawn until the agent
# learns to shoot, then Phase 2 introduces random spawns so the agent must
# read the visual observation.  Use --phase1-frac N (0-1) to tune the split.

set -e
cd "$(dirname "$0")"

CONFIG="${1:-low}"
STEPS="${2:-500000}"
shift 2 2>/dev/null || true   # consume first two args; remaining flags pass through

# Detect if the caller explicitly disabled curriculum
CURRICULUM="--curriculum"
for arg in "$@"; do
    if [ "$arg" = "--no-curriculum" ]; then
        CURRICULUM=""
    fi
done
# Filter out --no-curriculum so it isn't forwarded to Python
EXTRA=()
for arg in "$@"; do
    [ "$arg" != "--no-curriculum" ] && EXTRA+=("$arg")
done

python -m rl.train \
    --binary Build/Glitter/Glitter \
    --config "$CONFIG" \
    --steps  "$STEPS" \
    $CURRICULUM \
    "${EXTRA[@]}"
