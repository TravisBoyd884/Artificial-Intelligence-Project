#!/usr/bin/env bash
# Run the trained model in real-time demo mode.
#
# Usage:
#   ./demo.sh                         # best low model, 50 episodes
#   ./demo.sh medium                  # best medium model
#   ./demo.sh low 100                 # 100 episodes
#   ./demo.sh low 50 ppo_glitter_low  # specific model file

set -e
cd "$(dirname "$0")"

CONFIG="${1:-low}"
EPISODES="${2:-50}"
MODEL="${3:-ppo_glitter_${CONFIG}_best}"

# Fall back to the final model if no best checkpoint exists yet
if [[ ! -f "${MODEL}.zip" ]]; then
    FALLBACK="ppo_glitter_${CONFIG}"
    if [[ -f "${FALLBACK}.zip" ]]; then
        echo "No best checkpoint found (${MODEL}.zip), falling back to ${FALLBACK}.zip"
        MODEL="$FALLBACK"
    else
        echo "Error: no model found at ${MODEL}.zip or ${FALLBACK}.zip"
        echo "Run ./train.sh first."
        exit 1
    fi
fi

echo "Model: ${MODEL}.zip  |  Config: $CONFIG  |  Episodes: $EPISODES"
python -m rl.eval \
    --model    "$MODEL" \
    --binary   Build/Glitter/Glitter \
    --config   "$CONFIG" \
    --episodes "$EPISODES"
