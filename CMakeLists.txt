cmake_minimum_required(VERSION 3.27.8)
project(MarketDataDisseminationSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

include_directories(include)
include_directories(src)

# --- Main Executable ---
add_executable(main_simulate
        src/main.cpp
        src/generator/RandomWalkGenerator.h
        src/feedhandler/ZmqFeedHandler.h
        src/disseminator/ZmqDisseminator.h
        src/utils/SpinSpscQueue.h
        src/monitor/LatencyMonitor.h
        src/utils/CustomSpscQueue.h
        src/utils/QueueConcepts.h
        src/disseminator/UdpDisseminator.h
        src/feedhandler/UdpFeedHandler.h
        src/utils/config.h
)

target_link_libraries(main_simulate
        PRIVATE Boost::lockfree
        PRIVATE spdlog::spdlog
        PRIVATE cppzmq
        PRIVATE cxxopts::cxxopts
)

# --- Tests ---
enable_testing()

add_executable(tests
        src/generator/RandomWalkGenerator.h
        src/feedhandler/ZmqFeedHandler.h
        src/disseminator/ZmqDisseminator.h
        src/utils/SpinSpscQueue.h
        src/monitor/LatencyMonitor.h
        src/utils/CustomSpscQueue.h
        src/utils/QueueConcepts.h
        src/disseminator/UdpDisseminator.h
        src/feedhandler/UdpFeedHandler.h
        src/utils/config.h
        tests/test_integration_zmq_disseminator_feedhandler.cpp
        tests/test_UdpDisseminator.cpp
        tests/test_UdpFeedHandler.cpp
        tests/test_sanity_basic.cpp
        tests/test_RandomWalkGenerator.cpp
        tests/test_waitable_queue.cpp
        tests/test_ZmqDisseminator.cpp
        tests/test_integration_udp.cpp
)

target_link_libraries(tests
        PRIVATE GTest::gtest_main
        PRIVATE Boost::lockfree
        PRIVATE spdlog::spdlog
        PRIVATE cppzmq
        PRIVATE cxxopts::cxxopts
)

gtest_discover_tests(tests)

set_source_files_properties(
        src/generator/BaseGenerator.h
        src/utils/types.h
        PROPERTIES HEADER_FILE_ONLY TRUE
)