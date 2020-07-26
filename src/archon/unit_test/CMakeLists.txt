add_library(UnitTest
            random_seed.cpp
            test_context.cpp
            reporter.cpp
            test_list.cpp
            noinst/test_level_report_logger.cpp
            noinst/root_context_impl.cpp
            noinst/thread_context_impl.cpp
            run.cpp
            wildcard_filter.cpp
            pattern_based_test_order.cpp
            simple_reporter.cpp
            standard_path_mapper.cpp
            duplicating_reporter.cpp
            xml_reporter.cpp)

set_target_properties(UnitTest PROPERTIES
                      OUTPUT_NAME "archon-unit-test")

target_link_libraries(UnitTest PUBLIC
                      Base)
