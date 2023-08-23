add_executable(Test test.cpp)

set_target_properties(Test PROPERTIES OUTPUT_NAME "test")

target_link_libraries(Test PUBLIC
  CoreTest
  CliTest
  CheckTest
  MathTest
  UtilTest
  ImageTest
  FontTest
  Core
  Check
)

add_test(NAME Test
  COMMAND Test --describe-build-env --progress
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
