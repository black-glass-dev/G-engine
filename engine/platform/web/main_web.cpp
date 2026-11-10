// G-Engine — platform/web/main_web.cpp
// The WebAssembly entry points. JavaScript (web/engine.js) owns the canvas, the WebGL2 context,
// input and the frame timer, and calls these functions; strings cross the border as UTF-8.
#include <cstdlib>
#include <cstring>
#include <string>

#include "app/app.h"
#include "core/log.h"
#include "script/compiler.h"

GE_IMPORT(ge, geLog) extern "C" void geLog(int level, const char* text, int length);

namespace {

ge::App* app = nullptr;
std::string reply;  // keeps the last returned string alive until the next call

const char* give(std::string text) {
    reply = std::move(text);
    return reply.c_str();
}

void logToJs(ge::LogLevel level, const char* message, void*) { geLog(static_cast<int>(level), message, static_cast<int>(std::strlen(message))); }

}  // namespace

GE_EXPORT(ge_alloc) void* ge_alloc(int bytes) { return std::malloc(static_cast<std::size_t>(bytes > 0 ? bytes : 1)); }
GE_EXPORT(ge_free) void ge_free(void* pointer) { std::free(pointer); }

GE_EXPORT(ge_init) int ge_init(int width, int height) {
    ge::setLogSink(logToJs, nullptr);
    if (app == nullptr) app = new ge::App();
    if (!app->init(width, height)) return 0;
    ge::loadDemo(app->world());
    return 1;
}

GE_EXPORT(ge_resize) void ge_resize(int width, int height) {
    if (app != nullptr) app->resize(width, height);
}

GE_EXPORT(ge_frame) void ge_frame(float dt) {
    if (app != nullptr) app->frame(dt);
}

/// button: 0 left, 1 right, 2 up, 3 down, 4 jump, 5 action (ge::kotha::Button)
GE_EXPORT(ge_button) void ge_button(int button, int down) {
    if (app != nullptr && button >= 0 && button < static_cast<int>(ge::kotha::Button::Count)) {
        app->world().setButton(static_cast<ge::kotha::Button>(button), down != 0);
    }
}

GE_EXPORT(ge_script_count) int ge_script_count() { return app != nullptr ? app->world().scriptCount() : 0; }

GE_EXPORT(ge_script_name) const char* ge_script_name(int index) {
    return app != nullptr && index >= 0 && index < app->world().scriptCount() ? give(app->world().script(index).name) : "";
}

GE_EXPORT(ge_script_source) const char* ge_script_source(int index) {
    return app != nullptr && index >= 0 && index < app->world().scriptCount() ? give(app->world().script(index).source) : "";
}

/// Compiles a script without running it. Returns JSON: {"ok":..,"notes":[..],"errors":[..]}
GE_EXPORT(ge_check) const char* ge_check(const char* source, int length) {
    if (app == nullptr) return "{}";
    return give(ge::kotha::toJson(app->world().check(std::string(source, static_cast<std::size_t>(length)))));
}

/// Compiles a script and, if it has no errors, installs it and restarts the level.
GE_EXPORT(ge_set_script) const char* ge_set_script(int index, const char* source, int length) {
    if (app == nullptr) return "{}";
    return give(ge::kotha::toJson(app->world().setScript(index, std::string(source, static_cast<std::size_t>(length)))));
}

GE_EXPORT(ge_reset_scripts) void ge_reset_scripts() {
    if (app != nullptr) app->world().resetScripts();
}

GE_EXPORT(ge_restart) void ge_restart() {
    if (app != nullptr) app->world().restart();
}

/// The level picture that is loaded now (legend: games/demo/demo.cpp).
GE_EXPORT(ge_level) const char* ge_level() { return app != nullptr ? give(app->world().level()) : ""; }

/// Replaces the level picture and starts it; the scripts stay as they are.
GE_EXPORT(ge_set_level) void ge_set_level(const char* text, int length) {
    if (app != nullptr && length >= 0) app->world().load(std::string(text, static_cast<std::size_t>(length)));
}

GE_EXPORT(ge_hud) const char* ge_hud() { return app != nullptr ? app->hud().c_str() : "{}"; }

/// option: 0 lighting on/off, 1 bloom on/off, 2 light buffer scale (0.25..1), 3 exposure
GE_EXPORT(ge_option) void ge_option(int option, float value) {
    if (app == nullptr) return;
    ge::RenderSettings& s = app->settings();
    switch (option) {
        case 0: s.lighting = value > 0.5f; break;
        case 1: s.bloom = value > 0.5f; break;
        case 2: s.lightScale = value; break;
        case 3: s.exposure = value; break;
        default: break;
    }
}
