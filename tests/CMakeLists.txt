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
)
target_link_libraries(lumen_tests PRIVATE lumen::lumen doctest::doctest)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(lumen_tests)
