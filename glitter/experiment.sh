#!/usr/bin/env bash
# Run the full research experiment sweep (low / medium / high).
# Results are written to results/experiment.json when finished.
# Each config uses curriculum: 25% fixed spawn, then 75% random spawn.
#
# Usage:
#   ./experiment.sh                          # all three configs, 500k steps each
#   ./experiment.sh 500000                   # explicit step count
#   ./experiment.sh 500000 medium high       # train medium+high, eval low from saved model

set -e
cd "$(dirname "$0")"

STEPS="${1:-500000}"
shift 1 2>/dev/null || true

# Any remaining args are configs to TRAIN. Configs not listed that have a
# saved _best model are automatically evaluated instead of retrained.
TRAIN_CONFIGS=("$@")
EVAL_ARGS=()
ALL_CONFIGS=("low" "medium" "high")

if [ ${#TRAIN_CONFIGS[@]} -gt 0 ]; then
    for cfg in "${ALL_CONFIGS[@]}"; do
        skip=false
        for tc in "${TRAIN_CONFIGS[@]}"; do
            [ "$cfg" = "$tc" ] && skip=true
        done
        if [ "$skip" = false ]; then
            model="ppo_glitter_${cfg}_best"
            if [ -f "${model}.zip" ]; then
                EVAL_ARGS+=("--eval" "${cfg}=${model}")
            fi
        fi
    done
    python -m rl.experiment --steps "$STEPS" --binary Build/Glitter/Glitter \
        --configs "${TRAIN_CONFIGS[@]}" "${EVAL_ARGS[@]}"
else
    python -m rl.experiment --steps "$STEPS" --binary Build/Glitter/Glitter
fi
