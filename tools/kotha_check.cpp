// G-Engine — tools/kotha_check.cpp
// Command line checker for Kotha (কথা) scripts: prints how every line was understood.
//   kotha_check file.kotha            (or pipe the script on stdin)
//   kotha_check --game file.kotha     as the game checks it: names of things ("when touching X")
//                                     must be things the game has
#include <cstdio>
#include <cstring>
#include <string>

#include "scene/world.h"
#include "script/compiler.h"

int main(int argc, char** argv) {
    ge::kotha::CompileOptions options;
    const char* path = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--game") == 0) {
            options.knownNames = ge::World::builtinNames();
            options.strictObjects = true;
        } else {
            path = argv[i];
        }
    }

    std::string source;
    std::FILE* f = path != nullptr ? std::fopen(path, "rb") : stdin;
    if (f == nullptr) {
        std::fprintf(stderr, "cannot open %s\n", path);
        return 2;
    }
    char buffer[4096];
    std::size_t n;
    while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0) source.append(buffer, n);
    if (f != stdin) std::fclose(f);

    const ge::kotha::Program program = ge::kotha::compile(source, options);
    for (const ge::kotha::Note& note : program.notes) {
        std::printf("%3d  %s\n     %s\n", note.line, note.bn.c_str(), note.en.c_str());
    }
    for (const ge::kotha::Diagnostic& d : program.errors) {
        std::printf("%3d:%d  ✗ %s\n       ✗ %s\n", d.line, d.column, d.bn.c_str(), d.en.c_str());
    }
    std::printf("%zu rules, %zu instructions, %zu errors\n", program.rules.size(), program.code.size(), program.errors.size());
    return program.ok() ? 0 : 1;
}
