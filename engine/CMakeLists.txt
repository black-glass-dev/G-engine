# G-Engine core library.

set(GE_CORE_SOURCES
    core/log.cpp
    core/utf8.cpp
)

set(GE_SCRIPT_SOURCES
    script/text.cpp
    script/lexicon.cpp
    script/compiler.cpp
    script/vm.cpp
)

set(GE_PHYSICS_SOURCES
    physics/physics.cpp
)

set(GE_GFX_SOURCES
    gfx/renderer.cpp
    gfx/atlas_gen.cpp
)

set(GE_GAME_SOURCES
    scene/world.cpp
    app/app.cpp
)

# The graphics backend depends on the platform (see gfx/gl.h):
#   web      -> the gx* functions are imported from JavaScript (WebGL2), nothing to compile
#   headless -> do-nothing stubs, so everything runs on a PC without a GPU (tests)
#   android  -> OpenGL ES 3 wrappers (roadmap phase 2)
set(GE_PLATFORM_SOURCES)
if (GE_PLATFORM STREQUAL "headless")
    list(APPEND GE_PLATFORM_SOURCES gfx/gl_null.cpp)
elseif (GE_PLATFORM STREQUAL "android")
    list(APPEND GE_PLATFORM_SOURCES gfx/gl_gles3.cpp)
endif()

add_library(gengine STATIC
    ${GE_CORE_SOURCES}
    ${GE_SCRIPT_SOURCES}
    ${GE_PHYSICS_SOURCES}
    ${GE_GFX_SOURCES}
    ${GE_GAME_SOURCES}
    ${GE_PLATFORM_SOURCES}
)

target_include_directories(gengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gengine PUBLIC box2d)
target_compile_definitions(gengine PUBLIC GE_PLATFORM_$<UPPER_CASE:${GE_PLATFORM}>=1)

if (NOT MSVC)
    target_compile_options(gengine PRIVATE -Wall -Wextra -Wshadow)
    # The engine does not use C++ exceptions or RTTI (smaller and faster on phones and in WebAssembly).
    target_compile_options(gengine PUBLIC -fno-exceptions -fno-rtti)
endif()
