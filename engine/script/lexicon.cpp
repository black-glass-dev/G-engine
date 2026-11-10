// G-Engine — script/lexicon.cpp
// Built-in vocabulary of the Kotha (কথা) language. Bengali first, English second.
//
// HOW TO ADD WORDS
//   * A plain word (noun, adjective, particle): add it to the matching row of kWords.
//   * A Bengali verb: add its stem(s) to kVerbs — all the usual endings
//     (হাঁটো, হাঁটবে, হাঁটলে, হেঁটে, হাঁটতে ...) are generated automatically.
//   * Spelling variants do not need separate entries when they only differ by chandrabindu,
//     ি/ী, ু/ূ or ণ/ন (see matchKey in text.cpp).
//   * Run the tests afterwards: they fail if a word accidentally lands in two concepts.
#include "script/lexicon.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "core/log.h"
#include "core/utf8.h"
#include "script/text.h"

namespace ge::kotha {

namespace {

using C = Concept;

struct WordRow {
    Concept meaning;
    i32 value;
    const char* bn;  // space separated
    const char* en;  // space separated
};

// clang-format off
const WordRow kWords[] = {
    // ---- sentence structure -------------------------------------------------------------
    {C::When,     0, "যখন যখনই যেই", "when whenever while"},
    {C::If,       0, "যদি", "if"},
    {C::Then,     0, "তখন তাহলে তবে তারপর তারপরে এরপর এরপরে অতঃপর", "then next afterwards"},
    {C::Else,     0, "নাহলে নইলে নতুবা অন্যথায় নয়তো", "else otherwise"},
    {C::End,      0, "শেষ সমাপ্ত", "end done endif over"},
    {C::Forever,  0, "বারবার সবসময় সর্বদা চিরকাল অনবরত সারাক্ষণ", "forever always repeatedly continuously loop repeat"},
    {C::Every,    0, "প্রতি প্রত্যেক", "every each"},
    {C::And,      0, "এবং আর ও", "and"},
    {C::Or,       0, "অথবা বা কিংবা", "or"},
    {C::Not,      0, "না নয় নেই নি নহে", "not no isnt dont doesnt never arent"},
    {C::After,    0, "পর পরে", "after later"},

    // ---- events and states --------------------------------------------------------------
    {C::Start,    0, "শুরু শুরুতে শুরুতেই আরম্ভ প্রথমে", "start starts started begin begins beginning"},
    {C::Button,   0, "বোতাম বাটন বাটোন", "button key btn"},
    {C::Press,    0, "ক্লিক ট্যাপ", "press presses pressed pressing tap taps tapped click clicks clicked hold holds held holding"},
    {C::Release,  0, "", "release releases released releasing"},
    {C::Touch,    0, "ছোঁয় ছোঁও ছোঁবে ছোঁয়া ছুঁয়ে ছুঁই ছুঁক ছুঁতে স্পর্শ ধাক্কা সংঘর্ষ নাও নিন নেবে নিয়ে পৌঁছাও পৌঁছে পৌঁছায়",
                     "touch touches touched touching hit hits hitting collide collides collided colliding bump bumps bumped "
                     "reach reaches reached collect collects collected take takes"},
    {C::Ground,   0, "মাটি ভূমি মেঝে জমি জমিন", "ground floor grounded"},
    {C::Air,      0, "শূন্যে বাতাস হাওয়া আকাশ", "air airborne midair"},
    {C::Wall,     0, "দেয়াল দেওয়াল প্রাচীর", "wall walls"},
    {C::Land,     0, "অবতরণ", "land lands landed landing"},
    {C::Fall,     0, "", "fall falls falling fell"},
    {C::Receive,  0, "পাও পায় পাবে পেয়ে পাই পাক পেতে পাওয়া", "receive receives received get gets got arrive arrives arrived"},
    {C::Message,  0, "বার্তা সংকেত মেসেজ খবর সিগন্যাল", "message signal"},
    {C::Send,     0, "", "send sends sent broadcast broadcasts emit"},

    // ---- movement -----------------------------------------------------------------------
    {C::Walk,     0, "", "walk walks walked walking stroll"},
    {C::Run,      0, "", "run runs ran running sprint sprints dash rush"},
    {C::Go,       0, "যাও যাবে যান যাক যায় যাই যেতে যাচ্ছে গিয়ে যাওয়া যাবো যাব এগোও এগিয়ে এগোবে এগোতে এগোক আগাও আগাবে আগিয়ে "
                     "আসো এসো আসে আসবে এসে আসুক",
                     "go goes going went move moves moved moving head advance come comes coming approach approaches"},
    {C::Jump,     0, "ঝাঁপ লম্ফ", "jump jumps jumped jumping hop hops leap leaps"},
    {C::Stop,     0, "স্থির", "stop stops stopped stopping halt halts stand stands stay freeze"},
    {C::Turn,     0, "", "turn turns turned turning flip flips"},
    {C::Face,     0, "মুখ", "face faces facing look looks looking"},
    {C::Return,   0, "ফেরত", "return returns returned respawn respawns"},
    {C::Wait,     0, "অপেক্ষা বিরতি", "wait waits pause pauses sleep delay rest"},

    // ---- directions ---------------------------------------------------------------------
    {C::Right,    0, "ডান ডানদিক ডানপাশ ডাইনে", "right rightward rightwards"},
    {C::Left,     0, "বাম বাঁয়ে বামদিক বামপাশ বাঁদিক বাঁপাশ", "left leftward leftwards"},
    {C::Up,       0, "উপর ওপর ঊর্ধ্ব", "up upward upwards"},
    {C::Down,     0, "নিচ নিচের", "down downward downwards"},
    {C::Forward,  0, "সামনে সামনের সম্মুখে", "forward forwards ahead"},
    {C::Backward, 0, "পিছনে পেছনে পিছে পেছন পিছন পিছু পিছনের", "backward backwards back"},
    {C::Opposite, 0, "উল্টো উল্টা উলটো উল্টে বিপরীত", "opposite reverse around"},

    // ---- units --------------------------------------------------------------------------
    {C::Second,   0, "সেকেন্ড সেকেণ্ড", "second seconds sec secs s"},
    {C::Minute,   0, "মিনিট", "minute minutes min mins"},
    {C::Meter,    0, "মিটার ঘর টাইল ব্লক", "meter meters metre metres m tile tiles block blocks unit units"},
    {C::Step,     0, "ধাপ পা কদম", "step steps"},
    {C::Times,    0, "বার", "times time"},

    // ---- properties and arithmetic ------------------------------------------------------
    {C::Speed,    0, "গতি বেগ স্পিড গতিবেগ", "speed velocity pace"},
    {C::Power,    0, "জোর শক্তি ক্ষমতা", "power strength force"},
    {C::Height,   0, "উচ্চতা", "height"},
    {C::Gravity,  0, "মাধ্যাকর্ষণ অভিকর্ষ গ্র্যাভিটি", "gravity"},
    {C::Size,     0, "আকার সাইজ মাপ", "size scale"},
    {C::Increase, 0, "যোগ", "increase increases increased add adds added raise raises plus"},
    {C::Decrease, 0, "বিয়োগ বাদ", "decrease decreases decreased subtract subtracts reduce reduces minus"},
    {C::More,     0, "বেশি বড় বড়ো অধিক বিশাল", "more greater bigger larger big large huge above over"},
    {C::Less,     0, "কম ছোট ছোটো অল্প", "less fewer smaller small little below under"},
    {C::Equal,    0, "সমান", "equal equals same"},
    {C::Fast,     0, "দ্রুত জোরে তাড়াতাড়ি", "fast faster quick quickly"},
    {C::Slow,     0, "ধীরে আস্তে ধীর ধীরগতিতে", "slow slower slowly"},
    {C::High,     0, "উঁচু উঁচুতে উচ্চ", "high higher"},

    // ---- light --------------------------------------------------------------------------
    {C::Light,      0, "আলো বাতি লাইট মশাল লণ্ঠন প্রদীপ টর্চ", "light lights lamp torch lantern"},
    {C::On,         0, "চালু অন", "on enable enabled"},
    {C::Off,        0, "বন্ধ অফ", "off disable disabled extinguish"},
    {C::Color,      0, "রং রঙ কালার বর্ণ", "color colour"},
    {C::ColorName,  0xFF3B30, "লাল", "red"},
    {C::ColorName,  0x34C759, "সবুজ", "green"},
    {C::ColorName,  0x2F80FF, "নীল", "blue"},
    {C::ColorName,  0xFFD60A, "হলুদ", "yellow"},
    {C::ColorName,  0xFFFFFF, "সাদা", "white"},
    {C::ColorName,  0xFF8A1E, "কমলা", "orange"},
    {C::ColorName,  0xA855F7, "বেগুনি", "purple violet"},
    {C::ColorName,  0xFF5FA2, "গোলাপি", "pink"},
    {C::ColorName,  0x32D4F5, "আকাশি", "cyan"},
    {C::ColorName,  0xFFC14D, "সোনালি", "gold golden"},
    {C::Bright,     0, "উজ্জ্বল তীব্র জোরালো", "bright brighter strong"},
    {C::Dim,        0, "ম্লান মৃদু হালকা অনুজ্জ্বল", "dim dimmer faint soft"},
    {C::Brightness, 0, "উজ্জ্বলতা তীব্রতা", "brightness intensity"},

    // ---- other actions ------------------------------------------------------------------
    {C::Say,      0, "লেখো লিখো", "say says speak tell print write"},
    {C::Destroy,  0, "ধ্বংস নষ্ট উধাও অদৃশ্য", "destroy destroys delete remove die dies vanish disappear"},
    {C::Win,      0, "জয় বিজয়ী", "win wins won victory"},
    {C::Lose,     0, "পরাজয়", "lose loses lost"},
    {C::Again,    0, "আবার পুনরায় রিস্টার্ট", "again restart retry"},
    {C::Shake,    0, "ঝাঁকাও ঝাঁকুনি", "shake shakes"},
    {C::Camera,   0, "ক্যামেরা পর্দা স্ক্রিন", "camera screen"},

    // ---- who ----------------------------------------------------------------------------
    {C::Self,     0, "আমি আমার আমাকে নিজে নিজেকে নিজের", "i me my myself self"},
    {C::Player,   0, "খেলোয়াড় প্লেয়ার নায়ক", "player hero"},
    {C::Anything, 0, "কিছু যেকোনো", "anything something"},

    // ---- helper verbs -------------------------------------------------------------------
    {C::Do,       0, "", "do does did make makes set sets"},
    {C::Give,     0, "দাও দিন দেবে দিয়ে দেয় দিই দিতে দেওয়া দেয়া দিবে", "give gives"},
    {C::Keep,     0, "", "keep keeps"},
    {C::Be,       0, "হও হয় হবে হয়ে হোক হচ্ছে হলো হল হই আছে আছো আছি", "is are am be been being becomes become was were"},
    {C::Stay,     0, "", "remain remains"},
    {C::Change,   0, "বদলাও বদলে বদলাবে পাল্টাও পাল্টে পরিবর্তন", "change changes switch"},

    // ---- named buttons ------------------------------------------------------------------
    {C::ButtonAction, 0, "অ্যাকশন একশন ফায়ার গুলি আক্রমণ", "action fire attack shoot"},
    {C::KeySpace,     0, "স্পেস", "space spacebar"},

    // ---- distance -----------------------------------------------------------------------
    {C::Near,     0, "কাছে কাছাকাছি নিকটে", "near close nearby within"},
    {C::Far,      0, "দূরে দূর", "far away"},

    // ---- words that carry no meaning for us ----------------------------------------------
    {C::Filler,   0,
     "তুমি তুই আপনি তোমার তোর আপনার সে তার তাকে ওর ওকে ও এটা এটি ওটা সেটা এই ঐ ওই টা টি একটা একটি একটু "
     "দয়া প্লিজ অনুগ্রহ এখন এখনই সাথে সঙ্গে থেকে এর কে তে তো ই চরিত্র ক্যারেক্টার খেলা গেম জায়গা স্থান "
     "দিকে দিক পাশে পাশ ধারে ধরে জন্য যাবৎ পর্যন্ত চেয়ে মধ্যে ভিতরে অবস্থায় সময় হিসেবে টাকে টিকে খুব",
     "the a an to of it its this that these those please now with by at in into onto from as so just also "
     "you your he she his her they their them there here will would should shall must can could may let lets "
     "has have had some very really for during than toward towards side direction character sprite object game on"},
};

// Bengali verbs. Kind 'C' = consonant-final stem (হাঁট, চাপ, থাম ...): several stems may be
// listed because the vowel changes in some forms (হাঁট / হেঁট, থাম / থেম).
// Kind 'A' = "-আ" verbs written WITHOUT the final আ-কার (দৌড় -> দৌড়াও, দৌড়াবে, দৌড়িয়ে, দৌড়ে).
// Kind 'a' = like 'A' but without the short forms (stem + ে/ো/ুক/বে/লে) because those would be
//            different everyday words (তাক + ে = তাকে "him/her").
struct VerbRow {
    Concept meaning;
    char kind;
    const char* stems;
    bool allowBare;  // is the bare stem itself a form? (no when it is a different common word)
};

const VerbRow kVerbs[] = {
    {C::Walk,     'C', "হাঁট হেঁট", true},
    {C::Run,      'A', "দৌড়", true},
    {C::Run,      'C', "ছুট", false},
    {C::Go,       'C', "চল", true},
    {C::Go,       'C', "সর", false},      // সর = cream
    {C::Jump,     'a', "লাফ", true},
    {C::Stop,     'C', "থাম থেম", true},
    {C::Stop,     'a', "দাঁড়", false},
    {C::Turn,     'C', "ঘুর", true},
    {C::Turn,     'C', "ঘোর", false},     // ঘোর = deep, intense
    {C::Face,     'a', "তাক", false},   // (তাকে = him/her must stay free)
    {C::Return,   'C', "ফির ফের", false}, // ফের = again
    {C::Press,    'C', "চাপ চেপ", true},
    {C::Press,    'C', "টিপ টেপ", false},
    {C::Press,    'C', "ধর", true},
    {C::Release,  'C', "ছাড় ছেড়", true},
    {C::Touch,    'C', "লাগ লেগ", true},
    {C::Touch,    'C', "ঠেক", true},
    {C::Land,     'C', "নাম নেম", false},  // নাম = name
    {C::Fall,     'C', "পড়", true},
    {C::Send,     'a', "পাঠ", false},     // পাঠ = lesson
    {C::Increase, 'A', "বাড়", false},     // (বাড়ি = house must stay free)
    {C::Decrease, 'A', "কম", false},      // কম = less
    {C::On,       'a', "জ্বাল", false},
    {C::On,       'C', "জ্বল", false},
    {C::Off,      'A', "নেভ", false},
    {C::Off,      'A', "নিভ", false},
    {C::Say,      'C', "বল", false},      // বল = ball
    {C::Destroy,  'C', "মুছ মোছ", true},
    {C::Destroy,  'C', "মর", true},
    {C::Win,      'C', "জিত জেত", true},
    {C::Lose,     'C', "হার হের", false},  // হার = necklace
    {C::Shake,    'A', "কাঁপ", false},
    {C::Do,       'C', "কর", true},
    {C::Keep,     'C', "রাখ রেখ", true},
    {C::Stay,     'C', "থাক থেক", true},
};

// Individual conditional forms ("-লে": "when/if it ...") of irregular verbs.
struct CondRow {
    Concept meaning;
    const char* words;
};
const CondRow kConditionalWords[] = {
    {C::Touch,   "ছুঁলে নিলে পৌঁছালে পৌঁছলে"},
    {C::Receive, "পেলে"},
    {C::Go,      "গেলে এগোলে এলে আসলে"},
    {C::Give,    "দিলে"},
    {C::Be,      "হলে হলেই"},
};

struct Ending {
    const char* text;
    bool conditional;
};

// Endings for consonant-final stems: হাঁট + ো = হাঁটো, হাঁট + লে = হাঁটলে ...
const Ending kConsonantEndings[] = {
    {"ো", false},  {"ুন", false},  {"ুক", false},  {"ে", false},   {"ি", false},   {"িস", false},  {"বে", false},
    {"বো", false}, {"ব", false},   {"বেন", false}, {"বি", false},  {"ছে", false},  {"ছি", false},  {"ছো", false},
    {"ছ", false},  {"ছেন", false}, {"তে", false},  {"া", false},   {"ার", false},  {"লে", true},   {"লেই", true},
    {"ল", false},  {"লো", false},  {"লাম", false}, {"লেন", false}, {"েছে", false}, {"েছি", false}, {"েছো", false},
    // causative ("make it ..."): থামাও, ঘোরাও, সরাও
    {"াও", false}, {"াবে", false}, {"িয়ে", false}, {"ালে", true},  {"ানো", false}, {"ায়", false},  {"াতে", false},
    {"াক", false}, {"াই", false},
};

// Endings for "-আ" verbs, attached after the আ-কার: দৌড় + া + ও = দৌড়াও ...
const Ending kAEndings[] = {
    {"", false},    {"ও", false},   {"বে", false},   {"বো", false},  {"ব", false},  {"বেন", false}, {"ন", false},
    {"ক", false},   {"য়", false},   {"ই", false},    {"চ্ছে", false}, {"চ্ছি", false}, {"চ্ছো", false}, {"তে", false},
    {"নো", false},  {"নোর", false}, {"লে", true},    {"লেই", true},  {"লো", false}, {"ল", false},   {"লাম", false},
};
// Short forms of "-আ" verbs attached directly to the stem (kind 'A' only): দৌড় + ে = দৌড়ে
const Ending kAStemEndings[] = {
    {"ে", false}, {"ো", false}, {"ুক", false}, {"বে", false}, {"লে", true},
};

struct NumberRow {
    const char* words;
    double value;
    NumberKind kind;
};

const NumberRow kNumbers[] = {
    {"শূন্য zero", 0, NumberKind::Plain},
    {"এক one", 1, NumberKind::Plain},
    {"দুই দু two couple", 2, NumberKind::Plain},
    {"তিন three", 3, NumberKind::Plain},
    {"চার four", 4, NumberKind::Plain},
    {"পাঁচ five", 5, NumberKind::Plain},
    {"ছয় six", 6, NumberKind::Plain},
    {"সাত seven", 7, NumberKind::Plain},
    {"আট eight", 8, NumberKind::Plain},
    {"নয় nine", 9, NumberKind::Plain},  // "নয়" also means "not": the tokenizer decides from context
    {"দশ ten", 10, NumberKind::Plain},
    {"এগারো eleven", 11, NumberKind::Plain},
    {"বারো twelve", 12, NumberKind::Plain},
    {"তেরো thirteen", 13, NumberKind::Plain},
    {"চৌদ্দ fourteen", 14, NumberKind::Plain},
    {"পনেরো fifteen", 15, NumberKind::Plain},
    {"ষোলো sixteen", 16, NumberKind::Plain},
    {"সতেরো seventeen", 17, NumberKind::Plain},
    {"আঠারো eighteen", 18, NumberKind::Plain},
    {"উনিশ nineteen", 19, NumberKind::Plain},
    {"বিশ কুড়ি", 20, NumberKind::Plain},
    {"পঁচিশ", 25, NumberKind::Plain},
    {"ত্রিশ তিরিশ", 30, NumberKind::Plain},
    {"চল্লিশ", 40, NumberKind::Plain},
    {"পঞ্চাশ", 50, NumberKind::Plain},
    {"ষাট", 60, NumberKind::Plain},
    {"সত্তর", 70, NumberKind::Plain},
    {"আশি", 80, NumberKind::Plain},
    {"নব্বই", 90, NumberKind::Plain},
    {"একশ একশো শত hundred", 100, NumberKind::Plain},
    {"হাজার thousand", 1000, NumberKind::Plain},
    {"twenty", 20, NumberKind::Tens},
    {"thirty", 30, NumberKind::Tens},
    {"forty", 40, NumberKind::Tens},
    {"fifty", 50, NumberKind::Tens},
    {"sixty", 60, NumberKind::Tens},
    {"seventy", 70, NumberKind::Tens},
    {"eighty", 80, NumberKind::Tens},
    {"ninety", 90, NumberKind::Tens},
    {"আধা আধ অর্ধেক half", 0.5, NumberKind::Plain},
    {"quarter", 0.25, NumberKind::Plain},
    {"দেড়", 1.5, NumberKind::Plain},
    {"আড়াই", 2.5, NumberKind::Plain},
    {"সাড়ে", 0.5, NumberKind::AddHalf},
    {"পৌনে", 0.25, NumberKind::SubQuarter},
    {"সোয়া", 0.25, NumberKind::AddQuarter},
    {"একবার once", 1, NumberKind::CountTimes},
    {"দুবার দুইবার twice", 2, NumberKind::CountTimes},
    {"তিনবার thrice", 3, NumberKind::CountTimes},
};
// clang-format on

template <class Fn>
void forEachWord(const char* list, Fn&& fn) {
    if (list == nullptr) return;
    const char* p = list;
    while (*p != '\0') {
        while (*p == ' ') ++p;
        const char* start = p;
        while (*p != '\0' && *p != ' ') ++p;
        if (p > start) fn(std::string_view(start, static_cast<std::size_t>(p - start)));
    }
}

bool endsWith(std::string_view s, std::string_view suffix) {
    return s.size() > suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

bool isBengaliKey(std::string_view key) { return !key.empty() && static_cast<u8>(key[0]) >= 0x80; }

}  // namespace

struct Lexicon::Impl {
    struct Entry {
        WordInfo info;
        std::string display;  // a nicely spelled form for suggestions
    };
    struct Number {
        double value;
        NumberKind kind;
    };

    std::unordered_map<std::string, Entry> words;
    std::unordered_map<std::string, Number> numbers;

    void add(Concept meaning, std::string_view word, bool conditional, i32 value) {
        std::string key = matchKey(word);
        if (key.empty()) return;
        Entry& e = words[key];
        if (e.display.empty()) e.display = std::string(word);

        for (int i = 0; i < e.info.count; ++i) {
            Sense& s = e.info.senses[i];
            if (s.meaning == meaning) {
                s.conditional = s.conditional || conditional;
                return;
            }
        }
        if (e.info.count >= WordInfo::kMaxSenses) {
            GE_LOG_WARN("kotha lexicon: too many meanings for '%s'", key.c_str());
            return;
        }
        e.info.senses[e.info.count++] = Sense{meaning, conditional, value};
    }
};

Lexicon::Lexicon() : impl_(new Impl()) {
    for (const WordRow& row : kWords) {
        forEachWord(row.bn, [&](std::string_view w) { impl_->add(row.meaning, w, false, row.value); });
        forEachWord(row.en, [&](std::string_view w) { impl_->add(row.meaning, w, false, row.value); });
    }

    for (const VerbRow& row : kVerbs) {
        forEachWord(row.stems, [&](std::string_view stem) {
            const std::string base(stem);
            if (row.allowBare) impl_->add(row.meaning, base, false, 0);
            if (row.kind == 'C') {
                for (const Ending& e : kConsonantEndings) impl_->add(row.meaning, base + e.text, e.conditional, 0);
            } else {
                const std::string withA = base + "া";
                for (const Ending& e : kAEndings) impl_->add(row.meaning, withA + e.text, e.conditional, 0);
                impl_->add(row.meaning, base + "িয়ে", false, 0);  // দৌড়িয়ে, লাফিয়ে, তাকিয়ে
                if (row.kind == 'A') {
                    for (const Ending& e : kAStemEndings) impl_->add(row.meaning, base + e.text, e.conditional, 0);
                }
            }
        });
    }

    for (const CondRow& row : kConditionalWords) {
        forEachWord(row.words, [&](std::string_view w) { impl_->add(row.meaning, w, true, 0); });
    }

    for (const NumberRow& row : kNumbers) {
        forEachWord(row.words, [&](std::string_view w) { impl_->numbers[matchKey(w)] = Impl::Number{row.value, row.kind}; });
    }
}

const Lexicon& Lexicon::get() {
    static const Lexicon* instance = new Lexicon();  // intentionally never destroyed
    return *instance;
}

const WordInfo* Lexicon::find(std::string_view key) const {
    auto it = impl_->words.find(std::string(key));
    return it == impl_->words.end() ? nullptr : &it->second.info;
}

const WordInfo* Lexicon::findInflected(std::string_view key) const {
    if (const WordInfo* exact = find(key)) return exact;

    if (isBengaliKey(key)) {
        // Case endings and articles, longest first, normalised the same way as the keys.
        static const std::vector<std::string>* const kSuffixes = [] {
            static const char* const kRaw[] = {
                "গুলোকে", "গুলোর", "গুলো", "গুলি", "টাকে", "টিকে", "টার", "টির", "টায়", "টাতে", "দের", "য়ের",
                "ের",     "এর",    "কে",   "টা",   "টি",   "রা",   "তে",  "য়",   "র",   "ে",   "ই",
            };
            auto* list = new std::vector<std::string>();
            for (const char* raw : kRaw) list->push_back(matchKey(raw));
            return list;
        }();
        for (const std::string& suffix : *kSuffixes) {
            if (!endsWith(key, suffix)) continue;
            std::string_view base = key.substr(0, key.size() - suffix.size());
            if (utf8Length(base) < 2) continue;
            if (const WordInfo* info = find(base)) return info;
        }
        return nullptr;
    }

    // English: plural / tense endings.
    auto tryBase = [this](std::string base) -> const WordInfo* {
        if (base.size() < 2) return nullptr;
        if (const WordInfo* info = find(base)) return info;
        if (const WordInfo* info = find(base + "e")) return info;  // moving -> mov -> move
        if (base.size() >= 3 && base[base.size() - 1] == base[base.size() - 2]) {
            base.pop_back();  // stopped -> stopp -> stop, running -> runn -> run
            if (const WordInfo* info = find(base)) return info;
        }
        return nullptr;
    };
    static const char* const kEnglishSuffixes[] = {"ing", "ed", "es", "s", "d", "ly"};
    for (const char* suffix : kEnglishSuffixes) {
        if (!endsWith(key, suffix)) continue;
        std::string base(key.substr(0, key.size() - std::char_traits<char>::length(suffix)));
        if (const WordInfo* info = tryBase(std::move(base))) return info;
    }
    return nullptr;
}

bool Lexicon::findNumber(std::string_view key, double& value, NumberKind& kind) const {
    auto it = impl_->numbers.find(std::string(key));
    if (it == impl_->numbers.end()) return false;
    value = it->second.value;
    kind = it->second.kind;
    return true;
}

std::string Lexicon::suggest(std::string_view key) const {
    const std::size_t length = utf8Length(key);
    if (length < 3) return {};
    const int limit = length <= 5 ? 1 : 2;
    const bool bengali = isBengaliKey(key);
    // Scripts are made of commands: among equally close Bengali words the command form
    // ("হাঁটো", "যাও") is the likeliest one to have been meant.
    static const std::string kO = "ো", kOo = "ও";
    const auto command = [](const std::string& word) {
        return (word.size() >= kO.size() && word.compare(word.size() - kO.size(), kO.size(), kO) == 0) ||
               (word.size() >= kOo.size() && word.compare(word.size() - kOo.size(), kOo.size(), kOo) == 0);
    };

    // Smaller is better: distance, then the same length as what was typed (one wrong letter is
    // likelier than a missing one), then a command form, then the shorter and (so that the
    // answer does not depend on the order of a hash table) the alphabetically first word.
    struct Rank {
        int distance = 0;
        int otherLength = 0;
        int notCommand = 0;
        std::size_t size = 0;
        const std::string* word = nullptr;
        bool operator<(const Rank& o) const {
            if (distance != o.distance) return distance < o.distance;
            if (otherLength != o.otherLength) return otherLength < o.otherLength;
            if (notCommand != o.notCommand) return notCommand < o.notCommand;
            if (size != o.size) return size < o.size;
            return *word < *o.word;
        }
    };
    Rank best;
    for (const auto& [candidate, entry] : impl_->words) {
        if (isBengaliKey(candidate) != bengali) continue;
        if (entry.info.count == 1 && entry.info.senses[0].meaning == Concept::Filler) continue;
        const std::size_t candidateLength = utf8Length(candidate);
        if (candidateLength + static_cast<std::size_t>(limit) < length || candidateLength > length + static_cast<std::size_t>(limit)) continue;
        Rank rank;
        rank.distance = editDistance(key, candidate);
        if (rank.distance > limit) continue;
        rank.otherLength = candidateLength == length ? 0 : 1;
        rank.notCommand = bengali && !command(entry.display) ? 1 : 0;
        rank.size = entry.display.size();
        rank.word = &entry.display;
        if (best.word == nullptr || rank < best) best = rank;
    }
    return best.word != nullptr ? *best.word : std::string();
}

std::size_t Lexicon::wordCount() const { return impl_->words.size() + impl_->numbers.size(); }

std::string Lexicon::describeAmbiguousWords() const {
    std::vector<std::string> lines;
    for (const auto& [key, entry] : impl_->words) {
        if (entry.info.count < 2) continue;
        std::string line = entry.display + ":";
        for (int i = 0; i < entry.info.count; ++i) {
            line += i == 0 ? " " : ", ";
            line += conceptName(entry.info.senses[i].meaning);
        }
        lines.push_back(std::move(line));
    }
    // sort for stable output
    for (std::size_t i = 1; i < lines.size(); ++i) {
        for (std::size_t j = i; j > 0 && lines[j] < lines[j - 1]; --j) lines[j].swap(lines[j - 1]);
    }
    std::string out;
    for (const std::string& l : lines) {
        out += l;
        out += '\n';
    }
    return out;
}

const char* conceptName(Concept c) {
    static const char* const kNames[] = {
        "None",     "Filler",  "When",    "If",        "Then",     "Else",       "End",     "Forever", "Every",
        "And",      "Or",      "Not",     "After",     "Start",    "Button",     "Press",   "Release", "Touch",
        "Ground",   "Air",     "Wall",    "Land",      "Fall",     "Receive",    "Message", "Send",    "Walk",
        "Run",      "Go",      "Jump",    "Stop",      "Turn",     "Face",       "Return",  "Wait",    "Right",
        "Left",     "Up",      "Down",    "Forward",   "Backward", "Opposite",   "Second",  "Minute",  "Meter",
        "Step",     "Times",   "Speed",   "Power",     "Height",   "Gravity",    "Size",    "Increase", "Decrease",
        "More",     "Less",    "Equal",   "Fast",      "Slow",     "High",       "Light",   "On",      "Off",
        "Color",    "ColorName", "Bright", "Dim",      "Brightness", "Say",      "Destroy", "Win",     "Lose",
        "Again",    "Shake",   "Camera",  "Self",      "Player",   "Anything",   "Do",      "Give",    "Keep",
        "Be",       "Stay",    "Change",  "ButtonAction", "KeySpace", "Near",   "Far",
    };
    static_assert(GE_ARRAY_COUNT(kNames) == static_cast<std::size_t>(Concept::Count), "conceptName table out of date");
    const auto index = static_cast<std::size_t>(c);
    return index < GE_ARRAY_COUNT(kNames) ? kNames[index] : "?";
}

}  // namespace ge::kotha
