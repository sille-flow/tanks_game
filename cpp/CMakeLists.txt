cmake_minimum_required(VERSION 3.16)
project(gs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CLIENT "Build the raylib graphical client (gs)" ON)
option(BUILD_WEBSERVER "Build the Boost.Beast websocket echo server" OFF)
option(BUILD_RL "Build the pybind11 RL module" OFF)
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
  target_link_libraries(gs PRIVATE tanks_sim raylib)
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

# --- Optional RL module (pybind11; paths fixed for later enablement) ---
if(BUILD_RL)
  find_package(Python COMPONENTS Interpreter Development REQUIRED)
  find_package(pybind11 CONFIG REQUIRED)
  pybind11_add_module(tanks_env_cpp
    rl/rl-bindings.cpp
    rl/rl-environment.cpp
    ${GAME_SIM_SOURCES}
  )
  target_include_directories(tanks_env_cpp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/game"
  )
endif()

# --- Unit tests (headless sim only; no third-party test framework) ---
if(BUILD_TESTING)
  enable_testing()
  add_executable(tanks_tests
    tests/test_main.cpp
    tests/test_game_obj.cpp
    tests/test_tank.cpp
    tests/test_bullet.cpp
    tests/test_court.cpp
  )
  target_link_libraries(tanks_tests PRIVATE tanks_sim)
  target_include_directories(tanks_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  add_test(NAME tanks_tests COMMAND tanks_tests)
endif()
