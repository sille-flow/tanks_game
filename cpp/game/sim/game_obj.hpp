#pragma once

#include "constants.hpp"

namespace tanks {

class GameObj {
public:
  GameObj(int vx, int vy, int px, int py, int width, int height,
          int court_width, int court_height);
  virtual ~GameObj() = default;

  int px() const { return px_; }
  int py() const { return py_; }
  int vx() const { return vx_; }
  int vy() const { return vy_; }
  int width() const { return width_; }
  int height() const { return height_; }

  void set_px(int px);
  void set_py(int py);
  void set_vx(int vx) { vx_ = vx; }
  void set_vy(int vy) { vy_ = vy; }
  void set_size(int width, int height);

  void move();
  bool intersects(const GameObj& that) const;
  bool will_intersect(const GameObj& that) const;
  void bounce(Direction d);
  Direction hit_wall() const;
  Direction hit_obj(const GameObj& that) const;

protected:
  void clip();

  int px_ = 0;
  int py_ = 0;
  int width_ = 0;
  int height_ = 0;
  int vx_ = 0;
  int vy_ = 0;
  int max_x_ = 0;
  int max_y_ = 0;
};

}  // namespace tanks
