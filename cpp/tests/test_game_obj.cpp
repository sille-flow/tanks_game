#include "test_harness.hpp"

#include "sim/game_obj.hpp"

using tanks::Direction;
using tanks::GameObj;

TEST(GameObj, MoveUpdatesPosition) {
  GameObj obj(3, -2, 10, 20, 40, 40, 800, 800);
  obj.move();
  EXPECT_EQ(obj.px(), 13);
  EXPECT_EQ(obj.py(), 18);
}

TEST(GameObj, ClipKeepsInsideCourt) {
  GameObj obj(0, 0, 0, 0, 40, 40, 800, 800);
  obj.set_px(-50);
  obj.set_py(9999);
  EXPECT_EQ(obj.px(), 0);
  EXPECT_EQ(obj.py(), 760);  // 800 - 40
}

TEST(GameObj, IntersectsOverlappingBoxes) {
  GameObj a(0, 0, 0, 0, 40, 40, 800, 800);
  GameObj b(0, 0, 20, 20, 40, 40, 800, 800);
  GameObj c(0, 0, 100, 100, 40, 40, 800, 800);
  EXPECT_TRUE(a.intersects(b));
  EXPECT_FALSE(a.intersects(c));
}

TEST(GameObj, WillIntersectUsesNextPositions) {
  GameObj a(10, 0, 0, 0, 40, 40, 800, 800);
  GameObj b(-10, 0, 50, 0, 40, 40, 800, 800);
  EXPECT_TRUE(a.will_intersect(b));

  GameObj far(0, 0, 400, 400, 40, 40, 800, 800);
  EXPECT_FALSE(a.will_intersect(far));
}

TEST(GameObj, BounceFlipsVelocity) {
  GameObj obj(-5, 7, 100, 100, 20, 20, 800, 800);
  obj.bounce(Direction::Left);
  EXPECT_EQ(obj.vx(), 5);
  obj.bounce(Direction::Down);
  EXPECT_EQ(obj.vy(), -7);
}

TEST(GameObj, HitWallDetectsEdges) {
  GameObj left(-4, 0, 0, 100, 20, 20, 800, 800);
  EXPECT_EQ(left.hit_wall(), Direction::Left);

  GameObj right(4, 0, 780, 100, 20, 20, 800, 800);
  EXPECT_EQ(right.hit_wall(), Direction::Right);

  GameObj none(0, 0, 100, 100, 20, 20, 800, 800);
  EXPECT_EQ(none.hit_wall(), Direction::None);
}
