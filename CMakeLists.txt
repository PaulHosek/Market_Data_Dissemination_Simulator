cmake_minimum_required(VERSION 3.22)
project(MarketDataSimulator LANGUAGES CXX)

# Set C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure vcpkg toolchain is used
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Set policy CMP0167 to NEW to avoid FindBoost warning
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find dependencies
find_package(ZeroMQ REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS lockfree)

# Debug output for dependencies
message(STATUS "ZeroMQ found: ${ZeroMQ_FOUND}")
message(STATUS "ZeroMQ include dir: ${ZeroMQ_INCLUDE_DIR}")
message(STATUS "ZeroMQ library: ${ZeroMQ_LIBRARY}")
message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")

# Add source files
add_executable(simulator src/main.cpp)

# Include directories
target_include_directories(simulator PRIVATE include ${ZeroMQ_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

# Link libraries
target_link_libraries(simulator PRIVATE ${ZeroMQ_LIBRARY} spdlog::spdlog GTest::gtest)

# Compiler flags for warnings
if(MSVC)
    target_compile_options(simulator PRIVATE /W4 /WX)
else()
    target_compile_options(simulator PRIVATE -Wall -Wextra -Werror)
endif()

# Add test executable
add_executable(unit_tests tests/test_basic.cpp)
target_include_directories(unit_tests PRIVATE include ${ZeroMQ_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
target_link_libraries(unit_tests PRIVATE ${ZeroMQ_LIBRARY} spdlog::spdlog GTest::gtest GTest::gtest_main)
add_test(NAME UnitTests COMMAND unit_tests)

# Enable testing
enable_testing()