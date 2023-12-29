add_executable(RegenFallbackFont
  regen_fallback_font.cpp
)

set_target_properties(RegenFallbackFont PROPERTIES
  OUTPUT_NAME "archon-regen-fallback-font"
)

target_link_libraries(RegenFallbackFont PUBLIC
  Cli
  Font
)
