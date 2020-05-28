add_library(CheckTest OBJECT
  test_random_seed.cpp
  test_check.cpp
)

target_link_libraries(CheckTest
  Core
  Check
)
