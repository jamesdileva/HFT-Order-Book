// GUI.cpp - full replacement
#include "GUI.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>
#include <string>
#include <algorithm>
#include <cmath>

// ── Theme ─────────────────────────────────────────────────────────────────────
static void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c     = s.Colors;

    s.WindowRounding     = 4.0f;
    s.FrameRounding      = 3.0f;
    s.ScrollbarRounding  = 3.0f;
    s.GrabRounding       = 3.0f;
    s.TabRounding        = 3.0f;
    s.FramePadding       = { 8, 5 };
    s.ItemSpacing        = { 8, 5 };
    s.WindowPadding      = { 12, 12 };
    s.ScrollbarSize      = 10.0f;
    s.WindowBorderSize   = 0.5f;
    s.FrameBorderSize    = 0.5f;
    s.PopupRounding      = 4.0f;

    c[ImGuiCol_WindowBg]             = { 0.07f, 0.08f, 0.09f, 1.00f };
    c[ImGuiCol_ChildBg]              = { 0.09f, 0.10f, 0.12f, 1.00f };
    c[ImGuiCol_PopupBg]              = { 0.09f, 0.10f, 0.12f, 1.00f };
    c[ImGuiCol_Border]               = { 0.19f, 0.21f, 0.24f, 1.00f };
    c[ImGuiCol_BorderShadow]         = { 0.00f, 0.00f, 0.00f, 0.00f };
    c[ImGuiCol_FrameBg]              = { 0.13f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_FrameBgHovered]       = { 0.19f, 0.21f, 0.25f, 1.00f };
    c[ImGuiCol_FrameBgActive]        = { 0.24f, 0.27f, 0.31f, 1.00f };
    c[ImGuiCol_TitleBg]              = { 0.07f, 0.08f, 0.09f, 1.00f };
    c[ImGuiCol_TitleBgActive]        = { 0.09f, 0.10f, 0.12f, 1.00f };
    c[ImGuiCol_Header]               = { 0.12f, 0.37f, 0.62f, 0.40f };
    c[ImGuiCol_HeaderHovered]        = { 0.12f, 0.37f, 0.62f, 0.60f };
    c[ImGuiCol_HeaderActive]         = { 0.12f, 0.37f, 0.62f, 0.80f };
    c[ImGuiCol_Button]               = { 0.13f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_ButtonHovered]        = { 0.12f, 0.37f, 0.62f, 0.60f };
    c[ImGuiCol_ButtonActive]         = { 0.12f, 0.37f, 0.62f, 1.00f };
    c[ImGuiCol_Tab]                  = { 0.13f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_TabHovered]           = { 0.12f, 0.37f, 0.62f, 0.80f };
    c[ImGuiCol_TabActive]            = { 0.12f, 0.37f, 0.62f, 1.00f };
    c[ImGuiCol_Text]                 = { 0.90f, 0.93f, 0.95f, 1.00f };
    c[ImGuiCol_TextDisabled]         = { 0.40f, 0.43f, 0.47f, 1.00f };
    c[ImGuiCol_Separator]            = { 0.19f, 0.21f, 0.24f, 1.00f };
    c[ImGuiCol_CheckMark]            = { 0.24f, 0.69f, 0.31f, 1.00f };
    c[ImGuiCol_SliderGrab]           = { 0.12f, 0.37f, 0.62f, 1.00f };
    c[ImGuiCol_SliderGrabActive]     = { 0.20f, 0.50f, 0.80f, 1.00f };
    c[ImGuiCol_ScrollbarBg]          = { 0.07f, 0.08f, 0.09f, 1.00f };
    c[ImGuiCol_ScrollbarGrab]        = { 0.19f, 0.21f, 0.24f, 1.00f };
    c[ImGuiCol_PlotLines]            = { 0.24f, 0.69f, 0.31f, 1.00f };
    c[ImGuiCol_PlotHistogram]        = { 0.12f, 0.37f, 0.62f, 1.00f };
    c[ImGuiCol_PlotHistogramHovered] = { 0.20f, 0.50f, 0.80f, 1.00f };
}

// ── Color constants ───────────────────────────────────────────────────────────
static const ImVec4 COL_GREEN  = { 0.24f, 0.69f, 0.31f, 1.0f };
static const ImVec4 COL_RED    = { 0.97f, 0.32f, 0.29f, 1.0f };
static const ImVec4 COL_BLUE   = { 0.35f, 0.60f, 0.90f, 1.0f };
static const ImVec4 COL_AMBER  = { 0.82f, 0.60f, 0.13f, 1.0f };
static const ImVec4 COL_MUTED  = { 0.40f, 0.43f, 0.47f, 1.0f };
static const ImVec4 COL_WHITE  = { 0.90f, 0.93f, 0.95f, 1.0f };
static const ImVec4 COL_PURPLE = { 0.49f, 0.40f, 0.87f, 1.0f };

// ── Init ──────────────────────────────────────────────────────────────────────
bool GUI::init(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) return false;
    window_ = win;

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1);
    glctx_ = ctx;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL2_InitForOpenGL(win, ctx);
    ImGui_ImplOpenGL3_Init("#version 130");
    applyTheme();
    return true;
}
void GUI::drawMainMenu(GameManager& gm, GameState& gs) {
    float W = ImGui::GetContentRegionAvail().x;
    float H = ImGui::GetContentRegionAvail().y;

    // Title block
    ImGui::SetCursorPosY(H * 0.20f);
    ImGui::SetCursorPosX((W - 320.0f) * 0.5f);
    ImGui::TextColored(COL_BLUE, "HFT ORDER BOOK SIMULATOR");
    ImGui::SetCursorPosX((W - 320.0f) * 0.5f);
    ImGui::TextColored(COL_MUTED,
        "C++ matching engine  |  nanosecond latency");

    ImGui::SetCursorPosY(H * 0.38f);
    ImGui::Separator();
    ImGui::Spacing(); ImGui::Spacing();

    float btnW = 340.0f;
    float btnH = 56.0f;

    // Benchmark mode button
    ImGui::SetCursorPosX((W - btnW) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button,
        ImVec4(0.08f, 0.25f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(0.12f, 0.37f, 0.62f, 1.0f));
    if (ImGui::Button("BENCHMARK MODE", { btnW, btnH })) {
        gs.mtx.unlock();
        gm.selectMode(AppMode::Benchmark);
        gs.mtx.lock();
    }
    ImGui::PopStyleColor(2);

    ImGui::SetCursorPosX((W - btnW) * 0.5f);
    ImGui::TextColored(COL_MUTED,
        "  Max speed - throughput, latency stats, histogram");

    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

    // Trading game button
    ImGui::SetCursorPosX((W - btnW) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button,
        ImVec4(0.10f, 0.35f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(0.15f, 0.50f, 0.22f, 1.0f));
    if (ImGui::Button("TRADING GAME", { btnW, btnH })) {
        gs.mtx.unlock();
        gm.selectMode(AppMode::Game);
        gs.mtx.lock();
    }
    ImGui::PopStyleColor(2);

    ImGui::SetCursorPosX((W - btnW) * 0.5f);
    ImGui::TextColored(COL_MUTED,
        "  Pick a stock - trade live - leaderboard");

    // Footer
    ImGui::SetCursorPosY(H - 36.0f);
    ImGui::Separator();
    ImGui::TextColored(COL_MUTED,
        "  363K orders/sec  |  600ns median latency  |  "
        "price-time priority matching  |  memory pool allocation");
}

void GUI::drawTradingReady(GameManager& gm, GameState& gs) {
    float W = ImGui::GetContentRegionAvail().x;
    float H = ImGui::GetContentRegionAvail().y;

    if (gs.selectedStockIdx < 0 ||
        gs.selectedStockIdx >= (int)gs.stocks.size()) return;

    StockState& ss = gs.stocks[gs.selectedStockIdx];

    ImGui::SetCursorPosY(H * 0.30f);
    ImGui::SetCursorPosX((W - 300.0f) * 0.5f);
    ImGui::TextColored(
        ImVec4(ss.profile.color[0], ss.profile.color[1],
               ss.profile.color[2], ss.profile.color[3]),
        "%s - %s", ss.profile.ticker.c_str(),
        ss.profile.name.c_str());

    ImGui::SetCursorPosX((W - 300.0f) * 0.5f);
    ImGui::TextColored(COL_MUTED, "Starting price: $%.2f  |  "
        "Spread: $%.2f  |  Capital: $100,000",
        ss.profile.startPrice, ss.profile.baseSpread);

    ImGui::Spacing(); ImGui::Spacing();
    ImGui::SetCursorPosX((W - 300.0f) * 0.5f);
    ImGui::TextColored(COL_MUTED, "%s", ss.profile.description.c_str());

    ImGui::SetCursorPosY(H * 0.52f);
    ImGui::Separator();
    ImGui::Spacing(); ImGui::Spacing();

    float btnW = 240.0f;
    ImGui::SetCursorPosX((W - btnW) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button,
        ImVec4(0.10f, 0.35f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(0.15f, 0.50f, 0.22f, 1.0f));
    if (ImGui::Button("START TRADING", { btnW, 48.0f })) {
        gs.phase = GamePhase::Trading;
    }
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    ImGui::SetCursorPosX((W - btnW) * 0.5f);
    if (ImGui::Button("BACK TO STOCK PICKER", { btnW, 32.0f })) {
        gs.phase = GamePhase::StockPicker;
        gs.selectedStockIdx = -1;
    }
}

void GUI::drawBenchmark(GameManager& gm, GameState& gs) {
    float fullW = ImGui::GetContentRegionAvail().x;
    float fullH = ImGui::GetContentRegionAvail().y;
    float ctrlH = 52.0f;
    float panelH = fullH - ctrlH - 8.0f;
    float colW   = (fullW - 24.0f) / 4.0f;

    // ── Panel 1 — Order book ──────────────────────────────────────────────────
    ImGui::BeginChild("##bbook", { colW, panelH }, true);
    ImGui::TextColored(COL_MUTED, "ORDER BOOK");
    ImGui::Separator();
    ImGui::TextColored(COL_MUTED,
        "%-10s %6s %6s", "PRICE", "SIZE", "TOTAL");
    ImGui::Separator();

    int askShow = std::min((int)gs.asks.size(), 8);
    for (int i = askShow - 1; i >= 0; --i) {
        auto& lvl = gs.asks[i];
        ImGui::TextColored(COL_RED,   "%10.2f", lvl.price);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.size);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.total);
    }
    ImGui::Separator();
    ImGui::TextColored(COL_AMBER, "  spread $%.4f", gs.spread);
    ImGui::Separator();
    int bidShow = std::min((int)gs.bids.size(), 8);
    for (int i = 0; i < bidShow; ++i) {
        auto& lvl = gs.bids[i];
        ImGui::TextColored(COL_GREEN, "%10.2f", lvl.price);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.size);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.total);
    }
    ImGui::EndChild();

    ImGui::SameLine(0, 8);

    // ── Panel 2 — Latency histogram ───────────────────────────────────────────
    ImGui::BeginChild("##bhisto", { colW, panelH }, true);
    ImGui::TextColored(COL_MUTED, "LATENCY DISTRIBUTION (ns)");
    ImGui::Separator();
    ImGui::Spacing();

    if (!gs.histogram.empty()) {
        float availW = ImGui::GetContentRegionAvail().x;
        float availH = ImGui::GetContentRegionAvail().y - 24.0f;
        float barW   = (availW - (gs.histogram.size() - 1) * 2.0f)
                     / gs.histogram.size();

        uint64_t maxCount = 1;
        for (auto& b : gs.histogram)
            maxCount = std::max(maxCount, b.count);

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (std::size_t i = 0; i < gs.histogram.size(); ++i) {
            float pct  = (float)gs.histogram[i].count / (float)maxCount;
            float barH = pct * availH;
            float x    = cursor.x + i * (barW + 2.0f);
            float yTop = cursor.y + availH - barH;
            float yBot = cursor.y + availH;

            ImU32 col = (gs.histogram[i].rangeMin >= gs.p99LatNs)
                ? IM_COL32(200, 60,  60,  180)
                : IM_COL32( 50, 120, 200, 180);

            dl->AddRectFilled({ x, yTop }, { x + barW, yBot }, col, 2.0f);
            dl->AddRect      ({ x, yTop }, { x + barW, yBot },
                (gs.histogram[i].rangeMin >= gs.p99LatNs)
                    ? IM_COL32(220, 80,  80,  255)
                    : IM_COL32( 80, 150, 220, 255), 2.0f);
        }

        ImGui::SetCursorScreenPos(
            { cursor.x, cursor.y + availH + 4.0f });
        ImGui::TextColored(COL_MUTED, "100ns");
        ImGui::SameLine(availW * 0.5f - 15.0f);
        ImGui::TextColored(COL_MUTED, "~50us");
        ImGui::SameLine(availW - 35.0f);
        ImGui::TextColored(COL_AMBER, "p99+");
    } else {
        ImGui::Spacing();
        ImGui::TextColored(COL_MUTED, "Running...");
        ImGui::TextColored(COL_MUTED, "Histogram builds");
        ImGui::TextColored(COL_MUTED, "after completion.");
    }

    ImGui::EndChild();

    ImGui::SameLine(0, 8);

    // ── Panel 3 — Core latency stats ──────────────────────────────────────────
    ImGui::BeginChild("##bstats", { colW, panelH }, true);
    ImGui::TextColored(COL_MUTED, "LATENCY STATS");
    ImGui::Separator();
    ImGui::Spacing();

    char buf[64];

    auto bstat = [&](const char* label, const char* val, ImVec4 col) {
        ImGui::TextColored(COL_MUTED, "%s", label);
        ImGui::TextColored(col, "  %s", val);
        ImGui::Spacing();
    };

    snprintf(buf, sizeof(buf), "%.0f ns", gs.medianLatNs);
    bstat("MEDIAN (P50)", buf, COL_GREEN);

    if (gs.p99LatNs < 1000.0)
        snprintf(buf, sizeof(buf), "%.0f ns", gs.p99LatNs);
    else
        snprintf(buf, sizeof(buf), "%.1f us", gs.p99LatNs / 1000.0);
    bstat("P99", buf, COL_AMBER);

    snprintf(buf, sizeof(buf), "%.0f ns", gs.minLatNs);
    bstat("MIN", buf, COL_GREEN);

    snprintf(buf, sizeof(buf), "%.0f ns", gs.maxLatNs);
    bstat("MAX", buf, COL_RED);

    snprintf(buf, sizeof(buf), "%.0f ns", gs.meanLatNs);
    bstat("MEAN", buf, COL_MUTED);

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "THROUGHPUT");
    ImGui::Spacing();

    snprintf(buf, sizeof(buf), "%.0f K/s", gs.ordersPerSec / 1000.0);
    bstat("ORDERS/SEC", buf, COL_BLUE);

    snprintf(buf, sizeof(buf), "%.1f%%", gs.fillRate * 100.0);
    bstat("FILL RATE", buf, COL_GREEN);

    snprintf(buf, sizeof(buf), "%llu",
        (unsigned long long)gs.totalFills);
    bstat("TOTAL FILLS", buf, COL_WHITE);

    ImGui::EndChild();

    ImGui::SameLine(0, 8);

    // ── Panel 4 — Extended performance stats ──────────────────────────────────
    ImGui::BeginChild("##bext", { colW, panelH }, true);
    ImGui::TextColored(COL_MUTED, "SESSION STATS");
    ImGui::Separator();
    ImGui::Spacing();

    // Elapsed time
    double elapsedSec = gs.benchmarkOrders > 0 && gs.ordersPerSec > 0
        ? gs.processedOrders / gs.ordersPerSec : 0.0;
    if (elapsedSec < 60.0)
        snprintf(buf, sizeof(buf), "%.2f s", elapsedSec);
    else
        snprintf(buf, sizeof(buf), "%.1f min", elapsedSec / 60.0);
    bstat("ELAPSED", buf, COL_WHITE);

    // Orders processed
    snprintf(buf, sizeof(buf), "%llu",
        (unsigned long long)gs.processedOrders);
    bstat("ORDERS DONE", buf, COL_BLUE);

    // Target
    snprintf(buf, sizeof(buf), "%llu",
        (unsigned long long)gs.benchmarkOrders);
    bstat("TARGET", buf, COL_MUTED);

    // Completion percentage
    float pct = gs.benchmarkOrders > 0
        ? (float)gs.processedOrders / (float)gs.benchmarkOrders * 100.0f
        : 0.0f;
    snprintf(buf, sizeof(buf), "%.1f%%", pct);
    bstat("COMPLETE", buf, pct >= 100.0f ? COL_GREEN : COL_BLUE);

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "DESIGN");
    ImGui::Spacing();

    bstat("ALGORITHM",    "price-time priority", COL_WHITE);
    bstat("MEMORY",       "pool allocator",      COL_WHITE);
    bstat("CACHE",        "alignas(64)",         COL_WHITE);
    bstat("BUILD",        "-O3 -march=native",   COL_WHITE);
    bstat("ORDER TYPES",  "limit/market/IOC",    COL_WHITE);
    bstat("THREADING",    "lock-free hot path",  COL_WHITE);

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(COL_MUTED, "HARDWARE");
    ImGui::Spacing();

    // Latency quality rating
    const char* rating =
        gs.medianLatNs < 500   ? "EXCELLENT" :
        gs.medianLatNs < 1000  ? "GOOD"      :
        gs.medianLatNs < 5000  ? "FAIR"      : "MEASURING...";
    ImVec4 ratingCol =
        gs.medianLatNs < 500   ? COL_GREEN :
        gs.medianLatNs < 1000  ? COL_BLUE  :
        gs.medianLatNs < 5000  ? COL_AMBER : COL_MUTED;
    bstat("LATENCY GRADE", rating, ratingCol);

    const char* throughputRating =
        gs.ordersPerSec > 290000 ? "EXCELLENT" :
        gs.ordersPerSec > 100000 ? "GOOD"      :
        gs.ordersPerSec > 50000  ? "FAIR"      : "MEASURING...";
    ImVec4 tpCol =
        gs.ordersPerSec > 290000 ? COL_GREEN :
        gs.ordersPerSec > 100000 ? COL_BLUE  :
        gs.ordersPerSec > 50000  ? COL_AMBER : COL_MUTED;
    bstat("THROUGHPUT GRADE", throughputRating, tpCol);

    ImGui::EndChild();

    // ── Controls bar ──────────────────────────────────────────────────────────
    ImGui::BeginChild("##bctrl", { fullW, ctrlH }, true);

    if (gs.paused.load()) {
        if (ImGui::Button("  RESUME  ")) gs.paused.store(false);
    } else {
        if (ImGui::Button("  PAUSE   ")) gs.paused.store(true);
    }
    ImGui::SameLine();

    if (ImGui::Button("  RESET   ")) {
        gs.mtx.unlock();
        gm.selectMode(AppMode::Benchmark);
        gs.mtx.lock();
    }
    ImGui::SameLine();

    float progress = gs.benchmarkOrders > 0
        ? (float)gs.processedOrders / (float)gs.benchmarkOrders : 0.0f;
    char overlay[32];
    snprintf(overlay, sizeof(overlay), "%llu / %llu",
        (unsigned long long)gs.processedOrders,
        (unsigned long long)gs.benchmarkOrders);
    ImGui::SetNextItemWidth(300.0f);
    ImGui::ProgressBar(progress, { 300.0f, 0.0f }, overlay);
    ImGui::SameLine();

    if (gs.simRunning.load())
        ImGui::TextColored(COL_GREEN, "  RUNNING");
    else
        ImGui::TextColored(COL_BLUE,  "  COMPLETE");

    ImGui::SameLine(fullW - 160.0f);
    if (ImGui::Button("MAIN MENU", { 140.0f, 0.0f })) {
        gs.phase = GamePhase::MainMenu;
    }

    ImGui::EndChild();
}
// ── Render one frame ──────────────────────────────────────────────────────────
bool GUI::render(GameManager& gm) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) return false;
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_CLOSE) return false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::Begin("##root", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    GameState& gs = gm.state();
    {
        std::lock_guard<std::mutex> lock(gs.mtx);
        switch (gs.phase) {
            case GamePhase::MainMenu:
                drawMainMenu(gm, gs);     break;
            case GamePhase::StockPicker:
                drawStockPicker(gm, gs);  break;
            case GamePhase::TradingReady:
                drawTradingReady(gm, gs); break;
            case GamePhase::Trading:
                drawTradingScreen(gm, gs);break;
            case GamePhase::SessionEnd:
                drawSessionEnd(gm, gs);   break;
            case GamePhase::Leaderboard:
                drawLeaderboard(gs);      break;
            case GamePhase::Benchmark:
                drawBenchmark(gm, gs);    break;
        }
    }

    ImGui::End();
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.07f, 0.08f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow((SDL_Window*)window_);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// STOCK PICKER SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawStockPicker(GameManager& gm, GameState& gs) {
    float W = ImGui::GetContentRegionAvail().x;
    float H = ImGui::GetContentRegionAvail().y;

    // Title
    ImGui::SetCursorPosY(H * 0.06f);
    ImGui::SetCursorPosX((W - 300.0f) * 0.5f);
    ImGui::TextColored(COL_BLUE, "HFT ORDER BOOK");
    ImGui::SetCursorPosX((W - 300.0f) * 0.5f);
    ImGui::TextColored(COL_MUTED, "Choose a stock to trade");
    ImGui::Spacing(); ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing(); ImGui::Spacing();

    // Stock cards - two rows of three
    float cardW = (W - 80.0f) / 3.0f;
    float cardH = (H * 0.55f);

    for (int i = 0; i < (int)gs.stocks.size(); ++i) {
        StockState& ss = gs.stocks[i];

        if (i % 3 != 0) ImGui::SameLine(0, 12);

        ImGui::PushID(i);

        // Highlight selected card
        bool selected = (gs.selectedStockIdx == i);
        if (selected)
            ImGui::PushStyleColor(ImGuiCol_ChildBg,
                ImVec4(0.12f, 0.20f, 0.35f, 1.0f));

        ImGui::BeginChild("##card", { cardW, cardH * 0.48f }, true);

        // Ticker and name
        ImGui::TextColored(
            ImVec4(ss.profile.color[0], ss.profile.color[1],
                   ss.profile.color[2], ss.profile.color[3]),
            "%s", ss.profile.ticker.c_str());
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, " %s", ss.profile.sector.c_str());
        ImGui::TextColored(COL_WHITE, "%s",
            ss.profile.name.c_str());
        ImGui::Spacing();

        // Price and spread
        ImGui::TextColored(COL_MUTED, "Start price");
        ImGui::SameLine(100);
        ImGui::TextColored(COL_GREEN, "$%.2f",
            ss.profile.startPrice);

        ImGui::TextColored(COL_MUTED, "Spread");
        ImGui::SameLine(100);
        ImGui::TextColored(COL_AMBER, "$%.2f",
            ss.profile.baseSpread);

        ImGui::TextColored(COL_MUTED, "Volatility");
        ImGui::SameLine(100);

        // Volatility bar
        float vol = (float)(ss.profile.baseVolatility / 0.15);
        ImVec4 volCol = vol > 0.6f ? COL_RED
                      : vol > 0.3f ? COL_AMBER
                      : COL_GREEN;
        ImGui::TextColored(volCol, "%s",
            vol > 0.7f ? "EXTREME" :
            vol > 0.4f ? "HIGH" :
            vol > 0.2f ? "MEDIUM" : "LOW");

        ImGui::Spacing();

        // Mini sparkline of price history
        if (ss.priceHistory.size() > 2) {
            std::vector<float> hist(
                ss.priceHistory.begin(), ss.priceHistory.end());
            float mn = *std::min_element(hist.begin(), hist.end());
            float mx = *std::max_element(hist.begin(), hist.end());
            if (mx - mn < 0.01f) mx = mn + 0.01f;

            ImGui::PlotLines("##spark", hist.data(),
                (int)hist.size(), 0, nullptr, mn, mx,
                { ImGui::GetContentRegionAvail().x, 40.0f });
        }

        ImGui::Spacing();

        // Description
        ImGui::TextWrapped("%s", ss.profile.description.c_str());

        ImGui::Spacing();

        // Select button
        if (ImGui::Button("TRADE THIS STOCK",
            { ImGui::GetContentRegionAvail().x, 28.0f })) {
            // Unlock before calling selectStock to avoid deadlock
            gs.mtx.unlock();
            gm.selectStock(i);
            gs.mtx.lock();
        }

        ImGui::EndChild();

        if (selected)
            ImGui::PopStyleColor();

        ImGui::PopID();

        // New row after 3 cards
        if (i == 2) { ImGui::Spacing(); ImGui::Spacing(); }
    }

    // Footer
    ImGui::SetCursorPosY(H - 40.0f);
    ImGui::Separator();
    ImGui::TextColored(COL_MUTED,
        "Starting capital: $100,000   |   "
        "Margin call at -$80,000   |   "
        "Session: 200,000 orders");
    }


// ─────────────────────────────────────────────────────────────────────────────
// TRADING SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawTradingScreen(GameManager& gm, GameState& gs) {
    float fullW = ImGui::GetContentRegionAvail().x;
    float fullH = ImGui::GetContentRegionAvail().y;
    float hudH  = 110.0f;
    float newsH = gs.visibleBanners.empty() ? 0.0f : 32.0f;
    float ctrlH = 48.0f;
    float mainH = fullH - hudH - newsH - ctrlH - 20.0f;

    float bookW = 220.0f;
    float feedW = 220.0f;
    float midW  = fullW - bookW - feedW - 16.0f;

    // ── Player HUD ────────────────────────────────────────────────────────────
    ImGui::BeginChild("##hud", { fullW, hudH }, true);
    drawPlayerHUD(gm, gs);
    ImGui::EndChild();

    // ── News banners ──────────────────────────────────────────────────────────
    if (!gs.visibleBanners.empty()) {
        ImGui::BeginChild("##news", { fullW, newsH }, false);
        drawNewsBanners(gs);
        ImGui::EndChild();
    }

    // ── Three main panels ─────────────────────────────────────────────────────
    ImGui::BeginChild("##book", { bookW, mainH }, true);
    drawOrderBook(gs);
    ImGui::EndChild();

    ImGui::SameLine(0, 8);
    ImGui::BeginChild("##mid", { midW, mainH }, true);
    drawPnlAndStats(gs);
    ImGui::EndChild();

    ImGui::SameLine(0, 8);
    ImGui::BeginChild("##feed", { feedW, mainH }, true);
    drawFillFeed(gs);
    ImGui::EndChild();

    // ── Controls bar ──────────────────────────────────────────────────────────
    ImGui::BeginChild("##ctrl", { fullW, ctrlH }, true);
    drawTradingControls(gm, gs);
    ImGui::EndChild();
}

// ── Player HUD ────────────────────────────────────────────────────────────────
void GUI::drawPlayerHUD(GameManager& gm, GameState& gs) {
    PlayerState& p    = gs.player;
    float        W    = ImGui::GetContentRegionAvail().x;
    float        colW = W / 7.0f;

    auto hudStat = [&](const char* label, const char* value, ImVec4 col) {
        ImGui::BeginGroup();
        ImGui::TextColored(COL_MUTED, "%s", label);
        ImGui::TextColored(col, "%s", value);
        ImGui::EndGroup();
    };

    char buf[64];

    // Total P&L
    ImVec4 pnlCol = p.totalPnl >= 0 ? COL_GREEN : COL_RED;
    snprintf(buf, sizeof(buf), "%+.2f", p.totalPnl);
    hudStat("TOTAL P&L", buf, pnlCol);
    ImGui::SameLine(colW);

    // Realized P&L
    snprintf(buf, sizeof(buf), "%+.2f", p.realizedPnl);
    hudStat("REALIZED", buf, p.realizedPnl >= 0 ? COL_GREEN : COL_RED);
    ImGui::SameLine(colW * 2);

    // Unrealized P&L
    snprintf(buf, sizeof(buf), "%+.2f", p.unrealizedPnl);
    hudStat("UNREALIZED", buf, p.unrealizedPnl >= 0 ? COL_GREEN : COL_RED);
    ImGui::SameLine(colW * 3);

    // Position
    int qty = p.activePosition.quantity;
    ImVec4 posCol = qty > 0 ? COL_GREEN : qty < 0 ? COL_RED : COL_MUTED;
    snprintf(buf, sizeof(buf), qty == 0 ? "FLAT" : "%+d @ $%.2f",
             qty, p.activePosition.avgEntryPrice);
    hudStat("POSITION", buf, posCol);
    ImGui::SameLine(colW * 4);

    // Cash
    snprintf(buf, sizeof(buf), "$%.0f", p.cash);
    hudStat("CASH", buf, COL_WHITE);
    ImGui::SameLine(colW * 5);

    // Max drawdown
    snprintf(buf, sizeof(buf), "-$%.0f", p.maxDrawdown);
    hudStat("MAX DD", buf, COL_AMBER);
    ImGui::SameLine(colW * 6);

    // Win rate
    snprintf(buf, sizeof(buf), "%.0f%% (%d)",
        p.winRate() * 100.0, p.totalTrades());
    hudStat("WIN RATE", buf, COL_BLUE);

    // P&L sparkline
    ImGui::Spacing();
    if (p.pnlHistory.size() > 2) {
        std::vector<float> hist(
            p.pnlHistory.begin(), p.pnlHistory.end());
        float mn = *std::min_element(hist.begin(), hist.end());
        float mx = *std::max_element(hist.begin(), hist.end());
        float rng = mx - mn;
        if (rng < 1.0f) { mn -= 1.0f; mx += 1.0f; }

        ImGui::PlotLines("##pnlspark", hist.data(),
            (int)hist.size(), 0, nullptr, mn, mx,
            { W, 28.0f });
    }
}

// ── News banners ──────────────────────────────────────────────────────────────
void GUI::drawNewsBanners(GameState& gs) {
    for (auto& b : gs.visibleBanners) {
        ImVec4 col = b.isPositive ? COL_GREEN : COL_RED;
        ImGui::TextColored(COL_MUTED, "[NEWS]");
        ImGui::SameLine();
        ImGui::TextColored(col, "%s", b.headline.c_str());
        break; // show most recent only - others scroll naturally
    }
}

// ── Order book panel ──────────────────────────────────────────────────────────
void GUI::drawOrderBook(GameState& gs) {
    if (gs.selectedStockIdx < 0 ||
        gs.selectedStockIdx >= (int)gs.stocks.size()) return;

    StockState& ss = gs.stocks[gs.selectedStockIdx];

    // ── Header ────────────────────────────────────────────────────────────────
    ImGui::TextColored(
        ImVec4(ss.profile.color[0], ss.profile.color[1],
               ss.profile.color[2], ss.profile.color[3]),
        "%s", ss.profile.ticker.c_str());
    ImGui::SameLine();
    ImGui::TextColored(COL_WHITE, "$%.2f", ss.currentPrice);
    ImGui::SameLine();

    const char* regimeName =
        ss.currentRegime == Regime::Momentum      ? "[MOM]" :
        ss.currentRegime == Regime::MeanReversion ? "[REV]" :
        ss.currentRegime == Regime::Volatile      ? "[VOL]" : "[QUT]";
    ImVec4 regimeCol =
        ss.currentRegime == Regime::Momentum      ? COL_BLUE  :
        ss.currentRegime == Regime::MeanReversion ? COL_GREEN :
        ss.currentRegime == Regime::Volatile      ? COL_RED   : COL_MUTED;
    ImGui::TextColored(regimeCol, "%s", regimeName);

    // ── Price chart — colored line ─────────────────────────────────────────────
    if (ss.priceHistory.size() > 2) {
        float chartW = ImGui::GetContentRegionAvail().x;
        float chartH = 55.0f;

        std::vector<float> prices(
            ss.priceHistory.begin(), ss.priceHistory.end());

        float sessionOpen = prices.front();
        float currentPrice = prices.back();
        float mn = *std::min_element(prices.begin(), prices.end());
        float mx = *std::max_element(prices.begin(), prices.end());
        float rng = mx - mn;
        if (rng < 0.01f) { mn -= 0.05f; mx += 0.05f; rng = mx - mn; }

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Background
        dl->AddRectFilled(
            cursor,
            { cursor.x + chartW, cursor.y + chartH },
            IM_COL32(15, 18, 22, 255));

        // Session open horizontal reference line
        float openY = cursor.y + chartH -
            ((sessionOpen - mn) / rng) * chartH;
        dl->AddLine(
            { cursor.x, openY },
            { cursor.x + chartW, openY },
            IM_COL32(80, 80, 80, 120), 0.5f);

        // Price line — green if above open, red if below
        bool isUp = currentPrice >= sessionOpen;
        ImU32 lineCol = isUp
            ? IM_COL32(60, 180, 80, 255)
            : IM_COL32(220, 70, 70, 255);

        // Draw line segments
        int n = (int)prices.size();
        for (int p = 1; p < n; ++p) {
            float x0 = cursor.x + ((float)(p-1) / (n-1)) * chartW;
            float x1 = cursor.x + ((float)p     / (n-1)) * chartW;
            float y0 = cursor.y + chartH -
                ((prices[p-1] - mn) / rng) * chartH;
            float y1 = cursor.y + chartH -
                ((prices[p]   - mn) / rng) * chartH;
            dl->AddLine({ x0, y0 }, { x1, y1 }, lineCol, 1.2f);
        }

        // Current price label on right edge
        float lastY = cursor.y + chartH -
            ((currentPrice - mn) / rng) * chartH;
        dl->AddCircleFilled(
            { cursor.x + chartW - 3.0f, lastY }, 3.0f, lineCol);

        // Pct change from session open
        float pctChange = ((currentPrice - sessionOpen) / sessionOpen) * 100.0f;
        char pctBuf[16];
        snprintf(pctBuf, sizeof(pctBuf), "%+.2f%%", pctChange);
        ImVec4 pctCol = isUp ? COL_GREEN : COL_RED;

        // Advance cursor past the chart
        ImGui::SetCursorScreenPos(
            { cursor.x, cursor.y + chartH + 4.0f });

        ImGui::TextColored(COL_MUTED, "%.2f", mn);
        ImGui::SameLine(chartW - 60.0f);
        ImGui::TextColored(pctCol, "%s", pctBuf);
    }

    ImGui::Separator();

    // ── Order book ────────────────────────────────────────────────────────────
    ImGui::TextColored(COL_MUTED,
        "%-10s %6s %6s", "PRICE", "SIZE", "TOTAL");
    ImGui::Separator();

    int askShow = std::min((int)gs.asks.size(), 5);
    for (int i = askShow - 1; i >= 0; --i) {
        auto& lvl = gs.asks[i];
        ImGui::TextColored(COL_RED,   "%10.2f", lvl.price);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.size);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.total);
    }

    ImGui::Separator();
    ImGui::TextColored(COL_AMBER, "  spread $%.4f", gs.spread);
    ImGui::Separator();

    int bidShow = std::min((int)gs.bids.size(), 5);
    for (int i = 0; i < bidShow; ++i) {
        auto& lvl = gs.bids[i];
        ImGui::TextColored(COL_GREEN, "%10.2f", lvl.price);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.size);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.total);
    }
}

// ── P&L chart and stats ───────────────────────────────────────────────────────
void GUI::drawPnlAndStats(GameState& gs) {
    PlayerState& p = gs.player;
    float W = ImGui::GetContentRegionAvail().x;

    ImGui::TextColored(COL_MUTED, "PERFORMANCE");
    ImGui::Separator();

    // Stats grid
    float colW = (W - 8) / 2.0f;
    char buf[64];

    auto stat = [&](const char* label, const char* val, ImVec4 col) {
        ImGui::BeginGroup();
        ImGui::TextColored(COL_MUTED, "%s", label);
        ImGui::TextColored(col, "%s", val);
        ImGui::EndGroup();
    };

    snprintf(buf, sizeof(buf), "%.0f ns", gs.medianLatNs);
    stat("MEDIAN LAT", buf, COL_GREEN);
    ImGui::SameLine(colW);
    snprintf(buf, sizeof(buf), "%.0f K/s", gs.ordersPerSec / 1000.0);
    stat("THROUGHPUT", buf, COL_BLUE);

    snprintf(buf, sizeof(buf), "$%.0f", p.peakPnl);
    stat("PEAK P&L", buf, COL_GREEN);
    ImGui::SameLine(colW);
    snprintf(buf, sizeof(buf), "-$%.0f", p.maxDrawdown);
    stat("MAX DRAWDOWN", buf, COL_AMBER);

    snprintf(buf, sizeof(buf), "%d", p.totalTrades());
    stat("TRADES", buf, COL_WHITE);
    ImGui::SameLine(colW);
    snprintf(buf, sizeof(buf), "%.0f%%", p.winRate() * 100.0);
    stat("WIN RATE", buf, COL_BLUE);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(COL_MUTED, "P&L CURVE");
    ImGui::Spacing();

    // P&L curve - full height remaining
    if (p.pnlHistory.size() > 2) {
        std::vector<float> hist(
            p.pnlHistory.begin(), p.pnlHistory.end());
        float mn = *std::min_element(hist.begin(), hist.end());
        float mx = *std::max_element(hist.begin(), hist.end());
        float rng = mx - mn;
        if (rng < 10.0f) { mn -= 5.0f; mx += 5.0f; }

        // Zero line indicator
        ImGui::TextColored(mn < 0 ? COL_RED : COL_GREEN,
            "%.0f", (double)mn);
        ImGui::SameLine(W - 60.0f);
        ImGui::TextColored(COL_GREEN, "%.0f", (double)mx);

        float chartH = ImGui::GetContentRegionAvail().y - 30.0f;
        ImGui::PlotLines("##pnlcurve", hist.data(),
            (int)hist.size(), 0, nullptr, mn, mx,
            { W, chartH });
    }

    // Progress bar
    float progress = gs.totalOrders > 0
        ? (float)gs.processedOrders / (float)gs.totalOrders
        : 0.0f;
    char overlay[32];
    snprintf(overlay, sizeof(overlay), "%llu / %llu",
        (unsigned long long)gs.processedOrders,
        (unsigned long long)gs.totalOrders);
    ImGui::ProgressBar(progress, { W, 12.0f }, overlay);
}

// ── Fill feed panel ───────────────────────────────────────────────────────────
void GUI::drawFillFeed(GameState& gs) {
    ImGui::TextColored(COL_MUTED, "RECENT FILLS");
    ImGui::Separator();
    ImGui::TextColored(COL_MUTED,
        "%-4s %8s %5s %6s", "SIDE", "PRICE", "QTY", "LAT");
    ImGui::Separator();

    if (gs.recentFills.empty()) {
        ImGui::TextColored(COL_MUTED, "Waiting for fills...");
        return;
    }

    for (const auto& f : gs.recentFills) {
        ImGui::TextColored(f.isBuy ? COL_GREEN : COL_RED,
            "%-4s", f.isBuy ? "BUY" : "SELL");
        ImGui::SameLine();
        ImGui::TextColored(COL_WHITE,  "%8.2f", f.price);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED,  "%5u",   f.quantity);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED,  "%5.1fus", f.latencyUs);
    }
}

// ── Trading controls bar ──────────────────────────────────────────────────────
void GUI::drawTradingControls(GameManager& gm, GameState& gs) {
    static int orderQty = 10;

    // Pause / Resume
    if (gs.paused.load()) {
        if (ImGui::Button("  RESUME  ")) gs.paused.store(false);
    } else {
        if (ImGui::Button("  PAUSE   ")) gs.paused.store(true);
    }
    ImGui::SameLine();

    // Qty
    ImGui::TextColored(COL_MUTED, "  QTY:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::InputInt("##qty", &orderQty, 0, 0);
    orderQty = std::max(1, std::min(orderQty, 9999));
    ImGui::SameLine();

    // BUY
    ImGui::PushStyleColor(ImGuiCol_Button,
        ImVec4(0.10f, 0.35f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(0.15f, 0.50f, 0.20f, 1.0f));
    if (ImGui::Button("  BUY  ", { 70.0f, 0.0f })) {
        gs.mtx.unlock();
        gm.requestBuy(static_cast<uint32_t>(orderQty));
        gs.mtx.lock();
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();

    // SELL
    ImGui::PushStyleColor(ImGuiCol_Button,
        ImVec4(0.40f, 0.10f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(0.60f, 0.15f, 0.15f, 1.0f));
    if (ImGui::Button(" SELL  ", { 70.0f, 0.0f })) {
        gs.mtx.unlock();
        gm.requestSell(static_cast<uint32_t>(orderQty));
        gs.mtx.lock();
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();

    // FLATTEN
    ImGui::PushStyleColor(ImGuiCol_Button,
        ImVec4(0.20f, 0.20f, 0.40f, 1.0f));
    if (ImGui::Button(" FLAT ", { 60.0f, 0.0f })) {
        gs.mtx.unlock();
        gm.requestFlatten();
        gs.mtx.lock();
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // ── Speed buttons ─────────────────────────────────────────────────────────
    ImGui::TextColored(COL_MUTED, "  SPEED:");
    ImGui::SameLine();

    int currentSpeed = gs.speedSetting.load();
    const char* labels[] = { "0.25x", "0.5x", "1x", "2x", "5x" };

    for (int s = 0; s < 5; ++s) {
        bool active = (currentSpeed == s);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button,
            ImVec4(0.12f, 0.37f, 0.62f, 1.0f));
        if (ImGui::Button(labels[s])) gs.speedSetting.store(s);
        if (active) ImGui::PopStyleColor();
        if (s < 4) ImGui::SameLine();
    }

    ImGui::SameLine();

    // Regime indicator
    if (gs.selectedStockIdx >= 0) {
        StockState& ss = gs.stocks[gs.selectedStockIdx];
        const char* regimeFull =
            ss.currentRegime == Regime::Momentum      ? "MOMENTUM"       :
            ss.currentRegime == Regime::MeanReversion ? "MEAN REVERSION" :
            ss.currentRegime == Regime::Volatile      ? "VOLATILE"       :
                                                        "QUIET";
        ImVec4 rc =
            ss.currentRegime == Regime::Momentum      ? COL_BLUE  :
            ss.currentRegime == Regime::MeanReversion ? COL_GREEN :
            ss.currentRegime == Regime::Volatile      ? COL_RED   : COL_MUTED;
        ImGui::TextColored(COL_MUTED, "  REGIME:");
        ImGui::SameLine();
        ImGui::TextColored(rc, "%s", regimeFull);
    }

    // Margin warning
    if (gs.player.marginUsed > gs.player.maxMargin * 0.80f) {
        ImGui::SameLine();
        ImGui::TextColored(COL_RED, "  [!] MARGIN WARNING");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SESSION END SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawSessionEnd(GameManager& gm, GameState& gs) {
    float W = ImGui::GetContentRegionAvail().x;
    float H = ImGui::GetContentRegionAvail().y;
    PlayerState& p = gs.player;

    ImGui::SetCursorPosY(H * 0.12f);

    // Result headline
    bool won = p.sessionEndPnl > 0;
    bool marginCalled = p.isMarginCalled;

    ImGui::SetCursorPosX((W - 400.0f) * 0.5f);
    if (marginCalled) {
        ImGui::TextColored(COL_RED, "MARGIN CALL - SESSION TERMINATED");
    } else if (won) {
        ImGui::TextColored(COL_GREEN, "SESSION COMPLETE - PROFITABLE");
    } else {
        ImGui::TextColored(COL_AMBER, "SESSION COMPLETE - NET LOSS");
    }

    ImGui::Spacing(); ImGui::Spacing();
    ImGui::SetCursorPosX((W - 400.0f) * 0.5f);

    // Final P&L big number
    char pnlBuf[64];
    snprintf(pnlBuf, sizeof(pnlBuf), "%+.2f", p.sessionEndPnl);
    ImGui::TextColored(won ? COL_GREEN : COL_RED, "%s", pnlBuf);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Stats summary table
    float tableW = W * 0.50f;
    ImGui::SetCursorPosX((W - tableW) * 0.5f);
    ImGui::BeginChild("##results", { tableW, 240.0f }, true);

    auto resultRow = [&](const char* label, const char* value, ImVec4 col) {
        ImGui::TextColored(COL_MUTED, "%-20s", label);
        ImGui::SameLine(160);
        ImGui::TextColored(col, "%s", value);
    };

    char buf[64];

    snprintf(buf, sizeof(buf), "%+.2f", p.sessionEndPnl);
    resultRow("Final P&L",     buf, won ? COL_GREEN : COL_RED);

    snprintf(buf, sizeof(buf), "%+.2f", p.realizedPnl);
    resultRow("Realized P&L",  buf, p.realizedPnl >= 0 ? COL_GREEN : COL_RED);

    snprintf(buf, sizeof(buf), "+%.2f", p.peakPnl);
    resultRow("Peak P&L",      buf, COL_GREEN);

    snprintf(buf, sizeof(buf), "-%.2f", p.maxDrawdown);
    resultRow("Max drawdown",  buf, COL_AMBER);

    snprintf(buf, sizeof(buf), "%d", p.totalTrades());
    resultRow("Total trades",  buf, COL_WHITE);

    snprintf(buf, sizeof(buf), "%.1f%%", p.winRate() * 100.0);
    resultRow("Win rate",      buf, COL_BLUE);

    snprintf(buf, sizeof(buf), "%s",
        gs.selectedStockIdx >= 0
        ? gs.stocks[gs.selectedStockIdx].profile.ticker.c_str()
        : "N/A");
    resultRow("Stock traded",  buf, COL_PURPLE);

    ImGui::EndChild();

    ImGui::Spacing(); ImGui::Spacing();

    // Name entry for leaderboard
    ImGui::SetCursorPosX((W - tableW) * 0.5f);
    ImGui::TextColored(COL_MUTED, "Enter name for leaderboard:");
    ImGui::SetCursorPosX((W - tableW) * 0.5f);
    ImGui::SetNextItemWidth(tableW * 0.6f);
    ImGui::InputText("##name", nameBuffer_, sizeof(nameBuffer_));

    ImGui::SameLine();
    if (ImGui::Button("SUBMIT", { 100.0f, 0.0f })) {
        gs.mtx.unlock();
        gm.submitToLeaderboard(std::string(nameBuffer_));
        gs.mtx.lock();
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX((W - tableW) * 0.5f);
    if (ImGui::Button("PLAY AGAIN", { tableW * 0.48f, 32.0f })) {
        gs.mtx.unlock();
        gm.selectMode(AppMode::Game);  // restarts thread properly
        gs.mtx.lock();
}
    ImGui::SameLine();
    if (ImGui::Button("VIEW LEADERBOARD",
        { tableW * 0.48f, 32.0f })) {
        gs.phase = GamePhase::Leaderboard;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LEADERBOARD SCREEN
// ─────────────────────────────────────────────────────────────────────────────
void GUI::drawLeaderboard(GameState& gs) {
    float W = ImGui::GetContentRegionAvail().x;
    float H = ImGui::GetContentRegionAvail().y;

    ImGui::SetCursorPosY(H * 0.08f);
    ImGui::SetCursorPosX((W - 200.0f) * 0.5f);
    ImGui::TextColored(COL_BLUE, "LEADERBOARD");
    ImGui::SetCursorPosX((W - 200.0f) * 0.5f);
    ImGui::TextColored(COL_MUTED, "Top 10 sessions this run");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float tableW = W * 0.70f;
    ImGui::SetCursorPosX((W - tableW) * 0.5f);
    ImGui::BeginChild("##lb", { tableW, H * 0.70f }, true);

    // Header
    ImGui::TextColored(COL_MUTED,
        "%-4s %-14s %-8s %10s %10s %8s %8s",
        "#", "NAME", "STOCK", "FINAL P&L",
        "PEAK P&L", "DRAWDOWN", "WIN RATE");
    ImGui::Separator();

    if (gs.leaderboard.isEmpty()) {
        ImGui::Spacing();
        ImGui::SetCursorPosX(tableW * 0.3f);
        ImGui::TextColored(COL_MUTED, "No sessions recorded yet.");
    } else {
        int rank = 1;
        for (const auto& e : gs.leaderboard.entries) {
            ImVec4 rankCol = rank == 1 ? COL_AMBER  // gold
                           : rank == 2 ? COL_MUTED  // silver
                           : rank == 3 ? COL_RED     // bronze
                           : COL_WHITE;

            char buf[256];
            snprintf(buf, sizeof(buf),
                "%-4d %-14s %-8s %+10.2f %+10.2f %8.2f %7.1f%%",
                rank,
                e.playerName.c_str(),
                e.ticker.c_str(),
                e.finalPnl,
                e.peakPnl,
                e.maxDrawdown,
                e.winRate * 100.0);

            ImGui::TextColored(rankCol, "%s", buf);
            ++rank;
        }
    }

    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::SetCursorPosX((W - 220.0f) * 0.5f);
    // In drawLeaderboard - replace Play Again:
    if (ImGui::Button("PLAY AGAIN", { 220.0f, 32.0f })) {
        gs.phase = GamePhase::MainMenu;
    }
}

// ── Shutdown ──────────────────────────────────────────────────────────────────
void GUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    if (glctx_)  SDL_GL_DeleteContext((SDL_GLContext)glctx_);
    if (window_) SDL_DestroyWindow((SDL_Window*)window_);
    SDL_Quit();
}