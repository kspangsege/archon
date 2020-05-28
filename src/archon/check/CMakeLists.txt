add_subdirectory(test)

add_library(Check
  random_seed.cpp
  test_context.cpp
  test_trail.cpp
  reporter.cpp
  test_list.cpp
  noinst/test_level_report_logger.cpp
  noinst/root_context_impl.cpp
  noinst/thread_context_impl.cpp
  test_runner.cpp
  wildcard_filter.cpp
  pattern_based_test_order.cpp
  simple_reporter.cpp
  standard_path_mapper.cpp
  duplicating_reporter.cpp
  xml_reporter.cpp
  check_no_error.cpp
  command.cpp
)

set_target_properties(Check PROPERTIES OUTPUT_NAME "archon-check")

target_link_libraries(Check PUBLIC
  Core
  Log
  Cli
)

install(TARGETS Check)
