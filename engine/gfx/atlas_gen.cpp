// G-Engine — gfx/atlas_gen.cpp
#include "gfx/atlas_gen.h"

#include <cmath>

#include "core/math.h"
#include "core/random.h"

namespace ge {

namespace {

constexpr int kCell = 64;
constexpr int kAtlas = 512;

struct Cell {
    int x, y, w, h;  // in cells
    bool tiles;      // wraps around: neighbours of the same tile continue the surface
    f32 relief;      // strength of the normal map
};

constexpr Cell kCells[static_cast<int>(Sprite::Count)] = {
    {0, 0, 1, 1, true, 2.6f},   // BrickA
    {1, 0, 1, 1, true, 2.6f},   // BrickB
    {2, 0, 1, 1, true, 2.6f},   // BrickTop
    {3, 0, 1, 1, true, 1.8f},   // BackA
    {4, 0, 1, 1, true, 1.8f},   // BackB
    {5, 0, 1, 1, false, 2.4f},  // Crate
    {0, 1, 1, 1, false, 3.0f},  // Player
    {1, 1, 1, 1, false, 3.0f},  // Guard
    {2, 1, 1, 1, false, 3.0f},  // Coin
    {3, 1, 1, 1, false, 3.0f},  // Spikes
    {6, 0, 1, 2, false, 2.4f},  // Door
    {4, 1, 1, 1, false, 2.5f},  // Lamp
    {5, 1, 1, 1, false, 0.0f},  // White
};

struct Texel {
    f32 r = 0, g = 0, b = 0;  // sRGB
    f32 a = 0;
    f32 h = 0;                // height, 0..1
    f32 e = 0;                // emissive mask
};

Texel solid(f32 r, f32 g, f32 b, f32 h, f32 e = 0) { return {r, g, b, 1.0f, h, e}; }

/// Smooth noise that repeats every `period` cells.
f32 noise(f32 x, f32 y, int period, u32 seed) {
    const f32 fx = std::floor(x), fy = std::floor(y);
    const f32 tx = x - fx, ty = y - fy;
    const f32 sx = tx * tx * (3 - 2 * tx), sy = ty * ty * (3 - 2 * ty);
    auto at = [&](int ix, int iy) {
        const int px = ((ix % period) + period) % period, py = ((iy % period) + period) % period;
        return hash01(static_cast<u32>(px), static_cast<u32>(py), seed);
    };
    const int ix = static_cast<int>(fx), iy = static_cast<int>(fy);
    return lerp(lerp(at(ix, iy), at(ix + 1, iy), sx), lerp(at(ix, iy + 1), at(ix + 1, iy + 1), sx), sy);
}

/// Two octaves, repeating over a 64 px tile.
f32 grain(f32 x, f32 y, u32 seed) { return noise(x / 8, y / 8, 8, seed) * 0.65f + noise(x / 2.5f, y / 2.5f, 26, seed + 7) * 0.35f; }

/// Signed distance to a rounded box centred on (cx, cy).
f32 sdBox(f32 x, f32 y, f32 cx, f32 cy, f32 hx, f32 hy, f32 radius) {
    const f32 dx = std::fabs(x - cx) - hx + radius, dy = std::fabs(y - cy) - hy + radius;
    const f32 ox = max(dx, 0.0f), oy = max(dy, 0.0f);
    return std::sqrt(ox * ox + oy * oy) + min(max(dx, dy), 0.0f) - radius;
}

f32 sdCircle(f32 x, f32 y, f32 cx, f32 cy, f32 r) { return std::sqrt(square(x - cx) + square(y - cy)) - r; }

/// Soft coverage of a shape from its signed distance (anti-aliased edge).
f32 cover(f32 d) { return saturate(0.5f - d); }

/// A pillow-shaped height: 0 at the edge, 1 at `depth` pixels inside.
f32 dome(f32 d, f32 depth) {
    const f32 t = saturate(-d / depth);
    return std::sqrt(1.0f - square(1.0f - t));
}

void over(Texel& base, const Texel& top, f32 coverage) {
    const f32 a = coverage * top.a;
    if (a <= 0) return;
    const f32 outA = a + base.a * (1 - a);
    const f32 k = a / max(outA, 1e-5f);
    base.r = lerp(base.r, top.r, k);
    base.g = lerp(base.g, top.g, k);
    base.b = lerp(base.b, top.b, k);
    base.h = lerp(base.h, top.h, k);
    base.e = lerp(base.e, top.e, k);
    base.a = outA;
}

// ---- tiles ---------------------------------------------------------------------------------

Texel bricks(f32 x, f32 y, u32 seed, f32 brickW, f32 brickH, f32 br, f32 bg, f32 bb, f32 contrast) {
    const int row = static_cast<int>(y / brickH);
    const f32 shifted = x + (row % 2 == 1 ? brickW * 0.5f : 0.0f);
    const int col = static_cast<int>(shifted / brickW) % static_cast<int>(64 / brickW);
    const f32 lx = std::fmod(shifted, brickW), ly = std::fmod(y, brickH);
    const f32 edge = min(min(lx, brickW - lx), min(ly, brickH - ly));  // distance to the mortar
    const f32 tone = 0.8f + 0.4f * hash01(static_cast<u32>(col), static_cast<u32>(row), seed);
    const f32 g = grain(x, y, seed);
    Texel t;
    t.a = 1;
    if (edge < 1.5f) {  // mortar
        const f32 m = 0.32f + 0.1f * g;
        t.r = br * m;
        t.g = bg * m;
        t.b = bb * m;
        t.h = 0.05f * g;
    } else {
        const f32 shade = tone * (1.0f - contrast * 0.5f + contrast * g);
        t.r = br * shade;
        t.g = bg * shade;
        t.b = bb * shade;
        t.h = 0.45f + 0.4f * smoothstep(1.5f, 5.0f, edge) + 0.15f * g + 0.1f * (tone - 1.0f);
    }
    return t;
}

Texel paintBrick(f32 x, f32 y, u32 seed) {
    Texel t = bricks(x, y, seed, 32, 16, 0.52f, 0.53f, 0.60f, 0.45f);
    if (seed == 2) {  // a crack
        const f32 crack = std::fabs(x - (30 + 6 * std::sin(y * 0.21f) + 3 * std::sin(y * 0.7f)));
        if (crack < 1.2f && y > 10 && y < 56) {
            t.h *= 0.3f;
            t.r *= 0.55f;
            t.g *= 0.55f;
            t.b *= 0.55f;
        }
    }
    return t;
}

Texel paintBrickTop(f32 x, f32 y) {
    Texel t = bricks(x, y, 3, 32, 16, 0.52f, 0.53f, 0.60f, 0.45f);
    if (y < 11) {  // a worn cap stone with moss
        const f32 g = grain(x, y, 11);
        const f32 moss = smoothstep(0.45f, 0.7f, noise(x / 9, 0.5f, 7, 5) + (4.0f - y) * 0.06f);
        const f32 shade = 0.85f + 0.3f * g;
        t.r = lerp(0.66f, 0.30f, moss) * shade;
        t.g = lerp(0.66f, 0.52f, moss) * shade;
        t.b = lerp(0.70f, 0.26f, moss) * shade;
        t.h = (y < 9 ? 0.75f + 0.25f * smoothstep(0.0f, 3.0f, min(y, 9.0f - y)) : 0.2f) + 0.12f * g;
    }
    return t;
}

Texel paintBack(f32 x, f32 y, u32 seed) {
    Texel t = bricks(x, y, seed, 64, 32, 0.20f, 0.21f, 0.27f, 0.5f);
    t.h *= 0.8f;
    return t;
}

// ---- objects -------------------------------------------------------------------------------

Texel paintCrate(f32 x, f32 y) {
    Texel t;
    const f32 d = sdBox(x, y, 32, 32, 31, 31, 3);
    if (d > 0.5f) return t;
    const f32 g = grain(x * 0.6f, y * 2.2f, 21);  // wood grain runs along the planks
    const f32 border = min(min(x, 63 - x), min(y, 63 - y));
    const bool frame = border < 8;
    const f32 brace = std::fabs((x - y)) * 0.7071f;  // diagonal
    const bool onBrace = brace < 4.5f && !frame;
    const f32 plank = std::fmod(x - 8, 12.0f);
    const bool groove = !frame && !onBrace && (plank < 1.2f);
    f32 shade = 0.8f + 0.4f * g, h;
    f32 r = 0.60f, gg = 0.40f, b = 0.22f;
    if (frame) {
        r = 0.70f, gg = 0.49f, b = 0.28f;
        h = 0.85f + 0.15f * smoothstep(0.0f, 2.5f, min(border, 8 - border));
        const f32 nail = min(min(sdCircle(x, y, 4, 4, 1.5f), sdCircle(x, y, 59, 4, 1.5f)), min(sdCircle(x, y, 4, 59, 1.5f), sdCircle(x, y, 59, 59, 1.5f)));
        if (nail < 0) r = gg = b = 0.3f, h = 1.0f, shade = 1.0f;
    } else if (onBrace) {
        r = 0.66f, gg = 0.45f, b = 0.25f;
        h = 0.7f + 0.15f * smoothstep(4.5f, 2.0f, brace);
    } else {
        h = groove ? 0.2f : 0.5f;
        if (groove) shade *= 0.5f;
    }
    t = solid(r * shade, gg * shade, b * shade, h + 0.08f * g);
    t.a = cover(d);
    return t;
}

/// A small hooded creature, facing right. `guard` gives it armour colours and horns.
Texel paintCharacter(f32 x, f32 y, bool guard) {
    Texel t;
    const f32 cr = guard ? 0.62f : 0.16f, cg = guard ? 0.14f : 0.50f, cb = guard ? 0.16f : 0.62f;

    // feet
    for (int i = 0; i < 2; ++i) {
        const f32 d = sdBox(x, y, i == 0 ? 25.0f : 39.0f, 57.5f, 6, 4.5f, 3);
        over(t, solid(0.16f, 0.14f, 0.18f, 0.35f + 0.4f * dome(d, 4)), cover(d));
    }
    // body
    {
        const f32 d = sdBox(x, y, 32, 40, 14, 16, 11);
        const f32 fold = 0.92f + 0.08f * std::sin(x * 0.9f);
        over(t, solid(cr * fold, cg * fold, cb * fold, 0.35f + 0.55f * dome(d, 11)), cover(d));
        const f32 belt = sdBox(x, y, 32, 47, 14.5f, 1.8f, 1);
        if (d < 0) over(t, solid(0.85f, 0.68f, 0.25f, 0.95f), cover(belt));
    }
    if (guard) {  // horns
        for (int i = 0; i < 2; ++i) {
            const f32 cx = i == 0 ? 21.0f : 43.0f;
            const f32 lean = (i == 0 ? -1.0f : 1.0f) * (14.0f - y) * 0.35f;
            const f32 halfWidth = saturate((y - 3.0f) / 11.0f) * 3.6f;
            const f32 d = max(std::fabs(x - cx - lean) - halfWidth, max(3.0f - y, y - 15.0f));
            over(t, solid(0.86f, 0.82f, 0.70f, 0.6f + 0.3f * saturate(-d / 2)), cover(d));
        }
    }
    // hood
    {
        const f32 d = sdCircle(x, y, 32, 23, 14.5f);
        const f32 k = guard ? 0.8f : 1.12f;
        over(t, solid(cr * k, cg * k, cb * k, 0.45f + 0.55f * dome(d, 13)), cover(d));
    }
    // face opening and eyes
    {
        const f32 d = sdBox(x, y, 35.5f, 24.5f, 9.5f, 7, 6.5f);
        over(t, solid(0.05f, 0.05f, 0.09f, 0.42f), cover(d));
        for (int i = 0; i < 2; ++i) {
            const f32 ex = i == 0 ? 32.5f : 40.0f;
            const f32 eye = guard ? sdBox(x, y + (x - ex) * 0.35f, ex, 24.5f, 2.3f, 1.5f, 0.8f) : sdBox(x, y, ex, 24.5f, 1.7f, 2.6f, 1.6f);
            if (guard) {
                over(t, solid(1.0f, 0.35f, 0.15f, 0.5f, 1.0f), cover(eye));
            } else {
                over(t, solid(0.75f, 1.0f, 0.95f, 0.5f, 1.0f), cover(eye));
            }
        }
    }
    // a dark outline keeps the figure readable in the dark
    if (t.a > 0.02f && t.a < 0.98f) {
        t.r *= 0.35f;
        t.g *= 0.35f;
        t.b *= 0.35f;
    }
    return t;
}

Texel paintCoin(f32 x, f32 y) {
    Texel t;
    const f32 d = sdCircle(x, y, 32, 32, 15);
    if (d > 0.5f) return t;
    const f32 inner = sdCircle(x, y, 32, 32, 10.5f);
    f32 h = 0.5f + 0.5f * dome(d, 4);
    f32 r = 1.0f, g = 0.78f, b = 0.22f;
    if (inner < 0) {
        h = 0.55f;
        r = 0.95f, g = 0.66f, b = 0.14f;
        // a four-pointed star
        const f32 sx = std::fabs(x - 32), sy = std::fabs(y - 32);
        const f32 star = sx + sy + min(sx, sy) * 1.6f - 7.5f;
        if (star < 0) {
            h = 0.6f + 0.35f * saturate(-star / 3);
            r = 1.0f, g = 0.9f, b = 0.45f;
        }
    }
    t = solid(r, g, b, h, 0.22f);
    t.a = cover(d);
    if (d > -1.2f) t.r *= 0.6f, t.g *= 0.5f, t.b *= 0.4f;
    return t;
}

Texel paintSpikes(f32 x, f32 y) {
    Texel t;
    const f32 g = grain(x, y, 31);
    if (y >= 57) return solid(0.30f + 0.08f * g, 0.31f + 0.08f * g, 0.36f + 0.08f * g, 0.5f);  // base bar
    const f32 lx = std::fmod(x, 16.0f) - 8.0f;       // -8..8 within one spike
    const f32 halfWidth = (y - 26.0f) / 31.0f * 7.5f;  // widens downwards
    if (y < 26 || halfWidth <= 0) return t;
    const f32 d = std::fabs(lx) - halfWidth;
    if (d > 0.5f) return t;
    const f32 side = lx < 0 ? 0.80f : 0.55f;  // two facets
    t = solid(0.74f * side + 0.06f * g, 0.76f * side + 0.06f * g, 0.84f * side + 0.06f * g, 0.3f + 0.7f * saturate(1.0f - std::fabs(lx) / max(halfWidth, 0.5f)));
    t.a = cover(d);
    if (y < 30) t.r = t.g = t.b = 0.95f;  // shiny tip
    return t;
}

Texel paintDoor(f32 x, f32 y) {  // 64 x 128
    Texel t;
    const f32 d = sdBox(x, y, 32, 66, 30, 62, 5);
    if (d > 0.5f) return t;
    const f32 g = grain(x * 2.4f, y * 0.5f, 41);
    const f32 border = -d;
    const f32 plank = std::fmod(x - 4, 14.0f);
    f32 r = 0.42f, gg = 0.27f, b = 0.16f, h = 0.55f, shade = 0.78f + 0.44f * g;
    if (plank < 1.3f && border > 5) {
        h = 0.25f;
        shade *= 0.5f;
    }
    const bool band = (std::fabs(y - 30) < 5 || std::fabs(y - 102) < 5) && border > 2;
    if (border < 5) {  // iron frame
        r = 0.30f, gg = 0.31f, b = 0.36f, h = 0.9f, shade = 0.85f + 0.3f * g;
    } else if (band) {
        r = 0.34f, gg = 0.35f, b = 0.40f, h = 0.85f, shade = 0.85f + 0.3f * g;
        if (sdCircle(std::fmod(x, 14.0f), y, 7, std::fabs(y - 30) < 5 ? 30.0f : 102.0f, 1.8f) < 0) h = 1.0f, shade = 1.25f;
    }
    const f32 ring = std::fabs(sdCircle(x, y, 47, 70, 5)) - 1.3f;
    t = solid(r * shade, gg * shade, b * shade, h + 0.06f * g);
    if (ring < 0) t = solid(0.9f, 0.72f, 0.3f, 1.0f);
    t.a = cover(d);
    return t;
}

Texel paintLamp(f32 x, f32 y) {
    Texel t;
    // chain
    if (std::fabs(x - 32) < 1.3f && y < 19) {
        const bool link = std::fmod(y, 5.0f) < 3.0f;
        return solid(link ? 0.5f : 0.3f, link ? 0.5f : 0.3f, link ? 0.55f : 0.33f, link ? 0.8f : 0.4f);
    }
    // caps
    const f32 top = max(std::fabs(x - 32) - (5.0f + (y - 18.0f) * 1.1f), max(18.0f - y, y - 24.0f));
    over(t, solid(0.36f, 0.34f, 0.38f, 0.9f), cover(top));
    const f32 bottom = sdBox(x, y, 32, 48.5f, 9, 2.5f, 1.5f);
    over(t, solid(0.36f, 0.34f, 0.38f, 0.9f), cover(bottom));
    // glass
    const f32 glass = sdBox(x, y, 32, 35, 10, 11.5f, 4);
    const f32 core = saturate(1.0f - sdCircle(x, y, 32, 36, 0) / 12.0f);
    over(t, solid(1.0f, 0.80f + 0.2f * core, 0.45f + 0.5f * core, 0.5f + 0.4f * dome(glass, 8), 0.55f + 0.45f * core), cover(glass));
    // bars over the glass
    if (glass < 0 && (std::fabs(x - 25) < 0.8f || std::fabs(x - 39) < 0.8f)) over(t, solid(0.25f, 0.24f, 0.27f, 1.0f), 1.0f);
    return t;
}

Texel paint(Sprite sprite, f32 x, f32 y) {
    switch (sprite) {
        case Sprite::BrickA: return paintBrick(x, y, 1);
        case Sprite::BrickB: return paintBrick(x, y, 2);
        case Sprite::BrickTop: return paintBrickTop(x, y);
        case Sprite::BackA: return paintBack(x, y, 5);
        case Sprite::BackB: return paintBack(x, y, 6);
        case Sprite::Crate: return paintCrate(x, y);
        case Sprite::Player: return paintCharacter(x, y, false);
        case Sprite::Guard: return paintCharacter(x, y, true);
        case Sprite::Coin: return paintCoin(x, y);
        case Sprite::Spikes: return paintSpikes(x, y);
        case Sprite::Door: return paintDoor(x, y);
        case Sprite::Lamp: return paintLamp(x, y);
        case Sprite::White: return solid(1, 1, 1, 0.5f);
        case Sprite::Count: break;
    }
    return {};
}

u8 toByte(f32 v) { return static_cast<u8>(saturate(v) * 255.0f + 0.5f); }

}  // namespace

SpriteRegion spriteRegion(Sprite sprite) {
    const Cell& c = kCells[static_cast<int>(sprite)];
    // Half a texel inside the cell, so that filtering never reaches a neighbouring sprite.
    const f32 inset = 0.5f / kAtlas;
    return {static_cast<f32>(c.x * kCell) / kAtlas + inset, static_cast<f32>(c.y * kCell) / kAtlas + inset,
            static_cast<f32>((c.x + c.w) * kCell) / kAtlas - inset, static_cast<f32>((c.y + c.h) * kCell) / kAtlas - inset};
}

void generateAtlas(AtlasData& out) {
    out.width = out.height = kAtlas;
    out.albedo.assign(static_cast<std::size_t>(kAtlas) * kAtlas * 4, 0);
    out.normal.assign(static_cast<std::size_t>(kAtlas) * kAtlas * 4, 0);
    for (std::size_t i = 0; i < out.normal.size(); i += 4) {
        out.normal[i] = out.normal[i + 1] = 128;
        out.normal[i + 3] = 255;
    }

    std::vector<Texel> cell;
    for (int s = 0; s < static_cast<int>(Sprite::Count); ++s) {
        const Cell& c = kCells[s];
        const int w = c.w * kCell, h = c.h * kCell;
        cell.assign(static_cast<std::size_t>(w) * h, Texel{});
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) cell[static_cast<std::size_t>(y) * w + x] = paint(static_cast<Sprite>(s), x + 0.5f, y + 0.5f);
        }
        auto heightAt = [&](int x, int y) {
            if (c.tiles) {
                x = ((x % w) + w) % w;
                y = ((y % h) + h) % h;
            } else {
                x = clamp(x, 0, w - 1);
                y = clamp(y, 0, h - 1);
            }
            const Texel& t = cell[static_cast<std::size_t>(y) * w + x];
            return t.h * t.a;  // the silhouette itself is an edge: sprites get rounded rims
        };
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const Texel& t = cell[static_cast<std::size_t>(y) * w + x];
                const std::size_t o = (static_cast<std::size_t>(c.y * kCell + y) * kAtlas + static_cast<std::size_t>(c.x * kCell + x)) * 4;
                out.albedo[o] = toByte(t.r);
                out.albedo[o + 1] = toByte(t.g);
                out.albedo[o + 2] = toByte(t.b);
                out.albedo[o + 3] = toByte(t.a);
                // Sobel filter over the height field. Image y runs downwards, world y upwards.
                const f32 dx = (heightAt(x + 1, y - 1) + 2 * heightAt(x + 1, y) + heightAt(x + 1, y + 1)) -
                               (heightAt(x - 1, y - 1) + 2 * heightAt(x - 1, y) + heightAt(x - 1, y + 1));
                const f32 dy = (heightAt(x - 1, y + 1) + 2 * heightAt(x, y + 1) + heightAt(x + 1, y + 1)) -
                               (heightAt(x - 1, y - 1) + 2 * heightAt(x, y - 1) + heightAt(x + 1, y - 1));
                f32 nx = -dx * c.relief * 0.25f, ny = dy * c.relief * 0.25f;
                const f32 len = std::sqrt(nx * nx + ny * ny + 1.0f);
                nx /= len;
                ny /= len;
                out.normal[o] = toByte(nx * 0.5f + 0.5f);
                out.normal[o + 1] = toByte(ny * 0.5f + 0.5f);
                out.normal[o + 2] = toByte(t.e);
            }
        }
    }

    // Spread colours into fully transparent neighbours, so filtered edges do not turn dark.
    for (int pass = 0; pass < 2; ++pass) {
        std::vector<u8> copy = out.albedo;
        for (int y = 1; y < kAtlas - 1; ++y) {
            for (int x = 1; x < kAtlas - 1; ++x) {
                const std::size_t o = (static_cast<std::size_t>(y) * kAtlas + x) * 4;
                if (copy[o + 3] != 0 || (copy[o] | copy[o + 1] | copy[o + 2]) != 0) continue;
                int r = 0, g = 0, b = 0, n = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const std::size_t q = (static_cast<std::size_t>(y + dy) * kAtlas + (x + dx)) * 4;
                        if (copy[q + 3] == 0 && (copy[q] | copy[q + 1] | copy[q + 2]) == 0) continue;
                        r += copy[q];
                        g += copy[q + 1];
                        b += copy[q + 2];
                        ++n;
                    }
                }
                if (n == 0) continue;
                out.albedo[o] = static_cast<u8>(r / n);
                out.albedo[o + 1] = static_cast<u8>(g / n);
                out.albedo[o + 2] = static_cast<u8>(b / n);
            }
        }
    }
}

}  // namespace ge
