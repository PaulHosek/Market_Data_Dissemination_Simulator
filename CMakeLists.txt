cmake_minimum_required(VERSION 3.28.3)
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
        src/feedhandler/ZmqFeedHandler.h
        src/disseminator/ZmqDisseminator.h
        src/generator/RandomWalkGenerator.h
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
        tests/test_RandomWalkGenerator.cpp
        tests/test_waitable_queue.cpp
        tests/test_ZmqDisseminator.cpp
        tests/test_ZmqFeedhandler.cpp
        src/generator/RandomWalkGenerator.h
        src/feedhandler/ZmqFeedHandler.h
        src/disseminator/ZmqDisseminator.h
        src/utils/SpinSpscQueue.h
        tests/test_integration_zmq_disseminator_feedhandler.cpp
        src/monitor/LatencyMonitor.h
        src/utils/CustomSpscQueue.h
        src/utils/QueueConcepts.h
        src/disseminator/UdpDisseminator.h
)

target_link_libraries(generator_tests
        PRIVATE GTest::gtest_main
        PRIVATE Boost::lockfree
        PRIVATE spdlog::spdlog
        PRIVATE cppzmq
)

gtest_discover_tests(generator_tests)

set_source_files_properties(
        src/generator/BaseGenerator.h
        src/utils/types.h
        PROPERTIES HEADER_FILE_ONLY TRUE
)