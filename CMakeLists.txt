cmake_minimum_required(VERSION 3.20)
project(SimulateDataFeed LANGUAGES CXX)

set(COMMON_HEADERS
    src/market_feed_simulator.h
)

set(BIN1_SOURCES
    src/market_feed_simulator.cpp
    src/data_feed_simulator.cpp
)

set(BIN2_SOURCES
    src/market_feed_receiver.cpp
)

add_executable(SimulateDataFeed ${BIN1_SOURCES} ${COMMON_HEADERS})
add_executable(Receiver ${BIN2_SOURCES} ${COMMON_HEADERS})

target_include_directories(SimulateDataFeed PRIVATE src)
target_include_directories(Receiver PRIVATE src)

target_compile_features(SimulateDataFeed PRIVATE cxx_std_20)
target_compile_features(Receiver PRIVATE cxx_std_20)

set_target_properties(SimulateDataFeed PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
