#pragma once

#include "bullet.hpp"

#include <memory>
#include <vector>

namespace tanks {

class Tank : public GameObj {
public:
  Tank(int court_width, int court_height, PlayerId id, int init_pos_x);

  PlayerId id() const { return id_; }
  int health() const { return health_; }
  int ice_counter() const { return ice_counter_; }
  int poison_counter() const { return poison_counter_; }
  const std::vector<std::unique_ptr<Bullet>>& bullets() const { return bullets_; }
  std::vector<std::unique_ptr<Bullet>>& bullets() { return bullets_; }

  int speed() const;
  void set_ice_counter(int time) { ice_counter_ = time; }
  void set_poison_counter(int time) { poison_counter_ = time; }
  void set_health(int h) { health_ = h; }
  void decrement_health(int damage) { health_ -= damage; }
  bool died() const { return health_ <= 0; }

  // Returns true while poison remains active after this tick's decrement.
  bool decrement_counters();

  void set_move_intent(int dir_x, int dir_y);
  void shoot(BulletType type, int court_width, int court_height);
  void explode(int court_width, int court_height);

  int facing_x() const { return facing_x_; }
  int facing_y() const { return facing_y_; }

private:
  static int sign(int v);

  PlayerId id_;
  int health_ = TANK_MAX_HEALTH;
  int base_speed_ = TANK_SPEED;
  int ice_counter_ = 0;
  int poison_counter_ = 0;
  int facing_x_ = 1;
  int facing_y_ = 0;
  std::vector<std::unique_ptr<Bullet>> bullets_;
};

}  // namespace tanks
