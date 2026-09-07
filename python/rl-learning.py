"""
Train a PPO agent against the scripted C++ opponent.

Difficulty roadmap (play modes, not training opponent):
  easy   — scripted baseline (or earliest checkpoint)
  medium — mid-training checkpoint
  hard   — latest / strongest checkpoint

Training itself always uses a scripted opponent; pass --opponent-difficulty
to curriculum-train against easier/harder scripted bots.
"""

from __future__ import annotations

import json
from pathlib import Path

from stable_baselines3 import PPO
from stable_baselines3.common.env_checker import check_env

from onnx_export import export_policy_to_onnx, print_summary

import tanks_env_cpp

from rl_config import (
    DIFFICULTY_MODES,
    build_arg_parser,
    hyperparams_from_args,
)
from tankenv import TankEnv


def main(argv: list[str] | None = None) -> None:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    if args.list_difficulty_modes:
        print(json.dumps(DIFFICULTY_MODES, indent=2))
        return

    hp = hyperparams_from_args(args)
    difficulty = getattr(
        tanks_env_cpp.OpponentDifficulty, hp.opponent_difficulty
    )

    print("Creating environment...")
    print(f"  opponent_difficulty={hp.opponent_difficulty}")
    print(f"  hyperparameters={json.dumps(hp.to_dict(), indent=2)}")

    env = TankEnv(
        max_episode_steps=hp.max_episode_steps,
        opponent=difficulty,
    )

    if not hp.skip_env_check:
        print("Checking Gymnasium environment...")
        check_env(env, warn=True, skip_render_check=True)
        print("Environment is valid.")

    print("Starting PPO training...")
    model = PPO(
        policy="MlpPolicy",
        env=env,
        verbose=1,
        seed=hp.seed,
        learning_rate=hp.learning_rate,
        n_steps=hp.n_steps,
        batch_size=hp.batch_size,
        n_epochs=hp.n_epochs,
        gamma=hp.gamma,
        gae_lambda=hp.gae_lambda,
        clip_range=hp.clip_range,
        ent_coef=hp.ent_coef,
        vf_coef=hp.vf_coef,
        max_grad_norm=hp.max_grad_norm,
        policy_kwargs=dict(net_arch=hp.net_arch),
        tensorboard_log=hp.tensorboard_log,
    )

    model.learn(total_timesteps=hp.total_timesteps, progress_bar=True)

    output_path = Path("models")
    output_path.mkdir(parents=True, exist_ok=True)
    save_path = output_path / hp.model_name
    model.save(save_path)

    meta_path = output_path / f"{hp.model_name}.hparams.json"
    meta_path.write_text(json.dumps(hp.to_dict(), indent=2), encoding="utf-8")

    print()
    print("Training complete.")
    print(f"Model saved to {save_path}")
    print(f"Hyperparams saved to {meta_path}")
    onnx_path = output_path / f"{hp.model_name}.onnx"
    print()
    print(f"Exporting policy to ONNX ({onnx_path}) ...")
    try:
        summary = export_policy_to_onnx(model, onnx_path, opset=17, verify=True)
        print_summary(summary)

        # Fail loudly rather than silently shipping a mismatched model —
        # a shape mismatch here means the C++ side's RLPolicy constructor
        # will reject the file anyway, but it's cheaper to catch it now.
        # rl-bindings.cpp exposes these as module-level attributes:
        #   m.attr("ACTION_COUNT") = tanks::RLEnvironment::ACTION_COUNT;
        #   m.attr("OBSERVATION_SIZE") = tanks::RLEnvironment::OBSERVATION_SIZE;
        if summary.observation_size != tanks_env_cpp.OBSERVATION_SIZE:
            print(
                f"WARNING: exported observation_size={summary.observation_size} "
                f"!= tanks_env_cpp.OBSERVATION_SIZE={tanks_env_cpp.OBSERVATION_SIZE}. "
                "The C++ RLPolicy will refuse to load this file."
            )
        if (
            summary.action_kind == "discrete"
            and summary.action_output_size != tanks_env_cpp.ACTION_COUNT
        ):
            print(
                f"WARNING: exported action_output_size={summary.action_output_size} "
                f"!= tanks_env_cpp.ACTION_COUNT={tanks_env_cpp.ACTION_COUNT}. "
                "The C++ RLPolicy will refuse to load this file."
            )
    except ValueError as e:
        # Don't let a bad export nuke an otherwise-successful training run —
        # the .zip checkpoint above is already safely saved. Re-export later
        # with export_ppo_to_onnx.py once fixed.
        print(f"WARNING: ONNX export failed, but the .zip checkpoint was saved fine: {e}")

    print()
    print("Play-mode registry (fill checkpoints as you go):")
    print(json.dumps(DIFFICULTY_MODES, indent=2))

    env.close()


if __name__ == "__main__":
    main()
