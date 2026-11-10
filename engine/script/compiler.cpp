// G-Engine — script/compiler.cpp
// Kotha (কথা) compiler: natural Bengali / English sentences -> Program.
//
// The pipeline:
//   1. tokenizer   text -> words, numbers ("২", "2.5", "দেড়"), "quoted names", punctuation
//   2. lexicon     every word -> concept(s); from here on the language no longer matters
//   3. parser      a line is "<condition> <actions>" or just "<actions>". Conditions and actions
//                  are recognised from the *set of concepts* in a clause, not from word order,
//                  which is what makes both "ডানে হাঁটো" and "walk right" (and "হাঁটো ডানে") work.
//   4. code gen    rules + bytecode, plus a note per line saying how the line was understood.
#include "script/compiler.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/utf8.h"
#include "script/lexicon.h"
#include "script/text.h"

namespace ge::kotha {

namespace {

using C = Concept;

// ------------------------------------------------------------------------------------------
// Tokens
// ------------------------------------------------------------------------------------------

enum class Tok : u8 { Word, Number, String, Comma, Semi, Op };

struct Token {
    Tok kind = Tok::Word;
    std::string text;  // as written
    std::string key;   // normalised (words and strings)
    double number = 0;
    const WordInfo* info = nullptr;  // nullptr = a word we do not know (a name, or a typo)
    int column = 0;
    char op = 0;  // Tok::Op: '>' '<' '=' 'G' (>=) 'L' (<=) 'N' (!=).  Tok::Comma: ':' for a colon

    bool has(C c) const { return kind == Tok::Word && info != nullptr && info->has(c); }
    bool unknown() const { return kind == Tok::Word && info == nullptr; }
    bool conditional() const { return kind == Tok::Word && info != nullptr && info->conditional(); }
    bool filler() const { return has(C::Filler); }
};

using Tokens = std::vector<Token>;

struct Line {
    int number = 0;
    int indent = 0;
    bool colon = false;  // the line ended with ':'
    Tokens toks;
};

bool isOpenQuote(char32_t c) {
    return c == U'"' || c == U'\'' || c == 0x201C || c == 0x201D || c == 0x2018 || c == 0x2019 || c == 0x00AB || c == 0x201E;
}

bool isCloseQuote(char32_t open, char32_t c) {
    if (open == U'"' || open == 0x201C || open == 0x201D || open == 0x201E) return c == U'"' || c == 0x201C || c == 0x201D;
    if (open == U'\'' || open == 0x2018 || open == 0x2019) return c == U'\'' || c == 0x2018 || c == 0x2019;
    return c == 0x00BB;
}

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t')) --e;
    return s.substr(b, e - b);
}

void tokenizeLine(const std::u32string& s, Line& line) {
    const std::size_t n = s.size();
    std::size_t i = 0;
    int indent = 0;
    while (i < n && (s[i] == U' ' || s[i] == U'\t' || s[i] == 0x00A0)) {
        indent += s[i] == U'\t' ? 4 : 1;
        ++i;
    }
    line.indent = indent;
    Tokens& out = line.toks;

    while (i < n) {
        const char32_t c = s[i];
        if (isSpace(c)) {
            ++i;
            continue;
        }
        if (c == U'#') break;
        if (c == U'/' && i + 1 < n && s[i + 1] == U'/') break;

        Token t;
        t.column = static_cast<int>(i) + 1;

        if (isOpenQuote(c)) {
            std::size_t j = i + 1;
            std::u32string inner;
            while (j < n && !isCloseQuote(c, s[j])) inner.push_back(s[j++]);
            t.kind = Tok::String;
            t.text = trim(toUtf8(inner));
            t.key = matchKey(t.text);
            out.push_back(std::move(t));
            i = j < n ? j + 1 : n;
            continue;
        }

        const bool negative =
            c == U'-' && i + 1 < n && digitValue(s[i + 1]) >= 0 && (out.empty() || out.back().kind != Tok::Number);
        if (digitValue(c) >= 0 || negative || (c == U'.' && i + 1 < n && digitValue(s[i + 1]) >= 0)) {
            std::size_t j = negative ? i + 1 : i;
            double value = 0;
            while (j < n && digitValue(s[j]) >= 0) value = value * 10 + digitValue(s[j++]);
            if (j + 1 < n && s[j] == U'.' && digitValue(s[j + 1]) >= 0) {
                ++j;
                double scale = 0.1;
                while (j < n && digitValue(s[j]) >= 0) {
                    value += digitValue(s[j++]) * scale;
                    scale *= 0.1;
                }
            }
            t.kind = Tok::Number;
            t.number = negative ? -value : value;
            t.text = toUtf8(std::u32string_view(s).substr(i, j - i));
            out.push_back(std::move(t));
            i = j;
            continue;
        }

        if (isWordChar(c)) {
            std::size_t j = i;
            const bool latin = isAsciiAlpha(c);
            while (j < n) {
                if (isWordChar(s[j])) {
                    ++j;
                    continue;
                }
                const bool apostrophe =
                    (s[j] == U'\'' || s[j] == 0x2019) && j > i && j + 1 < n && isAsciiAlpha(s[j + 1]) && isAsciiAlpha(s[j - 1]);
                const bool digitInName = latin && s[j] >= U'0' && s[j] <= U'9';  // player2
                if (apostrophe || digitInName) {
                    ++j;
                    continue;
                }
                break;
            }
            t.kind = Tok::Word;
            t.text = toUtf8(std::u32string_view(s).substr(i, j - i));
            t.key = matchKey(t.text);
            i = j;
            if (!t.key.empty()) out.push_back(std::move(t));
            continue;
        }

        const char32_t next = i + 1 < n ? s[i + 1] : 0;
        t.text = toUtf8(std::u32string_view(s).substr(i, 1));
        switch (c) {
            case U',':
            case 0x060C:
            case 0x3001:
                t.kind = Tok::Comma;
                break;
            case U':':
                t.kind = Tok::Comma;
                t.op = ':';
                break;
            case U';':
            case U'.':
            case U'?':
            case U'|':
            case kDanda:
            case 0x0965:
                t.kind = Tok::Semi;
                break;
            case U'!':
                if (next == U'=') {
                    t.kind = Tok::Op;
                    t.op = 'N';
                    ++i;
                } else {
                    t.kind = Tok::Semi;
                }
                break;
            case U'>':
                t.kind = Tok::Op;
                t.op = '>';
                if (next == U'=') {
                    t.op = 'G';
                    ++i;
                }
                break;
            case U'<':
                t.kind = Tok::Op;
                t.op = '<';
                if (next == U'=') {
                    t.op = 'L';
                    ++i;
                }
                break;
            case U'=':
                t.kind = Tok::Op;
                t.op = '=';
                if (next == U'=') ++i;
                break;
            case 0x2265:
                t.kind = Tok::Op;
                t.op = 'G';
                break;
            case 0x2264:
                t.kind = Tok::Op;
                t.op = 'L';
                break;
            case 0x2260:
                t.kind = Tok::Op;
                t.op = 'N';
                break;
            default:
                ++i;
                continue;  // anything else (brackets, emoji, hyphens ...) is ignored
        }
        ++i;
        out.push_back(std::move(t));
    }

    // Trailing sentence marks, then a trailing colon ("যখন ...:").
    while (!out.empty() && out.back().kind == Tok::Semi) out.pop_back();
    if (!out.empty() && out.back().kind == Tok::Comma && out.back().op == ':') {
        line.colon = true;
        out.pop_back();
    }
    while (!out.empty() && (out.back().kind == Tok::Semi || out.back().kind == Tok::Comma)) out.pop_back();
    std::size_t lead = 0;
    while (lead < out.size() && (out[lead].kind == Tok::Semi || out[lead].kind == Tok::Comma)) ++lead;
    if (lead > 0) out.erase(out.begin(), out.begin() + static_cast<std::ptrdiff_t>(lead));
}

bool isUnitWord(const Token& t) {
    if (t.kind != Tok::Word) return false;
    static const std::string kTa = matchKey("টা"), kTi = matchKey("টি");
    if (t.key == kTa || t.key == kTi) return true;
    const WordInfo* info = Lexicon::get().findInflected(t.key);
    return info != nullptr &&
           (info->has(C::Second) || info->has(C::Minute) || info->has(C::Meter) || info->has(C::Step) || info->has(C::Times));
}

/// Multi-word expressions, number words and word meanings.
void refine(Tokens& toks) {
    const Lexicon& lex = Lexicon::get();
    static const std::string kBar = matchKey("বার"), kShob = matchKey("সব"), kShomoy = matchKey("সময়"), kTa = matchKey("তা"),
                             kHole = matchKey("হলে"), kNa = matchKey("না"), kNoy = matchKey("নয়");

    // 1. Expressions written as two or three words.
    Tokens merged;
    merged.reserve(toks.size());
    for (std::size_t i = 0; i < toks.size(); ++i) {
        Token t = toks[i];
        if (t.kind == Tok::Word && i + 1 < toks.size() && toks[i + 1].kind == Tok::Word) {
            const std::string& a = t.key;
            const std::string& b = toks[i + 1].key;
            const bool atStart = merged.empty() || merged.back().kind == Tok::Semi || merged.back().kind == Tok::Comma;
            const char* replacement = nullptr;
            std::size_t extra = 1;
            if (a == kBar && b == kBar && (merged.empty() || merged.back().kind != Tok::Number)) {
                replacement = "বারবার";
            } else if (a == kShob && b == kShomoy) {
                replacement = "সবসময়";
            } else if (a == kTa && b == kNa && i + 2 < toks.size() && toks[i + 2].key == kHole) {
                replacement = "নাহলে";
                extra = 2;
            } else if (a == kTa && b == kHole) {
                replacement = "তাহলে";
            } else if (a == kNa && b == kHole && atStart) {
                replacement = "নাহলে";
            } else if (a == "or" && b == "else") {
                replacement = "else";
            }
            if (replacement != nullptr) {
                for (std::size_t k = 1; k <= extra; ++k) t.text += " " + toks[i + k].text;
                t.key = matchKey(replacement);
                i += extra;
            }
        }
        merged.push_back(std::move(t));
    }

    // 2. Number words: "তিন", "three", "দেড়", "সাড়ে তিন", "twenty one", "twice".
    Tokens out;
    out.reserve(merged.size());
    auto numberAt = [&](std::size_t i, double& value) {
        if (i >= merged.size()) return false;
        if (merged[i].kind == Tok::Number) {
            value = merged[i].number;
            return true;
        }
        NumberKind kind = NumberKind::Plain;
        return merged[i].kind == Tok::Word && merged[i].key != kNoy && lex.findNumber(merged[i].key, value, kind) &&
               kind == NumberKind::Plain;
    };
    for (std::size_t i = 0; i < merged.size(); ++i) {
        const Token& t = merged[i];
        double value = 0;
        NumberKind kind = NumberKind::Plain;
        if (t.kind != Tok::Word || !lex.findNumber(t.key, value, kind)) {
            out.push_back(t);
            continue;
        }
        // "নয়" means "nine" only directly before a unit ("নয় সেকেন্ড"); otherwise it means "not".
        if (t.key == kNoy && !(i + 1 < merged.size() && isUnitWord(merged[i + 1]))) {
            out.push_back(t);
            continue;
        }
        Token num;
        num.kind = Tok::Number;
        num.text = t.text;
        num.column = t.column;
        num.number = value;
        switch (kind) {
            case NumberKind::Plain:
                if ((value == 100 || value == 1000) && !out.empty() && out.back().kind == Tok::Number) {
                    out.back().number *= value;  // "দুই শত", "three hundred"
                    out.back().text += " " + t.text;
                    continue;
                }
                break;
            case NumberKind::AddHalf:
            case NumberKind::SubQuarter:
            case NumberKind::AddQuarter: {
                double base = 0;
                if (numberAt(i + 1, base)) {
                    num.number = kind == NumberKind::AddHalf ? base + 0.5 : kind == NumberKind::SubQuarter ? base - 0.25 : base + 0.25;
                    num.text += " " + merged[i + 1].text;
                    ++i;
                }
                break;
            }
            case NumberKind::Tens: {
                double unit = 0;
                if (numberAt(i + 1, unit) && merged[i + 1].kind == Tok::Word && unit >= 1 && unit <= 9) {
                    num.number += unit;
                    num.text += " " + merged[i + 1].text;
                    ++i;
                }
                break;
            }
            case NumberKind::CountTimes: {
                out.push_back(num);
                Token times;
                times.kind = Tok::Word;
                times.text = t.text;
                times.key = kBar;
                times.column = t.column;
                out.push_back(std::move(times));
                continue;
            }
        }
        out.push_back(std::move(num));
    }

    // 3. Meanings.
    for (Token& t : out) {
        if (t.kind == Tok::Word) t.info = lex.findInflected(t.key);
    }

    // 4. "পরে লাফ দাও" / "after that, jump": an "after" that opens a statement and is not followed
    //    by an amount of time simply means "then".
    static const WordInfo* const kThen = lex.find("then");
    for (std::size_t i = 0; i < out.size(); ++i) {
        if (!out[i].has(C::After)) continue;
        bool opens = true;
        for (std::size_t j = i; j-- > 0;) {
            if (out[j].kind == Tok::Semi || out[j].kind == Tok::Comma) break;
            if (!out[j].filler()) {
                opens = false;
                break;
            }
        }
        if (!opens) continue;
        std::size_t j = i + 1;
        while (j < out.size() && out[j].filler()) ++j;
        if (j < out.size() && out[j].kind == Tok::Number) continue;
        out[i].info = kThen;
    }

    toks = std::move(out);
}

// ------------------------------------------------------------------------------------------
// A "bag" view of a clause: which concepts are present, and which tokens are still unexplained.
// ------------------------------------------------------------------------------------------

/// "খেলা" / "game": normally just a filler word, but "খেলা শেষ" / "game over" is a command.
bool isGameWord(const Token& t) {
    static const std::string* keys = new std::string[3]{matchKey("খেলা"), matchKey("গেম"), matchKey("game")};
    return t.kind == Tok::Word && (t.key == keys[0] || t.key == keys[1] || t.key == keys[2]);
}

bool ignorable(const WordInfo& info) {
    return info.has(C::Filler) || info.has(C::Do) || info.has(C::Give) || info.has(C::Keep) || info.has(C::Be) ||
           info.has(C::Stay) || info.has(C::And);
}

class Bag {
public:
    Bag(const Tokens& t, int b, int e) : t_(t), b_(b), e_(e), used_(static_cast<std::size_t>(std::max(0, e - b)), 0) {}

    bool used(int i) const { return used_[static_cast<std::size_t>(i - b_)] != 0; }
    void use(int i) { used_[static_cast<std::size_t>(i - b_)] = 1; }

    int find(C c) const {
        for (int i = b_; i < e_; ++i) {
            if (!used(i) && t_[static_cast<std::size_t>(i)].has(c)) return i;
        }
        return -1;
    }
    bool has(C c) const { return find(c) >= 0; }
    bool hasAny(std::initializer_list<C> cs) const {
        for (C c : cs) {
            if (has(c)) return true;
        }
        return false;
    }
    /// Marks every token carrying the concept as understood.
    bool take(C c) {
        bool any = false;
        for (int i = b_; i < e_; ++i) {
            if (!used(i) && t_[static_cast<std::size_t>(i)].has(c)) {
                use(i);
                any = true;
            }
        }
        return any;
    }
    bool takeAny(std::initializer_list<C> cs) {
        bool any = false;
        for (C c : cs) {
            if (take(c)) any = true;
        }
        return any;
    }
    int findKind(Tok kind) const {
        for (int i = b_; i < e_; ++i) {
            if (!used(i) && t_[static_cast<std::size_t>(i)].kind == kind) return i;
        }
        return -1;
    }
    int findUnknown() const {
        for (int i = b_; i < e_; ++i) {
            if (!used(i) && t_[static_cast<std::size_t>(i)].unknown()) return i;
        }
        return -1;
    }
    /// Nearest unexplained number before (dir = -1) or after (dir = +1) token i, skipping filler words.
    int numberNear(int i, int dir) const {
        for (int j = i + dir; j >= b_ && j < e_; j += dir) {
            const Token& t = t_[static_cast<std::size_t>(j)];
            if (used(j)) continue;
            if (t.kind == Tok::Number) return j;
            if (t.kind == Tok::Word && t.filler()) continue;
            return -1;
        }
        return -1;
    }
    /// First token that nobody explained, or -1 when the whole clause was understood.
    int leftover() const {
        for (int i = b_; i < e_; ++i) {
            if (used(i)) continue;
            const Token& t = t_[static_cast<std::size_t>(i)];
            if (t.kind == Tok::Comma) continue;
            if (t.kind == Tok::Word && t.info != nullptr && ignorable(*t.info)) continue;
            return i;
        }
        return -1;
    }

private:
    const Tokens& t_;
    int b_, e_;
    std::vector<char> used_;
};

struct Unit {
    C concept_;
    double factor;
};
const Unit kDurations[] = {{C::Second, 1.0}, {C::Minute, 60.0}};
const Unit kDistances[] = {{C::Meter, 1.0}, {C::Step, 0.5}};

template <std::size_t N>
bool takeQuantity(Bag& bag, const Tokens& t, const Unit (&units)[N], double& out) {
    for (const Unit& u : units) {
        const int idx = bag.find(u.concept_);
        if (idx < 0) continue;
        const int n = bag.numberNear(idx, -1);
        double value = 1.0;
        if (n >= 0) {
            value = t[static_cast<std::size_t>(n)].number;
            bag.use(n);
        }
        bag.take(u.concept_);
        out = value * u.factor;
        return true;
    }
    return false;
}

Dir takeDir(Bag& bag, const Tokens& t, int b, int e) {
    static const std::pair<C, Dir> kDirs[] = {
        {C::Right, Dir::Right},     {C::Left, Dir::Left},         {C::Up, Dir::Up},             {C::Down, Dir::Down},
        {C::Forward, Dir::Forward}, {C::Backward, Dir::Backward}, {C::Opposite, Dir::Opposite},
    };
    for (int i = b; i < e; ++i) {
        if (bag.used(i)) continue;
        for (const auto& [c, d] : kDirs) {
            if (t[static_cast<std::size_t>(i)].has(c)) {
                bag.use(i);
                return d;
            }
        }
    }
    return Dir::None;
}

bool hasDirection(const Bag& bag) {
    return bag.hasAny({C::Right, C::Left, C::Up, C::Down, C::Forward, C::Backward, C::Opposite});
}

bool takeButton(Bag& bag, const Tokens& t, int b, int e, Button& out) {
    static const std::pair<C, Button> kMap[] = {
        {C::Right, Button::Right}, {C::Left, Button::Left},    {C::Up, Button::Up},               {C::Down, Button::Down},
        {C::Jump, Button::Jump},   {C::KeySpace, Button::Jump}, {C::ButtonAction, Button::Action},
    };
    for (const auto& [c, button] : kMap) {
        if (bag.take(c)) {
            out = button;
            return true;
        }
    }
    static const std::string kKa = matchKey("ক"), kKha = matchKey("খ");
    for (int i = b; i < e; ++i) {
        if (bag.used(i)) continue;
        const Token& tok = t[static_cast<std::size_t>(i)];
        if (tok.kind != Tok::Word && tok.kind != Tok::String) continue;
        if (tok.key == "a" || tok.key == kKa) {
            out = Button::Jump;
            bag.use(i);
            return true;
        }
        if (tok.key == "b" || tok.key == kKha || tok.key == "x") {
            out = Button::Action;
            bag.use(i);
            return true;
        }
    }
    return false;
}

Prop peekProp(const Bag& bag) {
    if (bag.has(C::Speed)) return Prop::Speed;
    if (bag.hasAny({C::Power, C::Height})) return Prop::JumpHeight;
    if (bag.has(C::Gravity)) return Prop::Gravity;
    if (bag.has(C::Size)) return Prop::Size;
    if (bag.has(C::Brightness)) return Prop::Brightness;
    return Prop::None;
}

void takePropWords(Bag& bag, Prop prop) {
    switch (prop) {
        case Prop::Speed: bag.take(C::Speed); break;
        case Prop::JumpHeight: bag.takeAny({C::Power, C::Height, C::Jump}); break;
        case Prop::Gravity: bag.take(C::Gravity); break;
        case Prop::Size: bag.take(C::Size); break;
        case Prop::Brightness: bag.takeAny({C::Brightness, C::Light}); break;
        default: break;
    }
}

CmpOp invert(CmpOp op) {
    switch (op) {
        case CmpOp::Equal: return CmpOp::NotEqual;
        case CmpOp::NotEqual: return CmpOp::Equal;
        case CmpOp::Less: return CmpOp::GreaterEqual;
        case CmpOp::LessEqual: return CmpOp::Greater;
        case CmpOp::Greater: return CmpOp::LessEqual;
        case CmpOp::GreaterEqual: return CmpOp::Less;
    }
    return op;
}

bool isHeadVerb(const Token& t) {
    static const C kVerbs[] = {C::Walk, C::Run,  C::Go,      C::Jump,  C::Stop, C::Turn,     C::Face,     C::Return, C::Wait,
                               C::Say,  C::Send, C::Destroy, C::Shake, C::Off,  C::Increase, C::Decrease, C::Win,    C::Lose};
    for (C c : kVerbs) {
        if (t.has(c)) return true;
    }
    return t.has(C::On) && !t.filler();
}

std::string num(double v, bool bengali) { return formatNumber(v, bengali); }

std::string joinText(const Tokens& t, int b, int e) {
    std::string out;
    for (int i = b; i < e; ++i) {
        const Token& tok = t[static_cast<std::size_t>(i)];
        if (!out.empty() && tok.kind != Tok::Comma && tok.kind != Tok::Semi) out += ' ';
        if (tok.kind == Tok::String) {
            out += "\"" + tok.text + "\"";
        } else {
            out += tok.text;
        }
    }
    return out;
}

// ------------------------------------------------------------------------------------------
// Descriptions ("this is how I understood it")
// ------------------------------------------------------------------------------------------

const char* dirText(Dir d, bool bn) {
    switch (d) {
        case Dir::Right: return bn ? "ডানে" : "right";
        case Dir::Left: return bn ? "বামে" : "left";
        case Dir::Up: return bn ? "উপরে" : "up";
        case Dir::Down: return bn ? "নিচে" : "down";
        case Dir::Forward: return bn ? "সামনে" : "forward";
        case Dir::Backward: return bn ? "পিছনে" : "backward";
        case Dir::Opposite: return bn ? "উল্টো দিকে" : "the other way";
        case Dir::TowardPlayer: return bn ? "খেলোয়াড়ের দিকে" : "toward the player";
        case Dir::AwayFromPlayer: return bn ? "খেলোয়াড় থেকে দূরে" : "away from the player";
        case Dir::None: break;
    }
    return "";
}

const char* buttonText(Button b, bool bn) {
    switch (b) {
        case Button::Left: return bn ? "বাম" : "LEFT";
        case Button::Right: return bn ? "ডান" : "RIGHT";
        case Button::Up: return bn ? "উপর" : "UP";
        case Button::Down: return bn ? "নিচ" : "DOWN";
        case Button::Jump: return bn ? "লাফ" : "JUMP";
        case Button::Action: return bn ? "অ্যাকশন" : "ACTION";
        case Button::Count: break;
    }
    return "?";
}

const char* propText(Prop p, bool bn) {
    switch (p) {
        case Prop::Speed: return bn ? "গতি" : "speed";
        case Prop::JumpHeight: return bn ? "লাফের উচ্চতা" : "jump height";
        case Prop::Gravity: return bn ? "মাধ্যাকর্ষণ" : "gravity";
        case Prop::Size: return bn ? "আকার" : "size";
        case Prop::Brightness: return bn ? "আলোর উজ্জ্বলতা" : "light brightness";
        case Prop::PlayerDistance: return bn ? "খেলোয়াড়ের দূরত্ব" : "distance to the player";
        case Prop::None: break;
    }
    return "";
}

const char* cmpText(CmpOp op) {
    switch (op) {
        case CmpOp::Equal: return "=";
        case CmpOp::NotEqual: return "≠";
        case CmpOp::Less: return "<";
        case CmpOp::LessEqual: return "≤";
        case CmpOp::Greater: return ">";
        case CmpOp::GreaterEqual: return "≥";
    }
    return "=";
}

std::string colorText(u32 rgb, bool bn) {
    struct Named {
        u32 rgb;
        const char* bn;
        const char* en;
    };
    static const Named kColors[] = {
        {0xFF3B30, "লাল", "red"},     {0x34C759, "সবুজ", "green"},   {0x2F80FF, "নীল", "blue"},   {0xFFD60A, "হলুদ", "yellow"},
        {0xFFFFFF, "সাদা", "white"},  {0xFF8A1E, "কমলা", "orange"},  {0xA855F7, "বেগুনি", "purple"}, {0xFF5FA2, "গোলাপি", "pink"},
        {0x32D4F5, "আকাশি", "cyan"},  {0xFFC14D, "সোনালি", "golden"},
    };
    for (const Named& c : kColors) {
        if (c.rgb == rgb) return bn ? c.bn : c.en;
    }
    return "#";
}

const std::string& shown(const std::string& label, const std::string& key) { return label.empty() ? key : label; }

std::string quoted(const std::string& s, bool bn) { return bn ? "«" + s + "»" : "\"" + s + "\""; }

std::string numOpText(const char* whatBn, const char* whatEn, NumOp op, double value, bool bn) {
    const std::string v = num(value, bn);
    switch (op) {
        case NumOp::Set: return bn ? std::string(whatBn) + " " + v + " করো" : std::string("set ") + whatEn + " to " + v;
        case NumOp::Add: return bn ? std::string(whatBn) + " " + v + " বাড়াও" : std::string("increase ") + whatEn + " by " + v;
        case NumOp::Sub: return bn ? std::string(whatBn) + " " + v + " কমাও" : std::string("decrease ") + whatEn + " by " + v;
        case NumOp::Mul:
            if (value >= 1.0) return bn ? std::string(whatBn) + " বাড়াও (×" + v + ")" : std::string("increase ") + whatEn + " (×" + v + ")";
            return bn ? std::string(whatBn) + " কমাও (×" + v + ")" : std::string("decrease ") + whatEn + " (×" + v + ")";
    }
    return {};
}

std::string waitText(double seconds, bool bn) {
    return bn ? num(seconds, true) + " সেকেন্ড অপেক্ষা করো" : "wait " + num(seconds, false) + " s";
}

void jsonString(std::string& out, const std::string& s) {
    out += '"';
    for (const char ch : s) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    out += ' ';
                } else {
                    out += ch;
                }
        }
    }
    out += '"';
}

// ------------------------------------------------------------------------------------------
// Names shared by both languages
// ------------------------------------------------------------------------------------------

/// Everyday game words: the English name is treated as the same thing as the Bengali one, so a
/// Bengali script and an English script can talk about the same "স্কোর / score".
const std::unordered_map<std::string, std::string>& aliasTable() {
    static const std::unordered_map<std::string, std::string>* table = [] {
        static const char* const kPairs[][2] = {
            {"score", "স্কোর"},   {"points", "পয়েন্ট"}, {"point", "পয়েন্ট"}, {"lives", "জীবন"},   {"life", "জীবন"},
            {"health", "স্বাস্থ্য"}, {"coin", "কয়েন"},    {"coins", "কয়েন"},   {"enemy", "শত্রু"},   {"enemies", "শত্রু"},
            {"door", "দরজা"},    {"spike", "কাঁটা"},    {"spikes", "কাঁটা"},  {"star", "তারা"},    {"stars", "তারা"},
            {"box", "বাক্স"},     {"boxes", "বাক্স"},    {"crate", "বাক্স"},   {"crates", "বাক্স"},  {"flag", "পতাকা"},
            {"gem", "রত্ন"},      {"gems", "রত্ন"},      {"ghost", "ভূত"},    {"guard", "প্রহরী"},  {"stone", "পাথর"},
            {"rock", "পাথর"},    {"ball", "বল"},       {"water", "পানি"},   {"fire", "আগুন"},
            {"player", "খেলোয়াড়"}, {"lamp", "বাতি"},
        };
        auto* map = new std::unordered_map<std::string, std::string>();
        for (const auto& pair : kPairs) (*map)[matchKey(pair[0])] = matchKey(pair[1]);
        return map;
    }();
    return *table;
}

/// Is this one of the well-known names (in either language)?
bool hasAlias(const std::string& key) {
    static const std::unordered_set<std::string>* targets = [] {
        auto* set = new std::unordered_set<std::string>();
        for (const auto& entry : aliasTable()) set->insert(entry.second);
        return set;
    }();
    return aliasTable().count(key) != 0 || targets->count(key) != 0;
}

// ------------------------------------------------------------------------------------------
// Compiler
// ------------------------------------------------------------------------------------------

using Code = std::vector<Instr>;

constexpr f32 kSayDefaultSeconds = 2.5f;  // how long a speech bubble stays when the script does not say

struct Piece {
    enum Kind : u8 { Act, Wait, WaitMove, WaitLanded, LoopBegin, LoopEnd };
    Kind kind = Act;
    Action action;
    f32 seconds = 0;
    i32 count = 0;
};

using Pieces = std::vector<Piece>;

Piece pieceOf(Piece::Kind kind) {
    Piece p;
    p.kind = kind;
    return p;
}
Piece actPiece(const Action& a) {
    Piece p;
    p.action = a;
    return p;
}
Piece waitPiece(double seconds) {
    Piece p;
    p.kind = Piece::Wait;
    p.seconds = static_cast<f32>(std::max(0.0, seconds));
    return p;
}

struct BlockState {
    int pendingIf = -1;  // inside a rule: index of the JumpIfNot of the "if" just before
    int lastRule = -1;   // top level: index of the state rule just before (for "নাহলে / else")
};

struct Header {
    bool found = false;
    bool error = false;
    Trigger trigger;
    int ab = 0, ae = 0;  // the action part
};

class Compiler {
public:
    Compiler(std::string_view source, const CompileOptions& options) : source_(source), options_(options) {}

    Program run() {
        lexAll();
        collectNames();
        std::size_t i = 0;
        Code unused;
        parseBlock(i, -1, false, true, unused);
        if (!startCode_.empty()) {
            Trigger start;
            start.kind = TriggerKind::Start;
            addRule(start, startCode_, startLine_);
        }
        if (options_.strictObjects) checkObjectNames();
        std::stable_sort(program_.notes.begin(), program_.notes.end(), [](const Note& a, const Note& b) { return a.line < b.line; });
        std::stable_sort(program_.errors.begin(), program_.errors.end(),
                         [](const Diagnostic& a, const Diagnostic& b) { return a.line < b.line; });
        return std::move(program_);
    }

private:
    // ---- lexing -------------------------------------------------------------------------
    void lexAll() {
        const std::u32string text = toUtf32(source_);
        std::size_t start = 0;
        int number = 1;
        while (start <= text.size()) {
            std::size_t end = text.find(U'\n', start);
            if (end == std::u32string::npos) end = text.size();
            std::u32string raw = text.substr(start, end - start);
            if (!raw.empty() && raw.back() == U'\r') raw.pop_back();
            Line line;
            line.number = number;
            tokenizeLine(raw, line);
            refine(line.toks);
            if (!line.toks.empty()) lines_.push_back(std::move(line));
            start = end + 1;
            ++number;
        }
    }

    void collectNames() {
        for (const std::string& name : options_.knownNames) known_.insert(matchKey(name));
        for (const Line& line : lines_) {
            for (const Token& t : line.toks) {
                if (t.unknown() || t.kind == Tok::String) known_.insert(t.key);
            }
        }
    }

    /// "কয়েনগুলো" -> "কয়েন", "coins" -> "coin" when the shorter name is known from the scene or
    /// from elsewhere in the script. (Names shared by both languages are folded afterwards.)
    std::string withoutEnding(const std::string& key) const {
        std::string best = key;
        for (const std::string& candidate : nameCandidates(key)) {
            if (candidate.size() < best.size() && (known_.count(candidate) != 0 || hasAlias(candidate))) best = candidate;
        }
        return best;
    }

    /// See CompileOptions::strictObjects.
    void checkObjectNames() {
        std::vector<std::string> scene;  // keys, in the order given, without repeats
        std::string listBn, listEn;
        for (const std::string& name : options_.knownNames) {
            const std::string key = canonicalName(name);
            if (std::find(scene.begin(), scene.end(), key) != scene.end()) continue;
            scene.push_back(key);
            // The shortest English word that means the same, when there is one.
            std::string english;
            for (const auto& [from, to] : aliasTable()) {
                if (to == key && (english.empty() || from.size() < english.size() || (from.size() == english.size() && from < english))) english = from;
            }
            listBn += (listBn.empty() ? "" : ", ") + name;  // as it was spelt, not the folded key
            listEn += (listEn.empty() ? "" : ", ") + (english.empty() ? name : english);
        }
        std::unordered_set<std::string> reported;
        const auto complain = [&](int lineNumber, const std::string& key, const std::string& label) {
            if (std::find(scene.begin(), scene.end(), key) != scene.end()) return;
            if (!reported.insert(std::to_string(lineNumber) + ":" + key).second) return;
            for (const Diagnostic& d : program_.errors) {
                if (d.line == lineNumber) return;  // that line already has a problem of its own
            }
            const std::string shownName = shown(label, key);
            Diagnostic d;
            d.line = lineNumber;
            d.column = 1;
            d.bn = "এই খেলায় «" + shownName + "» নামে কিছু নেই।" + (listBn.empty() ? "" : " যা আছে: " + listBn + "।");
            d.en = "There is nothing called \"" + shownName + "\" in this game." + (listEn.empty() ? "" : " There is: " + listEn + ".");
            program_.errors.push_back(std::move(d));
        };
        for (const Rule& r : program_.rules) {
            const Trigger& t = r.trigger;
            if (t.kind == TriggerKind::Touch && !t.name.empty() && t.name[0] != '#') complain(r.line, t.name, t.label);
        }
        for (const Action& a : program_.actions) {
            if (a.kind == ActionKind::Destroy && !a.text.empty()) complain(a.line, a.text, a.label);
        }
    }

    // ---- diagnostics --------------------------------------------------------------------
    void error(const Line& line, int column, std::string bn, std::string en) {
        Diagnostic d;
        d.line = line.number;
        d.column = column;
        d.bn = std::move(bn);
        d.en = std::move(en);
        program_.errors.push_back(std::move(d));
    }

    void note(const Line& line, std::string bn, std::string en) {
        if (bn.empty() && en.empty()) return;
        // "<condition> ⇒ " with the actions on the following lines reads better as "<condition>:"
        static const std::string kArrow = " ⇒ ";
        for (std::string* s : {&bn, &en}) {
            if (s->size() >= kArrow.size() && s->compare(s->size() - kArrow.size(), kArrow.size(), kArrow) == 0) {
                s->erase(s->size() - kArrow.size());
                *s += ":";
            }
        }
        Note n;
        n.line = line.number;
        n.bn = std::move(bn);
        n.en = std::move(en);
        program_.notes.push_back(std::move(n));
    }

    void reportNotUnderstood(const Line& line, const Tokens& t, int b, int e, int failTok, bool condition) {
        const int column = (failTok >= 0 ? t[static_cast<std::size_t>(failTok)] : t[static_cast<std::size_t>(std::min(b, static_cast<int>(t.size()) - 1))]).column;
        if (failTok >= 0 && t[static_cast<std::size_t>(failTok)].unknown()) {
            const Token& tok = t[static_cast<std::size_t>(failTok)];
            std::string bn = "«" + tok.text + "» শব্দটা বুঝতে পারিনি।";
            std::string en = "I don't know the word \"" + tok.text + "\".";
            const std::string suggestion = Lexicon::get().suggest(tok.key);
            if (!suggestion.empty()) {
                bn += " আপনি কি «" + suggestion + "» বোঝাতে চেয়েছেন?";
                en += " Did you mean \"" + suggestion + "\"?";
            }
            error(line, column, std::move(bn), std::move(en));
            return;
        }
        const std::string text = joinText(t, b, e);
        std::string bn = condition ? "এই শর্তটা বুঝতে পারিনি: «" + text + "»" : "এই অংশটা বুঝতে পারিনি: «" + text + "»";
        std::string en = condition ? "I couldn't understand this condition: \"" + text + "\"" : "I couldn't understand this part: \"" + text + "\"";
        if (failTok >= 0 && e - b > 1) {
            const Token& tok = t[static_cast<std::size_t>(failTok)];
            bn += " — «" + tok.text + "» এখানে মিলছে না।";
            en += " — \"" + tok.text + "\" doesn't fit here.";
        }
        error(line, column, std::move(bn), std::move(en));
    }

    // ---- names --------------------------------------------------------------------------
    /// A name: "quoted text" or a word we do not know. `label` is how to show it to the user.
    bool takeName(Bag& bag, const Tokens& t, std::string& name, std::string& label) const {
        int index = bag.findKind(Tok::String);
        if (index < 0) index = bag.findUnknown();
        if (index < 0) return false;
        bag.use(index);
        setName(t[static_cast<std::size_t>(index)], name, label);
        return true;
    }

    void setName(const Token& tok, std::string& name, std::string& label) const {
        if (tok.kind == Tok::String) {
            name = canonicalName(tok.key);
            label = tok.text;
            return;
        }
        const std::string base = withoutEnding(tok.key);
        name = canonicalName(base);
        // Show the word as the user wrote it, without the ending that was cut off.
        label = tok.text;
        const std::size_t cut = utf8Length(tok.key) - utf8Length(base);
        if (cut > 0) {
            const std::u32string written = toUtf32(tok.text);
            label = base;
            if (cut < written.size()) {
                const std::string candidate = toUtf8(std::u32string_view(written).substr(0, written.size() - cut));
                if (matchKey(candidate) == base) label = candidate;
            }
        }
    }

    // ---- conditions ---------------------------------------------------------------------
    bool parseTrigger(const Tokens& t, int b, int e, Trigger& out) {
        fail_ = -1;
        if (b >= e) return false;
        Bag bag(t, b, e);
        bag.takeAny({C::When, C::If, C::Then, C::Self});  // the subject is always "I"
        const bool neg = bag.take(C::Not);
        Trigger tr;
        bool ok = false;

        if (bag.has(C::Every)) {
            double seconds = 0;
            if (takeQuantity(bag, t, kDurations, seconds) && seconds > 0) {
                bag.takeAny({C::Every, C::After});
                tr.kind = TriggerKind::Every;
                tr.seconds = static_cast<f32>(std::max(seconds, 0.02));
                ok = true;
            }
        } else if (bag.hasAny({C::Message, C::Receive})) {
            bag.takeAny({C::Message, C::Receive, C::Send, C::Go});  // "... বার্তা এলে", "when the message ... comes"
            if (takeName(bag, t, tr.name, tr.label)) {
                tr.kind = TriggerKind::Message;
                ok = true;
            }
        } else if (bag.hasAny({C::Press, C::Release, C::Button})) {
            const bool released = bag.take(C::Release);
            bag.takeAny({C::Press, C::Button});
            if (takeButton(bag, t, b, e, tr.button)) {
                tr.kind = released ? TriggerKind::ButtonReleased : TriggerKind::ButtonDown;
                tr.negate = neg && !released;
                ok = true;
            }
        } else if (bag.has(C::Land) || (bag.has(C::Ground) && bag.hasAny({C::Touch, C::Fall}))) {
            bag.takeAny({C::Land, C::Ground, C::Touch, C::Fall});
            tr.kind = TriggerKind::Landed;
            ok = true;
        } else if (bag.has(C::Touch)) {
            bag.take(C::Touch);
            tr.kind = TriggerKind::Touch;
            if (bag.take(C::Wall)) {
                tr.name = "#wall";
            } else if (bag.take(C::Player)) {
                tr.name = "#player";
            } else if (!bag.take(C::Anything)) {
                takeName(bag, t, tr.name, tr.label);
            }
            ok = true;
        } else if (bag.has(C::Player) && bag.hasAny({C::Near, C::Far, C::Meter})) {
            const bool far = bag.has(C::Far) && !bag.has(C::Near);
            double distance = far ? 6.0 : 3.0;
            if (!takeQuantity(bag, t, kDistances, distance)) {
                const int n = bag.findKind(Tok::Number);
                if (n >= 0) {
                    distance = t[static_cast<std::size_t>(n)].number;
                    bag.use(n);
                }
            }
            bag.takeAny({C::Player, C::Near, C::Far, C::Go});
            tr.kind = TriggerKind::Compare;
            tr.prop = Prop::PlayerDistance;
            tr.cmp = far ? CmpOp::Greater : CmpOp::Less;
            tr.value = distance;
            tr.negate = neg;
            ok = true;
        } else if (bag.hasAny({C::Ground, C::Air, C::Fall})) {
            const bool air = !bag.has(C::Ground);
            bag.takeAny({C::Ground, C::Air, C::Fall});
            tr.kind = TriggerKind::OnGround;
            tr.negate = air != neg;
            ok = true;
        } else if (bag.has(C::Start)) {
            bag.take(C::Start);
            tr.kind = TriggerKind::Start;
            ok = true;
        } else {
            // "<property or variable> [is] [more / less than] <number>"
            const Prop prop = peekProp(bag);
            std::string variable, variableLabel;
            if (prop != Prop::None) {
                takePropWords(bag, prop);
            } else {
                const int u = bag.findUnknown();
                if (u >= 0) {
                    setName(t[static_cast<std::size_t>(u)], variable, variableLabel);
                    bag.use(u);
                }
            }
            const int n = bag.findKind(Tok::Number);
            if ((prop != Prop::None || !variable.empty()) && n >= 0) {
                bag.use(n);
                CmpOp op = CmpOp::Equal;
                const int o = bag.findKind(Tok::Op);
                if (o >= 0) {
                    bag.use(o);
                    switch (t[static_cast<std::size_t>(o)].op) {
                        case '>': op = CmpOp::Greater; break;
                        case '<': op = CmpOp::Less; break;
                        case 'G': op = CmpOp::GreaterEqual; break;
                        case 'L': op = CmpOp::LessEqual; break;
                        case 'N': op = CmpOp::NotEqual; break;
                        default: break;
                    }
                } else {
                    const bool inclusive = bag.take(C::Or) || (bag.has(C::Equal) && bag.hasAny({C::More, C::Less}));
                    if (bag.take(C::More)) {
                        op = inclusive ? CmpOp::GreaterEqual : CmpOp::Greater;
                    } else if (bag.take(C::Less)) {
                        op = inclusive ? CmpOp::LessEqual : CmpOp::Less;
                    }
                    bag.take(C::Equal);
                }
                if (neg) op = invert(op);
                tr.kind = TriggerKind::Compare;
                tr.prop = prop;
                tr.name = variable;
                tr.label = variableLabel;
                tr.cmp = op;
                tr.value = t[static_cast<std::size_t>(n)].number;
                ok = true;
            }
        }

        const int left = bag.leftover();
        if (!ok || left >= 0) {
            fail_ = left;
            return false;
        }
        out = tr;
        return true;
    }

    // ---- actions ------------------------------------------------------------------------
    /// One action sentence: "ডানে ২ সেকেন্ড হাঁটো", "turn on the light", "স্কোর ১ বাড়াও" ...
    bool tryAction(const Tokens& t, int b, int e, Pieces& out, std::string& outBn, std::string& outEn) {
        fail_ = -1;
        if (b >= e) return false;
        Bag bag(t, b, e);
        bag.takeAny({C::Then, C::Self});

        Pieces pieces;
        std::string bn, en;
        auto say = [&](const std::string& partBn, const std::string& partEn) {
            if (!bn.empty()) {
                bn += ", তারপর ";
                en += ", then ";
            }
            bn += partBn;
            en += partEn;
        };
        auto act = [&](const Action& a) {
            pieces.push_back(actPiece(a));
            say(describe(a, true), describe(a, false));
        };
        auto numberOf = [&](int index) { return t[static_cast<std::size_t>(index)].number; };
        bool gameWord = false;
        for (int i = b; i < e; ++i) gameWord = gameWord || isGameWord(t[static_cast<std::size_t>(i)]);

        // "... ৩ বার" / "... 3 times"
        int repeat = 0;
        if (bag.has(C::Times)) {
            const int n = bag.numberNear(bag.find(C::Times), -1);
            if (n >= 0) {
                repeat = static_cast<int>(std::clamp(std::round(numberOf(n)), 1.0, 10000.0));
                bag.use(n);
                bag.take(C::Times);
            }
        }

        // "২ সেকেন্ড পরে ..." / "after 2 seconds ..."
        bool finished = false;
        if (bag.has(C::After)) {
            double seconds = 0;
            if (takeQuantity(bag, t, kDurations, seconds)) {
                bag.take(C::After);
                pieces.push_back(waitPiece(seconds));
                say(waitText(seconds, true), waitText(seconds, false));
                finished = bag.leftover() < 0;
            }
        }

        double seconds = 0, meters = 0;
        const bool hasDuration = !finished && takeQuantity(bag, t, kDurations, seconds);
        const bool hasDistance = !finished && takeQuantity(bag, t, kDistances, meters);
        bool durationUsed = false;
        bool restart = false;
        const bool moving = bag.hasAny({C::Walk, C::Run, C::Go});
        const bool otherVerb = moving || bag.hasAny({C::Jump, C::Stop, C::Face, C::Return, C::Destroy, C::Say, C::Send, C::Wait});

        if (finished) {
            // only the wait
        } else if (bag.has(C::Wait)) {
            bag.take(C::Wait);
            if (!hasDuration) seconds = 1.0;
            durationUsed = true;
            pieces.push_back(waitPiece(seconds));
            say(waitText(seconds, true), waitText(seconds, false));
        } else if (bag.has(C::Say) && bag.findKind(Tok::String) >= 0) {
            const int s = bag.findKind(Tok::String);
            bag.use(s);
            bag.take(C::Say);
            Action a;
            a.kind = ActionKind::Say;
            a.text = t[static_cast<std::size_t>(s)].text;
            a.seconds = hasDuration ? static_cast<f32>(seconds) : kSayDefaultSeconds;
            durationUsed = true;
            act(a);
        } else if (bag.has(C::Send) || (bag.has(C::Message) && (bag.findKind(Tok::String) >= 0 || bag.findUnknown() >= 0))) {
            bag.takeAny({C::Send, C::Message});
            Action a;
            a.kind = ActionKind::Send;
            if (!takeName(bag, t, a.text, a.label)) {
                fail_ = bag.leftover();
                return false;
            }
            act(a);
        } else if (bag.has(C::Shake)) {
            bag.takeAny({C::Shake, C::Camera});
            Action a;
            a.kind = ActionKind::Shake;
            a.value = 0.35;
            if (bag.takeAny({C::More, C::Fast})) a.value = 0.7;
            if (bag.takeAny({C::Less, C::Slow})) a.value = 0.15;
            a.seconds = hasDuration ? static_cast<f32>(seconds) : 0.35f;
            durationUsed = true;
            act(a);
        } else if (bag.has(C::Win) || bag.has(C::Lose) || (bag.has(C::End) && gameWord)) {
            // "জিতে যাও" / "win", "হেরে যাও" / "lose", "খেলা শেষ" / "game over"
            Action a;
            a.kind = ActionKind::Game;
            a.value = static_cast<f64>(bag.has(C::Win) ? GameEvent::Win : GameEvent::Lose);
            bag.takeAny({C::Win, C::Lose, C::Go, C::End});
            act(a);
        } else if (bag.has(C::Again) && !otherVerb && !bag.hasAny({C::Turn, C::On, C::Off}) && !hasDirection(bag)) {
            bag.takeAny({C::Again, C::Start});
            Action a;
            a.kind = ActionKind::Game;
            a.value = static_cast<f64>(GameEvent::Restart);
            restart = true;
            act(a);
        } else if (bag.hasAny({C::On, C::Off}) && (bag.hasAny({C::Light, C::ColorName}) || !otherVerb)) {
            const bool on = !bag.has(C::Off);
            const int colour = bag.find(C::ColorName);
            if (colour >= 0) {
                Action c;
                c.kind = ActionKind::LightColor;
                c.value = static_cast<f64>(t[static_cast<std::size_t>(colour)].info->sense(C::ColorName)->value);
                act(c);
            }
            bag.takeAny({C::On, C::Off, C::Light, C::Turn, C::Color, C::ColorName});
            Action a;
            a.kind = ActionKind::LightSwitch;
            a.value = on ? 1.0 : 0.0;
            act(a);
        } else if (bag.has(C::ColorName)) {
            const int colour = bag.find(C::ColorName);
            Action a;
            a.kind = ActionKind::LightColor;
            a.value = static_cast<f64>(t[static_cast<std::size_t>(colour)].info->sense(C::ColorName)->value);
            bag.takeAny({C::ColorName, C::Light, C::Color, C::Change});
            act(a);
        } else if (bag.hasAny({C::Brightness, C::Bright, C::Dim}) ||
                   (bag.has(C::Light) && bag.hasAny({C::Increase, C::Decrease, C::More, C::Less}))) {
            const bool increase = bag.has(C::Increase), decrease = bag.has(C::Decrease);
            const bool down = decrease || bag.hasAny({C::Dim, C::Less});
            bag.takeAny({C::Brightness, C::Bright, C::Dim, C::Light, C::Increase, C::Decrease, C::More, C::Less});
            Action a;
            a.kind = ActionKind::SetProp;
            a.prop = Prop::Brightness;
            const int n = bag.findKind(Tok::Number);
            if (n >= 0) {
                bag.use(n);
                a.value = numberOf(n);
                a.op = increase ? NumOp::Add : decrease ? NumOp::Sub : NumOp::Set;
            } else {
                a.op = NumOp::Mul;
                a.value = down ? 0.6 : 1.5;
            }
            act(a);
        } else if (!moving && peekProp(bag) != Prop::None &&
                   (bag.findKind(Tok::Number) >= 0 || bag.hasAny({C::Increase, C::Decrease, C::More, C::Less, C::Fast, C::Slow}))) {
            const Prop prop = peekProp(bag);
            const bool increase = bag.take(C::Increase), decrease = bag.take(C::Decrease);
            const bool less = bag.takeAny({C::Less, C::Slow});
            bag.takeAny({C::More, C::Fast, C::High});
            takePropWords(bag, prop);
            Action a;
            a.kind = ActionKind::SetProp;
            a.prop = prop;
            const int n = bag.findKind(Tok::Number);
            if (n >= 0) {
                bag.use(n);
                a.value = numberOf(n);
                a.op = increase ? NumOp::Add : decrease ? NumOp::Sub : NumOp::Set;
            } else {
                a.op = NumOp::Mul;
                a.value = (decrease || less) ? 0.8 : 1.25;
            }
            act(a);
        } else if (bag.findUnknown() >= 0 && !otherVerb && !bag.has(C::Turn) &&
                   (bag.findKind(Tok::Number) >= 0 || bag.hasAny({C::Increase, C::Decrease}))) {
            const int u = bag.findUnknown();
            bag.use(u);
            const bool increase = bag.take(C::Increase), decrease = bag.take(C::Decrease);
            const int o = bag.findKind(Tok::Op);
            if (o >= 0 && t[static_cast<std::size_t>(o)].op == '=') bag.use(o);
            Action a;
            a.kind = ActionKind::SetVar;
            setName(t[static_cast<std::size_t>(u)], a.text, a.label);
            a.value = 1.0;
            const int n = bag.findKind(Tok::Number);
            if (n >= 0) {
                bag.use(n);
                a.value = numberOf(n);
            }
            a.op = increase ? NumOp::Add : decrease ? NumOp::Sub : NumOp::Set;
            act(a);
        } else if (bag.hasAny({C::Turn, C::Face}) || (bag.has(C::Opposite) && !moving)) {
            bag.takeAny({C::Turn, C::Face, C::Go, C::Stop});
            Dir dir = takeDir(bag, t, b, e);
            if (bag.take(C::Player)) dir = bag.take(C::Far) ? Dir::AwayFromPlayer : Dir::TowardPlayer;
            bag.take(C::Near);
            Action a;
            if (dir == Dir::None || dir == Dir::Opposite || dir == Dir::Backward) {
                a.kind = ActionKind::Flip;
            } else {
                a.kind = ActionKind::Face;
                a.dir = dir;
            }
            act(a);
        } else if (bag.has(C::Stop) || (bag.has(C::Off) && moving)) {
            bag.takeAny({C::Stop, C::Off, C::Walk, C::Run, C::Go});
            Action a;
            a.kind = ActionKind::Stop;
            act(a);
        } else if (bag.has(C::Destroy)) {
            bag.takeAny({C::Destroy, C::Go, C::Self});
            Action a;
            a.kind = ActionKind::Destroy;
            takeName(bag, t, a.text, a.label);
            act(a);
        } else if (bag.has(C::Return) || (bag.has(C::Start) && bag.hasAny({C::Backward, C::Go}))) {
            bag.takeAny({C::Return, C::Go, C::Start, C::Backward});
            Action a;
            a.kind = ActionKind::Respawn;
            act(a);
        } else if (bag.has(C::Jump)) {
            bag.takeAny({C::Jump, C::Go});
            Action a;
            a.kind = ActionKind::Jump;
            if (bag.takeAny({C::More, C::High, C::Fast, C::Power})) {
                a.scale = 1.4f;
            } else if (bag.takeAny({C::Less, C::Slow})) {
                a.scale = 0.7f;
            }
            const Dir dir = takeDir(bag, t, b, e);
            if (dir == Dir::Right || dir == Dir::Left || dir == Dir::Forward || dir == Dir::Backward) {
                // "ডানে লাফ দাও": run-up, jump, and stop again after landing.
                a.dir = dir;
                Action go;
                go.kind = ActionKind::Move;
                go.dir = dir;
                Action halt;
                halt.kind = ActionKind::Stop;
                pieces.push_back(actPiece(go));
                pieces.push_back(actPiece(a));
                pieces.push_back(pieceOf(Piece::WaitLanded));
                pieces.push_back(actPiece(halt));
                say(describe(a, true), describe(a, false));
            } else {
                act(a);
                if (repeat > 0 || repeatDepth_ > 0) pieces.push_back(pieceOf(Piece::WaitLanded));
            }
        } else if (moving || hasDirection(bag) || hasDistance || bag.has(C::Player)) {
            Action a;
            a.kind = ActionKind::Move;
            a.run = bag.has(C::Run);
            bag.takeAny({C::Walk, C::Run, C::Go});
            if (bag.take(C::Fast)) {
                a.scale = 1.5f;
            } else if (bag.take(C::Slow)) {
                a.scale = 0.5f;
            }
            a.dir = takeDir(bag, t, b, e);
            if (bag.take(C::Player)) a.dir = bag.take(C::Far) ? Dir::AwayFromPlayer : Dir::TowardPlayer;
            bag.take(C::Near);
            if (a.dir == Dir::None) a.dir = Dir::Forward;
            if (bag.has(C::Speed)) {  // "৫ গতিতে হাঁটো"
                const int n = bag.numberNear(bag.find(C::Speed), -1);
                if (n >= 0) {
                    bag.use(n);
                    Action speed;
                    speed.kind = ActionKind::SetProp;
                    speed.prop = Prop::Speed;
                    speed.value = numberOf(n);
                    act(speed);
                }
                bag.take(C::Speed);
            }
            if (hasDuration) {
                a.seconds = static_cast<f32>(seconds);
                durationUsed = true;
            } else if (hasDistance) {
                a.meters = static_cast<f32>(meters);
            }
            act(a);
            if (a.seconds > 0 || a.meters > 0) pieces.push_back(pieceOf(Piece::WaitMove));
        } else if (hasDuration) {
            durationUsed = true;
            pieces.push_back(waitPiece(seconds));
            say(waitText(seconds, true), waitText(seconds, false));
        } else {
            fail_ = bag.leftover();
            return false;
        }

        if (!restart) bag.take(C::Again);
        const int left = bag.leftover();
        if (left >= 0) {
            fail_ = left;
            return false;
        }
        if (hasDuration && !durationUsed) {  // "২ সেকেন্ড থামো" = stop, and stay like that for 2 s
            pieces.push_back(waitPiece(seconds));
            say(waitText(seconds, true), waitText(seconds, false));
        }

        if (repeat > 1) {
            Piece begin = pieceOf(Piece::LoopBegin);
            begin.count = repeat;
            out.push_back(begin);
            out.insert(out.end(), pieces.begin(), pieces.end());
            out.push_back(pieceOf(Piece::LoopEnd));
            bn += " (" + num(repeat, true) + " বার)";
            en += " (" + num(repeat, false) + " times)";
        } else {
            out.insert(out.end(), pieces.begin(), pieces.end());
        }
        if (!outBn.empty()) {
            outBn += ", তারপর ";
            outEn += ", then ";
        }
        outBn += bn;
        outEn += en;
        return true;
    }

    /// A stretch without hard separators. It may still contain several actions:
    /// "লাফ দাও আর ডানে হাঁটো", "ডানে হেঁটে লাফ দাও", "walk right jump".
    bool parseSegment(const Tokens& t, int b, int e, Pieces& out, std::string& bn, std::string& en, int depth) {
        {
            Pieces pieces;
            std::string pb, pe;
            if (tryAction(t, b, e, pieces, pb, pe)) {
                merge(out, bn, en, pieces, pb, pe);
                return true;
            }
        }
        const int firstFail = fail_;
        if (depth < 8) {
            auto trySplit = [&](int leftEnd, int rightBegin) {
                Pieces left, right;
                std::string lb, le, rb, re;
                if (!tryAction(t, b, leftEnd, left, lb, le)) return false;
                if (!parseSegment(t, rightBegin, e, right, rb, re, depth + 1)) return false;
                merge(out, bn, en, left, lb, le);
                merge(out, bn, en, right, rb, re);
                return true;
            };
            for (int k = b + 1; k < e - 1; ++k) {
                const Token& tok = t[static_cast<std::size_t>(k)];
                if ((tok.kind == Tok::Comma || tok.has(C::And)) && trySplit(k, k + 1)) return true;
            }
            int firstVerb = -1, lastVerb = -1;
            for (int i = b; i < e; ++i) {
                if (!isHeadVerb(t[static_cast<std::size_t>(i)])) continue;
                if (firstVerb < 0) firstVerb = i;
                lastVerb = i;
            }
            if (firstVerb >= 0 && lastVerb > firstVerb) {
                for (int p = lastVerb; p > firstVerb; --p) {
                    if (trySplit(p, p)) return true;
                }
            }
        }
        fail_ = firstFail;
        failB_ = b;
        failE_ = e;
        return false;
    }

    static void merge(Pieces& out, std::string& bn, std::string& en, const Pieces& pieces, const std::string& pb, const std::string& pe) {
        out.insert(out.end(), pieces.begin(), pieces.end());
        if (!bn.empty() && !pb.empty()) {
            bn += ", তারপর ";
            en += ", then ";
        }
        bn += pb;
        en += pe;
    }

    /// A list of actions separated by "তারপর / then", ";" or "।".
    bool parseActions(const Tokens& t, int b, int e, Pieces& out, std::string& bn, std::string& en) {
        int start = b;
        bool any = false;
        for (int i = b; i <= e; ++i) {
            const bool separator = i == e || t[static_cast<std::size_t>(i)].kind == Tok::Semi || t[static_cast<std::size_t>(i)].has(C::Then);
            if (!separator) continue;
            if (i > start) {
                if (!parseSegment(t, start, i, out, bn, en, 0)) return false;
                any = true;
            }
            start = i + 1;
        }
        if (!any) {
            fail_ = -1;
            failB_ = b;
            failE_ = e;
        }
        return any;
    }

    void emitPieces(const Pieces& pieces, Code& out, int lineNumber) {
        std::vector<i32> loops;
        for (const Piece& p : pieces) {
            Instr in;
            switch (p.kind) {
                case Piece::Act:
                    in.op = Op::Act;
                    in.a = static_cast<i32>(program_.actions.size());
                    program_.actions.push_back(p.action);
                    program_.actions.back().line = lineNumber;
                    break;
                case Piece::Wait:
                    in.op = Op::Wait;
                    in.f = p.seconds;
                    break;
                case Piece::WaitMove: in.op = Op::WaitMove; break;
                case Piece::WaitLanded: in.op = Op::WaitLanded; break;
                case Piece::LoopBegin:
                    in.op = Op::LoopBegin;
                    in.a = p.count;
                    out.push_back(in);
                    loops.push_back(static_cast<i32>(out.size()));
                    continue;
                case Piece::LoopEnd:
                    in.op = Op::LoopEnd;
                    in.b = loops.empty() ? 0 : loops.back();
                    if (!loops.empty()) loops.pop_back();
                    break;
            }
            out.push_back(in);
        }
    }

    // ---- statements ---------------------------------------------------------------------
    static int firstMeaningful(const Tokens& t) {
        int i = 0;
        const int n = static_cast<int>(t.size());
        while (i < n && t[static_cast<std::size_t>(i)].kind == Tok::Word && t[static_cast<std::size_t>(i)].filler()) ++i;
        return i;
    }

    /// A line that only closes a block: "শেষ" / "end". ("খেলা শেষ" is not one: it ends the game.)
    static bool isEndLine(const Line& line) {
        bool any = false;
        for (const Token& t : line.toks) {
            if (isGameWord(t)) return false;
            if (t.has(C::End)) {
                any = true;
            } else if (!(t.kind == Tok::Word && t.info != nullptr && ignorable(*t.info))) {
                return false;
            }
        }
        return any;
    }

    /// Does this line begin a new top-level rule? (Ends a block written without indentation.)
    static bool startsRule(const Line& line) {
        const Tokens& t = line.toks;
        const int first = firstMeaningful(t);
        if (first >= static_cast<int>(t.size())) return false;
        const Token& head = t[static_cast<std::size_t>(first)];
        if (head.has(C::When) || head.has(C::If) || head.has(C::Start) || head.has(C::Every) || head.has(C::Else)) return true;
        for (const Token& tok : t) {
            if (tok.conditional()) return true;
        }
        return false;
    }

    Header parseHeader(const Line& line, bool top, bool hasBlock) {
        const Tokens& t = line.toks;
        const int n = static_cast<int>(t.size());
        Header h;
        const int first = firstMeaningful(t);
        if (first >= n) return h;
        auto at = [&](int i) -> const Token& { return t[static_cast<std::size_t>(i)]; };
        auto skipSeparators = [&](int i) {
            while (i < n && (at(i).kind == Tok::Comma || at(i).has(C::Then))) ++i;
            return i;
        };
        auto found = [&](int ab, int ae) {
            h.found = true;
            h.ab = ab;
            h.ae = ae;
            return h;
        };

        // A. "যখন ... / যদি ... / when ... / if ..."
        if (at(first).has(C::When) || at(first).has(C::If)) {
            const int cb = first + 1;
            for (int k = cb + 1; k < n; ++k) {  // an explicit "তখন / তাহলে / then"
                if (at(k).has(C::Then) && parseTrigger(t, cb, k, h.trigger)) return found(skipSeparators(k), n);
            }
            for (int k = n - 1; k >= cb; --k) {  // a Bengali "-লে" verb closes the condition
                if (at(k).conditional() && parseTrigger(t, cb, k + 1, h.trigger)) return found(skipSeparators(k + 1), n);
            }
            for (int k = cb + 1; k < n; ++k) {  // comma or colon
                if (at(k).kind == Tok::Comma && parseTrigger(t, cb, k, h.trigger)) return found(skipSeparators(k), n);
            }
            if ((line.colon || hasBlock) && parseTrigger(t, cb, n, h.trigger)) return found(n, n);
            for (int k = n - 1; k > cb; --k) {  // no separator: the longest condition that leaves valid actions
                Trigger candidate;
                if (!parseTrigger(t, cb, k, candidate)) continue;
                Pieces pieces;
                std::string bn, en;
                if (parseActions(t, k, n, pieces, bn, en)) {
                    h.trigger = candidate;
                    return found(k, n);
                }
            }
            if (parseTrigger(t, cb, n, h.trigger)) return found(n, n);
            failB_ = cb;
            failE_ = n;
            h.error = true;
            return h;
        }

        // A2. English "on <event>, ...": "on message "open", disappear" — the way the notes word events.
        if (top && first > 0 && at(first - 1).key == "on") {
            for (int k = first + 1; k < n; ++k) {
                if (at(k).kind == Tok::Comma && parseTrigger(t, first, k, h.trigger)) return found(skipSeparators(k), n);
            }
            if ((line.colon || hasBlock) && parseTrigger(t, first, n, h.trigger)) return found(n, n);
            for (int k = n - 1; k > first; --k) {  // no separator: the longest event that leaves valid actions
                Trigger candidate;
                if (!parseTrigger(t, first, k, candidate)) continue;
                Pieces pieces;
                std::string bn, en;
                if (parseActions(t, k, n, pieces, bn, en)) {
                    h.trigger = candidate;
                    return found(k, n);
                }
            }
        }

        // B. the condition comes last: "jump when the button is pressed"
        for (int w = first + 1; w < n - 1; ++w) {
            if (!(at(w).has(C::When) || at(w).has(C::If))) continue;
            if (parseTrigger(t, w + 1, n, h.trigger)) return found(first, w);
        }

        if (top) {
            // C. "শুরুতে ..." / "on start ..."
            if (at(first).has(C::Start)) {
                // "শুরুতে ফিরে যাও" is one action (go back to the start), not "at the start: go back".
                Bag rest(t, first + 1, n);
                const bool goBack = first + 1 < n && rest.takeAny({C::Return, C::Go, C::Backward}) && rest.leftover() < 0;
                if (goBack) return h;
                h.trigger.kind = TriggerKind::Start;
                return found(skipSeparators(first + 1), n);
            }
            // D. "প্রতি ২ সেকেন্ডে ..." / "every 2 seconds ..."
            if (at(first).has(C::Every)) {
                int k = first + 1;
                while (k < n && !(at(k).has(C::Second) || at(k).has(C::Minute))) ++k;
                if (k < n) {
                    ++k;
                    while (k < n && at(k).has(C::After)) ++k;
                    if (parseTrigger(t, first, k, h.trigger)) return found(skipSeparators(k), n);
                }
                failB_ = first;
                failE_ = n;
                h.error = true;
                return h;
            }
        }

        // E. a Bengali "-লে" verb without যখন / যদি: "ডান বোতাম চাপলে ডানে হাঁটো"
        for (int k = n - 1; k >= first; --k) {
            if (at(k).conditional() && parseTrigger(t, first, k + 1, h.trigger)) return found(skipSeparators(k + 1), n);
        }

        // F. "<condition>:" followed by a block, or "<condition>: <actions>"
        if ((line.colon || hasBlock) && parseTrigger(t, first, n, h.trigger)) return found(n, n);
        for (int k = first + 1; k < n; ++k) {
            if (at(k).kind == Tok::Comma && at(k).op == ':' && parseTrigger(t, first, k, h.trigger)) return found(skipSeparators(k), n);
        }
        return h;
    }

    using Children = std::function<void(Code&)>;

    /// "<actions>" and/or a child block, with optional "বারবার / forever" and "N বার / N times".
    void compileBody(const Line& line, const Tokens& source, int b, int e, bool hasBlock, const Children& children, Code& out,
                     std::string& bn, std::string& en) {
        Tokens t;
        bool forever = false;
        for (int i = b; i < e; ++i) {
            const Token& tok = source[static_cast<std::size_t>(i)];
            if (tok.has(C::Forever)) {
                forever = true;
                continue;
            }
            t.push_back(tok);
        }
        int n = static_cast<int>(t.size());

        int repeat = 0;
        {
            Bag bag(t, 0, n);
            const int times = bag.find(C::Times);
            const int count = times >= 0 ? bag.numberNear(times, -1) : -1;
            if (count >= 0) {
                if (forever) forever = false;  // "repeat 3 times" is a counted loop
                bag.use(count);
                bag.take(C::Times);
                if (bag.leftover() < 0) {  // the line says nothing but "N times": the block is the body
                    repeat = static_cast<int>(std::clamp(std::round(t[static_cast<std::size_t>(count)].number), 1.0, 10000.0));
                    t.clear();
                    n = 0;
                }
            }
        }

        const i32 loopStart = static_cast<i32>(out.size());
        i32 bodyStart = 0;
        if (repeat > 0) {
            Instr begin;
            begin.op = Op::LoopBegin;
            begin.a = repeat;
            out.push_back(begin);
            bodyStart = static_cast<i32>(out.size());
            ++repeatDepth_;
        }
        const std::size_t before = out.size();
        bool ok = true;
        if (n > 0) {
            Pieces pieces;
            if (parseActions(t, 0, n, pieces, bn, en)) {
                emitPieces(pieces, out, line.number);
            } else {
                ok = false;
                reportNotUnderstood(line, t, failB_, std::min(failE_, n), fail_, false);
            }
        }
        if (hasBlock) children(out);
        if (ok && out.size() == before) {
            const int column = source.empty() ? 1 : source.back().column;
            error(line, column, "এখানে কী করতে হবে তা লেখা নেই।", "Nothing to do here — please add an action.");
        }
        if (repeat > 0) {
            Instr end;
            end.op = Op::LoopEnd;
            end.b = bodyStart;
            out.push_back(end);
            --repeatDepth_;
            bn = num(repeat, true) + " বার:" + (bn.empty() ? "" : " " + bn);
            en = num(repeat, false) + " times:" + (en.empty() ? "" : " " + en);
        }
        if (forever) {
            Instr yield;
            yield.op = Op::Yield;
            out.push_back(yield);
            Instr jump;
            jump.op = Op::Jump;
            jump.a = loopStart;
            out.push_back(jump);
            bn = "বারবার:" + (bn.empty() ? std::string() : " " + bn);
            en = "forever:" + (en.empty() ? std::string() : " " + en);
        }
    }

    int addRule(const Trigger& trigger, const Code& body, int lineNumber) {
        Rule rule;
        rule.trigger = trigger;
        rule.entry = static_cast<i32>(program_.code.size());
        rule.line = lineNumber;
        const i32 base = rule.entry;
        for (std::size_t i = 0; i < body.size(); ++i) {
            Instr in = body[i];
            switch (in.op) {
                case Op::Jump:
                    if (in.a <= static_cast<i32>(i)) rule.stopOnFall = true;
                    in.a += base;
                    break;
                case Op::JumpIfNot:
                case Op::LoopEnd: in.b += base; break;
                case Op::Act: {
                    const Action& a = program_.actions[static_cast<std::size_t>(in.a)];
                    if (a.kind == ActionKind::Move && a.seconds <= 0 && a.meters <= 0) rule.holdsMove = true;
                    break;
                }
                default: break;
            }
            program_.code.push_back(in);
        }
        program_.code.push_back(Instr{});  // Op::End
        program_.rules.push_back(std::move(rule));
        return static_cast<int>(program_.rules.size()) - 1;
    }

    void parseBlock(std::size_t& i, int parentIndent, bool flat, bool top, Code& out) {
        BlockState state;
        while (i < lines_.size()) {
            const Line& line = lines_[i];
            if (flat) {
                if (line.indent < parentIndent) break;
                if (isEndLine(line)) {
                    ++i;
                    break;
                }
                if (line.indent == parentIndent && startsRule(line)) break;
            } else {
                if (line.indent <= parentIndent) break;
                if (isEndLine(line)) {
                    ++i;
                    state = BlockState{};
                    continue;
                }
            }
            parseStatement(i, top, out, state);
        }
    }

    void parseStatement(std::size_t& i, bool top, Code& out, BlockState& state) {
        const Line& line = lines_[i];
        ++i;
        const Tokens& t = line.toks;
        const int n = static_cast<int>(t.size());
        const bool indented = i < lines_.size() && lines_[i].indent > line.indent;
        const bool hasBlock = indented || line.colon;
        const Children children = [&](Code& buffer) {
            if (indented) {
                parseBlock(i, line.indent, false, false, buffer);
            } else if (line.colon) {
                parseBlock(i, line.indent, true, false, buffer);
            }
        };
        auto skipChildren = [&]() {
            if (!hasBlock) return;
            Code dummy;
            children(dummy);
        };

        const int first = firstMeaningful(t);
        if (first >= n) {
            skipChildren();
            return;
        }
        std::string bn, en;

        // ---- "নাহলে ..." / "else ..." ----
        if (t[static_cast<std::size_t>(first)].has(C::Else)) {
            int ab = first + 1;
            while (ab < n && (t[static_cast<std::size_t>(ab)].kind == Tok::Comma || t[static_cast<std::size_t>(ab)].has(C::Then))) ++ab;
            if (top && state.lastRule >= 0) {
                Trigger trigger = program_.rules[static_cast<std::size_t>(state.lastRule)].trigger;
                trigger.negate = !trigger.negate;
                Code body;
                compileBody(line, t, ab, n, hasBlock, children, body, bn, en);
                addRule(trigger, body, line.number);
                note(line, "নাহলে (" + describe(trigger, true) + ") ⇒ " + bn, "otherwise (" + describe(trigger, false) + ") ⇒ " + en);
            } else if (!top && state.pendingIf >= 0) {
                Instr jump;
                jump.op = Op::Jump;
                out.push_back(jump);
                const std::size_t jumpIndex = out.size() - 1;
                out[static_cast<std::size_t>(state.pendingIf)].b = static_cast<i32>(out.size());
                compileBody(line, t, ab, n, hasBlock, children, out, bn, en);
                out[jumpIndex].a = static_cast<i32>(out.size());
                note(line, "নাহলে ⇒ " + bn, "otherwise ⇒ " + en);
            } else {
                error(line, t[static_cast<std::size_t>(first)].column, "«নাহলে» এর ঠিক আগে একটা শর্ত থাকা দরকার (যেমন: মাটিতে থাকলে ...)।",
                      "\"else\" needs a condition right before it (for example: if on the ground ...).");
                skipChildren();
            }
            state = BlockState{};
            return;
        }

        // ---- "<condition> <actions>" ----
        const Header header = parseHeader(line, top, hasBlock);
        if (header.error) {
            reportNotUnderstood(line, t, failB_, std::min(failE_, n), fail_, true);
            skipChildren();
            state = BlockState{};
            return;
        }
        if (header.found) {
            // "<condition> A, নাহলে B" on one line.
            int elseAt = -1;
            if (header.trigger.isState()) {
                for (int k = header.ab; k < header.ae; ++k) {
                    if (t[static_cast<std::size_t>(k)].has(C::Else)) {
                        elseAt = k;
                        break;
                    }
                }
            }
            int thenEnd = elseAt >= 0 ? elseAt : header.ae;
            while (elseAt >= 0 && thenEnd > header.ab && t[static_cast<std::size_t>(thenEnd - 1)].kind != Tok::Word &&
                   t[static_cast<std::size_t>(thenEnd - 1)].kind != Tok::Number && t[static_cast<std::size_t>(thenEnd - 1)].kind != Tok::String) {
                --thenEnd;
            }
            int elseBegin = elseAt + 1;
            while (elseAt >= 0 && elseBegin < header.ae &&
                   (t[static_cast<std::size_t>(elseBegin)].kind == Tok::Comma || t[static_cast<std::size_t>(elseBegin)].has(C::Then))) {
                ++elseBegin;
            }
            const Children none = [](Code&) {};
            std::string elseBn, elseEn;

            if (top) {
                Code body;
                compileBody(line, t, header.ab, thenEnd, elseAt < 0 && hasBlock, elseAt < 0 ? children : none, body, bn, en);
                const int index = addRule(header.trigger, body, line.number);
                state = BlockState{};
                if (header.trigger.isState()) state.lastRule = index;
                if (elseAt >= 0) {
                    Trigger opposite = header.trigger;
                    opposite.negate = !opposite.negate;
                    Code elseBody;
                    compileBody(line, t, elseBegin, header.ae, hasBlock, children, elseBody, elseBn, elseEn);
                    addRule(opposite, elseBody, line.number);
                    state.lastRule = -1;
                    bn += "  |  নাহলে ⇒ " + elseBn;
                    en += "  |  otherwise ⇒ " + elseEn;
                }
                note(line, describe(header.trigger, true) + " ⇒ " + bn, describe(header.trigger, false) + " ⇒ " + en);
            } else if (header.trigger.isState()) {
                Instr test;
                test.op = Op::JumpIfNot;
                test.a = static_cast<i32>(program_.conditions.size());
                program_.conditions.push_back(header.trigger);
                out.push_back(test);
                const std::size_t testIndex = out.size() - 1;
                compileBody(line, t, header.ab, thenEnd, elseAt < 0 && hasBlock, elseAt < 0 ? children : none, out, bn, en);
                out[testIndex].b = static_cast<i32>(out.size());
                state = BlockState{};
                state.pendingIf = static_cast<int>(testIndex);
                if (elseAt >= 0) {
                    Instr jump;
                    jump.op = Op::Jump;
                    out.push_back(jump);
                    const std::size_t jumpIndex = out.size() - 1;
                    out[testIndex].b = static_cast<i32>(out.size());
                    compileBody(line, t, elseBegin, header.ae, hasBlock, children, out, elseBn, elseEn);
                    out[jumpIndex].a = static_cast<i32>(out.size());
                    state.pendingIf = -1;
                    bn += "  |  নাহলে ⇒ " + elseBn;
                    en += "  |  otherwise ⇒ " + elseEn;
                }
                // Inside a block a condition is checked once, when the script gets there: "if", not "while".
                std::string conditionEn = describe(header.trigger, false);
                if (conditionEn.rfind("while ", 0) == 0) conditionEn.erase(0, 6);
                note(line, describe(header.trigger, true) + " ⇒ " + bn, "if " + conditionEn + " ⇒ " + en);
            } else {
                error(line, t[static_cast<std::size_t>(first)].column,
                      "এই ধরনের ঘটনা («" + describe(header.trigger, true) + "») অন্য নিয়মের ভেতরে লেখা যায় না — একে আলাদা লাইনে, বাম ধার থেকে লিখুন।",
                      "This kind of event (\"" + describe(header.trigger, false) + "\") can't be nested inside another rule — write it on its own line, starting at the left edge.");
                skipChildren();
                state = BlockState{};
            }
            return;
        }

        // ---- plain actions ----
        state = BlockState{};
        if (top && foreverLine_ > 0) {
            // Lines outside any rule run one after another, and a "forever" never finishes.
            error(line, t[static_cast<std::size_t>(first)].column,
                  "এই লাইন কখনো চলবে না: উপরের «বারবার» (লাইন " + num(foreverLine_, true) +
                      ") কখনো শেষ হয় না। লাইনটা তার আগে লিখুন, অথবা শুরুতে «শুরুতে» যোগ করুন — তাহলে দুটো একসাথে চলবে।",
                  "This line would never run: the \"forever\" above (line " + num(foreverLine_, false) +
                      ") never ends. Move this line above it, or begin it with \"at the start\" so that both run together.");
            skipChildren();
            return;
        }
        if (top && startCode_.empty()) startLine_ = line.number;
        compileBody(line, t, 0, n, hasBlock, children, top ? startCode_ : out, bn, en);
        if (top && !startCode_.empty() && startCode_.back().op == Op::Jump && startCode_.back().a < static_cast<i32>(startCode_.size()) - 1) {
            foreverLine_ = line.number;
        }
        note(line, bn, en);
    }

    std::string_view source_;
    const CompileOptions& options_;
    std::vector<Line> lines_;
    std::unordered_set<std::string> known_;
    Program program_;
    Code startCode_;  // statements outside any rule run once at the start
    int startLine_ = 1;
    int foreverLine_ = 0;  // line of a top-level "forever": nothing written after it can run
    int repeatDepth_ = 0;
    int fail_ = -1;  // token that broke the last failed parse
    int failB_ = 0, failE_ = 0;
};

}  // namespace

// ------------------------------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------------------------------

std::string canonicalName(std::string_view name) {
    std::string key = matchKey(name);
    const auto& table = aliasTable();
    const auto it = table.find(key);
    return it != table.end() ? it->second : key;
}

Program compile(std::string_view source, const CompileOptions& options) {
    Compiler compiler(source, options);
    return compiler.run();
}

std::string describe(const Trigger& t, bool bn) {
    const std::string button = quoted(buttonText(t.button, bn), bn);
    switch (t.kind) {
        case TriggerKind::Start: return bn ? "শুরুতে" : "at the start";
        case TriggerKind::ButtonDown:
            if (t.negate) return bn ? button + " বোতাম চাপা না থাকলে" : "while " + button + " is not held";
            return bn ? button + " বোতাম চাপলে" : "while " + button + " is held";
        case TriggerKind::ButtonReleased: return bn ? button + " বোতাম ছাড়লে" : "when " + button + " is released";
        case TriggerKind::Every: return bn ? "প্রতি " + num(t.seconds, true) + " সেকেন্ডে" : "every " + num(t.seconds, false) + " s";
        case TriggerKind::Touch:
            if (t.name == "#wall") return bn ? "দেয়ালে ধাক্কা লাগলে" : "on hitting a wall";
            if (t.name == "#player") return bn ? "খেলোয়াড়কে ছুঁলে" : "on touching the player";
            if (t.name.empty()) return bn ? "কিছু ছুঁলে" : "on touching anything";
            return bn ? quoted(shown(t.label, t.name), true) + " ছুঁলে" : "on touching " + quoted(shown(t.label, t.name), false);
        case TriggerKind::Landed: return bn ? "মাটিতে নামলে" : "on landing";
        case TriggerKind::Message:
            return bn ? quoted(shown(t.label, t.name), true) + " বার্তা পেলে" : "on message " + quoted(shown(t.label, t.name), false);
        case TriggerKind::OnGround:
            if (t.negate) return bn ? "শূন্যে থাকলে" : "while in the air";
            return bn ? "মাটিতে থাকলে" : "while on the ground";
        case TriggerKind::Compare: {
            const CmpOp op = t.negate ? invert(t.cmp) : t.cmp;
            if (t.prop == Prop::PlayerDistance) {
                const bool near = op == CmpOp::Less || op == CmpOp::LessEqual;
                if (bn) return "খেলোয়াড় " + num(t.value, true) + (near ? " মিটারের মধ্যে থাকলে" : " মিটারের বেশি দূরে থাকলে");
                return std::string("while the player is ") + (near ? "within " : "more than ") + num(t.value, false) + " m" + (near ? "" : " away");
            }
            const std::string lhs = t.prop != Prop::None ? std::string(propText(t.prop, bn)) : quoted(shown(t.label, t.name), bn);
            return bn ? lhs + " " + cmpText(op) + " " + num(t.value, true) + " হলে" : "while " + lhs + " " + cmpText(op) + " " + num(t.value, false);
        }
    }
    return {};
}

std::string describe(const Action& a, bool bn) {
    switch (a.kind) {
        case ActionKind::Move: {
            std::string out = dirText(a.dir, bn);
            const bool vertical = a.dir == Dir::Up || a.dir == Dir::Down;
            if (bn) {
                if (a.scale > 1.01f) out += " দ্রুত";
                if (a.scale < 0.99f) out += " ধীরে";
                out += vertical ? " যাও" : a.run ? " দৌড়াও" : " হাঁটো";
                if (a.seconds > 0) out += ", " + num(a.seconds, true) + " সেকেন্ড";
                if (a.meters > 0) out += ", " + num(a.meters, true) + " মিটার";
                return out;
            }
            out = std::string(vertical ? "go " : a.run ? "run " : "walk ") + out;
            if (a.scale > 1.01f) out += " fast";
            if (a.scale < 0.99f) out += " slowly";
            if (a.seconds > 0) out += " for " + num(a.seconds, false) + " s";
            if (a.meters > 0) out += " for " + num(a.meters, false) + " m";
            return out;
        }
        case ActionKind::Stop: return bn ? "থামো" : "stop";
        case ActionKind::Jump: {
            const std::string dir = a.dir != Dir::None ? std::string(dirText(a.dir, bn)) + " " : std::string();
            if (bn) return dir + (a.scale > 1.01f ? "উঁচু লাফ দাও" : a.scale < 0.99f ? "ছোট লাফ দাও" : "লাফ দাও");
            std::string out = a.scale > 1.01f ? "jump high" : a.scale < 0.99f ? "small jump" : "jump";
            if (a.dir != Dir::None) out += std::string(" ") + dirText(a.dir, false);
            return out;
        }
        case ActionKind::Flip: return bn ? "উল্টো দিকে ঘোরো" : "turn around";
        case ActionKind::Face: return bn ? std::string(dirText(a.dir, true)) + " মুখ করো" : std::string("face ") + dirText(a.dir, false);
        case ActionKind::SetProp: return numOpText(propText(a.prop, true), propText(a.prop, false), a.op, a.value, bn);
        case ActionKind::SetVar: {
            const std::string name = quoted(shown(a.label, a.text), bn);
            return numOpText(name.c_str(), name.c_str(), a.op, a.value, bn);
        }
        case ActionKind::LightSwitch:
            if (a.value != 0.0) return bn ? "আলো জ্বালাও" : "turn the light on";
            return bn ? "আলো নেভাও" : "turn the light off";
        case ActionKind::LightColor: {
            const std::string colour = colorText(static_cast<u32>(a.value), bn);
            return bn ? "আলোর রং " + colour + " করো" : "make the light " + colour;
        }
        case ActionKind::Say: {
            const bool timed = std::fabs(a.seconds - kSayDefaultSeconds) > 1e-3f;  // only worth mentioning when it was asked for
            if (bn) return quoted(a.text, true) + " বলো" + (timed ? ", " + num(a.seconds, true) + " সেকেন্ড" : "");
            return "say " + quoted(a.text, false) + (timed ? " for " + num(a.seconds, false) + " s" : "");
        }
        case ActionKind::Send:
            return bn ? quoted(shown(a.label, a.text), true) + " বার্তা পাঠাও" : "send the message " + quoted(shown(a.label, a.text), false);
        case ActionKind::Destroy:
            if (a.text.empty()) return bn ? "নিজে মুছে যাও" : "remove myself";
            return bn ? quoted(shown(a.label, a.text), true) + " মুছে দাও" : "remove " + quoted(shown(a.label, a.text), false);
        case ActionKind::Respawn: return bn ? "শুরুর জায়গায় ফিরে যাও" : "go back to the start";
        case ActionKind::Shake: return bn ? "ক্যামেরা কাঁপাও" : "shake the camera";
        case ActionKind::Game:
            switch (static_cast<GameEvent>(static_cast<int>(a.value))) {
                case GameEvent::Win: return bn ? "খেলা জিতে যাও" : "win the game";
                case GameEvent::Lose: return bn ? "খেলা হেরে যাও" : "lose the game";
                case GameEvent::Restart: return bn ? "খেলা আবার শুরু করো" : "restart the game";
            }
            break;
    }
    return {};
}

std::string toJson(const Program& program) {
    std::string out = "{\"ok\":";
    out += program.ok() ? "true" : "false";
    out += ",\"rules\":" + std::to_string(program.rules.size());
    out += ",\"notes\":[";
    for (std::size_t i = 0; i < program.notes.size(); ++i) {
        const Note& n = program.notes[i];
        if (i > 0) out += ',';
        out += "{\"line\":" + std::to_string(n.line) + ",\"bn\":";
        jsonString(out, n.bn);
        out += ",\"en\":";
        jsonString(out, n.en);
        out += '}';
    }
    out += "],\"errors\":[";
    for (std::size_t i = 0; i < program.errors.size(); ++i) {
        const Diagnostic& d = program.errors[i];
        if (i > 0) out += ',';
        out += "{\"line\":" + std::to_string(d.line) + ",\"col\":" + std::to_string(d.column) + ",\"bn\":";
        jsonString(out, d.bn);
        out += ",\"en\":";
        jsonString(out, d.en);
        out += '}';
    }
    out += "]}";
    return out;
}

}  // namespace ge::kotha
