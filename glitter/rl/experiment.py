"""
    python -m rl.experiment --binary Build/Glitter/Glitter --steps 50000

Results are written to results/experiment.json.
Run rl/plot.py afterwards to visualise.
"""

import argparse
import json
import os
import time

import numpy as np
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import BaseCallback, CallbackList
from stable_baselines3.common.monitor import Monitor

from rl.complexity import measure_config
from rl.env import GlitterEnv
from rl.train import BestModelCheckpoint, make_env, make_model

CONFIGS = ["low", "medium", "high"]


class _ReturnLogger(BaseCallback):
    def __init__(self):
        super().__init__(verbose=0)
        self.episode_returns = []
        self._ep_reward = 0.0

    def _on_step(self) -> bool:
        self._ep_reward += float(self.locals["rewards"][0])
        if self.locals["dones"][0]:
            self.episode_returns.append(self._ep_reward)
            self._ep_reward = 0.0
        return True


def run_config(config: str, binary: str, steps: int) -> tuple:
    """
    Train PPO on `config` with curriculum, return (episode_returns, complexity_dict).
    Phase 1 (25% of steps): fixed spawn to bootstrap shooting skill.
    Phase 2 (75% of steps): random spawn to learn visual generalisation.
    """
    # Measure complexity using the fixed-spawn observation (consistent baseline)
    env_tmp = Monitor(make_env(config, binary, render=False, fixed_spawn=True))
    print(f"  Measuring complexity for '{config}'...")
    complexity = measure_config(env_tmp, n_frames=16)
    env_tmp.close()
    print(
        f"  edge_density={complexity['edge_density']:.4f}  "
        f"entropy={complexity['entropy']:.3f} bits  "
        f"combined={complexity['combined']:.4f}"
    )

    save_path = f"ppo_glitter_{config}_best"
    phase1_steps = steps // 4
    phase2_steps = steps - phase1_steps
    logger = _ReturnLogger()

    # ── Phase 1: fixed spawn ─────────────────────────────────────────────
    print(f"  Phase 1 — fixed spawn ({phase1_steps:,} steps)...")
    env1 = Monitor(make_env(config, binary, render=False, fixed_spawn=True))
    model = make_model(env1, steps)
    t0 = time.monotonic()
    model.learn(
        total_timesteps=phase1_steps,
        callback=CallbackList([logger, BestModelCheckpoint(save_path)]),
    )
    env1.close()

    # ── Phase 2: random spawn ────────────────────────────────────────────
    print(f"  Phase 2 — random spawn ({phase2_steps:,} steps)...")
    env2 = Monitor(make_env(config, binary, render=False, fixed_spawn=False))
    model.set_env(env2)
    model.learn(
        total_timesteps=phase2_steps,
        callback=CallbackList([logger, BestModelCheckpoint(save_path)]),
        reset_num_timesteps=False,
    )
    env2.close()

    elapsed = time.monotonic() - t0
    n_eps = len(logger.episode_returns)
    if n_eps:
        last100 = logger.episode_returns[-100:]
        print(
            f"  Done in {elapsed:.0f}s — {n_eps} episodes, "
            f"mean(last 100)={np.mean(last100):.3f}, "
            f"max={max(logger.episode_returns):.3f}"
        )
    else:
        print(f"  Done in {elapsed:.0f}s — no completed episodes recorded")

    return logger.episode_returns, complexity


def eval_config(
    config: str, binary: str, model_path: str, n_episodes: int = 1000
) -> tuple:
    """
    Load a pre-trained model and evaluate it for n_episodes, returning
    (episode_returns, complexity_dict). Use this instead of run_config when
    a model was already trained via train.sh.
    """
    env = Monitor(make_env(config, binary, render=False, fixed_spawn=False))

    print(f"  Measuring complexity for '{config}'...")
    complexity = measure_config(env, n_frames=16)
    print(
        f"  edge_density={complexity['edge_density']:.4f}  "
        f"entropy={complexity['entropy']:.3f} bits  "
        f"combined={complexity['combined']:.4f}"
    )

    print(f"  Loading {model_path} and evaluating for {n_episodes} episodes...")
    model = PPO.load(model_path, env=env)
    returns = []
    obs, _ = env.reset()
    ep_ret = 0.0
    while len(returns) < n_episodes:
        action, _ = model.predict(obs, deterministic=True)
        obs, reward, done, _, _ = env.step(action)
        ep_ret += reward
        if done:
            returns.append(ep_ret)
            ep_ret = 0.0
            obs, _ = env.reset()

    env.close()
    print(
        f"  Evaluated {n_episodes} episodes — "
        f"mean={np.mean(returns):.3f}  "
        f"success={100*sum(r>0.5 for r in returns)/len(returns):.1f}%"
    )
    return returns, complexity


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--binary", required=True, help="Path to compiled Glitter binary")
    p.add_argument(
        "--steps", type=int, default=500_000, help="Training steps per config"
    )
    p.add_argument(
        "--configs",
        nargs="+",
        default=CONFIGS,
        choices=CONFIGS,
        help="Which configs to train (default: all three). "
        "e.g. --configs medium high  to skip low",
    )
    p.add_argument(
        "--eval",
        nargs="+",
        default=[],
        metavar="CONFIG=MODEL_PATH",
        help="Evaluate a pre-trained model instead of training. "
        "e.g. --eval low=ppo_glitter_low_best",
    )
    args = p.parse_args()

    # Parse --eval entries into a dict: {config: model_path}
    eval_map = {}
    for entry in args.eval:
        cfg, _, model_path = entry.partition("=")
        if cfg not in CONFIGS:
            p.error(f"Unknown config in --eval: {cfg!r}")
        eval_map[cfg] = model_path

    os.makedirs("results", exist_ok=True)

    path = "results/experiment.json"
    if os.path.exists(path):
        with open(path) as f:
            existing = json.load(f)
        all_returns = existing.get("returns", {})
        all_complexity = existing.get("complexity", {})
    else:
        all_returns = {}
        all_complexity = {}

    for cfg, model_path in eval_map.items():
        print(f"\n=== Config: {cfg} (eval from {model_path}) ===")
        returns, complexity = eval_config(cfg, args.binary, model_path)
        all_returns[cfg] = returns
        all_complexity[cfg] = complexity

    for cfg in args.configs:
        print(f"\n=== Config: {cfg} ===")
        returns, complexity = run_config(cfg, args.binary, args.steps)
        all_returns[cfg] = returns
        all_complexity[cfg] = complexity

    out = {
        "configs": CONFIGS,
        "steps": args.steps,
        "returns": all_returns,
        "complexity": all_complexity,
    }

    with open(path, "w") as f:
        json.dump(out, f, indent=2)
    print(f"\nResults saved to {path}")

    print("\n=== Summary ===")
    print(
        f"{'Config':<10} {'Episodes':>10} {'Mean(last100)':>14} "
        f"{'Max':>8} {'Complexity':>12}"
    )
    for cfg in CONFIGS:
        r = all_returns.get(cfg, [])
        c = all_complexity.get(cfg, {}).get("combined", float("nan"))
        if r:
            print(
                f"{cfg:<10} {len(r):>10} {np.mean(r[-100:]):>14.3f} "
                f"{max(r):>8.3f} {c:>12.4f}"
            )
        else:
            print(f"{cfg:<10} {'0':>10} {'n/a':>14} {'n/a':>8} {c:>12.4f}")


if __name__ == "__main__":
    main()
