// G-Engine — gfx/renderer.cpp
#include "gfx/renderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "core/log.h"
#include "gfx/gl.h"
#include "gfx/shaders.h"

namespace ge {

namespace {

constexpr f32 kEmissiveGain = 9.0f;
constexpr f32 kBloomThreshold = 0.85f;

void setFilter(i32 filter, i32 wrap) {
    gxTexParameteri(gx::kTexture2D, gx::kTextureMinFilter, filter);
    gxTexParameteri(gx::kTexture2D, gx::kTextureMagFilter, filter);
    gxTexParameteri(gx::kTexture2D, gx::kTextureWrapS, wrap);
    gxTexParameteri(gx::kTexture2D, gx::kTextureWrapT, wrap);
}

/// One instanced attribute made of four floats.
void instanceAttribute(u32 index, i32 stride, i32 offset) {
    gxEnableVertexAttribArray(index);
    gxVertexAttribPointer(index, 4, stride, offset);
    gxVertexAttribDivisor(index, 1);
}

f32 distanceToSegment(Vec2 p, Vec2 a, Vec2 b) {
    const Vec2 e = b - a;
    const f32 len2 = dot(e, e);
    const f32 t = len2 > 1e-12f ? clamp(dot(p - a, e) / len2, 0.0f, 1.0f) : 0.0f;
    return length(p - (a + e * t));
}

}  // namespace

u32 Renderer::makeProgram(const char* name, const char* vertex, const char* fragment) {
    char log[1024];
    const u32 vs = gxCreateShader(gx::kVertexShader);
    gxShaderSource(vs, vertex, static_cast<i32>(std::strlen(vertex)));
    log[0] = 0;
    if (!gxCompileShader(vs, log, sizeof(log))) GE_LOG_ERROR("shader '%s' (vertex): %s", name, log);
    const u32 fs = gxCreateShader(gx::kFragmentShader);
    gxShaderSource(fs, fragment, static_cast<i32>(std::strlen(fragment)));
    log[0] = 0;
    if (!gxCompileShader(fs, log, sizeof(log))) GE_LOG_ERROR("shader '%s' (fragment): %s", name, log);
    const u32 program = gxCreateProgram();
    gxAttachShader(program, vs);
    gxAttachShader(program, fs);
    log[0] = 0;
    if (!gxLinkProgram(program, log, sizeof(log))) {
        GE_LOG_ERROR("shader '%s' (link): %s", name, log);
        ready_ = false;
    }
    gxDeleteShader(vs);
    gxDeleteShader(fs);
    return program;
}

i32 Renderer::uniform(u32 program, const char* name) {
    return gxGetUniformLocation(program, name, static_cast<i32>(std::strlen(name)));
}

bool Renderer::init() {
    ready_ = true;
    floatTargets_ = gxSupportsFloatTargets() != 0;

    spriteProgram_ = makeProgram("sprite", shaders::kSpriteVertex, shaders::kSpriteFragment);
    shadowProgram_ = makeProgram("shadow", shaders::kShadowVertex, shaders::kShadowFragment);
    lightProgram_ = makeProgram("light", shaders::kLightVertex, shaders::kLightFragment);
    compositeProgram_ = makeProgram("composite", shaders::kFullscreenVertex, shaders::kCompositeFragment);
    bloomDownProgram_ = makeProgram("bloom-down", shaders::kFullscreenVertex, shaders::kBloomDownFragment);
    bloomUpProgram_ = makeProgram("bloom-up", shaders::kFullscreenVertex, shaders::kBloomUpFragment);
    outputProgram_ = makeProgram("output", shaders::kFullscreenVertex, shaders::kOutputFragment);

    // Texture units are fixed per program.
    gxUseProgram(spriteProgram_);
    gxUniform1i(uniform(spriteProgram_, "uAlbedo"), 0);
    gxUniform1i(uniform(spriteProgram_, "uNormal"), 1);
    u_.spriteView = uniform(spriteProgram_, "uView");

    gxUseProgram(shadowProgram_);
    u_.shadowInfo = uniform(shadowProgram_, "uShadowInfo");

    gxUseProgram(lightProgram_);
    gxUniform1i(uniform(lightProgram_, "uNormalTex"), 0);
    gxUniform1i(uniform(lightProgram_, "uShadowTex"), 1);
    u_.lightView = uniform(lightProgram_, "uView");
    u_.lightInvRes = uniform(lightProgram_, "uInvRes");
    u_.lightShadowInfo = uniform(lightProgram_, "uShadowInfo");

    gxUseProgram(compositeProgram_);
    gxUniform1i(uniform(compositeProgram_, "uAlbedoTex"), 0);
    gxUniform1i(uniform(compositeProgram_, "uNormalTex"), 1);
    gxUniform1i(uniform(compositeProgram_, "uLightTex"), 2);
    u_.compositeAmbient = uniform(compositeProgram_, "uAmbient");
    u_.compositeParams = uniform(compositeProgram_, "uParams");

    gxUseProgram(bloomDownProgram_);
    gxUniform1i(uniform(bloomDownProgram_, "uTex"), 0);
    u_.downTexel = uniform(bloomDownProgram_, "uTexel");
    u_.downThreshold = uniform(bloomDownProgram_, "uThreshold");

    gxUseProgram(bloomUpProgram_);
    gxUniform1i(uniform(bloomUpProgram_, "uTex"), 0);
    u_.upTexel = uniform(bloomUpProgram_, "uTexel");

    gxUseProgram(outputProgram_);
    gxUniform1i(uniform(outputProgram_, "uSceneTex"), 0);
    gxUniform1i(uniform(outputProgram_, "uBloomTex"), 1);
    u_.outputParams = uniform(outputProgram_, "uParams");

    // A unit quad shared by everything that is instanced.
    const f32 corners[] = {0, 0, 1, 0, 0, 1, 1, 1};
    cornerBuffer_ = gxCreateBuffer();
    gxBindBuffer(gx::kArrayBuffer, cornerBuffer_);
    gxBufferData(gx::kArrayBuffer, corners, sizeof(corners), gx::kStaticDraw);

    auto makeVao = [this](u32& vao, u32& buffer, int vec4Count) {
        vao = gxCreateVertexArray();
        gxBindVertexArray(vao);
        gxBindBuffer(gx::kArrayBuffer, cornerBuffer_);
        gxEnableVertexAttribArray(0);
        gxVertexAttribPointer(0, 2, 8, 0);
        buffer = gxCreateBuffer();
        gxBindBuffer(gx::kArrayBuffer, buffer);
        for (int i = 0; i < vec4Count; ++i) instanceAttribute(static_cast<u32>(i + 1), vec4Count * 16, i * 16);
    };
    makeVao(spriteVao_, spriteBuffer_, 4);
    makeVao(shadowVao_, shadowBuffer_, 3);
    makeVao(lightVao_, lightBuffer_, 3);
    emptyVao_ = gxCreateVertexArray();
    gxBindVertexArray(0);

    // Shadow map: a depth texture, one row per light.
    shadowTexture_ = gxCreateTexture();
    gxBindTexture(gx::kTexture2D, shadowTexture_);
    gxTexImage2D(gx::kTexture2D, 0, gx::kDepthComponent24, kShadowWidth, kShadowRows, gx::kDepthComponent, gx::kUnsignedInt, nullptr, 0);
    setFilter(gx::kNearest, gx::kClampToEdge);
    shadowFbo_ = gxCreateFramebuffer();
    gxBindFramebuffer(gx::kFramebuffer, shadowFbo_);
    gxFramebufferTexture2D(gx::kFramebuffer, gx::kDepthAttachment, gx::kTexture2D, shadowTexture_, 0);
    gxDrawBuffers(0);
    if (!gxFramebufferComplete(gx::kFramebuffer)) {
        GE_LOG_ERROR("renderer: shadow framebuffer is incomplete");
        ready_ = false;
    }
    gxBindFramebuffer(gx::kFramebuffer, 0);

    GE_LOG_INFO("renderer: ready (%s colour targets)", floatTargets_ ? "16-bit float HDR" : "8-bit");
    return ready_;
}

void Renderer::makeTarget(Target& target, i32 width, i32 height, bool hdr) {
    destroyTarget(target);
    target.width = std::max(1, width);
    target.height = std::max(1, height);
    target.texture = gxCreateTexture();
    gxBindTexture(gx::kTexture2D, target.texture);
    if (hdr && floatTargets_) {
        gxTexImage2D(gx::kTexture2D, 0, gx::kRgba16F, target.width, target.height, gx::kRgba, gx::kHalfFloat, nullptr, 0);
    } else {
        gxTexImage2D(gx::kTexture2D, 0, gx::kRgba8, target.width, target.height, gx::kRgba, gx::kUnsignedByte, nullptr, 0);
    }
    setFilter(gx::kLinear, gx::kClampToEdge);
    target.fbo = gxCreateFramebuffer();
    gxBindFramebuffer(gx::kFramebuffer, target.fbo);
    gxFramebufferTexture2D(gx::kFramebuffer, gx::kColorAttachment0, gx::kTexture2D, target.texture, 0);
    gxDrawBuffers(1);
    if (!gxFramebufferComplete(gx::kFramebuffer)) GE_LOG_ERROR("renderer: a %dx%d render target is incomplete", target.width, target.height);
}

void Renderer::destroyTarget(Target& target) {
    if (target.fbo != 0) gxDeleteFramebuffer(target.fbo);
    if (target.texture != 0) gxDeleteTexture(target.texture);
    target = Target{};
}

void Renderer::createTargets() {
    // G-buffer: colour (sRGB) + normal/emissive.
    if (gbufferFbo_ != 0) gxDeleteFramebuffer(gbufferFbo_);
    if (gbufferAlbedo_ != 0) gxDeleteTexture(gbufferAlbedo_);
    if (gbufferNormal_ != 0) gxDeleteTexture(gbufferNormal_);
    gbufferAlbedo_ = gxCreateTexture();
    gxBindTexture(gx::kTexture2D, gbufferAlbedo_);
    gxTexImage2D(gx::kTexture2D, 0, gx::kSrgb8Alpha8, width_, height_, gx::kRgba, gx::kUnsignedByte, nullptr, 0);
    setFilter(gx::kLinear, gx::kClampToEdge);
    gbufferNormal_ = gxCreateTexture();
    gxBindTexture(gx::kTexture2D, gbufferNormal_);
    gxTexImage2D(gx::kTexture2D, 0, gx::kRgba8, width_, height_, gx::kRgba, gx::kUnsignedByte, nullptr, 0);
    setFilter(gx::kLinear, gx::kClampToEdge);
    gbufferFbo_ = gxCreateFramebuffer();
    gxBindFramebuffer(gx::kFramebuffer, gbufferFbo_);
    gxFramebufferTexture2D(gx::kFramebuffer, gx::kColorAttachment0, gx::kTexture2D, gbufferAlbedo_, 0);
    gxFramebufferTexture2D(gx::kFramebuffer, gx::kColorAttachment0 + 1, gx::kTexture2D, gbufferNormal_, 0);
    gxDrawBuffers(2);
    if (!gxFramebufferComplete(gx::kFramebuffer)) {
        GE_LOG_ERROR("renderer: G-buffer is incomplete");
        ready_ = false;
    }

    makeTarget(light_, static_cast<i32>(std::lround(width_ * lightScale_)), static_cast<i32>(std::lround(height_ * lightScale_)), true);
    makeTarget(hdr_, width_, height_, true);
    i32 w = width_, h = height_;
    for (Target& level : bloom_) {
        w = std::max(1, w / 2);
        h = std::max(1, h / 2);
        makeTarget(level, w, h, true);
    }
    gxBindFramebuffer(gx::kFramebuffer, 0);
}

void Renderer::resize(i32 width, i32 height) {
    width = std::max(1, width);
    height = std::max(1, height);
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    createTargets();
}

void Renderer::setAtlas(const u8* albedo, const u8* normal, i32 width, i32 height) {
    if (atlasAlbedo_ == 0) atlasAlbedo_ = gxCreateTexture();
    gxBindTexture(gx::kTexture2D, atlasAlbedo_);
    gxTexImage2D(gx::kTexture2D, 0, gx::kSrgb8Alpha8, width, height, gx::kRgba, gx::kUnsignedByte, albedo, width * height * 4);
    setFilter(gx::kLinear, gx::kClampToEdge);
    if (atlasNormal_ == 0) atlasNormal_ = gxCreateTexture();
    gxBindTexture(gx::kTexture2D, atlasNormal_);
    gxTexImage2D(gx::kTexture2D, 0, gx::kRgba8, width, height, gx::kRgba, gx::kUnsignedByte, normal, width * height * 4);
    setFilter(gx::kLinear, gx::kClampToEdge);
}

void Renderer::fullscreen() {
    gxBindVertexArray(emptyVao_);
    gxDrawArrays(gx::kTriangles, 0, 3);
    ++stats_.drawCalls;
}

void Renderer::render(RenderFrame& frame, const RenderSettings& settings) {
    stats_ = RenderStats{};
    if (!ready_ || width_ <= 0 || height_ <= 0) return;

    const f32 scale = clamp(settings.lightScale, 0.25f, 1.0f);
    if (std::fabs(scale - lightScale_) > 1e-3f) {
        lightScale_ = scale;
        makeTarget(light_, static_cast<i32>(std::lround(width_ * lightScale_)), static_cast<i32>(std::lround(height_ * lightScale_)), true);
    }

    const f32 viewH = std::max(frame.viewHeight, 0.01f);
    const f32 viewW = viewH * static_cast<f32>(width_) / static_cast<f32>(height_);
    const Vec2 cam = frame.camera;

    gxDisable(gx::kDepthTest);
    gxDisable(gx::kScissorTest);
    gxDisable(gx::kCullFace);
    gxDepthMask(0);
    gxColorMask(1, 1, 1, 1);
    gxBlendEquation(gx::kFuncAdd);

    // ---- 1. G-buffer ---------------------------------------------------------------------
    std::stable_sort(frame.sprites.begin(), frame.sprites.end(), [](const SpriteDraw& a, const SpriteDraw& b) { return a.layer < b.layer; });
    scratch_.clear();
    const f32 marginX = viewW * 0.5f, marginY = viewH * 0.5f;
    for (const SpriteDraw& s : frame.sprites) {
        const f32 reach = (std::fabs(s.size.x) + std::fabs(s.size.y)) * 0.75f;
        if (std::fabs(s.pos.x - cam.x) > marginX + reach || std::fabs(s.pos.y - cam.y) > marginY + reach) continue;
        const f32 c = std::cos(s.rotation), sn = std::sin(s.rotation);
        const f32 data[16] = {s.pos.x, s.pos.y, s.size.x, s.size.y, c,        sn,       s.emissive, s.flipX ? -1.0f : 1.0f,
                              s.u0,    s.v0,    s.u1,     s.v1,     s.tint.r, s.tint.g, s.tint.b,   s.tint.a};
        scratch_.insert(scratch_.end(), data, data + 16);
    }
    stats_.sprites = static_cast<i32>(scratch_.size() / 16);

    gxBindFramebuffer(gx::kFramebuffer, gbufferFbo_);
    gxViewport(0, 0, width_, height_);
    gxClearColorBuffer(0, frame.background.r, frame.background.g, frame.background.b, 1.0f);
    gxClearColorBuffer(1, 0.5f, 0.5f, 0.0f, 1.0f);  // flat normal, no emissive
    if (stats_.sprites > 0 && atlasAlbedo_ != 0) {
        gxEnable(gx::kBlend);
        gxBlendFunc(gx::kOne, gx::kOneMinusSrcAlpha);
        gxUseProgram(spriteProgram_);
        gxUniform4f(u_.spriteView, cam.x, cam.y, 2.0f / viewW, 2.0f / viewH);
        gxActiveTexture(gx::kTexture0);
        gxBindTexture(gx::kTexture2D, atlasAlbedo_);
        gxActiveTexture(gx::kTexture0 + 1);
        gxBindTexture(gx::kTexture2D, atlasNormal_);
        gxBindVertexArray(spriteVao_);
        gxBindBuffer(gx::kArrayBuffer, spriteBuffer_);
        gxBufferData(gx::kArrayBuffer, scratch_.data(), static_cast<i32>(scratch_.size() * sizeof(f32)), gx::kStreamDraw);
        gxDrawArraysInstanced(gx::kTriangleStrip, 0, 4, stats_.sprites);
        ++stats_.drawCalls;
    }

    // ---- 2. shadow map -------------------------------------------------------------------
    // Lights on screen only; the ones nearest to the camera get the shadow rows.
    struct Visible {
        const LightDraw* light;
        i32 row;
    };
    std::vector<Visible> visible;
    if (settings.lighting) {
        for (const LightDraw& l : frame.lights) {
            if (l.radius <= 0.01f) continue;
            if (std::fabs(l.pos.x - cam.x) > marginX + l.radius || std::fabs(l.pos.y - cam.y) > marginY + l.radius) continue;
            visible.push_back({&l, -1});
        }
        std::stable_sort(visible.begin(), visible.end(), [cam](const Visible& a, const Visible& b) {
            return lengthSq(a.light->pos - cam) < lengthSq(b.light->pos - cam);
        });
    }
    stats_.lights = static_cast<i32>(visible.size());

    scratch_.clear();
    i32 rows = 0;
    for (Visible& v : visible) {
        if (!v.light->shadows || rows >= kShadowRows) continue;
        v.row = rows++;
        const LightDraw& l = *v.light;
        for (const Segment& s : frame.occluders) {
            if (s.owner >= 0 && s.owner == l.owner) continue;
            if (distanceToSegment(l.pos, s.a, s.b) > l.radius) continue;
            for (int copy = 0; copy < 2; ++copy) {
                const f32 data[12] = {s.a.x, s.a.y, s.b.x, s.b.y, l.pos.x, l.pos.y, l.radius, static_cast<f32>(v.row), static_cast<f32>(copy), 0, 0, 0};
                scratch_.insert(scratch_.end(), data, data + 12);
            }
        }
    }
    stats_.shadowLights = rows;
    stats_.shadowEdges = static_cast<i32>(scratch_.size() / 24);

    if (rows > 0) {
        gxBindFramebuffer(gx::kFramebuffer, shadowFbo_);
        gxViewport(0, 0, kShadowWidth, kShadowRows);
        gxDisable(gx::kBlend);
        gxEnable(gx::kDepthTest);
        gxDepthFunc(gx::kLess);
        gxDepthMask(1);
        gxClearDepth(1.0f);
        gxClear(gx::kDepthBufferBit);
        if (!scratch_.empty()) {
            gxUseProgram(shadowProgram_);
            gxUniform4f(u_.shadowInfo, 1.0f / kShadowWidth, 1.0f / kShadowRows, 0, 0);
            gxBindVertexArray(shadowVao_);
            gxBindBuffer(gx::kArrayBuffer, shadowBuffer_);
            gxBufferData(gx::kArrayBuffer, scratch_.data(), static_cast<i32>(scratch_.size() * sizeof(f32)), gx::kStreamDraw);
            gxDrawArraysInstanced(gx::kTriangleStrip, 0, 4, static_cast<i32>(scratch_.size() / 12));
            ++stats_.drawCalls;
        }
        gxDepthMask(0);
        gxDisable(gx::kDepthTest);
    }

    // ---- 3. lights -----------------------------------------------------------------------
    gxBindFramebuffer(gx::kFramebuffer, light_.fbo);
    gxViewport(0, 0, light_.width, light_.height);
    gxClearColor(0, 0, 0, 1);
    gxClear(gx::kColorBufferBit);
    if (!visible.empty()) {
        scratch_.clear();
        for (const Visible& v : visible) {
            const LightDraw& l = *v.light;
            const f32 data[12] = {l.pos.x, l.pos.y, std::max(l.height, 0.01f), l.radius, l.color.r, l.color.g, l.color.b, static_cast<f32>(v.row),
                                  std::max(l.size, 0.01f), 0, 0, 0};
            scratch_.insert(scratch_.end(), data, data + 12);
        }
        gxEnable(gx::kBlend);
        gxBlendFunc(gx::kOne, gx::kOne);
        gxUseProgram(lightProgram_);
        gxUniform4f(u_.lightView, cam.x, cam.y, 2.0f / viewW, 2.0f / viewH);
        gxUniform2f(u_.lightInvRes, 1.0f / static_cast<f32>(light_.width), 1.0f / static_cast<f32>(light_.height));
        gxUniform2f(u_.lightShadowInfo, static_cast<f32>(kShadowWidth), std::max(settings.penetration, 0.01f));
        gxActiveTexture(gx::kTexture0);
        gxBindTexture(gx::kTexture2D, gbufferNormal_);
        gxActiveTexture(gx::kTexture0 + 1);
        gxBindTexture(gx::kTexture2D, shadowTexture_);
        gxBindVertexArray(lightVao_);
        gxBindBuffer(gx::kArrayBuffer, lightBuffer_);
        gxBufferData(gx::kArrayBuffer, scratch_.data(), static_cast<i32>(scratch_.size() * sizeof(f32)), gx::kStreamDraw);
        gxDrawArraysInstanced(gx::kTriangleStrip, 0, 4, static_cast<i32>(visible.size()));
        ++stats_.drawCalls;
    }
    gxDisable(gx::kBlend);

    // ---- 4. composite --------------------------------------------------------------------
    gxBindFramebuffer(gx::kFramebuffer, hdr_.fbo);
    gxViewport(0, 0, hdr_.width, hdr_.height);
    gxUseProgram(compositeProgram_);
    gxUniform4f(u_.compositeAmbient, frame.ambient.r, frame.ambient.g, frame.ambient.b, settings.lighting ? 1.0f : 0.0f);
    gxUniform4f(u_.compositeParams, kEmissiveGain, settings.haze, 0, 0);
    gxActiveTexture(gx::kTexture0);
    gxBindTexture(gx::kTexture2D, gbufferAlbedo_);
    gxActiveTexture(gx::kTexture0 + 1);
    gxBindTexture(gx::kTexture2D, gbufferNormal_);
    gxActiveTexture(gx::kTexture0 + 2);
    gxBindTexture(gx::kTexture2D, light_.texture);
    fullscreen();

    // ---- 5. bloom ------------------------------------------------------------------------
    const bool bloom = settings.bloom && settings.lighting;
    if (bloom) {
        gxUseProgram(bloomDownProgram_);
        gxActiveTexture(gx::kTexture0);
        const Target* source = &hdr_;
        for (int i = 0; i < kBloomLevels; ++i) {
            gxBindFramebuffer(gx::kFramebuffer, bloom_[i].fbo);
            gxViewport(0, 0, bloom_[i].width, bloom_[i].height);
            gxBindTexture(gx::kTexture2D, source->texture);
            gxUniform2f(u_.downTexel, 1.0f / static_cast<f32>(source->width), 1.0f / static_cast<f32>(source->height));
            gxUniform1f(u_.downThreshold, i == 0 ? kBloomThreshold : 0.0f);
            fullscreen();
            source = &bloom_[i];
        }
        gxUseProgram(bloomUpProgram_);
        gxEnable(gx::kBlend);
        gxBlendFunc(gx::kOne, gx::kOne);
        for (int i = kBloomLevels - 1; i > 0; --i) {
            gxBindFramebuffer(gx::kFramebuffer, bloom_[i - 1].fbo);
            gxViewport(0, 0, bloom_[i - 1].width, bloom_[i - 1].height);
            gxBindTexture(gx::kTexture2D, bloom_[i].texture);
            gxUniform2f(u_.upTexel, 1.0f / static_cast<f32>(bloom_[i].width), 1.0f / static_cast<f32>(bloom_[i].height));
            fullscreen();
        }
        gxDisable(gx::kBlend);
    }

    // ---- 6. output -----------------------------------------------------------------------
    gxBindFramebuffer(gx::kFramebuffer, 0);
    gxViewport(0, 0, width_, height_);
    gxUseProgram(outputProgram_);
    gxUniform4f(u_.outputParams, settings.lighting ? settings.exposure : 1.0f, bloom ? settings.bloomStrength : 0.0f, settings.lighting ? 0.55f : 0.0f,
                settings.lighting ? 1.0f : 0.0f);
    gxActiveTexture(gx::kTexture0);
    gxBindTexture(gx::kTexture2D, hdr_.texture);
    gxActiveTexture(gx::kTexture0 + 1);
    gxBindTexture(gx::kTexture2D, bloom_[0].texture);
    fullscreen();

    gxActiveTexture(gx::kTexture0);
    gxBindVertexArray(0);
}

}  // namespace ge
