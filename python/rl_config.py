"""Training hyperparameters and play-mode difficulty registry (no C++ deps)."""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass, field
from typing import List


# Play-mode registry. Paths fill in as you export checkpoints / ONNX later.
# easy   — scripted baseline (or earliest checkpoint)
# medium — mid-training checkpoint
# hard   — latest / strongest checkpoint
DIFFICULTY_MODES = {
    "easy": {
        "description": "Scripted opponent (easy preset) or earliest policy",
        "opponent": "scripted",
        "scripted_level": "Easy",
        "model_path": "models/tank_ppo_easy",
    },
    "medium": {
        "description": "Mid-strength learned policy",
        "opponent": "checkpoint",
        "scripted_level": "Medium",
        "model_path": "models/tank_ppo_medium",
    },
    "hard": {
        "description": "Latest / strongest learned policy",
        "opponent": "checkpoint",
        "scripted_level": "Hard",
        "model_path": "models/tank_ppo_hard",
    },
}


@dataclass
class TrainHyperParams:
    learning_rate: float = 3e-4
    n_steps: int = 2048
    batch_size: int = 64
    n_epochs: int = 10
    gamma: float = 0.99
    gae_lambda: float = 0.95
    clip_range: float = 0.2
    ent_coef: float = 0.01
    vf_coef: float = 0.5
    max_grad_norm: float = 0.5
    net_arch: List[int] = field(default_factory=lambda: [128, 128])
    total_timesteps: int = 100_000
    max_episode_steps: int = 1000
    seed: int = 0
    opponent_difficulty: str = "Medium"
    model_name: str = "tank_ppo"
    tensorboard_log: str = "./tensorboard/"
    skip_env_check: bool = False

    def to_dict(self) -> dict:
        return asdict(self)


def parse_net_arch(text: str) -> List[int]:
    parts = [p.strip() for p in text.split(",") if p.strip()]
    if not parts:
        raise argparse.ArgumentTypeError("net_arch must be like 128,128")
    return [int(p) for p in parts]


def build_arg_parser() -> argparse.ArgumentParser:
    defaults = TrainHyperParams()
    p = argparse.ArgumentParser(description="Train PPO on TankEnv")
    p.add_argument("--learning-rate", type=float, default=defaults.learning_rate)
    p.add_argument("--n-steps", type=int, default=defaults.n_steps)
    p.add_argument("--batch-size", type=int, default=defaults.batch_size)
    p.add_argument("--n-epochs", type=int, default=defaults.n_epochs)
    p.add_argument("--gamma", type=float, default=defaults.gamma)
    p.add_argument("--gae-lambda", type=float, default=defaults.gae_lambda)
    p.add_argument("--clip-range", type=float, default=defaults.clip_range)
    p.add_argument("--ent-coef", type=float, default=defaults.ent_coef)
    p.add_argument("--vf-coef", type=float, default=defaults.vf_coef)
    p.add_argument("--max-grad-norm", type=float, default=defaults.max_grad_norm)
    p.add_argument(
        "--net-arch",
        type=parse_net_arch,
        default=defaults.net_arch,
        help="Comma-separated MLP widths, e.g. 64,64 or 256,128,64",
    )
    p.add_argument("--total-timesteps", type=int, default=defaults.total_timesteps)
    p.add_argument(
        "--max-episode-steps", type=int, default=defaults.max_episode_steps
    )
    p.add_argument("--seed", type=int, default=defaults.seed)
    p.add_argument(
        "--opponent-difficulty",
        choices=["Easy", "Medium", "Hard"],
        default=defaults.opponent_difficulty,
        help="Scripted opponent preset used during training",
    )
    p.add_argument("--model-name", type=str, default=defaults.model_name)
    p.add_argument("--tensorboard-log", type=str, default=defaults.tensorboard_log)
    p.add_argument("--skip-env-check", action="store_true")
    p.add_argument(
        "--list-difficulty-modes",
        action="store_true",
        help="Print easy/medium/hard play-mode registry and exit",
    )
    return p


def hyperparams_from_args(args: argparse.Namespace) -> TrainHyperParams:
    return TrainHyperParams(
        learning_rate=args.learning_rate,
        n_steps=args.n_steps,
        batch_size=args.batch_size,
        n_epochs=args.n_epochs,
        gamma=args.gamma,
        gae_lambda=args.gae_lambda,
        clip_range=args.clip_range,
        ent_coef=args.ent_coef,
        vf_coef=args.vf_coef,
        max_grad_norm=args.max_grad_norm,
        net_arch=list(args.net_arch),
        total_timesteps=args.total_timesteps,
        max_episode_steps=args.max_episode_steps,
        seed=args.seed,
        opponent_difficulty=args.opponent_difficulty,
        model_name=args.model_name,
        tensorboard_log=args.tensorboard_log,
        skip_env_check=args.skip_env_check,
    )
