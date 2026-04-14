"""
Run a trained policy against the environment in real-time.

Usage:
    python -m rl.eval --model ppo_glitter_low_best \
                      --binary Build/Glitter/Glitter \
                      --config low
"""

import argparse
import numpy as np
from stable_baselines3 import PPO
from rl.env import GlitterEnv


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--model",    required=True,
                   help="Path to saved model (.zip or without extension)")
    p.add_argument("--config",   default="low",
                   choices=["low", "medium", "high"])
    p.add_argument("--binary",   default=None)
    p.add_argument("--episodes", type=int, default=50)
    args = p.parse_args()

    env   = GlitterEnv(config=args.config, binary_path=args.binary, render=True)
    model = PPO.load(args.model, env=env)

    scores, total = 0, 0
    obs, _ = env.reset()

    print(f"Running {args.episodes} episodes with '{args.model}' on '{args.config}'")
    print(f"{'Ep':>4}  {'Result':>8}  {'Score%':>7}")
    print("-" * 28)

    while total < args.episodes:
        action, _ = model.predict(obs, deterministic=True)
        obs, reward, terminated, truncated, _ = env.step(action)

        if terminated or truncated:
            total  += 1
            scored  = reward > 0.5
            if scored:
                scores += 1
            pct = 100.0 * scores / total
            print(f"{total:>4}  {'SCORE' if scored else 'miss':>8}  {pct:>6.1f}%")
            obs, _ = env.reset()

    env.close()
    print(f"\nFinal: {scores}/{total} shots scored ({100*scores/total:.1f}%)")


if __name__ == "__main__":
    main()
