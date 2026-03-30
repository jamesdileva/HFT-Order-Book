// GUI.cpp
#include "GUI.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>
#include <string>
#include <algorithm>

// ── Dark Bloomberg-style theme ────────────────────────────────────────────────
static void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;

    s.WindowRounding    = 4.0f;
    s.FrameRounding     = 3.0f;
    s.ScrollbarRounding = 3.0f;
    s.GrabRounding      = 3.0f;
    s.TabRounding       = 3.0f;
    s.FramePadding      = { 8, 4 };
    s.ItemSpacing       = { 8, 4 };
    s.WindowPadding     = { 10, 10 };
    s.ScrollbarSize     = 10.0f;
    s.GrabMinSize       = 10.0f;
    s.WindowBorderSize  = 0.5f;
    s.FrameBorderSize   = 0.5f;

    // Background layers
    c[ImGuiCol_WindowBg]         = { 0.09f, 0.10f, 0.12f, 1.00f };
    c[ImGuiCol_ChildBg]          = { 0.07f, 0.08f, 0.09f, 1.00f };
    c[ImGuiCol_PopupBg]          = { 0.09f, 0.10f, 0.12f, 1.00f };

    // Borders
    c[ImGuiCol_Border]           = { 0.19f, 0.21f, 0.24f, 1.00f };
    c[ImGuiCol_BorderShadow]     = { 0.00f, 0.00f, 0.00f, 0.00f };

    // Headers
    c[ImGuiCol_Header]           = { 0.12f, 0.37f, 0.62f, 0.40f };
    c[ImGuiCol_HeaderHovered]    = { 0.12f, 0.37f, 0.62f, 0.60f };
    c[ImGuiCol_HeaderActive]     = { 0.12f, 0.37f, 0.62f, 0.80f };

    // Frame
    c[ImGuiCol_FrameBg]          = { 0.13f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_FrameBgHovered]   = { 0.19f, 0.21f, 0.25f, 1.00f };
    c[ImGuiCol_FrameBgActive]    = { 0.24f, 0.27f, 0.31f, 1.00f };

    // Title
    c[ImGuiCol_TitleBg]          = { 0.07f, 0.08f, 0.09f, 1.00f };
    c[ImGuiCol_TitleBgActive]    = { 0.09f, 0.10f, 0.12f, 1.00f };
    c[ImGuiCol_TitleBgCollapsed] = { 0.07f, 0.08f, 0.09f, 1.00f };

    // Buttons
    c[ImGuiCol_Button]           = { 0.13f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_ButtonHovered]    = { 0.12f, 0.37f, 0.62f, 0.60f };
    c[ImGuiCol_ButtonActive]     = { 0.12f, 0.37f, 0.62f, 1.00f };

    // Tabs
    c[ImGuiCol_Tab]              = { 0.13f, 0.15f, 0.18f, 1.00f };
    c[ImGuiCol_TabHovered]       = { 0.12f, 0.37f, 0.62f, 0.80f };
    c[ImGuiCol_TabActive]        = { 0.12f, 0.37f, 0.62f, 1.00f };

    // Text
    c[ImGuiCol_Text]             = { 0.90f, 0.93f, 0.95f, 1.00f };
    c[ImGuiCol_TextDisabled]     = { 0.40f, 0.43f, 0.47f, 1.00f };

    // Misc
    c[ImGuiCol_Separator]        = { 0.19f, 0.21f, 0.24f, 1.00f };
    c[ImGuiCol_CheckMark]        = { 0.34f, 0.65f, 0.31f, 1.00f };
    c[ImGuiCol_SliderGrab]       = { 0.12f, 0.37f, 0.62f, 1.00f };
    c[ImGuiCol_SliderGrabActive] = { 0.20f, 0.50f, 0.80f, 1.00f };
    c[ImGuiCol_ScrollbarBg]      = { 0.07f, 0.08f, 0.09f, 1.00f };
    c[ImGuiCol_ScrollbarGrab]    = { 0.19f, 0.21f, 0.24f, 1.00f };
    c[ImGuiCol_PlotLines]        = { 0.34f, 0.65f, 0.31f, 1.00f };
    c[ImGuiCol_PlotHistogram]    = { 0.12f, 0.37f, 0.62f, 1.00f };
    c[ImGuiCol_PlotHistogramHovered] = { 0.20f, 0.50f, 0.80f, 1.00f };
}

// ── Colors ────────────────────────────────────────────────────────────────────
static const ImVec4 COL_GREEN  = { 0.24f, 0.69f, 0.31f, 1.0f };
static const ImVec4 COL_RED    = { 0.97f, 0.32f, 0.29f, 1.0f };
static const ImVec4 COL_BLUE   = { 0.35f, 0.60f, 0.90f, 1.0f };
static const ImVec4 COL_AMBER  = { 0.82f, 0.60f, 0.13f, 1.0f };
static const ImVec4 COL_MUTED  = { 0.40f, 0.43f, 0.47f, 1.0f };
static const ImVec4 COL_WHITE  = { 0.90f, 0.93f, 0.95f, 1.0f };

// ── Init ──────────────────────────────────────────────────────────────────────
bool GUI::init(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* win = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
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

// ── Render one frame ──────────────────────────────────────────────────────────
bool GUI::render(SimState& state) {
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

    // Full-screen dockspace
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    float fullW = ImGui::GetContentRegionAvail().x;
    float fullH = ImGui::GetContentRegionAvail().y;
    float ctrlH = 52.0f;
    float panelH = fullH - ctrlH - 8.0f;
    float bookW  = 220.0f;
    float feedW  = 220.0f;
    float midW   = fullW - bookW - feedW - 16.0f;

    // ── Three panels ─────────────────────────────────────────────────────────
    ImGui::BeginChild("book", { bookW, panelH }, true);
    drawOrderBook(state);
    ImGui::EndChild();

    ImGui::SameLine(0, 8);
    ImGui::BeginChild("stats", { midW, panelH }, true);
    drawStatsAndHistogram(state);
    ImGui::EndChild();

    ImGui::SameLine(0, 8);
    ImGui::BeginChild("fills", { feedW, panelH }, true);
    drawFillFeed(state);
    ImGui::EndChild();

    // ── Controls bar ─────────────────────────────────────────────────────────
    ImGui::BeginChild("controls", { fullW, ctrlH }, true);
    drawControls(state);
    ImGui::EndChild();

    ImGui::End();

    // Render
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.07f, 0.08f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow((SDL_Window*)window_);
    return true;
}

// ── Order Book Panel ──────────────────────────────────────────────────────────
void GUI::drawOrderBook(SimState& state) {
    ImGui::TextColored(COL_MUTED, "ORDER BOOK");
    ImGui::Separator();

    std::lock_guard<std::mutex> lock(state.mtx);

    // Column headers
    ImGui::TextColored(COL_MUTED, "%-10s %6s %6s", "PRICE", "SIZE", "TOTAL");
    ImGui::Separator();

    // Asks — show top 6 in reverse (worst to best at bottom)
    int askShow = std::min((int)state.asks.size(), 6);
    for (int i = askShow - 1; i >= 0; --i) {
        auto& lvl = state.asks[i];
        ImGui::TextColored(COL_RED,   "%10.2f", lvl.price);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.size);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.total);
    }

    // Spread
    ImGui::Separator();
    ImGui::TextColored(COL_AMBER, "  spread  $%.2f", state.spread);
    ImGui::Separator();

    // Bids — show top 6
    int bidShow = std::min((int)state.bids.size(), 6);
    for (int i = 0; i < bidShow; ++i) {
        auto& lvl = state.bids[i];
        ImGui::TextColored(COL_GREEN, "%10.2f", lvl.price);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.size);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%6u", lvl.total);
    }
}

// ── Stats + Histogram Panel ───────────────────────────────────────────────────
void GUI::drawStatsAndHistogram(SimState& state) {
    ImGui::TextColored(COL_MUTED, "PERFORMANCE");
    ImGui::Separator();

    std::lock_guard<std::mutex> lock(state.mtx);

    // Stat cards in a 2x3 grid
    float cardW = (ImGui::GetContentRegionAvail().x - 8) / 2.0f;

    auto statCard = [&](const char* label, const char* value, ImVec4 col) {
        ImGui::BeginGroup();
        ImGui::TextColored(COL_MUTED, "%s", label);
        ImGui::TextColored(col, "%s", value);
        ImGui::EndGroup();
    };

    char buf[64];

    snprintf(buf, sizeof(buf), "%.0f ns", state.medianLatNs);
    statCard("MEDIAN (P50)", buf, COL_GREEN);
    ImGui::SameLine(cardW);
    snprintf(buf, sizeof(buf), "%.0f us", state.p99LatNs / 1000.0);
    statCard("P99", buf, COL_AMBER);

    snprintf(buf, sizeof(buf), "%.0f K/s", state.ordersPerSec / 1000.0);
    statCard("THROUGHPUT", buf, COL_BLUE);
    ImGui::SameLine(cardW);
    snprintf(buf, sizeof(buf), "%.1f%%", state.fillRate * 100.0);
    statCard("FILL RATE", buf, COL_GREEN);

    snprintf(buf, sizeof(buf), "%.0f ns", state.minLatNs);
    statCard("MIN LAT", buf, COL_WHITE);
    ImGui::SameLine(cardW);
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)state.totalFills);
    statCard("TOTAL FILLS", buf, COL_WHITE);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(COL_MUTED, "LATENCY DISTRIBUTION");
    ImGui::Spacing();

    // Histogram
    if (!state.histogram.empty()) {
        float availW = ImGui::GetContentRegionAvail().x;
        float availH = ImGui::GetContentRegionAvail().y - 20.0f;
        float barW   = (availW - (state.histogram.size() - 1) * 2.0f) / state.histogram.size();

        uint64_t maxCount = 1;
        for (auto& b : state.histogram)
            maxCount = std::max(maxCount, b.count);

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (std::size_t i = 0; i < state.histogram.size(); ++i) {
            float pct    = (float)state.histogram[i].count / (float)maxCount;
            float barH   = pct * availH;
            float x      = cursor.x + i * (barW + 2.0f);
            float yTop   = cursor.y + availH - barH;
            float yBot   = cursor.y + availH;

            // Hot bars (p99+ range) in red, rest in blue
            ImU32 col = (state.histogram[i].rangeMin >= state.p99LatNs)
                ? IM_COL32(200, 60,  60,  180)
                : IM_COL32( 50, 120, 200, 180);

            dl->AddRectFilled({ x, yTop }, { x + barW, yBot }, col, 2.0f);
            dl->AddRect      ({ x, yTop }, { x + barW, yBot },
                (state.histogram[i].rangeMin >= state.p99LatNs)
                    ? IM_COL32(220, 80, 80, 255)
                    : IM_COL32(80, 150, 220, 255),
                2.0f);
        }

        // Axis labels
        ImGui::SetCursorScreenPos({ cursor.x, cursor.y + availH + 4.0f });
        ImGui::TextColored(COL_MUTED, "100ns");
        ImGui::SameLine(availW * 0.5f - 15.0f);
        ImGui::TextColored(COL_MUTED, "~50us");
        ImGui::SameLine(availW - 35.0f);
        ImGui::TextColored(COL_AMBER, "p99+");
    }
}

// ── Fill Feed Panel ───────────────────────────────────────────────────────────
void GUI::drawFillFeed(SimState& state) {
    ImGui::TextColored(COL_MUTED, "RECENT FILLS");
    ImGui::Separator();
    ImGui::TextColored(COL_MUTED, "%-4s %8s %5s %6s", "SIDE", "PRICE", "QTY", "LAT");
    ImGui::Separator();

    std::lock_guard<std::mutex> lock(state.mtx);

    for (auto& f : state.recentFills) {
        ImGui::TextColored(f.isBuy ? COL_GREEN : COL_RED,
            "%-4s", f.isBuy ? "BUY" : "SELL");
        ImGui::SameLine();
        ImGui::TextColored(COL_WHITE, "%8.2f", f.price);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%5u", f.quantity);
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, "%5.1fus", f.latencyUs);
    }
}

// ── Controls Bar ─────────────────────────────────────────────────────────────
void GUI::drawControls(SimState& state) {
    float progress = state.targetOrders > 0
        ? (float)state.processedOrders / (float)state.targetOrders
        : 0.0f;

    // Pause / Resume
    if (state.paused.load()) {
        if (ImGui::Button("  RESUME  "))
            state.paused.store(false);
    } else {
        if (ImGui::Button("  PAUSE   "))
            state.paused.store(true);
    }
    ImGui::SameLine();

    if (ImGui::Button("  RESET   "))
        state.resetFlag.store(true);
    ImGui::SameLine();

    ImGui::TextColored(COL_MUTED, "  SPEED:");
    ImGui::SameLine();
    int speed = state.speedMulti.load();
    if (ImGui::Button("1x")) state.speedMulti.store(1);
    ImGui::SameLine();
    if (ImGui::Button("2x")) state.speedMulti.store(2);
    ImGui::SameLine();
    if (ImGui::Button("5x")) state.speedMulti.store(5);
    ImGui::SameLine();
    if (ImGui::Button("MAX")) state.speedMulti.store(100);
    ImGui::SameLine();

    ImGui::TextColored(COL_MUTED, "  PROGRESS:");
    ImGui::SameLine();

    char overlay[32];
    snprintf(overlay, sizeof(overlay), "%llu / %llu",
        (unsigned long long)state.processedOrders,
        (unsigned long long)state.targetOrders);

    ImGui::SetNextItemWidth(200.0f);
    ImGui::ProgressBar(progress, { 200.0f, 0.0f }, overlay);
    ImGui::SameLine();

    if (state.finished.load())
        ImGui::TextColored(COL_GREEN, "  COMPLETE");
    else if (state.paused.load())
        ImGui::TextColored(COL_AMBER, "  PAUSED");
    else
        ImGui::TextColored(COL_BLUE, "  RUNNING");
}

// ── Shutdown ──────────────────────────────────────────────────────────────────
void GUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    if (glctx_)    SDL_GL_DeleteContext((SDL_GLContext)glctx_);
    if (window_)   SDL_DestroyWindow((SDL_Window*)window_);
    SDL_Quit();
}