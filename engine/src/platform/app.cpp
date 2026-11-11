#include "lumen/platform/app.hpp"

#include <SDL3/SDL.h>

#include "lumen/log.hpp"

namespace lumen {
namespace {

gl::ProcAddress sdl_proc(const char* name) {
    return SDL_GL_GetProcAddress(name);
}

void configure_gl_attributes(bool gles) {
    if (gles) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    } else {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
}

}  // namespace

int run_app(const AppConfig& config, const FrameCallback& on_frame) {
#ifdef __ANDROID__
    constexpr bool kGles = true;
#else
    constexpr bool kGles = false;
#endif

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        log(LogLevel::Error, "platform", std::string("SDL_Init failed: ") + SDL_GetError());
        return 1;
    }

    configure_gl_attributes(kGles);
    SDL_Window* window = SDL_CreateWindow(config.title.c_str(), config.width, config.height,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        log(LogLevel::Error, "platform", std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        log(LogLevel::Error, "platform", std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(config.vsync ? 1 : 0);

    int exit_code = 0;
    {
        render::Renderer renderer;
        std::string error;
        if (!renderer.init(&sdl_proc, kGles, error)) {
            log(LogLevel::Error, "render", error);
            exit_code = 1;
        } else {
            const Uint64 start_ns = SDL_GetTicksNS();
            Uint64 last_ns = start_ns;
            bool running = true;
            int frame = 0;
            while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_EVENT_QUIT) {
                        running = false;
                    }
                }
                if (!running) break;

                const Uint64 now_ns = SDL_GetTicksNS();
                FrameContext ctx{
                    renderer,
                    static_cast<double>(now_ns - start_ns) * 1e-9,
                    static_cast<float>(static_cast<double>(now_ns - last_ns) * 1e-9),
                    frame,
                    0,
                    0,
                };
                last_ns = now_ns;

                int w = 0, h = 0;
                SDL_GetWindowSizeInPixels(window, &w, &h);
                ctx.width_px = w;
                ctx.height_px = h;
                renderer.set_viewport(w, h);

                if (on_frame) on_frame(ctx);

                SDL_GL_SwapWindow(window);
                ++frame;
                if (config.max_frames > 0 && frame >= config.max_frames) {
                    running = false;
                }
            }
            log(LogLevel::Info, "platform", "exited after " + std::to_string(frame) + " frames");
        }
    }

    SDL_GL_MakeCurrent(window, nullptr);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}

}  // namespace lumen
