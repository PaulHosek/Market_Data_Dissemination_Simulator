cmake_minimum_required(VERSION 3.31)
project(MarketDataDisseminationSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED COMPONENTS lockfree)
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

include_directories(include)
include_directories(src)

# --- Main Executable ---
add_executable(main_simulate
        src/main.cpp
        src/feedhandler/FeedHandler.h
        src/disseminator/ZmqDisseminator.h
        src/generator/MarketDataGenerator.cpp
)

target_link_libraries(main_simulate
        PRIVATE Boost::lockfree
        PRIVATE spdlog::spdlog
        PRIVATE cppzmq
)

# --- Tests ---
enable_testing()

add_executable(generator_tests
        tests/test_sanity_basic.cpp
        tests/test_generator.cpp
        tests/test_waitable_queue.cpp
        tests/test_disseminator.cpp
        tests/test_subscriber_interface.cpp
        src/generator/MarketDataGenerator.cpp
        src/feedhandler/FeedHandler.h
        src/disseminator/ZmqDisseminator.h
)

target_link_libraries(generator_tests
        PRIVATE GTest::gtest_main
        PRIVATE Boost::lockfree
        PRIVATE spdlog::spdlog
        PRIVATE cppzmq
)

gtest_discover_tests(generator_tests)

set_source_files_properties(
        src/generator/IGenerator.h
        src/utils/types.h
        PROPERTIES HEADER_FILE_ONLY TRUE
)