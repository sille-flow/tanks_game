#pragma once

#include <memory>
#include <string>
#include <vector>

namespace tanks {

/**
 * Loads a policy exported by export_ppo_to_onnx.py and runs inference to
 * produce a discrete action index compatible with RLEnvironment::decode_action().
 *
 * This class wraps ONNX Runtime entirely; nothing in tanks_sim
 * (Court/Tank/Bullet) or even this header knows ONNX Runtime types exist —
 * all Ort:: usage is confined to rl-inference.cpp via a pImpl. That's why
 * CMake links onnxruntime::onnxruntime as PRIVATE to tanks_rl_inference:
 * anything that includes just this header (e.g. game/ code) does not need
 * ONNX Runtime's own include directories.
 *
 * Expected model shape: input (batch, observation_size) float32 ->
 * output (batch, action_count) float32 logits, matching what
 * export_ppo_to_onnx.py produces for a Discrete action space. This class
 * always argmaxes the logits (deterministic policy) — see infer() below.
 *
 * Thread-safety: a single RLPolicy instance is NOT safe to call
 * concurrently from multiple threads. Construct one instance per
 * thread/opponent if you need concurrent inference (e.g. local coop vs.
 * two independent RL opponents).
 */
class RLPolicy {
 public:
  /**
   * @param onnx_model_path Path to a .onnx file produced by
   *        export_ppo_to_onnx.py.
   * @param observation_size Expected length of the observation vector
   *        passed to infer(). Pass RLEnvironment::OBSERVATION_SIZE.
   * @param action_count Expected number of discrete actions (logits) the
   *        model outputs. Pass RLEnvironment::ACTION_COUNT.
   * @param num_threads ONNX Runtime intra-op thread count. 1 is a
   *        reasonable default for a single opponent running alongside a
   *        real-time game loop; raise it only if profiling shows
   *        inference is the bottleneck and you have cores to spare.
   *
   * Throws std::runtime_error if the model file can't be loaded, doesn't
   * have exactly one input and one output tensor, or its declared
   * input/output shapes don't match observation_size/action_count.
   */
  RLPolicy(const std::string& onnx_model_path, int observation_size,
           int action_count, int num_threads = 1);

  ~RLPolicy();

  RLPolicy(const RLPolicy&) = delete;
  RLPolicy& operator=(const RLPolicy&) = delete;
  RLPolicy(RLPolicy&&) noexcept;
  RLPolicy& operator=(RLPolicy&&) noexcept;

  /**
   * Runs the policy on a single observation and returns the argmax action
   * index — i.e. exactly what you'd pass to
   * RLEnvironment::step()/decode_action().
   *
   * observation.size() must equal the observation_size given at
   * construction time; throws std::invalid_argument otherwise.
   */
  int infer(const std::vector<float>& observation) const;

  /**
   * Same as infer(), but also writes the raw logits (pre-argmax) into
   * out_logits. Useful for debugging, logging model confidence, or
   * building an epsilon-greedy / temperature-sampled opponent on top of
   * this later without touching the ONNX Runtime plumbing.
   */
  int infer(const std::vector<float>& observation,
            std::vector<float>& out_logits) const;

  int observation_size() const { return observation_size_; }
  int action_count() const { return action_count_; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  int observation_size_;
  int action_count_;
};

}  // namespace tanks