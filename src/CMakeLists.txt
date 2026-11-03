set(ARCHON_BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}")
set(ARCHON_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")


if(ARCHON_INCLUDE_TEST_SUITE)
  add_executable(Test test.cpp)
  set_target_properties(Test PROPERTIES OUTPUT_NAME "test")
  target_link_libraries(Test
    PRIVATE Core
    PRIVATE Check
  )
  add_test(NAME Test
    COMMAND Test --describe-build-env --progress
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
endif()


if(ARCHON_INCLUDE_LIBRARY_Core)
  include(core_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Log)
  include(log_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Cli)
  include(cli_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Check)
  include(check_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Math)
  include(math_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Util)
  include(util_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Image)
  include(image_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Font)
  include(font_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Gfx)
  include(gfx_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Display)
  include(display_library.cmake)
endif()

if(ARCHON_INCLUDE_LIBRARY_Render)
  include(render_library.cmake)
endif()
