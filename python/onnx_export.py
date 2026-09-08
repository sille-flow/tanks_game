"""
onnx_export.py

Shared logic for exporting a trained Stable-Baselines3 PPO policy to ONNX,
for the C++ ONNX Runtime inference path (rl-inference.hpp/cpp).

Used by:
  - rl-learning.py        exports automatically right after training,
                           straight from the in-memory model (no reload).
  - export_ppo_to_onnx.py CLI to (re-)export an already-saved .zip
                           checkpoint on demand, without retraining.

Design notes (why the export looks the way it does)
-----------------------------------------------------
- POLICY-ONLY graph: no value head. The game only needs actions at
  inference time; skipping the critic keeps the graph small, which
  matters more once the Emscripten/WASM target exists.

- We export the RAW action_net output rather than a sampled action:
    Discrete: logits, shape (batch, n_actions)
    Box:      Gaussian mean, shape (batch, action_dim)
  The C++ side (RLPolicy::infer in rl-inference.cpp) argmaxes the logits
  for the discrete case used by this project (RLEnvironment::ACTION_COUNT
  = 72). Sampling stochastically from the distribution instead is
  possible but adds RNG-parity concerns across Python/C++/WASM that
  aren't worth it for an opponent AI at inference time.

- CAVEAT — VecNormalize: if training wraps the env in VecNormalize
  (observation normalization), those running stats are NOT part of the
  policy network and will NOT be exported. rl-learning.py currently does
  not use VecNormalize, so this doesn't apply yet, but if you add it
  later you'll need to either bake the same running mean/var into the
  C++ inference wrapper, or drop normalization before exporting.

- CAVEAT — squash_output: if the policy was created with
  squash_output=True (tanh-squashed Gaussian, common with continuous
  control + SDE), the exported mean is PRE-squash; apply tanh() on the
  C++ side. Only relevant for Box action spaces — this project's action
  space is Discrete, so it doesn't apply, but export_policy_to_onnx()
  reports it either way for future continuous action spaces.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import numpy as np
import torch
import torch.nn as nn


class PolicyOnlyWrapper(nn.Module):
    """
    Wraps an SB3 ActorCriticPolicy to expose ONLY the actor path:
        observation -> features -> latent_pi -> action_net -> raw_output
    """

    def __init__(self, policy: nn.Module):
        super().__init__()
        self.mlp_extractor = policy.mlp_extractor
        self.action_net = policy.action_net
        # Falls back to features_extractor for SB3 versions that don't
        # split pi/vf feature extractors.
        self.pi_features_extractor = getattr(
            policy, "pi_features_extractor", policy.features_extractor
        )

    def forward(self, obs: torch.Tensor) -> torch.Tensor:
        features = self.pi_features_extractor(obs)
        latent_pi = self.mlp_extractor.forward_actor(features)
        return self.action_net(latent_pi)


@dataclass
class ExportSummary:
    onnx_path: Path
    observation_size: int
    action_kind: str  # "discrete" or "continuous"
    action_output_size: int
    squash_output: bool
    max_verification_diff: Optional[float]  # None if verification was skipped


def export_policy_to_onnx(
    model,  # a loaded/trained stable_baselines3.PPO instance
    out_path: Path,
    opset: int = 17,
    verify: bool = True,
) -> ExportSummary:
    """
    Exports model.policy to ONNX at out_path.

    Raises ValueError if the observation/action space shapes aren't
    supported (flat Box observation; Discrete or Box action space).
    """
    from gymnasium import spaces

    policy = model.policy
    policy.eval()

    obs_space = model.observation_space
    act_space = model.action_space

    if not isinstance(obs_space, spaces.Box) or len(obs_space.shape) != 1:
        raise ValueError(
            f"Expected a flat Box observation space, got {obs_space!r}. "
            "This exporter assumes a 1D observation vector (matches "
            "RLEnvironment::observation() / OBSERVATION_SIZE)."
        )
    obs_dim = obs_space.shape[0]

    if isinstance(act_space, spaces.Discrete):
        action_kind = "discrete"
        n_actions = int(act_space.n)
    elif isinstance(act_space, spaces.Box):
        action_kind = "continuous"
        n_actions = int(np.prod(act_space.shape))
    else:
        raise ValueError(f"Unsupported action space type: {type(act_space)}")

    squash_output = bool(getattr(policy, "squash_output", False))

    wrapper = PolicyOnlyWrapper(policy).to("cpu").eval()
    dummy_obs = torch.zeros((1, obs_dim), dtype=torch.float32)

    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    torch.onnx.export(
        wrapper,
        dummy_obs,
        str(out_path),
        input_names=["observation"],
        output_names=["action_output"],
        dynamic_axes={
            "observation": {0: "batch"},
            "action_output": {0: "batch"},
        },
        opset_version=opset,
        do_constant_folding=True,
    )

    max_diff = _verify(wrapper, out_path, obs_dim) if verify else None

    return ExportSummary(
        onnx_path=out_path,
        observation_size=obs_dim,
        action_kind=action_kind,
        action_output_size=n_actions,
        squash_output=squash_output,
        max_verification_diff=max_diff,
    )


def _verify(wrapper: nn.Module, onnx_path: Path, obs_dim: int) -> Optional[float]:
    """Random-observation sanity check: PyTorch wrapper output should match
    the exported ONNX graph's output. Returns None if onnxruntime isn't
    installed (export itself doesn't require it, only this check does)."""
    try:
        import onnxruntime as ort
    except ImportError:
        print("(skipping ONNX numerical verification: `pip install onnxruntime` to enable)")
        return None

    session = ort.InferenceSession(str(onnx_path), providers=["CPUExecutionProvider"])

    rng = np.random.default_rng(0)
    max_abs_diff = 0.0
    for _ in range(8):
        sample = rng.normal(size=(1, obs_dim)).astype(np.float32)
        with torch.no_grad():
            torch_out = wrapper(torch.from_numpy(sample)).numpy()
        onnx_out = session.run(None, {"observation": sample})[0]
        max_abs_diff = max(max_abs_diff, float(np.max(np.abs(torch_out - onnx_out))))
    return max_abs_diff


def print_summary(summary: ExportSummary) -> None:
    print()
    print("ONNX export summary")
    print("--------------------")
    print(f"  observation size (input) : {summary.observation_size}"
          "   <-- must match C++ RLEnvironment::OBSERVATION_SIZE")
    print(f"  action kind               : {summary.action_kind}")
    print(f"  action output size        : {summary.action_output_size}"
          "   <-- must match C++ RLEnvironment::ACTION_COUNT (if discrete)")
    if summary.action_kind == "discrete":
        print("  C++ side should:            argmax(action_output) -> RLEnvironment::decode_action()")
    else:
        note = " after tanh()" if summary.squash_output else ""
        print(f"  C++ side should:            use action_output directly as the continuous action{note}")
    if summary.max_verification_diff is not None:
        status = "OK" if summary.max_verification_diff < 1e-4 else "WARNING: large diff, investigate before trusting export"
        print(f"  verification max abs diff : {summary.max_verification_diff:.2e}  [{status}]")
    print(f"  output file               : {summary.onnx_path}")