#pragma once

#include <string_view>

namespace lumen {

// Engine version. Keep in sync with the project() call in the root CMakeLists.txt.
inline constexpr int kVersionMajor = 0;
inline constexpr int kVersionMinor = 1;
inline constexpr int kVersionPatch = 0;

// Returns "Lumen2D 0.1.0" style string.
std::string_view engine_version_string() noexcept;

}  // namespace lumen
