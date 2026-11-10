// G-Engine — script/compiler.h
// Turns Kotha (কথা) source text — natural Bengali and/or English sentences — into a Program.
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "script/program.h"

namespace ge::kotha {

struct CompileOptions {
    /// Names of things in the scene (objects, tags). Lets "কয়েনগুলো" / "coins" resolve to "কয়েন" / "coin".
    std::vector<std::string> knownNames;
    /// With a scene at hand: a name used for a thing ("when touching X", "remove X") must be one of
    /// `knownNames`. Otherwise a misspelt or unknown word would quietly become the name of
    /// something that never shows up. (Variables and messages stay free to invent.)
    bool strictObjects = false;
};

Program compile(std::string_view source, const CompileOptions& options = {});

/// Notes and errors as JSON for the editor UI:
/// {"ok":true,"rules":3,"notes":[{"line":1,"bn":"…","en":"…"}],"errors":[{"line":2,"col":5,"bn":"…","en":"…"}]}
std::string toJson(const Program& program);

/// The key under which a name (object, tag, variable, message) is known to scripts: spelling
/// variants are folded and well-known English names map to their Bengali twin
/// ("Coin" -> "কয়েন", "score" -> "স্কোর"). The engine uses this for the names in a scene.
std::string canonicalName(std::string_view name);

/// Human readable descriptions (used for notes, tests and debugging).
std::string describe(const Trigger& trigger, bool bengali);
std::string describe(const Action& action, bool bengali);

}  // namespace ge::kotha
