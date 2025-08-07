add_library(DisplayTest OBJECT
  test_noinst_timestamp_unwrapper.cpp
)

target_link_libraries(DisplayTest PRIVATE
  Core
  Check
)
