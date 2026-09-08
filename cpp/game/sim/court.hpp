#pragma once

#include "tank.hpp"

#include <string>

namespace tanks {

struct PlayerInput {
  int move_x = 0;  // -1, 0, 1
  int move_y = 0;
  bool shoot_regular = false;
  bool shoot_ice = false;
  bool shoot_poison = false;
};

class Court {
public:
  Court();

  void reset();
  void random_reset(int seed);
  void apply_input(PlayerId id, const PlayerInput& input);
  void tick();

  bool playing() const { return playing_; }
  const std::string& status() const { return status_; }
  const Tank& player1() const { return *player1_; }
  const Tank& player2() const { return *player2_; }
  int explosion_ticks_left() const { return explosion_; }

private:
  void resolve_same_team(std::vector<std::unique_ptr<Bullet>>& bullets,
                         std::vector<Bullet*>& remove,
                         std::vector<std::unique_ptr<Bullet>>& spawned);
  void resolve_cross_team();

  std::unique_ptr<Tank> player1_;
  std::unique_ptr<Tank> player2_;
  bool playing_ = false;
  int explosion_ = 5000;
  std::string status_ = "Press R to start";
};

}  // namespace tanks
