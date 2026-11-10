# Unit tests (run on a PC / in CI, no GPU needed).
add_executable(ge_tests
    test_main.cpp
    test_core.cpp
    test_lexicon.cpp
    test_kotha.cpp
    test_game.cpp
    test_docs.cpp
)
target_link_libraries(ge_tests PRIVATE demo_game)
target_include_directories(ge_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# test_docs.cpp reads the Markdown files of the repository.
target_compile_definitions(ge_tests PRIVATE GE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME ge_tests COMMAND ge_tests)

# Command line tool: shows how a Kotha script is understood.
add_executable(kotha_check ${CMAKE_SOURCE_DIR}/tools/kotha_check.cpp)
target_link_libraries(kotha_check PRIVATE gengine)

# The agent edition: build, check and test games from the command line (no screen).
add_executable(ge-game ${CMAKE_SOURCE_DIR}/tools/game_cli.cpp)
target_link_libraries(ge-game PRIVATE gengine)
set_target_properties(ge-game PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
