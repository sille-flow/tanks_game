#include "test_harness.hpp"

#include "sim/tank.hpp"

using tanks::BulletType;
using tanks::PlayerId;
using tanks::Tank;
using tanks::TANK_MAX_HEALTH;
using tanks::TANK_SIZE;
using tanks::TANK_SPEED;
using tanks::COURT_HEIGHT;
using tanks::COURT_WIDTH;

TEST(Tank, PlayerFacingDefaults) {
  Tank p1(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 0);
  EXPECT_EQ(p1.facing_x(), 1);
  EXPECT_EQ(p1.facing_y(), 0);

  Tank p2(COURT_WIDTH, COURT_HEIGHT, PlayerId::Two, COURT_WIDTH - TANK_SIZE);
  EXPECT_EQ(p2.facing_x(), -1);
  EXPECT_EQ(p2.facing_y(), 0);
}

TEST(Tank, SpeedReducedByIceAndPoison) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 0);
  EXPECT_EQ(tank.speed(), TANK_SPEED);

  tank.set_ice_counter(10);
  EXPECT_EQ(tank.speed(), TANK_SPEED / 2);

  tank.set_ice_counter(0);
  tank.set_poison_counter(5);
  EXPECT_EQ(tank.speed(), static_cast<int>(TANK_SPEED * 0.75));
}

TEST(Tank, SetMoveIntentUpdatesVelocityAndFacing) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 0);
  tank.set_move_intent(1, 0);
  EXPECT_EQ(tank.vx(), TANK_SPEED);
  EXPECT_EQ(tank.facing_x(), 1);

  tank.set_move_intent(0, -1);
  EXPECT_EQ(tank.vy(), -TANK_SPEED);
  EXPECT_EQ(tank.facing_y(), -1);
}

TEST(Tank, ShootWhileStationaryUsesFacing) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 0);
  tank.shoot(BulletType::Regular, COURT_WIDTH, COURT_HEIGHT);
  EXPECT_EQ(tank.bullets().size(), 1u);
  EXPECT_EQ(tank.bullets()[0]->vx(), tanks::BULLET_SPEED);  // facing +x
  EXPECT_EQ(tank.bullets()[0]->owner(), PlayerId::One);
}

TEST(Tank, ShootWhileMovingUsesVelocityDirection) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 0);
  tank.set_move_intent(0, 1);
  tank.shoot(BulletType::Ice, COURT_WIDTH, COURT_HEIGHT);
  EXPECT_EQ(tank.bullets().size(), 1u);
  EXPECT_EQ(tank.bullets()[0]->vy(), tanks::BULLET_SPEED);
  EXPECT_EQ(tank.bullets()[0]->type(), BulletType::Ice);
}

TEST(Tank, DecrementCountersAndPoisonFlag) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 0);
  tank.set_ice_counter(2);
  tank.set_poison_counter(2);
  EXPECT_TRUE(tank.decrement_counters());  // poison still > 0
  EXPECT_EQ(tank.ice_counter(), 1);
  EXPECT_EQ(tank.poison_counter(), 1);
  EXPECT_FALSE(tank.decrement_counters());  // poison now 0
  EXPECT_EQ(tank.health(), TANK_MAX_HEALTH);
}

TEST(Tank, ExplodeSpawnsEightBullets) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 100);
  tank.explode(COURT_WIDTH, COURT_HEIGHT);
  EXPECT_EQ(tank.bullets().size(), 8u);
}

TEST(Tank, DiedWhenHealthNonPositive) {
  Tank tank(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 0);
  EXPECT_FALSE(tank.died());
  tank.set_health(0);
  EXPECT_TRUE(tank.died());
}
