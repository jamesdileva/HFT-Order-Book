#include <SDL.h>
#include <thread>
#include <string>
#include "GameManager.hpp"
#include "GUI.hpp"

int main(int argc, char* argv[]) {
    uint64_t totalOrders = 100000;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--orders" && i + 1 < argc)
            totalOrders = std::stoull(argv[++i]);
    }

    GameManager gm;
    gm.state().totalOrders = totalOrders;
    gm.start();

    GUI gui;
    if (!gui.init("HFT Order Book", 1280, 760)) return 1;

    while (gui.render(gm)) {}

    gui.shutdown();
    gm.stop();
    return 0;
}