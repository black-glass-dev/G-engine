// G-Engine — tests/test_core.cpp
#include "core/math.h"
#include "core/random.h"
#include "core/utf8.h"
#include "test_framework.h"

using namespace ge;

GE_TEST(utf8_roundtrip_bengali) {
    const std::string text = "ডানে হাঁটো, jump! ২ সেকেন্ড 😀";
    const std::u32string wide = toUtf32(text);
    CHECK_EQ(toUtf8(wide), text);
    CHECK_EQ(utf8Length("ডানে"), std::size_t{4});  // ড া ন ে
    CHECK_EQ(utf8Length("abc"), std::size_t{3});
    CHECK_EQ(wide[0], char32_t{0x09A1});
}

GE_TEST(utf8_invalid_input_never_hangs) {
    const std::string broken = std::string("ab\xE0\xA6") + "\xFF" + "c" + "\xC0\x80" + "\xF0\x9F";
    std::size_t i = 0;
    int count = 0;
    while (i < broken.size()) {
        decodeUtf8(broken, i);
        ++count;
        CHECK(count < 64);
        if (count >= 64) break;
    }
    CHECK_EQ(i, broken.size());
    // A truncated sequence at the very end yields the replacement character.
    std::size_t j = 0;
    CHECK_EQ(decodeUtf8("\xE0\xA6", j), kReplacementChar);
}

GE_TEST(math_basics) {
    CHECK_NEAR(length(Vec2{3, 4}), 5.0, 1e-6);
    CHECK_NEAR(dot(Vec2{1, 0}, Vec2{0, 1}), 0.0, 1e-6);
    CHECK_NEAR(cross(Vec2{1, 0}, Vec2{0, 1}), 1.0, 1e-6);
    const Vec2 r = rotate(Vec2{1, 0}, kHalfPi);
    CHECK_NEAR(r.x, 0.0, 1e-6);
    CHECK_NEAR(r.y, 1.0, 1e-6);
    CHECK_NEAR(moveTowards(0.0f, 10.0f, 3.0f), 3.0, 1e-6);
    CHECK_NEAR(moveTowards(9.0f, 10.0f, 3.0f), 10.0, 1e-6);
    CHECK_NEAR(wrapAngle(3.0f * kPi), kPi, 1e-5);
    CHECK_NEAR(smoothstep(0.0f, 1.0f, 0.5f), 0.5, 1e-6);
    CHECK_NEAR(linearToSrgb(srgbToLinear(0.5f)), 0.5, 1e-5);
    const Rect a = Rect::fromCenter({0, 0}, {1, 1});
    CHECK(a.contains({0.5f, -0.5f}));
    CHECK(!a.contains({1.5f, 0.0f}));
    CHECK(a.overlaps(Rect::fromCenter({1.5f, 0}, {1, 1})));
    CHECK(!a.overlaps(Rect::fromCenter({3.0f, 0}, {1, 1})));
}

GE_TEST(random_is_deterministic) {
    Random a(42), b(42), c(43);
    bool allSame = true, anyDifferent = false;
    for (int i = 0; i < 100; ++i) {
        const u32 x = a.nextU32(), y = b.nextU32(), z = c.nextU32();
        allSame = allSame && (x == y);
        anyDifferent = anyDifferent || (x != z);
    }
    CHECK(allSame);
    CHECK(anyDifferent);
    Random r(7);
    for (int i = 0; i < 1000; ++i) {
        const f32 v = r.range(2.0f, 5.0f);
        CHECK(v >= 2.0f && v < 5.0f);
        const i32 k = r.rangeInt(-2, 2);
        CHECK(k >= -2 && k <= 2);
    }
}
