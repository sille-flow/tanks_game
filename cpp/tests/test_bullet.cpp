#include "test_harness.hpp"

#include "sim/bullet.hpp"
#include "sim/tank.hpp"

using tanks::Bullet;
using tanks::BulletType;
using tanks::BULLET_SIZE;
using tanks::BULLET_SPEED;
using tanks::COURT_HEIGHT;
using tanks::COURT_WIDTH;
using tanks::PlayerId;
using tanks::Tank;
using tanks::TANK_MAX_HEALTH;

TEST(Bullet, RegularCollideDamagesByWidth) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::Two, 400);
  Bullet bull(COURT_WIDTH, COURT_HEIGHT, 1, 0, 0, 0, PlayerId::One,
              BulletType::Regular);
  bull.collide(tank);
  EXPECT_EQ(tank.health(), TANK_MAX_HEALTH - BULLET_SIZE);
}

TEST(Bullet, IceCollideSetsCounterAndSmallDamage) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::Two, 400);
  Bullet bull(COURT_WIDTH, COURT_HEIGHT, 1, 0, 0, 0, PlayerId::One,
              BulletType::Ice);
  bull.collide(tank);
  EXPECT_EQ(tank.ice_counter(), BULLET_SIZE);
  EXPECT_EQ(tank.health(), TANK_MAX_HEALTH - BULLET_SIZE / 5);
}

TEST(Bullet, PoisonCollideSetsCounterWithoutDirectDamage) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::Two, 400);
  const int health_before = tank.health();
  Bullet bull(COURT_WIDTH, COURT_HEIGHT, 1, 0, 0, 0, PlayerId::One,
              BulletType::Poison);
  bull.collide(tank);
  EXPECT_EQ(tank.poison_counter(), BULLET_SIZE / 2);
  EXPECT_EQ(tank.health(), health_before);
}

TEST(Bullet, CombineGrowsSize) {
  Bullet a(COURT_WIDTH, COURT_HEIGHT, 1, 0, 100, 100, PlayerId::One,
           BulletType::Regular);
  Bullet b(COURT_WIDTH, COURT_HEIGHT, -1, 0, 100, 100, PlayerId::One,
           BulletType::Regular);
  auto combined = a.combine(b, COURT_WIDTH, COURT_HEIGHT);
  EXPECT_TRUE(combined != nullptr);
  EXPECT_GT(combined->width(), BULLET_SIZE);
  EXPECT_EQ(combined->owner(), PlayerId::One);
}

TEST(Bullet, ReboundScalesVelocitiesByMultiplier) {
  // Equal size + opposite equal speeds: the mass terms yield vx' = -other.vx
  // (so values are unchanged at multiplier 1.0). A non-1 multiplier must apply.
  Bullet a(COURT_WIDTH, COURT_HEIGHT, 1, 0, 100, 100, PlayerId::One,
           BulletType::Regular);
  Bullet b(COURT_WIDTH, COURT_HEIGHT, -1, 0, 100, 100, PlayerId::Two,
           BulletType::Regular);
  EXPECT_EQ(a.vx(), BULLET_SPEED);
  EXPECT_EQ(b.vx(), -BULLET_SPEED);

  a.rebound(b, 0.5);
  EXPECT_EQ(a.vx(), BULLET_SPEED / 2);
  EXPECT_EQ(b.vx(), -BULLET_SPEED / 2);
}

TEST(Bullet, InitialVelocityScaledByBulletSpeed) {
  Bullet bull(COURT_WIDTH, COURT_HEIGHT, -1, 1, 50, 50, PlayerId::One,
              BulletType::Regular);
  EXPECT_EQ(bull.vx(), -BULLET_SPEED);
  EXPECT_EQ(bull.vy(), BULLET_SPEED);
}
