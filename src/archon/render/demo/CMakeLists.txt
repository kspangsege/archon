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


add_executable(Blocks
  blocks.cpp
)

set_target_properties(Blocks PROPERTIES
  OUTPUT_NAME "archon-blocks"
)

target_link_libraries(Blocks PRIVATE
  Core
  Log
  Cli
  Math
  Display
  Render
)
