#pragma once

#include <string>

#include "lumen/render/gl_api.hpp"

namespace lumen::render {

// M1 renderer skeleton: owns the GL function table and a colored-triangle pipeline.
// Real 2D batching/atlases/cameras arrive in M2. Must be used on the thread that owns the GL context.
class Renderer {
public:
    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // `gles` selects GLSL/GL flavour for shader rewriting; the context must already be current.
    bool init(gl::ProcLoader loader, bool gles, std::string& error);
    void shutdown();

    bool ready() const noexcept { return ready_; }
    void set_viewport(int width_px, int height_px);
    void clear(float r, float g, float b, float a);
    void draw_color_triangle(float angle_radians);

    const gl::Api& api() const noexcept { return gl_; }
    // "vendor / renderer / version" string for logs.
    std::string describe() const;

private:
    gl::Api gl_{};
    bool gles_ = false;
    bool ready_ = false;
    gl::GLuint program_ = 0;
    gl::GLuint vao_ = 0;
    gl::GLuint vbo_ = 0;
    gl::GLint angle_loc_ = -1;
};

}  // namespace lumen::render
