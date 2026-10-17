add_executable(RenderText
  render_text.cpp
)

set_target_properties(RenderText PROPERTIES
  OUTPUT_NAME "archon-render-text"
)

target_link_libraries(RenderText
  PRIVATE Core
  PRIVATE Log
  PRIVATE Cli
  PRIVATE Math
  PRIVATE Util
  PRIVATE Image
  PRIVATE Font
)
