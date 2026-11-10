// G-Engine — gfx/gl_null.cpp
// A graphics backend that draws nothing. It lets the complete engine (game loop, renderer
// bookkeeping, scripts, physics) run on a PC without a GPU — used by the unit tests.
#include "gfx/gl.h"

namespace {
ge::u32 nextName = 1;
}

extern "C" {

void gxViewport(ge::i32, ge::i32, ge::i32, ge::i32) {}
void gxEnable(ge::u32) {}
void gxDisable(ge::u32) {}
void gxBlendFunc(ge::u32, ge::u32) {}
void gxBlendEquation(ge::u32) {}
void gxDepthFunc(ge::u32) {}
void gxDepthMask(ge::i32) {}
void gxColorMask(ge::i32, ge::i32, ge::i32, ge::i32) {}
void gxClearColor(float, float, float, float) {}
void gxClearDepth(float) {}
void gxClear(ge::u32) {}
void gxClearColorBuffer(ge::i32, float, float, float, float) {}

ge::u32 gxCreateShader(ge::u32) { return nextName++; }
void gxShaderSource(ge::u32, const char*, ge::i32) {}
ge::i32 gxCompileShader(ge::u32, char*, ge::i32) { return 1; }
void gxDeleteShader(ge::u32) {}
ge::u32 gxCreateProgram() { return nextName++; }
void gxAttachShader(ge::u32, ge::u32) {}
ge::i32 gxLinkProgram(ge::u32, char*, ge::i32) { return 1; }
void gxUseProgram(ge::u32) {}
void gxDeleteProgram(ge::u32) {}
ge::i32 gxGetUniformLocation(ge::u32, const char*, ge::i32) { return 0; }
void gxUniform1i(ge::i32, ge::i32) {}
void gxUniform1f(ge::i32, float) {}
void gxUniform2f(ge::i32, float, float) {}
void gxUniform3f(ge::i32, float, float, float) {}
void gxUniform4f(ge::i32, float, float, float, float) {}

ge::u32 gxCreateBuffer() { return nextName++; }
void gxBindBuffer(ge::u32, ge::u32) {}
void gxBufferData(ge::u32, const void*, ge::i32, ge::u32) {}
void gxDeleteBuffer(ge::u32) {}
ge::u32 gxCreateVertexArray() { return nextName++; }
void gxBindVertexArray(ge::u32) {}
void gxDeleteVertexArray(ge::u32) {}
void gxEnableVertexAttribArray(ge::u32) {}
void gxVertexAttribPointer(ge::u32, ge::i32, ge::i32, ge::i32) {}
void gxVertexAttribDivisor(ge::u32, ge::u32) {}

ge::u32 gxCreateTexture() { return nextName++; }
void gxBindTexture(ge::u32, ge::u32) {}
void gxActiveTexture(ge::u32) {}
void gxTexImage2D(ge::u32, ge::i32, ge::u32, ge::i32, ge::i32, ge::u32, ge::u32, const void*, ge::i32) {}
void gxTexParameteri(ge::u32, ge::u32, ge::i32) {}
void gxGenerateMipmap(ge::u32) {}
void gxDeleteTexture(ge::u32) {}

ge::u32 gxCreateFramebuffer() { return nextName++; }
void gxBindFramebuffer(ge::u32, ge::u32) {}
void gxFramebufferTexture2D(ge::u32, ge::u32, ge::u32, ge::u32, ge::i32) {}
ge::i32 gxFramebufferComplete(ge::u32) { return 1; }
void gxDrawBuffers(ge::i32) {}
void gxDeleteFramebuffer(ge::u32) {}

void gxDrawArrays(ge::u32, ge::i32, ge::i32) {}
void gxDrawArraysInstanced(ge::u32, ge::i32, ge::i32, ge::i32) {}

ge::i32 gxSupportsFloatTargets() { return 1; }
ge::u32 gxGetError() { return 0; }

}  // extern "C"
