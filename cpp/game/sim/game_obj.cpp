#include "game_obj.hpp"

#include <algorithm>
#include <cmath>

namespace tanks {

GameObj::GameObj(int vx, int vy, int px, int py, int width, int height,
                 int court_width, int court_height)
    : px_(px),
      py_(py),
      width_(width),
      height_(height),
      vx_(vx),
      vy_(vy),
      max_x_(court_width - width),
      max_y_(court_height - height) {}

void GameObj::set_px(int px) {
  px_ = px;
  clip();
}

void GameObj::set_py(int py) {
  py_ = py;
  clip();
}

void GameObj::set_size(int width, int height) {
  const int width_change = width - width_;
  const int height_change = height - height_;
  width_ = width;
  height_ = height;
  max_x_ -= width_change;
  max_y_ -= height_change;
}

void GameObj::clip() {
  px_ = std::min(std::max(px_, 0), max_x_);
  py_ = std::min(std::max(py_, 0), max_y_);
}

void GameObj::move() {
  px_ += vx_;
  py_ += vy_;
  clip();
}

bool GameObj::intersects(const GameObj& that) const {
  return px_ + width_ >= that.px_ && py_ + height_ >= that.py_ &&
         that.px_ + that.width_ >= px_ && that.py_ + that.height_ >= py_;
}

bool GameObj::will_intersect(const GameObj& that) const {
  const int this_next_x = px_ + vx_;
  const int this_next_y = py_ + vy_;
  const int that_next_x = that.px_ + that.vx_;
  const int that_next_y = that.py_ + that.vy_;
  return this_next_x + width_ >= that_next_x &&
         this_next_y + height_ >= that_next_y &&
         that_next_x + that.width_ >= this_next_x &&
         that_next_y + that.height_ >= this_next_y;
}

void GameObj::bounce(Direction d) {
  switch (d) {
    case Direction::Up:
      vy_ = std::abs(vy_);
      break;
    case Direction::Down:
      vy_ = -std::abs(vy_);
      break;
    case Direction::Left:
      vx_ = std::abs(vx_);
      break;
    case Direction::Right:
      vx_ = -std::abs(vx_);
      break;
    default:
      break;
  }
}

Direction GameObj::hit_wall() const {
  if (px_ + vx_ < 0) {
    return Direction::Left;
  }
  if (px_ + vx_ > max_x_) {
    return Direction::Right;
  }
  if (py_ + vy_ < 0) {
    return Direction::Up;
  }
  if (py_ + vy_ > max_y_) {
    return Direction::Down;
  }
  return Direction::None;
}

Direction GameObj::hit_obj(const GameObj& that) const {
  if (!will_intersect(that)) {
    return Direction::None;
  }

  const double half_this_w = width_ / 2.0;
  const double half_that_w = that.width_ / 2.0;
  const double half_this_h = height_ / 2.0;
  const double half_that_h = that.height_ / 2.0;
  constexpr double kPiOver4 = 0.7853981633974483;

  const double dx = that.px_ + half_that_w - (px_ + half_this_w);
  const double dy = that.py_ + half_that_h - (py_ + half_this_h);
  const double dist = std::sqrt(dx * dx + dy * dy);
  if (dist == 0.0) {
    return Direction::Right;
  }

  const double theta = std::acos(dx / dist);
  if (theta <= kPiOver4) {
    return Direction::Right;
  }
  if (theta <= 3.141592653589793 - kPiOver4) {
    return dy > 0 ? Direction::Down : Direction::Up;
  }
  return Direction::Left;
}

}  // namespace tanks
