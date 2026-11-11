#pragma once

#include <string>
#include <string_view>

#include "lumen/render/gl_api.hpp"

namespace lumen::render {

// All engine shaders are authored as GLSL ES 3.00 ("#version 300 es").
// On desktop GL 3.3 the version line is rewritten automatically to "#version 330 core".
std::string adapt_shader_source(std::string_view source, bool gles);

// Compiles and links a program. On failure returns 0 and writes the compiler/linker log to `error`.
gl::GLuint compile_program(const gl::Api& gl, std::string_view vertex_src, std::string_view fragment_src,
                           bool gles, std::string& error);

}  // namespace lumen::render
