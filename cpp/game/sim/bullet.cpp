#include "bullet.hpp"

#include "tank.hpp"

#include <cmath>

namespace tanks {

Bullet::Bullet(int court_width, int court_height, double vel_mult_x,
               double vel_mult_y, int pos_x, int pos_y, PlayerId owner,
               BulletType type)
    : GameObj(static_cast<int>(vel_mult_x * BULLET_SPEED),
              static_cast<int>(vel_mult_y * BULLET_SPEED), pos_x, pos_y,
              BULLET_SIZE, BULLET_SIZE, court_width, court_height),
      owner_(owner),
      type_(type) {}

void Bullet::collide(Tank& tank) const {
  switch (type_) {
    case BulletType::Regular:
      tank.decrement_health(width_);
      break;
    case BulletType::Ice:
      tank.set_ice_counter(width_);
      tank.decrement_health(width_ / 5);
      break;
    case BulletType::Poison:
      tank.set_poison_counter(width_ / 2);
      break;
  }
}

std::unique_ptr<Bullet> Bullet::combine(const Bullet& that, int court_width,
                                        int court_height) const {
  const double m1 = 3.141592653589793 * (width_ / 2.0) * (width_ / 2.0);
  const double m2 =
      3.141592653589793 * (that.width_ / 2.0) * (that.width_ / 2.0);
  const double vx = (m1 * vx_ + m2 * that.vx_) / (m1 + m2);
  const double vy = (m1 * vy_ + m2 * that.vy_) / (m1 + m2);

  auto bull = std::make_unique<Bullet>(court_width, court_height,
                                       vx / BULLET_SPEED, vy / BULLET_SPEED,
                                       px_, py_, owner_, type_);
  const int r = static_cast<int>(std::round(std::sqrt((m1 + m2) / 3.141592653589793)));
  bull->set_size(2 * r, 2 * r);
  return bull;
}

void Bullet::rebound(Bullet& that, double multiplier) {
  const int v_x1 = vx_;
  const int v_y1 = vy_;
  const int v_x2 = that.vx_;
  const int v_y2 = that.vy_;
  const int w1 = width_;
  const int w2 = that.width_;

  // Match the CIS 1200 rebound formulas (including quirks) for gameplay parity.
  const double v_fx1 =
      ((static_cast<double>(w1 - w2) / (w1 + w2)) * v_x1) -
      ((2.0 * w2 / (w1 + w2)) * v_x2);
  const double v_fy1 =
      ((static_cast<double>(w1 - w2) / (w1 + w2)) * v_y1) -
      ((2.0 * w2 / (w1 + w2)) * v_y2);
  const double v_fx2 =
      ((static_cast<double>(w2 - w1) / (w1 + w2)) * v_x2) -
      ((2.0 * w1 / (w1 + w2)) * v_x1);
  const double v_fy2 =
      ((static_cast<double>(w2 - w1) / (w1 + w2)) * v_x1) -
      ((2.0 * w1 / (w1 + w2)) * v_y1);

  set_vx(static_cast<int>(multiplier * v_fx1));
  set_vy(static_cast<int>(multiplier * v_fy1));
  that.set_vx(static_cast<int>(multiplier * v_fx2));
  that.set_vy(static_cast<int>(multiplier * v_fy2));
}

}  // namespace tanks
