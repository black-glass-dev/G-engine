# liblumen — core engine library (renderer, lighting, physics, ECS, VM, audio ...)
# M0: only version + tagged logging exist. Other subsystems arrive in later milestones.

add_library(lumen
  src/log.cpp
  src/version.cpp
)

add_library(lumen::lumen ALIAS lumen)

target_include_directories(lumen PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(lumen PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(lumen PRIVATE /W4 /permissive-)
else()
  target_compile_options(lumen PRIVATE -Wall -Wextra -Wpedantic)
endif()
