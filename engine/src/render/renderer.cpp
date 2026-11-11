#include "lumen/render/renderer.hpp"

#include <array>

#include "lumen/log.hpp"
#include "lumen/render/builtin_shaders.hpp"
#include "lumen/render/shader.hpp"

namespace lumen::render {
namespace {

// x, y, r, g, b — one triangle, counter-clockwise, in normalized device coordinates.
constexpr std::array<float, 15> kTriangleVertices = {
    0.0f,  0.6f, 1.0f, 0.35f, 0.25f,
   -0.6f, -0.5f, 0.25f, 0.85f, 0.95f,
    0.6f, -0.5f, 0.55f, 1.0f, 0.45f,
};

std::string str_or(const gl::Api& gl, gl::GLenum name) {
    const gl::GLubyte* s = gl.GetString(name);
    return s ? reinterpret_cast<const char*>(s) : "unknown";
}

}  // namespace

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(gl::ProcLoader loader, bool gles, std::string& error) {
    shutdown();
    gles_ = gles;
    if (!gl::load_api(gl_, loader, error)) {
        error = "failed to load GL functions (" + error + ")";
        return false;
    }
    log(LogLevel::Info, "render", "GL: " + describe());

    std::string shader_error;
    program_ = compile_program(gl_, builtin::kColorVertex, builtin::kColorFragment, gles_, shader_error);
    if (program_ == 0) {
        error = "shader build failed: " + shader_error;
        log(LogLevel::Error, "render", error);
        return false;
    }
    angle_loc_ = gl_.GetUniformLocation(program_, "u_angle");

    gl_.GenVertexArrays(1, &vao_);
    gl_.BindVertexArray(vao_);
    gl_.GenBuffers(1, &vbo_);
    gl_.BindBuffer(gl::kArrayBuffer, vbo_);
    gl_.BufferData(gl::kArrayBuffer, static_cast<gl::GLsizeiptr>(sizeof(kTriangleVertices)),
                   kTriangleVertices.data(), gl::kStaticDraw);

    const gl::GLsizei stride = 5 * static_cast<gl::GLsizei>(sizeof(float));
    gl_.EnableVertexAttribArray(0);
    gl_.VertexAttribPointer(0, 2, gl::kFloat, gl::kFalse, stride, nullptr);
    gl_.EnableVertexAttribArray(1);
    gl_.VertexAttribPointer(1, 3, gl::kFloat, gl::kFalse, stride,
                            reinterpret_cast<const void*>(2 * sizeof(float)));
    gl_.BindVertexArray(0);

    gl_.Enable(gl::kBlend);
    gl_.BlendFunc(gl::kSrcAlpha, gl::kOneMinusSrcAlpha);

    ready_ = true;
    return true;
}

void Renderer::shutdown() {
    // Safe to call even if init() failed partway: every handle is checked against 0.
    if (!ready_ && program_ == 0 && vao_ == 0 && vbo_ == 0) {
        return;
    }
    if (gl_.DeleteBuffers && vbo_) gl_.DeleteBuffers(1, &vbo_);
    if (gl_.DeleteVertexArrays && vao_) gl_.DeleteVertexArrays(1, &vao_);
    if (gl_.DeleteProgram && program_) gl_.DeleteProgram(program_);
    vbo_ = vao_ = program_ = 0;
    angle_loc_ = -1;
    ready_ = false;
}

void Renderer::set_viewport(int width_px, int height_px) {
    if (!gl_.Viewport || width_px <= 0 || height_px <= 0) return;
    gl_.Viewport(0, 0, width_px, height_px);
}

void Renderer::clear(float r, float g, float b, float a) {
    gl_.ClearColor(r, g, b, a);
    gl_.Clear(gl::kColorBufferBit);
}

void Renderer::draw_color_triangle(float angle_radians) {
    if (!ready_) return;
    gl_.UseProgram(program_);
    gl_.Uniform1f(angle_loc_, angle_radians);
    gl_.BindVertexArray(vao_);
    gl_.DrawArrays(gl::kTriangles, 0, 3);
    gl_.BindVertexArray(0);
}

std::string Renderer::describe() const {
    if (!gl_.GetString) return "GL (not loaded)";
    return str_or(gl_, gl::kVendor) + " / " + str_or(gl_, gl::kRenderer) + " / " + str_or(gl_, gl::kVersion);
}

}  // namespace lumen::render
