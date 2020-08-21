add_subdirectory(base)
add_subdirectory(unit_test)

configure_file(config.h.in config.h)

add_executable(Foo
               foo.cpp)
set_target_properties(Foo PROPERTIES
                      OUTPUT_NAME "foo")
target_link_libraries(Foo PUBLIC Base)

add_executable(Test
               base/test/test_type_list.cpp
               base/test/test_super_int.cpp
               base/test/test_integer.cpp
               base/test/test_scope_exit.cpp
               base/test/test_circular_buffer.cpp
               base/test/test_memory_input_stream.cpp
               base/test/test_value_parser.cpp
               base/test/test_quote.cpp
               base/test/test_enum.cpp
               base/test/test_hex_dump.cpp
               base/test/test_base64.cpp
               base/test/test_random.cpp
               base/test/test_file.cpp
               unit_test/test/test_random_seed.cpp
               unit_test/test/test_unit_test.cpp
               test.cpp)
set_target_properties(Test PROPERTIES
                      OUTPUT_NAME "test")
target_link_libraries(Test PUBLIC
                      Base
                      UnitTest)
