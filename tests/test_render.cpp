#include <doctest/doctest.h>

#include <string>

#include "lumen/render/gl_api.hpp"
#include "lumen/render/renderer.hpp"

namespace {

// Non-null dummy address: the loader only stores it, it is never called in these tests.
void dummy_gl_function() {}

lumen::gl::ProcAddress all_present(const char*) {
    return &dummy_gl_function;
}

lumen::gl::ProcAddress none_present(const char*) {
    return nullptr;
}

lumen::gl::ProcAddress only_glcreateshader_missing(const char* name) {
    return std::string(name) == "glCreateShader" ? nullptr : &dummy_gl_function;
}

}  // namespace

TEST_CASE("GL loader succeeds when every function resolves") {
    lumen::gl::Api api{};
    std::string error;
    CHECK(lumen::gl::load_api(api, &all_present, error));
    CHECK(error.empty());
    CHECK(api.CreateShader != nullptr);
    CHECK(api.DrawArrays != nullptr);
    CHECK(api.GetString != nullptr);
}

TEST_CASE("GL loader reports every missing function by name") {
    lumen::gl::Api api{};
    std::string error;
    CHECK_FALSE(lumen::gl::load_api(api, &none_present, error));
    CHECK(error.find("glCreateShader") != std::string::npos);
    CHECK(error.find("glDrawArrays") != std::string::npos);
    CHECK(api.CreateShader == nullptr);
}

TEST_CASE("GL loader flags a single missing function") {
    lumen::gl::Api api{};
    std::string error;
    CHECK_FALSE(lumen::gl::load_api(api, &only_glcreateshader_missing, error));
    CHECK(error == "glCreateShader");
}

TEST_CASE("renderer init fails cleanly without GL functions") {
    lumen::render::Renderer renderer;
    std::string error;
    CHECK_FALSE(renderer.init(&none_present, false, error));
    CHECK(error.find("failed to load GL functions") != std::string::npos);
    CHECK_FALSE(renderer.ready());
}

TEST_CASE("renderer draw calls before init are no-ops") {
    lumen::render::Renderer renderer;
    CHECK_FALSE(renderer.ready());
    renderer.draw_color_triangle(1.0f);  // must not crash: ready() is false
    renderer.set_viewport(640, 480);     // GL table is empty; must be ignored
    renderer.shutdown();                 // safe on a never-initialised renderer
    CHECK_FALSE(renderer.ready());
}

TEST_CASE("renderer describe() works without GL loaded") {
    lumen::render::Renderer renderer;
    CHECK(renderer.describe() == "GL (not loaded)");
}
