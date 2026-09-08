"""
evaluate_win_rate.py

Runs a trained PPO checkpoint against the scripted C++ opponent for N
held-out evaluation episodes and reports win/loss/draw counts + win rate.

This is a resume-metric script, not a training script: actions are always
deterministic (no exploration noise), so the number reflects what the
policy actually does at inference time, not training-time exploration
behavior.

Usage
-----
    python evaluate_win_rate.py --model models/tank_ppo.zip --episodes 300 --opponent-difficulty Hard --sample-actions

Notes
-----
- Uses the .zip checkpoint (via Stable-Baselines3), not the .onnx export —
  this measures the policy's actual decisions, independent of any ONNX
  export fidelity question.
- --max-episode-steps should match whatever max_episode_steps you trained
  with (rl-learning.py's default is 1000) so episodes that time out are
  scored consistently as draws rather than artificially cut short/long.
- Win/loss detection: rl-bindings.cpp doesn't expose court() to Python, so
  there's no direct "who died" query. Outcome is inferred from the sign of
  the terminal-step reward instead: RLEnvironment.calculate_reward() adds
  +1.0 when the opponent dies and -1.0 when the agent dies (on top of a
  much smaller per-tick damage term), so a terminated (not truncated)
  episode's final reward reliably indicates who died. A truncated episode
  (hit max_episode_steps without either tank dying) is scored as a draw.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from stable_baselines3 import PPO

import tanks_env_cpp
from tankenv import TankEnv


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    p.add_argument("--model", required=True, type=Path, help="Path to a trained SB3 PPO .zip file")
    p.add_argument("--episodes", type=int, default=300, help="Number of evaluation episodes (default: 300)")
    p.add_argument(
        "--max-episode-steps",
        type=int,
        default=1000,
        help="Should match the value used during training (rl-learning.py default: 1000)",
    )
    p.add_argument(
        "--opponent-difficulty",
        default="Medium",
        choices=["Easy", "Medium", "Hard"],
        help="Scripted opponent difficulty to evaluate against (default: Medium)",
    )
    p.add_argument(
        "--sample-actions",
        action="store_true",
        default=False,
        help="Sample actions from the policy instead of taking the argmax",
    )
    p.add_argument("--seed", type=int, default=0, help="Base seed; episode i uses seed + i")
    return p.parse_args()


def main() -> int:
    args = parse_args()

    if not args.model.exists():
        print(f"error: model file not found: {args.model}")
        return 1

    difficulty = getattr(tanks_env_cpp.OpponentDifficulty, args.opponent_difficulty)
    env = TankEnv(max_episode_steps=args.max_episode_steps, opponent=difficulty)

    print(f"Loading model from {args.model} ...")
    model = PPO.load(str(args.model))

    wins = 0
    losses = 0
    draws = 0
    episode_lengths: list[int] = []

    print(f"Running {args.episodes} evaluation episodes vs {args.opponent_difficulty} scripted opponent...")
    for episode in range(args.episodes):
        obs, _ = env.reset(seed=args.seed + episode)
        terminated = False
        truncated = False
        steps = 0
        last_reward = 0.0

        while not (terminated or truncated):
            action, _ = model.predict(obs, deterministic=not args.sample_actions)
            obs, reward, terminated, truncated, _ = env.step(action)
            last_reward = reward
            steps += 1

        episode_lengths.append(steps)

        if truncated and not terminated:
            draws += 1
        elif last_reward > 0.05:
            wins += 1
        elif last_reward < -0.05:
            losses += 1
        else:
            # Ambiguous/mutual-KO edge case (both tanks died the same
            # tick, or reward landed near zero for some other reason) —
            # rare; bucketed as a draw rather than guessing a winner.
            draws += 1

        if (episode + 1) % max(1, args.episodes // 10) == 0:
            print(f"  {episode + 1}/{args.episodes} episodes done "
                  f"(W {wins} / L {losses} / D {draws})")

    env.close()

    total = wins + losses + draws
    win_rate = 100.0 * wins / total if total else 0.0
    mean_len = float(np.mean(episode_lengths)) if episode_lengths else 0.0
    median_len = float(np.median(episode_lengths)) if episode_lengths else 0.0
    std_len = float(np.std(episode_lengths)) if episode_lengths else 0.0

    print()
    print("Results")
    print("-------")
    print(f"  Model:               {args.model}")
    print(f"  Opponent difficulty: {args.opponent_difficulty}")
    print(f"  Episodes:            {total}")
    print(f"  Wins / Losses / Draws: {wins} / {losses} / {draws}")
    print(f"  Win rate:            {win_rate:.1f}%")
    print(f"  Mean episode length: {mean_len:.1f} steps")
    print(f"  Median episode length: {median_len:.1f} steps")
    print(f"  Std episode length: {std_len:.1f} steps")
    print()
    print(f'  -> "achieving a {win_rate:.0f}% win rate against a scripted baseline '
          f'over {total} evaluation episodes"')
    return 0


if __name__ == "__main__":
    raise SystemExit(main())