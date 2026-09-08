"""Gymnasium wrapper around the C++ tanks RL environment."""

from __future__ import annotations

import gymnasium as gym
import numpy as np

import tanks_env_cpp


def _resolve_opponent(opponent):
    """Accept OpponentDifficulty, OpponentConfig, or None (medium default)."""
    if opponent is None:
        return tanks_env_cpp.OpponentConfig.for_difficulty(
            tanks_env_cpp.OpponentDifficulty.Medium
        )
    if isinstance(opponent, tanks_env_cpp.OpponentDifficulty):
        return tanks_env_cpp.OpponentConfig.for_difficulty(opponent)
    return opponent


class TankEnv(gym.Env):
    """
    Player 1 = RL agent, Player 2 = scripted C++ opponent.
    One env step = one simulation tick.
    """

    metadata = {"render_modes": []}

    def __init__(self, max_episode_steps=1000, opponent=None):
        super().__init__()

        self.cpp_env = tanks_env_cpp.RLEnvironment(_resolve_opponent(opponent))
        self.max_episode_steps = max_episode_steps
        self.steps = 0

        self.action_space = gym.spaces.Discrete(tanks_env_cpp.ACTION_COUNT)
        self.observation_space = gym.spaces.Box(
            low=-1.0,
            high=1.0,
            shape=(tanks_env_cpp.OBSERVATION_SIZE,),
            dtype=np.float32,
        )

    def _get_observation(self):
        return np.asarray(self.cpp_env.observation(), dtype=np.float32)

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)
        if seed is None:
            self.cpp_env.reset()
        else:
            seed = int(seed)
            self.cpp_env.random_reset(seed)
        self.steps = 0
        return self._get_observation(), {}

    def step(self, action):
        action = int(action)
        if not self.action_space.contains(action):
            raise ValueError(
                f"Invalid action {action}. Expected an integer in "
                f"[0, {tanks_env_cpp.ACTION_COUNT - 1}]."
            )

        self.cpp_env.step(action)
        self.steps += 1

        observation = self._get_observation()
        reward = float(self.cpp_env.reward())
        terminated = bool(self.cpp_env.done())
        truncated = self.steps >= self.max_episode_steps and not terminated
        return observation, reward, terminated, truncated, {}

    def render(self):
        return None

    def close(self):
        self.cpp_env = None
