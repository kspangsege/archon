set(ARCHON_BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}")
set(ARCHON_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

include(core_library.cmake)
include(log_library.cmake)
include(cli_library.cmake)
include(check_library.cmake)
include(math_library.cmake)
include(util_library.cmake)
include(image_library.cmake)
include(font_library.cmake)
include(display_library.cmake)

add_executable(Test test.cpp)

set_target_properties(Test PROPERTIES OUTPUT_NAME "test")

target_link_libraries(Test PUBLIC
  Core
  Check
  CoreTest
  CliTest
  CheckTest
  MathTest
  UtilTest
  ImageTest
  FontTest
)

add_test(NAME Test
  COMMAND Test --describe-build-env --progress
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
