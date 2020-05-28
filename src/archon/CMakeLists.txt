add_subdirectory(base)

configure_file(config.h.in config.h)

add_executable(Foo
               foo.cpp)
set_target_properties(Foo PROPERTIES
                      OUTPUT_NAME "foo")
target_link_libraries(Foo PUBLIC Base)
