// G-Engine — core/math.h
// Small 2D math library: vectors, rotations, colours, rectangles and scalar helpers.
// World convention: +X right, +Y up, angles in radians (counter-clockwise), 1 unit = 1 metre.
#pragma once

#include <cmath>

#include "core/base.h"

namespace ge {

inline constexpr f32 kPi = 3.14159265358979323846f;
inline constexpr f32 kTwoPi = 6.28318530717958647692f;
inline constexpr f32 kHalfPi = 1.57079632679489661923f;

// ---- scalars ---------------------------------------------------------------------------

template <class T>
constexpr T min(T a, T b) { return a < b ? a : b; }
template <class T>
constexpr T max(T a, T b) { return a > b ? a : b; }
template <class T>
constexpr T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

constexpr f32 saturate(f32 v) { return clamp(v, 0.0f, 1.0f); }
constexpr f32 lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
constexpr f32 radians(f32 degrees) { return degrees * (kPi / 180.0f); }
constexpr f32 degrees(f32 radians) { return radians * (180.0f / kPi); }
constexpr f32 sign(f32 v) { return v > 0.0f ? 1.0f : (v < 0.0f ? -1.0f : 0.0f); }
constexpr f32 square(f32 v) { return v * v; }

inline f32 smoothstep(f32 edge0, f32 edge1, f32 x) {
    f32 t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

/// Move `current` towards `target` by at most `maxDelta` (never overshoots).
inline f32 moveTowards(f32 current, f32 target, f32 maxDelta) {
    f32 d = target - current;
    if (std::fabs(d) <= maxDelta) return target;
    return current + (d > 0.0f ? maxDelta : -maxDelta);
}

/// Frame-rate independent exponential smoothing. `rate` is in 1/seconds (higher = snappier).
inline f32 damp(f32 current, f32 target, f32 rate, f32 dt) {
    return lerp(target, current, std::exp(-rate * dt));
}

/// Wrap an angle to (-pi, pi].
inline f32 wrapAngle(f32 a) {
    a = std::fmod(a + kPi, kTwoPi);
    if (a <= 0.0f) a += kTwoPi;
    return a - kPi;
}

// ---- Vec2 ------------------------------------------------------------------------------

struct Vec2 {
    f32 x = 0.0f;
    f32 y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(f32 x_, f32 y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(f32 s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(f32 s) const { return {x / s, y / s}; }
    constexpr Vec2 operator*(Vec2 o) const { return {x * o.x, y * o.y}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    constexpr Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    constexpr Vec2& operator*=(f32 s) { x *= s; y *= s; return *this; }
    constexpr bool operator==(const Vec2&) const = default;
};

constexpr Vec2 operator*(f32 s, Vec2 v) { return {v.x * s, v.y * s}; }

constexpr f32 dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
constexpr f32 cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
constexpr f32 lengthSq(Vec2 v) { return dot(v, v); }
inline f32 length(Vec2 v) { return std::sqrt(dot(v, v)); }
inline f32 distance(Vec2 a, Vec2 b) { return length(a - b); }
/// Counter-clockwise perpendicular.
constexpr Vec2 perp(Vec2 v) { return {-v.y, v.x}; }
constexpr Vec2 lerp(Vec2 a, Vec2 b, f32 t) { return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)}; }
constexpr Vec2 min(Vec2 a, Vec2 b) { return {min(a.x, b.x), min(a.y, b.y)}; }
constexpr Vec2 max(Vec2 a, Vec2 b) { return {max(a.x, b.x), max(a.y, b.y)}; }

inline Vec2 normalize(Vec2 v) {
    f32 len = length(v);
    return len > 1e-12f ? v / len : Vec2{0.0f, 0.0f};
}

inline Vec2 rotate(Vec2 v, f32 angle) {
    f32 c = std::cos(angle), s = std::sin(angle);
    return {c * v.x - s * v.y, s * v.x + c * v.y};
}

inline Vec2 fromAngle(f32 angle) { return {std::cos(angle), std::sin(angle)}; }

// ---- Vec3 / Vec4 ------------------------------------------------------------------------

struct Vec3 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;
    constexpr Vec3() = default;
    constexpr Vec3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}
    constexpr Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(f32 s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator*(Vec3 o) const { return {x * o.x, y * o.y, z * o.z}; }
};

constexpr f32 dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 normalize(Vec3 v) {
    f32 len = std::sqrt(dot(v, v));
    return len > 1e-12f ? v * (1.0f / len) : Vec3{0.0f, 0.0f, 1.0f};
}
constexpr Vec3 lerp(Vec3 a, Vec3 b, f32 t) { return {lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)}; }

// ---- Colour ----------------------------------------------------------------------------

/// Linear-light RGBA colour (0..1, HDR values above 1 allowed for lights).
struct Color {
    f32 r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

    constexpr Color() = default;
    constexpr Color(f32 r_, f32 g_, f32 b_, f32 a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    constexpr Color operator*(f32 s) const { return {r * s, g * s, b * s, a}; }
    constexpr Color operator*(Color o) const { return {r * o.r, g * o.g, b * o.b, a * o.a}; }
    constexpr Vec3 rgb() const { return {r, g, b}; }
    constexpr Color withAlpha(f32 alpha) const { return {r, g, b, alpha}; }

    static constexpr Color white() { return {1, 1, 1, 1}; }
    static constexpr Color black() { return {0, 0, 0, 1}; }
    static constexpr Color clear() { return {0, 0, 0, 0}; }
};

inline f32 srgbToLinear(f32 c) { return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f); }
inline f32 linearToSrgb(f32 c) { return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f; }

/// Build a linear colour from the usual "screen" (sRGB) hex value, e.g. 0xFF8800.
inline Color colorFromHex(u32 rgb, f32 alpha = 1.0f) {
    return {srgbToLinear(static_cast<f32>((rgb >> 16) & 0xFF) / 255.0f),
            srgbToLinear(static_cast<f32>((rgb >> 8) & 0xFF) / 255.0f),
            srgbToLinear(static_cast<f32>(rgb & 0xFF) / 255.0f), alpha};
}

constexpr Color lerp(Color a, Color b, f32 t) {
    return {lerp(a.r, b.r, t), lerp(a.g, b.g, t), lerp(a.b, b.b, t), lerp(a.a, b.a, t)};
}

/// 8-bit sRGB colour as stored in textures and vertex data.
struct Rgba8 {
    u8 r = 255, g = 255, b = 255, a = 255;
    constexpr bool operator==(const Rgba8&) const = default;
};

inline Rgba8 toRgba8(Color c) {
    auto q = [](f32 v) { return static_cast<u8>(saturate(v) * 255.0f + 0.5f); };
    return {q(linearToSrgb(saturate(c.r))), q(linearToSrgb(saturate(c.g))), q(linearToSrgb(saturate(c.b))), q(c.a)};
}

// ---- Rect ------------------------------------------------------------------------------

/// Axis-aligned rectangle (min = bottom-left, max = top-right in world space).
struct Rect {
    Vec2 min;
    Vec2 max;

    constexpr Vec2 center() const { return (min + max) * 0.5f; }
    constexpr Vec2 size() const { return max - min; }
    constexpr bool contains(Vec2 p) const { return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y; }
    constexpr bool overlaps(const Rect& o) const {
        return min.x <= o.max.x && max.x >= o.min.x && min.y <= o.max.y && max.y >= o.min.y;
    }
    constexpr Rect expanded(f32 m) const { return {{min.x - m, min.y - m}, {max.x + m, max.y + m}}; }
    static constexpr Rect fromCenter(Vec2 c, Vec2 halfSize) { return {c - halfSize, c + halfSize}; }
};

}  // namespace ge
