// G-Engine — app/app.cpp
#include "app/app.h"

#include <cstdio>

#include "core/log.h"
#include "gfx/atlas_gen.h"
#include "script/compiler.h"

namespace ge {

namespace {

void appendJsonString(std::string& out, const std::string& s) {
    out += '"';
    for (const char c : s) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            out += ' ';
        } else {
            out += c;
        }
    }
    out += '"';
}

void appendNumber(std::string& out, f64 v) {
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%.5g", v);
    out += buffer;
}

}  // namespace

bool App::init(i32 width, i32 height) {
    if (!renderer_.init()) return false;
    AtlasData atlas;
    generateAtlas(atlas);
    renderer_.setAtlas(atlas.albedo.data(), atlas.normal.data(), atlas.width, atlas.height);
    resize(width, height);
    return true;
}

void App::resize(i32 width, i32 height) {
    renderer_.resize(width, height);
    world_.setAspect(static_cast<f32>(renderer_.width()) / static_cast<f32>(renderer_.height()));
}

void App::frame(f32 dt) {
    // Fixed steps keep physics and scripts identical on every device; a long pause
    // (the browser tab was hidden) must not turn into a burst of catch-up steps.
    accumulator_ += clamp(dt, 0.0f, 0.1f);
    int steps = 0;
    while (accumulator_ >= World::kStep && steps < 6) {
        world_.step();
        accumulator_ -= World::kStep;
        ++steps;
    }
    world_.buildFrame(frame_);
    renderer_.render(frame_, settings_);
}

const std::string& App::hud() {
    std::string& out = hud_;
    out.clear();
    out += "{\"state\":\"";
    out += world_.state() == GameState::Won ? "won" : world_.state() == GameState::Lost ? "lost" : "playing";
    out += "\",\"time\":";
    appendNumber(out, world_.time());
    out += ",\"score\":";
    appendNumber(out, world_.variable(kotha::canonicalName("score")));
    if (const Object* p = world_.player()) {
        out += ",\"player\":{\"x\":";
        appendNumber(out, p->pos.x);
        out += ",\"y\":";
        appendNumber(out, p->pos.y);
        out += ",\"ground\":";
        out += p->mover.onGround ? "true" : "false";
        out += '}';
    }
    out += ",\"vars\":{";
    bool first = true;
    for (const auto& [key, value] : world_.variables()) {
        if (!first) out += ',';
        first = false;
        appendJsonString(out, key);
        out += ':';
        appendNumber(out, value);
    }
    out += "},\"bubbles\":[";
    std::vector<Bubble> bubbles;
    world_.bubbles(bubbles);
    const f32 viewH = world_.viewHeight();
    const f32 viewW = viewH * static_cast<f32>(renderer_.width()) / static_cast<f32>(max(renderer_.height(), 1));
    const Vec2 cam = world_.camera();
    for (std::size_t i = 0; i < bubbles.size(); ++i) {
        if (i > 0) out += ',';
        out += "{\"x\":";
        appendNumber(out, (bubbles[i].world.x - cam.x) / viewW + 0.5f);
        out += ",\"y\":";
        appendNumber(out, 0.5f - (bubbles[i].world.y - cam.y) / viewH);
        out += ",\"text\":";
        appendJsonString(out, bubbles[i].text);
        out += '}';
    }
    out += "],\"lights\":[";
    std::vector<World::ObjectView> views;
    world_.objectViews(views);
    bool firstLight = true;
    for (const World::ObjectView& v : views) {
        if (!v.light) continue;
        if (!firstLight) out += ',';
        firstLight = false;
        out += "{\"x\":";
        appendNumber(out, v.pos.x);
        out += ",\"y\":";
        appendNumber(out, v.pos.y);
        out += ",\"radius\":";
        appendNumber(out, v.lightRadius);
        out += ",\"intensity\":";
        appendNumber(out, v.lightIntensity);
        out += '}';
    }
    const RenderStats& stats = renderer_.stats();
    out += "],\"stats\":{\"sprites\":" + std::to_string(stats.sprites) + ",\"lights\":" + std::to_string(stats.lights) +
           ",\"shadowLights\":" + std::to_string(stats.shadowLights) + ",\"edges\":" + std::to_string(stats.shadowEdges) +
           ",\"draws\":" + std::to_string(stats.drawCalls) + ",\"hdr\":" + (renderer_.floatTargets() ? "true" : "false") + "}}";
    return out;
}

}  // namespace ge
