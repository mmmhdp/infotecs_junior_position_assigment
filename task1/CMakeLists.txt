cmake_minimum_required(VERSION 3.15...4.0)
project(task1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

add_library(driver_facade STATIC
  ${CMAKE_SOURCE_DIR}/src/driver_facade.cpp
)

target_include_directories(driver_facade PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

add_executable(driver_facade_impl
  ${CMAKE_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(driver_facade_impl PRIVATE driver_facade)
target_compile_options(driver_facade_impl PRIVATE)
target_link_options(driver_facade_impl PRIVATE)
