#pragma once

#include <string_view>

namespace lumen {

// Tagged logging, e.g. log(LogLevel::Error, "physics", "body overflow").
// Output format: "[tag] message" on stderr (errors/warnings) or stdout (info/debug).
// Errors are never silently swallowed: callers must log or propagate them.
enum class LogLevel { Debug, Info, Warn, Error };

void log(LogLevel level, std::string_view tag, std::string_view message);

// Sink used by tests to capture output instead of writing to the console.
// Pass nullptr to restore console output.
using LogSink = void (*)(LogLevel level, std::string_view tag, std::string_view message, void* user);
void set_log_sink(LogSink sink, void* user) noexcept;

}  // namespace lumen
