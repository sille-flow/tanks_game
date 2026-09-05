#include "rl/rl-environment.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace tanks {

namespace {

float clamp_float(float value, float low, float high) {
  return std::max(low, std::min(value, high));
}

float normalize_position_x(int x) {
  return clamp_float(static_cast<float>(x) / static_cast<float>(COURT_WIDTH),
                     -1.0f, 1.0f);
}

float normalize_position_y(int y) {
  return clamp_float(static_cast<float>(y) / static_cast<float>(COURT_HEIGHT),
                     -1.0f, 1.0f);
}

float normalize_health(int health) {
  return clamp_float(
      static_cast<float>(health) / static_cast<float>(TANK_MAX_HEALTH), -1.0f,
      1.0f);
}

float normalize_velocity(int velocity) {
  return std::tanh(static_cast<float>(velocity) / 8.0f);
}

float normalize_counter(int counter) {
  return clamp_float(static_cast<float>(counter) / 20.0f, 0.0f, 1.0f);
}

}  // namespace

OpponentConfig OpponentConfig::for_difficulty(OpponentDifficulty difficulty) {
  OpponentConfig config;
  switch (difficulty) {
    case OpponentDifficulty::Easy:
      config.fire_interval = 18;
      config.ice_interval = 90;
      config.poison_interval = 120;
      config.position_tolerance = 16;
      config.ice_range = 140;
      config.poison_range = 100;
      break;
    case OpponentDifficulty::Medium:
      config.fire_interval = 10;
      config.ice_interval = 60;
      config.poison_interval = 90;
      config.position_tolerance = 10;
      config.ice_range = 200;
      config.poison_range = 150;
      break;
    case OpponentDifficulty::Hard:
      config.fire_interval = 6;
      config.ice_interval = 35;
      config.poison_interval = 50;
      config.position_tolerance = 6;
      config.ice_range = 280;
      config.poison_range = 220;
      break;
  }
  return config;
}

RLEnvironment::RLEnvironment(OpponentConfig opponent)
    : opponent_config_(opponent) {
  reset();
}

void RLEnvironment::set_opponent_config(OpponentConfig config) {
  opponent_config_ = config;
}

void RLEnvironment::reset() {
  court_.reset();
  last_reward_ = 0.0f;
  previous_enemy_health_ = court_.player2().health();
  previous_self_health_ = court_.player1().health();
  opponent_ticks_ = 0;
}

PlayerInput RLEnvironment::decode_action(int action) const {
  if (action < 0 || action >= ACTION_COUNT) {
    throw std::out_of_range("RL action must be between 0 and 71");
  }

  PlayerInput input;

  // 9 movement combinations x 8 shooting bitmasks = 72 actions.
  const int movement_index = action / 8;
  const int shooting_index = action % 8;

  input.move_x = movement_index / 3 - 1;
  input.move_y = movement_index % 3 - 1;
  input.shoot_regular = (shooting_index & 1) != 0;
  input.shoot_ice = (shooting_index & 2) != 0;
  input.shoot_poison = (shooting_index & 4) != 0;

  return input;
}

PlayerInput RLEnvironment::scripted_opponent() {
  PlayerInput input;

  const Tank& agent = court_.player1();
  const Tank& opponent = court_.player2();

  const int dx = agent.px() - opponent.px();
  const int dy = agent.py() - opponent.py();

  if (std::abs(dx) > opponent_config_.position_tolerance) {
    input.move_x = dx > 0 ? 1 : -1;
  }
  if (std::abs(dy) > opponent_config_.position_tolerance) {
    input.move_y = dy > 0 ? 1 : -1;
  }

  if (opponent_config_.fire_interval > 0 &&
      opponent_ticks_ % opponent_config_.fire_interval == 0) {
    input.shoot_regular = true;
  }

  const int manhattan = std::abs(dx) + std::abs(dy);
  if (opponent_config_.ice_interval > 0 &&
      opponent_ticks_ % opponent_config_.ice_interval == 0 &&
      manhattan < opponent_config_.ice_range) {
    input.shoot_ice = true;
  }
  if (opponent_config_.poison_interval > 0 &&
      opponent_ticks_ % opponent_config_.poison_interval == 0 &&
      manhattan < opponent_config_.poison_range) {
    input.shoot_poison = true;
  }

  return input;
}

void RLEnvironment::step(int action) {
  if (done()) {
    return;
  }

  court_.apply_input(PlayerId::One, decode_action(action));
  court_.apply_input(PlayerId::Two, scripted_opponent());
  court_.tick();

  last_reward_ = calculate_reward();
  previous_enemy_health_ = court_.player2().health();
  previous_self_health_ = court_.player1().health();
  ++opponent_ticks_;
}

float RLEnvironment::calculate_reward() const {
  const int current_enemy_health = court_.player2().health();
  const int current_self_health = court_.player1().health();

  const int damage_dealt = previous_enemy_health_ - current_enemy_health;
  const int damage_received = previous_self_health_ - current_self_health;

  float reward = 0.01f * static_cast<float>(damage_dealt) -
                 0.01f * static_cast<float>(damage_received);

  if (court_.player2().died()) {
    reward += 1.0f;
  }
  if (court_.player1().died()) {
    reward -= 1.0f;
  }

  return reward;
}

float RLEnvironment::reward() const { return last_reward_; }

bool RLEnvironment::done() const { return !court_.playing(); }

void RLEnvironment::append_bullet_observation(
    std::vector<float>& observation, const Bullet& bullet,
    const Tank& reference_tank) const {
  const int relative_x = bullet.px() - reference_tank.px();
  const int relative_y = bullet.py() - reference_tank.py();

  observation.push_back(normalize_position_x(relative_x));
  observation.push_back(normalize_position_y(relative_y));
  observation.push_back(normalize_velocity(bullet.vx()));
  observation.push_back(normalize_velocity(bullet.vy()));
  observation.push_back(
      static_cast<float>(static_cast<int>(bullet.type())) / 2.0f);
  observation.push_back(bullet.owner() == PlayerId::One ? 1.0f : -1.0f);
}

std::vector<float> RLEnvironment::observation() const {
  const Tank& agent = court_.player1();
  const Tank& opponent = court_.player2();

  std::vector<float> observation;
  observation.reserve(OBSERVATION_SIZE);

  observation.push_back(normalize_position_x(agent.px()));
  observation.push_back(normalize_position_y(agent.py()));
  observation.push_back(normalize_velocity(agent.vx()));
  observation.push_back(normalize_velocity(agent.vy()));
  observation.push_back(static_cast<float>(agent.facing_x()));
  observation.push_back(static_cast<float>(agent.facing_y()));
  observation.push_back(normalize_health(agent.health()));
  observation.push_back(normalize_counter(agent.ice_counter()));
  observation.push_back(normalize_counter(agent.poison_counter()));

  observation.push_back(normalize_position_x(opponent.px() - agent.px()));
  observation.push_back(normalize_position_y(opponent.py() - agent.py()));
  observation.push_back(normalize_velocity(opponent.vx()));
  observation.push_back(normalize_velocity(opponent.vy()));
  observation.push_back(static_cast<float>(opponent.facing_x()));
  observation.push_back(static_cast<float>(opponent.facing_y()));
  observation.push_back(normalize_health(opponent.health()));
  observation.push_back(normalize_counter(opponent.ice_counter()));
  observation.push_back(normalize_counter(opponent.poison_counter()));

  struct BulletInfo {
    const Bullet* bullet;
    float distance_squared;
  };

  std::vector<BulletInfo> bullets;
  for (const auto& bullet : agent.bullets()) {
    if (!bullet) {
      continue;
    }
    const float dx = static_cast<float>(bullet->px() - agent.px());
    const float dy = static_cast<float>(bullet->py() - agent.py());
    bullets.push_back({bullet.get(), dx * dx + dy * dy});
  }
  for (const auto& bullet : opponent.bullets()) {
    if (!bullet) {
      continue;
    }
    const float dx = static_cast<float>(bullet->px() - agent.px());
    const float dy = static_cast<float>(bullet->py() - agent.py());
    bullets.push_back({bullet.get(), dx * dx + dy * dy});
  }

  std::sort(bullets.begin(), bullets.end(),
            [](const BulletInfo& a, const BulletInfo& b) {
              return a.distance_squared < b.distance_squared;
            });

  constexpr int MAX_BULLETS = 4;
  for (int i = 0; i < MAX_BULLETS; ++i) {
    if (i < static_cast<int>(bullets.size())) {
      append_bullet_observation(observation, *bullets[i].bullet, agent);
    } else {
      observation.insert(observation.end(), 6, 0.0f);
    }
  }

  return observation;
}

}  // namespace tanks
