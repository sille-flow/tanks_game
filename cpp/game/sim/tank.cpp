#include "tank.hpp"

namespace tanks {

Tank::Tank(int court_width, int court_height, PlayerId id, int init_pos_x)
    : GameObj(0, 0, init_pos_x, 0, TANK_SIZE, TANK_SIZE, court_width,
              court_height),
      id_(id) {
  // Player 1 faces right; player 2 faces left.
  if (id_ == PlayerId::Two) {
    facing_x_ = -1;
    facing_y_ = 0;
  }
  clip();
}

int Tank::sign(int v) {
  if (v < 0) {
    return -1;
  }
  if (v > 0) {
    return 1;
  }
  return 0;
}

int Tank::speed() const {
  double multiplier = 1.0;
  if (ice_counter_ > 0) {
    multiplier *= 0.5;
  }
  if (poison_counter_ > 4) {
    multiplier *= 0.75;
  }
  return static_cast<int>(base_speed_ * multiplier);
}

bool Tank::decrement_counters() {
  if (ice_counter_ > 0) {
    --ice_counter_;
  }
  if (poison_counter_ > 0) {
    --poison_counter_;
  }
  return poison_counter_ > 0;
}

void Tank::set_move_intent(int dir_x, int dir_y) {
  const int spd = speed();
  set_vx(dir_x * spd);
  set_vy(dir_y * spd);
  if (dir_x != 0 || dir_y != 0) {
    facing_x_ = sign(dir_x);
    facing_y_ = sign(dir_y);
  }
}

void Tank::shoot(BulletType type, int court_width, int court_height) {
  int v_mult_x = sign(vx_);
  int v_mult_y = sign(vy_);
  // If stationary, fire along last facing direction (usable pointer).
  if (v_mult_x == 0 && v_mult_y == 0) {
    v_mult_x = facing_x_;
    v_mult_y = facing_y_;
  }
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, static_cast<double>(v_mult_x),
      static_cast<double>(v_mult_y), px_, py_, id_, type));
}

void Tank::explode(int court_width, int court_height) {
  const int x = px_;
  const int y = py_;
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, -1, 1, x, y, id_, BulletType::Regular));
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, 1, 1, x, y, id_, BulletType::Regular));
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, 1, -1, x, y, id_, BulletType::Regular));
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, -1, -1, x, y, id_, BulletType::Regular));
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, 0, 1, x, y, id_, BulletType::Ice));
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, 0, -1, x, y, id_, BulletType::Ice));
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, 1, 0, x, y, id_, BulletType::Poison));
  bullets_.push_back(std::make_unique<Bullet>(
      court_width, court_height, -1, 0, x, y, id_, BulletType::Poison));
}

}  // namespace tanks
