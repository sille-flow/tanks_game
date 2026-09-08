#include "court.hpp"
#include <random>
#include <algorithm>

namespace tanks {

Court::Court() { reset(); }

void Court::reset() {
  player1_ = std::make_unique<Tank>(COURT_WIDTH, COURT_HEIGHT, PlayerId::One, 0);
  player2_ = std::make_unique<Tank>(COURT_WIDTH, COURT_HEIGHT, PlayerId::Two,
                                    COURT_WIDTH - TANK_SIZE);
  playing_ = true;
  explosion_ = 5000;
  status_ = "Running...";
}

void Court::random_reset(int seed) {
  std::mt19937 rng(seed);
  reset();
  std::uniform_int_distribution<int> x_dist(0, COURT_WIDTH - TANK_SIZE);
  std::uniform_int_distribution<int> y_dist(0, COURT_HEIGHT - TANK_SIZE);
  int x1 = x_dist(rng);
  int y1 = y_dist(rng);
  int x2 = x_dist(rng);
  int y2 = y_dist(rng);
  while (std::abs(x1 - x2) < MIN_TANK_DISTANCE || std::abs(y1 - y2) < MIN_TANK_DISTANCE) {
    x1 = x_dist(rng);
    y1 = y_dist(rng);
    x2 = x_dist(rng);
    y2 = y_dist(rng);
  }
  player1_->set_px(x1);
  player1_->set_py(y1);
  player2_->set_px(x2);
  player2_->set_py(y2);
}

void Court::apply_input(PlayerId id, const PlayerInput& input) {
  if (!playing_) {
    return;
  }
  Tank& tank = (id == PlayerId::One) ? *player1_ : *player2_;
  tank.set_move_intent(input.move_x, input.move_y);
  if (input.shoot_ice) {
    tank.shoot(BulletType::Ice, COURT_WIDTH, COURT_HEIGHT);
  }
  if (input.shoot_regular) {
    tank.shoot(BulletType::Regular, COURT_WIDTH, COURT_HEIGHT);
  }
  if (input.shoot_poison) {
    tank.shoot(BulletType::Poison, COURT_WIDTH, COURT_HEIGHT);
  }
}

void Court::resolve_same_team(std::vector<std::unique_ptr<Bullet>>& bullets,
                              std::vector<Bullet*>& remove,
                              std::vector<std::unique_ptr<Bullet>>& spawned) {
  const auto marked = [&](Bullet* b) {
    return std::find(remove.begin(), remove.end(), b) != remove.end();
  };

  for (size_t i = 0; i < bullets.size(); ++i) {
    for (size_t j = i + 1; j < bullets.size(); ++j) {
      Bullet* a = bullets[i].get();
      Bullet* b = bullets[j].get();
      if (!a || !b || marked(a) || marked(b)) {
        continue;
      }
      if (!a->intersects(*b)) {
        continue;
      }
      if (a->type() == b->type()) {
        remove.push_back(a);
        remove.push_back(b);
        spawned.push_back(a->combine(*b, COURT_WIDTH, COURT_HEIGHT));
      } else {
        a->rebound(*b, 0.7);
      }
    }
  }
}

void Court::resolve_cross_team() {
  auto& list1 = player1_->bullets();
  auto& list2 = player2_->bullets();
  for (auto& b1 : list1) {
    for (auto& b2 : list2) {
      if (!b1 || !b2 || !b1->intersects(*b2)) {
        continue;
      }
      if (b1->type() == b2->type()) {
        b1->rebound(*b2, 1.0);
      } else {
        b1->bounce(b1->hit_obj(*b2));
        b2->bounce(b2->hit_obj(*b1));
      }
    }
  }
}

void Court::tick() {
  if (playing_) {
    player1_->move();
    if (player1_->decrement_counters()) {
      player1_->decrement_health(1);
    }
    player2_->move();
    if (player2_->decrement_counters()) {
      player2_->decrement_health(1);
    }

    auto& list1 = player1_->bullets();
    auto& list2 = player2_->bullets();
    std::vector<Bullet*> remove1;
    std::vector<Bullet*> remove2;

    for (auto& bull : list1) {
      bull->move();
      bull->bounce(bull->hit_wall());
      if (player2_->intersects(*bull)) {
        remove1.push_back(bull.get());
        bull->collide(*player2_);
      }
    }
    for (auto& bull : list2) {
      bull->move();
      bull->bounce(bull->hit_wall());
      if (player1_->intersects(*bull)) {
        remove2.push_back(bull.get());
        bull->collide(*player1_);
      }
    }

    std::vector<std::unique_ptr<Bullet>> new1;
    std::vector<std::unique_ptr<Bullet>> new2;
    resolve_same_team(list1, remove1, new1);
    resolve_same_team(list2, remove2, new2);
    resolve_cross_team();

    const auto erase_marked = [](std::vector<std::unique_ptr<Bullet>>& list,
                                 std::vector<Bullet*>& remove) {
      list.erase(std::remove_if(list.begin(), list.end(),
                                [&](const std::unique_ptr<Bullet>& b) {
                                  return std::find(remove.begin(), remove.end(),
                                                   b.get()) != remove.end();
                                }),
                 list.end());
    };
    erase_marked(list1, remove1);
    erase_marked(list2, remove2);
    for (auto& b : new1) {
      list1.push_back(std::move(b));
    }
    for (auto& b : new2) {
      list2.push_back(std::move(b));
    }

    if (player1_->died()) {
      playing_ = false;
      player1_->explode(COURT_WIDTH, COURT_HEIGHT);
      status_ = "Player 2 wins!";
    } else if (player2_->died()) {
      playing_ = false;
      player2_->explode(COURT_WIDTH, COURT_HEIGHT);
      status_ = "Player 1 wins!";
    }
  } else if (explosion_ > 0) {
    for (auto& bull : player1_->bullets()) {
      bull->move();
    }
    for (auto& bull : player2_->bullets()) {
      bull->move();
    }
    --explosion_;
  }
}

}  // namespace tanks
