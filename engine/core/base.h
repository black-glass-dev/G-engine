// G-Engine — core/base.h
// Basic types, assertions and small helpers shared by the whole engine.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ge {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

/// Called when a GE_ASSERT fails. Logs and aborts.
[[noreturn]] void assertFail(const char* expr, const char* file, int line, const char* msg);

}  // namespace ge

#define GE_UNUSED(x) ((void)(x))
#define GE_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#if !defined(NDEBUG) || defined(GE_ENABLE_ASSERTS)
#define GE_ASSERT(cond)                                                       \
    do {                                                                      \
        if (!(cond)) ::ge::assertFail(#cond, __FILE__, __LINE__, nullptr);    \
    } while (0)
#define GE_ASSERT_MSG(cond, msg)                                              \
    do {                                                                      \
        if (!(cond)) ::ge::assertFail(#cond, __FILE__, __LINE__, (msg));      \
    } while (0)
#else
#define GE_ASSERT(cond) ((void)0)
#define GE_ASSERT_MSG(cond, msg) ((void)0)
#endif

// Symbol export for the WebAssembly build: makes a C function callable from JavaScript.
#if defined(__wasm__)
#define GE_EXPORT(name) extern "C" __attribute__((export_name(#name), used))
#define GE_IMPORT(module, name) __attribute__((import_module(#module), import_name(#name)))
#else
#define GE_EXPORT(name) extern "C" __attribute__((used))
#define GE_IMPORT(module, name)
#endif
