// G-Engine — core/random.h
// PCG32 random number generator: small, fast, and identical on every platform (deterministic).
#pragma once

#include "core/base.h"

namespace ge {

class Random {
public:
    explicit constexpr Random(u64 seed = 0x853c49e6748fea9bULL) { reseed(seed); }

    constexpr void reseed(u64 seed) {
        state_ = 0;
        nextU32();
        state_ += seed;
        nextU32();
    }

    constexpr u32 nextU32() {
        u64 old = state_;
        state_ = old * 6364136223846793005ULL + kIncrement;
        u32 xorshifted = static_cast<u32>(((old >> 18u) ^ old) >> 27u);
        u32 rot = static_cast<u32>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
    }

    /// Uniform float in [0, 1).
    constexpr f32 next01() { return static_cast<f32>(nextU32() >> 8) * (1.0f / 16777216.0f); }

    /// Uniform float in [lo, hi).
    constexpr f32 range(f32 lo, f32 hi) { return lo + (hi - lo) * next01(); }

    /// Uniform integer in [lo, hi] (inclusive).
    constexpr i32 rangeInt(i32 lo, i32 hi) {
        if (hi <= lo) return lo;
        u32 span = static_cast<u32>(hi - lo) + 1u;
        return lo + static_cast<i32>(nextU32() % span);
    }

private:
    static constexpr u64 kIncrement = 1442695040888963407ULL;
    u64 state_ = 0;
};

/// Stateless hash of two integers to [0, 1) — handy for procedural textures.
constexpr f32 hash01(u32 x, u32 y, u32 seed = 0) {
    u32 h = x * 0x8da6b343u ^ y * 0xd8163841u ^ (seed + 0x9e3779b9u) * 0xcb1ab31fu;
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    h *= 0x297a2d39u;
    h ^= h >> 15;
    return static_cast<f32>(h >> 8) * (1.0f / 16777216.0f);
}

/// FNV-1a hash for strings.
constexpr u64 hashString(const char* data, std::size_t size) {
    u64 h = 0xcbf29ce484222325ULL;
    for (std::size_t i = 0; i < size; ++i) {
        h ^= static_cast<u8>(data[i]);
        h *= 0x100000001b3ULL;
    }
    return h;
}

}  // namespace ge
