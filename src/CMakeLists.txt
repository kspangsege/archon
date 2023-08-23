set(ARCHON_BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}")
set(ARCHON_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

include(core.cmake)
include(log.cmake)
include(cli.cmake)
include(check.cmake)
include(math.cmake)
include(util.cmake)
include(image.cmake)
include(font.cmake)

add_subdirectory(archon)
