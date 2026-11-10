// G-Engine — script/text.h
// Text utilities for the "Kotha" (কথা) behaviour language: Unicode classification,
// Bengali-aware normalisation and number formatting.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/base.h"

namespace ge::kotha {

inline constexpr char32_t kDanda = 0x0964;        // ।  (Bengali full stop)
inline constexpr char32_t kZeroWidthNonJoiner = 0x200C;
inline constexpr char32_t kZeroWidthJoiner = 0x200D;

inline bool isBengali(char32_t c) { return c >= 0x0980 && c <= 0x09FF; }
inline bool isAsciiAlpha(char32_t c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

/// Decimal digit value for ASCII (0-9) or Bengali (০-৯) digits, otherwise -1.
int digitValue(char32_t c);

bool isSpace(char32_t c);

/// True for characters that can be part of a word (Bengali letters and signs, Latin letters, '_').
bool isWordChar(char32_t c);

/// Canonical lookup key for a word. Two spellings that a person would consider "the same word"
/// produce the same key:
///   * Latin letters are lower-cased, apostrophes dropped ("Don't" -> "dont")
///   * zero-width joiners are removed
///   * decomposed Bengali letters are composed (ে+া -> ো, ড+় -> ড়, য+় -> য়, ...)
///   * chandrabindu is ignored (হাঁটো == হাটো), and ী/ি, ূ/ু, ণ/ন, ঈ/ই, ঊ/উ are folded together
std::string matchKey(std::string_view word);

/// Format a number without trailing zeros ("2", "2.5"). With bengaliDigits: "২", "২.৫".
std::string formatNumber(double value, bool bengaliDigits);

/// Edit distance between two UTF-8 strings measured in code points (for "did you mean ...?").
int editDistance(std::string_view a, std::string_view b);

/// Possible base names of a Bengali word with case endings removed, most specific first.
/// e.g. "শত্রুকে" -> {"শত্রুকে", "শত্রু"};  "কয়েনগুলো" -> {"কয়েনগুলো", "কয়েন"}.
/// English plurals are handled too: "coins" -> {"coins", "coin"}.
std::vector<std::string> nameCandidates(std::string_view key);

}  // namespace ge::kotha
