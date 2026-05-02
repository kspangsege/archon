add_library(FontTest OBJECT
  test_fallback.cpp
  test_freetype.cpp
)

target_link_libraries(FontTest PRIVATE
  Core
  Check
  Font
)
