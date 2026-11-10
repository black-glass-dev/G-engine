# Third-party libraries, vendored as source so the engine builds offline on every platform.

# ---- Box2D v3.2.0 (2D rigid-body physics, MIT) -------------------------------------------
file(GLOB BOX2D_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/box2d/src/*.c)
add_library(box2d STATIC ${BOX2D_SOURCES})
target_include_directories(box2d
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/box2d/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/box2d/src)
set_target_properties(box2d PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
    POSITION_INDEPENDENT_CODE ON)
# Same result on every CPU (phone, PC, browser): no fused multiply-add contraction.
if (NOT MSVC)
    target_compile_options(box2d PRIVATE -ffp-contract=off -w)
endif()
if (UNIX AND NOT ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
    find_package(Threads REQUIRED)
    target_link_libraries(box2d PUBLIC Threads::Threads m)
endif()
