add_library(ImageTest OBJECT
  test_iter.cpp
  comp_repr_utils.cpp
  test_integer_pixel_format.cpp
  test_subword_pixel_format.cpp
  test_packed_pixel_format.cpp
  test_indexed_pixel_format.cpp
  test_reader.cpp
  test_writer.cpp
)

target_link_libraries(ImageTest
  Core
  Check
  Image
)
