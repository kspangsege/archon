add_subdirectory(core)
add_subdirectory(log)
add_subdirectory(cli)
add_subdirectory(check)

configure_file(impl/config.h.in impl/config.h)

add_executable(Test test.cpp)

set_target_properties(Test PROPERTIES OUTPUT_NAME "test")

target_link_libraries(Test PUBLIC
  CoreTest
  CliTest
  CheckTest
  Core
  Check
)

add_test(NAME Test
  COMMAND Test --describe-build-env --progress
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
