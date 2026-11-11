#pragma once

// Minimal, self-contained OpenGL ES 3.0 / OpenGL 3.3 function table.
//
// Why not a generated loader (glad)? We only need ~35 functions, and resolving them through
// SDL_GL_GetProcAddress keeps one code path for desktop GL 3.3 core and Android GLES 3.0.
// No GL headers are included, so the engine compiles on any host without GL dev packages.

#include <cstddef>
#include <string>

#ifndef APIENTRY
#  ifdef _WIN32
#    define APIENTRY __stdcall
#  else
#    define APIENTRY
#  endif
#endif

namespace lumen::gl {

using GLenum = unsigned int;
using GLbitfield = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLboolean = unsigned char;
using GLfloat = float;
using GLchar = char;
using GLubyte = unsigned char;
using GLsizeiptr = std::ptrdiff_t;

// Constants (identical in GL 3.3 core and GLES 3.0)
inline constexpr GLenum kFalse = 0;
inline constexpr GLenum kTrue = 1;
inline constexpr GLenum kTriangles = 0x0004;
inline constexpr GLenum kBlend = 0x0BE2;
inline constexpr GLenum kSrcAlpha = 0x0302;
inline constexpr GLenum kOneMinusSrcAlpha = 0x0303;
inline constexpr GLenum kFloat = 0x1406;
inline constexpr GLenum kColorBufferBit = 0x00004000;
inline constexpr GLenum kArrayBuffer = 0x8892;
inline constexpr GLenum kStaticDraw = 0x88E4;
inline constexpr GLenum kFragmentShader = 0x8B30;
inline constexpr GLenum kVertexShader = 0x8B31;
inline constexpr GLenum kCompileStatus = 0x8B81;
inline constexpr GLenum kLinkStatus = 0x8B82;
inline constexpr GLenum kInfoLogLength = 0x8B84;
inline constexpr GLenum kVendor = 0x1F00;
inline constexpr GLenum kRenderer = 0x1F01;
inline constexpr GLenum kVersion = 0x1F02;

// X-macro list: X(return, name, parameter-list)
#define LUMEN_GL_FUNCTIONS(X)                                                                      \
    X(void, GenBuffers, GLsizei, GLuint*)                                                          \
    X(void, DeleteBuffers, GLsizei, const GLuint*)                                                 \
    X(void, BindBuffer, GLenum, GLuint)                                                            \
    X(void, BufferData, GLenum, GLsizeiptr, const void*, GLenum)                                   \
    X(void, GenVertexArrays, GLsizei, GLuint*)                                                     \
    X(void, DeleteVertexArrays, GLsizei, const GLuint*)                                            \
    X(void, BindVertexArray, GLuint)                                                               \
    X(void, EnableVertexAttribArray, GLuint)                                                       \
    X(void, VertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)           \
    X(GLuint, CreateShader, GLenum)                                                                \
    X(void, DeleteShader, GLuint)                                                                  \
    X(void, ShaderSource, GLuint, GLsizei, const GLchar* const*, const GLint*)                     \
    X(void, CompileShader, GLuint)                                                                 \
    X(void, GetShaderiv, GLuint, GLenum, GLint*)                                                   \
    X(void, GetShaderInfoLog, GLuint, GLsizei, GLsizei*, GLchar*)                                  \
    X(GLuint, CreateProgram, void)                                                                 \
    X(void, DeleteProgram, GLuint)                                                                 \
    X(void, AttachShader, GLuint, GLuint)                                                          \
    X(void, LinkProgram, GLuint)                                                                   \
    X(void, GetProgramiv, GLuint, GLenum, GLint*)                                                  \
    X(void, GetProgramInfoLog, GLuint, GLsizei, GLsizei*, GLchar*)                                 \
    X(void, UseProgram, GLuint)                                                                    \
    X(GLint, GetUniformLocation, GLuint, const GLchar*)                                            \
    X(void, Uniform1f, GLint, GLfloat)                                                             \
    X(void, Uniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat)                                  \
    X(void, Viewport, GLint, GLint, GLsizei, GLsizei)                                              \
    X(void, ClearColor, GLfloat, GLfloat, GLfloat, GLfloat)                                        \
    X(void, Clear, GLbitfield)                                                                     \
    X(void, Enable, GLenum)                                                                        \
    X(void, Disable, GLenum)                                                                       \
    X(void, BlendFunc, GLenum, GLenum)                                                             \
    X(void, DrawArrays, GLenum, GLint, GLsizei)                                                    \
    X(const GLubyte*, GetString, GLenum)

struct Api {
#define LUMEN_GL_MEMBER(ret, name, ...) ret(APIENTRY* name)(__VA_ARGS__) = nullptr;
    LUMEN_GL_FUNCTIONS(LUMEN_GL_MEMBER)
#undef LUMEN_GL_MEMBER
};

// Resolves every function via `proc` (e.g. SDL_GL_GetProcAddress). Returns false and fills
// `error` with the names of missing functions if any is unavailable.
using ProcAddress = void (*)(void);
using ProcLoader = ProcAddress (*)(const char* name);
bool load_api(Api& api, ProcLoader proc, std::string& error);

}  // namespace lumen::gl
