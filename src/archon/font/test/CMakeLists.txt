add_library(FontTest OBJECT
  test_loader.cpp
)

target_link_libraries(FontTest
  Core
  Check
  Font
)
