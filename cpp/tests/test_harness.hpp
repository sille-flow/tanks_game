#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace test {

struct Registry {
  using Fn = void (*)();
  std::vector<std::pair<const char*, Fn>> cases;
  int failures = 0;

  static Registry& instance() {
    static Registry r;
    return r;
  }

  void add(const char* name, Fn fn) { cases.emplace_back(name, fn); }

  int run_all() {
    failures = 0;
    int passed = 0;
    for (const auto& [name, fn] : cases) {
      std::printf("RUN  %s\n", name);
      const int before = failures;
      fn();
      if (failures == before) {
        std::printf("OK   %s\n", name);
        ++passed;
      } else {
        std::printf("FAIL %s\n", name);
      }
    }
    const int failed = static_cast<int>(cases.size()) - passed;
    std::printf("\n%d passed, %d failed, %zu total\n", passed, failed,
                cases.size());
    return failed == 0 ? 0 : 1;
  }
};

struct Registrar {
  Registrar(const char* name, Registry::Fn fn) {
    Registry::instance().add(name, fn);
  }
};

inline void expect(bool cond, const char* expr, const char* file, int line) {
  if (!cond) {
    std::printf("  assertion failed: %s (%s:%d)\n", expr, file, line);
    ++Registry::instance().failures;
  }
}

}  // namespace test

#define TEST(suite, name)                                            \
  static void suite##_##name();                                      \
  static test::Registrar suite##_##name##_reg(#suite "." #name,      \
                                              &suite##_##name);      \
  static void suite##_##name()

#define EXPECT_TRUE(cond) \
  test::expect(static_cast<bool>(cond), #cond, __FILE__, __LINE__)

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

#define EXPECT_EQ(a, b) EXPECT_TRUE((a) == (b))

#define EXPECT_NE(a, b) EXPECT_TRUE((a) != (b))

#define EXPECT_GT(a, b) EXPECT_TRUE((a) > (b))

#define EXPECT_GE(a, b) EXPECT_TRUE((a) >= (b))

#define EXPECT_LT(a, b) EXPECT_TRUE((a) < (b))

#define EXPECT_LE(a, b) EXPECT_TRUE((a) <= (b))
