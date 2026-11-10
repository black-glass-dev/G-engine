// G-Engine — script/lexicon.h
// The vocabulary of the "Kotha" (কথা) behaviour language.
//
// Every word a script may use (Bengali first, English second) maps to one or more *concepts*.
// The parser never looks at spelling again after this step, so Bengali and English — and any
// mix of the two — are understood by exactly the same rules.
#pragma once

#include <string>
#include <string_view>

#include "core/base.h"

namespace ge::kotha {

enum class Concept : u8 {
    None = 0,
    Filler,  // a word that carries no meaning for us ("the", "তুমি", "please", ...)

    // --- sentence structure ---
    When,      // যখন / when
    If,        // যদি / if
    Then,      // তখন, তাহলে, তারপর / then
    Else,      // নাহলে / else, otherwise
    End,       // শেষ / end
    Forever,   // বারবার, সবসময় / forever, always
    Every,     // প্রতি / every
    And,       // এবং, আর / and
    Or,        // অথবা, বা / or
    Not,       // না, নয় / not
    After,     // পরে / after, later

    // --- events and states ---
    Start,     // শুরু / start
    Button,    // বোতাম / button, key
    Press,     // চাপ- / press
    Release,   // ছাড়- / release
    Touch,     // ছোঁ-, লাগ-, ধাক্কা / touch, hit
    Ground,    // মাটি / ground
    Air,       // শূন্যে, বাতাসে / air
    Wall,      // দেয়াল / wall
    Land,      // নাম- / land
    Fall,      // পড়- / fall
    Receive,   // পা- / receive
    Message,   // বার্তা / message
    Send,      // পাঠা- / send

    // --- movement ---
    Walk,      // হাঁট- / walk
    Run,       // দৌড়- / run
    Go,        // যা-, চল- / go, move
    Jump,      // লাফ / jump
    Stop,      // থাম-, দাঁড়া- / stop
    Turn,      // ঘোর- / turn
    Face,      // তাকা- / face, look
    Return,    // ফির- / return
    Wait,      // অপেক্ষা / wait

    // --- directions ---
    Right,     // ডান
    Left,      // বাম
    Up,        // উপর
    Down,      // নিচ
    Forward,   // সামনে
    Backward,  // পিছনে
    Opposite,  // উল্টো

    // --- units ---
    Second,    // সেকেন্ড
    Minute,    // মিনিট
    Meter,     // মিটার
    Step,      // ধাপ, পা
    Times,     // বার

    // --- properties and arithmetic ---
    Speed,     // গতি
    Power,     // জোর, শক্তি
    Height,    // উচ্চতা
    Gravity,   // মাধ্যাকর্ষণ
    Size,      // আকার
    Increase,  // বাড়া- / increase, add
    Decrease,  // কমা- / decrease, subtract
    More,      // বেশি, বড় / more, greater, big
    Less,      // কম, ছোট / less, smaller, small
    Equal,     // সমান / equal
    Fast,      // দ্রুত, জোরে / fast
    Slow,      // ধীরে, আস্তে / slow
    High,      // উঁচু / high

    // --- light ---
    Light,       // আলো, বাতি
    On,          // জ্বালা-, চালু
    Off,         // নেভা-, বন্ধ
    Color,       // রং
    ColorName,   // লাল, নীল, ...   (value = 0xRRGGBB)
    Bright,      // উজ্জ্বল
    Dim,         // ম্লান
    Brightness,  // উজ্জ্বলতা

    // --- other actions ---
    Say,       // বলো
    Destroy,   // মুছে যাও, মরে যাও
    Win,       // জিত-
    Lose,      // হার-
    Again,     // আবার
    Shake,     // কাঁপা-
    Camera,    // ক্যামেরা

    // --- who ---
    Self,      // আমি, নিজে
    Player,    // খেলোয়াড়
    Anything,  // কিছু

    // --- helper verbs (usually ignorable) ---
    Do,        // করো
    Give,      // দাও
    Keep,      // রাখো
    Be,        // হয়, হলে
    Stay,      // থাকো, থাকলে
    Change,    // বদলাও

    // --- named buttons ---
    ButtonAction,  // অ্যাকশন / action, fire
    KeySpace,      // স্পেস / space

    // --- distance ---
    Near,      // কাছে / near, within
    Far,       // দূরে / far, away

    Count
};

/// One meaning of a word.
struct Sense {
    Concept meaning = Concept::None;
    bool conditional = false;  // Bengali "-লে" verb form ("চাপলে" = "when pressed"): ends a condition
    i32 value = 0;             // payload, e.g. the RGB value of a colour name
};

/// All meanings of a word (most words have exactly one).
struct WordInfo {
    static constexpr int kMaxSenses = 3;
    Sense senses[kMaxSenses];
    u8 count = 0;

    const Sense* sense(Concept c) const {
        for (int i = 0; i < count; ++i) {
            if (senses[i].meaning == c) return &senses[i];
        }
        return nullptr;
    }
    bool has(Concept c) const { return sense(c) != nullptr; }
    bool conditional() const {
        for (int i = 0; i < count; ++i) {
            if (senses[i].conditional) return true;
        }
        return false;
    }
};

/// How a number word combines with its neighbours.
enum class NumberKind : u8 {
    Plain,       // এক, two, দেড়, half ...
    AddHalf,     // সাড়ে N  ->  N + 0.5
    SubQuarter,  // পৌনে N  ->  N - 0.25
    AddQuarter,  // সোয়া N  ->  N + 0.25
    Tens,        // twenty, thirty ... (can be followed by a unit digit word in English)
    CountTimes,  // once, twice, thrice (number + "times" in one word)
};

class Lexicon {
public:
    /// The built-in vocabulary (built on first use).
    static const Lexicon& get();

    /// Exact lookup of a normalised key (see matchKey). nullptr if unknown.
    const WordInfo* find(std::string_view key) const;

    /// Lookup that also understands simple inflection: Bengali case endings
    /// (মাটি*তে*, বোতাম*টা*, শত্রু*কে*) and English suffixes (walk*s*, walk*ing*).
    const WordInfo* findInflected(std::string_view key) const;

    /// Number words: "তিন", "three", "দেড়", "সাড়ে" ...
    bool findNumber(std::string_view key, double& value, NumberKind& kind) const;

    /// Closest known word for "did you mean ...?" — empty when nothing is close enough.
    std::string suggest(std::string_view key) const;

    /// Number of distinct word forms known (for tests and statistics).
    std::size_t wordCount() const;

    /// Words that map to more than one meaning, as "word: A, B" lines (used by tests to
    /// catch accidental collisions when the vocabulary grows).
    std::string describeAmbiguousWords() const;

private:
    Lexicon();
    struct Impl;
    Impl* impl_;
};

const char* conceptName(Concept c);

}  // namespace ge::kotha
