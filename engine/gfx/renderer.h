// G-Engine — gfx/renderer.h
// The 2D renderer: normal-mapped sprites, dynamic lights with soft shadows, HDR, bloom.
//
// A frame is drawn in these steps (see docs/RENDERING.md for the pictures):
//   1. G-buffer   all sprites -> colour + (normal, emissive)                    [1 draw call]
//   2. shadows    for every light: distance to the nearest occluder in each     [1 draw call]
//                 direction, stored as one row of a small depth texture
//   3. lights     every light adds its contribution, using normals + shadows    [1 draw call]
//   4. composite  colour x (ambient + light) + emissive + light haze  (HDR)
//   5. bloom      bright parts glow
//   6. output     tone mapping, vignette, dithering
#pragma once

#include <vector>

#include "core/math.h"

namespace ge {

struct SpriteDraw {
    Vec2 pos;            // centre, world units
    Vec2 size{1, 1};     // world units
    f32 rotation = 0;    // radians
    f32 u0 = 0, v0 = 0, u1 = 1, v1 = 1;  // atlas region (v0 = top)
    Color tint{1, 1, 1, 1};
    f32 emissive = 0;    // >= 0: scales the sprite's emissive mask; < 0: the whole sprite glows with -emissive
    bool flipX = false;
    i32 layer = 0;       // drawn in ascending order
};

struct LightDraw {
    Vec2 pos;
    f32 height = 1.0f;   // distance in front of the picture plane: low = grazing light, strong relief
    Color color{1, 1, 1, 1};  // linear RGB, already multiplied by the intensity
    f32 radius = 8.0f;
    f32 size = 0.25f;    // radius of the light source itself: bigger = softer shadows
    bool shadows = true;
    i32 owner = -1;      // object carrying the light: its own outline does not block it
};

struct Segment {
    Vec2 a, b;
    i32 owner = -1;
};

struct RenderSettings {
    bool lighting = true;
    bool bloom = true;
    f32 lightScale = 1.0f;     // resolution of the light buffer relative to the screen (0.5 = faster)
    f32 exposure = 1.0f;
    f32 haze = 0.03f;          // how much light is scattered by the air (visible glow around lamps)
    f32 bloomStrength = 0.55f;
    f32 penetration = 0.45f;   // how deep (m) light reaches into solid things before it is fully blocked
};

struct RenderFrame {
    Vec2 camera;
    f32 viewHeight = 12.0f;    // world units visible vertically
    Color ambient{0.035f, 0.04f, 0.07f, 1.0f};
    Color background{0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<SpriteDraw> sprites;
    std::vector<LightDraw> lights;
    std::vector<Segment> occluders;

    void clear() {
        sprites.clear();
        lights.clear();
        occluders.clear();
    }
};

struct RenderStats {
    i32 sprites = 0;
    i32 lights = 0;
    i32 shadowLights = 0;
    i32 shadowEdges = 0;
    i32 drawCalls = 0;
};

class Renderer {
public:
    static constexpr i32 kShadowWidth = 1024;  // directions per light
    static constexpr i32 kShadowRows = 32;     // lights that can cast shadows at the same time

    bool init();
    void resize(i32 width, i32 height);
    /// One texture atlas: colour (sRGB, straight alpha) and a matching normal map
    /// (rg = normal xy, b = emissive mask). Both w*h*4 bytes.
    void setAtlas(const u8* albedo, const u8* normal, i32 width, i32 height);
    void render(RenderFrame& frame, const RenderSettings& settings);

    const RenderStats& stats() const { return stats_; }
    i32 width() const { return width_; }
    i32 height() const { return height_; }
    bool floatTargets() const { return floatTargets_; }

private:
    struct Target {
        u32 fbo = 0;
        u32 texture = 0;
        i32 width = 0, height = 0;
    };

    u32 makeProgram(const char* name, const char* vertex, const char* fragment);
    i32 uniform(u32 program, const char* name);
    void makeTarget(Target& target, i32 width, i32 height, bool hdr);
    void destroyTarget(Target& target);
    void createTargets();
    void fullscreen();

    bool ready_ = false;
    bool floatTargets_ = false;
    i32 width_ = 0, height_ = 0;
    f32 lightScale_ = 1.0f;
    RenderStats stats_;

    // shaders
    u32 spriteProgram_ = 0, shadowProgram_ = 0, lightProgram_ = 0, compositeProgram_ = 0;
    u32 bloomDownProgram_ = 0, bloomUpProgram_ = 0, outputProgram_ = 0;
    struct {
        i32 spriteView, shadowInfo, lightView, lightInvRes, lightShadowInfo, compositeAmbient, compositeParams;
        i32 downTexel, downThreshold, upTexel, outputParams, outputTexel;
    } u_{};

    // geometry
    u32 cornerBuffer_ = 0;
    u32 spriteVao_ = 0, spriteBuffer_ = 0;
    u32 shadowVao_ = 0, shadowBuffer_ = 0;
    u32 lightVao_ = 0, lightBuffer_ = 0;
    u32 emptyVao_ = 0;
    std::vector<f32> scratch_;

    // textures and render targets
    u32 atlasAlbedo_ = 0, atlasNormal_ = 0;
    u32 gbufferFbo_ = 0, gbufferAlbedo_ = 0, gbufferNormal_ = 0;
    u32 shadowFbo_ = 0, shadowTexture_ = 0;
    Target light_, hdr_;
    static constexpr int kBloomLevels = 5;
    Target bloom_[kBloomLevels];
};

}  // namespace ge
