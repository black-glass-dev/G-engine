#pragma once

#include <functional>
#include <string>

#include "lumen/render/renderer.hpp"

namespace lumen {

struct AppConfig {
    std::string title = "Lumen2D";
    int width = 960;
    int height = 540;
    bool vsync = true;
    // 0 = run until the window is closed. >0 = run exactly this many frames (used by CI smoke tests).
    int max_frames = 0;
};

struct FrameContext {
    render::Renderer& renderer;
    double time_s;     // seconds since the loop started
    float dt_s;        // seconds since the previous frame
    int frame_index;   // 0-based
    int width_px;      // drawable size in pixels
    int height_px;
};

using FrameCallback = std::function<void(const FrameContext&)>;

// Creates a window + GL context via SDL3, runs the event/frame loop and returns a process exit code.
// Handles SDL_EVENT_QUIT; on Android the app lifecycle is delegated to SDL3.
int run_app(const AppConfig& config, const FrameCallback& on_frame);

}  // namespace lumen
