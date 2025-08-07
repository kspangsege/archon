add_executable(CommandlineSubinterfaces
  commandline_subinterfaces.cpp
)

set_target_properties(CommandlineSubinterfaces PROPERTIES
  OUTPUT_NAME "archon-commandline-subinterfaces"
)

target_link_libraries(CommandlineSubinterfaces PRIVATE
  Core
  Cli
)
