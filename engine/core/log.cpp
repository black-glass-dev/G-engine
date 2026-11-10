// G-Engine — core/log.cpp
#include "core/log.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace ge {

namespace {

void defaultSink(LogLevel level, const char* message, void*) {
#if defined(__ANDROID__)
    static const int prio[] = {ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR};
    __android_log_print(prio[static_cast<int>(level)], "GEngine", "%s", message);
#else
    static const char* const tag[] = {"debug", "info", "warn", "error"};
    FILE* out = (level >= LogLevel::Warn) ? stderr : stdout;
    std::fprintf(out, "[ge:%s] %s\n", tag[static_cast<int>(level)], message);
    std::fflush(out);
#endif
}

LogSink g_sink = defaultSink;
void* g_sinkUser = nullptr;

}  // namespace

void setLogSink(LogSink sink, void* user) {
    g_sink = sink ? sink : defaultSink;
    g_sinkUser = sink ? user : nullptr;
}

void logMessage(LogLevel level, const char* fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    g_sink(level, buffer, g_sinkUser);
}

void assertFail(const char* expr, const char* file, int line, const char* msg) {
    logMessage(LogLevel::Error, "ASSERT FAILED: %s (%s:%d)%s%s", expr, file, line, msg ? " — " : "", msg ? msg : "");
    std::abort();
}

}  // namespace ge
