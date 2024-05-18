add_library(Check
  archon/check/random_seed.cpp
  archon/check/test_context.cpp
  archon/check/test_trail.cpp
  archon/check/reporter.cpp
  archon/check/test_list.cpp
  archon/check/noinst/test_level_report_logger.cpp
  archon/check/noinst/root_context_impl.cpp
  archon/check/noinst/thread_context_impl.cpp
  archon/check/test_runner.cpp
  archon/check/wildcard_filter.cpp
  archon/check/pattern_based_test_order.cpp
  archon/check/simple_reporter.cpp
  archon/check/standard_path_mapper.cpp
  archon/check/duplicating_reporter.cpp
  archon/check/xml_reporter.cpp
  archon/check/check_no_error.cpp
  archon/check/command.cpp
)

set_target_properties(Check PROPERTIES OUTPUT_NAME "archon-check")

target_link_libraries(Check PUBLIC
  Core
  Log
  Cli
)

target_sources(Check PUBLIC FILE_SET HEADERS FILES
  archon/check/seed_seq.hpp
  archon/check/random_seed.hpp
  archon/check/test_details.hpp
  archon/check/check_arg.hpp
  archon/check/root_context.hpp
  archon/check/thread_context.hpp
  archon/check/test_context.hpp
  archon/check/test_list.hpp
  archon/check/fail_context.hpp
  archon/check/reporter.hpp
  archon/check/test_config.hpp
  archon/check/test_runner.hpp
  archon/check/run.hpp
  archon/check/command.hpp
  archon/check/test_macros.hpp
  archon/check/test_batch_macros.hpp
  archon/check/test_trail.hpp
  archon/check/test_path.hpp
  archon/check/check_macros.hpp
  archon/check/check_no_error.hpp
  archon/check/standard_path_mapper.hpp
  archon/check/pattern_based_test_order.hpp
  archon/check/wildcard_filter.hpp
  archon/check/simple_reporter.hpp
  archon/check/duplicating_reporter.hpp
  archon/check/xml_reporter.hpp
  archon/check.hpp
)

install(TARGETS Check FILE_SET HEADERS)

add_subdirectory(archon/check/test)
