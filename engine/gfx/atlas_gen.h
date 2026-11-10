// G-Engine — gfx/atlas_gen.h
// The built-in art: every sprite of the sample game is painted by code at start-up, together
// with a height field from which the normal map is derived. (Importing your own pictures is
// roadmap phase 4; until then this keeps the engine free of binary assets.)
#pragma once

#include <vector>

#include "core/base.h"

namespace ge {

enum class Sprite : u8 { BrickA, BrickB, BrickTop, BackA, BackB, Crate, Player, Guard, Coin, Spikes, Door, Lamp, White, Count };

struct SpriteRegion {
    f32 u0 = 0, v0 = 0, u1 = 1, v1 = 1;
};

struct AtlasData {
    i32 width = 0, height = 0;
    std::vector<u8> albedo;  // sRGB, straight alpha
    std::vector<u8> normal;  // rg = normal xy, b = emissive mask, a = 255
};

void generateAtlas(AtlasData& out);
SpriteRegion spriteRegion(Sprite sprite);

}  // namespace ge
