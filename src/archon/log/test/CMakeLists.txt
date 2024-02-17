add_library(LogTest OBJECT
  test_encoding_logger.cpp
)

target_link_libraries(LogTest
  Core
  Log
  Check
)
