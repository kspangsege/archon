add_executable(FormatText
  format_text.cpp
)

set_target_properties(FormatText PROPERTIES
  OUTPUT_NAME "archon-format-text"
)

target_link_libraries(FormatText PUBLIC
  Core
  Cli
)
