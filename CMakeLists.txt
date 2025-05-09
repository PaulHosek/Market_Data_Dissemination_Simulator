cmake_minimum_required(VERSION 3.22)
project(MarketDataSimulator LANGUAGES CXX)

# Set C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure vcpkg toolchain is used (only if not set by command line)
if(NOT CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
elseif(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Debug vcpkg toolchain
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

# Set policy CMP0167 to NEW to avoid FindBoost warning
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find dependencies
# Attempt to find libzmq via find_package
find_package(libzmq CONFIG QUIET)
if(NOT libzmq_FOUND)
  message(WARNING "libzmq not found via find_package, attempting fallback")
  find_library(libzmq_LIBRARY NAMES libzmq zmq PATHS "$ENV{VCPKG_ROOT}/installed/x64-mingw-dynamic/lib" "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-mingw-dynamic/lib")
  find_path(libzmq_INCLUDE_DIR NAMES zmq.h PATHS "$ENV{VCPKG_ROOT}/installed/x64-mingw-dynamic/include" "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-mingw-dynamic/include")
  if(NOT libzmq_LIBRARY OR NOT libzmq_INCLUDE_DIR)
    message(FATAL_ERROR "libzmq not found. Library: ${libzmq_LIBRARY}, Include: ${libzmq_INCLUDE_DIR}")
  endif()
endif()

find_package(spdlog REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS lockfree)

# Debug output for dependencies
message(STATUS "libzmq found: ${libzmq_FOUND}")
message(STATUS "libzmq include dir: ${libzmq_INCLUDE_DIR}")
message(STATUS "libzmq library: ${libzmq_LIBRARY}")
message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
message(STATUS "spdlog found: ${spdlog_FOUND}")
message(STATUS "GTest found: ${GTest_FOUND}")

# Add source files
add_executable(simulator src/main.cpp)

# Include directories
target_include_directories(simulator PRIVATE include ${libzmq_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

# Link libraries
target_link_libraries(simulator PRIVATE ${libzmq_LIBRARY} spdlog::spdlog GTest::gtest)

# Compiler flags for warnings
if(MSVC)
    target_compile_options(simulator PRIVATE /W4 /WX)
else()
    target_compile_options(simulator PRIVATE -Wall -Wextra -Werror)
endif()

# Add test executable
add_executable(unit_tests tests/test_basic.cpp)
target_include_directories(unit_tests PRIVATE include ${libzmq_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
target_link_libraries(unit_tests PRIVATE ${libzmq_LIBRARY} spdlog::spdlog GTest::gtest GTest::gtest_main)
add_test(NAME UnitTests COMMAND unit_tests)

# Enable testing
enable_testing()