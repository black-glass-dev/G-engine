# The sample game.
add_library(demo_game STATIC demo.cpp)
target_link_libraries(demo_game PUBLIC gengine)

if (GE_PLATFORM STREQUAL "web")
    # One WebAssembly module: engine + sample game. web/engine.js loads it.
    add_executable(g-engine ${CMAKE_SOURCE_DIR}/engine/platform/web/main_web.cpp)
    target_link_libraries(g-engine PRIVATE demo_game)
    set_target_properties(g-engine PROPERTIES SUFFIX ".wasm")
    target_link_options(g-engine PRIVATE
        -mexec-model=reactor          # a library with exported functions, not a program with main()
        -Wl,-z,stack-size=1048576
        -Wl,--strip-debug
    )
endif()

if (GE_PLATFORM STREQUAL "android")
    # One shared library (libmain.so) that NativeActivity loads; see android/AndroidManifest.xml.
    add_library(native_app_glue STATIC ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
    target_include_directories(native_app_glue PUBLIC ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)

    add_library(g-engine SHARED ${CMAKE_SOURCE_DIR}/engine/platform/android/main_android.cpp)
    set_target_properties(g-engine PROPERTIES OUTPUT_NAME "main")
    target_link_libraries(g-engine PRIVATE demo_game native_app_glue android log EGL GLESv3)
    target_link_options(g-engine PRIVATE
        -u ANativeActivity_onCreate    # keep the NativeActivity entry point from the glue archive
        -Wl,-z,max-page-size=16384     # 16 KB page-size devices (newer phones)
    )
endif()
