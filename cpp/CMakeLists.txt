cmake_minimum_required(VERSION 3.16)
project(gs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CLIENT "Build the raylib graphical client (gs)" ON)
option(BUILD_WEBSERVER "Build the Boost.Beast websocket echo server" OFF)
option(BUILD_RL "Build the pybind11 RL module" OFF)
option(BUILD_ONNX_INFERENCE "Build the ONNX Runtime C++ RL inference module" OFF)
option(BUILD_TESTING "Build unit tests" ON)

# --- Transport-agnostic simulation core (no raylib) ---
set(GAME_SIM_SOURCES
  game/sim/game_obj.cpp
  game/sim/bullet.cpp
  game/sim/tank.cpp
  game/sim/court.cpp
)

add_library(tanks_sim STATIC ${GAME_SIM_SOURCES})
target_include_directories(tanks_sim PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/game")

# Headless RL env (no pybind). Always available for tests / future ONNX play.
# gs links this unconditionally too (see BUILD_CLIENT block below) — it's a
# small, ONNX-Runtime-free static lib providing RLEnvironment::decode_action
# and RLEnvironment::build_observation, which the --vs-rl game mode needs
# regardless of whether ONNX inference itself is enabled for this build.
add_library(tanks_rl STATIC rl/rl-environment.cpp)
target_link_libraries(tanks_rl PUBLIC tanks_sim)
target_include_directories(tanks_rl PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/game"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin")

# --- Raylib (optional client; find_package / RAYLIB_ROOT / local tree — no FetchContent) ---
set(GS_HAS_RAYLIB FALSE)

if(BUILD_CLIENT)
  find_package(raylib QUIET)

  if(NOT raylib_FOUND)
    if(WIN32)
      set(RAYLIB_ROOT_DEFAULT "C:/raylib/raylib")
    else()
      set(RAYLIB_ROOT_DEFAULT "/usr/local")
    endif()
    set(RAYLIB_ROOT "${RAYLIB_ROOT_DEFAULT}" CACHE PATH
        "Path to raylib source/install root (contains src/raylib.h or CMakeLists.txt)")

    set(_raylib_header "")
    if(EXISTS "${RAYLIB_ROOT}/src/raylib.h")
      set(_raylib_header "${RAYLIB_ROOT}/src/raylib.h")
    elseif(EXISTS "${RAYLIB_ROOT}/include/raylib.h")
      set(_raylib_header "${RAYLIB_ROOT}/include/raylib.h")
    endif()

    # Prefer a prebuilt static library with a platform-correct name.
    set(_raylib_candidates
      "${RAYLIB_ROOT}/src/libraylib.a"
      "${RAYLIB_ROOT}/src/raylib.lib"
      "${RAYLIB_ROOT}/lib/libraylib.a"
      "${RAYLIB_ROOT}/lib/raylib.lib"
    )
    set(_raylib_lib "")
    foreach(_cand IN LISTS _raylib_candidates)
      if(EXISTS "${_cand}")
        set(_raylib_lib "${_cand}")
        break()
      endif()
    endforeach()

    if(_raylib_lib AND _raylib_header)
      get_filename_component(_raylib_inc_dir "${_raylib_header}" DIRECTORY)
      add_library(raylib STATIC IMPORTED GLOBAL)
      set_target_properties(raylib PROPERTIES
        IMPORTED_LOCATION "${_raylib_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${_raylib_inc_dir}"
      )
      if(WIN32)
        target_link_libraries(raylib INTERFACE opengl32 gdi32 winmm)
      elseif(APPLE)
        find_library(COCOA_FRAMEWORK Cocoa)
        find_library(OPENGL_FRAMEWORK OpenGL)
        find_library(IOKIT_FRAMEWORK IOKit)
        find_library(COREVIDEO_FRAMEWORK CoreVideo)
        target_link_libraries(raylib INTERFACE
          ${COCOA_FRAMEWORK} ${OPENGL_FRAMEWORK} ${IOKIT_FRAMEWORK} ${COREVIDEO_FRAMEWORK}
        )
      else()
        find_package(Threads REQUIRED)
        target_link_libraries(raylib INTERFACE GL m pthread dl rt X11)
      endif()
      set(GS_HAS_RAYLIB TRUE)
      message(STATUS "Using prebuilt raylib: ${_raylib_lib}")
    elseif(EXISTS "${RAYLIB_ROOT}/CMakeLists.txt")
      set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
      set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
      add_subdirectory("${RAYLIB_ROOT}" "${CMAKE_BINARY_DIR}/_deps/raylib")
      set(GS_HAS_RAYLIB TRUE)
      message(STATUS "Building raylib from source: ${RAYLIB_ROOT}")
    else()
      message(STATUS
        "raylib not found (looked via find_package and RAYLIB_ROOT=${RAYLIB_ROOT}). "
        "Skipping gs client. Install raylib or pass -DRAYLIB_ROOT=...")
    endif()
  else()
    set(GS_HAS_RAYLIB TRUE)
    message(STATUS "Using system raylib from find_package")
  endif()
endif()

if(BUILD_CLIENT AND GS_HAS_RAYLIB)
  add_executable(gs game/main.cpp)
  # tanks_rl is always built (see above) and has no ONNX Runtime dependency
  # of its own, so gs can always link it — this gives the client
  # RLEnvironment::decode_action / build_observation for the --vs-rl mode's
  # action-decoding and observation-building, independent of whether ONNX
  # inference itself (tanks_rl_inference) is enabled for this build.
  target_link_libraries(gs PRIVATE tanks_sim tanks_rl raylib)
elseif(BUILD_CLIENT AND NOT GS_HAS_RAYLIB)
  message(STATUS "BUILD_CLIENT=ON but raylib unavailable — gs not built")
endif()

# --- Optional webserver (Boost.Beast required when enabled) ---
if(BUILD_WEBSERVER)
  find_package(Threads REQUIRED)
  find_package(Boost REQUIRED COMPONENTS system)
  add_executable(webserver webserver/websocket.cpp)
  target_link_libraries(webserver PRIVATE Threads::Threads Boost::system)
endif()

# --- Optional Python bindings (pybind11) ---
if(BUILD_RL)
  find_package(Python COMPONENTS Interpreter Development REQUIRED)
  #find_package(pybind11 CONFIG REQUIRED)
  set(Python_FIND_VIRTUALENV FIRST)

  find_package(Python COMPONENTS Interpreter Development REQUIRED)

  # First try a normal system/package-manager installation.
  find_package(pybind11 CONFIG QUIET)

  # If pybind11 is not on CMake's search path, ask the selected Python
  # installation where its pybind11 CMake package lives.
  if(NOT pybind11_FOUND)
    execute_process(
      COMMAND "${Python_EXECUTABLE}" -m pybind11 --cmakedir
      RESULT_VARIABLE PYBIND11_RESULT
      OUTPUT_VARIABLE PYBIND11_CMAKE_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )

    if(PYBIND11_RESULT EQUAL 0 AND EXISTS "${PYBIND11_CMAKE_DIR}")
      #message(STATUS "pybind11 found at ${PYBIND11_CMAKE_DIR}")
      set(pybind11_DIR "${PYBIND11_CMAKE_DIR}")

      find_package(pybind11 CONFIG REQUIRED)
    else()
      message(FATAL_ERROR
        "pybind11 was not found.\n"
        "Install it in the Python environment being used by CMake:\n"
        "  ${Python_EXECUTABLE} -m pip install pybind11"
      )
    endif()
  endif()

  #message(STATUS "Using Python: ${Python_EXECUTABLE}")
  #message(STATUS "Using pybind11: ${pybind11_DIR}")
  pybind11_add_module(tanks_env_cpp rl/rl-bindings.cpp)
  target_link_libraries(tanks_env_cpp PRIVATE tanks_rl)
  set_target_properties(tanks_env_cpp PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../python"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../python"
  )
endif()

if(BUILD_ONNX_INFERENCE)
 
  # --- Step 1: normal package discovery (vcpkg / Conan / distro package) ---
  find_package(onnxruntime CONFIG QUIET)
 
  if(onnxruntime_FOUND)
    message(STATUS "Found ONNX Runtime via CMake package config.")
 
    # Different distributions expose slightly different target names;
    # normalize to a single alias we use everywhere below.
    if(TARGET onnxruntime::onnxruntime)
      # already the name we want
    elseif(TARGET onnxruntime)
      add_library(onnxruntime::onnxruntime ALIAS onnxruntime)
    endif()
 
  else()
    # --- Step 2: explicit root dir (official prebuilt release archive) ---
    if(NOT DEFINED ONNXRUNTIME_ROOT_DIR AND DEFINED ENV{ONNXRUNTIME_ROOT_DIR})
      set(ONNXRUNTIME_ROOT_DIR "$ENV{ONNXRUNTIME_ROOT_DIR}")
    endif()
 
    if(DEFINED ONNXRUNTIME_ROOT_DIR)
      message(STATUS "Looking for ONNX Runtime under ONNXRUNTIME_ROOT_DIR=${ONNXRUNTIME_ROOT_DIR}")
      find_path(ONNXRUNTIME_INCLUDE_DIR
        NAMES onnxruntime_cxx_api.h
        PATHS "${ONNXRUNTIME_ROOT_DIR}/include"
        NO_DEFAULT_PATH
      )
      find_library(ONNXRUNTIME_LIBRARY
        NAMES onnxruntime
        PATHS "${ONNXRUNTIME_ROOT_DIR}/lib"
        NO_DEFAULT_PATH
      )
    else()
      # --- Step 3: last-resort system search ---
      find_path(ONNXRUNTIME_INCLUDE_DIR NAMES onnxruntime_cxx_api.h)
      find_library(ONNXRUNTIME_LIBRARY NAMES onnxruntime)
    endif()
 
    if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
      message(STATUS "Found ONNX Runtime:")
      message(STATUS "  include: ${ONNXRUNTIME_INCLUDE_DIR}")
      message(STATUS "  library: ${ONNXRUNTIME_LIBRARY}")
 
      add_library(onnxruntime::onnxruntime UNKNOWN IMPORTED)
      set_target_properties(onnxruntime::onnxruntime PROPERTIES
        IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}"
      )
      set(onnxruntime_FOUND TRUE)
    else()
      message(FATAL_ERROR
        "BUILD_ONNX_INFERENCE is ON but ONNX Runtime could not be found.\n"
        "Either:\n"
        "  (a) install it via a package manager that provides a CMake config "
        "(vcpkg install onnxruntime, or Conan), or\n"
        "  (b) download the official prebuilt release from "
        "https://github.com/microsoft/onnxruntime/releases and set "
        "ONNXRUNTIME_ROOT_DIR to the extracted folder (the one containing "
        "include/ and lib/), e.g.:\n"
        "    cmake -B build -S cpp -DBUILD_ONNX_INFERENCE=ON "
        "-DONNXRUNTIME_ROOT_DIR=/path/to/onnxruntime-<platform>-<version>"
      )
    endif()
  endif()

  add_library(tanks_rl_inference STATIC
    rl/rl-inference.cpp
    # rl/rl-inference.hpp is header-only interface, doesn't need listing,
    # but keeping it alongside for IDE visibility is fine:
    # rl/rl-inference.hpp
  )
  # tanks_sim only publishes game/ as an include dir (see its own
  # target_include_directories above) — it doesn't cover the cpp/ root
  # that "rl/rl-inference.hpp"-style includes resolve against. Add it
  # explicitly rather than relying on tanks_sim or tanks_rl for it.
  target_include_directories(tanks_rl_inference PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  target_link_libraries(tanks_rl_inference
    PUBLIC
      tanks_sim
    PRIVATE
      onnxruntime::onnxruntime
  )
  target_compile_features(tanks_rl_inference PUBLIC cxx_std_17)

  add_executable(bench-inference rl/bench-inference.cpp)
  target_link_libraries(bench-inference PRIVATE tanks_rl tanks_rl_inference)

  # --- Wire ONNX inference into the graphical client (--vs-rl mode) ---
  # gs already links tanks_rl unconditionally (see BUILD_CLIENT block
  # above); here we additionally link tanks_rl_inference and tell
  # game/main.cpp (via GS_HAS_ONNX_INFERENCE) that it's safe to #include
  # rl/rl-inference.hpp and compile the --vs-rl code path. Guarded by
  # `if(TARGET gs)` since gs might not exist (BUILD_CLIENT=OFF, or raylib
  # wasn't found even though BUILD_CLIENT=ON).
  if(TARGET gs)
    target_link_libraries(gs PRIVATE tanks_rl_inference)
    target_compile_definitions(gs PRIVATE GS_HAS_ONNX_INFERENCE=1)

    # Windows-only: onnxruntime.dll must sit next to gs.exe at runtime —
    # static linking of tanks_rl_inference doesn't apply to ONNX Runtime's
    # own shared library. Only wired up when we resolved ONNX Runtime via
    # an explicit root dir (Step 2/3 above); if it came from a CMake
    # package config (Step 1), that package is responsible for its own
    # runtime deployment story and ONNXRUNTIME_ROOT_DIR won't be set here.
    if(WIN32 AND DEFINED ONNXRUNTIME_ROOT_DIR)
      add_custom_command(TARGET gs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ONNXRUNTIME_ROOT_DIR}/lib/onnxruntime.dll"
                "$<TARGET_FILE_DIR:gs>"
        COMMENT "Copying onnxruntime.dll next to gs.exe"
      )
    endif()
  else()
    message(STATUS
      "BUILD_ONNX_INFERENCE=ON but the gs target doesn't exist "
      "(BUILD_CLIENT=OFF, or raylib wasn't found) — --vs-rl mode won't be "
      "compiled into anything this run.")
  endif()
 
endif() # BUILD_ONNX_INFERENCE

# --- Unit tests (headless sim + RL env; no third-party test framework) ---
if(BUILD_TESTING)
  enable_testing()
  add_executable(tanks_tests
    tests/test_main.cpp
    tests/test_game_obj.cpp
    tests/test_tank.cpp
    tests/test_bullet.cpp
    tests/test_court.cpp
    tests/test_rl_environment.cpp
  )
  target_link_libraries(tanks_tests PRIVATE tanks_sim tanks_rl)
  target_include_directories(tanks_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  add_test(NAME tanks_tests COMMAND tanks_tests)
endif()