add_library(Cli
            command_line.cpp)

set_target_properties(Cli PROPERTIES
                      OUTPUT_NAME "archon-cli")

target_link_libraries(Cli PUBLIC
                      Base)
