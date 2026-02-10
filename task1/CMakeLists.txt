# ==============================
# PROJECT DESCRIPTION
# ==============================
cmake_minimum_required(VERSION 3.15...4.0)
project(task1 LANGUAGES CXX)

# ==============================
# PROJECT PROPERTIES
# ==============================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

# ==============================
# C++ LIBRARY (CORE IMPLEMENTATION)
# ==============================
add_library(driver_facade STATIC
  ${CMAKE_SOURCE_DIR}/src/driver_facade.cpp
)

target_include_directories(driver_facade PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

# ==============================
# MAIN EXECUTABLE (C++)
# ==============================
add_executable(driver_facade_impl
  ${CMAKE_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(driver_facade_impl PRIVATE driver_facade)
target_compile_options(driver_facade_impl PRIVATE)
target_link_options(driver_facade_impl PRIVATE)

# ==============================
# TESTS (C++)
# ==============================
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_driver_facade
  ${CMAKE_SOURCE_DIR}/tests/test_driver_facade.cpp
)

target_link_libraries(test_driver_facade
  PRIVATE
    driver_facade
    GTest::gtest_main
)

target_include_directories(test_driver_facade PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(test_driver_facade)

