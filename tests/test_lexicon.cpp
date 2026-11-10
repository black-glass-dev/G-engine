// G-Engine — tests/test_lexicon.cpp
#include <cstdio>

#include "script/lexicon.h"
#include "script/text.h"
#include "test_framework.h"

using namespace ge;
using namespace ge::kotha;

namespace {

bool means(const char* word, Concept c) {
    const WordInfo* info = Lexicon::get().findInflected(matchKey(word));
    return info != nullptr && info->has(c);
}

bool isConditional(const char* word) {
    const WordInfo* info = Lexicon::get().findInflected(matchKey(word));
    return info != nullptr && info->conditional();
}

}  // namespace

GE_TEST(matchkey_normalises_bengali_spelling_variants) {
    // chandrabindu optional
    CHECK_EQ(matchKey("হাঁটো"), matchKey("হাটো"));
    // decomposed vs composed letters:  ড + ়  ==  ড়   and   ে + া  ==  ো
    CHECK_EQ(matchKey("দৌ\u09A1\u09BCাও"), matchKey("দৌ\u09DCাও"));
    CHECK_EQ(matchKey("হাঁট\u09C7\u09BE"), matchKey("হাঁট\u09CB"));
    CHECK_EQ(matchKey("খেলো\u09AF\u09BCাড়"), matchKey("খেলো\u09DFাড়"));
    // long/short vowel variants
    CHECK_EQ(matchKey("নীচে"), matchKey("নিচে"));
    CHECK_EQ(matchKey("গোলাপী"), matchKey("গোলাপি"));
    // zero-width joiners are invisible
    CHECK_EQ(matchKey("র\u200Dং"), matchKey("রং"));
    // Latin: case-insensitive, apostrophes dropped
    CHECK_EQ(matchKey("Walk"), std::string("walk"));
    CHECK_EQ(matchKey("Don't"), std::string("dont"));
    // different words stay different
    CHECK(matchKey("ডান") != matchKey("বাম"));
}

GE_TEST(lexicon_knows_bengali_movement_words) {
    CHECK(means("হাঁটো", Concept::Walk));
    CHECK(means("হাটো", Concept::Walk));   // without chandrabindu
    CHECK(means("হাঁটবে", Concept::Walk));
    CHECK(means("হেঁটে", Concept::Walk));
    CHECK(means("হাঁটতে", Concept::Walk));
    CHECK(means("দৌড়াও", Concept::Run));
    CHECK(means("দৌড়াবে", Concept::Run));
    CHECK(means("দৌড়ে", Concept::Run));
    CHECK(means("লাফ", Concept::Jump));
    CHECK(means("লাফাও", Concept::Jump));
    CHECK(means("লাফিয়ে", Concept::Jump));
    CHECK(means("থামো", Concept::Stop));
    CHECK(means("থেমে", Concept::Stop));
    CHECK(means("দাঁড়াও", Concept::Stop));
    CHECK(means("ঘোরো", Concept::Turn));
    CHECK(means("ঘুরে", Concept::Turn));
    CHECK(means("যাও", Concept::Go));
    CHECK(means("চলো", Concept::Go));
    CHECK(means("অপেক্ষা", Concept::Wait));
}

GE_TEST(lexicon_knows_directions_with_case_endings) {
    CHECK(means("ডান", Concept::Right));
    CHECK(means("ডানে", Concept::Right));
    CHECK(means("ডানদিকে", Concept::Right));
    CHECK(means("বাম", Concept::Left));
    CHECK(means("বামে", Concept::Left));
    CHECK(means("বাঁয়ে", Concept::Left));
    CHECK(means("উপরে", Concept::Up));
    CHECK(means("ওপরে", Concept::Up));
    CHECK(means("নিচে", Concept::Down));
    CHECK(means("নীচে", Concept::Down));
    CHECK(means("সামনে", Concept::Forward));
    CHECK(means("পিছনে", Concept::Backward));
    CHECK(means("পেছনে", Concept::Backward));
}

GE_TEST(lexicon_strips_noun_endings) {
    CHECK(means("মাটিতে", Concept::Ground));
    CHECK(means("বোতামটা", Concept::Button));
    CHECK(means("বোতামে", Concept::Button));
    CHECK(means("দেয়ালে", Concept::Wall));
    CHECK(means("আলোর", Concept::Light));
    CHECK(means("আলোটা", Concept::Light));
    CHECK(means("লাফের", Concept::Jump));
    CHECK(means("গতিতে", Concept::Speed));
    CHECK(means("রঙের", Concept::Color));
    CHECK(means("খেলোয়াড়কে", Concept::Player));
    CHECK(means("সেকেন্ডে", Concept::Second));
    CHECK(means("শুরুর", Concept::Start));
    CHECK(means("বাতাসে", Concept::Air));
    CHECK(means("শূন্যে", Concept::Air));
}

GE_TEST(lexicon_marks_conditional_verb_forms) {
    // "-লে" forms mean "when/if ..." and end the condition part of a sentence.
    CHECK(isConditional("চাপলে"));
    CHECK(means("চাপলে", Concept::Press));
    CHECK(isConditional("ছাড়লে"));
    CHECK(means("ছাড়লে", Concept::Release));
    CHECK(isConditional("ছুঁলে"));
    CHECK(means("ছুঁলে", Concept::Touch));
    CHECK(isConditional("হলে"));
    CHECK(isConditional("থাকলে"));
    CHECK(isConditional("পেলে"));
    CHECK(isConditional("লাগলে"));
    CHECK(isConditional("নামলে"));
    CHECK(isConditional("গেলে"));
    CHECK(!isConditional("চাপো"));
    CHECK(!isConditional("হাঁটো"));
    CHECK(!isConditional("চেপে"));
}

GE_TEST(lexicon_knows_english) {
    CHECK(means("walk", Concept::Walk));
    CHECK(means("Walking", Concept::Walk));
    CHECK(means("walks", Concept::Walk));
    CHECK(means("run", Concept::Run));
    CHECK(means("running", Concept::Run));
    CHECK(means("jump", Concept::Jump));
    CHECK(means("jumped", Concept::Jump));
    CHECK(means("stop", Concept::Stop));
    CHECK(means("stopped", Concept::Stop));
    CHECK(means("moving", Concept::Go));
    CHECK(means("right", Concept::Right));
    CHECK(means("left", Concept::Left));
    CHECK(means("pressed", Concept::Press));
    CHECK(means("released", Concept::Release));
    CHECK(means("seconds", Concept::Second));
    CHECK(means("button", Concept::Button));
    CHECK(means("touches", Concept::Touch));
    CHECK(means("the", Concept::Filler));
}

GE_TEST(lexicon_everyday_nouns_stay_free_for_object_names) {
    // These are ordinary words a game maker will use as object names; they must not be
    // swallowed by the vocabulary.
    const char* const kFree[] = {"বল", "বাড়ি", "নাম", "পাথর", "কয়েন", "শত্রু", "বাক্স", "দরজা", "গাছ", "তারা", "পাখি", "coin", "enemy", "door", "box", "ball"};
    for (const char* w : kFree) {
        CHECK_MSG(Lexicon::get().findInflected(matchKey(w)) == nullptr, w);
    }
    // "তাকে" (him/her) is only a filler pronoun, not the verb "look".
    const WordInfo* info = Lexicon::get().findInflected(matchKey("তাকে"));
    CHECK(info != nullptr && info->has(Concept::Filler) && !info->has(Concept::Face));
}

GE_TEST(lexicon_numbers) {
    const Lexicon& lex = Lexicon::get();
    double v = 0;
    NumberKind k = NumberKind::Plain;
    CHECK(lex.findNumber(matchKey("তিন"), v, k));
    CHECK_NEAR(v, 3.0, 1e-9);
    CHECK(lex.findNumber(matchKey("পাঁচ"), v, k));
    CHECK_NEAR(v, 5.0, 1e-9);
    CHECK(lex.findNumber(matchKey("দেড়"), v, k));
    CHECK_NEAR(v, 1.5, 1e-9);
    CHECK(lex.findNumber(matchKey("আড়াই"), v, k));
    CHECK_NEAR(v, 2.5, 1e-9);
    CHECK(lex.findNumber(matchKey("সাড়ে"), v, k));
    CHECK(k == NumberKind::AddHalf);
    CHECK(lex.findNumber("seven", v, k));
    CHECK_NEAR(v, 7.0, 1e-9);
    CHECK(lex.findNumber("twice", v, k));
    CHECK(k == NumberKind::CountTimes);
    CHECK(!lex.findNumber(matchKey("হাঁটো"), v, k));
}

GE_TEST(lexicon_suggestions) {
    const Lexicon& lex = Lexicon::get();
    CHECK_EQ(lex.suggest("wolk"), std::string("walk"));
    CHECK_EQ(lex.suggest("jmp"), std::string("jump"));
    CHECK_EQ(lex.suggest("wlak"), std::string("walk"));  // two neighbours swapped
    CHECK_EQ(lex.suggest("jupm"), std::string("jump"));
    CHECK_EQ(lex.suggest("rigth"), std::string("right"));
    CHECK(!lex.suggest(matchKey("হাঁটোো")).empty());
    // A real word a script would use, not a bare verb stem:
    CHECK_EQ(matchKey(lex.suggest(matchKey("হাঁটু"))), matchKey("হাঁটো"));
    CHECK_EQ(matchKey(lex.suggest(matchKey("থামু"))), matchKey("থামো"));
    CHECK(lex.suggest("zzzzqqqq").empty());
    CHECK_EQ(editDistance("walk", "wlak"), 1);
    CHECK_EQ(editDistance("walk", "walk"), 0);
    CHECK_EQ(editDistance("walk", "wak"), 1);
    CHECK_EQ(editDistance("walk", "klaw"), 3);  // two wrong letters and one swap
    CHECK_EQ(editDistance("ab", "ba"), 1);
    CHECK_EQ(editDistance("abc", "ca"), 3);
}

GE_TEST(lexicon_ambiguous_words_are_only_the_expected_ones) {
    const Lexicon& lex = Lexicon::get();
    const std::string ambiguous = lex.describeAmbiguousWords();
    std::printf("    vocabulary: %zu word forms\n    ambiguous words:\n", lex.wordCount());
    std::size_t start = 0, lines = 0;
    while (start < ambiguous.size()) {
        std::size_t end = ambiguous.find('\n', start);
        if (end == std::string::npos) end = ambiguous.size();
        std::printf("      %.*s\n", static_cast<int>(end - start), ambiguous.data() + start);
        start = end + 1;
        ++lines;
    }
    // A small, reviewed set (see the comments in lexicon.cpp). If this grows, check for typos.
    CHECK(lines <= 8);
    CHECK(lex.wordCount() > 1200);
}
