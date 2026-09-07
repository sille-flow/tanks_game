#include "rl/rl-inference.hpp"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>

namespace tanks {

struct RLPolicy::Impl {
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "tanks_rl_policy"};
  Ort::SessionOptions session_options;
  std::unique_ptr<Ort::Session> session;
  Ort::MemoryInfo memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  std::string input_name;
  std::string output_name;
};

namespace {

// Shape is expected to look like {batch, feature}. The batch dim is
// exported as dynamic (see export_ppo_to_onnx.py's dynamic_axes), which
// ONNX Runtime reports as <= 0, so only the trailing "feature" dim is
// actually checked against what the caller expects.
bool trailing_dim_matches(const std::vector<int64_t>& shape, int expected) {
  if (shape.empty()) return false;
  const int64_t last = shape.back();
  return last <= 0 || last == static_cast<int64_t>(expected);
}

std::string shape_to_string(const std::vector<int64_t>& shape) {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < shape.size(); ++i) {
    oss << shape[i];
    if (i + 1 < shape.size()) oss << ", ";
  }
  oss << "]";
  return oss.str();
}

}  // namespace

RLPolicy::RLPolicy(const std::string& onnx_model_path, int observation_size,
                    int action_count, int num_threads)
    : impl_(std::make_unique<Impl>()),
      observation_size_(observation_size),
      action_count_(action_count) {
  impl_->session_options.SetIntraOpNumThreads(num_threads);
  impl_->session_options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_ALL);

  try {
#ifdef _WIN32
    // ORT's Windows API takes a wide-char path.
    const std::wstring wide_path(onnx_model_path.begin(),
                                  onnx_model_path.end());
    impl_->session = std::make_unique<Ort::Session>(
        impl_->env, wide_path.c_str(), impl_->session_options);
#else
    impl_->session = std::make_unique<Ort::Session>(
        impl_->env, onnx_model_path.c_str(), impl_->session_options);
#endif
  } catch (const Ort::Exception& e) {
    throw std::runtime_error("Failed to load ONNX model '" +
                              onnx_model_path + "': " + e.what());
  }

  Ort::AllocatorWithDefaultOptions allocator;

  if (impl_->session->GetInputCount() != 1) {
    throw std::runtime_error(
        "RLPolicy expects exactly one input tensor (the observation "
        "vector); model '" +
        onnx_model_path +
        "' has: " + std::to_string(impl_->session->GetInputCount()));
  }
  if (impl_->session->GetOutputCount() != 1) {
    throw std::runtime_error(
        "RLPolicy expects exactly one output tensor (action logits); "
        "model '" +
        onnx_model_path +
        "' has: " + std::to_string(impl_->session->GetOutputCount()));
  }

  auto input_name_ptr = impl_->session->GetInputNameAllocated(0, allocator);
  auto output_name_ptr = impl_->session->GetOutputNameAllocated(0, allocator);
  impl_->input_name = input_name_ptr.get();
  impl_->output_name = output_name_ptr.get();

  const auto input_shape = impl_->session->GetInputTypeInfo(0)
                                .GetTensorTypeAndShapeInfo()
                                .GetShape();
  const auto output_shape = impl_->session->GetOutputTypeInfo(0)
                                 .GetTensorTypeAndShapeInfo()
                                 .GetShape();

  if (!trailing_dim_matches(input_shape, observation_size_)) {
    throw std::runtime_error(
        "ONNX model input shape " + shape_to_string(input_shape) +
        " doesn't match expected observation_size=" +
        std::to_string(observation_size_) +
        ". Did this model come from a different "
        "RLEnvironment::OBSERVATION_SIZE, or a stale export?");
  }
  if (!trailing_dim_matches(output_shape, action_count_)) {
    throw std::runtime_error(
        "ONNX model output shape " + shape_to_string(output_shape) +
        " doesn't match expected action_count=" +
        std::to_string(action_count_) +
        ". Did this model come from a different "
        "RLEnvironment::ACTION_COUNT, or was it exported for a "
        "continuous action space instead of Discrete(72)?");
  }
}

RLPolicy::~RLPolicy() = default;
RLPolicy::RLPolicy(RLPolicy&&) noexcept = default;
RLPolicy& RLPolicy::operator=(RLPolicy&&) noexcept = default;

int RLPolicy::infer(const std::vector<float>& observation) const {
  std::vector<float> logits;
  return infer(observation, logits);
}

int RLPolicy::infer(const std::vector<float>& observation,
                     std::vector<float>& out_logits) const {
  if (static_cast<int>(observation.size()) != observation_size_) {
    throw std::invalid_argument(
        "RLPolicy::infer observation size mismatch: got " +
        std::to_string(observation.size()) +
        ", expected " + std::to_string(observation_size_) +
        " (RLEnvironment::OBSERVATION_SIZE)");
  }

  const std::array<int64_t, 2> input_shape{
      1, static_cast<int64_t>(observation_size_)};

  // CreateTensor wraps `observation`'s buffer rather than copying it, so
  // it must stay alive until Run() returns below. The const_cast is
  // required by ORT's non-owning constructor signature even though we
  // only ever read from it here.
  Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
      impl_->memory_info, const_cast<float*>(observation.data()),
      observation.size(), input_shape.data(), input_shape.size());

  const char* input_names[] = {impl_->input_name.c_str()};
  const char* output_names[] = {impl_->output_name.c_str()};

  auto output_tensors =
      impl_->session->Run(Ort::RunOptions{nullptr}, input_names,
                           &input_tensor, 1, output_names, 1);

  const float* logits_data = output_tensors[0].GetTensorData<float>();
  out_logits.assign(logits_data, logits_data + action_count_);

  const auto max_it = std::max_element(out_logits.begin(), out_logits.end());
  return static_cast<int>(std::distance(out_logits.begin(), max_it));
}

}  // namespace tanks