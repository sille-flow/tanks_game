#pragma once

namespace tanks {

constexpr int COURT_WIDTH = 800;
constexpr int COURT_HEIGHT = 800;
constexpr int TICK_MS = 35;

constexpr int TANK_SIZE = 40;
constexpr int TANK_SPEED = 4;
constexpr int TANK_MAX_HEALTH = 100;

constexpr int BULLET_SIZE = 20;
constexpr int BULLET_SPEED = 4;

enum class Direction { None, Up, Down, Left, Right };

enum class BulletType { Regular = 0, Ice = 1, Poison = 2 };

enum class PlayerId { One = 0, Two = 1 };

}  // namespace tanks
