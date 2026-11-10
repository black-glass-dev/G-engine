// G-Engine — platform/android/main_android.cpp
// The Android entry point (NativeActivity). Owns the window, the EGL / OpenGL ES 3 context and
// the frame loop, feeds touch and key input to the game. The level and its scripts are compiled
// in (games/demo); no files are read at run time.
#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <ctime>

#include "app/app.h"
#include "core/base.h"
#include "core/log.h"
#include "script/program.h"

namespace {

using ge::i32;
using ge::kotha::Button;

constexpr const char* kLogTag = "GEngine";

void logToAndroid(ge::LogLevel level, const char* message, void*) {
    int priority = ANDROID_LOG_INFO;
    switch (level) {
        case ge::LogLevel::Debug: priority = ANDROID_LOG_DEBUG; break;
        case ge::LogLevel::Info: priority = ANDROID_LOG_INFO; break;
        case ge::LogLevel::Warn: priority = ANDROID_LOG_WARN; break;
        case ge::LogLevel::Error: priority = ANDROID_LOG_ERROR; break;
    }
    __android_log_print(priority, kLogTag, "%s", message);
}

double nowSeconds() {
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return static_cast<double>(t.tv_sec) + static_cast<double>(t.tv_nsec) * 1e-9;
}

struct AndroidState {
    struct android_app* app = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    ge::App game;
    bool gameReady = false;
    i32 width = 0, height = 0;
    double lastTime = 0;

    // One held button per active finger (-1 = none).
    struct Touch {
        i32 id = -1;
        i32 button = -1;  // Button as an integer
    };
    static constexpr i32 kMaxTouches = 16;
    Touch touches[kMaxTouches] = {};
};

void pressButton(AndroidState* s, i32 button, bool down) {
    if (button >= 0 && button < static_cast<i32>(Button::Count)) {
        s->game.world().setButton(static_cast<Button>(button), down);
    }
}

void releaseTouch(AndroidState* s, i32 id) {
    for (AndroidState::Touch& t : s->touches) {
        if (t.id == id) {
            pressButton(s, t.button, false);
            t.id = -1;
            t.button = -1;
            return;
        }
    }
}

void releaseAllTouches(AndroidState* s) {
    for (AndroidState::Touch& t : s->touches) {
        if (t.id >= 0) {
            pressButton(s, t.button, false);
            t.id = -1;
            t.button = -1;
        }
    }
}

/// Which on-screen button a screen position sits on: walk left / right on the left edge,
/// action / jump on the right edge, everything above is just looking.
i32 buttonAt(const AndroidState* s, float x, float y) {
    if (s->width <= 0 || s->height <= 0 || y < s->height * 0.35f) return -1;
    const float fx = x / static_cast<float>(s->width);
    if (fx < 0.17f) return static_cast<i32>(Button::Left);
    if (fx < 0.34f) return static_cast<i32>(Button::Right);
    if (fx < 0.66f) return -1;
    if (fx < 0.83f) return static_cast<i32>(Button::Action);
    return static_cast<i32>(Button::Jump);
}

void trackTouch(AndroidState* s, i32 id, float x, float y) {
    AndroidState::Touch* slot = nullptr;
    for (AndroidState::Touch& t : s->touches) {
        if (t.id == id) {
            slot = &t;
            break;
        }
    }
    if (slot == nullptr) {
        for (AndroidState::Touch& t : s->touches) {
            if (t.id < 0) {
                slot = &t;
                break;
            }
        }
        if (slot == nullptr) return;
        slot->id = id;
        slot->button = -1;
    }
    const i32 want = buttonAt(s, x, y);
    if (want == slot->button) return;
    pressButton(s, slot->button, false);
    slot->button = want;
    pressButton(s, slot->button, true);
}

bool keyButton(i32 keyCode, i32* out) {
    switch (keyCode) {
        case AKEYCODE_DPAD_LEFT:
        case AKEYCODE_A: *out = static_cast<i32>(Button::Left); return true;
        case AKEYCODE_DPAD_RIGHT:
        case AKEYCODE_D: *out = static_cast<i32>(Button::Right); return true;
        case AKEYCODE_DPAD_UP:
        case AKEYCODE_W:
        case AKEYCODE_SPACE: *out = static_cast<i32>(Button::Jump); return true;
        case AKEYCODE_DPAD_DOWN:
        case AKEYCODE_S: *out = static_cast<i32>(Button::Down); return true;
        case AKEYCODE_E:
        case AKEYCODE_ENTER:
        case AKEYCODE_DPAD_CENTER: *out = static_cast<i32>(Button::Action); return true;
        default: return false;
    }
}

int32_t handleInput(struct android_app* app, struct AInputEvent* event) {
    auto* s = static_cast<AndroidState*>(app->userData);
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        const int32_t action = AMotionEvent_getAction(event);
        const int32_t kind = action & AMOTION_EVENT_ACTION_MASK;
        const int32_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        switch (kind) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                trackTouch(s, AMotionEvent_getPointerId(event, index), AMotionEvent_getX(event, index),
                           AMotionEvent_getY(event, index));
                break;
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                releaseTouch(s, AMotionEvent_getPointerId(event, index));
                break;
            case AMOTION_EVENT_ACTION_MOVE:
                for (size_t i = 0; i < AMotionEvent_getPointerCount(event); ++i) {
                    trackTouch(s, AMotionEvent_getPointerId(event, i), AMotionEvent_getX(event, i), AMotionEvent_getY(event, i));
                }
                break;
            case AMOTION_EVENT_ACTION_CANCEL:
                releaseAllTouches(s);
                break;
            default: break;
        }
        return 1;
    }
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        const int32_t action = AKeyEvent_getAction(event);
        i32 button = -1;
        if (!keyButton(AKeyEvent_getKeyCode(event), &button)) return 0;
        if (action == AKEY_EVENT_ACTION_DOWN) pressButton(s, button, true);
        if (action == AKEY_EVENT_ACTION_UP) pressButton(s, button, false);
        return 1;
    }
    return 0;
}

bool initEgl(AndroidState* s) {
    s->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (s->display == EGL_NO_DISPLAY) return false;
    if (!eglInitialize(s->display, nullptr, nullptr)) return false;

    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE,
    };
    EGLConfig config = nullptr;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(s->display, configAttribs, &config, 1, &numConfigs) || numConfigs == 0) return false;

    EGLint format = 0;
    eglGetConfigAttrib(s->display, config, EGL_NATIVE_VISUAL_ID, &format);

    s->surface = eglCreateWindowSurface(s->display, config, s->app->window, nullptr);
    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    s->context = eglCreateContext(s->display, config, nullptr, contextAttribs);
    if (s->surface == EGL_NO_SURFACE || s->context == EGL_NO_CONTEXT) return false;
    if (!eglMakeCurrent(s->display, s->surface, s->surface, s->context)) return false;

    eglQuerySurface(s->display, s->surface, EGL_WIDTH, &s->width);
    eglQuerySurface(s->display, s->surface, EGL_HEIGHT, &s->height);
    eglSwapInterval(s->display, 1);
    return s->width > 0 && s->height > 0;
}

void killEgl(AndroidState* s) {
    if (s->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(s->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (s->context != EGL_NO_CONTEXT) eglDestroyContext(s->display, s->context);
        if (s->surface != EGL_NO_SURFACE) eglDestroySurface(s->display, s->surface);
        eglTerminate(s->display);
    }
    s->display = EGL_NO_DISPLAY;
    s->surface = EGL_NO_SURFACE;
    s->context = EGL_NO_CONTEXT;
}

void handleCmd(struct android_app* app, int32_t cmd) {
    auto* s = static_cast<AndroidState*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (s->app->window != nullptr && initEgl(s)) {
                if (!s->game.init(s->width, s->height)) {
                    GE_LOG_ERROR("the renderer could not start");
                } else {
                    ge::loadDemo(s->game.world());
                    s->gameReady = true;
                    s->lastTime = nowSeconds();
                }
            }
            break;
        case APP_CMD_TERM_WINDOW:
            releaseAllTouches(s);
            s->gameReady = false;
            killEgl(s);
            break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED:
            if (s->gameReady) {
                EGLint w = 0, h = 0;
                eglQuerySurface(s->display, s->surface, EGL_WIDTH, &w);
                eglQuerySurface(s->display, s->surface, EGL_HEIGHT, &h);
                if (w > 0 && h > 0 && (w != s->width || h != s->height)) {
                    s->width = w;
                    s->height = h;
                    s->game.resize(w, h);
                }
            }
            break;
        case APP_CMD_LOST_FOCUS:
            releaseAllTouches(s);
            break;
        default:
            break;
    }
}

}  // namespace

void android_main(struct android_app* app) {
    AndroidState state;
    state.app = app;
    ge::setLogSink(logToAndroid, nullptr);

    app->userData = &state;
    app->onAppCmd = handleCmd;
    app->onInputEvent = handleInput;

    while (true) {
        int ident = 0;
        int events = 0;
        struct android_poll_source* source = nullptr;
        while ((ident = ALooper_pollAll(state.gameReady ? 0 : -1, nullptr, &events, reinterpret_cast<void**>(&source))) >= 0) {
            if (source != nullptr) source->process(app, source);
            if (app->destroyRequested != 0) {
                releaseAllTouches(&state);
                state.gameReady = false;
                killEgl(&state);
                return;
            }
        }

        if (state.gameReady) {
            const double t = nowSeconds();
            float dt = static_cast<float>(t - state.lastTime);
            state.lastTime = t;
            if (dt < 0.0f) dt = 0.0f;
            if (dt > 0.1f) dt = 0.1f;  // no fast-forward after a pause
            state.game.frame(dt);
            eglSwapBuffers(state.display, state.surface);
        }
    }
}
