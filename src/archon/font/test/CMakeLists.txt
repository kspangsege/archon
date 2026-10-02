add_library(FontTest OBJECT
  test_fallback.cpp
  test_freetype.cpp
)

target_link_libraries(FontTest
  PRIVATE Core
  PRIVATE Check
  PRIVATE Math
  PRIVATE Util
  PRIVATE Image
  PRIVATE Font
)

target_link_libraries(Test PRIVATE FontTest)
