#pragma once

#include "game_obj.hpp"

#include <memory>

namespace tanks {

class Tank;

class Bullet : public GameObj {
public:
  Bullet(int court_width, int court_height, double vel_mult_x, double vel_mult_y,
         int pos_x, int pos_y, PlayerId owner, BulletType type);

  PlayerId owner() const { return owner_; }
  BulletType type() const { return type_; }

  void collide(Tank& tank) const;
  std::unique_ptr<Bullet> combine(const Bullet& that, int court_width,
                                  int court_height) const;
  void rebound(Bullet& that, double multiplier);

private:
  PlayerId owner_;
  BulletType type_;
};

}  // namespace tanks
