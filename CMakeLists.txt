cmake_minimum_required(VERSION 3.20)
project(OrderbookEngine VERSION 1.0.0 LANGUAGES CXX)

# ── Language standard ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)   # stick to the ISO standard, no GNU extensions

# ── Default build type ───────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Compiler optimisation flags ──────────────────────────────────────────────
include(cmake/CompilerFlags.cmake)

# ── Sub-projects ─────────────────────────────────────────────────────────────
add_subdirectory(src)
add_subdirectory(benchmarks)
add_subdirectory(tests)
