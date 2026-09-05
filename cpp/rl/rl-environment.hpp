#pragma once

#include "sim/court.hpp"

#include <vector>

namespace tanks {

/**
 * Scripted Player-2 aggressiveness knobs.
 *
 * Easy/Medium/Hard here are curriculum baselines for training and for the
 * eventual "easy" play mode (scripted). Learned checkpoints will back
 * medium/hard play modes later.
 */
enum class OpponentDifficulty { Easy, Medium, Hard };

struct OpponentConfig {
  int fire_interval = 10;
  int ice_interval = 60;
  int poison_interval = 90;
  int position_tolerance = 10;
  int ice_range = 200;
  int poison_range = 150;

  static OpponentConfig for_difficulty(OpponentDifficulty difficulty);
};

/**
 * Headless reinforcement-learning interface to the Tanks game.
 *
 * Player 1 is controlled by the RL agent.
 * Player 2 is controlled by a scripted opponent.
 */
class RLEnvironment {
public:
  static constexpr int ACTION_COUNT = 72;
  static constexpr int OBSERVATION_SIZE = 42;

  explicit RLEnvironment(
      OpponentConfig opponent = OpponentConfig::for_difficulty(
          OpponentDifficulty::Medium));

  void reset();

  /** Execute one simulation tick. Action must be in [0, ACTION_COUNT). */
  void step(int action);

  std::vector<float> observation() const;
  float reward() const;
  bool done() const;

  /** Convert a discrete action index into PlayerInput (9 move x 8 shoot). */
  PlayerInput decode_action(int action) const;

  void set_opponent_config(OpponentConfig config);
  const OpponentConfig& opponent_config() const { return opponent_config_; }

  const Court& court() const { return court_; }

private:
  PlayerInput scripted_opponent();
  float calculate_reward() const;
  void append_bullet_observation(std::vector<float>& observation,
                                 const Bullet& bullet,
                                 const Tank& reference_tank) const;

  Court court_;
  OpponentConfig opponent_config_;

  float last_reward_ = 0.0f;
  int previous_enemy_health_ = TANK_MAX_HEALTH;
  int previous_self_health_ = TANK_MAX_HEALTH;
  int opponent_ticks_ = 0;
};

}  // namespace tanks
