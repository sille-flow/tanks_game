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

TEST(ADDITIONAL, RewardTracksDamageDealt) {
  RLEnvironment env;

  const int enemy_health_before = env.court().player2().health();

  // Simulate damage to the opponent between RL steps.
  const_cast<tanks::Tank&>(env.court().player2()).set_health(
      enemy_health_before - 10);

  env.step(32);  // idle

  EXPECT_EQ(env.reward(), 0.10f);
}

TEST(ADDITIONAL, RewardTracksDamageReceived) {
  RLEnvironment env;

  const int self_health_before = env.court().player1().health();

  // Simulate damage received between RL steps.
  const_cast<tanks::Tank&>(env.court().player1()).set_health(
      self_health_before - 10);

  env.step(32);  // idle

  EXPECT_EQ(env.reward(), -0.10f);
}

TEST(ADDITIONAL, RewardTracksNetDamage) {
  RLEnvironment env;

  const int enemy_health = env.court().player2().health();
  const int self_health = env.court().player1().health();

  const_cast<tanks::Tank&>(env.court().player2()).set_health(
      enemy_health - 20);

  const_cast<tanks::Tank&>(env.court().player1()).set_health(
      self_health - 5);

  env.step(32);  // idle

  // 20 damage dealt - 5 damage received = +0.15
  EXPECT_EQ(env.reward(), 0.15f);
}
TEST(ADDITIONAL, RewardTracksDamagePerStep) {
  RLEnvironment env;

  auto& enemy =
      const_cast<tanks::Tank&>(env.court().player2());

  const int starting_health = enemy.health();

  enemy.set_health(starting_health - 10);
  env.step(32);

  EXPECT_EQ(env.reward(), 0.10f);

  enemy.set_health(starting_health - 20);
  env.step(32);

  EXPECT_EQ(env.reward(), 0.10f);
}
TEST(ADDITIONAL, KillingOpponentAddsTerminalBonus) {
  RLEnvironment env;

  auto& opponent =
      const_cast<tanks::Tank&>(env.court().player2());

  opponent.set_health(0);

  env.step(32);

  EXPECT_TRUE(env.done());
  EXPECT_EQ(env.reward(), 1.0f);
}
TEST(ADDITIONAL, LosingAddsTerminalPenalty) {
  RLEnvironment env;

  auto& agent =
      const_cast<tanks::Tank&>(env.court().player1());

  agent.set_health(0);

  env.step(32);

  EXPECT_TRUE(env.done());
  EXPECT_EQ(env.reward(), -1.0f);
}
TEST(ADDITIONAL, ObservationContainsInitialTankState) {
  RLEnvironment env;

  const auto obs = env.observation();

  EXPECT_EQ(obs[0], 0.0f);  // Player 1 x
  EXPECT_EQ(obs[1], 0.0f);  // Player 1 y

  EXPECT_EQ(obs[2], 0.0f);  // Player 1 vx
  EXPECT_EQ(obs[3], 0.0f);  // Player 1 vy

  EXPECT_EQ(obs[4], 1.0f);  // Player 1 facing x
  EXPECT_EQ(obs[5], 0.0f);  // Player 1 facing y

  EXPECT_EQ(obs[6], 1.0f);  // Full health
}
TEST(ADDITIONAL, ObservationUpdatesAfterMovement) {
  RLEnvironment env;

  const auto before = env.observation();

  env.step(56);  // +x movement

  const auto after = env.observation();

  EXPECT_GT(after[0], before[0]);
  EXPECT_GT(after[2], 0.0f);
}
TEST(ADDITIONAL, ObservationContainsOpponentRelativePosition) {
  RLEnvironment env;

  const auto obs = env.observation();

  const auto& p1 = env.court().player1();
  const auto& p2 = env.court().player2();

  const float expected_x =
      static_cast<float>(p2.px() - p1.px()) /
      static_cast<float>(COURT_WIDTH);

  const float expected_y =
      static_cast<float>(p2.py() - p1.py()) /
      static_cast<float>(COURT_HEIGHT);

  EXPECT_EQ(obs[9], expected_x);
  EXPECT_EQ(obs[10], expected_y);
}
TEST(ADDITIONAL, ObservationContainsOpponentRelativePosition) {
  RLEnvironment env;

  const auto obs = env.observation();

  const auto& p1 = env.court().player1();
  const auto& p2 = env.court().player2();

  const float expected_x =
      static_cast<float>(p2.px() - p1.px()) /
      static_cast<float>(COURT_WIDTH);

  const float expected_y =
      static_cast<float>(p2.py() - p1.py()) /
      static_cast<float>(COURT_HEIGHT);

  EXPECT_EQ(obs[9], expected_x);
  EXPECT_EQ(obs[10], expected_y);
}
TEST(ADDITIONAL, ObservationReflectsHealthChanges) {
  RLEnvironment env;

  auto& opponent =
      const_cast<tanks::Tank&>(env.court().player2());

  opponent.set_health(TANK_MAX_HEALTH / 2);

  const auto obs = env.observation();

  EXPECT_EQ(obs[15], 0.5f);
}
TEST(ADDITIONAL, ObservationPadsMissingBulletsWithZeros) {
  RLEnvironment env;

  const auto obs = env.observation();

  constexpr int BULLET_OFFSET = 18;
  constexpr int BULLET_SIZE = 6;

  // Initially there should be no bullets.
  for (int i = 0; i < 4 * BULLET_SIZE; ++i) {
    EXPECT_EQ(obs[BULLET_OFFSET + i], 0.0f);
  }
}
TEST(ADDITIONAL, ObservationLimitsBulletCountToFourSlots) {
  RLEnvironment env;

  PlayerInput shoot;
  shoot.shoot_regular = true;

  for (int i = 0; i < 8; ++i) {
    env.court().apply_input(PlayerId::One, shoot);
  }

  const auto obs = env.observation();

  EXPECT_EQ(static_cast<int>(obs.size()), kObsSize);
}
TEST(ADDITIONAL, ObservationLimitsBulletCountToFourSlots) {
  RLEnvironment env;

  PlayerInput shoot;
  shoot.shoot_regular = true;

  for (int i = 0; i < 8; ++i) {
    env.court().apply_input(PlayerId::One, shoot);
  }

  const auto obs = env.observation();

  EXPECT_EQ(static_cast<int>(obs.size()), kObsSize);
}
TEST(ADDITIONAL, OpponentPositionIsRelativeToAgent) {
  RLEnvironment env;

  auto& p1 = const_cast<tanks::Tank&>(env.court().player1());
  auto& p2 = const_cast<tanks::Tank&>(env.court().player2());

  const auto before = env.observation();

  const int p1_x = p1.px();
  const int p2_x = p2.px();

  p1.set_px(p1_x + 50);
  p2.set_px(p2_x + 50);

  const auto after = env.observation();

  EXPECT_EQ(after[9], before[9]);
}