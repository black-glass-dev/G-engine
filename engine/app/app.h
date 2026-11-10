// G-Engine — app/app.h
// One running game: world + renderer + the fixed-step game loop. Platform layers (web,
// Android, tests) create an App, feed it input and time, and ask it to draw.
#pragma once

#include <string>

#include "gfx/renderer.h"
#include "scene/world.h"

namespace ge {

class App {
public:
    /// Needs a current graphics context. Returns false when the renderer cannot start.
    bool init(i32 width, i32 height);
    void resize(i32 width, i32 height);
    /// Advances the game by `dt` seconds of real time (in fixed 1/60 s steps) and draws a frame.
    void frame(f32 dt);

    World& world() { return world_; }
    Renderer& renderer() { return renderer_; }
    RenderSettings& settings() { return settings_; }
    /// Game state for the user interface, as JSON (score, speech bubbles, statistics).
    const std::string& hud();

private:
    World world_;
    Renderer renderer_;
    RenderSettings settings_;
    RenderFrame frame_;
    f32 accumulator_ = 0;
    std::string hud_;
};

/// The sample game shipped with the engine (games/demo).
void loadDemo(World& world);

}  // namespace ge
