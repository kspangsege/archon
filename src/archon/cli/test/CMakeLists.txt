add_library(CliTest OBJECT
  test_processor.cpp
  test_basics.cpp
  test_pattern_forms.cpp
  test_options.cpp
  test_delegation.cpp
)

target_link_libraries(CliTest
  PRIVATE Log
  PRIVATE Cli
  PRIVATE Check
)

target_link_libraries(Test PRIVATE CliTest)
