"""Tests for training config / difficulty registry (no C++ module required)."""

from __future__ import annotations

import unittest

from rl_config import (
    DIFFICULTY_MODES,
    build_arg_parser,
    hyperparams_from_args,
    parse_net_arch,
)


class DifficultyRegistryTests(unittest.TestCase):
    def test_modes_exist(self):
        self.assertEqual(set(DIFFICULTY_MODES), {"easy", "medium", "hard"})
        self.assertEqual(DIFFICULTY_MODES["easy"]["opponent"], "scripted")
        self.assertEqual(DIFFICULTY_MODES["hard"]["opponent"], "checkpoint")

    def test_parse_net_arch(self):
        self.assertEqual(parse_net_arch("64,128,64"), [64, 128, 64])

    def test_hyperparams_from_cli(self):
        args = build_arg_parser().parse_args(
            [
                "--learning-rate",
                "1e-3",
                "--n-steps",
                "512",
                "--batch-size",
                "32",
                "--net-arch",
                "64,64,32",
                "--opponent-difficulty",
                "Easy",
                "--total-timesteps",
                "1000",
                "--model-name",
                "tank_ppo_easy",
                "--gamma",
                "0.95",
                "--ent-coef",
                "0.02",
            ]
        )
        hp = hyperparams_from_args(args)
        self.assertAlmostEqual(hp.learning_rate, 1e-3)
        self.assertEqual(hp.n_steps, 512)
        self.assertEqual(hp.batch_size, 32)
        self.assertEqual(hp.net_arch, [64, 64, 32])
        self.assertEqual(hp.opponent_difficulty, "Easy")
        self.assertEqual(hp.model_name, "tank_ppo_easy")
        self.assertEqual(hp.total_timesteps, 1000)
        self.assertAlmostEqual(hp.gamma, 0.95)
        self.assertAlmostEqual(hp.ent_coef, 0.02)


if __name__ == "__main__":
    unittest.main()
