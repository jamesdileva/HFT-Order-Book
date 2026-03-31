cmake_minimum_required(VERSION 3.16)
project(HFTOrderBook CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

set(CMAKE_PREFIX_PATH "C:/msys64/mingw64")

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/Benchmark.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
    src/OrderGenerator.cpp
    src/LatencyProfiler.cpp
    src/GUI.cpp
    src/imgui.cpp
    src/imgui_draw.cpp
    src/imgui_tables.cpp
    src/imgui_widgets.cpp
    src/imgui_impl_sdl2.cpp
    src/imgui_impl_opengl3.cpp
    src/GameManager.cpp
)

add_executable(hft ${SOURCES})

if(WIN32)
    set_target_properties(hft PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(hft PRIVATE
    src
    C:/msys64/mingw64/include/SDL2
)

# ── Link order matters — SDL2main must come before SDL2 ──────────────────────
target_link_libraries(hft PRIVATE
    -lmingw32
    -lSDL2main
    -lSDL2
    opengl32
    -static-libgcc
    -static-libstdc++
)