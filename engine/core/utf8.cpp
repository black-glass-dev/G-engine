// G-Engine — core/utf8.cpp
#include "core/utf8.h"

namespace ge {

char32_t decodeUtf8(std::string_view s, std::size_t& i) {
    const std::size_t n = s.size();
    if (i >= n) return kReplacementChar;

    const u8 b0 = static_cast<u8>(s[i]);
    if (b0 < 0x80) {
        ++i;
        return b0;
    }

    int extra = 0;
    char32_t cp = 0;
    char32_t minValue = 0;
    if ((b0 & 0xE0) == 0xC0) {
        extra = 1;
        cp = b0 & 0x1F;
        minValue = 0x80;
    } else if ((b0 & 0xF0) == 0xE0) {
        extra = 2;
        cp = b0 & 0x0F;
        minValue = 0x800;
    } else if ((b0 & 0xF8) == 0xF0) {
        extra = 3;
        cp = b0 & 0x07;
        minValue = 0x10000;
    } else {
        ++i;  // stray continuation byte or invalid lead byte
        return kReplacementChar;
    }

    if (i + static_cast<std::size_t>(extra) >= n) {
        ++i;  // truncated sequence: the continuation bytes would run past the end
        return kReplacementChar;
    }

    for (int k = 1; k <= extra; ++k) {
        const u8 b = static_cast<u8>(s[i + static_cast<std::size_t>(k)]);
        if ((b & 0xC0) != 0x80) {
            ++i;
            return kReplacementChar;
        }
        cp = (cp << 6) | (b & 0x3F);
    }

    i += static_cast<std::size_t>(extra) + 1;
    if (cp < minValue || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return kReplacementChar;
    return cp;
}

void appendUtf8(std::string& out, char32_t cp) {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = kReplacementChar;
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

std::u32string toUtf32(std::string_view s) {
    std::u32string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size();) out.push_back(decodeUtf8(s, i));
    return out;
}

std::string toUtf8(std::u32string_view s) {
    std::string out;
    out.reserve(s.size() * 3);
    for (char32_t cp : s) appendUtf8(out, cp);
    return out;
}

std::size_t utf8Length(std::string_view s) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < s.size(); ++count) decodeUtf8(s, i);
    return count;
}

}  // namespace ge
