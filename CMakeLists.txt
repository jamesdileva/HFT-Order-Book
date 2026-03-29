cmake_minimum_required(VERSION 3.16)
project(HFTOrderBook CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

add_executable(hft
    src/main.cpp
    src/Benchmark.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
    src/OrderGenerator.cpp
    src/LatencyProfiler.cpp
)

target_include_directories(hft PRIVATE src)

# Windows threading fix
if(WIN32)
    target_link_libraries(hft PRIVATE -static-libgcc -static-libstdc++)
endif()