add_library(UtilTest OBJECT
  test_kdtree.cpp
  test_unit_frac.cpp
  test_color_space.cpp
  test_css_color.cpp
  test_pixel_box.cpp
  test_rectangle_packer.cpp
)

target_link_libraries(UtilTest PRIVATE
  Core
  Check
  Util
)
