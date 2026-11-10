// G-Engine — script/text.cpp
#include "script/text.h"

#include <cmath>
#include <cstdio>

#include "core/utf8.h"

namespace ge::kotha {

int digitValue(char32_t c) {
    if (c >= '0' && c <= '9') return static_cast<int>(c - '0');
    if (c >= 0x09E6 && c <= 0x09EF) return static_cast<int>(c - 0x09E6);
    return -1;
}

bool isSpace(char32_t c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x00A0 || c == 0x200B || c == 0xFEFF || c == 0x3000 ||
           (c >= 0x2000 && c <= 0x200A);
}

bool isWordChar(char32_t c) {
    if (isAsciiAlpha(c) || c == '_') return true;
    if (isBengali(c)) return digitValue(c) < 0;  // letters, vowel signs, hasanta, ... but not digits
    if (c == kZeroWidthJoiner || c == kZeroWidthNonJoiner) return true;  // used inside Bengali conjuncts
    if (c >= 0x00C0 && c <= 0x024F && c != 0x00D7 && c != 0x00F7) return true;  // accented Latin
    return false;
}

std::string matchKey(std::string_view word) {
    std::u32string out;
    out.reserve(word.size());
    for (std::size_t i = 0; i < word.size();) {
        char32_t c = decodeUtf8(word, i);

        // Invisible characters and apostrophes never matter for matching.
        if (c == kZeroWidthJoiner || c == kZeroWidthNonJoiner || c == 0xFEFF || c == 0x00AD) continue;
        if (c == '\'' || c == 0x2019 || c == 0x2018 || c == '`') continue;

        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';

        if (c == 0x0981) continue;  // chandrabindu (ঁ) is very often left out when typing

        // Canonical composition of sequences that keyboards produce in two different ways.
        if (!out.empty()) {
            char32_t& prev = out.back();
            if (c == 0x09BE && prev == 0x09C7) { prev = 0x09CB; continue; }  // ে + া -> ো
            if (c == 0x09D7 && prev == 0x09C7) { prev = 0x09CC; continue; }  // ে + ৗ -> ৌ
            if (c == 0x09BC) {                                               // nukta
                if (prev == 0x09A1) prev = 0x09DC;       // ড + ় -> ড়
                else if (prev == 0x09A2) prev = 0x09DD;  // ঢ + ় -> ঢ়
                else if (prev == 0x09AF) prev = 0x09DF;  // য + ় -> য়
                continue;
            }
        } else if (c == 0x09BC) {
            continue;
        }

        // Fold spelling variants that people mix up freely.
        switch (c) {
            case 0x09C0: c = 0x09BF; break;  // ী -> ি
            case 0x09C2: c = 0x09C1; break;  // ূ -> ু
            case 0x09A3: c = 0x09A8; break;  // ণ -> ন
            case 0x0988: c = 0x0987; break;  // ঈ -> ই
            case 0x098A: c = 0x0989; break;  // ঊ -> উ
            default: break;
        }
        out.push_back(c);
    }
    return toUtf8(out);
}

std::string formatNumber(double value, bool bengaliDigits) {
    char buffer[64];
    double rounded = std::round(value * 1000.0) / 1000.0;
    if (rounded == std::floor(rounded) && std::fabs(rounded) < 1e15) {
        std::snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(rounded));
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.3f", rounded);
        // trim trailing zeros
        std::size_t len = 0;
        while (buffer[len] != '\0') ++len;
        while (len > 0 && buffer[len - 1] == '0') buffer[--len] = '\0';
        if (len > 0 && buffer[len - 1] == '.') buffer[--len] = '\0';
    }

    std::string out;
    for (const char* p = buffer; *p != '\0'; ++p) {
        if (bengaliDigits && *p >= '0' && *p <= '9') {
            appendUtf8(out, static_cast<char32_t>(0x09E6 + (*p - '0')));
        } else {
            out.push_back(*p);
        }
    }
    return out;
}

int editDistance(std::string_view a8, std::string_view b8) {
    const std::u32string a = toUtf32(a8);
    const std::u32string b = toUtf32(b8);
    const std::size_t n = a.size(), m = b.size();
    if (n == 0) return static_cast<int>(m);
    if (m == 0) return static_cast<int>(n);

    // Typing mistakes: a wrong, missing or extra letter, or two neighbours swapped ("wlak").
    std::vector<int> before(m + 1), prev(m + 1), cur(m + 1);
    for (std::size_t j = 0; j <= m; ++j) prev[j] = static_cast<int>(j);
    for (std::size_t i = 1; i <= n; ++i) {
        cur[0] = static_cast<int>(i);
        for (std::size_t j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            int best = prev[j - 1] + cost;
            if (prev[j] + 1 < best) best = prev[j] + 1;
            if (cur[j - 1] + 1 < best) best = cur[j - 1] + 1;
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1] && before[j - 2] + 1 < best) best = before[j - 2] + 1;
            cur[j] = best;
        }
        before.swap(prev);
        prev.swap(cur);
    }
    return prev[m];
}

namespace {

bool endsWith(std::string_view s, std::string_view suffix) {
    return s.size() > suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

}  // namespace

std::vector<std::string> nameCandidates(std::string_view key) {
    std::vector<std::string> out;
    out.emplace_back(key);

    auto add = [&out](std::string_view s) {
        if (utf8Length(s) < 2) return;
        for (const std::string& e : out) {
            if (e == s) return;
        }
        out.emplace_back(s);
    };

    const bool bengali = !key.empty() && static_cast<u8>(key[0]) >= 0x80;
    if (bengali) {
        // Longest endings first, normalised the same way as the keys.
        static const std::vector<std::string>* const kSuffixes = [] {
            static const char* const kRaw[] = {
                "গুলোকে", "গুলোর", "গুলিকে", "গুলো", "গুলি", "টাকে", "টিকে", "টার", "টির", "টায়", "টাতে", "দের",
                "য়ের",   "ের",    "এর",     "কে",   "টা",   "টি",   "রা",   "তে",  "য়",   "র",   "ে",
            };
            auto* list = new std::vector<std::string>();
            for (const char* raw : kRaw) list->push_back(matchKey(raw));
            return list;
        }();
        for (const std::string& suffix : *kSuffixes) {
            if (endsWith(key, suffix)) add(key.substr(0, key.size() - suffix.size()));
        }
    } else {
        if (endsWith(key, "es")) add(key.substr(0, key.size() - 2));
        if (endsWith(key, "s")) add(key.substr(0, key.size() - 1));
    }
    return out;
}

}  // namespace ge::kotha
