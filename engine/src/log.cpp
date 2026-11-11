#include "lumen/log.hpp"

#include <cstdio>

namespace lumen {
namespace {

LogSink g_sink = nullptr;
void* g_sink_user = nullptr;

const char* level_name(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info:  return "info";
        case LogLevel::Warn:  return "warn";
        case LogLevel::Error: return "error";
    }
    return "?";
}

}  // namespace

void set_log_sink(LogSink sink, void* user) noexcept {
    g_sink = sink;
    g_sink_user = user;
}

void log(LogLevel level, std::string_view tag, std::string_view message) {
    if (g_sink) {
        g_sink(level, tag, message, g_sink_user);
        return;
    }
    FILE* out = (level == LogLevel::Warn || level == LogLevel::Error) ? stderr : stdout;
    std::fprintf(out, "[%.*s] %s: %.*s\n",
                 static_cast<int>(tag.size()), tag.data(),
                 level_name(level),
                 static_cast<int>(message.size()), message.data());
}

}  // namespace lumen
