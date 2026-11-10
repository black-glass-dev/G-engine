// G-Engine — gfx/gl.h
// The graphics API the renderer is written against: OpenGL ES 3.0, reduced to what the engine
// uses and with pointer-free signatures so the very same C++ renderer runs on
//   * the web      — the functions are imported from JavaScript (WebGL2), see web/engine.js
//   * Android      — thin wrappers over GLES 3 (gfx/gl_gles3.cpp, roadmap phase 2)
//   * a PC test    — do-nothing stubs (gfx/gl_null.cpp), so the whole game loop runs in unit tests
// Object names are plain integers (0 = none), enum values are the standard OpenGL ES ones.
#pragma once

#include "core/base.h"

#define GX(name) GE_IMPORT(gx, name)

extern "C" {

// ---- state ----
GX(gxViewport) void gxViewport(ge::i32 x, ge::i32 y, ge::i32 w, ge::i32 h);
GX(gxEnable) void gxEnable(ge::u32 cap);
GX(gxDisable) void gxDisable(ge::u32 cap);
GX(gxBlendFunc) void gxBlendFunc(ge::u32 src, ge::u32 dst);
GX(gxBlendEquation) void gxBlendEquation(ge::u32 mode);
GX(gxDepthFunc) void gxDepthFunc(ge::u32 func);
GX(gxDepthMask) void gxDepthMask(ge::i32 enabled);
GX(gxColorMask) void gxColorMask(ge::i32 r, ge::i32 g, ge::i32 b, ge::i32 a);
GX(gxClearColor) void gxClearColor(float r, float g, float b, float a);
GX(gxClearDepth) void gxClearDepth(float depth);
GX(gxClear) void gxClear(ge::u32 mask);
/// Clears one colour attachment of the bound framebuffer (glClearBufferfv with GL_COLOR).
GX(gxClearColorBuffer) void gxClearColorBuffer(ge::i32 drawBuffer, float r, float g, float b, float a);

// ---- shaders ----
GX(gxCreateShader) ge::u32 gxCreateShader(ge::u32 type);
GX(gxShaderSource) void gxShaderSource(ge::u32 shader, const char* source, ge::i32 length);
/// Compiles and returns 1 on success. On failure the log is written to `log` (truncated to `capacity`).
GX(gxCompileShader) ge::i32 gxCompileShader(ge::u32 shader, char* log, ge::i32 capacity);
GX(gxDeleteShader) void gxDeleteShader(ge::u32 shader);
GX(gxCreateProgram) ge::u32 gxCreateProgram();
GX(gxAttachShader) void gxAttachShader(ge::u32 program, ge::u32 shader);
GX(gxLinkProgram) ge::i32 gxLinkProgram(ge::u32 program, char* log, ge::i32 capacity);
GX(gxUseProgram) void gxUseProgram(ge::u32 program);
GX(gxDeleteProgram) void gxDeleteProgram(ge::u32 program);
GX(gxGetUniformLocation) ge::i32 gxGetUniformLocation(ge::u32 program, const char* name, ge::i32 length);
GX(gxUniform1i) void gxUniform1i(ge::i32 location, ge::i32 v);
GX(gxUniform1f) void gxUniform1f(ge::i32 location, float x);
GX(gxUniform2f) void gxUniform2f(ge::i32 location, float x, float y);
GX(gxUniform3f) void gxUniform3f(ge::i32 location, float x, float y, float z);
GX(gxUniform4f) void gxUniform4f(ge::i32 location, float x, float y, float z, float w);

// ---- vertex data ----
GX(gxCreateBuffer) ge::u32 gxCreateBuffer();
GX(gxBindBuffer) void gxBindBuffer(ge::u32 target, ge::u32 buffer);
GX(gxBufferData) void gxBufferData(ge::u32 target, const void* data, ge::i32 bytes, ge::u32 usage);
GX(gxDeleteBuffer) void gxDeleteBuffer(ge::u32 buffer);
GX(gxCreateVertexArray) ge::u32 gxCreateVertexArray();
GX(gxBindVertexArray) void gxBindVertexArray(ge::u32 vao);
GX(gxDeleteVertexArray) void gxDeleteVertexArray(ge::u32 vao);
GX(gxEnableVertexAttribArray) void gxEnableVertexAttribArray(ge::u32 index);
/// Float attributes only (which is all the engine uses).
GX(gxVertexAttribPointer) void gxVertexAttribPointer(ge::u32 index, ge::i32 size, ge::i32 stride, ge::i32 offset);
GX(gxVertexAttribDivisor) void gxVertexAttribDivisor(ge::u32 index, ge::u32 divisor);

// ---- textures ----
GX(gxCreateTexture) ge::u32 gxCreateTexture();
GX(gxBindTexture) void gxBindTexture(ge::u32 target, ge::u32 texture);
GX(gxActiveTexture) void gxActiveTexture(ge::u32 unit);
/// `pixels` may be null (allocate only); otherwise it holds w*h*bytesPerPixel bytes.
GX(gxTexImage2D) void gxTexImage2D(ge::u32 target, ge::i32 level, ge::u32 internalFormat, ge::i32 w, ge::i32 h, ge::u32 format,
                                   ge::u32 type, const void* pixels, ge::i32 bytes);
GX(gxTexParameteri) void gxTexParameteri(ge::u32 target, ge::u32 name, ge::i32 value);
GX(gxGenerateMipmap) void gxGenerateMipmap(ge::u32 target);
GX(gxDeleteTexture) void gxDeleteTexture(ge::u32 texture);

// ---- framebuffers ----
GX(gxCreateFramebuffer) ge::u32 gxCreateFramebuffer();
GX(gxBindFramebuffer) void gxBindFramebuffer(ge::u32 target, ge::u32 framebuffer);
GX(gxFramebufferTexture2D) void gxFramebufferTexture2D(ge::u32 target, ge::u32 attachment, ge::u32 textarget, ge::u32 texture, ge::i32 level);
/// 1 when the bound framebuffer is complete.
GX(gxFramebufferComplete) ge::i32 gxFramebufferComplete(ge::u32 target);
/// count colour attachments are drawn to (0 = none: depth only).
GX(gxDrawBuffers) void gxDrawBuffers(ge::i32 count);
GX(gxDeleteFramebuffer) void gxDeleteFramebuffer(ge::u32 framebuffer);

// ---- drawing ----
GX(gxDrawArrays) void gxDrawArrays(ge::u32 mode, ge::i32 first, ge::i32 count);
GX(gxDrawArraysInstanced) void gxDrawArraysInstanced(ge::u32 mode, ge::i32 first, ge::i32 count, ge::i32 instances);

// ---- capabilities ----
/// 1 when rendering to 16-bit float colour targets works (EXT_color_buffer_half_float / _float).
GX(gxSupportsFloatTargets) ge::i32 gxSupportsFloatTargets();
GX(gxGetError) ge::u32 gxGetError();

}  // extern "C"

#undef GX

namespace ge::gx {

inline constexpr u32 kTriangles = 0x0004, kTriangleStrip = 0x0005;
inline constexpr u32 kDepthBufferBit = 0x0100, kColorBufferBit = 0x4000;
inline constexpr u32 kBlend = 0x0BE2, kDepthTest = 0x0B71, kScissorTest = 0x0C11, kCullFace = 0x0B44;
inline constexpr u32 kZero = 0, kOne = 1, kSrcAlpha = 0x0302, kOneMinusSrcAlpha = 0x0303;
inline constexpr u32 kFuncAdd = 0x8006, kMin = 0x8007, kMax = 0x8008;
inline constexpr u32 kLess = 0x0201, kLequal = 0x0203, kAlways = 0x0207;
inline constexpr u32 kVertexShader = 0x8B31, kFragmentShader = 0x8B30;
inline constexpr u32 kArrayBuffer = 0x8892, kStaticDraw = 0x88E4, kDynamicDraw = 0x88E8, kStreamDraw = 0x88E0;
inline constexpr u32 kTexture2D = 0x0DE1, kTexture0 = 0x84C0;
inline constexpr u32 kTextureMagFilter = 0x2800, kTextureMinFilter = 0x2801, kTextureWrapS = 0x2802, kTextureWrapT = 0x2803;
inline constexpr i32 kNearest = 0x2600, kLinear = 0x2601, kLinearMipmapLinear = 0x2703, kClampToEdge = 0x812F, kRepeat = 0x2901;
inline constexpr u32 kRgba = 0x1908, kRgba8 = 0x8058, kSrgb8Alpha8 = 0x8C43, kRgba16F = 0x881A;
inline constexpr u32 kDepthComponent = 0x1902, kDepthComponent24 = 0x81A6, kDepthComponent16 = 0x81A5;
inline constexpr u32 kUnsignedByte = 0x1401, kUnsignedShort = 0x1403, kUnsignedInt = 0x1405, kHalfFloat = 0x140B, kFloat = 0x1406;
inline constexpr u32 kFramebuffer = 0x8D40, kColorAttachment0 = 0x8CE0, kDepthAttachment = 0x8D00;

}  // namespace ge::gx
