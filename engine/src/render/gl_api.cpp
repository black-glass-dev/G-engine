#include "lumen/render/gl_api.hpp"

#include "lumen/log.hpp"

namespace lumen::gl {

bool load_api(Api& api, ProcLoader proc, std::string& error) {
    error.clear();
    bool ok = true;

#define LUMEN_GL_LOAD(ret, name, ...)                                                              \
    do {                                                                                           \
        using Fn = ret(APIENTRY*)(__VA_ARGS__);                                                    \
        api.name = reinterpret_cast<Fn>(proc("gl" #name));                                         \
        if (!api.name) {                                                                           \
            ok = false;                                                                            \
            error += std::string(error.empty() ? "" : ", ") + "gl" #name;                          \
        }                                                                                          \
    } while (0);
    LUMEN_GL_FUNCTIONS(LUMEN_GL_LOAD)
#undef LUMEN_GL_LOAD

    if (!ok) {
        log(LogLevel::Error, "render", "missing GL functions: " + error);
    }
    return ok;
}

}  // namespace lumen::gl
