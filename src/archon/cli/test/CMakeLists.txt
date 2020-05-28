add_library(CliTest OBJECT
  test_processor.cpp
  test_basics.cpp
  test_pattern_forms.cpp
  test_options.cpp
  test_delegation.cpp
)

target_link_libraries(CliTest
  Core
  Cli
  Check
)
