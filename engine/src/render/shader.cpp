#include "lumen/render/shader.hpp"

#include <vector>

#include "lumen/log.hpp"

namespace lumen::render {
namespace {

constexpr std::string_view kVersionPrefix = "#version";

std::string read_info_log(const gl::Api& gl, gl::GLuint object, bool is_program) {
    gl::GLint len = 0;
    if (is_program) {
        gl.GetProgramiv(object, gl::kInfoLogLength, &len);
    } else {
        gl.GetShaderiv(object, gl::kInfoLogLength, &len);
    }
    if (len <= 1) {
        return {};
    }
    std::vector<char> buf(static_cast<size_t>(len));
    if (is_program) {
        gl.GetProgramInfoLog(object, len, nullptr, buf.data());
    } else {
        gl.GetShaderInfoLog(object, len, nullptr, buf.data());
    }
    return std::string(buf.data());
}

gl::GLuint compile_stage(const gl::Api& gl, gl::GLenum type, std::string_view src, std::string& error) {
    gl::GLuint shader = gl.CreateShader(type);
    if (shader == 0) {
        error = "glCreateShader failed";
        return 0;
    }
    const char* text = src.data();
    gl::GLint length = static_cast<gl::GLint>(src.size());
    gl.ShaderSource(shader, 1, &text, &length);
    gl.CompileShader(shader);

    gl::GLint ok = 0;
    gl.GetShaderiv(shader, gl::kCompileStatus, &ok);
    if (ok != static_cast<gl::GLint>(gl::kTrue)) {
        error = read_info_log(gl, shader, false);
        gl.DeleteShader(shader);
        return 0;
    }
    return shader;
}

}  // namespace

std::string adapt_shader_source(std::string_view source, bool gles) {
    const std::string_view header = gles ? "#version 300 es" : "#version 330 core";
    std::string out;
    if (source.substr(0, kVersionPrefix.size()) == kVersionPrefix) {
        const size_t eol = source.find('\n');
        out.assign(header);
        if (eol != std::string_view::npos) {
            out.append(source.substr(eol));
        }
    } else {
        out.assign(header);
        out.append("\n");
        out.append(source);
    }
    return out;
}

gl::GLuint compile_program(const gl::Api& gl, std::string_view vertex_src, std::string_view fragment_src,
                           bool gles, std::string& error) {
    error.clear();
    const std::string vs_text = adapt_shader_source(vertex_src, gles);
    const std::string fs_text = adapt_shader_source(fragment_src, gles);

    std::string stage_error;
    gl::GLuint vs = compile_stage(gl, gl::kVertexShader, vs_text, stage_error);
    if (vs == 0) {
        error = "vertex shader: " + stage_error;
        return 0;
    }
    gl::GLuint fs = compile_stage(gl, gl::kFragmentShader, fs_text, stage_error);
    if (fs == 0) {
        gl.DeleteShader(vs);
        error = "fragment shader: " + stage_error;
        return 0;
    }

    gl::GLuint program = gl.CreateProgram();
    gl.AttachShader(program, vs);
    gl.AttachShader(program, fs);
    gl.LinkProgram(program);
    // Shader objects stay alive until the program is deleted; mark them for deletion now.
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);

    gl::GLint ok = 0;
    gl.GetProgramiv(program, gl::kLinkStatus, &ok);
    if (ok != static_cast<gl::GLint>(gl::kTrue)) {
        error = "link: " + read_info_log(gl, program, true);
        gl.DeleteProgram(program);
        return 0;
    }
    return program;
}

}  // namespace lumen::render
