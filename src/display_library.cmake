find_package(X11)
find_package(SDL2)

set(ARCHON_DISPLAY_HAVE_XLIB 0)
if (X11_FOUND)
  set(ARCHON_DISPLAY_HAVE_XLIB 1)
endif()

set(ARCHON_DISPLAY_HAVE_SDL 0)
if (SDL2_FOUND)
  set(ARCHON_DISPLAY_HAVE_SDL 1)
endif()

add_subdirectory(archon/display/probe)

add_library(Display
  archon/display/event_handler.cpp
  archon/display/connection.cpp
  archon/display/implementation.cpp
  archon/display/implementation_sdl.cpp
)

set_target_properties(Display PROPERTIES OUTPUT_NAME "archon-display")

target_link_libraries(Display PUBLIC
  Core
  Image
)

if (X11_FOUND)
  target_include_directories(Display PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(Display PRIVATE ${X11_LIBRARIES})
endif()

if (SDL2_FOUND)
  target_include_directories(Display PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(Display PRIVATE ${SDL2_LIBRARIES})
endif()

configure_file(archon/display/impl/config.h.in archon/display/impl/config.h)

target_sources(Display PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" "${ARCHON_SOURCE_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/display/impl/config.h"
  archon/display/types.hpp
  archon/display/texture.hpp
  archon/display/window.hpp
  archon/display/event.hpp
  archon/display/keysyms.hpp
  archon/display/event_handler.hpp
  archon/display/mandates.hpp
  archon/display/connection.hpp
  archon/display/implementation.hpp
  archon/display/implementation_sdl.hpp
  archon/display.hpp
)

install(TARGETS Display FILE_SET HEADERS)

add_subdirectory(archon/display/demo)
