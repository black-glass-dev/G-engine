// G-Engine — core/utf8.h
// UTF-8 <-> Unicode code point helpers. All engine strings are UTF-8 (Bengali works everywhere).
#pragma once

#include <string>
#include <string_view>

#include "core/base.h"

namespace ge {

inline constexpr char32_t kReplacementChar = 0xFFFD;

/// Decode one code point starting at byte offset `i` and advance `i` past it.
/// Malformed input yields U+FFFD and always advances by at least one byte.
char32_t decodeUtf8(std::string_view s, std::size_t& i);

/// Append one code point as UTF-8.
void appendUtf8(std::string& out, char32_t cp);

std::u32string toUtf32(std::string_view s);
std::string toUtf8(std::u32string_view s);

/// Number of code points (not bytes).
std::size_t utf8Length(std::string_view s);

}  // namespace ge
