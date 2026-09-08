// bench-inference.cpp
//
// Benchmarks RLPolicy::infer() latency — this is the "Xms inference
// latency per decision step" resume number. A fixed random observation
// vector is reused for every call: latency depends on the ONNX graph and
// input size, not the specific input values, so there's no need to drive
// an actual game loop just to get this number.
//
// Usage: bench_inference <model.onnx> [iterations]
//   iterations defaults to 2000 if not given.

#include "rl-environment.hpp"
#include "rl-inference.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <model.onnx> [iterations]\n", argv[0]);
    return 1;
  }
  const std::string model_path = argv[1];
  const int iterations = argc >= 3 ? std::atoi(argv[2]) : 2000;
  constexpr int kWarmupIterations = 50;

  tanks::RLPolicy policy(model_path, tanks::RLEnvironment::OBSERVATION_SIZE,
                          tanks::RLEnvironment::ACTION_COUNT);

  std::mt19937 rng(0);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  std::vector<float> observation(
      static_cast<size_t>(tanks::RLEnvironment::OBSERVATION_SIZE));
  for (auto& v : observation) {
    v = dist(rng);
  }

  // First few ONNX Runtime calls pay for lazy graph optimization / memory
  // arena setup that a steady-state game loop never pays again — warm up
  // before timing so the benchmark reflects sustained per-tick cost, not
  // one-time startup cost (which matters for load time, but is a
  // different number than "per decision step" latency).
  for (int i = 0; i < kWarmupIterations; ++i) {
    policy.infer(observation);
  }

  std::vector<double> latencies_ms;
  latencies_ms.reserve(static_cast<size_t>(iterations));
  for (int i = 0; i < iterations; ++i) {
    const auto start = std::chrono::high_resolution_clock::now();
    policy.infer(observation);
    const auto end = std::chrono::high_resolution_clock::now();
    latencies_ms.push_back(
        std::chrono::duration<double, std::milli>(end - start).count());
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());

  double total = 0.0;
  for (double v : latencies_ms) {
    total += v;
  }
  const double mean = total / static_cast<double>(latencies_ms.size());
  const double p50 = latencies_ms[latencies_ms.size() / 2];
  const double p99 =
      latencies_ms[static_cast<size_t>(
          static_cast<double>(latencies_ms.size()) * 0.99)];
  const double max_latency = latencies_ms.back();

  std::printf("model:      %s\n", model_path.c_str());
  std::printf("iterations: %d (after %d warm-up calls)\n", iterations,
              kWarmupIterations);
  std::printf("mean:       %.4f ms\n", mean);
  std::printf("p50:        %.4f ms\n", p50);
  std::printf("p99:        %.4f ms\n", p99);
  std::printf("max:        %.4f ms\n", max_latency);
  return 0;
}