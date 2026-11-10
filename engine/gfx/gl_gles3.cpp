// G-Engine — gfx/gl_gles3.cpp
// The graphics backend for Android: thin wrappers over OpenGL ES 3. The renderer calls the gx*
// functions (gfx/gl.h); here each one forwards to the matching GLES 3 call. Compiled only when
// building for Android; the web build imports the same names from JavaScript instead.
#include "gfx/gl.h"

#include <GLES3/gl3.h>

#include <cstring>
#include <string>

namespace {

// 1 when the context can render to 16-bit float colour targets (checked once, cached).
ge::i32 floatTargetsChecked = -1;

}  // namespace

extern "C" {

// ---- state ----
void gxViewport(ge::i32 x, ge::i32 y, ge::i32 w, ge::i32 h) { glViewport(x, y, w, h); }
void gxEnable(ge::u32 cap) { glEnable(cap); }
void gxDisable(ge::u32 cap) { glDisable(cap); }
void gxBlendFunc(ge::u32 src, ge::u32 dst) { glBlendFunc(src, dst); }
void gxBlendEquation(ge::u32 mode) { glBlendEquation(mode); }
void gxDepthFunc(ge::u32 func) { glDepthFunc(func); }
void gxDepthMask(ge::i32 enabled) { glDepthMask(enabled ? GL_TRUE : GL_FALSE); }
void gxColorMask(ge::i32 r, ge::i32 g, ge::i32 b, ge::i32 a) {
    glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE, b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
}
void gxClearColor(float r, float g, float b, float a) { glClearColor(r, g, b, a); }
void gxClearDepth(float depth) { glClearDepthf(depth); }
void gxClear(ge::u32 mask) { glClear(mask); }
void gxClearColorBuffer(ge::i32 drawBuffer, float r, float g, float b, float a) {
    const GLfloat color[4] = {r, g, b, a};
    glClearBufferfv(GL_COLOR, drawBuffer, color);
}

// ---- shaders ----
ge::u32 gxCreateShader(ge::u32 type) { return glCreateShader(type); }
void gxShaderSource(ge::u32 shader, const char* source, ge::i32 length) {
    const GLchar* text = source;
    const GLint size = length;
    glShaderSource(shader, 1, &text, &size);
}
ge::i32 gxCompileShader(ge::u32 shader, char* log, ge::i32 capacity) {
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (log != nullptr && capacity > 0) {
        log[0] = 0;
        if (!ok) glGetShaderInfoLog(shader, capacity, nullptr, log);
    }
    return ok ? 1 : 0;
}
void gxDeleteShader(ge::u32 shader) { glDeleteShader(shader); }
ge::u32 gxCreateProgram() { return glCreateProgram(); }
void gxAttachShader(ge::u32 program, ge::u32 shader) { glAttachShader(program, shader); }
ge::i32 gxLinkProgram(ge::u32 program, char* log, ge::i32 capacity) {
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (log != nullptr && capacity > 0) {
        log[0] = 0;
        if (!ok) glGetProgramInfoLog(program, capacity, nullptr, log);
    }
    return ok ? 1 : 0;
}
void gxUseProgram(ge::u32 program) { glUseProgram(program); }
void gxDeleteProgram(ge::u32 program) { glDeleteProgram(program); }
ge::i32 gxGetUniformLocation(ge::u32 program, const char* name, ge::i32 length) {
    const std::string key = length < 0 ? std::string(name) : std::string(name, static_cast<std::size_t>(length));
    return glGetUniformLocation(program, key.c_str());
}
void gxUniform1i(ge::i32 location, ge::i32 v) { glUniform1i(location, v); }
void gxUniform1f(ge::i32 location, float x) { glUniform1f(location, x); }
void gxUniform2f(ge::i32 location, float x, float y) { glUniform2f(location, x, y); }
void gxUniform3f(ge::i32 location, float x, float y, float z) { glUniform3f(location, x, y, z); }
void gxUniform4f(ge::i32 location, float x, float y, float z, float w) { glUniform4f(location, x, y, z, w); }

// ---- vertex data ----
ge::u32 gxCreateBuffer() {
    GLuint name = 0;
    glGenBuffers(1, &name);
    return name;
}
void gxBindBuffer(ge::u32 target, ge::u32 buffer) { glBindBuffer(target, buffer); }
void gxBufferData(ge::u32 target, const void* data, ge::i32 bytes, ge::u32 usage) {
    glBufferData(target, static_cast<GLsizeiptr>(bytes), data, usage);
}
void gxDeleteBuffer(ge::u32 buffer) { glDeleteBuffers(1, &buffer); }
ge::u32 gxCreateVertexArray() {
    GLuint name = 0;
    glGenVertexArrays(1, &name);
    return name;
}
void gxBindVertexArray(ge::u32 vao) { glBindVertexArray(vao); }
void gxDeleteVertexArray(ge::u32 vao) { glDeleteVertexArrays(1, &vao); }
void gxEnableVertexAttribArray(ge::u32 index) { glEnableVertexAttribArray(index); }
void gxVertexAttribPointer(ge::u32 index, ge::i32 size, ge::i32 stride, ge::i32 offset) {
    glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(static_cast<std::intptr_t>(offset)));
}
void gxVertexAttribDivisor(ge::u32 index, ge::u32 divisor) { glVertexAttribDivisor(index, divisor); }

// ---- textures ----
ge::u32 gxCreateTexture() {
    GLuint name = 0;
    glGenTextures(1, &name);
    return name;
}
void gxBindTexture(ge::u32 target, ge::u32 texture) { glBindTexture(target, texture); }
void gxActiveTexture(ge::u32 unit) { glActiveTexture(unit); }
void gxTexImage2D(ge::u32 target, ge::i32 level, ge::u32 internalFormat, ge::i32 w, ge::i32 h, ge::u32 format, ge::u32 type,
                  const void* pixels, ge::i32) {
    glTexImage2D(target, level, static_cast<GLint>(internalFormat), w, h, 0, format, type, pixels);
}
void gxTexParameteri(ge::u32 target, ge::u32 name, ge::i32 value) { glTexParameteri(target, name, value); }
void gxGenerateMipmap(ge::u32 target) { glGenerateMipmap(target); }
void gxDeleteTexture(ge::u32 texture) { glDeleteTextures(1, &texture); }

// ---- framebuffers ----
ge::u32 gxCreateFramebuffer() {
    GLuint name = 0;
    glGenFramebuffers(1, &name);
    return name;
}
void gxBindFramebuffer(ge::u32 target, ge::u32 framebuffer) { glBindFramebuffer(target, framebuffer); }
void gxFramebufferTexture2D(ge::u32 target, ge::u32 attachment, ge::u32 textarget, ge::u32 texture, ge::i32 level) {
    glFramebufferTexture2D(target, attachment, textarget, texture, level);
}
ge::i32 gxFramebufferComplete(ge::u32 target) { return glCheckFramebufferStatus(target) == GL_FRAMEBUFFER_COMPLETE ? 1 : 0; }
void gxDrawBuffers(ge::i32 count) {
    GLenum buffers[8] = {};
    const GLsizei n = count > 8 ? 8 : static_cast<GLsizei>(count);
    for (GLsizei i = 0; i < n; ++i) buffers[i] = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i);
    glDrawBuffers(n, buffers);
}
void gxDeleteFramebuffer(ge::u32 framebuffer) { glDeleteFramebuffers(1, &framebuffer); }

// ---- drawing ----
void gxDrawArrays(ge::u32 mode, ge::i32 first, ge::i32 count) { glDrawArrays(mode, first, count); }
void gxDrawArraysInstanced(ge::u32 mode, ge::i32 first, ge::i32 count, ge::i32 instances) {
    glDrawArraysInstanced(mode, first, count, instances);
}

// ---- capabilities ----
ge::i32 gxSupportsFloatTargets() {
    if (floatTargetsChecked < 0) {
        floatTargetsChecked = 0;
        const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        if (extensions != nullptr &&
            (std::strstr(extensions, "GL_EXT_color_buffer_float") != nullptr ||
             std::strstr(extensions, "GL_EXT_color_buffer_half_float") != nullptr)) {
            floatTargetsChecked = 1;
        }
    }
    return floatTargetsChecked;
}
ge::u32 gxGetError() { return glGetError(); }

}  // extern "C"
