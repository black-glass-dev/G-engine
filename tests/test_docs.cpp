// G-Engine — tests/test_docs.cpp
// The documentation must not promise what the engine cannot do: every Kotha example in the
// language guide and the README is compiled here, the way the sample game compiles scripts
// (so the things an example talks about have to exist in the game).
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "app/app.h"
#include "scene/world.h"
#include "test_framework.h"

using namespace ge;

namespace {

bool readFile(const std::string& path, std::string& out) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return false;
    char buffer[4096];
    std::size_t n;
    while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0) out.append(buffer, n);
    std::fclose(f);
    return true;
}

struct Example {
    int line = 0;
    std::string text;
};

/// The ```kotha blocks of a Markdown file and, if asked for, the `code spans` in its tables.
std::vector<Example> examplesIn(const std::string& text, bool tables) {
    std::vector<Example> out;
    bool fenced = false, kotha = false;
    Example block;
    int number = 0;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        const std::string line = text.substr(start, end - start);
        start = end + 1;
        ++number;
        if (line.rfind("```", 0) == 0) {
            if (!fenced) {
                fenced = true;
                kotha = line.compare(3, std::string::npos, "kotha") == 0;
                block = Example{number + 1, ""};
            } else {
                if (kotha) out.push_back(block);
                fenced = kotha = false;
            }
            continue;
        }
        if (fenced) {
            if (kotha) block.text += line + "\n";
            continue;
        }
        if (tables && !line.empty() && line[0] == '|') {
            std::size_t at = 0;
            while (true) {
                const std::size_t open = line.find('`', at);
                if (open == std::string::npos) break;
                const std::size_t close = line.find('`', open + 1);
                if (close == std::string::npos) break;
                out.push_back(Example{number, line.substr(open + 1, close - open - 1) + "\n"});
                at = close + 1;
            }
        }
    }
    return out;
}

}  // namespace

GE_TEST(docs_examples_are_understood_by_the_engine) {
    struct Doc {
        const char* file;
        bool tables;
        std::size_t atLeast;  // guards against the extraction quietly finding nothing
    };
    World world;
    loadDemo(world);
    for (const Doc& doc : {Doc{"docs/KOTHA.md", true, 100}, Doc{"README.md", false, 2}}) {
        std::string text;
        const std::string path = std::string(GE_SOURCE_DIR) + "/" + doc.file;
        const bool found = readFile(path, text);
        CHECK_MSG(found, "cannot read " + path);
        if (!found) continue;
        const std::vector<Example> examples = examplesIn(text, doc.tables);
        CHECK_MSG(examples.size() >= doc.atLeast, std::string(doc.file) + ": only " + std::to_string(examples.size()) + " examples found");
        for (const Example& e : examples) {
            const kotha::Program p = world.check(e.text);
            CHECK_MSG(p.ok() && !p.notes.empty(), std::string(doc.file) + ":" + std::to_string(e.line) + "  " + e.text.substr(0, e.text.find('\n')) +
                                                      (p.ok() ? "  =>  understood nothing" : "  =>  " + p.errors[0].en));
        }
        std::printf("    %s: %zu examples compiled\n", doc.file, examples.size());
    }
}

GE_TEST(docs_first_example_does_what_it_says) {
    // The first thing the README asks a reader to type (a guard that walks 3 m to the left and
    // back, and speaks up when the player is near), run in a small room where the walk takes
    // the guard from 5 m away to 2 m away from the player.
    std::string readme;
    const bool found = readFile(std::string(GE_SOURCE_DIR) + "/README.md", readme);
    CHECK(found);
    const std::vector<Example> examples = examplesIn(readme, false);
    CHECK(!examples.empty());
    if (examples.empty()) return;

    World world;
    loadDemo(world);
    world.load("############\n"
               "#..........#\n"
               "#.P....G...#\n"
               "############\n");
    int slot = -1;
    for (int i = 0; i < world.scriptCount(); ++i) {
        if (world.script(i).name == "প্রহরী") slot = i;
    }
    CHECK(slot >= 0);
    if (slot < 0) return;
    CHECK(world.setScript(slot, examples[0].text).ok());
    CHECK(world.player() != nullptr && world.find("প্রহরী") != nullptr);
    if (world.player() == nullptr || world.find("প্রহরী") == nullptr) return;

    const f32 start = world.find("প্রহরী")->pos.x;
    f32 low = start, high = start, before = start, heading = 0, spokeAt = -1;
    int turns = 0;
    for (int i = 0; i < 60 * 15; ++i) {
        world.step();
        const Object* guard = world.find("প্রহরী");
        if (guard == nullptr) {
            CHECK_MSG(false, "the guard disappeared");
            return;
        }
        const f32 x = guard->pos.x;
        low = std::fmin(low, x);
        high = std::fmax(high, x);
        if (std::fabs(x - before) > 0.001f) {
            const f32 now = x > before ? 1.0f : -1.0f;
            if (heading != 0 && now != heading) ++turns;
            heading = now;
        }
        before = x;
        if (spokeAt < 0) {
            std::vector<Bubble> bubbles;
            world.bubbles(bubbles);
            for (const Bubble& b : bubbles) {
                if (b.text == "থামো!") spokeAt = std::fabs(x - world.player()->pos.x);
            }
        }
    }
    CHECK_NEAR(low, start - 3.0f, 0.25);  // 3 m to the left ...
    CHECK_NEAR(high, start, 0.25);         // ... and back, without drifting away
    CHECK(turns >= 3);                     // again and again
    CHECK_MSG(spokeAt > 0 && spokeAt <= 3.1f, "the guard spoke at a distance of " + std::to_string(spokeAt));
    std::printf("    guard walked %.2f .. %.2f (start %.2f), %d turns, spoke at %.2f m\n", low, high, start, turns, spokeAt);
}
