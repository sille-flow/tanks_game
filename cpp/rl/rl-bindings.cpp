#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "rl/rl-environment.hpp"

namespace py = pybind11;

PYBIND11_MODULE(tanks_env_cpp, m) {
  m.doc() = "C++ reinforcement learning environment for 1200 Tanks";

  py::enum_<tanks::OpponentDifficulty>(m, "OpponentDifficulty")
      .value("Easy", tanks::OpponentDifficulty::Easy)
      .value("Medium", tanks::OpponentDifficulty::Medium)
      .value("Hard", tanks::OpponentDifficulty::Hard);

  py::class_<tanks::OpponentConfig>(m, "OpponentConfig")
      .def(py::init<>())
      .def_static("for_difficulty", &tanks::OpponentConfig::for_difficulty,
                  py::arg("difficulty"))
      .def_readwrite("fire_interval", &tanks::OpponentConfig::fire_interval)
      .def_readwrite("ice_interval", &tanks::OpponentConfig::ice_interval)
      .def_readwrite("poison_interval", &tanks::OpponentConfig::poison_interval)
      .def_readwrite("position_tolerance",
                     &tanks::OpponentConfig::position_tolerance)
      .def_readwrite("ice_range", &tanks::OpponentConfig::ice_range)
      .def_readwrite("poison_range", &tanks::OpponentConfig::poison_range);

  py::class_<tanks::RLEnvironment>(m, "RLEnvironment")
      .def(py::init<>(), "Create env with medium scripted opponent.")
      .def(py::init<tanks::OpponentConfig>(), py::arg("opponent"),
           "Create env with a custom scripted opponent config.")
      .def("reset", &tanks::RLEnvironment::reset)
      .def("random_reset", &tanks::RLEnvironment::random_reset, py::arg("seed"))
      .def("step", &tanks::RLEnvironment::step, py::arg("action"))
      .def("observation", &tanks::RLEnvironment::observation)
      .def("reward", &tanks::RLEnvironment::reward)
      .def("done", &tanks::RLEnvironment::done)
      .def_static("decode_action", &tanks::RLEnvironment::decode_action,
           py::arg("action"))
      .def("set_opponent_config", &tanks::RLEnvironment::set_opponent_config,
           py::arg("config"));

  m.attr("ACTION_COUNT") = tanks::RLEnvironment::ACTION_COUNT;
  m.attr("OBSERVATION_SIZE") = tanks::RLEnvironment::OBSERVATION_SIZE;
}
