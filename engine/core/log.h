// G-Engine — core/log.h
// Tiny printf-style logger. The sink can be replaced by the platform layer.
#pragma once

#include "core/base.h"

namespace ge {

enum class LogLevel : u8 { Debug, Info, Warn, Error };

using LogSink = void (*)(LogLevel level, const char* message, void* user);

/// Replace the default sink (stdout/stderr). Pass nullptr to restore the default.
void setLogSink(LogSink sink, void* user);

void logMessage(LogLevel level, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

}  // namespace ge

#define GE_LOG_DEBUG(...) ::ge::logMessage(::ge::LogLevel::Debug, __VA_ARGS__)
#define GE_LOG_INFO(...) ::ge::logMessage(::ge::LogLevel::Info, __VA_ARGS__)
#define GE_LOG_WARN(...) ::ge::logMessage(::ge::LogLevel::Warn, __VA_ARGS__)
#define GE_LOG_ERROR(...) ::ge::logMessage(::ge::LogLevel::Error, __VA_ARGS__)
