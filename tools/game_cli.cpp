// G-Engine — tools/game_cli.cpp
// The agent edition: build, check and test games from the command line, without a screen.
//
//   ge-game new <dir>                     write a starter game folder (level + Kotha scripts)
//   ge-game check <dir>                   compile every script; print JSON errors and notes
//   ge-game simulate <dir> [options]      run the game headless; print JSON of what happened
//       --seconds N        how long to play (default 30)
//       --input LIST       press buttons during the run: "t:button:hold,..."
//                          (t = start in seconds, hold = how long; buttons:
//                           left right up down jump action)
//   ge-game pack <dir> [-o game.json]     bundle the folder into one JSON file
//
// A game folder is:
//   level.txt        the level picture (walls #, player P, guard G, coin c, …)
//   <thing>.kotha    one script per thing — the file name is the thing name
//                    (খেলোয়াড়.kotha, প্রহরী.kotha, কয়েন.kotha, …)
//
// Everything on stdout is one JSON object; diagnostics go to stderr.
// Exit code 0 = good, 1 = the game has errors, 2 = usage problem.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include "scene/world.h"
#include "script/compiler.h"

using ge::GameState;
using ge::World;
using ge::kotha::Button;
using ge::kotha::Program;

namespace {

const char* kUsage =
    "usage:\n"
    "  ge-game new <dir>                  write a starter game folder\n"
    "  ge-game check <dir>                compile the scripts, print JSON\n"
    "  ge-game simulate <dir> [--seconds N] [--input t:button:hold,...]\n"
    "  ge-game pack <dir> [-o game.json]  bundle the folder into one JSON file\n";

std::string esc(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    o += buf;
                } else {
                    o += static_cast<char>(c);
                }
        }
    }
    return o;
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

bool writeFile(const std::string& path, const std::string& data) {
    std::ofstream out(path.c_str(), std::ios::binary);
    if (!out) return false;
    out << data;
    return out.good();
}

/// mkdir -p: creates every missing folder of the path.
bool makeDirs(const std::string& dir) {
    std::string p;
    for (std::size_t i = 0; i <= dir.size(); i++) {
        if (i == dir.size() || dir[i] == '/') {
            if (p.empty() || p == "/") {
                // nothing to create yet
            } else {
                struct stat st;
                if (stat(p.c_str(), &st) != 0 && mkdir(p.c_str(), 0755) != 0) return false;
            }
            if (i < dir.size()) p += '/';
        } else {
            p += dir[i];
        }
    }
    return true;
}

struct GameFile {
    std::string name;   // the thing it talks about (the file name without .kotha)
    std::string file;
    std::string source;
};

struct Game {
    std::string dir;
    std::string level;
    std::vector<GameFile> scripts;
    std::vector<std::string> errors;
};

void loadGame(const std::string& dir, Game& g) {
    g.dir = dir;
    struct stat st;
    if (stat(dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        g.errors.push_back("no such game folder: " + dir);
        return;
    }
    if (!readFile(dir + "/level.txt", g.level)) {
        g.errors.push_back("level.txt is missing — a game folder needs one (the level picture)");
    }
    DIR* d = opendir(dir.c_str());
    if (!d) {
        g.errors.push_back("cannot open " + dir);
        return;
    }
    std::vector<std::string> names;
    for (dirent* e = readdir(d); e; e = readdir(d)) {
        std::string f = e->d_name;
        if (f.size() > 6 && f.compare(f.size() - 6, 6, ".kotha") == 0) names.push_back(f);
    }
    closedir(d);
    std::sort(names.begin(), names.end());
    for (const std::string& f : names) {
        GameFile gf;
        gf.file = f;
        gf.name = f.substr(0, f.size() - 6);
        if (!readFile(dir + "/" + f, gf.source)) {
            g.errors.push_back("cannot read " + f);
            continue;
        }
        g.scripts.push_back(gf);
    }
}

std::string jsonFiles(const std::vector<GameFile>& files) {
    std::string o = "[";
    for (std::size_t i = 0; i < files.size(); i++) {
        if (i) o += ",";
        o += "{\"name\":\"" + esc(files[i].name) + "\",\"source\":\"" + esc(files[i].source) + "\"}";
    }
    return o + "]";
}

int cmdNew(const std::string& dir) {
    struct stat st;
    if (stat(dir.c_str(), &st) == 0) {
        std::fprintf(stderr, "ge-game: %s already exists\n", dir.c_str());
        return 2;
    }
    if (!makeDirs(dir)) {
        std::fprintf(stderr, "ge-game: cannot create %s\n", dir.c_str());
        return 2;
    }
    const char* level =
        "########################\n"
        "#......................#\n"
        "#......................#\n"
        "#......................#\n"
        "#.......L..............#\n"
        "#......................#\n"
        "#..P...................#\n"
        "########################\n";
    struct { const char* name; const char* source; } scripts[] = {
        {"খেলোয়াড়",
         "# খেলোয়াড় — নিয়ম এখানে লেখো\n"
         "ডান বোতাম চাপলে ডানে হাঁটো\n"
         "বাম বোতাম চাপলে বামে হাঁটো\n"
         "লাফ বোতাম চাপলে লাফ দাও\n"},
        {"প্রহরী", "# প্রহরী\n"},
        {"কয়েন",
         "# কয়েন\n"
         "খেলোয়াড়কে ছুঁলে মুছে যাও\n"},
        {"দরজা", "# দরজা\n"},
        {"বাতি", "# বাতি\n"},
        {"বাক্স", "# বাক্স\n"},
    };
    writeFile(dir + "/level.txt", level);
    for (const auto& s : scripts) writeFile(dir + "/" + s.name + ".kotha", s.source);
    std::printf("{\"ok\":true,\"created\":\"%s\",\"files\":[\"level.txt\",\"খেলোয়াড়.kotha\","
                "\"প্রহরী.kotha\",\"কয়েন.kotha\",\"দরজা.kotha\",\"বাতি.kotha\",\"বাক্স.kotha\"]}\n",
                esc(dir).c_str());
    return 0;
}

int cmdCheck(const std::string& dir) {
    Game g;
    loadGame(dir, g);
    World world;
    if (g.level.empty() == false) world.load(g.level);

    bool ok = g.errors.empty() && !g.level.empty();
    std::string out = "{\"ok\":";
    std::string files = "[";
    for (std::size_t i = 0; i < g.scripts.size(); i++) {
        Program p = world.check(g.scripts[i].source);
        if (!p.ok()) ok = false;
        if (i) files += ",";
        files += "{\"file\":\"" + esc(g.scripts[i].file) + "\",\"name\":\"" + esc(g.scripts[i].name)
               + "\",\"check\":" + ge::kotha::toJson(p) + "}";
    }
    files += "]";
    std::string errs = "[";
    for (std::size_t i = 0; i < g.errors.size(); i++) {
        if (i) errs += ",";
        errs += "\"" + esc(g.errors[i]) + "\"";
    }
    errs += "]";
    out += ok ? "true" : "false";
    out += ",\"dir\":\"" + esc(dir) + "\",\"errors\":" + errs + ",\"files\":" + files + "}\n";
    std::fputs(out.c_str(), stdout);
    return ok ? 0 : 1;
}

struct InputEvent {
    double t;
    int button;
    double hold;
};

bool buttonFromName(const std::string& n, int& out) {
    if (n == "left" || n == "l") out = static_cast<int>(Button::Left);
    else if (n == "right" || n == "r") out = static_cast<int>(Button::Right);
    else if (n == "up" || n == "u") out = static_cast<int>(Button::Up);
    else if (n == "down" || n == "d") out = static_cast<int>(Button::Down);
    else if (n == "jump" || n == "j") out = static_cast<int>(Button::Jump);
    else if (n == "action" || n == "a") out = static_cast<int>(Button::Action);
    else return false;
    return true;
}

int cmdSimulate(const std::string& dir, double seconds, const std::string& inputSpec) {
    Game g;
    loadGame(dir, g);
    if (!g.errors.empty() || g.level.empty()) {
        std::printf("{\"ok\":false,\"errors\":[\"%s\"]}\n", esc(g.errors.empty() ? "level.txt is missing" : g.errors[0]).c_str());
        return 1;
    }

    World world;
    world.load(g.level);
    bool ok = true;
    std::string checks = "[";
    for (std::size_t i = 0; i < g.scripts.size(); i++) {
        Program p = world.check(g.scripts[i].source);
        if (!p.ok()) ok = false;
        if (i) checks += ",";
        checks += "{\"file\":\"" + esc(g.scripts[i].file) + "\",\"name\":\"" + esc(g.scripts[i].name)
                + "\",\"check\":" + ge::kotha::toJson(p) + "}";
    }
    checks += "]";
    if (!ok) {
        std::printf("{\"ok\":false,\"dir\":\"%s\",\"files\":%s}\n", esc(dir).c_str(), checks.c_str());
        return 1;
    }
    for (const GameFile& f : g.scripts) world.addScript(f.name, f.source);

    // input schedule: "t:button:hold,t:button:hold,…"
    std::vector<InputEvent> events;
    std::string spec = inputSpec;
    std::size_t pos = 0;
    while (!spec.empty()) {
        std::size_t comma = spec.find(',', pos);
        std::string item = spec.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        std::size_t c1 = item.find(':');
        std::size_t c2 = c1 == std::string::npos ? std::string::npos : item.find(':', c1 + 1);
        if (c1 != std::string::npos && c2 != std::string::npos) {
            InputEvent ev;
            ev.t = std::atof(item.substr(0, c1).c_str());
            ev.hold = std::atof(item.substr(c2 + 1).c_str());
            if (buttonFromName(item.substr(c1 + 1, c2 - c1 - 1), ev.button)) events.push_back(ev);
        }
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }

    const int totalSteps = static_cast<int>(seconds * 60.0 + 0.5);
    std::set<std::string> heard;
    std::string said = "[";
    bool firstSaid = true;
    const char* stateName = "playing";

    for (int step = 0; step < totalSteps; step++) {
        const double t = step / 60.0;
        for (int b = 0; b < static_cast<int>(Button::Count); b++) world.setButton(static_cast<Button>(b), false);
        for (const InputEvent& ev : events) {
            if (t >= ev.t && t < ev.t + ev.hold) world.setButton(static_cast<Button>(ev.button), true);
        }
        world.step();
        if (step % 15 == 0) {
            std::vector<ge::Bubble> bubbles;
            world.bubbles(bubbles);
            for (const ge::Bubble& b : bubbles) {
                if (heard.insert(b.text).second) {
                    if (!firstSaid) said += ",";
                    firstSaid = false;
                    said += "\"" + esc(b.text) + "\"";
                }
            }
        }
        if (world.state() != GameState::Playing) {
            stateName = world.state() == GameState::Won ? "won" : "lost";
            break;
        }
    }
    said += "]";

    std::string vars = "{";
    bool firstVar = true;
    for (const auto& kv : world.variables()) {
        if (!firstVar) vars += ",";
        firstVar = false;
        char num[32];
        std::snprintf(num, sizeof num, "%g", kv.second);
        vars += "\"" + esc(kv.first) + "\":" + num;
    }
    vars += "}";

    std::string px = "null", py = "null", player = "null";
    if (const ge::Object* p = world.player()) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.3f", p->pos.x);
        px = buf;
        std::snprintf(buf, sizeof buf, "%.3f", p->pos.y);
        py = buf;
        player = "{\"x\":" + px + ",\"y\":" + py + "}";
    }

    std::printf("{\"ok\":true,\"dir\":\"%s\",\"seconds\":%g,\"steps\":%d,\"state\":\"%s\","
                "\"player\":%s,\"vars\":%s,\"said\":%s}\n",
                esc(dir).c_str(), seconds, totalSteps, stateName, player.c_str(), vars.c_str(), said.c_str());
    return 0;
}

int cmdPack(const std::string& dir, const std::string& outPath) {
    Game g;
    loadGame(dir, g);
    if (!g.errors.empty() || g.level.empty()) {
        std::fprintf(stderr, "ge-game: %s\n", g.errors.empty() ? "level.txt is missing" : g.errors[0].c_str());
        return 1;
    }
    std::string name = dir;
    std::size_t slash = name.find_last_of('/');
    if (slash != std::string::npos) name = name.substr(slash + 1);
    std::string json = "{\"app\":\"g-engine\",\"version\":1,\"name\":\"" + esc(name) + "\",\"level\":\""
                     + esc(g.level) + "\",\"scripts\":" + jsonFiles(g.scripts) + "}\n";
    if (!writeFile(outPath, json)) {
        std::fprintf(stderr, "ge-game: cannot write %s\n", outPath.c_str());
        return 2;
    }
    std::printf("{\"ok\":true,\"wrote\":\"%s\",\"scripts\":%zu}\n", esc(outPath).c_str(), g.scripts.size());
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fputs(kUsage, stderr);
        return 2;
    }
    std::string cmd = argv[1];
    std::string dir = argv[2];
    if (cmd == "new") return cmdNew(dir);
    if (cmd == "check") return cmdCheck(dir);
    if (cmd == "simulate") {
        double seconds = 30.0;
        std::string input;
        for (int i = 3; i + 1 < argc; i++) {
            if (std::strcmp(argv[i], "--seconds") == 0) seconds = std::atof(argv[++i]);
            else if (std::strcmp(argv[i], "--input") == 0) input = argv[++i];
        }
        return cmdSimulate(dir, seconds, input);
    }
    if (cmd == "pack") {
        std::string out = dir + ".json";
        for (int i = 3; i + 1 < argc; i++) {
            if (std::strcmp(argv[i], "-o") == 0) out = argv[++i];
        }
        return cmdPack(dir, out);
    }
    std::fputs(kUsage, stderr);
    return 2;
}
