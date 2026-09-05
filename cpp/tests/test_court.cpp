#include "test_harness.hpp"

#include "sim/court.hpp"

using tanks::COURT_WIDTH;
using tanks::Court;
using tanks::PlayerId;
using tanks::PlayerInput;
using tanks::TANK_MAX_HEALTH;
using tanks::TANK_SIZE;
using tanks::TANK_SPEED;

TEST(Court, ResetPlacesPlayersAndStartsPlaying) {
  Court court;
  EXPECT_TRUE(court.playing());
  EXPECT_EQ(court.player1().px(), 0);
  EXPECT_EQ(court.player2().px(), COURT_WIDTH - TANK_SIZE);
  EXPECT_EQ(court.player1().health(), TANK_MAX_HEALTH);
  EXPECT_EQ(court.player2().health(), TANK_MAX_HEALTH);
  EXPECT_EQ(court.status(), "Running...");
}

TEST(Court, ApplyInputMovesTank) {
  Court court;
  PlayerInput in;
  in.move_x = 1;
  court.apply_input(PlayerId::One, in);
  court.tick();
  EXPECT_EQ(court.player1().px(), TANK_SPEED);
}

TEST(Court, ApplyInputShootsBullet) {
  Court court;
  PlayerInput in;
  in.shoot_regular = true;
  court.apply_input(PlayerId::One, in);
  EXPECT_EQ(court.player1().bullets().size(), 1u);
}

TEST(Court, PoisonBulletHitThenDoT) {
  Court court;
  PlayerInput shoot;
  shoot.shoot_poison = true;
  court.apply_input(PlayerId::Two, shoot);

  auto& bullets = const_cast<tanks::Tank&>(court.player2()).bullets();
  EXPECT_EQ(bullets.size(), 1u);
  bullets[0]->set_px(court.player1().px());
  bullets[0]->set_py(court.player1().py());
  bullets[0]->set_vx(0);
  bullets[0]->set_vy(0);

  const int health_before = court.player1().health();
  court.tick();  // collide applies poison; no instant HP loss from poison type
  EXPECT_GT(court.player1().poison_counter(), 0);
  EXPECT_EQ(court.player1().health(), health_before);
  EXPECT_EQ(court.player2().bullets().size(), 0u);

  court.tick();  // poison still active after decrement -> 1 DoT
  EXPECT_EQ(court.player1().health(), health_before - 1);
}

TEST(Court, PlayerDiesEndsMatch) {
  Court court;
  const_cast<tanks::Tank&>(court.player2()).set_health(0);
  court.tick();
  EXPECT_FALSE(court.playing());
  EXPECT_EQ(court.status(), "Player 1 wins!");
  EXPECT_EQ(court.player2().bullets().size(), 8u);  // explode
}

TEST(Court, IgnoresInputWhenNotPlaying) {
  Court court;
  const_cast<tanks::Tank&>(court.player1()).set_health(0);
  court.tick();
  EXPECT_FALSE(court.playing());

  PlayerInput in;
  in.move_x = 1;
  in.shoot_regular = true;
  const int px = court.player2().px();
  const auto bullet_count = court.player2().bullets().size();
  court.apply_input(PlayerId::Two, in);
  EXPECT_EQ(court.player2().px(), px);
  EXPECT_EQ(court.player2().bullets().size(), bullet_count);
}
