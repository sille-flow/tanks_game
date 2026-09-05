cmake_minimum_required(VERSION 3.16)

project(gs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- local raylib (Windows installer default: C:/raylib/raylib) ---
set(RAYLIB_ROOT "C:/raylib/raylib" CACHE PATH "Path to raylib source root (contains src/raylib.h)")
if(NOT EXISTS "${RAYLIB_ROOT}/src/raylib.h")
  message(FATAL_ERROR
    "raylib not found at RAYLIB_ROOT=${RAYLIB_ROOT}\n"
    "Install raylib or pass -DRAYLIB_ROOT=...")
endif()

# Prefer the installer's prebuilt static library; fall back to building from source.
set(RAYLIB_STATIC_LIB "${RAYLIB_ROOT}/src/libraylib.a")
if(EXISTS "${RAYLIB_STATIC_LIB}")
  add_library(raylib STATIC IMPORTED GLOBAL)
  set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION "${RAYLIB_STATIC_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_ROOT}/src"
  )
  if(WIN32)
    target_link_libraries(raylib INTERFACE opengl32 gdi32 winmm)
  endif()
  message(STATUS "Using prebuilt raylib: ${RAYLIB_STATIC_LIB}")
else()
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  add_subdirectory("${RAYLIB_ROOT}" "${CMAKE_BINARY_DIR}/_deps/raylib")
  message(STATUS "Building raylib from source: ${RAYLIB_ROOT}")
endif()

# --- transport-agnostic tanks sim + local coop client ---
set(GAME_SIM_SOURCES
  game/sim/game_obj.cpp
  game/sim/bullet.cpp
  game/sim/tank.cpp
  game/sim/court.cpp
)

add_executable(gs
  game/main.cpp
  ${GAME_SIM_SOURCES}
)
target_include_directories(gs PRIVATE game)
target_link_libraries(gs PRIVATE raylib)

# Keep the existing Boost.Beast echo server target.
add_executable(webserver webserver/websocket.cpp)
