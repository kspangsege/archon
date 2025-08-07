add_executable(ObjectViewer
  object_viewer.cpp
)

set_target_properties(ObjectViewer PROPERTIES
  OUTPUT_NAME "archon-object-viewer"
)

target_link_libraries(ObjectViewer PRIVATE
  Core
  Log
  Cli
  Math
  Gfx
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
  Gfx
  Display
  Render
)
