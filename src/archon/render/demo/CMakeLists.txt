add_executable(Box
  box.cpp
)

set_target_properties(Box PROPERTIES
  OUTPUT_NAME "archon-box"
)

target_link_libraries(Box PRIVATE
  Core
  Log
  Cli
  Math
  Display
  Render
)
