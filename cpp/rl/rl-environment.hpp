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

  /**
   * Convert a discrete action index into PlayerInput (9 move x 8 shoot).
   *
   * Static because this mapping is a pure function of the action index —
   * it doesn't depend on any RLEnvironment instance state. This lets game
   * code decode an action without needing a full RLEnvironment/scripted-
   * opponent instance around, e.g. when driving Court directly for a
   * "play vs. trained bot" game mode instead of the training harness.
   */
  static PlayerInput decode_action(int action);

  /**
   * Builds the observation vector for `agent` playing against whichever
   * tank isn't `agent`, within `court`, in the exact layout the
   * ONNX-exported policy expects (see OBSERVATION_SIZE).
   *
   * Static and Court-agnostic for the same reason decode_action is
   * static: game code driving a plain Court directly (no scripted-
   * opponent training wrapper around it) needs to build the identical
   * observation encoding used during training rather than duplicating
   * it — any drift here would silently produce a wrong observation for
   * the trained model. observation() below is just this called with
   * (court_, PlayerId::One), since training always treats player 1 as
   * the learning agent.
   *
   * `agent` may be either PlayerId — the encoding is symmetric (bullet
   * ownership, positions, etc. are all computed relative to whichever
   * side is passed as `agent`).
   */
  static std::vector<float> build_observation(const Court& court,
                                               PlayerId agent);

  void set_opponent_config(OpponentConfig config);
  const OpponentConfig& opponent_config() const { return opponent_config_; }

  const Court& court() const { return court_; }

private:
  PlayerInput scripted_opponent();
  float calculate_reward() const;

  Court court_;
  OpponentConfig opponent_config_;

  float last_reward_ = 0.0f;
  int previous_enemy_health_ = TANK_MAX_HEALTH;
  int previous_self_health_ = TANK_MAX_HEALTH;
  int opponent_ticks_ = 0;
};

}  // namespace tanks