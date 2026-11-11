# liblumen — core engine library (renderer, lighting, physics, ECS, VM, audio ...)
# M1: version, tagged logging, GL 3.3 / GLES 3.0 loader, shader pipeline, SDL3 window loop.

add_library(lumen
  src/log.cpp
  src/version.cpp
  src/render/gl_api.cpp
  src/render/shader.cpp
  src/render/renderer.cpp
  src/platform/app.cpp
)

add_library(lumen::lumen ALIAS lumen)

target_include_directories(lumen PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(lumen PUBLIC cxx_std_20)

# SDL3 is public because the player includes SDL_main.h.
target_link_libraries(lumen PUBLIC SDL3::SDL3)

if(MSVC)
  target_compile_options(lumen PRIVATE /W4 /permissive-)
else()
  target_compile_options(lumen PRIVATE -Wall -Wextra -Wpedantic)
endif()
