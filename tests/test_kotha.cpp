// G-Engine — tests/test_kotha.cpp
// The Kotha (কথা) language: Bengali and English sentences must be understood the same way.
#include <string>
#include <vector>

#include "script/compiler.h"
#include "script/text.h"
#include "script/vm.h"
#include "test_framework.h"

using namespace ge;
using namespace ge::kotha;

namespace {

/// The English note for a one-line script ("how the compiler understood it").
std::string understood(const std::string& source) {
    const Program p = compile(source);
    if (!p.ok()) return "ERROR: " + p.errors[0].en;
    std::string out;
    for (const Note& n : p.notes) {
        if (!out.empty()) out += " / ";
        out += n.en;
    }
    return out;
}

struct Case {
    const char* source;
    const char* expected;
};

void checkCases(const std::vector<Case>& cases) {
    for (const Case& c : cases) {
        const std::string got = understood(c.source);
        // matchKey() on both sides makes the comparison independent of how "য়" is encoded.
        CHECK_MSG(matchKey(got) == matchKey(c.expected), std::string("\n        source:   ") + c.source + "\n        got:      " + got + "\n        expected: " + c.expected);
    }
}

}  // namespace

GE_TEST(kotha_movement_sentences) {
    checkCases({
        // The example from the project brief: tell the character to walk right or left.
        {"ডানে হাঁটো", "walk right"},
        {"বামে হাঁটো", "walk left"},
        {"ডান দিকে হাঁটো", "walk right"},
        {"বাম দিকে হাঁটতে থাকো", "walk left"},
        {"হাঁটো ডানে", "walk right"},
        {"তুমি ডানদিকে হাঁটবে", "walk right"},
        {"ডানে হাটো", "walk right"},  // without the chandrabindu
        {"walk right", "walk right"},
        {"Walk to the left", "walk left"},
        {"please walk right", "walk right"},
        {"go left", "walk left"},
        {"ডানে যাও", "walk right"},
        {"ডানে", "walk right"},
        {"ডানে দৌড়াও", "run right"},
        {"run left", "run left"},
        {"ডানে দ্রুত হাঁটো", "walk right fast"},
        {"ধীরে বামে হাঁটো", "walk left slowly"},
        {"সামনে এগিয়ে যাও", "walk forward"},
        {"পিছনে যাও", "walk backward"},
        {"ডানে ২ সেকেন্ড হাঁটো", "walk right for 2 s"},
        {"২ সেকেন্ড ধরে ডানে হাঁটো", "walk right for 2 s"},
        {"walk right for 2 seconds", "walk right for 2 s"},
        {"ডানে ৩ মিটার দৌড়াও", "run right for 3 m"},
        {"run right 3 meters", "run right for 3 m"},
        {"দুই ধাপ সামনে যাও", "walk forward for 1 m"},
        {"থামো", "stop"},
        {"থেমে যাও", "stop"},
        {"দাঁড়াও", "stop"},
        {"stop", "stop"},
        {"হাঁটা বন্ধ করো", "stop"},
        {"stop walking", "stop"},
        {"লাফ দাও", "jump"},
        {"লাফাও", "jump"},
        {"jump", "jump"},
        {"উঁচু লাফ দাও", "jump high"},
        {"জোরে লাফ দাও", "jump high"},
        {"jump high", "jump high"},
        {"ছোট লাফ দাও", "small jump"},
        {"ঘুরে দাঁড়াও", "turn around"},
        {"ঘুরে যাও", "turn around"},
        {"উল্টো দিকে ঘোরো", "turn around"},
        {"turn around", "turn around"},
        {"বামে ঘোরো", "face left"},
        {"turn left", "face left"},
        {"ডানে তাকাও", "face right"},
        {"খেলোয়াড়ের দিকে হাঁটো", "walk toward the player"},
        {"walk toward the player", "walk toward the player"},
        {"খেলোয়াড় থেকে দূরে দৌড়াও", "run away from the player"},
        // "to the start" is where to go, not "at the start, ...":
        {"শুরুর জায়গায় ফিরে যাও", "go back to the start"},
        {"শুরুতে ফিরে যাও", "go back to the start"},
        {"go back to the start", "go back to the start"},
        {"শুরুতে লাফ দাও", "at the start ⇒ jump"},
        {"শুরুতে বামে যাও", "at the start ⇒ walk left"},
    });
}

GE_TEST(kotha_time_and_sequences) {
    checkCases({
        {"২ সেকেন্ড অপেক্ষা করো", "wait 2 s"},
        {"wait 2 seconds", "wait 2 s"},
        {"দেড় সেকেন্ড অপেক্ষা করো", "wait 1.5 s"},
        {"সাড়ে তিন সেকেন্ড অপেক্ষা করো", "wait 3.5 s"},
        {"আধা সেকেন্ড অপেক্ষা করো", "wait 0.5 s"},
        {"wait half a second", "wait 0.5 s"},
        {"১ মিনিট অপেক্ষা করো", "wait 60 s"},
        {"২ সেকেন্ড পরে লাফ দাও", "wait 2 s, then jump"},
        {"after 2 seconds jump", "wait 2 s, then jump"},
        {"২ সেকেন্ড থামো", "stop, then wait 2 s"},
        {"ডানে হাঁটো, তারপর লাফ দাও", "walk right, then jump"},
        {"ডানে হাঁটো। তারপর লাফ দাও।", "walk right, then jump"},
        {"ডানে হেঁটে লাফ দাও", "walk right, then jump"},
        {"walk right then jump", "walk right, then jump"},
        {"walk right and jump", "walk right, then jump"},
        {"walk right jump", "walk right, then jump"},
        {"লাফ দাও আর বামে হাঁটো", "jump, then walk left"},
        {"৩ বার লাফ দাও", "jump (3 times)"},
        {"jump 3 times", "jump (3 times)"},
        {"তিনবার লাফ দাও", "jump (3 times)"},
        {"jump twice", "jump (2 times)"},
        {"বারবার লাফ দাও", "forever: jump"},
        {"বার বার লাফ দাও", "forever: jump"},
        {"সবসময় ডানে হাঁটো", "forever: walk right"},
        {"jump forever", "forever: jump"},
    });
}

GE_TEST(kotha_light_and_properties) {
    checkCases({
        {"আলো জ্বালাও", "turn the light on"},
        {"বাতি জ্বালাও", "turn the light on"},
        {"turn on the light", "turn the light on"},
        {"turn the light on", "turn the light on"},
        {"আলো নিভিয়ে দাও", "turn the light off"},
        {"আলো বন্ধ করো", "turn the light off"},
        {"turn off the light", "turn the light off"},
        {"আলোর রং লাল করো", "make the light red"},
        {"make the light red", "make the light red"},
        {"আলো নীল করো", "make the light blue"},
        {"লাল আলো জ্বালাও", "make the light red, then turn the light on"},
        {"আলো উজ্জ্বল করো", "increase light brightness (×1.5)"},
        {"আলো ম্লান করো", "decrease light brightness (×0.6)"},
        {"উজ্জ্বলতা ২ করো", "set light brightness to 2"},
        {"গতি ৫ করো", "set speed to 5"},
        {"set speed to 5", "set speed to 5"},
        {"গতি ২ বাড়াও", "increase speed by 2"},
        {"increase speed by 2", "increase speed by 2"},
        {"গতি কমাও", "decrease speed (×0.8)"},
        {"লাফের জোর ৪ করো", "set jump height to 4"},
        {"লাফের উচ্চতা বাড়াও", "increase jump height (×1.25)"},
        {"স্কোর ১ বাড়াও", "increase \"স্কোর\" by 1"},
        {"increase score by 1", "increase \"score\" by 1"},
        {"স্কোর ০ করো", "set \"স্কোর\" to 0"},
        {"score = 10", "set \"score\" to 10"},
        {"\"হ্যালো\" বলো", "say \"হ্যালো\""},
        {"say \"hello\"", "say \"hello\""},
        {"\"খোলো\" বার্তা পাঠাও", "send the message \"খোলো\""},
        {"send \"open\"", "send the message \"open\""},
        {"মুছে যাও", "remove myself"},
        {"disappear", "remove myself"},
        {"ক্যামেরা কাঁপাও", "shake the camera"},
        {"shake the camera", "shake the camera"},
        {"খেলা জিতে যাও", "win the game"},
        {"আবার শুরু করো", "restart the game"},
    });
}

GE_TEST(kotha_rules_bengali_and_english_agree) {
    checkCases({
        {"ডান বোতাম চাপলে ডানে হাঁটো", "while \"RIGHT\" is held ⇒ walk right"},
        {"ডান বোতাম চাপা থাকলে ডানে হাঁটো", "while \"RIGHT\" is held ⇒ walk right"},
        {"যখন ডান বোতাম চাপা হয় তখন ডানে হাঁটো", "while \"RIGHT\" is held ⇒ walk right"},
        {"when the right button is pressed, walk right", "while \"RIGHT\" is held ⇒ walk right"},
        {"when right button pressed walk right", "while \"RIGHT\" is held ⇒ walk right"},
        {"walk right when the right button is pressed", "while \"RIGHT\" is held ⇒ walk right"},
        {"if the right button is held then walk right", "while \"RIGHT\" is held ⇒ walk right"},
        {"লাফ বোতাম চাপলে লাফ দাও", "while \"JUMP\" is held ⇒ jump"},
        {"when jump button pressed jump", "while \"JUMP\" is held ⇒ jump"},
        {"স্পেস চাপলে লাফ দাও", "while \"JUMP\" is held ⇒ jump"},
        {"ডান বোতাম ছাড়লে থামো", "when \"RIGHT\" is released ⇒ stop"},
        {"when the right button is released, stop", "when \"RIGHT\" is released ⇒ stop"},
        {"শুরুতে আলো জ্বালাও", "at the start ⇒ turn the light on"},
        {"on start, turn on the light", "at the start ⇒ turn the light on"},
        {"প্রতি ২ সেকেন্ডে লাফ দাও", "every 2 s ⇒ jump"},
        {"প্রতি ২ সেকেন্ড পর পর লাফ দাও", "every 2 s ⇒ jump"},
        {"every 2 seconds jump", "every 2 s ⇒ jump"},
        {"কয়েন ছুঁলে স্কোর ১ বাড়াও", "on touching \"কয়েন\" ⇒ increase \"স্কোর\" by 1"},
        {"when touching a coin, increase score by 1", "on touching \"coin\" ⇒ increase \"score\" by 1"},
        {"when touching coins, increase score by 1", "on touching \"coin\" ⇒ increase \"score\" by 1"},
        {"কয়েনগুলো ছুঁলে স্কোর ১ বাড়াও", "on touching \"কয়েন\" ⇒ increase \"স্কোর\" by 1"},
        {"কাঁটা ছুঁলে শুরুতে ফিরে যাও", "on touching \"কাঁটা\" ⇒ go back to the start"},
        {"দেয়ালে ধাক্কা লাগলে ঘুরে যাও", "on hitting a wall ⇒ turn around"},
        {"when hitting a wall, turn around", "on hitting a wall ⇒ turn around"},
        {"খেলোয়াড়কে ছুঁলে মুছে যাও", "on touching the player ⇒ remove myself"},
        {"when touched by the player, disappear", "on touching the player ⇒ remove myself"},
        {"মাটিতে নামলে ক্যামেরা কাঁপাও", "on landing ⇒ shake the camera"},
        {"when landing, shake the camera", "on landing ⇒ shake the camera"},
        {"\"খোলো\" বার্তা পেলে মুছে যাও", "on message \"খোলো\" ⇒ remove myself"},
        {"when I receive \"open\", disappear", "on message \"open\" ⇒ remove myself"},
        {"মাটিতে থাকলে লাফ দাও", "while on the ground ⇒ jump"},
        {"if on ground then jump", "while on the ground ⇒ jump"},
        {"শূন্যে থাকলে আলো জ্বালাও", "while in the air ⇒ turn the light on"},
        {"মাটিতে না থাকলে আলো জ্বালাও", "while in the air ⇒ turn the light on"},
        {"if not on the ground, turn on the light", "while in the air ⇒ turn the light on"},
        {"স্কোর ১০ এর বেশি হলে খেলা জিতে যাও", "while \"স্কোর\" > 10 ⇒ win the game"},
        {"যদি স্কোর ১০ এর চেয়ে বেশি হয় তাহলে খেলা জিতে যাও", "while \"স্কোর\" > 10 ⇒ win the game"},
        {"if score is more than 10 then win", "while \"score\" > 10 ⇒ win the game"},
        {"when score > 10, win", "while \"score\" > 10 ⇒ win the game"},
        {"স্কোর ৫ হলে আলো জ্বালাও", "while \"স্কোর\" = 5 ⇒ turn the light on"},
        {"খেলোয়াড় কাছে এলে খেলোয়াড়ের দিকে দৌড়াও", "while the player is within 3 m ⇒ run toward the player"},
        {"when the player is near, run toward the player", "while the player is within 3 m ⇒ run toward the player"},
        {"খেলোয়াড় ৫ মিটারের মধ্যে থাকলে আলো জ্বালাও", "while the player is within 5 m ⇒ turn the light on"},
        {"মাটিতে থাকলে আলো জ্বালাও, নাহলে আলো নেভাও", "while on the ground ⇒ turn the light on  |  otherwise ⇒ turn the light off"},
        {"if on ground then turn on the light else turn off the light", "while on the ground ⇒ turn the light on  |  otherwise ⇒ turn the light off"},
    });
}

GE_TEST(kotha_names_are_shared_between_languages) {
    // "score" and "স্কোর", "coins" and "কয়েনগুলো" are the same variable / object for the engine.
    const Program en = compile("when touching coins, increase score by 1");
    const Program bn = compile("কয়েনগুলো ছুঁলে স্কোর ১ বাড়াও");
    CHECK(en.ok() && bn.ok());
    CHECK(en.rules.size() == 1 && bn.rules.size() == 1);
    CHECK_EQ(en.rules[0].trigger.name, bn.rules[0].trigger.name);
    CHECK_EQ(en.actions[0].text, bn.actions[0].text);
    CHECK_EQ(canonicalName("Coin"), canonicalName("কয়েন"));
    CHECK_EQ(canonicalName("guard"), canonicalName("প্রহরী"));
    CHECK(canonicalName("door") != canonicalName("coin"));
}

GE_TEST(kotha_game_over_and_end_lines) {
    // "খেলা শেষ" is a command; a lone "শেষ" only closes a block.
    for (const char* source : {"খেলা শেষ", "খেলা শেষ করো", "game over", "হেরে যাও", "lose the game"}) {
        const Program p = compile(source);
        CHECK_MSG(p.ok() && p.actions.size() == 1 && p.actions[0].kind == ActionKind::Game &&
                      p.actions[0].value == static_cast<f64>(GameEvent::Lose),
                  std::string("must mean 'lose the game': ") + source);
    }
    const Program block = compile("বারবার:\n    লাফ দাও\n    ১ সেকেন্ড অপেক্ষা করো\nশেষ\n");
    CHECK(block.ok());
    CHECK_EQ(block.rules.size(), static_cast<std::size_t>(1));
    CHECK_EQ(block.actions.size(), static_cast<std::size_t>(1));  // the jump; the closing line adds nothing
    CHECK(compile("when score is over 10, win").ok());             // "over" still means "more than"
}

GE_TEST(kotha_nothing_is_silently_unreachable) {
    // Lines outside any rule run in order, so nothing after a top-level "forever" could ever run.
    const Program p = compile("বারবার:\n    লাফ দাও\n    ১ সেকেন্ড অপেক্ষা করো\nডানে হাঁটো\n");
    CHECK(!p.ok());
    CHECK(p.errors.size() == 1 && p.errors[0].line == 4);
    // ... but rules ("when ...", "at the start ...") are independent and may follow.
    CHECK(compile("বারবার:\n    লাফ দাও\n    ১ সেকেন্ড অপেক্ষা করো\nশুরুতে ডানে হাঁটো\nকয়েন ছুঁলে স্কোর ১ বাড়াও\n").ok());
    CHECK(compile("forever jump\nwalk right").errors.size() == 1);
    CHECK(compile("repeat 3 times:\n    jump\n    wait 1 second\nwalk right").ok());  // a counted loop does end
}

GE_TEST(kotha_messages) {
    checkCases({
        {"\"খোলো\" বার্তা পাঠাও", "send the message \"খোলো\""},
        {"send the message \"open\"", "send the message \"open\""},
        {"\"খোলো\" বার্তা পেলে মুছে যাও", "on message \"খোলো\" ⇒ remove myself"},
        {"\"খোলো\" বার্তা এলে মুছে যাও", "on message \"খোলো\" ⇒ remove myself"},
        {"when the message \"open\" arrives, disappear", "on message \"open\" ⇒ remove myself"},
        {"when the message \"open\" comes, disappear", "on message \"open\" ⇒ remove myself"},
        {"when the message \"open\" is received, disappear", "on message \"open\" ⇒ remove myself"},
        {"on message \"open\", disappear", "on message \"open\" ⇒ remove myself"},
    });
}

GE_TEST(kotha_notes_can_be_typed_back) {
    // The English notes say "on <event> ⇒ ...": writing an event that way must work too.
    checkCases({
        {"on touching a coin, increase score by 1", "on touching \"coin\" ⇒ increase \"score\" by 1"},
        {"on touching the coin increase score by 1", "on touching \"coin\" ⇒ increase \"score\" by 1"},
        {"on hitting a wall, turn around", "on hitting a wall ⇒ turn around"},
        {"on landing, shake the screen", "on landing ⇒ shake the camera"},
        {"on touching the player, disappear", "on touching the player ⇒ remove myself"},
        {"on touching anything, jump", "on touching anything ⇒ jump"},
        {"on start, set speed to 7", "at the start ⇒ set speed to 7"},
        {"on start set speed to 7", "at the start ⇒ set speed to 7"},
        {"at the start, jump", "at the start ⇒ jump"},
        {"every 3 s, jump", "every 3 s ⇒ jump"},
        {"while on the ground, make the light blue", "while on the ground ⇒ make the light blue"},
        {"while the player is within 4 m, say \"hi\"", "while the player is within 4 m ⇒ say \"hi\""},
        {"on touching a coin:\n    jump", "on touching \"coin\": / jump"},
        // "on" in other places keeps its meaning
        {"turn on the light", "turn the light on"},
        {"turn the light on", "turn the light on"},
        {"if on the ground, jump", "while on the ground ⇒ jump"},
    });
}

GE_TEST(kotha_names_of_things_must_exist_in_the_game) {
    // Without a scene any unknown word may be a name ...
    CHECK(compile("when touching a wal, jump").ok());
    // ... but inside a game a misspelt name is reported instead of quietly never happening.
    CompileOptions game;
    game.knownNames = {"খেলোয়াড়", "কয়েন", "প্রহরী"};
    game.strictObjects = true;
    const Program typo = compile("when touching a wal, jump\nwalk right", game);
    CHECK(!typo.ok());
    if (!typo.ok()) {
        CHECK(typo.errors.size() == 1 && typo.errors[0].line == 1);
        CHECK(typo.errors[0].en.find("\"wal\"") != std::string::npos);
        CHECK(typo.errors[0].en.find("coin") != std::string::npos);    // says what there is, in English ...
        CHECK(typo.errors[0].bn.find("«wal»") != std::string::npos);
        CHECK(matchKey(typo.errors[0].bn).find(matchKey("কয়েন")) != std::string::npos);  // ... and in Bengali,
        CHECK(typo.errors[0].bn.find("প্রহরী") != std::string::npos);                    // spelt the way it was given
    }
    CHECK(!compile("when the message \"open\" appears, disappear", game).ok());  // "appears" would have become a thing to remove
    CHECK(!compile("ভূত ছুঁলে লাফ দাও", game).ok());
    CHECK(!compile("ভূত মুছে দাও", game).ok());

    // Names that exist — in either language, with endings — and words that are not things at all are fine.
    for (const char* source : {"when touching a coin, jump", "কয়েন ছুঁলে লাফ দাও", "কয়েনগুলো ছুঁলে লাফ দাও", "when touching coins, jump", "when touching a guard, jump",
                               "প্রহরীকে ছুঁলে শুরুতে ফিরে যাও", "remove the coin", "কয়েন মুছে দাও", "when touching a wall, turn around", "when touching the player, disappear",
                               "কিছু ছুঁলে লাফ দাও", "increase gems by 1", "রত্ন ১ বাড়াও", "send the message \"anything\"", "when gems is 3, win the game"}) {
        const Program p = compile(source, game);
        CHECK_MSG(p.ok(), std::string(source) + (p.ok() ? "" : "  =>  " + p.errors[0].en));
    }
}

GE_TEST(kotha_blocks) {
    const char* patrol =
        "# পাহারাদার\n"
        "বারবার:\n"
        "    ডানে ৩ সেকেন্ড হাঁটো\n"
        "    ১ সেকেন্ড অপেক্ষা করো\n"
        "    বামে ৩ সেকেন্ড হাঁটো\n"
        "    ১ সেকেন্ড অপেক্ষা করো\n";
    CHECK_EQ(understood(patrol), std::string("forever: / walk right for 3 s / wait 1 s / walk left for 3 s / wait 1 s"));
    CHECK_EQ(understood("যখন মাটিতে থাকো:\n    লাফ দাও\n"), std::string("while on the ground: / jump"));

    // The same block without indentation, closed with "শেষ".
    const char* flat =
        "বারবার:\n"
        "ডানে ৩ সেকেন্ড হাঁটো\n"
        "বামে ৩ সেকেন্ড হাঁটো\n"
        "শেষ\n"
        "লাফ বোতাম চাপলে লাফ দাও\n";
    const Program p = compile(flat);
    CHECK(p.ok());
    CHECK_EQ(p.rules.size(), std::size_t{2});

    const char* nested =
        "যখন খেলোয়াড়কে ছোঁয়:\n"
        "    \"ধরেছি!\" বলো\n"
        "    ৩ বার:\n"
        "        লাফ দাও\n"
        "    মাটিতে থাকলে আলো জ্বালাও\n"
        "    নাহলে আলো নেভাও\n";
    const Program q = compile(nested);
    CHECK_MSG(q.ok(), q.ok() ? "" : q.errors[0].en);
    CHECK_EQ(q.rules.size(), std::size_t{1});
    CHECK_EQ(q.conditions.size(), std::size_t{1});

    // "else" on its own line negates the rule before it.
    const Program r = compile("খেলোয়াড় কাছে এলে খেলোয়াড়ের দিকে দৌড়াও\nনাহলে থামো\n");
    CHECK(r.ok());
    CHECK_EQ(r.rules.size(), std::size_t{2});
    CHECK(r.rules[1].trigger.negate != r.rules[0].trigger.negate);
}

GE_TEST(kotha_errors_are_helpful_and_bilingual) {
    {
        const Program p = compile("ডানে হাঁটোো");  // typo
        CHECK(!p.ok());
        CHECK(!p.errors.empty() && p.errors[0].bn.find("বুঝতে পারিনি") != std::string::npos);
        CHECK(!p.errors.empty() && p.errors[0].en.find("Did you mean") != std::string::npos);
        CHECK(!p.errors.empty() && p.errors[0].line == 1);
    }
    {
        const Program p = compile("wolk right");
        CHECK(!p.ok());
        CHECK(!p.errors.empty() && p.errors[0].en.find("\"walk\"") != std::string::npos);
    }
    {
        const Program p = compile("ডানে হাঁটো\nনাহলে থামো");
        CHECK(!p.ok());
        CHECK(!p.errors.empty() && p.errors[0].line == 2);
    }
    {
        const Program p = compile("ডান বোতাম চাপলে");
        CHECK(!p.ok());
    }
    {
        // An event cannot be nested inside another rule.
        const Program p = compile("শুরুতে:\n    কয়েন ছুঁলে লাফ দাও\n");
        CHECK(!p.ok());
    }
    {
        // Errors never stop the rest of the script from being understood.
        const Program p = compile("ডানে হাঁটো\nআবোলতাবোল কিছু একটা\nলাফ বোতাম চাপলে লাফ দাও\n");
        CHECK_EQ(p.errors.size(), std::size_t{1});
        CHECK_EQ(p.rules.size(), std::size_t{2});
    }
    {
        const Program p = compile("");
        CHECK(p.ok());
        CHECK(p.rules.empty());
    }
    {
        const Program p = compile("ডানে \"হাঁটো");  // unterminated string must not crash
        CHECK(!p.ok() || p.ok());
    }
    const std::string json = toJson(compile("ডানে হাঁটো\nwolk \"x\"\n"));
    CHECK(json.find("\"ok\":false") != std::string::npos);
    CHECK(json.find("\"line\":2") != std::string::npos);
    CHECK(json.find("\\\"") != std::string::npos);  // quotes inside messages are escaped
}

// ------------------------------------------------------------------------------------------
// VM
// ------------------------------------------------------------------------------------------

namespace {

/// Records everything a script does.
struct MockHost final : Host {
    bool buttons[static_cast<int>(Button::Count)] = {};
    bool ground = true;
    bool moving = false;
    f32 moveTimeLeft = 0;
    f64 speed = 4.0;
    f64 playerDistance = 100.0;
    std::vector<std::pair<std::string, f64>> vars;
    std::vector<std::string> log;
    std::string touchName;

    void tick(f32 dt) {
        if (moveTimeLeft > 0) {
            moveTimeLeft -= dt;
            if (moveTimeLeft <= 1e-4f) {
                moveTimeLeft = 0;
                moving = false;
            }
        }
    }
    int count(const std::string& entry) const {
        int n = 0;
        for (const std::string& e : log) n += e == entry ? 1 : 0;
        return n;
    }
    std::string last() const { return log.empty() ? std::string() : log.back(); }

    bool buttonDown(Button b) override { return buttons[static_cast<int>(b)]; }
    bool onGround() override { return ground; }
    bool moveDone() override { return moveTimeLeft <= 0; }
    f64 getProp(Prop p) override { return p == Prop::Speed ? speed : p == Prop::PlayerDistance ? playerDistance : 1.0; }
    f64 getVar(const std::string& key) override {
        for (const auto& v : vars) {
            if (v.first == key) return v.second;
        }
        return 0.0;
    }
    bool touchMatches(i32, const std::string& key) override { return key == touchName; }

    void move(Dir dir, bool run, f32, f32 seconds, f32 meters) override {
        moving = true;
        moveTimeLeft = seconds > 0 ? seconds : meters > 0 ? meters / 4.0f : 0.0f;
        log.push_back(std::string(run ? "run:" : "walk:") + (dir == Dir::Right ? "right" : dir == Dir::Left ? "left" : "other"));
    }
    void stop() override {
        moving = false;
        moveTimeLeft = 0;
        log.push_back("stop");
    }
    void jump(f32) override { log.push_back("jump"); }
    void face(Dir dir) override { log.push_back(dir == Dir::Opposite ? "flip" : "face"); }
    void setProp(Prop p, f64 v) override {
        if (p == Prop::Speed) speed = v;
        log.push_back("prop");
    }
    void setVar(const std::string& key, f64 v) override {
        for (auto& e : vars) {
            if (e.first == key) {
                e.second = v;
                return;
            }
        }
        vars.emplace_back(key, v);
    }
    void lightSwitch(bool on) override { log.push_back(on ? "light:on" : "light:off"); }
    void lightColor(u32) override { log.push_back("light:color"); }
    void say(const std::string& text, f32) override { log.push_back("say:" + text); }
    void send(const std::string& m) override { log.push_back("send:" + m); }
    void destroy(const std::string& key) override { log.push_back("destroy:" + key); }
    void respawn() override { log.push_back("respawn"); }
    void shake(f32, f32) override { log.push_back("shake"); }
    void gameEvent(GameEvent e) override { log.push_back(e == GameEvent::Win ? "win" : e == GameEvent::Lose ? "lose" : "restart"); }
};

struct Runner {
    Program program;
    Vm vm;
    MockHost host;
    static constexpr f32 kDt = 1.0f / 60.0f;

    explicit Runner(const std::string& source) : program(compile(source)) {
        CHECK_MSG(program.ok(), program.ok() ? "" : program.errors[0].en);
        vm.load(&program);
    }
    void step(int ticks = 1) {
        for (int i = 0; i < ticks; ++i) {
            vm.update(host, kDt);
            host.tick(kDt);
        }
    }
    void seconds(f32 s) { step(static_cast<int>(s * 60.0f + 0.5f)); }
};

}  // namespace

GE_TEST(vm_start_statements_run_once) {
    Runner r("আলো জ্বালাও\nডানে হাঁটো\n");
    r.step(5);
    CHECK_EQ(r.host.count("light:on"), 1);
    CHECK_EQ(r.host.count("walk:right"), 1);
    CHECK(r.host.moving);
    CHECK_EQ(r.vm.runningTasks(), 0);
}

GE_TEST(vm_held_button_moves_and_release_stops) {
    Runner r("ডান বোতাম চাপলে ডানে হাঁটো\nবাম বোতাম চাপলে বামে হাঁটো\nলাফ বোতাম চাপলে লাফ দাও\n");
    r.step(3);
    CHECK(r.host.log.empty());

    r.host.buttons[static_cast<int>(Button::Right)] = true;
    r.step(30);
    CHECK_EQ(r.host.count("walk:right"), 1);  // started once, not every tick
    CHECK(r.host.moving);

    r.host.buttons[static_cast<int>(Button::Right)] = false;
    r.step(2);
    CHECK_EQ(r.host.last(), std::string("stop"));
    CHECK(!r.host.moving);

    // Jumping does not interrupt walking, and releasing JUMP does not stop the walk.
    r.host.buttons[static_cast<int>(Button::Left)] = true;
    r.step(2);
    r.host.buttons[static_cast<int>(Button::Jump)] = true;
    r.step(2);
    r.host.buttons[static_cast<int>(Button::Jump)] = false;
    r.step(2);
    CHECK_EQ(r.host.count("jump"), 1);
    CHECK(r.host.moving);
    CHECK_EQ(r.host.count("stop"), 1);

    // Holding both directions: the newer one wins; releasing it hands control back.
    r.host.buttons[static_cast<int>(Button::Right)] = true;
    r.step(2);
    CHECK_EQ(r.host.last(), std::string("walk:right"));
    r.host.buttons[static_cast<int>(Button::Right)] = false;
    r.step(2);
    CHECK_EQ(r.host.last(), std::string("walk:left"));
    CHECK(r.host.moving);
}

GE_TEST(vm_timed_moves_and_waits) {
    Runner r("ডানে ২ সেকেন্ড হাঁটো\n১ সেকেন্ড অপেক্ষা করো\nবামে ১ সেকেন্ড হাঁটো\nলাফ দাও\n");
    r.seconds(1.0f);
    CHECK_EQ(r.host.count("walk:right"), 1);
    CHECK_EQ(r.host.count("walk:left"), 0);
    r.seconds(1.5f);  // t = 2.5: the walk is over, still waiting
    CHECK(!r.host.moving);
    CHECK_EQ(r.host.count("walk:left"), 0);
    r.seconds(1.0f);  // t = 3.5
    CHECK_EQ(r.host.count("walk:left"), 1);
    CHECK_EQ(r.host.count("jump"), 0);
    r.seconds(1.0f);  // t = 4.5
    CHECK_EQ(r.host.count("jump"), 1);
    CHECK_EQ(r.vm.runningTasks(), 0);
}

GE_TEST(vm_loops) {
    {
        Runner r("বারবার:\n    ডানে ১ সেকেন্ড হাঁটো\n    বামে ১ সেকেন্ড হাঁটো\n");
        r.seconds(6.2f);
        CHECK_EQ(r.host.count("walk:right"), 4);
        CHECK_EQ(r.host.count("walk:left"), 3);
        CHECK_EQ(r.vm.runningTasks(), 1);
    }
    {
        Runner r("৩ বার:\n    \"হ্যালো\" বলো\n    ১ সেকেন্ড অপেক্ষা করো\n\"শেষ\" বলো\n");
        r.seconds(10.0f);
        CHECK_EQ(r.host.count("say:হ্যালো"), 3);
        CHECK_EQ(r.host.count("say:শেষ"), 1);
        CHECK_EQ(r.vm.runningTasks(), 0);
    }
    {
        // A loop without any waiting must not freeze the game: one round per tick.
        Runner r("বারবার স্কোর ১ বাড়াও");
        r.step(10);
        CHECK_NEAR(r.host.getVar("স্কোর"), 10.0, 1e-9);
    }
    {
        // "N times" waits for each jump to land before the next one.
        Runner r("৩ বার লাফ দাও");
        r.step(3);
        CHECK_EQ(r.host.count("jump"), 1);
        r.host.ground = false;
        r.seconds(0.5f);
        CHECK_EQ(r.host.count("jump"), 1);
        r.host.ground = true;
        r.step(2);
        CHECK_EQ(r.host.count("jump"), 2);
        r.seconds(1.0f);
        CHECK_EQ(r.host.count("jump"), 3);
        r.seconds(1.0f);
        CHECK_EQ(r.host.count("jump"), 3);
    }
}

GE_TEST(vm_events) {
    {
        Runner r("প্রতি ২ সেকেন্ডে লাফ দাও");
        r.seconds(1.9f);
        CHECK_EQ(r.host.count("jump"), 0);
        r.seconds(0.2f);
        CHECK_EQ(r.host.count("jump"), 1);
        r.seconds(6.0f);
        CHECK_EQ(r.host.count("jump"), 4);
    }
    {
        Runner r("কয়েন ছুঁলে স্কোর ১ বাড়াও\nদেয়ালে ধাক্কা লাগলে ঘুরে যাও\n");
        r.host.touchName = canonicalName("কয়েন");
        r.step(2);
        r.vm.notifyTouch(7, false);
        r.step(1);
        CHECK_NEAR(r.host.getVar(canonicalName("স্কোর")), 1.0, 1e-9);
        CHECK_EQ(r.host.count("flip"), 0);
        r.vm.notifyTouch(-1, true);
        r.step(1);
        CHECK_EQ(r.host.count("flip"), 1);
        CHECK_NEAR(r.host.getVar(canonicalName("স্কোর")), 1.0, 1e-9);
        // A different object does not count.
        r.host.touchName = "অন্য";
        r.vm.notifyTouch(8, false);
        r.step(1);
        CHECK_NEAR(r.host.getVar(canonicalName("স্কোর")), 1.0, 1e-9);
    }
    {
        Runner r("\"খোলো\" বার্তা পেলে মুছে যাও\nশুরুতে \"খোলো\" বার্তা পাঠাও\n");
        r.step(2);
        CHECK_EQ(r.host.count("send:" + canonicalName("খোলো")), 1);
        CHECK_EQ(r.host.count("destroy:"), 0);
        r.vm.notifyMessage(canonicalName("খোলো"));
        r.step(1);
        CHECK_EQ(r.host.count("destroy:"), 1);
    }
    {
        Runner r("মাটিতে নামলে ক্যামেরা কাঁপাও");
        r.step(3);
        CHECK_EQ(r.host.count("shake"), 0);
        r.host.ground = false;
        r.step(10);
        r.host.ground = true;
        r.step(2);
        CHECK_EQ(r.host.count("shake"), 1);
    }
}

GE_TEST(vm_conditions) {
    {
        Runner r("স্কোর ৩ এর বেশি হলে খেলা জিতে যাও\nকয়েন ছুঁলে স্কোর ১ বাড়াও\n");
        r.host.touchName = canonicalName("কয়েন");
        for (int i = 0; i < 3; ++i) {
            r.vm.notifyTouch(i, false);
            r.step(2);
        }
        CHECK_EQ(r.host.count("win"), 0);
        r.vm.notifyTouch(9, false);
        r.step(3);
        CHECK_EQ(r.host.count("win"), 1);
        r.step(30);
        CHECK_EQ(r.host.count("win"), 1);  // fires when it becomes true, not on every tick
    }
    {
        Runner r("খেলোয়াড় কাছে এলে খেলোয়াড়ের দিকে দৌড়াও\nনাহলে থামো\n");
        r.step(3);
        CHECK_EQ(r.host.last(), std::string("stop"));
        r.host.playerDistance = 2.0;
        r.step(3);
        CHECK_EQ(r.host.last(), std::string("run:other"));
        r.host.playerDistance = 9.0;
        r.step(3);
        CHECK_EQ(r.host.last(), std::string("stop"));
    }
    {
        // "if / else" inside a loop is evaluated every time round.
        Runner r("বারবার:\n    মাটিতে থাকলে আলো জ্বালাও\n    নাহলে আলো নেভাও\n    ১ সেকেন্ড অপেক্ষা করো\n");
        r.step(2);
        CHECK_EQ(r.host.last(), std::string("light:on"));
        r.host.ground = false;
        r.seconds(1.1f);
        CHECK_EQ(r.host.last(), std::string("light:off"));
    }
    {
        // A held button with a loop: the loop ends when the button is released.
        Runner r("অ্যাকশন বোতাম চাপলে বারবার স্কোর ১ বাড়াও");
        r.host.buttons[static_cast<int>(Button::Action)] = true;
        r.step(5);
        r.host.buttons[static_cast<int>(Button::Action)] = false;
        r.step(5);
        CHECK_NEAR(r.host.getVar(canonicalName("স্কোর")), 5.0, 1e-9);
        CHECK_EQ(r.vm.runningTasks(), 0);
    }
}
