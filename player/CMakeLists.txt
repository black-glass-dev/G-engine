# lumen_player: standalone player.
# Desktop: executable `lumen_player`. Android: shared library `lumen_player` loaded by SDLActivity.
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  add_library(lumen_player SHARED main.cpp)
else()
  add_executable(lumen_player main.cpp)
endif()
target_link_libraries(lumen_player PRIVATE lumen::lumen)
