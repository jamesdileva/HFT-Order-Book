// GUI.hpp
#pragma once

#include "SimState.hpp"

class GUI {
public:
    // Returns false if the window was closed
    bool init(const char* title, int width, int height);

    // Render one frame — returns false when user closes window
    bool render(SimState& state);

    void shutdown();

private:
    void drawOrderBook(SimState& state);
    void drawStatsAndHistogram(SimState& state);
    void drawFillFeed(SimState& state);
    void drawControls(SimState& state);

    void* window_  = nullptr;
    void* glctx_   = nullptr;
};