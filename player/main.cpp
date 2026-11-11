// lumen_player — standalone player. M1: opens a window and draws the colored-triangle pipeline proof.
// On Android SDL3 calls SDL_main (see SDL_main.h), so keep this include.
#include <SDL3/SDL_main.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "lumen/log.hpp"
#include "lumen/platform/app.hpp"

namespace {

void print_usage() {
    std::puts(
        "lumen_player [--frames N] [--title TEXT]\n"
        "  --frames N   run N frames then exit (smoke test); default: run until closed\n"
        "  --title TEXT window title\n");
}

}  // namespace

int main(int argc, char** argv) {
    lumen::AppConfig config;
    config.title = "Lumen2D — M1 triangle";

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--frames") == 0 && i + 1 < argc) {
            config.max_frames = std::atoi(argv[++i]);
        } else if (std::strcmp(arg, "--title") == 0 && i + 1 < argc) {
            config.title = argv[++i];
        } else if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            print_usage();
            return 0;
        } else {
            lumen::log(lumen::LogLevel::Error, "player", std::string("unknown argument: ") + arg);
            print_usage();
            return 2;
        }
    }

    return lumen::run_app(config, [](const lumen::FrameContext& f) {
        // Background color slowly pulses; the triangle spins. Proves clear + shader + draw path.
        const float pulse = 0.5f + 0.5f * static_cast<float>(std::sin(f.time_s * 0.8));
        f.renderer.clear(0.05f + 0.05f * pulse, 0.07f, 0.12f + 0.08f * pulse, 1.0f);
        f.renderer.draw_color_triangle(static_cast<float>(f.time_s));
    });
}
