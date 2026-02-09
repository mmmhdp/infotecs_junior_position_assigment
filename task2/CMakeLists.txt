# ==============================
# PROJECT DESCRIPTION
# ==============================
cmake_minimum_required(VERSION 3.15...4.0)
project(task2 LANGUAGES C CXX)

# ==============================
# PROJECT PROPERTIES
# ==============================
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

# ==============================
# C LIBRARY (CORE IMPLEMENTATION)
# ==============================
add_library(mymem STATIC
  ${CMAKE_SOURCE_DIR}/src/mymem.c
)

target_include_directories(mymem PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

# Force C linkage (important when linking from C++)
set_target_properties(mymem PROPERTIES
  LINKER_LANGUAGE C
)

# ==============================
# MAIN EXECUTABLE (C)
# ==============================
add_executable(mymem_impl
  ${CMAKE_SOURCE_DIR}/src/main.c
)
target_link_libraries(mymem_impl PRIVATE mymem)
target_compile_options(mymem_impl PRIVATE)
target_link_options(mymem_impl PRIVATE)

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

add_executable(test_mymem
  ${CMAKE_SOURCE_DIR}/tests/test_mymem.cpp
)

target_link_libraries(test_mymem
  PRIVATE
    mymem
    GTest::gtest_main
)

target_include_directories(test_mymem PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(test_mymem)

