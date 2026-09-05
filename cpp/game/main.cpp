#include "sim/constants.hpp"
#include "sim/court.hpp"

#include "raylib.h"

#include <string>

namespace {

constexpr int kHudHeight = 88;
constexpr int kWindowWidth = tanks::COURT_WIDTH;
constexpr int kWindowHeight = tanks::COURT_HEIGHT + kHudHeight;
constexpr int kCourtOffsetY = kHudHeight;

Color player_color(tanks::PlayerId id) {
  return id == tanks::PlayerId::One ? Color{220, 60, 60, 255}
                                    : Color{60, 110, 220, 255};
}

tanks::PlayerInput read_player1_input() {
  tanks::PlayerInput in;
  if (IsKeyDown(KEY_LEFT)) {
    in.move_x -= 1;
  }
  if (IsKeyDown(KEY_RIGHT)) {
    in.move_x += 1;
  }
  if (IsKeyDown(KEY_UP)) {
    in.move_y -= 1;
  }
  if (IsKeyDown(KEY_DOWN)) {
    in.move_y += 1;
  }
  // Match Java: shoot on key release.
  in.shoot_ice = IsKeyReleased(KEY_B);
  in.shoot_regular = IsKeyReleased(KEY_N);
  in.shoot_poison = IsKeyReleased(KEY_M);
  return in;
}

tanks::PlayerInput read_player2_input() {
  tanks::PlayerInput in;
  if (IsKeyDown(KEY_A)) {
    in.move_x -= 1;
  }
  if (IsKeyDown(KEY_D)) {
    in.move_x += 1;
  }
  if (IsKeyDown(KEY_W)) {
    in.move_y -= 1;
  }
  if (IsKeyDown(KEY_S)) {
    in.move_y += 1;
  }
  in.shoot_ice = IsKeyReleased(KEY_X);
  in.shoot_regular = IsKeyReleased(KEY_C);
  in.shoot_poison = IsKeyReleased(KEY_V);
  return in;
}

void draw_ice_bullet(const tanks::Bullet& b, Color outline) {
  const int x = b.px();
  const int y = b.py() + kCourtOffsetY;
  const int w = b.width();
  Vector2 pts[8] = {
      {static_cast<float>(x), static_cast<float>(y + w / 2)},
      {static_cast<float>(x + w / 3), static_cast<float>(y + w / 3)},
      {static_cast<float>(x + w / 2), static_cast<float>(y)},
      {static_cast<float>(x + (2 * w) / 3), static_cast<float>(y + w / 3)},
      {static_cast<float>(x + w), static_cast<float>(y + w / 2)},
      {static_cast<float>(x + (2 * w) / 3), static_cast<float>(y + (2 * w) / 3)},
      {static_cast<float>(x + w / 2), static_cast<float>(y + w)},
      {static_cast<float>(x + w / 3), static_cast<float>(y + (2 * w) / 3)},
  };
  DrawTriangle(pts[0], pts[1], pts[7], SKYBLUE);
  DrawTriangle(pts[1], pts[2], pts[3], SKYBLUE);
  DrawTriangle(pts[3], pts[4], pts[5], SKYBLUE);
  DrawTriangle(pts[5], pts[6], pts[7], SKYBLUE);
  DrawTriangle(pts[1], pts[3], pts[5], SKYBLUE);
  DrawTriangle(pts[1], pts[5], pts[7], SKYBLUE);
  for (int i = 0; i < 8; ++i) {
    DrawLineV(pts[i], pts[(i + 1) % 8], outline);
  }
}

void draw_poison_bullet(const tanks::Bullet& b, Color outline) {
  const int x = b.px();
  const int y = b.py() + kCourtOffsetY;
  const int w = b.width();
  Vector2 pts[6] = {
      {static_cast<float>(x), static_cast<float>(y + w / 3)},
      {static_cast<float>(x + w / 2), static_cast<float>(y)},
      {static_cast<float>(x + w), static_cast<float>(y + w / 3)},
      {static_cast<float>(x + w), static_cast<float>(y + (2 * w) / 3)},
      {static_cast<float>(x + w / 2), static_cast<float>(y + w)},
      {static_cast<float>(x), static_cast<float>(y + (2 * w) / 3)},
  };
  DrawTriangle(pts[0], pts[1], pts[5], LIME);
  DrawTriangle(pts[1], pts[2], pts[3], LIME);
  DrawTriangle(pts[1], pts[3], pts[4], LIME);
  DrawTriangle(pts[1], pts[4], pts[5], LIME);
  for (int i = 0; i < 6; ++i) {
    DrawLineV(pts[i], pts[(i + 1) % 6], outline);
  }
}

void draw_bullet(const tanks::Bullet& b) {
  const Color outline = player_color(b.owner());
  switch (b.type()) {
    case tanks::BulletType::Regular: {
      const float cx = b.px() + b.width() / 2.0f;
      const float cy = b.py() + kCourtOffsetY + b.height() / 2.0f;
      DrawCircle(static_cast<int>(cx), static_cast<int>(cy), b.width() / 2.0f,
                 BLACK);
      DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy),
                      b.width() / 2.0f, outline);
      break;
    }
    case tanks::BulletType::Ice:
      draw_ice_bullet(b, outline);
      break;
    case tanks::BulletType::Poison:
      draw_poison_bullet(b, outline);
      break;
  }
}

void draw_tank(const tanks::Tank& tank) {
  if (tank.died()) {
    return;
  }
  const Color color = player_color(tank.id());
  const int x = tank.px();
  const int y = tank.py() + kCourtOffsetY;
  DrawRectangle(x, y, tank.width(), tank.height(), color);

  // Ice crystals above the tank.
  for (int i = 0; i < tank.ice_counter(); ++i) {
    Vector2 a{static_cast<float>(x + i * 8), static_cast<float>(y)};
    Vector2 b{static_cast<float>(x + 3 + i * 8), static_cast<float>(y - 4)};
    Vector2 c{static_cast<float>(x + 6 + i * 8), static_cast<float>(y)};
    DrawTriangle(a, b, c, SKYBLUE);
  }
  // Poison dots.
  for (int i = 0; i < tank.poison_counter(); ++i) {
    DrawCircle(x + i * 5 + 2, y - 2, 2.0f, LIME);
  }
  // Health bar.
  const int bar_w = (tank.width() * tank.health()) / tanks::TANK_MAX_HEALTH;
  DrawRectangle(x, y + tank.height() + 2, bar_w, 4, RED);
}

void draw_hud(const tanks::Court& court) {
  DrawRectangle(0, 0, kWindowWidth, kHudHeight, Color{28, 30, 36, 255});
  DrawText("1200 Tanks — Local Coop", 12, 10, 18, RAYWHITE);
  DrawText(court.status().c_str(), 12, 34, 16, LIGHTGRAY);

  const int h1 = court.player1().health();
  const int h2 = court.player2().health();
  DrawText(TextFormat("P1 HP %d", h1), 12, 58, 14, Color{220, 60, 60, 255});
  DrawText(TextFormat("P2 HP %d", h2), 120, 58, 14, Color{60, 110, 220, 255});
  DrawText("R reset", 230, 58, 14, GRAY);
  DrawText("P1: arrows B/N/M   P2: WASD X/C/V", 12, 74, 12, DARKGRAY);
}

void draw_court(const tanks::Court& court) {
  DrawRectangle(0, kCourtOffsetY, tanks::COURT_WIDTH, tanks::COURT_HEIGHT,
                Color{245, 245, 240, 255});
  DrawRectangleLines(0, kCourtOffsetY, tanks::COURT_WIDTH, tanks::COURT_HEIGHT,
                     BLACK);

  draw_tank(court.player1());
  draw_tank(court.player2());
  for (const auto& b : court.player1().bullets()) {
    draw_bullet(*b);
  }
  for (const auto& b : court.player2().bullets()) {
    draw_bullet(*b);
  }
}

}  // namespace

int main() {
  SetTraceLogLevel(LOG_WARNING);
  InitWindow(kWindowWidth, kWindowHeight, "1200 Tanks — Local Coop");
  SetTargetFPS(60);

  tanks::Court court;
  double accumulator_ms = 0.0;

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_R)) {
      court.reset();
      accumulator_ms = 0.0;
    }

    court.apply_input(tanks::PlayerId::One, read_player1_input());
    court.apply_input(tanks::PlayerId::Two, read_player2_input());

    accumulator_ms += GetFrameTime() * 1000.0;
    while (accumulator_ms >= tanks::TICK_MS) {
      court.tick();
      accumulator_ms -= tanks::TICK_MS;
    }

    BeginDrawing();
    ClearBackground(Color{18, 18, 22, 255});
    draw_hud(court);
    draw_court(court);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
