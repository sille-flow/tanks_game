#include "test_harness.hpp"

#include "rl/rl-environment.hpp"

#include <stdexcept>

using tanks::OpponentConfig;
using tanks::OpponentDifficulty;
using tanks::RLEnvironment;
using tanks::TANK_MAX_HEALTH;

namespace {
constexpr int kActionCount = tanks::RLEnvironment::ACTION_COUNT;
constexpr int kObsSize = tanks::RLEnvironment::OBSERVATION_SIZE;
}  // namespace

TEST(RL, ObservationHasFixedSizeAndBounds) {
  RLEnvironment env;
  const auto obs = env.observation();
  EXPECT_EQ(static_cast<int>(obs.size()), kObsSize);
  for (float v : obs) {
    EXPECT_GE(v, -1.0f);
    EXPECT_LE(v, 1.0f);
  }
}

TEST(RL, ResetRestoresHealthAndClearsDone) {
  RLEnvironment env;
  for (int i = 0; i < 5; ++i) {
    env.step(32);  // idle
  }
  env.reset();
  EXPECT_FALSE(env.done());
  EXPECT_EQ(env.reward(), 0.0f);
  EXPECT_EQ(env.court().player1().health(), TANK_MAX_HEALTH);
  EXPECT_EQ(env.court().player2().health(), TANK_MAX_HEALTH);
}

TEST(RL, DecodeActionMovementAndShootBits) {
  RLEnvironment env;

  auto in0 = env.decode_action(0);
  EXPECT_EQ(in0.move_x, -1);
  EXPECT_EQ(in0.move_y, -1);
  EXPECT_FALSE(in0.shoot_regular);
  EXPECT_FALSE(in0.shoot_ice);
  EXPECT_FALSE(in0.shoot_poison);

  auto in7 = env.decode_action(7);
  EXPECT_EQ(in7.move_x, -1);
  EXPECT_EQ(in7.move_y, -1);
  EXPECT_TRUE(in7.shoot_regular);
  EXPECT_TRUE(in7.shoot_ice);
  EXPECT_TRUE(in7.shoot_poison);

  auto in32 = env.decode_action(32);
  EXPECT_EQ(in32.move_x, 0);
  EXPECT_EQ(in32.move_y, 0);
  EXPECT_FALSE(in32.shoot_regular);

  // idle + regular: movement 4, shoot bit 1 -> action 33
  auto in33 = env.decode_action(33);
  EXPECT_EQ(in33.move_x, 0);
  EXPECT_EQ(in33.move_y, 0);
  EXPECT_TRUE(in33.shoot_regular);
  EXPECT_FALSE(in33.shoot_ice);
  EXPECT_FALSE(in33.shoot_poison);

  auto in71 = env.decode_action(71);
  EXPECT_EQ(in71.move_x, 1);
  EXPECT_EQ(in71.move_y, 1);
  EXPECT_TRUE(in71.shoot_regular);
  EXPECT_TRUE(in71.shoot_ice);
  EXPECT_TRUE(in71.shoot_poison);
}

TEST(RL, DecodeActionRejectsOutOfRange) {
  RLEnvironment env;
  bool threw = false;
  try {
    env.decode_action(-1);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  EXPECT_TRUE(threw);

  threw = false;
  try {
    env.decode_action(kActionCount);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  EXPECT_TRUE(threw);
}

TEST(RL, StepMovesAgentAndAdvancesEpisode) {
  RLEnvironment env;
  const int start_x = env.court().player1().px();
  // move (+1, 0), no shoot -> movement_index 7 -> action 56
  env.step(56);
  EXPECT_GT(env.court().player1().px(), start_x);
  EXPECT_FALSE(env.done());
}

TEST(RL, AgentShootSpawnsBullet) {
  RLEnvironment env;
  EXPECT_EQ(env.court().player1().bullets().size(), 0u);
  env.step(33);  // idle + regular
  EXPECT_EQ(env.court().player1().bullets().size(), 1u);
}

TEST(RL, EasyOpponentFiresLessOftenThanHard) {
  // Both difficulties fire on tick 0 (0 % interval == 0), so time-to-first
  // shot is identical. Compare scheduled fire attempts over a window instead.
  auto shot_attempts = [](OpponentDifficulty d) {
    const auto cfg = OpponentConfig::for_difficulty(d);
    int attempts = 0;
    constexpr int kTicks = 60;
    for (int t = 0; t < kTicks; ++t) {
      if (cfg.fire_interval > 0 && t % cfg.fire_interval == 0) {
        ++attempts;
      }
    }
    return attempts;
  };

  EXPECT_GT(shot_attempts(OpponentDifficulty::Hard),
            shot_attempts(OpponentDifficulty::Easy));
}

TEST(RL, TerminalWinGivesPositiveReward) {
  RLEnvironment env;
  const_cast<tanks::Tank&>(env.court().player2()).set_health(0);
  env.step(32);
  EXPECT_TRUE(env.done());
  EXPECT_GT(env.reward(), 0.0f);
}

TEST(RL, TerminalLossGivesNegativeReward) {
  RLEnvironment env;
  const_cast<tanks::Tank&>(env.court().player1()).set_health(0);
  env.step(32);
  EXPECT_TRUE(env.done());
  EXPECT_LT(env.reward(), 0.0f);
}

TEST(RL, DifficultyPresetsDiffer) {
  const auto easy = OpponentConfig::for_difficulty(OpponentDifficulty::Easy);
  const auto hard = OpponentConfig::for_difficulty(OpponentDifficulty::Hard);
  EXPECT_GT(easy.fire_interval, hard.fire_interval);
  EXPECT_GT(easy.ice_interval, hard.ice_interval);
  EXPECT_LT(easy.ice_range, hard.ice_range);
}

TEST(RL, StepAfterDoneIsNoOp) {
  RLEnvironment env;
  const_cast<tanks::Tank&>(env.court().player2()).set_health(0);
  env.step(32);
  EXPECT_TRUE(env.done());
  const float reward_after_end = env.reward();
  const auto bullets = env.court().player1().bullets().size();
  env.step(33);  // would shoot if still playing
  EXPECT_EQ(env.reward(), reward_after_end);
  EXPECT_EQ(env.court().player1().bullets().size(), bullets);
}
