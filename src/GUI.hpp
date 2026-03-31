// GUI.hpp
#pragma once

#include "GameManager.hpp"

class GUI {
public:
    bool init(const char* title, int width, int height);
    bool render(GameManager& gm);
    void shutdown();

private:
    void drawStockPicker   (GameManager& gm, GameState& gs);
    void drawTradingScreen (GameManager& gm, GameState& gs);
    void drawPlayerHUD     (GameManager& gm, GameState& gs);
    void drawNewsBanners   (GameState& gs);
    void drawOrderBook     (GameState& gs);
    void drawPnlAndStats   (GameState& gs);
    void drawFillFeed      (GameState& gs);
    void drawTradingControls(GameManager& gm, GameState& gs);
    void drawSessionEnd    (GameManager& gm, GameState& gs);
    void drawLeaderboard   (GameState& gs);
    void drawMainMenu    (GameManager& gm, GameState& gs);
    void drawTradingReady(GameManager& gm, GameState& gs);
    void drawBenchmark(GameManager& gm, GameState& gs);
    void*  window_  = nullptr;
    void*  glctx_   = nullptr;
    char   nameBuffer_[32] = "TRADER_01";
};