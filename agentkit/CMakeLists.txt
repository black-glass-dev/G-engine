# lumen — GUI-free CLI for humans and AI agents (AgentKit).
# M0: only `version` and `help` are wired. Other commands arrive in M3/M10.

add_executable(lumen_cli src/main.cpp)
set_target_properties(lumen_cli PROPERTIES OUTPUT_NAME lumen)
target_link_libraries(lumen_cli PRIVATE lumen::lumen)
