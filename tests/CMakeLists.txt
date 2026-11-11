include(FetchContent)

# doctest (MIT) — header-only test framework, chosen for its light footprint.
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
  GIT_SHALLOW TRUE
)
# doctest 2.4.x declares cmake_minimum_required(2.8)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(doctest)

add_executable(lumen_tests
  test_main.cpp
  test_log.cpp
  test_version.cpp
  test_shader.cpp
  test_render.cpp
)
target_link_libraries(lumen_tests PRIVATE lumen::lumen doctest::doctest)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(lumen_tests)

# CLI and player smoke tests that do not need a display.
if(TARGET lumen_cli)
  add_test(NAME cli_version COMMAND lumen_cli version)
  set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "Lumen2D 0\\.1\\.0")
  add_test(NAME cli_unknown_command_fails COMMAND lumen_cli definitely-not-a-command)
  set_tests_properties(cli_unknown_command_fails PROPERTIES WILL_FAIL TRUE)
endif()

if(TARGET lumen_player AND NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
  add_test(NAME player_rejects_unknown_argument COMMAND lumen_player --no-such-flag)
  set_tests_properties(player_rejects_unknown_argument PROPERTIES WILL_FAIL TRUE)
endif()
