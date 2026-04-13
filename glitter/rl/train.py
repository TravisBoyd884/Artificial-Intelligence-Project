"""
Train a PPO agent on a single Glitter config.

Usage (binary already running):
    python -m rl.train --config low --steps 200000

Usage (auto-launch binary):
    python -m rl.train --config low --steps 200000 \
        --binary Build/Glitter/Glitter
"""

import argparse
import time

import numpy as np
from stable_baselines3 import PPO
from stable_baselines3.common.monitor import Monitor
from stable_baselines3.common.callbacks import BaseCallback, CallbackList

from rl.env import GlitterEnv


class BestModelCheckpoint(BaseCallback):
    """Saves the model whenever the windowed success rate hits a new peak."""

    def __init__(self, save_path: str, window: int = 200, threshold: float = 0.5):
        super().__init__(verbose=0)
        self.save_path = save_path
        self.window    = window
        self.threshold = threshold

        self._ep_reward  = 0.0
        self._recent     = []   # recent episode rewards
        self._best_pct   = -1.0

    def _on_step(self) -> bool:
        self._ep_reward += float(self.locals["rewards"][0])

        if self.locals["dones"][0]:
            self._recent.append(self._ep_reward)
            if len(self._recent) > self.window:
                self._recent.pop(0)
            self._ep_reward = 0.0

            if len(self._recent) == self.window:
                pct = 100.0 * sum(r > self.threshold for r in self._recent) / self.window
                if pct > self._best_pct:
                    self._best_pct = pct
                    self.model.save(self.save_path)
                    print(f"  *** new best {pct:.1f}% — saved to {self.save_path}.zip ***")

        return True


class TrainingStats(BaseCallback):
    """Prints a one-line progress update after every `interval` episodes."""

    def __init__(self, total_steps: int, interval: int = 200):
        super().__init__(verbose=0)
        self.total_steps = total_steps
        self.interval    = interval

        self._ep_count   = 0
        self._ep_reward  = 0.0
        self._window     = []   # recent episode rewards
        self._t0         = None

    def _on_training_start(self):
        self._t0 = time.monotonic()
        print(f"\n{'Step':>8}  {'Episode':>8}  {'Success%':>9}  "
              f"{'MeanRew':>9}  {'Elapsed':>8}")
        print("-" * 54)

    def _on_step(self) -> bool:
        self._ep_reward += float(self.locals["rewards"][0])

        if self.locals["dones"][0]:
            self._ep_count += 1
            self._window.append(self._ep_reward)
            if len(self._window) > self.interval:
                self._window.pop(0)
            self._ep_reward = 0.0

            if self._ep_count % self.interval == 0:
                recent      = self._window
                success_pct = 100.0 * sum(r > 0.5 for r in recent) / len(recent)
                mean_rew    = float(np.mean(recent))
                elapsed     = time.monotonic() - self._t0
                pct_done    = 100.0 * self.num_timesteps / self.total_steps
                bar_len     = 20
                filled      = int(bar_len * pct_done / 100)
                bar         = "█" * filled + "░" * (bar_len - filled)
                print(f"[{bar}] {pct_done:5.1f}%  "
                      f"ep {self._ep_count:6d}  "
                      f"score {success_pct:5.1f}%  "
                      f"rew {mean_rew:+.3f}  "
                      f"{elapsed:6.0f}s")

        return True


def make_env(config, binary, render, fixed_spawn):
    return Monitor(GlitterEnv(
        config=config,
        binary_path=binary,
        render=render,
        fixed_spawn=fixed_spawn,
    ))


def make_model(env, total_steps):
    return PPO(
        "CnnPolicy", env,
        verbose=0,
        ent_coef=0.02,
    )


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--config",  default="low",
                   choices=["low", "medium", "high"])
    p.add_argument("--steps",   type=int, default=200_000)
    p.add_argument("--binary",  default=None,
                   help="Path to Glitter binary for auto-launch")
    p.add_argument("--render",  action="store_true",
                   help="Enable real-time rendering (slows training)")
    p.add_argument("--out",     default=None,
                   help="Output model path (default: ppo_glitter_<config>)")
    p.add_argument("--curriculum", action="store_true",
                   help="Two-phase training: fixed spawn then random spawn")
    p.add_argument("--phase1-frac", type=float, default=0.25,
                   help="Fraction of steps for phase 1 (default 0.25)")
    args = p.parse_args()

    out  = args.out or f"ppo_glitter_{args.config}"
    best = out + "_best"

    if args.curriculum:
        phase1_steps = int(args.steps * args.phase1_frac)
        phase2_steps = args.steps - phase1_steps

        print(f"\n--- phase 1: fixed spawn ({phase1_steps:,} steps) ---")
        env1 = make_env(args.config, args.binary, args.render, fixed_spawn=True)
        model = make_model(env1, args.steps)
        cb1 = CallbackList([
            TrainingStats(total_steps=phase1_steps),
            BestModelCheckpoint(save_path=best, threshold=0.5),
        ])
        model.learn(total_timesteps=phase1_steps, callback=cb1)
        env1.close()

        print(f"\n--- phase 2: random spawn ({phase2_steps:,} steps) ---")
        env2 = make_env(args.config, args.binary, args.render, fixed_spawn=False)
        model.set_env(env2)
        cb2 = CallbackList([
            TrainingStats(total_steps=phase2_steps),
            BestModelCheckpoint(save_path=best, threshold=0.5),
        ])
        model.learn(total_timesteps=phase2_steps, callback=cb2, reset_num_timesteps=False)
        env2.close()

    else:
        env = make_env(args.config, args.binary, args.render, fixed_spawn=False)
        model = make_model(env, args.steps)
        cb = CallbackList([
            TrainingStats(total_steps=args.steps),
            BestModelCheckpoint(save_path=best),
        ])
        print(f"Training on '{args.config}' for {args.steps:,} steps → {out}.zip")
        model.learn(total_timesteps=args.steps, callback=cb)
        env.close()

    model.save(out)
    print(f"\nSaved final → {out}.zip")
    print(f"Best model  → {best}.zip")


if __name__ == "__main__":
    main()
